#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#define DEV_LED "/dev/led"
#define DEV_SWITCH "/dev/tactsw"
#define DEV_CLCD "/dev/clcd"
#define DEV_DOT "/dev/dot" 

// LED 정의
#define LED_RED_0    0x01
#define LED_RED_4    0x10
#define LED_YELLOW_1 0x02
#define LED_YELLOW_5 0x20
#define LED_ORANGE_2 0x04
#define LED_ORANGE_6 0x40
#define LED_BLUE_3   0x08
#define LED_BLUE_7   0x80

int fd_led, fd_switch, fd_clcd, fd_dot; // 장치 파일 디스크립터

unsigned char BASEBALL[8][8] = {
    {0x7c, 0x18, 0x18, 0x7e, 0x7c, 0x18, 0x60, 0x60},
    {0x62, 0x3c, 0x3c, 0x7e, 0x62, 0x3c, 0x60, 0x60},
    {0x62, 0x66, 0x66, 0x60, 0x62, 0x66, 0x60, 0x60},
    {0x7c, 0xc3, 0x70, 0x7e, 0x7c, 0xc3, 0x60, 0x60},
    {0x62, 0xff, 0x0e, 0x7e, 0x62, 0xff, 0x60, 0x60},
    {0x62, 0xff, 0x66, 0x60, 0x62, 0xff, 0x7f, 0x60},
    {0x62, 0xc3, 0x3c, 0x7e, 0x62, 0xc3, 0x7f, 0x60},
    {0x7c, 0xc3, 0x18, 0x7e, 0x7c, 0xc3, 0x7f, 0x60}
};

unsigned char BASEBALL_ICON[8] = { 0x3c, 0x42, 0xa5, 0xa5, 0xa5, 0xa5, 0x42, 0x3c };
unsigned char STRIKE[8] = { 0x18, 0x3c, 0x66, 0x70, 0x0e, 0x66, 0x3c, 0x18 };
unsigned char OUT[8] = { 0x3c, 0x7e, 0xc3, 0xc3, 0xc3, 0xc3, 0x7e, 0x3c };
unsigned char HOME_RUN[8] = { 0x66, 0x66, 0x66, 0x7e, 0x7e, 0x66, 0x66, 0x66 };
unsigned char BALL[8] = { 0x3c, 0x44, 0x44, 0x3c, 0x44, 0x44, 0x3c, 0x3c };

// CLCD 초기화
void clcd_init() {
    fd_clcd = open(DEV_CLCD, O_RDWR);
    if (fd_clcd < 0) {
        perror("CLCD 장치 열기 실패");
        exit(1);
    }
    const char clear_screen[] = "\f"; // 화면 지우기 명령
    write(fd_clcd, clear_screen, sizeof(clear_screen));
}

// CLCD 쓰기
void clcd_write(const char* str) {
    write(fd_clcd, str, strlen(str));
}

// Dot Matrix 초기화
void dot_matrix_init() {
    fd_dot = open(DEV_DOT, O_RDWR);
    if (fd_dot < 0) {
        perror("Dot Matrix 장치 열기 실패");
        exit(1);
    }
}

// Dot Matrix 쓰기
void dot_matrix_write(unsigned char* pattern) {
    write(fd_dot, pattern, sizeof(unsigned char) * 8);
}

// Dot Matrix에 "BASEBALL" 표시
void display_baseball() {
    for (int i = 0; i < 8; i++) {
        dot_matrix_write(BASEBALL[i]);
        usleep(500000); // 0.5초 지연
    }
}

// 야구공 아이콘 표시
void display_baseball_icon() {
    dot_matrix_write(BASEBALL_ICON);
}

// Dot Matrix에 상태 표시
void display_status(char status) {
    switch (status) {
    case 'S': dot_matrix_write(STRIKE); break;
    case 'O': dot_matrix_write(OUT); break;
    case 'H': dot_matrix_write(HOME_RUN); break;
    case 'B': dot_matrix_write(BALL); break;
    }
    usleep(1500000); // 1.5초 지연
}

// 입력된 숫자가 유효한지 확인
int is_valid_number(const char number[], int length) {
    if (strlen(number) != length) return 0;
    for (int i = 0; i < length; i++) {
        for (int j = i + 1; j < length; j++) {
            if (number[i] == number[j]) return 0;
        }
    }
    return 1;
}

// 스트라이크와 볼 계산
void check_guess(const char guess[], const char secret_number[], int length, int* strikes, int* balls) {
    *strikes = 0;
    *balls = 0;
    for (int i = 0; i < length; i++) {
        for (int j = 0; j < length; j++) {
            if (guess[i] == secret_number[j]) {
                if (i == j) (*strikes)++;
                else if (i != j) (*balls)++;
            }
        }
    }
}

// LED 제어
void led_control(int led_bits, int count) {
    for (int i = 0; i < count; i++) {
        ioctl(fd_led, led_bits, 1);
        usleep(500000);
        ioctl(fd_led, led_bits, 0);
        usleep(500000);
    }
}

