// Header File
#include<stdio.h>          		// 입출력 관련 
#include<stdlib.h>         		// 문자열 변환, 메모리 관련 
#include<unistd.h>       		// POSIX 운영체제 API에 대한 액세스 제공 
#include<fcntl.h>			// 타겟시스템 입출력 장치 관련 
#include<sys/types.h>    		// 시스템에서 사용하는 자료형 정보 
#include<sys/ioctl.h>    		// 하드웨어의 제어와 상태 정보 
#include<sys/stat.h>     		// 파일의 상태에 대한 정보 
#include <string.h>       		// 문자열 처리 
#include <time.h>         		// 시간 관련 


// Target System
#define fnd "/dev/fnd"			// 7-Segment FND 
#define dot "/dev/dot"			// Dot Matrix
#define tact "/dev/tactsw"    		// Tact Switch
#define led "/dev/led"			// LED 
#define dip "/dev/dipsw"		// Dip Switch
#define clcd "/dev/clcd"		// Character LCD


// 함수 선언부

// 입출력함수

// 문구를 입력받아 Character LCD 에 출력하는 함수 
void print(char P[]); 

// 출력할 카드와 출력 시간을 입력 받아 출력시간만큼 Dot Matrix LED 에 출력해주는 함수
void writeToDotDevice(int card, int time); 

// 제한 시간을 입력 받아 입력된 제한 시간 동안 Dip Switch 의 입력값을 읽고 동시에 1 초 단위마다 7-Segment LED 에 남은 제한시간을 표시해주는 함수 
int dipsw_get_with_timer(int t_second);

// 제한 시간을 입력 받아 입력된 제한 시간 동안 Tact Switch 의 입력값을 읽고 값에 따른 베팅이나 힌트 그리고 규칙 출력 등의 처리를 해주는 함수
int* tactsw_get_with_timer(int t_second);

// 라운드가 끝나고 호출되며, 플레이어의 승리 수 만큼 Chip LED 를 점등해주는 함수 
void led_on(int user_score); 


// 게임 진행 전 함수 

// 인트로 메시지를 먼저 출력해주고 Dip Switch 입력값을 이용해 조작하지 않았을 경우 게임시작여부를 다시 한 번 물어 게임을 시작할지 여부를 결정하는 함수
int intro_key(); 

// 인트로 메시지를 입력받아 Character LCD 에 출력하고 Dip Switch의 입력이 있으면 입력값을 반환해주는 함수 
int intro(char p[]); 

// 카드를 섞는 알고리즘이 저장되어 있는 함수 -> 랜덤 컴퓨터 숫자 생성
void shuffle_card(int start, int* cards);

// 메인 함수에서 호출되며, 게임 시작 전 컴퓨터와 플레이어의 카드를 섞기 위해 shuffle_card 함수를 호출해주는 함수
void prepare(int* cards1, int* cards2);

// 게임의 규칙을 Character LCD 에 출력해주는 함수
void game_rule();



// 게임 진행 함수

// 메인 함수
int main(); 
 
//게임 전체 과정을 진행해주는 함수로, 컴퓨터와 플레이어의 카드배열을 입력 받음 
void start(); 

//베팅을 담당하는 함수 -> 플레이어의 정답 입력
int* betting_start(int round);

//배열 정렬 함수로, 카드 배열과 정렬할 부분의 마지막 위치를 입력받아 오름차순 정렬 
void ascending(int arr[], int cnt);

//힌트 출력 함수, 플레이어의 입력 값인 user_answer 와 플레이어의 카드배열, 현재 라운드를 입력받아 user_answer 에 맞는 힌트를 Dot Matrix LED 에  출력 
void hint(int user_answer, int* user_card, int round);

//컴퓨터와 플레이어의 카드 비교 함수 
//각 라운드별로 컴퓨터와 플레이어의 카드를 입력 받고 비교를 해 비교 값을 반환.
int compare_card(int com_card, int user_card); 

//승패 결과를 확인하는 함수
//플레이어가 입력한 정답과 compare_card 함수에서 저장한 값을 비교해 입력한 정답과 compare_card 함수에서 저장한 값이 같으면 1 다르면 0 반환 
int win_lose(int user_answer, int correct_answer);

//컴퓨터의 랜덤 숫자를 생성하는 함수
int* enerate_random_number();

// 스트라이크/ 볼 개수를 판단하는 함수
int* compare_num(int* com_num, int* user_num);

