#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// 디바이스 경로 정의
#define fnd "/dev/fnd"
#define tact "/dev/tactsw"
#define led "/dev/led"
#define clcd "/dev/clcd"

// 함수 선언부
void init_devices();
void close_devices();
void test_devices();
int read_tact_switch();
void print_fnd(int number);

// 전역 변수
int dipsw;
int leds;
int dot_mtx;
int tactsw;
int clcds;
int fnds;

// 디바이스 초기화 함수
void init_devices() {
    if ((leds = open(led, O_RDWR)) < 0 ||
        (tactsw = open(tact, O_RDWR)) < 0 ||
        (clcds = open(clcd, O_RDWR)) < 0 ||
        (fnds = open(fnd, O_RDWR)) < 0) {
        printf("디바이스 열기 실패\n");
        exit(0);
    }
    printf("모든 디바이스가 성공적으로 열렸습니다.\n");
}

void close_devices() {
    close(leds);
    close(tactsw);
    close(clcds);
    close(fnds);
    printf("모든 디바이스가 성공적으로 닫혔습니다.\n");
}

// Tact 스위치 읽기 함수
int read_tact_switch() {
    unsigned char b;
    read(tactsw, &b, sizeof(b));
    return b;
}

// FND 출력 함수
void print_fnd(int number) {
    unsigned char fnd_data[4];
    unsigned char fnd_codes[10] = { 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xD8, 0x80, 0x90 };

    fnd_data[0] = fnd_codes[number / 1000 % 10];
    fnd_data[1] = fnd_codes[number / 100 % 10];
    fnd_data[2] = fnd_codes[number / 10 % 10];
    fnd_data[3] = fnd_codes[number % 10];

    write(fnds, fnd_data, sizeof(fnd_data));
}

// 디바이스 테스트 함수
void test_devices() {
    // 간단한 테스트를 위해 CLCD에 메시지 출력
    write(clcds, "Tact 스위치를 누르세요", 30);

    // Tact 스위치 상태를 읽어 FND에 표시
    int tact_value;
    while (1) {
        tact_value = read_tact_switch();
        if (tact_value > 0 && tact_value <= 9) {
            print_fnd(tact_value);
            break;
        }
    }
}

int main() {
    init_devices();

    // 테스트 함수 호출
    test_devices();

    // 대기 시간 설정 (5초)
    sleep(5);

    close_devices();
    return 0;
}
