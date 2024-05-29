#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h> // For sleep function
#include <math.h>

// Target System
#define fnd "/dev/fnd"			// 7-Segment FND 
#define dot "/dev/dot"			// Dot Matrix
#define tact "/dev/tactsw"    		// Tact Switch
#define led "/dev/led"			// LED 
#define dip "/dev/dipsw"		// Dip Switch
#define clcd "/dev/clcd"		// Character LCD

// 입출력 장치
int dipsw;
int leds;
int dot_mtx;
int tactsw;
int clcds;
int fnds;

// Dot Matrix 패턴 정의
unsigned char pattern[5][8] = {
    // BASEBALL_ICON
    {0x7E, 0x81, 0xA5, 0xDB, 0x81, 0xA5, 0x99, 0x7E},
    // STRIKE
    {0x3C, 0x42, 0x40, 0x3C, 0x02, 0x02, 0x42, 0x3C},
    // OUT
    {0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C},
    // HOME_RUN
    {0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42},
    // BALL
    {0x1E, 0x22, 0x22, 0x1E, 0x22, 0x22, 0x22, 0x1E}
};

// FND 숫자 정의
unsigned char FND_NUMBER[11] = {~0x3f, ~0x06, ~0x5b, ~0x4f, ~0x66, ~0x6d, ~0x7d, ~0x07, ~0x7f, ~0x67, ~0x00};
unsigned char fnd_num[4] = {0,};

// LED 설정
char led_array[] = { 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00};

// 하드웨어 제어 함수 프로토타입
void clcd_display(char *message);
void dot_matrix_display(unsigned char *pattern);
void fnd_display(int number);
int read_tact_switch(int length);
void led_control(int strikes);

// 비밀 번호 입력 함수 & (추가) 예상 숫자 입력 함수
void input_secret_number(int length, int *secret) {
    int switch_val;
    switch_val = read_tact_switch(length);
    if (switch_val >= 0 && switch_val <= (int)pow(10, length) - 1) {
        for (int i = length - 1; i >= 0; i--) {
            secret[i] = switch_val % 10;
            switch_val /= 10;
        }
    }
}

// 스트라이크와 볼 계산 함수
void calculate_score(int *secret, int *guess, int length, int *strikes, int *balls) {
    *strikes = 0;
    *balls = 0;
    for (int i = 0; i < length; i++) {
        if (secret[i] == guess[i]) {
            (*strikes)++;
        } else {
            for (int j = 0; j < length; j++) {
                if (secret[i] == guess[j]) {
                    (*balls)++;
                }
            }
        }
    }
}

// 게임 실행 함수
void play_game() {
    srand(time(NULL)); // 랜덤 시드 설정

    // 초기 메시지 표시
    clcd_display("Welcome to Number Baseball Game!");
    dot_matrix_display(pattern[0]);                     // 야구공 모형 도트 매트릭스 출력

    char *players[2] = {"Player 1", "Player 2"};
    int scores[2] = {1000, 1000}; // 각 플레이어의 초기 점수
    int rounds[2] = {3, 4}; // 각 라운드에서 설정할 비밀 번호의 길이
    int secrets[2][4];      // 최대 4자리 비밀 번호 저장

    for (int round_idx = 0; round_idx < 2; round_idx++) {           // 게임 라운드 부분. 0 = 1라운드(길이 3), 1 = 2라운드 (길이 4)
        int length = rounds[round_idx];
        clcd_display("Setting secret numbers");

        for (int i = 0; i < 2; i++) {                               // 비밀번호 설정 구간. 두 플레이어가 번갈아서 비밀번호 설정.
            clcd_display(players[i]);
            input_secret_number(length, secrets[i]);
        }

        int round_over = 0; // 라운드 종료 여부를 체크
        while (!round_over) {
            for (int player_idx = 0; player_idx < 2; player_idx++) {
                clcd_display(players[player_idx]);

                int guess[4] = {0};             // 상대 플레이어가 추측할 예상 비밀번호
                int switch_val;

                input_secret_number(length, guess);             // 상대 플레이어가 예측 비밀번호를 입력하는 구간. 함수명은 input_secret_number지만 값은 guess로 들어감.

                int strikes, balls;
                calculate_score(secrets[1 - player_idx], guess, length, &strikes, &balls);      // 예상 비밀번호의 정답 계산
                led_control(strikes);

                if (strikes == length) {
                    clcd_display("Home Run!");
                    dot_matrix_display(pattern[3]);
                    round_over = 1;
                    break;
                } else if (strikes > 0) {
                    clcd_display("Strikes!");
                    dot_matrix_display(pattern[2]);
                } else if (balls > 0) {
                    clcd_display("Balls!");
                    dot_matrix_display(pattern[4]);
                } else {
                    clcd_display("Out!");
                    dot_matrix_display(pattern[1]);
                }
                scores[player_idx] -= 10; // 점수 차감
            }
        }
        clcd_display("Round Over");
    }

    int winner_idx = (scores[0] > scores[1]) ? 0 : 1;           // 게임 종료 후 점수에 따라 승리자 결정
    char winner_message[20] = "Win Player: ";
    strcat(winner_message,players[winner_idx]);
    clcd_display(winner_message);
}

