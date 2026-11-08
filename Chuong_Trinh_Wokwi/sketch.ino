/*
  ĐỒ ÁN: HỆ THỐNG KIỂM SOÁT CỬA THÔNG MINH ESP32 + MÃ PIN (Wokwi)
  Board: ESP32 DevKit V1 | Thư viện: Keypad, ESP32Servo
  CHỈ DÙNG CHO HỌC TẬP - KHÔNG PHẢI HỆ THỐNG AN NINH THẬT.
*/

#include <Keypad.h>
#include <ESP32Servo.h>

// ===================== 1. CẤU HÌNH (hằng số) =====================
// --- Chân GPIO ---
const byte GPIO_SERVO     = 18;
const byte GPIO_LED_GREEN = 4;
const byte GPIO_LED_RED   = 5;
const byte GPIO_BUZZER    = 23;

// --- Servo ---
const int ANGLE_LOCKED = 0;    // góc khi cửa khóa
const int ANGLE_OPEN   = 90;   // góc khi cửa mở

// --- PIN ---
const char CORRECT_PIN[] = "1234";  // PIN thử nghiệm (không in ra Serial)
const byte PIN_LENGTH    = 4;       // PIN dài đúng 4 số

// --- Thời gian và giới hạn ---
const unsigned long DOOR_OPEN_MS         = 5000UL;   // giữ cửa mở 5 giây
const unsigned long LOCKOUT_MS           = 30000UL;  // khóa tạm thời 30 giây
const unsigned long LOCKOUT_REPORT_MS    = 5000UL;   // báo thời gian còn lại mỗi 5 giây
const byte          MAX_WRONG_ATTEMPTS   = 3;        // sai tối đa 3 lần liên tiếp
const unsigned int  BUZZER_HALF_PERIOD_US = 250;     // nửa chu kỳ sóng vuông (~2 kHz)

// ===================== 2. KEYPAD =====================
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {13, 14, 16, 17};   // R1..R4
byte colPins[COLS] = {25, 26, 27, 32};   // C1..C4
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ===================== 3. BIẾN TRẠNG THÁI =====================
enum SystemState { STATE_LOCKED, STATE_INPUTTING, STATE_UNLOCKED, STATE_LOCKOUT };
SystemState systemState = STATE_LOCKED;   // trạng thái hiện tại của hệ thống

Servo doorServo;

char inputBuffer[PIN_LENGTH + 1];   // bộ đệm PIN (+1 cho ký tự kết thúc '\0')
byte inputLength = 0;               // số chữ số đã nhập
byte wrongCount  = 0;               // số lần nhập sai liên tiếp

unsigned long unlockStartMs      = 0;   // thời điểm bắt đầu mở cửa
unsigned long lockoutStartMs     = 0;   // thời điểm bắt đầu khóa tạm thời
unsigned long lastLockoutReportMs = 0;  // lần cuối báo thời gian còn lại

// --- Biến cho buzzer không chặn ---
bool beepActive = false;        // đang trong chuỗi bíp
bool beepSounding = false;      // đang kêu (true) hay đang nghỉ (false)
bool buzzerLevel = false;       // mức hiện tại của chân buzzer
unsigned int beepOnMs = 0, beepOffMs = 0;
byte beepRemaining = 0;
unsigned long beepPhaseStartMs = 0;
unsigned long lastToggleUs = 0;

// ===================== 4. KHAI BÁO HÀM =====================
void processKey(char key);
void confirmPIN();
bool checkPIN();
void unlockDoor();
void lockDoor();
void handleWrongPIN();
void startLockout();
void updateLockout();
void autoCloseDoor();
void showStatus();
void clearInput();
void startBeep(unsigned int onMs, unsigned int offMs, byte count);
void updateBuzzer();
const char* stateName();

