#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

// 디바이스 경로 정의
#define fnd "/dev/fnd"
#define dot "/dev/dot"
#define tact "/dev/tactsw"
#define led "/dev/led"
#define dip "/dev/dipsw"
#define clcd "/dev/clcd"

// 함수 선언부
void print_clcd(const char* message);
void writeToDotDevice(unsigned char* data, int time);
int tactsw_get_with_timer(int t_second);
int dipsw_get_with_timer(int t_second);
void led_on(int strikes, int balls, int outs, int homerun);
void init_devices();
void game_rule(int round);
void start_game();
void input_number(char* number, int digits);
void check_guess(const char* guess, const char* secret, int length, int* strikes, int* balls, int* outs);
void display_score(int player1_score, int player2_score);
void print_game_start();
void print_round_start(int round);
void print_result(int strikes, int balls, int outs);
void print_final_score(int player1_score, int player2_score);
void print_game_over();
void print_winner(int player1_score, int player2_score);
void print_homerun();
int intro();
int is_valid_number(const char* number, int length);
void blink_fnd();
void blink_led();

// 전역 변수
int dipsw;
int leds;
int dot_mtx;
int tactsw;
int clcds;
int fnds;
unsigned char fnd_data[4];

// Dot Matrix 패턴
unsigned char patterns[4][8] = {
    {0x3C, 0x42, 0x40, 0x3C, 0x02, 0x02, 0x42, 0x3C}, // STRIKE
    {0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C}, // OUT
    {0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42}, // HOME RUN
    {0x1E, 0x22, 0x22, 0x1E, 0x22, 0x22, 0x22, 0x1E}  // BALL
};

// Character LCD 함수
void print_clcd(const char* message) {
    clcds = open(clcd, O_RDWR);
    if (clcds < 0) {
        printf("Character LCD 열기 실패.\n");
        exit(0);
    }
    write(clcds, message, strlen(message));
    close(clcds);
}

void print_game_start() {
    print_clcd("Game Start!");
    usleep(2000000);  // 2초 대기
}

void print_round_start(int round) {
    char buffer[32];
    sprintf(buffer, "Round %d Start!", round);
    print_clcd(buffer);
    usleep(2000000);  // 2초 대기
}

void print_result(int strikes, int balls, int outs) {
    char buffer[32];
    sprintf(buffer, "S:%d B:%d O:%d", strikes, balls, outs);
    print_clcd(buffer);
    usleep(2000000);  // 2초 대기
}

void print_homerun() {
    print_clcd("HOMERUN!!!");
    usleep(1500000);  // 1.5초 대기
    unsigned char h_pattern[8] = { 0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42 }; // H 패턴
    writeToDotDevice(h_pattern, 1500000); // Dot Matrix에 H 패턴 1.5초 표시
}

void print_final_score(int player1_score, int player2_score) {
    char buffer[32];
    sprintf(buffer, "P1: %d P2: %d", player1_score, player2_score);
    print_clcd(buffer);
    usleep(2000000);  // 2초 대기
}

void print_game_over() {
    print_clcd("Game Over");
    usleep(2000000);  // 2초 대기
}

void print_winner(int player1_score, int player2_score) {
    if (player1_score > player2_score) {
        print_clcd("P1 Wins!");
    }
    else if (player1_score < player2_score) {
        print_clcd("P2 Wins!");
    }
    else {
        print_clcd("It's a Draw!");
    }
    usleep(2000000);  // 2초 대기
}

// Dot Matrix 함수
void writeToDotDevice(unsigned char* data, int time) {
    dot_mtx = open(dot, O_RDWR);
    if (dot_mtx < 0) {
        printf("Dot 디바이스 열기 실패\n");
        exit(0);
    }
    write(dot_mtx, data, 8);
    usleep(time);
    close(dot_mtx);
}

