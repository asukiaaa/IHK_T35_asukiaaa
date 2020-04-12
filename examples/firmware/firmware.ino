#include <Arduino.h>
#include <IHK_T35_asukiaaa.h>
#include <Wire.h>

#define PIN_LED_GND 2
#define PIN_CLK 3
#define PIN_AB 4
#define PIN_SIG_1 A0
#define PIN_SIG_2 A1
#define PIN_SIG_3 A2
#define PIN_SIG_4 A3
#define PIN_SIG_5 10
#define PIN_SIG_6 11
#define PIN_SIG_7 12
#define PIN_SIG_8 13
#define PIN_POWER_NORMAL 8
#define PIN_POWER_OIL 7
#define PIN_POWER_UP 6
#define PIN_POWER_DOWN 5
#define PIN_ANALOG_THERMISTOR 6 // analog pin

#define BUFFER_MS_TO_DETECT_OFF 1000UL

#define PIN_ADDR0 0
#define PIN_ADDR1 1
#define ANALOG_MAX 1023

#define DEBUG

// 0: plotocol version
// 1: read powerOnOil[7], powerOnNormal[6], error[5-3], level[2-0]
// 2: thermistor voltage low byte
// 3: thermistor voltage high byte
// 4: write powerOnOil[7], powerOnNormal[6], level[2-0]
#define REGISTER_LEN 5

#define BUTTON_PRESS_INTERVAL_MS 200UL
#define PROTOCOL_VERSION 1

uint8_t registers[REGISTER_LEN];
uint8_t registerIndex = 0;
bool handledWriteRegister = true;

class PowerState {
  public:
  unsigned long changedAt;
  bool powerNormal = false;
  bool powerOil = false;
  uint8_t error = 0;
  unsigned long readErrorAt;
  uint8_t level = 0;

  // TODO compare prev led state and current led state to detect error

  PowerState() {
    changedAt = 0;
    powerNormal = 0;
    powerOil = false;
    readErrorAt = 0;
    level = 0;
  };

  void updateChangedAt() {
    changedAt = millis();
  }

  void updateByReadingPins() {
    bool level1 = digitalRead(PIN_SIG_1);
    bool level2 = digitalRead(PIN_SIG_2);
    bool level3 = digitalRead(PIN_SIG_3);
    bool level4 = digitalRead(PIN_SIG_4);
    bool level5 = digitalRead(PIN_SIG_5);
    bool level6 = digitalRead(PIN_SIG_6);
    bool powerNormal = digitalRead(PIN_SIG_7);
    bool powerOil = digitalRead(PIN_SIG_8);

    int level = 0;
    if (level6 && !level5) level = 0; // check 5 to detect error
    else if (level6) level = 6;
    else if (level5) level = 5;
    else if (level4) level = 4;
    else if (level3) level = 3;
    else if (level2) level = 2;
    else if (level1) level = 1;

    if (level1 && !level2 && level6) {
      this->readErrorAt = millis();
      if (this->error == 0) {
        this->error = 1;
        this->updateChangedAt();
      }
    } else if (powerOil) {
      if (!this->powerOil || this->level != level) {
        this->powerOil = true;
        this->powerNormal = false;
        this->level = level;
        this->error = 0;
        this->updateChangedAt();
      }
    } else if (powerNormal) {
      if (!this->powerNormal || this->level != level) {
        this->powerOil = false;
        this->powerNormal = true;
        this->level = level;
        this->error = 0;
        this->updateChangedAt();
      }
    } else {
      // LEDが消えていてもエラー時の点滅の場合があるので、一定時間LEDが消えていたら電源OFFと判定する
      if ((this->powerNormal || this->powerOil) && (this->error == 0 || millis() - this->readErrorAt > BUFFER_MS_TO_DETECT_OFF)) {
        this->powerOil = false;
        this->powerNormal = false;
        this->error = 0;
        this->updateChangedAt();
      }
    }
  }