// Tact Switch 입력 읽기
char read_switch() {
    unsigned int state;
    read(fd_switch, &state, sizeof(state));
    switch (state) {
    case 0x01: return '7';
    case 0x02: return '8';
    case 0x04: return '9';
    case 0x08: return '4';
    case 0x10: return '5';
    case 0x20: return '6';
    case 0x40: return '1';
    case 0x80: return '2';
    case 0x100: return '3';
    case 0x200: return '0';
    case 0x400: case 0x800: return 'E'; // Enter 키
    default: return '\0';
    }
}

// 플레이어가 숫자를 입력하는 함수
void get_player_input(char* buffer, int length, int round) {
    int idx = 0;
    while (idx < length) {
        char input = read_switch();
        if (input == 'E') {
            if (round == 0 && (idx < 2 || idx > 3)) {  // 1라운드에서 잘못된 길이
                clcd_write("Invalid length. Retry\n");
                idx = 0;
                memset(buffer, 0, length);
                continue;
            }
            else if (round == 1 && (idx < 3 || idx > 4)) {  // 2라운드에서 잘못된 길이
                clcd_write("Invalid length. Retry\n");
                idx = 0;
                memset(buffer, 0, length);
                continue;
            }
            else if (is_valid_number(buffer, idx)) {
                break;
            }
        }
        else if (input >= '0' && input <= '9') {
            buffer[idx++] = input;
        }
        usleep(100000); // 0.1초 지연
    }
}

int main() {
    char secret_number1[5], secret_number2[5], guess[10] = { 0 };
    int strikes, balls, turn = 1, score1 = 1000, score2 = 1000, rounds = 2, digits[2] = { 3, 4 };
    char clcd_buffer[32];  // 여기에 clcd_buffer 변수를 선언

    fd_led = open(DEV_LED, O_RDWR);
    if (fd_led < 0) {
        perror("LED 장치 열기 실패");
        return -1;
    }

    fd_switch = open(DEV_SWITCH, O_RDONLY);
    if (fd_switch < 0) {
        perror("Tact Switch 장치 열기 실패");
        close(fd_led);
        return -1;
    }

    clcd_init();
    dot_matrix_init();

    display_baseball(); // 게임 시작 전에 "BASEBALL" 표시

    for (int round = 0; round < rounds; round++) {
        snprintf(clcd_buffer, sizeof(clcd_buffer), "Round %d: Guess %d digits\n", round + 1, digits[round]);
        clcd_write(clcd_buffer);

        for (int player = 1; player <= 2; player++) {
            snprintf(clcd_buffer, sizeof(clcd_buffer), "Player %d: Set number\n", player);
            clcd_write(clcd_buffer);
            memset(guess, 0, sizeof(guess));
            get_player_input(guess, digits[round], round);
            if (player == 1) {
                strncpy(secret_number1, guess, digits[round]);
            }
            else {
                strncpy(secret_number2, guess, digits[round]);
            }
        }

        time_t start_time, end_time;
        time(&start_time);

        int home_run1 = 0, home_run2 = 0;
        while (home_run1 == 0 || home_run2 == 0) {
            if ((turn == 1 && home_run1) || (turn == 2 && home_run2)) {
                turn = turn == 1 ? 2 : 1;
                continue;
            }

            snprintf(clcd_buffer, sizeof(clcd_buffer), "Player %d: Guess number\n", turn);
            clcd_write(clcd_buffer);
            memset(guess, 0, sizeof(guess));
            get_player_input(guess, digits[round], round);

            time(&end_time);
            double seconds = difftime(end_time, start_time);

            check_guess(guess, turn == 1 ? secret_number2 : secret_number1, digits[round], &strikes, &balls);

            if (strikes == digits[round]) {
                snprintf(clcd_buffer, sizeof(clcd_buffer), "HR! Player %d right!\n", turn);
                clcd_write(clcd_buffer);
                display_status('H');
                if (turn == 1) home_run1 = 1;
                else home_run2 = 1;
            }
            else if (strikes == 0 && balls == 0) {
                clcd_write("Out! No S/B\n");
                display_status('O');
            }
            else {
                snprintf(clcd_buffer, sizeof(clcd_buffer), "%d Strikes, %d Balls\n", strikes, balls);
                clcd_write(clcd_buffer);
                if (strikes >= balls) {
                    display_status('S');
                }
                else {
                    display_status('B');
                }
            }

            if (turn == 1) score1 -= (10 + (int)seconds);
            else score2 -= (10 + (int)seconds);

            turn = turn == 1 ? 2 : 1;
            time(&start_time);  // 다음 추측을 위한 시간 리셋
        }

        snprintf(clcd_buffer, sizeof(clcd_buffer), "End R%d. P1: %d, P2: %d\n", round + 1, score1, score2);
        clcd_write(clcd_buffer);
    }

    snprintf(clcd_buffer, sizeof(clcd_buffer), "Final P1: %d, P2: %d\n", score1, score2);
    clcd_write(clcd_buffer);
    if (score1 > score2) clcd_write("P1 Wins!\n");
    else if (score1 < score2) clcd_write("P2 Wins!\n");
    else clcd_write("It's a tie!\n");

    close(fd_led);
    close(fd_switch);
    close(fd_clcd);
    close(fd_dot);
    return 0;
}