// ===================== 5. SETUP =====================
void setup() {
  Serial.begin(115200);

  pinMode(GPIO_LED_GREEN, OUTPUT);
  pinMode(GPIO_LED_RED, OUTPUT);
  pinMode(GPIO_BUZZER, OUTPUT);
  digitalWrite(GPIO_BUZZER, LOW);

  ESP32PWM::allocateTimer(0);
  doorServo.setPeriodHertz(50);
  doorServo.attach(GPIO_SERVO, 500, 2400);

  clearInput();
  wrongCount = 0;
  lockDoor();   // servo 0°, LED đỏ sáng, LED xanh tắt, trạng thái LOCKED

  Serial.println("========================================");
  Serial.println("  HE THONG KIEM SOAT CUA THONG MINH");
  Serial.println("  ESP32 + Keypad + Servo + LED + Buzzer");
  Serial.println("========================================");
  showStatus();
}

// ===================== 6. LOOP =====================
void loop() {
  char key = keypad.getKey();
  if (key) {
    processKey(key);
  }

  // Xử lý tự động đóng cửa
  if (systemState == STATE_UNLOCKED) {
    autoCloseDoor();
  }

  // Xử lý khóa tạm thời
  if (systemState == STATE_LOCKOUT) {
    updateLockout();
  }

  // Cập nhật buzzer không chặn
  updateBuzzer();
}

// ===================== 7. XỬ LÝ PHÍM =====================
void processKey(char key) {
  if (systemState == STATE_LOCKOUT) {
    Serial.println("[KHÓA] Hệ thống đang khóa tạm thời. Không nhận phím.");
    return;
  }

  if (systemState == STATE_UNLOCKED) {
    // Khi cửa đang mở, chỉ cho phép * để đóng sớm (tùy chọn)
    if (key == '*') {
      lockDoor();
      Serial.println("[ĐÓNG] Đóng cửa thủ công bằng *");
    }
    return;
  }

  // STATE_LOCKED hoặc STATE_INPUTTING
  if (key >= '0' && key <= '9') {
    if (inputLength < PIN_LENGTH) {
      inputBuffer[inputLength++] = key;
      inputBuffer[inputLength] = '\0';
      systemState = STATE_INPUTTING;
      Serial.print("[NHẬP] ");
      for (byte i = 0; i < inputLength; i++) Serial.print('*');
      Serial.println();
    }
  }
  else if (key == 'A') {   // Xác nhận
    confirmPIN();
  }
  else if (key == '*') {   // Xóa
    clearInput();
    systemState = STATE_LOCKED;
    Serial.println("[XÓA] Đã xóa bộ đệm PIN");
    showStatus();
  }
}

void confirmPIN() {
  if (inputLength == 0) {
    Serial.println("[LỖI] Chưa nhập PIN");
    return;
  }

  if (checkPIN()) {
    wrongCount = 0;
    unlockDoor();
  } else {
    handleWrongPIN();
  }
  clearInput();
}

bool checkPIN() {
  if (inputLength != PIN_LENGTH) return false;
  for (byte i = 0; i < PIN_LENGTH; i++) {
    if (inputBuffer[i] != CORRECT_PIN[i]) return false;
  }
  return true;
}

// ===================== 8. MỞ / ĐÓNG CỬA =====================
void unlockDoor() {
  doorServo.write(ANGLE_OPEN);
  digitalWrite(GPIO_LED_GREEN, HIGH);
  digitalWrite(GPIO_LED_RED, LOW);
  systemState = STATE_UNLOCKED;
  unlockStartMs = millis();
  startBeep(100, 50, 2);   // 2 tiếng bíp ngắn
  Serial.println("[MỞ] PIN đúng! Cửa đang mở (5 giây)...");
  showStatus();
}

void lockDoor() {
  doorServo.write(ANGLE_LOCKED);
  digitalWrite(GPIO_LED_GREEN, LOW);
  digitalWrite(GPIO_LED_RED, HIGH);
  systemState = STATE_LOCKED;
  clearInput();
}

void autoCloseDoor() {
  if (millis() - unlockStartMs >= DOOR_OPEN_MS) {
    lockDoor();
    Serial.println("[TỰ ĐÓNG] Hết thời gian mở cửa. Đã khóa lại.");
    showStatus();
  }
}

