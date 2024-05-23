#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#define fnd "/dev/fnd"
#define dot "/dev/dot"
#define tact "/dev/tactsw"
#define led "/dev/led"
#define clcd "/dev/clcd"

// 게임 상수
#define MAX_ROUNDS 2
#define MAX_TRIES 10
#define INIT_SCORE 1000

// 장치 파일 디스크립터
int fnd_fd, dot_fd, tact_fd, led_fd, clcd_fd;
int player_scores[2] = { INIT_SCORE, INIT_SCORE };

// 함수 선언
void initialize_devices();
void close_devices();
void initialize_game();
void show_message(const char* msg);
void clear_clcd();
void show_score(int player);
void show_dot_matrix(const unsigned char* data);
int read_tact_switch();
void blink_led(int led_num);
int get_number_input();
void play_round(int round_num, int num_digits);
int check_guess(const char* guess, const char* answer);
void get_secret_number(char* secret, int num_digits, int player);

// 도트 매트릭스 패턴
unsigned char BASEBALL_ICON[] = { 0x7E, 0x81, 0xA5, 0xDB, 0x81, 0xA5, 0x99, 0x7E };
unsigned char STRIKE_ICON[] = { 0x3C, 0x42, 0x40, 0x3C, 0x02, 0x02, 0x42, 0x3C };
unsigned char OUT_ICON[] = { 0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C };
unsigned char HOME_RUN_ICON[] = { 0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42 };
unsigned char BALL_ICON[] = { 0x1E, 0x22, 0x22, 0x1E, 0x22, 0x22, 0x22, 0x1E };

int main() {
    initialize_devices();
    initialize_game();

    for (int round = 1; round <= MAX_ROUNDS; ++round) {
        play_round(round, round + 2); // 1라운드: 3자리 숫자, 2라운드: 4자리 숫자
    }

    close_devices();
    return 0;
}

void initialize_devices() {
    fnd_fd = open(fnd, O_RDWR);
    dot_fd = open(dot, O_RDWR);
    tact_fd = open(tact, O_RDWR);
    led_fd = open(led, O_RDWR);
    clcd_fd = open(clcd, O_RDWR);

    if (fnd_fd < 0 || dot_fd < 0 || tact_fd < 0 || led_fd < 0 || clcd_fd < 0) {
        printf("장치를 열 수 없습니다\n");
        exit(1);
    }

    clear_clcd();
    show_message("숫자 야구 게임에 오신 것을 환영합니다!");
}

void close_devices() {
    close(fnd_fd);
    close(dot_fd);
    close(tact_fd);
    close(led_fd);
    close(clcd_fd);
}

void initialize_game() {
    // 모든 장치를 초기화하여 기본 상태로 설정
    // CLCD, 도트 매트릭스, FND를 초기화하고 모든 LED를 끔
    clear_clcd();
    show_message("초기화 중...");
    show_dot_matrix(BASEBALL_ICON);
    usleep(1000000);
}

void show_message(const char* msg) {
    clear_clcd();
    write(clcd_fd, msg, strlen(msg));
}

void clear_clcd() {
    char clear_msg[] = "                "; // 16개의 빈 칸
    write(clcd_fd, clear_msg, sizeof(clear_msg) - 1);
}

void show_score(int player) {
    char score_msg[32];
    sprintf(score_msg, "P%d 점수: %d", player + 1, player_scores[player]);
    show_message(score_msg);
}

void show_dot_matrix(const unsigned char* data) {
    write(dot_fd, data, 8);
}

int read_tact_switch() {
    unsigned char b;
    read(tact_fd, &b, sizeof(b));
    return b;
}

void blink_led(int led_num) {
    unsigned char data = (1 << led_num);
    write(led_fd, &data, sizeof(data));
    usleep(500000);
    data = 0;
    write(led_fd, &data, sizeof(data));
}

int get_number_input() {
    int number = 0;
    while (1) {
        int key = read_tact_switch();
        if (key >= 1 && key <= 9) {
            number = number * 10 + key;
            if (number >= 1000) break; // 최대 4자리 숫자 입력
        }
        else if (key == 12) {
            break; // 입력 완료
        }
    }
    return number;
}

void get_secret_number(char* secret, int num_digits, int player) {
    show_message("비밀 번호를 입력하세요:");
    int secret_num = get_number_input();
    snprintf(secret, num_digits + 1, "%0*d", num_digits, secret_num);
    printf("Player %d 비밀 번호: %s\n", player + 1, secret); // 디버그 출력
}

void play_round(int round_num, int num_digits) {
    char secrets[2][5]; // 두 플레이어의 비밀 번호를 저장

    // 각 플레이어가 비밀 번호 설정
    for (int player = 0; player < 2; ++player) {
        get_secret_number(secrets[player], num_digits, player);
    }

    for (int try = 1; try <= MAX_TRIES; ++try) {
        for (int player = 0; player < 2; ++player) {
            show_message("숫자를 입력하세요:");
            int player_guess = get_number_input();
            char guess[5];
            snprintf(guess, sizeof(guess), "%0*d", num_digits, player_guess);

            int strikes, balls;
            int result = check_guess(guess, secrets[1 - player]);

            if (result == num_digits * 10) {
                show_message("홈런!");
                blink_led(3); // 홈런 시 파란색 LED
                player_scores[player] += 100;
                break;
            }
            else {
                // 스트라이크와 볼 계산
                strikes = result / 10;
                balls = result % 10;
                if (strikes > 0) {
                    show_message("스트라이크!");
                    show_dot_matrix(STRIKE_ICON);
                    blink_led(1); // 스트라이크 시 초록색 LED
                }
                else if (balls > 0) {
                    show_message("볼!");
                    show_dot_matrix(BALL_ICON);
                    blink_led(2); // 볼 시 노란색 LED
                }
                else {
                    show_message("아웃!");
                    show_dot_matrix(OUT_ICON);
                    blink_led(0); // 아웃 시 빨간색 LED
                }
                player_scores[player] -= 10;
            }

            show_score(player);
            usleep(2000000); // 다음 시도 전 대기
        }
    }
}

int check_guess(const char* guess, const char* answer) {
    int strikes = 0, balls = 0;
    for (int i = 0; i < strlen(guess); ++i) {
        if (guess[i] == answer[i]) {
            strikes++;
        }
        else if (strchr(answer, guess[i])) {
            balls++;
        }
    }
    return strikes * 10 + balls;
}
