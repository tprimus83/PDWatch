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
#include "driver/ledc.h"

RTC_DS3231 rtc;

// Define button pins
#define BUTTON1_PIN 35  // Left button
#define BUTTON2_PIN 0   // Right button

#define UNUSED_1 32     // Szabad GPIO, energiatakarékos módra állítva
#define UNUSED_2 33     // Szabad GPIO, energiatakarékos módra állítva
#define UNUSED_3 36     // Szabad GPIO, energiatakarékos módra állítva
#define UNUSED_4 39     // Szabad GPIO, energiatakarékos módra állítva
#define UNUSED_5 13     // Szabad GPIO
#define UNUSED_6 14     // Szabad GPIO


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
Menu stopperMenu;
Menu tempMenu;
Menu voltMenu;

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
#define DEFAULT_SCREEN_ON_TIME 6
#define MENU_SCREEN_ON_TIME 30
#define STOPPER_SCREEN_ON_TIME 1000

int screenOnTime = DEFAULT_SCREEN_ON_TIME;
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
#define DEFAULT_UPDATE_HZ 1
#define STOPPER_UPDATE_HZ 5

int displayUpdateRateHz = DEFAULT_UPDATE_HZ;
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
  //digitalWrite(TFT_BL, HIGH); 
  //getDisplay()->writecommand(0x29);
}