// 전역 변수
// 입출력장치
int dipsw;
int leds;
int dot_mtx;
int tactsw;
int clcds;
int fnds;
int selected_tact[3];
int random_number[3];

// 재시작했을 때 룰 출력 안 하기 위한 룰 카운트 생성
int rule_count = 1;

// 힌트 횟수 전역변수 설정
int hint_count[2];

// ChipLED 16진수 값
char led_array[] = { 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00};

//카드 관련 전역 변수 설정
#define CARD_NUM 13
int usercards[CARD_NUM] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
int comcards[CARD_NUM] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

// fnd led 관련 전역 변수 설정
unsigned char fnd_num[4] = {0,};

//0,1,2,3,4,5,6,7,8,9,turn off
unsigned char Time_Table[11] = {~0x3f, ~0x06, ~0x5b, ~0x4f, ~0x66, ~0x6d, ~0x7d, ~0x07, ~0x7f, ~0x67, ~0x00};

//dot matrix로 표현한 트럼프 카드
unsigned char deck[13][8] = {
    // Number 1 (A)
    {0x18, 0x24, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42},
    // Number 2
    {0x1C, 0x22, 0x02, 0x02, 0x1C, 0x20, 0x20, 0x3E},
    // Number 3
    {0x1C, 0x22, 0x02, 0x1C, 0x02, 0x02, 0x22, 0x1C},
    // Number 4
    {0x04, 0x0C, 0x14, 0x24, 0x44, 0x3E, 0x04, 0x04},
    // Number 5
    {0x1E, 0x20, 0x20, 0x1C, 0x02, 0x02, 0x22, 0x1C},
    // Number 6
    {0x3C, 0x40, 0x40, 0x7C, 0x42, 0x42, 0x42, 0x3C},
    // Number 7
    {0x3E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02},
    // Number 8
    {0x1C, 0x22, 0x22, 0x1C, 0x22, 0x22, 0x22, 0x1C},
    // Number 9
    {0x1C, 0x22, 0x22, 0x22, 0x1E, 0x02, 0x02, 0x02},
    // Number 10
    {0x84, 0x8A, 0x91, 0x91, 0x91, 0x91, 0x8A, 0x84},
    // Number 11 (J)
    {0x1C, 0x08, 0x08, 0x08, 0x08, 0x48, 0x48, 0x30},
    // Number 12 (Q)
    {0x38, 0x44, 0x82, 0x82, 0x82, 0x8A, 0x44, 0x3A},
    // Number 13 (K)
    {0x44, 0x48, 0x50, 0x60, 0x50, 0x48, 0x44, 0x44}
};

// 입출력 함수
void print(char P[]){
  clcds = open(clcd, O_RDWR);
  if(clcds < 0){printf("Can't open Character LCD.\n"); exit(0);}
  write(clcds, P, strlen(P));
  close(clcds);
}

void writeToDotDevice(int card, int time) {
  int dot_mtx = open(dot, O_RDWR);
  if (dot_mtx < 0) {
    printf("Cannot open dot device\n");
    exit(0);
  }
  write(dot_mtx, &deck[card-1], sizeof(deck[card-1]));
  usleep(time);
  close(dot_mtx);
}

int dipsw_get_with_timer(int t_second) {   
  int selected_dip = 0;
  unsigned char d=0;
  int dipsw; 
   
  //dip switch 제한 시간이 0초 이하일 경우 입력값 없음 
  if(t_second <= 0){
    return 0;
  }
	
  if((dipsw = open( dip , O_RDWR )) < 0){        // 예외처리    
    printf("dip open error");
    return 0;
  }
  if((fnds = open(fnd,O_RDWR)) <0){
    printf("fnd open error");
    return 0;
  }
   
  int i,j;

  //i=10~0초까지 
  for(i = t_second; i>-1;i--){
    for(j = 100; j>0;j--){
      usleep(10000); //0.01 초 쉬기 
      read(dipsw, &d, sizeof(d));
      //입력값이 1~128 사이일 때 
      if(1<=d && d<=128){
        selected_dip = d;
        //close 전 fnd 초기화 
        int turnOff = Time_Table[10];
        fnd_num[0] = turnOff;
        fnd_num[1] = turnOff;
        fnd_num[2] = turnOff;
        fnd_num[3] = turnOff;

        write(fnds, &fnd_num, sizeof(fnd_num));
        close(dipsw);
        close(fnds);
        return selected_dip;
      }
    }
		//1초 지남 = 0.01초*100번 
    int s = i / 10;
    int ss = i % 10;
    fnd_num[0] = Time_Table[0];
    fnd_num[1] = Time_Table[0];
    fnd_num[2] = Time_Table[s];
    fnd_num[3] = Time_Table[ss];
    write(fnds, &fnd_num, sizeof(fnd_num));
  }
  //close 전 fnd 초기화 
  int turnOff = Time_Table[10];
  fnd_num[0] = turnOff;
  fnd_num[1] = turnOff;
  fnd_num[2] = turnOff;
  fnd_num[3] = turnOff;

  write(fnds, &fnd_num, sizeof(fnd_num));
  close(dipsw);
  close(fnds);
  return 0; //제한시간 끝   
}