// CLCD에 메시지 표시 함수
void clcd_display(char *message) {
    clcds = open(clcd, O_WRONLY); // CLCD 예외 처리
    if (clcds < 0) {
        printf("Failed to open CLCD device. 출력하고자 한 문자열: %s\n", message);
        exit(0);
    }
    write(clcds, message, strlen(message));
    usleep(500000);
    close(clcds);
}

// Dot Matrix에 패턴 표시 함수
void dot_matrix_display(unsigned char *pattern) {
    // Dot Matrix에 패턴 표시
    // 실제 하드웨어 제어 코드는 여기에 작성
    dot_mtx = open(dot, O_RDWR);
    if (dot_mtx < 0) {
        printf("Cannot open dot device\n");
        exit(0);
    }
    write(dot_mtx, pattern, 8);
    usleep(500000);
    close(dot_mtx);
}

// FND에 숫자 표시 함수
void fnd_display(int TactNumber) {
    // FND에 숫자 표시
    // 실제 하드웨어 제어 코드는 여기에 작성
    if((fnds = open(fnd,O_RDWR)) <0){       // 예외처리
        printf("fnd open error");
        exit(0);
    }
	fnd_num[0] = FND_NUMBER[TactNumber / 1000];
	fnd_num[1] = FND_NUMBER[TactNumber / 100 % 10];
	fnd_num[2] = FND_NUMBER[TactNumber / 10 % 10];
	fnd_num[3] = FND_NUMBER[TactNumber % 10];
    write(fnds, &fnd_num, sizeof(fnd_num));
    close(fnds);
}


// Tact Switch 입력 읽기 함수
int read_tact_switch(int length) {
    // Tact Switch 상태 읽기
    // 실제 하드웨어 제어 코드는 여기에 작성
    unsigned char b = 0;
    int number = 0; // number 변수 초기화
    if((tactsw = open( tact, O_RDWR )) < 0){     	// 예외처리    
		printf("tact open error");
		return 0;
	}

    clcd_display("Enter a number 0 to 9 : ");

    while(1){
        read(tactsw, &b, sizeof(b));
        if(1<=b && b<=12){
            switch (b){
                case 1: case 2: case 3: case 4: case 5:
                case 6: case 7: case 8: case 9:
                    if (number < pow(10, length-1)) {       // 숫자가 지정된 길이보다 작을 때만 추가
                        number = number * 10 + b; 
                        fnd_display(number);
                    } 
                    break; 
                case 10: 
                    if (number > 9) {                       // 마지막 숫자 제거
                        number /= 10; 
                        fnd_display(number);
                    } 
                    break; 
                case 11: 
                    if (number < pow(10, length-1)) { 
                        number = number * 10; 
                        fnd_display(number);
                    } 
                    break;  
                case 12:
                    if (number > 0){
                        fnd_num[0] = FND_NUMBER[10];
                        fnd_num[1] = FND_NUMBER[10];
                        fnd_num[2] = FND_NUMBER[10];
                        fnd_num[3] = FND_NUMBER[10];
                        close(tactsw);
                        return number;
                    }
                    else {                                          // 값이 정상적으로 입력되지 않았을 경우 경고 문구
                        clcd_display("At least one digit must be entered.");
                    }
			}	
		}
    }
}

// LED 제어 함수
void led_control(int strikes) {
    // LED 상태 제어
    // 실제 하드웨어 제어 코드는 여기에 작성
    int data;
    leds = open(led, O_RDWR);
    if (leds < 0) {                             // 예외처리
        printf("Can't open LED.\n");
        exit(0);
    }
    data = led_array[strikes];
    write(leds, &data, sizeof(unsigned char));  usleep(500000);
    close(leds);
}

int main() {
    play_game();
    return 0;
}
