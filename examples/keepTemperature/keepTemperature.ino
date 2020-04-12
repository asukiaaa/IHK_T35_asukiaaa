#include <IHK_T35_asukiaaa.h>

#define TARGET_TEMPERATURE 65
#define TEMPERATURE_ALLOW_DIFF 10

IHK_T35_asukiaaa heater;

void setup() {
  heater.begin();
  Serial.begin(9600);
}

void loop() {
  IHK_T35_asukiaaa_ReadInfo readInfo;
  int result = heater.read(&readInfo);
  if (result != 0) {
    Serial.println("Cannot read info. Error: " + String(result));
  } else {
    float t = readInfo.temperatureC;
    Serial.println("Temperature: " + String(t) + "C");
    // if (t < TARGET_TEMPERATURE - TEMPERATURE_ALLOW_DIFF) {
    //   heater.setNormalLevel(3);
    // } else if (t > TARGET_TEMPERATURE + TEMPERATURE_ALLOW_DIFF) {
    //   heater.turnOff();
    // }
  }
  delay(1000);
}
