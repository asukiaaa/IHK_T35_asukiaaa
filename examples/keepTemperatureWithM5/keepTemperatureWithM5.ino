#include <IHK_T35_asukiaaa.h>
#include <M5Stack.h>

#define TARGET_TEMPERATURE 110
#define TEMPERATURE_ALLOW_DIFF 12

IHK_T35_asukiaaa heater;

void setup() {
  heater.begin();
  M5.begin();
  M5.Speaker.begin();
  M5.Speaker.mute();
  M5.Lcd.setTextSize(2);
  Serial.begin(9600);
}

String padStart(String str, uint8_t length, char pad) {
  while(str.length() < length) {
    str = pad + str;
  }
  return str;
}

String padEnd(String str, uint8_t length, char pad) {
  while(str.length() < length) {
    str += pad;
  }
  return str;
}

void loop() {
  IHK_T35_asukiaaa_ReadInfo readInfo;
  int result = heater.read(&readInfo);
  M5.Lcd.setCursor(0,0);
  if (result != 0) {
    M5.Lcd.println("Error: " + String(result));
  } else {
    float t = readInfo.temperatureC;
    M5.Lcd.println("Current");
    M5.Lcd.println("Temperature:" + padStart(String(t), 6, ' ') + "C");
    M5.Lcd.println("Level: " + String(readInfo.normalLevel));
    String str;
    if (t < TARGET_TEMPERATURE - TEMPERATURE_ALLOW_DIFF) {
      heater.setNormalLevel(3);
      str = "Request turn on";
    } else if (t > TARGET_TEMPERATURE + TEMPERATURE_ALLOW_DIFF) {
      heater.turnOff();
      str = "Request turn off";
    } else {
      str = "Request nothing";
    }
    M5.Lcd.println(padEnd(str, 20, ' '));
    M5.Lcd.println("");
    M5.Lcd.println("Target");
    M5.Lcd.println("Temperature:" + padStart(String(TARGET_TEMPERATURE), 6, ' ') + "C");
    M5.Lcd.println("Plus minus: " + padStart(String(TEMPERATURE_ALLOW_DIFF), 6, ' ') + "C");
  }
  delay(5000);
}
