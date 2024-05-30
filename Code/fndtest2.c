#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <asm/ioctls.h>

#define tact_d "/dev/tactsw"
#define fnd_d  "/dev/fnd"

unsigned char fnd_data[4];

// 숫자를 FND에 출력하는 함수
void PrintFnd(int* nums, int count);
void ResetFnd();

int main() {
    int tact;
    unsigned char c;
    int nums[4] = { 0 };
    int count = 0;

    tact = open(tact_d, O_RDWR);
    if (tact < 0) {
        printf("tact : open failed!\n");
        return -1;
    }

    int fnd = open(fnd_d, O_RDWR);
    if (fnd < 0) {
        printf("fnd : open failed!\n");
        close(tact);
        return -1;
    }

    while (1) {
        while (1) {
            read(tact, &c, sizeof(c));
            usleep(100000);
            if (c) break;
        }

        if (c >= 1 && c <= 9) {
            if (count < 4) {
                nums[count] = c;
                count++;
                PrintFnd(nums, count);
                write(fnd, fnd_data, sizeof(fnd_data));
            }
        }

        else if (c == 12) {
            printf("Shutdown!\n");
            close(tact);
            close(fnd);
            return 0;
        }
        else {
            printf("Unknown input: %d\n", c);
        }
    }

    close(tact);
    close(fnd);
    return 0;
}

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

void ResetFnd() {
    // FND 데이터를 초기화합니다.
    fnd_data[0] = 0xFF;
    fnd_data[1] = 0xFF;
    fnd_data[2] = 0xFF;
    fnd_data[3] = 0xFF;
}