int* tactsw_get_with_timer(int t_second){   
  int tactnum = 0;
  unsigned char b=0;
  int tactsw; 
   
  //tact switch 제한 시간이 0초 이하일 경우 입력값 없음 
  if(t_second <= 0){
    return 0;
  }
   
  if((tactsw = open( tact, O_RDWR )) < 0){        	// 예외처리    
    printf("tact open error");
    return 0;
  }

  if((fnds = open(fnd,O_RDWR)) <0){                     // 예외처리 
    printf("fnd open error");
    return 0;
  }
   
  int i,j;
  //i=10~0초까지
  for(i = t_second; i>-1;i--){
    for(j = 100; j>0;j--){
      usleep(10000); //0.01 초 쉬기 
      read(tactsw, &b, sizeof(b));
      //입력값이 1~12 사이일 경우            
      if(1<=b && b<=12){
        if (tactnum < 3){
            switch (b){
                case 1:  selected_tact[tactnum] = 1 ; tactnum++; break;
                case 2:  selected_tact[tactnum] = 2 ; tactnum++; break;
                case 3:  selected_tact[tactnum] = 3 ; tactnum++; break;
                case 4:  selected_tact[tactnum] = 4 ; tactnum++; break;
                case 5:  selected_tact[tactnum] = 5 ; tactnum++; break;
                case 6:  selected_tact[tactnum] = 6 ; tactnum++; break;
                case 7:  selected_tact[tactnum] = 7 ; tactnum++; break;
                case 8:  selected_tact[tactnum] = 8 ; tactnum++; break;
                case 9:  selected_tact[tactnum] = 9 ; tactnum++;break;
                case 10:  selected_tact[tactnum] = 0 ; tactnum++; break;                
            }
        }
          
        if (b == 12){
            //12눌렀을 때 이전에 4개의 숫자를 입력했을 경우
            if(tactnum == 3){
              //printf("tactswitch 입력값: %d\n", selected_tact[4]);
              int turnOff = Time_Table[10];
              fnd_num[0] = turnOff;
              fnd_num[1] = turnOff;
              fnd_num[2] = turnOff;
              fnd_num[3] = turnOff;
              write(fnds, &fnd_num, sizeof(fnd_num));
              close(tactsw);
              close(fnds);
              //return selected_tact;
            }
            //12를 눌렀지만 이전에 4개의 숫자를 입력X 
            else {
              printf("press 12 after  press 3 numbers");                              
            }
        }

          //11무시 
        else {
            printf("press other key"); break;
        }
      }
    }
    //1초 지남 = 0.01초*100번 
    int s = i / 10;
    int ss = i % 10;
    fnd_num[0] = Time_Table[0];
    fnd_num[1] = Time_Table[0];
    fnd_num[2] = Time_Table[s];
    fnd_num[3] = Time_Table[ss];
    write(fnds, &fnd_num, sizeof(fnd_num));
  }
  //close 전 fnd 초기화 
  int turnOff = Time_Table[10];
  fnd_num[0] = turnOff;
  fnd_num[1] = turnOff;
  fnd_num[2] = turnOff;
  fnd_num[3] = turnOff;
  write(fnds, &fnd_num, sizeof(fnd_num));
  						
  close(tactsw);
  close(fnds);
  return (int*) selected_tact; //제한시간 끝, 배열 반환
}

void led_on(int user_score){
  unsigned char data;

  // chip led 불러오기
  leds = open(led, O_RDWR);
  if (leds < 0) {
    printf("Can't open LED.\n");
    exit(0);
  }

  data = led_array[user_score];
  
  // 5초동안 출력
  write(leds, &data, sizeof(unsigned char));  usleep(5000000);

  close(leds);
}