  void printInfo(HardwareSerial *serial) {
    String powerStr = "power: ";
    powerStr += powerNormal ? "on normal" : powerOil ? "on oil" : "off";
    serial->println(powerStr);
    serial->println("level: " + String(level));
    serial->println("error: " + String(this->error));
    serial->println("changedAt: " + String(this->changedAt));
  }
};

PowerState powerState = PowerState();

void onFallingLedGnd() {
  powerState.updateByReadingPins();
}

void onReceive(int howMany) {
#ifdef DEBUG
  Serial.print("onReceive:");
#endif
  uint8_t receivedLen = 0;
  boolean changedLedsRegister = false;
  while (0 < Wire.available()) {
    uint8_t v = Wire.read();
#ifdef DEBUG
    Serial.print(" ");
    Serial.print(v, HEX);
#endif
    if (receivedLen == 0) {
      registerIndex = v;
    } else {
      if (registerIndex == IHK_T35_ASUKIAAA_REGISTER_ADDR_WRITE) {
        registers[registerIndex] = v;
        handledWriteRegister = false;
        Serial.println("update write register");
      }
      ++registerIndex;
    }
    ++receivedLen;
  }
#ifdef DEBUG
  Serial.println("");
#endif
}

void onRequest() {
  if (registerIndex < REGISTER_LEN) {
    Wire.write(&registers[registerIndex], REGISTER_LEN - registerIndex);
  } else {
    Wire.write(0);
  }
}

void updateRegistersToRead() {
  uint32_t thermistorSum = 0;
  static const uint8_t sumNumber = 10;
  for (uint8_t i = 0; i < sumNumber; ++i) {
    thermistorSum += analogRead(PIN_ANALOG_THERMISTOR);
  }
  uint16_t thermistorValue = thermistorSum / sumNumber;

  powerState.updateByReadingPins();
  uint8_t r = 0;
  if (powerState.powerOil) {
    r += 0b10000000;
  }
  if (powerState.powerNormal) {
    r += 0b01000000;
  }
  r += ((powerState.error & 0b111) << 3);
  r += (powerState.level & 0b111);
  registers[0] = PROTOCOL_VERSION;
  registers[1] = r;
  registers[2] = (uint8_t) (thermistorValue & 0xff);
  registers[3] = (uint8_t) (thermistorValue >> 8);
}

void setup() {
  pinMode(PIN_CLK, INPUT);
  pinMode(PIN_AB, INPUT);
  pinMode(PIN_LED_GND, INPUT);
  pinMode(PIN_SIG_1, INPUT);
  pinMode(PIN_SIG_2, INPUT);
  pinMode(PIN_SIG_3, INPUT);
  pinMode(PIN_SIG_4, INPUT);
  pinMode(PIN_SIG_5, INPUT);
  pinMode(PIN_SIG_6, INPUT);
  pinMode(PIN_SIG_7, INPUT);
  pinMode(PIN_SIG_8, INPUT);
  pinMode(PIN_POWER_NORMAL, OUTPUT);
  pinMode(PIN_POWER_OIL, OUTPUT);
  pinMode(PIN_POWER_DOWN, OUTPUT);
  pinMode(PIN_POWER_UP, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_LED_GND), onFallingLedGnd, FALLING);

  int address = 0x18;
#ifdef DEBUG
  Serial.begin(9600);
#else
  pinMode(PIN_ADDR0, INPUT_PULLUP);
  pinMode(PIN_ADDR1, INPUT_PULLUP);
  delay(1);
  if (digitalRead(PIN_ADDR0) == LOW) {
    address += 1;
  }
  if (digitalRead(PIN_ADDR1) == LOW) {
    address += 2;
  }
#endif
  updateRegistersToRead();
  registers[IHK_T35_ASUKIAAA_REGISTER_ADDR_WRITE] = 0;

  Wire.begin(address);

  // Disable pull up for I2C pins
  pinMode(SDA, INPUT);
  pinMode(SCL, INPUT);

  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
}