// TACT Switch 함수
int tactsw_get_with_timer(int t_second) {
    int tactswFd, selected_tact = 0;
    unsigned char b = 0;

    tactswFd = open(tact, O_RDONLY);
    if (tactswFd < 0) {
        perror("TACT 스위치 디바이스 열기 실패");
        return -1;
    }

    while (t_second > 0) {
        usleep(100000); // 100ms 대기
        read(tactswFd, &b, sizeof(b));
        if (b) {
            selected_tact = b;
            close(tactswFd);
            return selected_tact;
        }
        t_second -= 0.1;
    }

    close(tactswFd);
    return 0; // 타임아웃
}

// DIP Switch 함수
int dipsw_get_with_timer(int t_second) {
    int dipswFd, selected_dip = 0;
    unsigned char d = 0;

    dipswFd = open(dip, O_RDONLY);
    if (dipswFd < 0) {
        perror("DIP 스위치 디바이스 열기 실패");
        return -1;
    }

    while (t_second > 0) {
        usleep(100000); // 100ms 대기
        read(dipswFd, &d, sizeof(d));
        if (d) {
            selected_dip = d;
            close(dipswFd);
            return selected_dip;
        }
        t_second -= 0.1;
    }

    close(dipswFd);
    return 0; // 타임아웃
}

// LED 제어 함수
void led_on(int strikes, int balls, int outs, int homerun) {
    unsigned char led_data = 0;
    if (strikes > 0) led_data |= 0x22; // Green LEDs
    if (balls > 0) led_data |= 0x44;   // Yellow LEDs
    if (outs > 0) led_data |= 0x11;    // Red LEDs
    if (homerun > 0) led_data |= 0x88; // Blue LEDs
    leds = open(led, O_RDWR);
    if (leds < 0) {
        printf("LED 열기 실패.\n");
        exit(0);
    }
    write(leds, &led_data, sizeof(unsigned char));
    close(leds);
}

// 디바이스 초기화 함수
void init_devices() {
    dipsw = open(dip, O_RDWR);
    leds = open(led, O_RDWR);
    dot_mtx = open(dot, O_RDWR);
    tactsw = open(tact, O_RDWR);
    clcds = open(clcd, O_RDWR);
    fnds = open(fnd, O_RDWR);
    if (dipsw < 0 || leds < 0 || dot_mtx < 0 || tactsw < 0 || clcds < 0 || fnds < 0) {
        printf("디바이스 열기 실패\n");
        exit(0);
    }
    close(dipsw);
    close(leds);
    close(dot_mtx);
    close(tactsw);
    close(clcds);
    close(fnds);
}

// 게임 규칙 출력 함수
void game_rule(int round) {
    if (round == 1) {
        print_clcd("Round 1, 3 digits");
    }
    else if (round == 2) {
        print_clcd("Round 2, 4 digits");
    }
    usleep(3000000);
}

// 숫자 유효성 검사 함수
int is_valid_number(const char* number, int length) {
    if (strlen(number) != length) return 0; // 지정된 길이가 아님
    for (int i = 0; i < length; i++) {
        for (int j = i + 1; j < length; j++) {
            if (number[i] == number[j]) {
                return 0; // 중복된 숫자 발견
            }
        }
    }
    return 1; // 유효한 입력
}

// 사용자 추측 검사 및 스트라이크, 볼 계산
void check_guess(const char* guess, const char* secret, int length, int* strikes, int* balls, int* outs) {
    *strikes = 0;
    *balls = 0;
    *outs = 0;

    for (int i = 0; i < length; i++) {
        int is_out = 1; // 기본값을 아웃으로 설정

        if (guess[i] == secret[i]) {
            (*strikes)++;
            is_out = 0;
        }
        else {
            for (int j = 0; j < length; j++) {
                if (guess[i] == secret[j]) {
                    (*balls)++;
                    is_out = 0;
                    break;
                }
            }
        }

        if (is_out) {
            (*outs)++;
        }
    }

    // 모든 숫자가 아웃인 경우 outs가 length와 같아짐
    if (*outs != length) {
        *outs = 0; // 모든 숫자가 아웃이 아닌 경우 outs를 0으로 설정
    }
    else {
        *strikes = 0;
        *balls = 0;
    }
}

