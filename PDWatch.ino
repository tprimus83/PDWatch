#include <TFT_eSPI.h> 
#include "pd.h"
#include "font.h"
#include "button.h"
#include "display.h"
#include "menu.h"
#include <Button2.h>
#include "esp_bt.h"
#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
RTC_DS3231 rtc;

// Define button pins
#define BUTTON1_PIN 35  // Left button
#define BUTTON2_PIN 0   // Right button
#define BACKLIGHT_PIN 4

// Create Button2 objects for each button
Button2 upButton(BUTTON1_PIN);
Button2 downButton(BUTTON2_PIN);

template <typename T, size_t N>
size_t getArrayLength(T (&)[N]) {
    return N;
}

int batteryPin = 34;  // Analóg bemeneti pin az akkumulátor feszültségének mérésére (GPIO34)

Menu timeMenu;
Menu mainMenu;
Menu setTimeMenu;
Menu setDateMenu;
Menu setterMenu;
Menu tempMenu;

boolean binaryFormat = false;

void setDateSelected() {
  // Az aktuális idő lekérése
  DateTime now = rtc.now();

  setDateMenu.items[0].name = String(now.year());
  setDateMenu.items[1].name = String(now.month());
  setDateMenu.items[2].name = String(now.day());
  pushMenu(&setDateMenu);
  beginRender();
  topMenu()->render();
}

void setTimeSelected() {
  // Az aktuális idő lekérése
  DateTime now = rtc.now();

  setTimeMenu.items[0].name = String(now.hour());
  setTimeMenu.items[1].name = String(now.minute());
  setTimeMenu.items[2].name = String(now.second());
  pushMenu(&setTimeMenu);
  beginRender();
  topMenu()->render();
}

const int screenOnTime = 6;
volatile int loopCounter = 0;
boolean screenOn = true;
String tM="ww";

void switchBinaryMode() {
  binaryFormat = !binaryFormat;
  tM = "ww";
  mainMenu.items[2].name = binaryFormat ? "Binary mode OFF" : "Binary mode ON";
  beginRender();
  topMenu()->render();
}

#define background TFT_BLACK

int displayUpdateRateHz = 1;
unsigned long displayLastUpdate;
unsigned long currentMillis;

bool updateDisplay() {
  if (!screenOn) return false;
  if (getButtonState() != BUTTON_NO) return true;
  // Update Display
  if (currentMillis - displayLastUpdate > (1000 / displayUpdateRateHz)) {
    displayLastUpdate = millis();
    return true;
  }
  return false;
}

void pushTimeMenu() {
  pushMenu(&timeMenu);
  drawBG();
  tM="ww";
}

void turnScreenOn() {
  loopCounter = 0;
  displayLastUpdate = 0;
  if (screenOn) return;
  screenOn = true;
  Serial.println("Turn on screen");
  digitalWrite(BACKLIGHT_PIN, HIGH); 
  getDisplay()->writecommand(0x29);
}

void turnScreenOff() {
  if (!screenOn) return;
  Serial.println("Turn off screen");
  getDisplay()->writecommand(0x28);
  digitalWrite(BACKLIGHT_PIN, LOW); 
  screenOn = false;
  esp_light_sleep_start(); // Belép light sleep módba
}

// Callback function for Button 1 press
void upButtonPressed(Button2 &btn) {
  Serial.println("Button up pressed!");
  turnScreenOn();
  setButtonState(BUTTON_UP);
}

// Callback function for Button 2 press
void downButtonPressed(Button2 &btn) {
  Serial.println("Button down pressed!");
  turnScreenOn();
  setButtonState(BUTTON_DOWN);
}

// Callback function for Button 1 press
void upButtonLongPressed(Button2 &btn) {
  Serial.println("Button up long pressed!");
  turnScreenOn();
  setButtonState(BUTTON_OK);
}

// Callback function for Button 2 press
void downButtonLongPressed(Button2 &btn) {
  Serial.println("Button down long pressed!");
  turnScreenOn();
  setButtonState(BUTTON_CANCEL);
}

