#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define FND_DEVICE "/dev/fnd"

int main() {
    unsigned char test_data[4] = { 0xC0, 0x98, 0xD8, 0xB0 }; // 테스트 데이터 (1234)

    int fndFd = open(FND_DEVICE, O_RDWR);
    if (fndFd < 0) {
        perror("FND 디바이스 열기 실패");
        printf("errno: %d\n", errno);
        return -1;
    }
    else {
        printf("FND 디바이스 열기 성공, 파일 디스크립터: %d\n", fndFd);
    }

    ssize_t written = write(fndFd, test_data, sizeof(test_data));
    if (written < 0) {
        perror("FND 데이터 쓰기 실패");
        printf("errno: %d\n", errno);
    }
    else {
        printf("FND 데이터 쓰기 성공, %zd 바이트\n", written);
    }

    int close_result = close(fndFd);
    if (close_result < 0) {
        perror("FND 디바이스 닫기 실패");
        printf("errno: %d\n", errno);
    }
    else {
        printf("FND 디바이스 닫기 성공\n");
    }

    return 0;
}
