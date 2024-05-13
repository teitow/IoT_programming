#include <stdio.h>
#include <string.h>
#include <time.h>

// 입력된 숫자가 유효한지 검사하는 함수
int is_valid_number(const char number[], int length) {
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
void check_guess(const char guess[], const char secret_number[], int length, int* strikes, int* balls) {
    *strikes = 0;
    *balls = 0;
    for (int i = 0; i < length; i++) {
        for (int j = 0; j < length; j++) {
            if (guess[i] == secret_number[j]) {
                if (i == j)
                    (*strikes)++;
                else
                    (*balls)++;
            }
        }
    }
}

int main() {
    char secret_number1[5]; // 플레이어 1의 비밀 숫자 저장 (최대 4자리)
    char secret_number2[5]; // 플레이어 2의 비밀 숫자 저장
    char guess[10];         // 추측 저장 (넉넉한 길이)
    int strikes, balls;
    int turn = 1;
    int score1 = 1000, score2 = 1000; // 게임 시작 점수
    int rounds = 2;         // 총 라운드 수
    int digits[2] = { 3, 4 }; // 각 라운드의 자릿수

    for (int round = 0; round < rounds; round++) {
        int current_digits = digits[round];
        printf("\n라운드 %d: %d 자리\n", round + 1, current_digits);

        // 숫자 설정
        for (int player = 1; player <= 2; player++) {
            while (1) {
                printf("플레이어 %d, 숫자를 설정해주세요 (%d 자리, 중복 없음): ", player, current_digits);
                scanf("%s", player == 1 ? secret_number1 : secret_number2);
                if (!is_valid_number(player == 1 ? secret_number1 : secret_number2, current_digits)) {
                    printf("오류: 유효하지 않은 숫자입니다 (%d 자리가 아니거나 중복된 숫자가 있습니다). 다시 입력해주세요.\n", current_digits);
                }
                else {
                    break; // 유효한 입력
                }
            }
        }

        // 게임 시작 시간 기록
        time_t start_time, end_time;
        time(&start_time);

        // 게임 진행
        int home_run1 = 0, home_run2 = 0;
        while (home_run1 == 0 || home_run2 == 0) {
            if ((turn == 1 && home_run1) || (turn == 2 && home_run2)) {
                turn = turn == 1 ? 2 : 1; // 홈런을 친 플레이어는 건너뛰기
                continue;
            }

            while (1) {
                printf("플레이어 %d, 상대방의 숫자를 추측해보세요: ", turn);
                scanf("%s", guess);
                if (!is_valid_number(guess, current_digits)) {
                    printf("오류: 유효하지 않은 추측입니다 (%d 자리가 아니거나 중복된 숫자가 있습니다). 다시 입력해주세요.\n", current_digits);
                }
                else {
                    break; // 유효한 입력
                }
            }

            time(&end_time); // 추측 시간 기록
            double seconds = difftime(end_time, start_time);
            if (turn == 1) {
                check_guess(guess, secret_number2, current_digits, &strikes, &balls);
                if (strikes == current_digits) home_run1 = 1;
                score1 -= (10 + (int)seconds); // 점수 감점
            }
            else {
                check_guess(guess, secret_number1, current_digits, &strikes, &balls);
                if (strikes == current_digits) home_run2 = 1;
                score2 -= (10 + (int)seconds);
            }

            printf("%d 스트라이크, %d 볼\n", strikes, balls);
            if (strikes == current_digits) {
                printf("홈런! 플레이어 %d가 정답을 맞췄습니다!\n", turn);
            }

            // 턴 변경
            turn = turn == 1 ? 2 : 1;
            time(&start_time); // 다음 턴의 시작 시간 업데이트
        }

        printf("라운드 %d 종료. 플레이어 1 점수: %d, 플레이어 2 점수: %d\n", round + 1, score1, score2);
    }

    // 최종 스코어 결정
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

    return 0;
}