// ===================== 9. XỬ LÝ SAI PIN =====================
void handleWrongPIN() {
  wrongCount++;
  startBeep(200, 100, 3);   // 3 tiếng bíp dài hơn
  Serial.print("[SAI] PIN không đúng. Lần sai: ");
  Serial.print(wrongCount);
  Serial.print("/");
  Serial.println(MAX_WRONG_ATTEMPTS);

  if (wrongCount >= MAX_WRONG_ATTEMPTS) {
    startLockout();
  } else {
    systemState = STATE_LOCKED;
    showStatus();
  }
}

void startLockout() {
  systemState = STATE_LOCKOUT;
  lockoutStartMs = millis();
  lastLockoutReportMs = millis();
  digitalWrite(GPIO_LED_RED, HIGH);
  digitalWrite(GPIO_LED_GREEN, LOW);
  Serial.println("[KHÓA] Sai 3 lần liên tiếp. Hệ thống khóa 30 giây!");
  startBeep(300, 150, 5);
}

void updateLockout() {
  unsigned long elapsed = millis() - lockoutStartMs;
  if (elapsed >= LOCKOUT_MS) {
    wrongCount = 0;
    systemState = STATE_LOCKED;
    Serial.println("[MỞ KHÓA] Hết thời gian khóa. Hệ thống hoạt động lại.");
    showStatus();
  } else if (millis() - lastLockoutReportMs >= LOCKOUT_REPORT_MS) {
    lastLockoutReportMs = millis();
    unsigned long remain = (LOCKOUT_MS - elapsed) / 1000;
    Serial.print("[KHÓA] Còn ");
    Serial.print(remain);
    Serial.println(" giây...");
  }
}

// ===================== 10. HỖ TRỢ =====================
void clearInput() {
  inputLength = 0;
  inputBuffer[0] = '\0';
}

const char* stateName() {
  switch (systemState) {
    case STATE_LOCKED:    return "LOCKED";
    case STATE_INPUTTING: return "INPUTTING";
    case STATE_UNLOCKED:  return "UNLOCKED";
    case STATE_LOCKOUT:   return "LOCKOUT";
    default:              return "UNKNOWN";
  }
}

void showStatus() {
  Serial.print("[TRẠNG THÁI] ");
  Serial.print(stateName());
  Serial.print(" | Số lần sai: ");
  Serial.println(wrongCount);
  if (systemState == STATE_LOCKED) {
    Serial.println("Đang chờ nhập PIN...");
  }
}

// ===================== 11. BUZZER KHÔNG CHẶN =====================
// Phát 'count' tiếng bíp, mỗi tiếng kêu onMs, nghỉ offMs
void startBeep(unsigned int onMs, unsigned int offMs, byte count) {
  beepOnMs = onMs;
  beepOffMs = offMs;
  beepRemaining = count;
  beepActive = true;
  beepSounding = true;
  beepPhaseStartMs = millis();
  lastToggleUs = micros();
}

void updateBuzzer() {
  if (!beepActive) return;
  unsigned long nowMs = millis();

  if (beepSounding) {
    if (nowMs - beepPhaseStartMs >= beepOnMs) {
      digitalWrite(GPIO_BUZZER, LOW);
      buzzerLevel = false;
      beepSounding = false;
      beepPhaseStartMs = nowMs;
      if (beepRemaining > 0) beepRemaining--;
      if (beepRemaining == 0) beepActive = false;
    } else {
      unsigned long nowUs = micros();
      if (nowUs - lastToggleUs >= BUZZER_HALF_PERIOD_US) {
        lastToggleUs = nowUs;
        buzzerLevel = !buzzerLevel;
        digitalWrite(GPIO_BUZZER, buzzerLevel);   // tạo sóng vuông ~2 kHz
      }
    }
  } else {
    if (nowMs - beepPhaseStartMs >= beepOffMs) {
      beepSounding = true;
      beepPhaseStartMs = nowMs;
      lastToggleUs = micros();
    }
  }
}