// FND 출력 함수
void PrintFnd(int* nums, int count) {
    unsigned char fnd_codes[10] = { 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xD8, 0x80, 0x98 };

    // FND 데이터를 초기화합니다.
    fnd_data[0] = 0xFF;
    fnd_data[1] = 0xFF;
    fnd_data[2] = 0xFF;
    fnd_data[3] = 0xFF;

    // 입력된 숫자를 FND에 표시합니다.
    for (int i = 0; i < count; i++) {
        fnd_data[i] = fnd_codes[nums[i]];
    }
}




// 점수 표시 함수
void display_score(int player1_score, int player2_score) {
    char score[32];
    sprintf(score, "P1: %d, P2: %d", player1_score, player2_score);
    print_clcd(score);
    usleep(5000000); // 5초 대기
}

// 숫자 입력 함수
void input_number(char* number, int digits) {
    int tactsw_value;
    int idx = 0;

    int nums[4] = { 0 }; // 정수 배열 초기화

    while (idx < digits) {
        tactsw_value = tactsw_get_with_timer(100);
        if (tactsw_value >= 1 && tactsw_value <= 9) {
            number[idx] = '0' + tactsw_value;
            nums[idx] = tactsw_value; // 정수 배열에 값 저장
            idx++;
            PrintFnd(nums, idx); // 정수 배열과 현재 인덱스 전달
            int fndFd = open(fnd, O_RDWR);
            if (fndFd < 0) {
                perror("FND 디바이스 열기 실패");
                return;
            }
            write(fndFd, &fnd_data, sizeof(fnd_data));
            close(fndFd);
            printf("input_number: TACT switch value %d, Current number: %s\n", tactsw_value, number);
        }
    }
    number[digits] = '\0';
}