// 게임 진행 전 함수
int intro_key(){
  int dip_value = 0;
    
  char first_msg[] = " PRESS ANY KEY!  USE DIP SWITCH ";
  char second_msg[] = " PRESS ANY KEY!  NO INPUT: QUIT ";
   
  //게임시작여부 묻기(첫번째  메시지로) 
  dip_value = intro(first_msg);
   
  //dip switch 입력 있으면 입력값 반환 
  if( dip_value != 0 ) return dip_value;
   
  //dip switch 입력 없으면 게임시작여부 묻기(두번째 메시지로) 
  dip_value = intro(second_msg);
   
  return dip_value;
}

int intro(char P[]){
   
  //clcd에 인트로 메시지 출력 
  print(P);
   
  // dip switch 10초 동안 입력했냐 안 했냐
  int dip_value = 0;
  dip_value =  dipsw_get_with_timer(10); 
  printf("dip value: %d\n", dip_value);
   
  return dip_value;
}


void generate_random_number(){
    int used[10] = {0};  // 숫자가 사용되었는지 체크하는 배열;
    for (int i = 0; i < 3; i++) {
        int digit;
        do {
            digit = rand() % 10;
        } while (used[digit]);

        random_number[i] = digit;
        used[digit] = 1;
    }
}

void prepare(int* cards1, int* cards2){
  //shuffle_card(0,cards1);
  //shuffle_card(0,cards2);
  //shuffle_card(0,cards2); // 다르게 섞이기 위해 한 번 더 셔플
}
 
void game_rule(){
  print("  NUMBER BASEBALL RULE   ");  usleep(1500000);
  print("     ON THE       TACT  SWITCH  ");  usleep(1500000);
  print("1ST, 2ND, 3RD IS BETTING BUTTON ");  usleep(1500000);
  print("   1ST BUTTON     PLAYER = COM  ");  usleep(1500000);
  print("   2ND BUTTON     PLAYER < COM  ");  usleep(1500000);
  print("   3RD BUTTON     PLAYER > COM  ");  usleep(1500000);
  print("  4TH, 5TH  IS    HINT  BUTTON  ");  usleep(1500000);
  print("  THE  HINT IS    GIVEN  TWICE  ");  usleep(1500000);
  print("   4TH BUTTON   SHOW UNUSED CARD");  usleep(1500000);
  print("   5TH BUTTON    SHOW USED CARD ");  usleep(1500000);
  print("  12TH  BUTTON       CHOOSE     ");  usleep(1500000);
}



// 게임 진행 함수
void start(){
  int ROUND = 9;
  int com_score = 0;
  int user_score = 0;
  char round_clcd[32];
  char score_clcd[32];

  int number[3];
  int guess[3];
  int strikes = 0;
  int balls = 0;
  int attempts = 0;

  int com_num[3];

  generate_random_number();
  for (int i=0; i < 3; i++){
      com_num[i] = random_number[i];
  }

  print("      GAME           START!     ");  usleep(1500000);

  // 첫 판인 경우 Game Rule 출력
  if (rule_count >= 1){
    // Game Rule 설명
    game_rule();

    rule_count = rule_count - 1;
  }

  // Round 반복
  int i;
  for(i = 0; i < ROUND; i++){
    //int com_num[4] = generate_random_number();
    //int user_card = cards2[i];
    
    // teraterm으로 해당 라운드 숫자 확인
    printf("com_num: %d\n", com_num[3]);
    //printf("user_card: %d\n", user_card);
    
    // CLCD로 라운드 출력
    sprintf(round_clcd, "     ROUND  %d       START!     ", i+1);
    print(round_clcd);  usleep(2000000);

    // betting_start 함수 호출해 user_answer에 베팅 값 저장
    int* user_answer = betting_start(i);         // 베팅 값 저장
      
    print("  BETTING DONE ");

    // 사용자 카드 공개(3초)
    //writeToDotDevice(user_card, 3000000);

    // 카드 비교 결과 저장
    int* SBO = compare_num(com_num, user_answer);    
    
    // 베팅 결과 확인
    if(SBO[0] == 3){           
      user_score++;
      print("     3 STRIKES!!!     "); usleep(2000000);
    }
    else if(SBO[0] == 0 && SBO[1] == 0){
      print("     OUT!!!      "); usleep(2000000);
    }
    else{
        sprintf(score_clcd, "   %d STRIKE   %d BALL ", SBO[0], SBO[1]);
    }
    
    // 스코어 공개와 동시에 CHIP LED 키기(5초로 설정되어 있음)
    sprintf(score_clcd, "PLAYER SCORE = %d ", user_score);
    print(score_clcd);
    led_on(user_score);

    if(user_score>=1){
      print("   GAME CLEAR      PLAYER WIN   ");   usleep(2000000);
      break;
    }
    if(com_score>=5){
      print("   GAME  OVER     PLAYER  LOSE  ");   usleep(2000000);
      break;
    }
  }
}

