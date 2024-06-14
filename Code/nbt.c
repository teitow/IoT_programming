#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

// ����̽� ��� ����
#define fnd "/dev/fnd"
#define dot "/dev/dot"
#define tact "/dev/tactsw"
#define led "/dev/led"
#define dip "/dev/dipsw"
#define clcd "/dev/clcd"

// �Լ� �����
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
void blink_led(int type);
void display_baseball_message();

// ���� ����
int dipsw;
int leds;
int dot_mtx;
int tactsw;
int clcds;
int fnds;
unsigned char fnd_data[4];

// Dot Matrix ����
unsigned char patterns[4][8] = {
    {0x3C, 0x42, 0x40, 0x3C, 0x02, 0x02, 0x42, 0x3C}, // STRIKE
    {0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C}, // OUT
    {0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42}, // HOME RUN
    {0x1E, 0x22, 0x22, 0x1E, 0x22, 0x22, 0x22, 0x1E}  // BALL
};

// Define the 8x8 patterns for each letter in "BASEBALL"
unsigned char letters[8][8] = {
    {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}, // B
    {0x3C, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00}, // A
    {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00}, // S
    {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00}, // E
    {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}, // B
    {0x3C, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00}, // A
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}, // L
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}  // L
};

// Character LCD �Լ�
void print_clcd(const char* message) {
    clcds = open(clcd, O_RDWR);
    if (clcds < 0) {
        printf("Failed to open Character LCD.\n");
        exit(0);
    }
    write(clcds, message, strlen(message));
    close(clcds);
    usleep(100000);  // 0.1�� ���
}

void print_game_start() {
    print_clcd("Game Start!");
    usleep(2000000);  // 2�� ���
}

void print_round_start(int round) {
    char buffer[32];
    sprintf(buffer, "Round %d Start!", round);
    print_clcd(buffer);
    usleep(2000000);  // 2�� ���
}

void print_result(int strikes, int balls, int outs) {
    char buffer[32];
    sprintf(buffer, "S:%d B:%d O:%d", strikes, balls, outs);
    print_clcd(buffer);
    usleep(2000000);  // 2�� ���
}

void print_homerun() {
    print_clcd("HOMERUN!!!");
    usleep(1500000);  // 1.5�� ���
    unsigned char h_pattern[8] = { 0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42 }; // H ����
    writeToDotDevice(h_pattern, 1500000); // Dot Matrix�� H ���� 1.5�� ǥ��
}

void print_final_score(int player1_score, int player2_score) {
    char buffer[32];
    sprintf(buffer, "P1: %d P2: %d", player1_score, player2_score);
    print_clcd(buffer);
    usleep(2000000);  // 2�� ���
}

void print_game_over() {
    print_clcd("Game Over");
    usleep(2000000);  // 2�� ���
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
    usleep(2000000);  // 2�� ���
}

// Dot Matrix �Լ�
void writeToDotDevice(unsigned char* data, int time) {
    dot_mtx = open(dot, O_RDWR);
    if (dot_mtx < 0) {
        printf("Failed to open Dot device.\n");
        exit(0);
    }
    write(dot_mtx, data, 8);
    usleep(time);
    close(dot_mtx);
    usleep(100000);  // 0.1�� ���
}

// TACT Switch �Լ�
int tactsw_get_with_timer(int t_second) {
    int tactswFd, selected_tact = 0;
    unsigned char b = 0;

    tactswFd = open(tact, O_RDONLY);
    if (tactswFd < 0) {
        printf("Failed to open TACT switch device.\n");
        return -1;
    }

    while (t_second > 0) {
        usleep(100000); // 100ms ���
        read(tactswFd, &b, sizeof(b));
        if (b) {
            selected_tact = b;
            close(tactswFd);
            return selected_tact;
        }
        t_second -= 0.1;
    }

    close(tactswFd);
    return 0; // Ÿ�Ӿƿ�
}

// DIP Switch �Լ�
int dipsw_get_with_timer(int t_second) {
    int dipswFd, selected_dip = 0;
    unsigned char d = 0;

    dipswFd = open(dip, O_RDONLY);
    if (dipswFd < 0) {
        printf("Failed to open DIP switch device.\n");
        return -1;
    }

    while (t_second > 0) {
        usleep(100000); // 100ms ���
        read(dipswFd, &d, sizeof(d));
        if (d) {
            selected_dip = d;
            close(dipswFd);
            return selected_dip;
        }
        t_second -= 0.1;
    }

    close(dipswFd);
    return 0; // Ÿ�Ӿƿ�
}

