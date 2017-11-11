
#include <LiquidCrystal.h>
#include <OneWire.h>
#include "EEPROM.h"
#include <SoftPWM.h>

#define DS18B20_PIN A4
#define FAN_PIN 9
#define LAMP_PIN 8
#define LIGHT_SENSOR_PIN A5
#define PIR_PIN A3
#define BUTTON_DOWN A2
#define BUTTON_UP A1
#define BUTTON_MODE A0
#define LED 13

OneWire ds(DS18B20_PIN);
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

bool isHumanDetected = false;
bool isLightTurnedOff = false;
int displayMode = 0;
long prev[7];
int style;
float temperature;
float tempSet = 30;
int lightIntensity;
int lightIntensitySet = 0;

void setup()
{
  lcd.begin(16, 2);
  Serial.begin(9600);
  SoftPWMBegin();
  SoftPWMSet(LAMP_PIN, 0);
  SoftPWMSetFadeTime(LAMP_PIN, 1000, 1000);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  readFromEEPROM();
  if (tempSet > 100)
  {
    lightIntensitySet = 0;
    tempSet = 0;
    saveToEEPROM();
  }
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Saving");
  lcd.setCursor(0, 1);
  lcd.print("Electricity");
  delay(2000);
  lcd.clear();
}

void loop()
{
  int LDRValue = 0;

  if (millis() - prev[6] > 100) {
    prev[6] = millis();
    temperature = getTemp();
    LDRValue = getLight();
  }
  Serial.print(LDRValue);
  Serial.print("\t");
  if (LDRValue >= 800)
    lightIntensity = map(LDRValue, 800, 1023, 10, 0);
  else if (LDRValue >= 682)
    lightIntensity = map(LDRValue, 682, 800, 30, 10);
  else if (LDRValue >= 100)
    lightIntensity = map(LDRValue, 682, 150, 30, 1000);
  else
    lightIntensity = map(LDRValue, 150, 0, 1000, 3000);
  Serial.print(lightIntensity);
  Serial.print("\t");
  Serial.println(temperature);
  digitalWrite(LED, digitalRead(PIR_PIN));

  if (digitalRead(BUTTON_MODE) == LOW)
  {
    while (digitalRead(BUTTON_MODE) == LOW) {}
    displayMode = 1;
    setTemp();
  }
  if (digitalRead(PIR_PIN) == HIGH)
  {
    isHumanDetected = true;
    prev[0] = millis();
  }
  if (millis() - prev[0] > 120000)
  {
    if (digitalRead(PIR_PIN) == LOW)
    {
      lcd.clear();
      prev[0] = millis();
      isHumanDetected = false;
      isLightTurnedOff = false;
    }
  }
  if (isHumanDetected)
  {
    isLightTurnedOff = false;
    if (millis() - prev[4] > 2000)
    {
      prev[4] = millis();
      ControlFan(temperature >= tempSet);
      ControlLamp(lightIntensity <= lightIntensitySet);
    }
    if (millis() - prev[1] > 2000)
    {
      prev[1] = millis();
      lcd.clear();
      style = !style;
    }
    if (style)
    {
      if (millis() - prev[5] > 100)
      {
        prev[5] = millis();
        lcd.setCursor(0, 0);
        lcd.print("Temp:  ");
        lcd.print(temperature, 1);
        lcd.print(char(223));
        lcd.print("C   ");
        lcd.setCursor(0, 1);
        lcd.print("tempSet: ");
        lcd.print(tempSet, 1);
        lcd.print(char(223));
        lcd.print("C   ");
      }
    }
    else
    {
      if (millis() - prev[5] > 100)
      {
        prev[5] = millis();
        lcd.setCursor(0, 0);
        lcd.print("Light: ");
        lcd.print(lightIntensity);
        lcd.print(" Lux  ");
        lcd.setCursor(0, 1);
        lcd.print("LightSet: ");
        lcd.print(lightIntensitySet);
        lcd.print(" Lux  ");
      }
    }
  }
  else if (!isLightTurnedOff)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Turn OFF all!");
    ControlLamp(0);
    ControlFan(0);
    isLightTurnedOff = true;
  }
}
int getLight() {
  float currentValue = (float)analogRead(LIGHT_SENSOR_PIN);
  static float averageValue = 0;
  averageValue = 0.8 * averageValue + 0.2 * currentValue;
  return (int)averageValue;
}
void setTemp()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set T: ");
  while (displayMode == 1)
  {
    lcd.setCursor(8, 0);
    lcd.print(tempSet, 1);
    lcd.print(char(223));
    lcd.print("C");
    lcd.print("     ");
    if (digitalRead(BUTTON_DOWN) == LOW)
    {
      prev[2] = millis();
      prev[3] = millis();
      while (digitalRead(BUTTON_DOWN) == LOW)
      {
        delay(10);
        if (millis() - prev[2] > 500 && millis() - prev[3] > 100)
        {
          prev[3] = millis();
          if (tempSet > 20)
            tempSet --;
          else tempSet = 20;
          lcd.setCursor(8, 0);
          lcd.print(tempSet, 1);
          lcd.print(char(223));
          lcd.print("C");
          lcd.print("     ");
        }
      }
      if (tempSet > 20)
        tempSet -= 0.5;
      else tempSet = 20;
    }
    if (digitalRead(BUTTON_UP) == LOW)
    {
      prev[2] = millis();
      prev[3] = millis();
      while (digitalRead(BUTTON_UP) == LOW)
      {
        delay(10);
        if (millis() - prev[2] > 500 && millis() - prev[3] > 100)
        {
          prev[3] = millis();
          if (tempSet < 40)
            tempSet++;
          else tempSet = 40;
          lcd.setCursor(8, 0);
          lcd.print(tempSet, 1);
          lcd.print(char(223));
          lcd.print("C");
          lcd.print("     ");
        }
      }
      if (tempSet < 40)
        tempSet += 0.5;
      else tempSet = 40;
    }
    if (digitalRead(BUTTON_MODE) == LOW)
    {
      while (digitalRead(BUTTON_MODE) == LOW) delay(10);
      displayMode = 2;
    }
  }
  lcd.setCursor(0, 0);
  lcd.print("Light: ");
  while (displayMode == 2)
  {
    lcd.setCursor(9, 0);
    lcd.print(lightIntensitySet);
    lcd.print("Lux  ");
    lcd.print("     ");
    if (digitalRead(BUTTON_DOWN) == LOW)
    {
      prev[2] = millis();
      prev[3] = millis();
      while (digitalRead(BUTTON_DOWN) == LOW)
      {
        delay(10);
        if (millis() - prev[2] > 500 && millis() - prev[3] > 30)
        {
          prev[3] = millis();
          if (lightIntensitySet > 200)
            lightIntensitySet--;
          lcd.setCursor(9, 0);
          lcd.print(lightIntensitySet);
          lcd.print("Lux  ");
          lcd.print("     ");
        }
      }
      if (lightIntensitySet > 200)
        lightIntensitySet--;
    }
    if (digitalRead(BUTTON_UP) == LOW)
    {
      prev[2] = millis();
      prev[3] = millis();
      while (digitalRead(BUTTON_UP) == LOW)
      {
        delay(10);
        if (millis() - prev[2] > 500 && millis() - prev[3] > 30)
        {
          prev[3] = millis();
          if (lightIntensitySet < 1000)
            lightIntensitySet++;
          lcd.setCursor(9, 0);
          lcd.print(lightIntensitySet);
          lcd.print("Lux  ");
          lcd.print("     ");
        }
      }
      if (lightIntensitySet < 1000)
        lightIntensitySet++;
    }
    if (digitalRead(BUTTON_MODE) == LOW)
    {
      while (digitalRead(BUTTON_MODE) == LOW) delay(10);
      displayMode = 0;
    }
  }
  saveToEEPROM();
  isLightTurnedOff = false;
}
void ControlFan(bool state)
{
  static bool lastState;
  if (state && !lastState)
    for (int i = 0; i < 8; i++)
    {
      analogWrite(FAN_PIN, i);
      delay(200);
    }
  else if (!state && lastState) for (int i = 8; i > 0; i--)
    {
      analogWrite(FAN_PIN, i);
      delay(200);
    }
  lastState = state;
}
void ControlLamp(bool state)
{
  static bool lastState;
  if (state && !lastState) {
    SoftPWMSet(LAMP_PIN, 255);
    lastState = state;
    delay(1000);
  }
  else if (!state  && lastState) {
    SoftPWMSet(LAMP_PIN, 0);
    lastState = state;
    delay(1000);
  }
}
float getTemp()
{
  byte data[12];
  byte addr[8];

  ds.search(addr);
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);
  for (int i = 0; i < 9; i++)
  {
    data[i] = ds.read();
  }
  ds.reset_search();
  float TRead = ((data[1] << 8) | data[0]);
  float Temperature = TRead / 16;
  static float AverageValue = 0;

  AverageValue = 0.8 * AverageValue + 0.2 * Temperature;

  return AverageValue;
}
void readFromEEPROM()
{
  EEPROM.get(0, lightIntensitySet);
  EEPROM.get(2, tempSet);
}
void saveToEEPROM()
{
  EEPROM.put(0, lightIntensitySet);
  EEPROM.put(2, tempSet);
}

