#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEV_LED "/dev/led"
#define DEV_SWITCH "/dev/tactsw"

// LED 정의
#define LED_RED_0    0x01
#define LED_RED_4    0x10
#define LED_YELLOW_1 0x02
#define LED_YELLOW_5 0x20
#define LED_ORANGE_2 0x04
#define LED_ORANGE_6 0x40
#define LED_BLUE_3   0x08
#define LED_BLUE_7   0x80

int fd_led, fd_switch; // 파일 디스크립터

// 숫자 유효성 검사 함수
int is_valid_number(const char number[], int length) {
    if (strlen(number) != length) return 0;
    for (int i = 0; i < length; i++) {
        for (int j = i + 1; j < length; j++) {
            if (number[i] == number[j]) return 0;
        }
    }
    return 1;
}

// 스트라이크와 볼 계산 함수
void check_guess(const char guess[], const char secret_number[], int length, int* strikes, int* balls) {
    *strikes = 0;
    *balls = 0;
    for (int i = 0; i < length; i++) {
        if (guess[i] == secret_number[i]) (*strikes)++;
        for (int j = 0; j < length; j++) {
            if (guess[i] == secret_number[j] && i != j) (*balls)++;
        }
    }
}

// LED 제어 함수
void led_control(int led_bits, int count) {
    for (int i = 0; i < count; i++) {
        ioctl(fd_led, led_bits, 1);
        usleep(500000);
        ioctl(fd_led, led_bits, 0);
        usleep(500000);
    }
}

// Tact 스위치 입력 읽기
char read_switch() {
    unsigned char state;
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
    case 0x400: case 0x800: return 'E'; // 엔터
    default: return '\0';
    }
}

// 메인 함수
int main() {
    char secret_number1[5], secret_number2[5], guess[10] = { 0 };
    int strikes, balls, turn = 1, score1 = 1000, score2 = 1000, rounds = 2, digits[2] = { 3, 4 };

    fd_led = open(DEV_LED, O_RDWR);
    fd_switch = open(DEV_SWITCH, O_RDONLY);

    if (fd_led < 0 || fd_switch < 0) {
        perror("장치 파일 열기 실패");
        return -1;
    }

    for (int round = 0; round < rounds; round++) {
        int current_digits = digits[round];
        printf("\n라운드 %d: %d 자리 숫자를 맞춰보세요.\n", round + 1, current_digits);

        for (int player = 1; player <= 2; player++) {
            memset(secret_number1, 0, sizeof(secret_number1));
            memset(secret_number2, 0, sizeof(secret_number2));
            printf("플레이어 %d, 숫자를 설정하세요 (%d자리, 중복 없음): ", player, current_digits);
            for (int i = 0; i < current_digits;) {
                char input = read_switch();
                if (input == 'E') {
                    if (i == current_digits && is_valid_number(player == 1 ? secret_number1 : secret_number2, current_digits)) {
                        break;
                    }
                }
                else if (input != '\0') {
                    if (player == 1) {
                        secret_number1[i] = input;
                    }
                    else {
                        secret_number2[i] = input;
                    }
                    i++;
                }
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

            memset(guess, 0, sizeof(guess));
            printf("플레이어 %d, 상대방의 숫자를 추측해보세요: ", turn);
            for (int i = 0; i < current_digits;) {
                char input = read_switch();
                if (input == 'E') {
                    if (i == current_digits && is_valid_number(guess, current_digits)) {
                        break;
                    }
                }
                else if (input != '\0') {
                    guess[i] = input;
                    i++;
                }
            }

            time(&end_time);
            double seconds = difftime(end_time, start_time);

            check_guess(guess, turn == 1 ? secret_number2 : secret_number1, current_digits, &strikes, &balls);

            if (strikes == current_digits) {
                led_control(LED_BLUE_3 | LED_BLUE_7, 5);  // 홈런: 파란색 LED
                printf("홈런! 플레이어 %d가 정답을 맞췄습니다!\n", turn);
                if (turn == 1) home_run1 = 1;
                else home_run2 = 1;
            }
            else if (strikes == 0 && balls == 0) {
                led_control(LED_RED_0 | LED_RED_4, 5);  // 아웃: 빨간색 LED
                printf("아웃! 스트라이크와 볼이 하나도 없습니다.\n");
            }
            else if (strikes > balls) {
                led_control(LED_YELLOW_1 | LED_YELLOW_5, 5);  // 스트라이크가 많을 때: 노란색 LED
            }
            else {
                led_control(LED_ORANGE_2 | LED_ORANGE_6, 5);  // 볼이 많을 때: 주황색 LED
            }

            if (turn == 1) {
                score1 -= (10 + (int)seconds);
            }
            else {
                score2 -= (10 + (int)seconds);
            }

            turn = turn == 1 ? 2 : 1;
            time(&start_time);  // 다음 추측을 위해 시간 초기화
        }

        printf("라운드 %d 종료. 플레이어 1 점수: %d, 플레이어 2 점수: %d\n", round + 1, score1, score2);
    }

    printf("\n최종 점수 - 플레이어 1: %d, 플레이어 2: %d\n", score1, score2);
    if (score1 > score2) {
        printf("플레이어 1이 승리했습니다!\n");
    }
    else if (score1 < score2) {
        printf("플레이어 2가 승리했습니다!\n");
    }
    else {
        printf("무승부입니다!\n");
    }

    close(fd_led);  // LED 디바이스 파일 닫기
    close(fd_switch); // 스위치 디바이스 파일 닫기
    return 0;
}