// LED ���� �Լ�
void led_on(int strikes, int balls, int outs, int homerun) {
    unsigned char led_data = 0;
    if (strikes > 0) led_data |= 0x22; // Green LEDs
    if (balls > 0) led_data |= 0x44;   // Yellow LEDs
    if (outs > 0) led_data |= 0x11;    // Red LEDs
    if (homerun > 0) led_data |= 0x88; // Blue LEDs
    leds = open(led, O_RDWR);
    if (leds < 0) {
        printf("Failed to open LED device.\n");
        exit(0);
    }
    write(leds, &led_data, sizeof(unsigned char));
    close(leds);
    usleep(100000);  // 0.1�� ���
}

// LED Blink �Լ�
void blink_led(int type) {
    unsigned char led_on_data = 0x00;
    unsigned char led_off_data = 0xFF;

    // Define LED patterns based on the type
    switch (type) {
    case 1: // Outs
        led_on_data = 0xEE; // 0b11101110
        break;
    case 2: // Strikes
        led_on_data = 0xDD; // 0b11011101
        break;
    case 3: // Balls
        led_on_data = 0xBB; // 0b10111011
        break;
    case 4: // Homerun
        led_on_data = 0x77; // 0b01110111
        break;
    default:
        led_on_data = 0x00;
        break;
    }

    int ledFd = open(led, O_RDWR);
    if (ledFd < 0) {
        printf("Failed to open LED device.\n");
        return;
    }

    for (int i = 0; i < 3; i++) {
        write(ledFd, &led_on_data, sizeof(unsigned char));
        usleep(100000); // 0.3�� ���
        write(ledFd, &led_off_data, sizeof(unsigned char));
        usleep(100000); // 0.3�� ���
    }
    close(ledFd);
}

// "BASEBALL" �޽��� ǥ�� �Լ�
void display_baseball_message() {
    for (int i = 0; i < 8; i++) {
        writeToDotDevice(letters[i], 500000); // ���ں��� 0.5�ʾ� ǥ��
    }
}

// ����̽� �ʱ�ȭ �Լ�
void init_devices() {
    dipsw = open(dip, O_RDWR);
    leds = open(led, O_RDWR);
    dot_mtx = open(dot, O_RDWR);
    tactsw = open(tact, O_RDWR);
    clcds = open(clcd, O_RDWR);
    fnds = open(fnd, O_RDWR);
    if (dipsw < 0 || leds < 0 || dot_mtx < 0 || tactsw < 0 || clcds < 0 || fnds < 0) {
        printf("Failed to open devices.\n");
        exit(0);
    }
    close(dipsw);
    close(leds);
    close(dot_mtx);
    close(tactsw);
    close(clcds);
    close(fnds);
}

// ���� ��Ģ ��� �Լ�
void game_rule(int round) {
    if (round == 1) {
        print_clcd("Round 1, 3 digits");
    }
    else if (round == 2) {
        print_clcd("Round 2, 4 digits");
    }
    usleep(3000000);
}

// ���� ��ȿ�� �˻� �Լ�
int is_valid_number(const char* number, int length) {
    if (strlen(number) != length) return 0; // ������ ���̰� �ƴ�
    for (int i = 0; i < length; i++) {
        for (int j = i + 1; j < length; j++) {
            if (number[i] == number[j]) {
                return 0; // �ߺ��� ���� �߰�
            }
        }
    }
    return 1; // ��ȿ�� �Է�
}