// FND Blink 함수
void blink_fnd() {
    unsigned char fnd_codes[10] = { 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xD8, 0x80, 0x98 };
    unsigned char fnd_blink_data[4] = { fnd_codes[8], fnd_codes[8], fnd_codes[8], fnd_codes[8] };
    unsigned char fnd_clear_data[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

    int fndFd = open(fnd, O_RDWR);
    if (fndFd < 0) {
        perror("FND 디바이스 열기 실패");
        return;
    }

    for (int i = 0; i < 5; i++) {
        write(fndFd, &fnd_blink_data, sizeof(fnd_blink_data));
        usleep(200000); // 0.2초 대기
        write(fndFd, &fnd_clear_data, sizeof(fnd_clear_data));
        usleep(200000); // 0.2초 대기
    }
    close(fndFd);
}

// LED Blink 함수
void blink_led() {
    unsigned char led_on_data = 0xFF;
    unsigned char led_off_data = 0x00;

    int ledFd = open(led, O_RDWR);
    if (ledFd < 0) {
        perror("LED 디바이스 열기 실패");
        return;
    }

    for (int i = 0; i < 5; i++) {
        write(ledFd, &led_on_data, sizeof(unsigned char));
        usleep(200000); // 0.2초 대기
        write(ledFd, &led_off_data, sizeof(unsigned char));
        usleep(200000); // 0.2초 대기
    }
    close(ledFd);
}

// 게임 시작 함수
void start_game() {
    char secret_number1[5]; // 플레이어 1의 비밀 숫자 저장 (최대 4자리)
    char secret_number2[5]; // 플레이어 2의 비밀 숫자 저장
    char guess[5];          // 추측 저장 (최대 4자리)
    int strikes, balls, outs;
    int turn = 1;
    int score1 = 1000, score2 = 1000; // 게임 시작 점수
    int rounds = 2;                   // 총 라운드 수
    int digits[2] = { 3, 4 };         // 각 라운드의 자릿수

    for (int round = 0; round < rounds; round++) {
        int current_digits = digits[round];
        game_rule(round + 1);

        // 플레이어 1과 2의 비밀 숫자 설정
        for (int player = 1; player <= 2; player++) {
            while (1) {
                print_clcd(player == 1 ? "P1 Set Number:" : "P2 Set Number:");
                input_number(player == 1 ? secret_number1 : secret_number2, current_digits);
                if (!is_valid_number(player == 1 ? secret_number1 : secret_number2, current_digits)) {
                    print_clcd("Invalid Number!");
                    usleep(2000000); // 2초 대기
                }
                else {
                    break; // 유효한 입력
                }
            }
        }

        // 게임 진행
        int home_run1 = 0, home_run2 = 0;
        while (home_run1 == 0 || home_run2 == 0) {
            if (home_run1 == 1 && home_run2 == 1) {
                break;
            }
            if (turn == 1 && home_run1 == 1) {
                turn = 2; // 홈런을 친 플레이어는 건너뛰기
                continue;
            }
            if (turn == 2 && home_run2 == 1) {
                turn = 1; // 홈런을 친 플레이어는 건너뛰기
                continue;
            }

            time_t start_time = time(NULL);

            while (1) {
                print_clcd(turn == 1 ? "P1 Guess:" : "P2 Guess:");
                input_number(guess, current_digits);
                if (!is_valid_number(guess, current_digits)) {
                    print_clcd("Invalid Guess!");
                    usleep(2000000); // 2초 대기
                }
                else {
                    break; // 유효한 입력
                }
            }

            time_t end_time = time(NULL);
            int time_diff = difftime(end_time, start_time);

            if (turn == 1) {
                check_guess(guess, secret_number2, current_digits, &strikes, &balls, &outs);
                if (strikes == current_digits) {
                    home_run1 = 1;
                }
                else {
                    score1 -= 10; // 홈런이 아닌 경우 10점 감점
                }
                score1 -= time_diff; // 초당 1점 감점
            }
            else {
                check_guess(guess, secret_number1, current_digits, &strikes, &balls, &outs);
                if (strikes == current_digits) {
                    home_run2 = 1;
                }
                else {
                    score2 -= 10; // 홈런이 아닌 경우 10점 감점
                }
                score2 -= time_diff; // 초당 1점 감점
            }

            print_result(strikes, balls, outs);
            led_on(strikes, balls, outs, (strikes == current_digits));

            if (strikes == current_digits) {
                print_homerun();
            }
            else {
                if (strikes > 0) {
                    writeToDotDevice(patterns[0], 2000000); // 스트라이크 패턴 표시
                }
                if (balls > 0) {
                    writeToDotDevice(patterns[3], 2000000); // 볼 패턴 표시
                }
                if (outs == current_digits) {
                    writeToDotDevice(patterns[1], 2000000); // 아웃 패턴 표시
                }
            }

            // 턴 변경
            turn = turn == 1 ? 2 : 1;
        }

        display_score(score1, score2);
    }

    // 최종 결과 출력
    print_final_score(score1, score2);

    // 승자 출력
    print_winner(score1, score2);

    // 최종 점수 FND 출력
    int final_scores[4] = { score1 / 1000, (score1 / 100) % 10, (score1 / 10) % 10, score1 % 10 };
    PrintFnd(final_scores, 4);
    int fnd_fd = open(fnd, O_RDWR);
    if (fnd_fd < 0) {
        perror("FND 디바이스 열기 실패");
        return;
    }
    write(fnd_fd, fnd_data, sizeof(fnd_data));
    close(fnd_fd);
}


// 게임 소개 함수
int intro() {
    int dip_value = 0;

    print_clcd("DIP UP to Start");
    dip_value = dipsw_get_with_timer(100);  // 10초로 증가

    if (dip_value != 0) {
        blink_fnd();
        blink_led();
    }
    return dip_value;
}

// 메인 함수
int main() {
    init_devices();
    if (intro() != 0) {
        print_game_start();
        start_game();
        print_game_over();
    }
    else {
        print_clcd("Game Not Started");
        usleep(2000000); // 2초 대기
    }
    return 0;
}