int* compare_num(int* com_num, int* user_num){
    int SBO[3] = {0,0,0};
    for (int i = 0; i < 3; ++i) {
        if (com_num[i] == user_num[i]) {
            (SBO[0])++;
        } 
        else {
            for (int j = 0; j < 3; ++j) {
                if (com_num[i] == user_num[i]) {
                    (SBO[1])++;
                }
            }
        }
    }
}

int* betting_start(int round){
  // COM 카드 확인 문구 
  //print("   CHECK  THE       COM CARD    ");

  // COM 카드 출력 
  //writeToDotDevice(com_card, 3000000);
    
  while(1){
    print(" ANSWERING USE TACTSWITCH ");
      
    // tactswitch 베팅 입력, fnd 10초 시작
    int* user_answer = tactsw_get_with_timer(10); 
      
    //유저가 베팅했을 경우 
    //int user_bet = user_answer ==0 ||user_answer ==1 || user_answer ==2 ||user_answer ==3;
      
    //유저가 힌트를 요청했을 경우 
    //int user_hint = user_answer == 4 || user_answer == 5;

    //int user_rule = user_answer == 6;

    //베팅값 입력시 베팅값 반환 
    //if(user_bet){
      //int* bet_answer = user_answer;
      //return (int*) bet_answer; 
    //}

    //힌트 요청시 베팅 재진행 
    //else if(user_hint){
      //요청한 힌트 4의 잔여 힌트 남아있을 시 
      //if(user_answer == 4 && hint_count[0] >= 1){
        //힌트 함수 호출
        //hint(4,cards2, round);
        //hint_count[0]--;
      //}

      //요청한 힌트 5의 잔여 힌트 남아있을 시 
      //else if(user_answer == 5 && hint_count[1] >= 1){
        //힌트 함수 호출 
        //hint(5,cards2, round);
        //hint_count[1]--;
      //}

      //잔여 힌트 남아있지 않을 시 
      //else{
        //print(" HINT COUNT = 0  CAN'T USE HINT ");   usleep(2000000);
      //}
    //}

    //else if(user_rule){
      //game_rule();
    //}
  }   
}

void ascending(int arr[], int cnt) {        
  int i, j, tmp = 0;
  for (i = 0; i < cnt; i++) {
    for (j = i; j < cnt; j++) {
      if (arr[i] > arr[j]) {
        tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
      }
    }
  }
}

void hint(int user_answer, int* user_card, int round) {
  int hint_result[13] = {};

   // 4인 경우 해당 라운드 카드부터 안쓴 카드까지 쭉 출력
  if (user_answer == 4) {
    int j=round;
    print(" DISPLAY UNUSED   PLAYER  CARD  ");
      
    int cnt = 0;

    // 해당 라운드 카드부터 카드 배열 크기만큼까지 저장
    for (round; round < 13; round++) {
      hint_result[cnt] = user_card[round];
      cnt = cnt + 1;
    }
  
    // 힌트 저장되어 있는 배열 오름차순 정렬
    ascending(hint_result, 13-j);
      
    int k;
    for(k=0; k < 13-j; k++){
      writeToDotDevice(hint_result[k], 1500000);
    }   
  }

   // 5인 경우 지금까지 사용한 카드를 출력
  else if (user_answer == 5) {
    int j;
    print("  DISPLAY USED    PLAYER  CARD  ");
    for (j = 0; j < round; j++) {
      int card = user_card[j];
      writeToDotDevice(card, 1500000);
    }
  }
}

int compare_card(int com_card, int user_card){
  if(com_card==user_card){
    return 1;
  }
  else if(com_card>user_card){
    return 2;
  }
  else{
    return 3;
  }
}
    
int win_lose(int user_answer, int correct_answer){
  if(user_answer == correct_answer){
    return 1;
  }
  else{
    return 0;
  }
}

int main(){
  while(1){
    if(intro_key() != 0){
      //hint_count[0] = 1;
      //hint_count[1] = 1; 
      //prepare(usercards, comcards);
      start();
      print("    CONTINUE         GAME ?     ");   usleep(2000000);

      if (intro(" DIP SWITCH  ON NEW GAME STARTS!") == 0){
        print("      GAME            END.      ");   usleep(2000000);
        return 0;
      }
    }
    else{

    }
  }
}