void setup() {
  Serial.begin(115200);  //buttons
  esp_bt_controller_disable();
  WiFi.mode(WIFI_OFF);
  
// Konfiguráljuk a megszakítást az ébresztéshez
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); 

  upButton.setLongClickTime(1250);  // 2 seconds for long press
  downButton.setLongClickTime(1250);
  upButton.setClickHandler(upButtonPressed);
  upButton.setLongClickHandler(upButtonLongPressed);   // Long press
  downButton.setClickHandler(downButtonPressed);
  downButton.setLongClickHandler(downButtonLongPressed);   // Long press

  beginDisplay();
  
  Wire.begin(21, 22);

  if (!rtc.begin()) {
    Serial.println("Nem található DS3231 modul!");
    while (1);
    //TODO tft write no rtc
  }
  if (rtc.lostPower()) {
    Serial.println("Az óra megállt. Újra kell indítani az időt.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.writeSqwPinMode(DS3231_OFF);
  rtc.disableAlarm(1); // Első riasztás kikapcsolása
  rtc.disableAlarm(2); // Második riasztás kikapcsolása
  Wire.setClock(10000); // I2C órajel 10kHz-re állítása
  //rtc.adjust(DateTime(2025, 1, 1, 12, 0, 0)); // 2025. január 1., 12:00:00
  
  timeMenu.name = "Time";
  timeMenu.render = printLocalTime;

  //Menu
  //static MenuItem configsMenu[MAX_CONFIG_SIZE];
  static MenuItem mainMenuItems[4] = { 
        {"Set date", setDateSelected},
        {"Set time", setTimeSelected},
        {"Binary mode ON", switchBinaryMode},
        {"Temperature", tempSelected}
    };


  // Initialize the menus
  mainMenu.name = "Main";
  mainMenu.items = mainMenuItems;
  mainMenu.size = getArrayLength(mainMenuItems);
  mainMenu.render = displayMenu;

  setterMenu.name = "Setter menu";
  setterMenu.items = {};
  setterMenu.size = 0;
  setterMenu.render = displayIncreaseDecreaseMenu;

  tempMenu.name = "Temperature";
  tempMenu.items = {};
  tempMenu.size = 0;
  tempMenu.render = tempMenuRenderer;

  static MenuItem setTimeMenuItems[3] = {
        {"12", setHourMenu},
        {"00", setMinuteMenu},
        {"Reset sec", resetSecMenu},
    };
  setTimeMenu.name = "Set Time";
  setTimeMenu.items = setTimeMenuItems;
  setTimeMenu.size = getArrayLength(setTimeMenuItems);
  setTimeMenu.render = displayMenu;

  static MenuItem setDateMenuItems[3] = {
        {"2025", setYearMenu},
        {"01", setMonthMenu},
        {"01", setDayMenu}
    };
  setDateMenu.name = "Set Date";
  setDateMenu.items = setDateMenuItems;
  setDateMenu.size = getArrayLength(setDateMenuItems);
  setDateMenu.render = displayMenu;
  pushTimeMenu();
  loopCounter = 0;
}

void loop() {  
  currentMillis = millis();

  upButton.loop();
  downButton.loop();

  if (updateDisplay()) {
    loopCounter++;
    topMenu()->render();
    endRender();
  }

  if ((loopCounter > screenOnTime) && screenOn) {
    Serial.println("Time is up!!" + String(loopCounter));
    turnScreenOff();
    turnScreenOn();
  } 
}

void printLocalTime() {
  // Óra, perc, másodperc
  char timeMin[3];
  char timeHour[3];
  char timeSec[3];
  char timeDay[3];
  String m, d;
  int minute, hour;
  
  DateTime now = rtc.now();
  hour = now.hour();
  minute = now.minute();
  snprintf(timeHour, sizeof(timeHour), "%02d", hour);
  snprintf(timeMin, sizeof(timeMin), "%02d", minute);
  m = String(timeMin) + String(timeHour);
  snprintf(timeSec, sizeof(timeSec), "%02d", now.second());
  // Nap neve és napja
  const char* weekDays[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
  d = String(weekDays[now.dayOfTheWeek()]); // DS3231 nap indexe alapján (0-6)
  snprintf(timeDay, sizeof(timeDay), "%02d", now.day());

  TFT_eSPI *display = getDisplay();
  display->setTextColor(((252 >> 3) << 11) | ((3 >> 2) << 5) | (3 >> 3), TFT_BLACK);

  //Serial.println("refresh:" + String(minute));

  if(tM!=m) {//TODO az orat is figyelni kell
    if (binaryFormat) {
    //Serial.println("call:" + String(minute));
      drawBinaryTime(display, hour, minute);
    } else {
      display->setFreeFont(&DSEG7_Classic_Bold_30);
      display->drawString(String(timeHour) + ":" + String(timeMin), 2, 7);
    
      display->setFreeFont(&DSEG14_Classic_Bold_18);
      display->drawString(d.substring(0,2) + " " + String(timeDay), 164, 7);
    }
    tM=m;
  }
  if (!binaryFormat) {
      display->setFreeFont(&DSEG7_Classic_Bold_20);
      display->drawString(String(timeSec), 113 , 17);
  }

  // Akkumulátor feszültség mérés
  float batteryVoltage = getBatteryVoltage();

  // Töltöttség kiszámítása
  int batteryLevel = calculateBatteryLevel(batteryVoltage);
  
  display->setFreeFont(&DSEG14_Classic_Bold_18);
  if (batteryLevel > 100) display->drawString("USB", 190, 65);
  else display->drawString((batteryLevel < 10 ? "0" : "") + String(batteryLevel) + "%", 190, 65);


  ButtonState button = getButtonState();
  if (button == BUTTON_OK) {
    Serial.println("Entering main menu");
    pushMenu(&mainMenu);
    tM="ww";
    beginRender();
    topMenu()->render();
  }
}

// Akkumulátor feszültségének mérésére szolgáló függvény
float getBatteryVoltage() {
  int total = 0;
  int sampleCount = 0;
  int numSamples = 5;

  // Mérés és átlagolás
  for (int i = 0; i < numSamples; i++) {
    int adcValue = analogRead(batteryPin);
    total += adcValue;
    sampleCount++;
    delay(10);  // Kis késleltetés a mintavételek között (10ms)
  }

  // Átlagolt ADC érték
  long averageValue = total / sampleCount;

  float voltage = averageValue * (3.3 / 4095.0);  // Átalakítás 0-3.3V között
  return voltage * (float)2;  // Mivel osztót használsz, szorozd meg 2-t
}

// Töltöttség kiszámítása a feszültség alapján
int calculateBatteryLevel(float voltage) {
  int level = voltage * (float)10;
  if (voltage >= 41) return 101;
  level = map(level, 30, 37, 0, 100);  // Szorozd meg 10-tel, hogy a map() egész számokkal dolgozhasson
  level = constrain(level, 0, 100);  // Biztosítjuk, hogy a szint 0 és 100 között legyen
  return level;
}

// Változók a függvények és szám tárolására
void (*increaseFunction)();
void (*decreaseFunction)();
int idNumber;

void setSetterMenu(void (*ifn)(), void (*dfn)(), int num) {
  increaseFunction = ifn;
  decreaseFunction = dfn;
  idNumber = num;
  pushMenu(&setterMenu);
  beginRender();
  topMenu()->render();
}

void tempSelected() {
  pushMenu(&tempMenu);
  beginRender();
  topMenu()->render();
}

void tempMenuRenderer() {
  ButtonState button = getButtonState();
  if (button == BUTTON_CANCEL || button == BUTTON_LONG_PRESS || button == BUTTON_OK) {
    beginRender();
    popMenu();
    topMenu()->render();
    return;
  }
  //debugln("Clear display");
  TFT_eSPI* display = getDisplay();
  // Clear the buffer.
  display->fillScreen(TFT_BLACK);
  // Display Text
  display->setTextSize(1);
  display->setTextColor(TFT_YELLOW, TFT_BLACK); // 'inverted' text
  display->setCursor(0, MENU_MIN_Y_POS + 22);
  display->println(String(rtc.getTemperature()) + "C");
  endRender();
}

void displayIncreaseDecreaseMenu() {
  ButtonState button = getButtonState();
  bool changed = button != BUTTON_NO;
  if (!changed) {
    //debugln("Nothing changed no rendering");
    return;
  }
  if (button == BUTTON_DOWN) {
    //dec fn
    decreaseFunction();
  } else if (button == BUTTON_UP) {
    //inc fn
    increaseFunction();
  } else if (button == BUTTON_OK) {
    beginRender();
    popMenu();
    topMenu()->render();
    return;
  } else if (button == BUTTON_CANCEL || button == BUTTON_LONG_PRESS) {
    beginRender();
    popMenu();
    topMenu()->render();
    return;
  }
  //debugln("Clear display");
  TFT_eSPI* display = getDisplay();
  // Clear the buffer.
  display->fillScreen(TFT_BLACK);
  // Display Text
  display->setTextSize(1);
  display->setTextColor(TFT_YELLOW, TFT_BLACK); // 'inverted' text
  display->setCursor(0, MENU_MIN_Y_POS + 22);
  display->println(String(idNumber));
  endRender();
}

// Év növelése
void increaseYear() {
  /**time_t rawtime = lastCorrectedTime.tv_sec;
  struct tm* timeinfo = localtime(&rawtime);
  timeinfo->tm_year++;
  idNumber = timeinfo->tm_year;
  time_t t = mktime(timeinfo);
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);       // Idő beállítása az RTC-ben*/
}

// Év csökkentése
void decreaseYear() {
  /**time_t rawtime = lastCorrectedTime.tv_sec;
  struct tm* timeinfo = localtime(&rawtime);
  timeinfo->tm_year--;
  idNumber = timeinfo->tm_year;
  time_t t = mktime(timeinfo);
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);       // Idő beállítása az RTC-ben*/
}

void setYearMenu() {
  // Az aktuális idő lekérése
  setSetterMenu(increaseYear, decreaseYear, rtc.now().year());
}

// Hónap növelése
void increaseMonth() {
  /**time_t rawtime = lastCorrectedTime.tv_sec;
  struct tm* timeinfo = localtime(&rawtime);
  timeinfo->tm_mon++;
  if (timeinfo->tm_mon == 11) return;
  idNumber = timeinfo->tm_mon;
  time_t t = mktime(timeinfo);
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);       // Idő beállítása az RTC-ben*/
}

// Hónap csökkentése
void decreaseMonth() {
  // time_t rawtime = lastCorrectedTime.tv_sec;
  // struct tm* timeinfo = localtime(&rawtime);
  // if (timeinfo->tm_mon == 0) return;
  // timeinfo->tm_mon--;
  // idNumber = timeinfo->tm_mon;
  // time_t t = mktime(timeinfo);
  // struct timeval now = { .tv_sec = t };
  // settimeofday(&now, NULL);       // Idő beállítása az RTC-ben
}

void setMonthMenu() {
  setSetterMenu(increaseMonth, decreaseMonth, rtc.now().month());
}

// Nap növelése
void increaseDay() {
  // time_t rawtime = lastCorrectedTime.tv_sec;
  // struct tm* timeinfo = localtime(&rawtime);
  // if (timeinfo->tm_mday == 30) return;
  // timeinfo->tm_mday++;
  // idNumber = timeinfo->tm_mday + 1;
  // time_t t = mktime(timeinfo);
  // struct timeval now = { .tv_sec = t };
  // settimeofday(&now, NULL);       // Idő beállítása az RTC-ben
}

// Nap csökkentése
void decreaseDay() {
  // time_t rawtime = lastCorrectedTime.tv_sec;
  // struct tm* timeinfo = localtime(&rawtime);
  // if (timeinfo->tm_mday == 0) return;
  // timeinfo->tm_mday--;
  // idNumber = timeinfo->tm_mday + 1;
  // time_t t = mktime(timeinfo);
  // struct timeval now = { .tv_sec = t };
  // settimeofday(&now, NULL);       // Idő beállítása az RTC-ben
}

void setDayMenu() {
  setSetterMenu(increaseDay, decreaseDay, rtc.now().day());
}

// Óra növelése
void increaseHour() {
  // time_t rawtime = lastCorrectedTime.tv_sec;
  // struct tm* timeinfo = localtime(&rawtime);
  // if (timeinfo->tm_hour == 23) return;
  // timeinfo->tm_hour++;
  // idNumber = timeinfo->tm_hour;
  // time_t t = mktime(timeinfo);
  // struct timeval now = { .tv_sec = t };
  // settimeofday(&now, NULL);       // Idő beállítása az RTC-ben
}

// Óra csökkentése
void decreaseHour() {
  // time_t rawtime = lastCorrectedTime.tv_sec;
  // struct tm* timeinfo = localtime(&rawtime);
  // if (timeinfo->tm_hour == 0) return;
  // timeinfo->tm_hour--;
  // idNumber = timeinfo->tm_hour;
  // time_t t = mktime(timeinfo);
  // struct timeval now = { .tv_sec = t };
  // settimeofday(&now, NULL);       // Idő beállítása az RTC-ben
}

void setHourMenu() {
  setSetterMenu(increaseHour, decreaseHour, rtc.now().hour());
}

// Perc növelése
void increaseMinute() {
  // time_t rawtime = lastCorrectedTime.tv_sec;
  // struct tm* timeinfo = localtime(&rawtime);
  // if (timeinfo->tm_min == 59) return;
  // timeinfo->tm_min++;
  // idNumber = timeinfo->tm_min;
  // time_t t = mktime(timeinfo);
  // struct timeval now = { .tv_sec = t };
  // settimeofday(&now, NULL);       // Idő beállítása az RTC-ben
}

// Perc csökkentése
void decreaseMinute() {
  // time_t rawtime = lastCorrectedTime.tv_sec;
  // struct tm* timeinfo = localtime(&rawtime);
  // if (timeinfo->tm_min == 0) return;
  // timeinfo->tm_min--;
  // idNumber = timeinfo->tm_min;
  // time_t t = mktime(timeinfo);
  // struct timeval now = { .tv_sec = t };
  // settimeofday(&now, NULL);       // Idő beállítása az RTC-ben
}

void setMinuteMenu() {
  setSetterMenu(increaseMinute, decreaseMinute, rtc.now().minute());
}

// Perc csökkentése
void resetSecMenu() {
  // DateTime dt = rtc.datetime();
  // dt = dt + TimeSpan(0, 0, 0, 30);
  // rtc.adjust(dt); // Állítsd be a számítógép idejét
  //TODO
}

void drawBinaryTime(TFT_eSPI* display, int hour, int minute) {
  //Serial.println("Min:" + String(minute));
  const int rectWidth = 12;  // Téglalap szélessége
  const int rectHeight = 12; // Téglalap magassága
  const int spacing = 6;     // Téglalapok közötti távolság
  const int startX = 74;     // Kiinduló X koordináta (középre igazítva)
  const int startY = 10;     // Kiinduló Y koordináta (felső sor)

  // Óra bináris rajzolása (felső sor)
  for (int i = 0; i < 5; i++) { // 5 bit az órához (0-23)
    int bit = (hour >> (4 - i)) & 1; // Az aktuális bit kiolvasása
    int x = startX + rectWidth / 2 + i * (rectWidth + spacing);
    int y = startY;

    if (bit == 1) {
      display->fillRect(x, y, rectWidth, rectHeight, ((252 >> 3) << 11) | ((3 >> 2) << 5) | (3 >> 3)); // Kitöltött téglalap
    } else {
      display->drawRect(x, y, rectWidth, rectHeight, ((252 >> 3) << 11) | ((3 >> 2) << 5) | (3 >> 3)); // Csak keret
    }
  }

  // Perc bináris rajzolása (alsó sor)
  for (int i = 0; i < 6; i++) { // 6 bit a perchez (0-59)
    int bit = (minute >> (5 - i)) & 1; // Az aktuális bit kiolvasása
    int x = startX + i * (rectWidth + spacing);
    int y = startY + rectHeight + spacing;

    if (bit == 1) {
      display->fillRect(x, y, rectWidth, rectHeight, ((252 >> 3) << 11) | ((3 >> 2) << 5) | (3 >> 3)); // Kitöltött téglalap
    } else {
      display->drawRect(x, y, rectWidth, rectHeight, ((252 >> 3) << 11) | ((3 >> 2) << 5) | (3 >> 3)); // Csak keret
    }
  }
}