#ifndef __IHK_T35_ASUKIAAA_H__
#define __IHK_T35_ASUKIAAA_H__
#include <Wire.h>

#define IHK_T35_ASUKIAAA_ADDRESS_A0_OPEN_A1_OPEN 0x18
#define IHK_T35_ASUKIAAA_ADDRESS_A0_CLOSE_A1_OPEN 0x19
#define IHK_T35_ASUKIAAA_ADDRESS_A0_OPEN_A1_CLOSE 0x1a
#define IHK_T35_ASUKIAAA_ADDRESS_A0_CLOSE_A1_CLOSE 0x1b

#define IHK_T35_ASUKIAAA_REGISTER_ADDR_READ 0
#define IHK_T35_ASUKIAAA_REGISTER_ADDR_WRITE 4

typedef struct {
  uint8_t normalLevel;
  uint8_t oilLevel;
  uint8_t error;
  float thermistorVoltage;
  float temperatureC;
} IHK_T35_asukiaaa_ReadInfo;

class IHK_T35_asukiaaa {
 public:
  IHK_T35_asukiaaa(uint8_t address = IHK_T35_ASUKIAAA_ADDRESS_A0_OPEN_A1_OPEN) {
    this->address = address;
    this->wire = NULL;
  }
  void setWire(TwoWire *wire) {
    this->wire = wire;
  }
  void begin();
  int setOilLevel(uint8_t level);
  int setNormalLevel(uint8_t level);
  int turnOff();
  int read(IHK_T35_asukiaaa_ReadInfo *readInfo);

 private:
  TwoWire *wire;
  uint8_t address;

  float voltToTemperatureC(float volt);
  int write(bool oilPower, bool normalPower, uint8_t level);
};

#endif