void turnScreenOff() {
  if (!screenOn) return;
  Serial.println("Turn off screen");
  getDisplay()->writecommand(0x28);
  digitalWrite(TFT_BL, LOW); 
  screenOn = false;
  esp_deep_sleep_start();
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
  
  // LEDC konfiguráció
  ledc_timer_config_t ledc_timer = {
      .speed_mode       = LEDC_HIGH_SPEED_MODE,
      .duty_resolution  = LEDC_TIMER_8_BIT,  // 8 bit felbontás
      .timer_num        = LEDC_TIMER_0,
      .freq_hz          = 5000,             // Frekvencia: 5 kHz
      .clk_cfg          = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);

  ledc_channel_config_t ledc_channel = {
      .gpio_num       = TFT_BL,
      .speed_mode     = LEDC_HIGH_SPEED_MODE,
      .channel        = LEDC_CHANNEL_0,
      .intr_type      = LEDC_INTR_DISABLE,
      .timer_sel      = LEDC_TIMER_0,
      .duty           = 128,  // 50% fényerő (256 a maximális duty cycle érték 8 bitnél)
      .hpoint         = 0
  };
  ledc_channel_config(&ledc_channel);
  
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
  static MenuItem mainMenuItems[6] = { 
        {"Set date", setDateSelected},
        {"Set time", setTimeSelected},
        {"Binary mode ON", switchBinaryMode},
        {"Stopper", stopperSelected},
        {"Temperature", tempSelected},
        {"Battery status", voltSelected}
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

  stopperMenu.name = "Stopper";
  stopperMenu.items = {};
  stopperMenu.size = 0;
  stopperMenu.render = displayStopper;

  tempMenu.name = "Temperature";
  tempMenu.items = {};
  tempMenu.size = 0;
  tempMenu.render = tempMenuRenderer;

  voltMenu.name = "Battery status";
  voltMenu.items = {};
  voltMenu.size = 0;
  voltMenu.render = voltMenuRenderer;

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

  // Akkumulátor méréshez használt pin
  pinMode(batteryPin, INPUT);
  // Nem használt pin-ek energiatakarékos módhoz
// Nem használt pin-ek energiatakarékos módra állítása
  pinMode(UNUSED_1, INPUT);
  pinMode(UNUSED_2, INPUT);
  pinMode(UNUSED_3, INPUT);
  pinMode(UNUSED_4, INPUT);
  pinMode(UNUSED_5, INPUT);
  pinMode(UNUSED_6, INPUT);

  // További nem használt RTC pin-ek alvó módra állítása
  pinMode(36, INPUT); // GPIO36 (RTC input)
  pinMode(39, INPUT); // GPIO39 (RTC input)
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
  screenOnTime = DEFAULT_SCREEN_ON_TIME;
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

  if(tM!=m) {
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
    screenOnTime = MENU_SCREEN_ON_TIME;
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
  int level = voltage * (float)100;
  if (level >= 410) return 101;
  level = map(level, 300, 370, 0, 100);  // Szorozd meg 10-tel, hogy a map() egész számokkal dolgozhasson
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

void simpleMenuSelected(Menu *menu) {
  pushMenu(menu);
  beginRender();
  topMenu()->render();
} 

void tempSelected() {
  simpleMenuSelected(&tempMenu);
}

void stopperSelected() {
  simpleMenuSelected(&stopperMenu);
}

void voltSelected() {
  simpleMenuSelected(&voltMenu);
}

DateTime startTime;               // To store the start time
uint32_t pausedTime = 0;          // Store elapsed time when paused
bool stopwatchRunning = false;    // State of the stopwatch
bool stopwatchPaused = false;     // State of pause
unsigned long startMillis = 0;
unsigned long endMillis = 0;
uint32_t lastElapsedTime = 0;

// Array to store the last 4 recorded times (in seconds)
unsigned long lastTimes[4] = {0, 0, 0, 0};

// Function to calculate and print elapsed time
void printElapsedTime(unsigned long elapsedMillis, int i) {
  if (elapsedMillis == 0 && i > 0) return;
  int elapsed = elapsedMillis / 1000;
  uint32_t hours = elapsed / 3600;
  uint32_t minutes = (elapsed % 3600) / 60;
  uint32_t seconds = elapsed % 60;
  
  int milliseconds = elapsedMillis % 1000;  // Milliszekundumok (ezredmásodpercek)
  int tenths = milliseconds / 10;  // Tizedmásodperc
  //debugln("Clear display");
  TFT_eSPI* display = getDisplay();
  // Display Text
  display->setTextSize(1);
  display->setTextColor(i == 0 && stopwatchRunning ? ((252 >> 3) << 11) | ((3 >> 2) << 5) | (3 >> 3) : TFT_YELLOW, TFT_BLACK); 
  display->setCursor(0, MENU_MIN_Y_POS + 22 * i);
  display->println(String(hours) + ":" + String(minutes) + ":" + String(seconds) + "." + String(tenths));
}

// Function to add an elapsed time to the history array
void addToHistory(unsigned long elapsedTime) {
  // Shift the array to make room for the new time at the top
  for (int i = 3; i > 0; i--) {
    lastTimes[i] = lastTimes[i - 1];
  }
  // Add the new time at the top
  lastTimes[0] = elapsedTime;
}

void displayStopper() {
  uint32_t elapsedTime;
  screenOnTime = STOPPER_SCREEN_ON_TIME;
  displayUpdateRateHz = STOPPER_UPDATE_HZ;
  if (stopwatchRunning && !stopwatchPaused) {
    lastElapsedTime = elapsedTime;
    elapsedTime = rtc.now().unixtime() - startTime.unixtime();
    lastTimes[0] = elapsedTime;
    if (lastElapsedTime != elapsedTime) {
      startMillis = millis();
    }
  }
  ButtonState button = getButtonState();
  if (button == BUTTON_CANCEL || button == BUTTON_OK) {
    beginRender();
    popMenu();
    topMenu()->render();
    screenOnTime = MENU_SCREEN_ON_TIME;
    displayUpdateRateHz = DEFAULT_UPDATE_HZ;
    return;
  } else if (button == BUTTON_UP) {
    //start stop
    if (!stopwatchRunning) {
      // Start the stopwatch
      startTime = rtc.now();
      pausedTime = 0;
      startMillis = millis();
      endMillis = 0;
      lastElapsedTime = 0;
      stopwatchRunning = true;
      stopwatchPaused = false;
    } else if (stopwatchPaused) {
      // Resume the stopwatch
      startTime = DateTime(rtc.now().unixtime() - pausedTime);  // Adjust startTime to account for paused time
      stopwatchPaused = false;
    } else {
      // Pause the stopwatch
      pausedTime = rtc.now().unixtime() - startTime.unixtime();
      stopwatchPaused = true;
      loopCounter = 1;
    }
  } else if (button == BUTTON_DOWN) {
    loopCounter = 1;
    //stop and reset
    if (stopwatchRunning) {
      elapsedTime = pausedTime;
      if (!stopwatchPaused) {
        elapsedTime = rtc.now().unixtime() - startTime.unixtime();
      }

      stopwatchRunning = false;
      stopwatchPaused = false;

      Serial.print("Stopwatch stopped. Elapsed time: ");

      addToHistory(elapsedTime * 1000 + ((endMillis - startMillis) % 1000));
      elapsedTime = 0;
      startMillis = 0;
      endMillis = 0;
      lastElapsedTime = 0;
    }
  }
  
  if (loopCounter != 1) return;

  TFT_eSPI* display = getDisplay();
  // Clear the buffer.
  display->fillScreen(TFT_BLACK);

  for (int i = 0; i < 4; i++) {
    printElapsedTime(lastTimes[i], i);
  }
  endRender();
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

void voltMenuRenderer() {
  ButtonState button = getButtonState();
  if (button == BUTTON_CANCEL || button == BUTTON_LONG_PRESS || button == BUTTON_OK) {
    beginRender();
    popMenu();
    topMenu()->render();
    return;
  }
  TFT_eSPI* display = getDisplay();
  display->fillScreen(TFT_BLACK);
  // Display Text
  display->setTextSize(1);
  display->setTextColor(TFT_YELLOW, TFT_BLACK); // 'inverted' text
  display->setCursor(0, MENU_MIN_Y_POS + 22);
  display->println(String(getBatteryVoltage()) + " V");
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
  DateTime now = rtc.now();
  // Adjust the RTC with the new date
  DateTime newDate(now.year() + 1, now.month(), now.day(), now.hour(), now.minute(), now.second());
  rtc.adjust(newDate);
  idNumber = newDate.year();
}

// Év csökkentése
void decreaseYear() {
  DateTime now = rtc.now();
  // Adjust the RTC with the new date
  DateTime newDate(now.year() - 1, now.month(), now.day(), now.hour(), now.minute(), now.second());
  rtc.adjust(newDate);
  idNumber = newDate.year();
}

void setYearMenu() {
  // Az aktuális idő lekérése
  setSetterMenu(increaseYear, decreaseYear, rtc.now().year());
}

// Hónap növelése
void increaseMonth() {
  DateTime now = rtc.now();

  // Increase the month
  uint16_t year = now.year();
  uint8_t month = now.month();

  if (month == 1) {
    month = 12; // Wrap around to January
    year++;    // Increment the year
  } else {
    month++;   // Increment the month
  }

  // Adjust the RTC with the new date
  DateTime newDate(year, month, now.day(), now.hour(), now.minute(), now.second());
  rtc.adjust(newDate);
  idNumber = newDate.month();
}

// Hónap csökkentése
void decreaseMonth() {
  DateTime now = rtc.now();

  // Increase the month
  uint16_t year = now.year();
  uint8_t month = now.month();

  if (month == 1) {
    month = 12; // Wrap around to January
    year--;    // Increment the year
  } else {
    month--;   // Increment the month
  }

  // Adjust the RTC with the new date
  DateTime newDate(year, month, now.day(), now.hour(), now.minute(), now.second());
  rtc.adjust(newDate);
  idNumber = newDate.month();
}

void setMonthMenu() {
  setSetterMenu(increaseMonth, decreaseMonth, rtc.now().month());
}

// Nap növelése
void increaseDay() {
  DateTime dt = rtc.now();
  dt = dt + TimeSpan(1, 0, 0, 0);
  rtc.adjust(dt); // Állítsd be a számítógép idejét
  idNumber = dt.day();
}

// Nap csökkentése
void decreaseDay() {
  DateTime dt = rtc.now();
  dt = dt + TimeSpan(-1, 0, 0, 0);
  rtc.adjust(dt); // Állítsd be a számítógép idejét
  idNumber = dt.day();
}

void setDayMenu() {
  setSetterMenu(increaseDay, decreaseDay, rtc.now().day());
}

// Óra növelése
void increaseHour() {
  DateTime dt = rtc.now();
  if (dt.hour() == 23) return;
  dt = dt + TimeSpan(0, 1, 0, 0);
  rtc.adjust(dt); // Állítsd be a számítógép idejét
  idNumber = dt.hour();
}

// Óra csökkentése
void decreaseHour() {
  DateTime dt = rtc.now();
  if (dt.hour() == 0) return;
  dt = dt + TimeSpan(0, -1, 0, 0);
  rtc.adjust(dt); // Állítsd be a számítógép idejét
  idNumber = dt.hour();
}

void setHourMenu() {
  setSetterMenu(increaseHour, decreaseHour, rtc.now().hour());
}

// Perc növelése
void increaseMinute() {
  DateTime dt = rtc.now();
  if (dt.minute() == 59) return;
  dt = dt + TimeSpan(0, 0, 1, 0);
  rtc.adjust(dt); // Állítsd be a számítógép idejét
  idNumber = dt.minute();
}

// Perc csökkentése
void decreaseMinute() {
  DateTime dt = rtc.now();
  if (dt.minute() == 0) return;
  dt = dt + TimeSpan(0, 0, -1, 0);
  rtc.adjust(dt); // Állítsd be a számítógép idejét
  idNumber = dt.minute();
}

void setMinuteMenu() {
  setSetterMenu(increaseMinute, decreaseMinute, rtc.now().minute());
}

// Perc csökkentése
void resetSecMenu() {
  DateTime now = rtc.now();
  DateTime newTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), 0); // Reset seconds to 0
  rtc.adjust(newTime); // Update RTC with the new time
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