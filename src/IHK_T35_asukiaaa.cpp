#include <IHK_T35_asukiaaa.h>

#define ANALOG_MAX 1023
#define VOLT_100C 0.7
#define VOLT_20C 0.12

// #define DEBUG
#ifdef DEBUG
#include <Arduino.h>
#endif

void IHK_T35_asukiaaa::begin() {
  if (wire == NULL) {
    Wire.begin();
    wire = &Wire;
  }
}

int IHK_T35_asukiaaa::setOilLevel(uint8_t level) {
  return write(true, false, level);
}

int IHK_T35_asukiaaa::setNormalLevel(uint8_t level) {
  return write(false, true, level);
}

int IHK_T35_asukiaaa::turnOff() {
  return write(false, false, 0);
}

int IHK_T35_asukiaaa::write(bool oilPower, bool normalPower, uint8_t level) {
  uint8_t r = 0;
  if (level == 0) {
    oilPower = false;
    normalPower = false;
  } else if (level > 6) {
    level = 6;
  } else if (oilPower && normalPower) {
    oilPower = false;
    normalPower = false;
  }
  if (oilPower) {
    r += 0b10000000;
  } else if (normalPower) {
    r += 0b01000000;
  }
  r += level & 0b111;
  wire->beginTransmission(address);
  wire->write(IHK_T35_ASUKIAAA_REGISTER_ADDR_WRITE);
  wire->write(r);
  return wire->endTransmission();
}

int IHK_T35_asukiaaa::read(IHK_T35_asukiaaa_ReadInfo *readInfo) {
  wire->beginTransmission(address);
  wire->write(IHK_T35_ASUKIAAA_REGISTER_ADDR_READ);
  uint8_t result = wire->endTransmission();
  if (result != 0) {
    return result;
  }
  static const uint8_t buffLen = 4;
  uint8_t buff[buffLen];
  uint8_t buffIndex = 0;
  wire->requestFrom((int) address, (int) buffLen);

#ifdef DEBUG
  Serial.print("received:");
#endif
  while (wire->available() > 0) {
    uint8_t d = wire->read();
    if (buffIndex < buffLen) {
      buff[buffIndex] = d;
      ++buffIndex;
    }
#ifdef DEBUG
    Serial.print(" ");
    Serial.print(d, HEX);
#endif
  }
#ifdef DEBUG
  Serial.println("");
#endif

  // uint8_t protocolVersion = buff[0];
  bool oilPower = (buff[1] & 0b10000000) != 0;
  bool normalPower = (buff[1] & 0b01000000) != 0;
  uint8_t level = buff[1] & 0b111;
  uint8_t error = (buff[1] >> 3) & 0b111;

  readInfo->oilLevel = oilPower ? level : 0;
  readInfo->normalLevel = normalPower ? level : 0;
  readInfo->error = error;
  uint8_t v = buff[2] | ((uint16_t) (buff[3] << 8));
  float volt = ((float) v) * 5 / ANALOG_MAX;
  readInfo->thermistorVoltage = volt;
  readInfo->temperatureC = voltToTemperatureC(volt);

  return 0;
}

float IHK_T35_asukiaaa::voltToTemperatureC(float volt) {
  float c = (((volt - VOLT_20C) / (VOLT_100C - VOLT_20C)) * (100 - 20)) + 20;
  return c;
}