// ����� ���� �˻� �� ��Ʈ����ũ, �� ���
void check_guess(const char* guess, const char* secret, int length, int* strikes, int* balls, int* outs) {
    *strikes = 0;
    *balls = 0;
    *outs = 0;

    for (int i = 0; i < length; i++) {
        int is_out = 1; // �⺻���� �ƿ����� ����

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

    // ��� ���ڰ� �ƿ��� ��� outs�� length�� ������
    if (*outs != length) {
        *outs = 0; // ��� ���ڰ� �ƿ��� �ƴ� ��� outs�� 0���� ����
    }
    else {
        *strikes = 0;
        *balls = 0;
    }
}

// FND ��� �Լ�
void PrintFnd(int* nums, int count) {
    unsigned char fnd_codes[10] = { 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xD8, 0x80, 0x98 };
    unsigned char read_data[4];

    // FND �����͸� �ʱ�ȭ�մϴ�.
    fnd_data[0] = 0xFF;
    fnd_data[1] = 0xFF;
    fnd_data[2] = 0xFF;
    fnd_data[3] = 0xFF;

    // �Էµ� ���ڸ� FND�� ǥ���մϴ�.
    for (int i = 0; i < count; i++) {
        fnd_data[i] = fnd_codes[nums[i]];
        printf("FND position %d, write value %d\n", i, nums[i]); // ����� ���� ���
    }

    int fndFd = open(fnd, O_RDWR);
    if (fndFd < 0) {
        printf("Failed to open FND device.\n");
        return;
    }
    write(fndFd, &fnd_data, sizeof(fnd_data));
    usleep(100000);  // 0.1�� ���

    // FND�� ���� ���� �о�ͼ� ����� ���� ���
    read(fndFd, &read_data, sizeof(read_data));
    for (int i = 0; i < 4; i++) {
        printf("FND position %d, read value %02X\n", i, read_data[i]); // ���� FND �� ���
    }

    close(fndFd);
}

// ���� �Է� �Լ�
void input_number(char* number, int digits) {
    int tactsw_value;
    int idx = 0;

    int nums[4] = { 0 }; // ���� �迭 �ʱ�ȭ
    int fndFd = open(fnd, O_RDWR);
    if (fndFd < 0) {
        printf("Failed to open FND device.\n");
        return;
    }

    while (idx < digits) {
        tactsw_value = tactsw_get_with_timer(100);
        if (tactsw_value >= 1 && tactsw_value <= 9) {
            number[idx] = '0' + tactsw_value;
            nums[idx] = tactsw_value; // ���� �迭�� �� ����
            idx++;
            PrintFnd(nums, idx); // ���� �迭�� ���� �ε��� ����
            printf("input_number: TACT switch value %d, Current number: %s\n", tactsw_value, number);
        }
        // ���� �Էµ� ���� FND�� ǥ���Ͽ� ����
        write(fndFd, &fnd_data, sizeof(fnd_data));
        usleep(300000);  // 0.3�� ���
    }
    number[digits] = '\0';

    close(fndFd);
}

// FND Blink �Լ�
void blink_fnd() {
    unsigned char fnd_codes[10] = { 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xD8, 0x80, 0x98 };
    unsigned char fnd_blink_data[4] = { fnd_codes[8], fnd_codes[8], fnd_codes[8], fnd_codes[8] };
    unsigned char fnd_clear_data[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

    int fndFd = open(fnd, O_RDWR);
    if (fndFd < 0) {
        printf("Failed to open FND device.\n");
        return;
    }

    for (int i = 0; i < 5; i++) {
        write(fndFd, &fnd_blink_data, sizeof(fnd_blink_data));
        usleep(100000); // 0.1�� ���
        write(fndFd, &fnd_clear_data, sizeof(fnd_clear_data));
        usleep(100000); // 0.1�� ���
    }
    close(fndFd);
}

// ���� ���� �Լ�
void start_game() {
    char secret_number1[5]; // �÷��̾� 1�� ��� ���� ���� (�ִ� 4�ڸ�)
    char secret_number2[5]; // �÷��̾� 2�� ��� ���� ����
    char guess[5];          // ���� ���� (�ִ� 4�ڸ�)
    int strikes, balls, outs;
    int turn = 1;
    int score1 = 1000, score2 = 1000; // ���� ���� ����
    int rounds = 2;                   // �� ���� ��
    int digits[2] = { 3, 4 };         // �� ������ �ڸ���

    for (int round = 0; round < rounds; round++) {
        int current_digits = digits[round];
        game_rule(round + 1);

        // �÷��̾� 1�� 2�� ��� ���� ����
        for (int player = 1; player <= 2; player++) {
            while (1) {
                print_clcd(player == 1 ? "P1 Set Number:" : "P2 Set Number:");
                input_number(player == 1 ? secret_number1 : secret_number2, current_digits);
                if (!is_valid_number(player == 1 ? secret_number1 : secret_number2, current_digits)) {
                    print_clcd("Invalid Number!");
                    usleep(1500000); // 1.5�� ���
                }
                else {
                    break; // ��ȿ�� �Է�
                }
            }
        }

        // ���� ����
        int home_run1 = 0, home_run2 = 0;
        while (home_run1 == 0 || home_run2 == 0) {
            if (home_run1 == 1 && home_run2 == 1) {
                break;
            }

            if (turn == 1 && home_run1 == 1) {
                turn = 2; // Ȩ���� ģ �÷��̾�� �ǳʶٱ�
                continue;
            }
            if (turn == 2 && home_run2 == 1) {
                turn = 1; // Ȩ���� ģ �÷��̾�� �ǳʶٱ�
                continue;
            }

            time_t start_time = time(NULL);

            while (1) {
                print_clcd(turn == 1 ? "P1 Guess:" : "P2 Guess:");
                input_number(guess, current_digits);
                if (!is_valid_number(guess, current_digits)) {
                    print_clcd("Invalid Guess!");
                    usleep(1500000); // 1.5�� ���
                }
                else {
                    break; // ��ȿ�� �Է�
                }
            }

            // FND�� �Էµ� ���� ǥ��
            int guess_nums[4];
            for (int i = 0; i < current_digits; i++) {
                guess_nums[i] = guess[i] - '0';
            }
            PrintFnd(guess_nums, current_digits);
            usleep(2000000); // 2�� ���� ǥ��

            time_t end_time = time(NULL);
            int time_diff = difftime(end_time, start_time);

            if (turn == 1) {
                check_guess(guess, secret_number2, current_digits, &strikes, &balls, &outs);
                if (strikes == current_digits) {
                    home_run1 = 1;
                }
                else {
                    score1 -= 10; // Ȩ���� �ƴ� ��� 10�� ����
                }
                score1 -= time_diff; // �ʴ� 1�� ����
            }
            else {
                check_guess(guess, secret_number1, current_digits, &strikes, &balls, &outs);
                if (strikes == current_digits) {
                    home_run2 = 1;
                }
                else {
                    score2 -= 10; // Ȩ���� �ƴ� ��� 10�� ����
                }
                score2 -= time_diff; // �ʴ� 1�� ����
            }

            print_result(strikes, balls, outs);
            led_on(strikes, balls, outs, (strikes == current_digits));

            // ���� Dot Matrix�� ������ ǥ��
            if (strikes == current_digits) {
                print_homerun();
                blink_led(4); // Ȩ���� �� LED ������
            }
            else {
                if (strikes > 0) {
                    writeToDotDevice(patterns[0], 2000000); // ��Ʈ����ũ ���� ǥ��
                    blink_led(2); // ��Ʈ����ũ�� �� LED ������
                }
                if (balls > 0) {
                    writeToDotDevice(patterns[3], 2000000); // �� ���� ǥ��
                    blink_led(3); // ���� �� LED ������
                }
                if (outs == current_digits) {
                    writeToDotDevice(patterns[1], 2000000); // �ƿ� ���� ǥ��
                    blink_led(1); // �ƿ��� �� LED ������
                }
            }

            // �� ����
            turn = turn == 1 ? 2 : 1;
        }
    }

    // ���� ��� ���
    print_final_score(score1, score2);

    // ���� ���� FND ���
    int final_score = (score1 > score2) ? score1 : score2;
    int final_scores[4] = { final_score / 1000, (final_score / 100) % 10, (final_score / 10) % 10, final_score % 10 };
    PrintFnd(final_scores, 4);
    int fnd_fd = open(fnd, O_RDWR);
    if (fnd_fd < 0) {
        printf("Failed to open FND device.\n");
        return;
    }
    write(fnd_fd, fnd_data, sizeof(fnd_data));
    usleep(5000000); // 5�� ���� FND�� ���� ǥ��
    close(fnd_fd);

    // ���� ���
    print_winner(score1, score2);
}

// ���� �Ұ� �Լ�
int intro() {
    int dip_value = 0;

    print_clcd("DIP UP to Start");
    dip_value = dipsw_get_with_timer(100);  // 10�ʷ� ����

    if (dip_value != 0) {
        blink_fnd();
        blink_led(0); // Default blink
        display_baseball_message(); // Dot Matrix�� "BASEBALL" ǥ��
        usleep(500000); // 0.5�� ���
    }
    return dip_value;
}

// ���� �Լ�
int main() {
    init_devices();
    if (intro() != 0) {
        print_game_start();
        start_game();
        print_game_over();
    }
    else {
        print_clcd("Game Not Started");
        usleep(2000000); // 2�� ���
    }
    return 0;
}