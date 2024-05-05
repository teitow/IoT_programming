# Hybus-Smart4412를 이용한 숫자야구게임

# 숫자 야구 게임

## 게임 시작 방법
- **게임 시작**: 시작 버튼을 눌러 게임을 시작합니다.

## 게임 플레이
1. **숫자 설정**:
   - 플레이어 1이 Tact Switch를 사용하여 숨겨진 숫자를 설정합니다.
   - 설정된 숫자는 자동으로 숨겨집니다.

2. **숫자 추측**:
   - 플레이어 2가 Tact Switch를 사용하여 추측할 숫자를 입력합니다.
   - 추측 버튼을 눌러 숫자를 제출합니다.

3. **결과 계산 및 표시**:
   - 기기가 입력된 숫자와 설정된 숫자를 비교합니다.
   - 결과(스트라이크, 볼, 아웃)를 계산하고 Character LCD에 표시합니다.

4. **LED 상태 표시**:
   - 스트라이크: 파란색 LED가 점등됩니다.
   - 볼: 노랑색 LED가 점등됩니다.
   - 아웃: 빨간색 LED가 점등됩니다.

5. **턴 변경 및 반복**:
   - 결과 확인 후 플레이어의 턴을 교체합니다.
   - 아웃이나 스트라이크가 모두 기록될 때까지 숫자 추측으로 돌아갑니다.

## 승리 조건
- 설정된 숫자가 모두 맞춰졌는지 검사합니다.
- 승리 조건을 충족 시 승리 표시를 FND에 표시합니다.

## 게임 종료 및 재시작
- 게임 종료 버튼을 누르거나, 재시작 버튼을 눌러 새 게임을 시작할 수 있습니다.

## 게임 플로우차트
<img src="https://github.com/teitow/IoT_programming/blob/main/image/%EC%88%AB%EC%9E%90%EC%95%BC%EA%B5%AC%EA%B2%8C%EC%9E%84%ED%94%8C%EB%A1%9C%EC%9A%B0%EC%B0%A8%ED%8A%B8.drawio.png" width="50%" height="50%">