void switchPower(int pin) {
  digitalWrite(pin, HIGH);
  delay(100);
  digitalWrite(pin, LOW);
  delay(10);
}

#ifdef DEBUG
void printInfo() {
  powerState.printInfo(&Serial);
  uint16_t v = analogRead(PIN_ANALOG_THERMISTOR);
  // float c = voltToTemperature(v);
  // Serial.println("Thermistor volt: " + String(v) + "v");
  // Serial.println("Temperature: " + String(c) + "C");
  Serial.println("analog: " + String(v));
  float volt = ((float) v) * 5 / ANALOG_MAX;
  Serial.println("volt: " + String(volt));
  Serial.println("at " + String(millis()));
  Serial.println();
}
#endif

void updateByTargetValues() {
  // static unsigned long handledAt = 0;
  // static bool neededToWait = false;
  // static unsigned long controlCount = 0;

  // if (neededToWait) {
  //   if (millis() - handledAt < BUTTON_PRESS_INTERVAL_MS) {
  //     return;
  //   } else {
  //     neededToWait = false;
  //   }
  // }

  // Serial.println("handledWriteRegister:" + String(handledWriteRegister ? "YES" : "NO"));

  if (handledWriteRegister) return;
  if (powerState.error != 0) {
    Serial.println("error" + String(powerState.error));
    handledWriteRegister = true;
    // neededToWait = false;
    // controlCount = 0;
    return;
  }

  uint8_t r = registers[IHK_T35_ASUKIAAA_REGISTER_ADDR_WRITE];
  bool neededToTurnOnOil = (r & 0b10000000) != 0;
  bool neededToTurnOnNormal = (r & 0b01000000) != 0;
  uint8_t targetLevel = r & 0b111;
  int pinToSendSignal = -1;

  Serial.println("neededToTurnOnNormal:" + String(neededToTurnOnNormal ? "YES" : "NO"));
  Serial.println("powerState.powerNormal:" + String(powerState.powerNormal ? "YES" : "NO"));

  if (neededToTurnOnOil && neededToTurnOnNormal) {
    // Turn off both because of invalid setting
    neededToTurnOnOil = false;
    neededToTurnOnNormal = false;
  }

  if (
    (neededToTurnOnOil && !powerState.powerOil) ||
    (!neededToTurnOnOil && powerState.powerOil)
  ) {
    pinToSendSignal = PIN_POWER_OIL;
  } else if (
    (neededToTurnOnNormal && !powerState.powerNormal) ||
    (!neededToTurnOnNormal && powerState.powerNormal)
  ) {
    pinToSendSignal = PIN_POWER_NORMAL;
  } else if (powerState.powerOil || powerState.powerNormal) {
    if (targetLevel < powerState.level) {
      pinToSendSignal = PIN_POWER_DOWN;
    } else if (targetLevel > powerState.level) {
      pinToSendSignal = PIN_POWER_UP;
    }
  }

  if (pinToSendSignal < 0) {
    // if (pinToSendSignal < 0 || controlCount > 10) {
    handledWriteRegister = true;
    // neededToWait = false;
    // controlCount = 0;
    return;
  }

  if (pinToSendSignal >= 0) {
    digitalWrite(pinToSendSignal, HIGH);
    delay(BUTTON_PRESS_INTERVAL_MS);
    digitalWrite(pinToSendSignal, LOW);
    delay(BUTTON_PRESS_INTERVAL_MS);
    // handledAt = millis();
    // ++ controlCount;
  }
}

void loop() {
  updateRegistersToRead();
  updateByTargetValues();
#ifdef DEBUG
  // static unsigned long printedAt = 0;
  // if (millis() - printedAt > 1000UL) {
  //   printedAt = millis();
  //   printInfo();
  // }
#endif
  delay(1);
}
