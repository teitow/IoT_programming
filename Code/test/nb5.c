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
#define dot "/dev/dot"
#define tact "/dev/tactsw"
#define led "/dev/led"
#define dip "/dev/dipsw"
#define clcd "/dev/clcd"
#define fnd "/dev/fnd"

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
void blink_led();
void update_fnd(int number);

// ���� ����
int dipsw;
int leds;
int dot_mtx;
int tactsw;
int clcds;
int fnds;

// Dot Matrix ����
unsigned char patterns[4][8] = {
    {0x3C, 0x42, 0x40, 0x3C, 0x02, 0x02, 0x42, 0x3C}, // STRIKE
    {0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C}, // OUT
    {0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42}, // HOME RUN
    {0x1E, 0x22, 0x22, 0x1E, 0x22, 0x22, 0x22, 0x1E}  // BALL
};

// Character LCD �Լ�
void print_clcd(const char* message) {
    clcds = open(clcd, O_RDWR);
    if (clcds < 0) {
        printf("Character LCD ���� ����.\n");
        exit(0);
    }
    write(clcds, message, strlen(message));
    close(clcds);
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
        printf("Dot ����̽� ���� ����\n");
        exit(0);
    }
    write(dot_mtx, data, 8);
    usleep(time);
    close(dot_mtx);
}

// TACT Switch �Լ�
int tactsw_get_with_timer(int t_second) {
    int tactswFd, selected_tact = 0;
    unsigned char b = 0;

    tactswFd = open(tact, O_RDONLY);
    if (tactswFd < 0) {
        perror("TACT ����ġ ����̽� ���� ����");
        return -1;
    }

    while (t_second > 0) {
        usleep(100000); // 100ms ���
        read(tactswFd, &b, sizeof(b));
        if (b) {
            selected_tact = b;
            update_fnd(selected_tact);
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
        perror("DIP ����ġ ����̽� ���� ����");
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
        printf("LED ���� ����.\n");
        exit(0);
    }
    write(leds, &led_data, sizeof(unsigned char));
    close(leds);
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
        printf("����̽� ���� ����\n");
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

// ���� ǥ�� �Լ�
void display_score(int player1_score, int player2_score) {
    char score[32];
    sprintf(score, "P1: %d, P2: %d", player1_score, player2_score);
    print_clcd(score);
    usleep(5000000); // 5�� ���
}

// ���� �Է� �Լ�
void input_number(char* number, int digits) {
    int tactsw_value;
    int idx = 0;

    while (idx < digits) {
        tactsw_value = tactsw_get_with_timer(100);
        if (tactsw_value >= 1 && tactsw_value <= 9) {
            number[idx] = '0' + tactsw_value;
            idx++;
            printf("input_number: TACT switch value %d, Current number: %s\n", tactsw_value, number);
        }
    }
    number[digits] = '\0';
}

// FND ������Ʈ �Լ�
void update_fnd(int number) {
    unsigned char fnd_data[4] = { 0x00, 0x00, 0x00, 0x00 };
    fnds = open(fnd, O_RDWR);
    if (fnds < 0) {
        printf("FND ���� ����.\n");
        exit(0);
    }
    fnd_data[3] = 0x3F & ~0x01; // FND�� ǥ���� �� (��: ���� 1)
    fnd_data[2] = 0x06 & ~0x01; // FND�� ǥ���� �� (��: ���� 2)
    fnd_data[1] = 0x5B & ~0x01; // FND�� ǥ���� �� (��: ���� 3)
    fnd_data[0] = 0x4F & ~0x01; // FND�� ǥ���� �� (��: ���� 4)
    fnd_data[3] = 0x3F & ~0x01; // FND�� ǥ���� �� (��: ���� 0)
    write(fnds, fnd_data, 4);
    close(fnds);
}

// LED Blink �Լ�
void blink_led() {
    unsigned char led_on_data = 0xFF;
    unsigned char led_off_data = 0x00;

    int ledFd = open(led, O_RDWR);
    if (ledFd < 0) {
        perror("LED ����̽� ���� ����");
        return;
    }

    for (int i = 0; i < 5; i++) {
        write(ledFd, &led_on_data, sizeof(unsigned char));
        usleep(200000); // 0.2�� ���
        write(ledFd, &led_off_data, sizeof(unsigned char));
        usleep(200000); // 0.2�� ���
    }
    close(ledFd);
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
                    usleep(2000000); // 2�� ���
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
                    usleep(2000000); // 2�� ���
                }
                else {
                    break; // ��ȿ�� �Է�
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

            if (strikes == current_digits) {
                print_homerun();
            }
            else {
                if (strikes > 0) {
                    writeToDotDevice(patterns[0], 2000000); // ��Ʈ����ũ ���� ǥ��
                }
                if (balls > 0) {
                    writeToDotDevice(patterns[3], 2000000); // �� ���� ǥ��
                }
                if (outs == current_digits) {
                    writeToDotDevice(patterns[1], 2000000); // �ƿ� ���� ǥ��
                }
            }

            // �� ����
            turn = turn == 1 ? 2 : 1;
        }

        display_score(score1, score2);
    }

    // ���� ��� ���
    print_final_score(score1, score2);

    // ���� ���
    print_winner(score1, score2);
}


// ���� �Ұ� �Լ�
int intro() {
    int dip_value = 0;

    print_clcd("DIP UP to Start");
    dip_value = dipsw_get_with_timer(100);  // 10�ʷ� ����

    if (dip_value != 0) {
        blink_led();
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