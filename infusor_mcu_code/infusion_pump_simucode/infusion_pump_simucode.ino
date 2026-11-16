#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BTN_UP    16
#define BTN_DOWN  17
#define BTN_OK    18
#define BTN_BACK  19

#define MENU_W 104
#define NAV_X  108

const uint16_t checkIconCols[18] = {
  0x0060, 0x00F0, 0x01F8, 0x03FC, 0x07F8, 0x0FF0,
  0x1FE0, 0x3FC0, 0x1F80, 0x0F00, 0x0780, 0x03C0,
  0x01E0, 0x00F0, 0x0078, 0x003C, 0x0018, 0x0000
};

enum Screen : uint8_t {
  SCREEN_LOADING,
  SCREEN_MAIN,
  SCREEN_SETTINGS,
  SCREEN_EDIT
};

Screen currentScreen = SCREEN_MAIN;
uint8_t mainCursor     = 0;
uint8_t settingsCursor = 0;
uint8_t editItem       = 0;

uint8_t stepValues[5] = {5, 10, 25, 50, 100}; // в микролитрах
uint8_t savedStepValue = 3;
uint8_t currentStepValue = savedStepValue;

float flowRateMin = 0.1f; // мл/час
float flowRateMax = 2.0f; // мл/час
float flowRateStep = 0.1f; // мл/час
uint8_t currentFlowRate = 10;
float savedFlowRate = currentFlowRate * flowRateStep;

float bolusVolumeMin = 0.1f; // мл
float bolusVolumeMax = 2.0f; // мл
float bolusVolumeStep = 0.1f; // мл
uint8_t currentBolusVolume = 10;
float savedBolusVolume = currentBolusVolume * bolusVolumeStep;

uint8_t bolusTimeValues[3] = {20, 40, 60}; // сек
uint8_t savedBolusTime = 1;
uint8_t currentBolusTime = savedBolusTime;

const char* mainItems[4] = {
  "Move forward",
  "Move back",
  "Bolus",
  "Settings"
};

const char* settingsItems[4] = {
  "Step",
  "Flowrate",
  "Bolus volume",
  "Bolus time"
};

struct Button {
  uint8_t pin;
  bool lastRead;
  bool stable;
  unsigned long lastChange;
};

Button btnUp   = {BTN_UP,   true, true, 0};
Button btnDown = {BTN_DOWN, true, true, 0};
Button btnOk   = {BTN_OK,   true, true, 0};
Button btnBack = {BTN_BACK, true, true, 0};

void initButton(Button &b) {
  pinMode(b.pin, INPUT_PULLUP);
  b.lastRead  = digitalRead(b.pin);
  b.stable    = b.lastRead;
  b.lastChange = millis();
}

bool pollButton(Button &b) {
  bool r = digitalRead(b.pin);
  if (r != b.lastRead) {
    b.lastChange = millis();
    b.lastRead = r;
  }
  if ((millis() - b.lastChange) > 25) {
    if (r != b.stable) {
      b.stable = r;
      return (b.stable == LOW);
    }
  }
  return false;
}

// ====== Отрисовка: иконки навигации ======
void drawNav() {
  // ↑
  display.fillTriangle(118, 3, 112, 13, 124, 13, WHITE);
  // ↓
  display.fillTriangle(112, 19, 124, 19, 118, 29, WHITE);
  // OK
  display.setTextColor(WHITE, BLACK);
  display.setCursor(112, 36);
  display.print("OK");
  // ←
  // display.fillTriangle(118, 51, 118, 61, 110, 56, WHITE);
  display.setCursor(112, 52);
  display.print("↩");
}

void showSavedPopup() {
  display.fillRect(0, 12, MENU_W, display.height() - 12, BLACK);

  int8_t w = 84;
  int8_t h = 44;
  int8_t x = (MENU_W - w) / 2;
  int8_t y = 12 + ((display.height() - 12) - h) / 2;

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(x + (w - 36) / 2, y + 30);
  display.print("Saved!");
  display.display();

  int8_t iconW = 18;
  int8_t iconH = 16;
  int8_t startX = x + (w - iconW) / 2;
  int8_t startY = y + 7;

  for (int8_t col = 0; col < iconW; col++) {
    uint16_t colData = checkIconCols[col];
    for (int8_t row = 0; row < iconH; row++) {
      if (colData & (1UL << row)) {
        display.drawPixel(startX + col, startY + row, WHITE);
      }
    }
    display.display();
    delay(32);        
  }
  delay(700);
}

void drawMenuLine(int y, const char* text, bool selected) {
  if (selected) {
    display.fillRect(0, y - 2, MENU_W, 12, WHITE);
    display.setTextColor(BLACK, WHITE);
  } else {
    display.setTextColor(WHITE, BLACK);
  }
  display.setCursor(2, y);
  display.print(text);
}

void drawMenuScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  for (int i = 0; i < 4; i++) {
    if (currentScreen == SCREEN_MAIN) drawMenuLine(2 + i * 16, mainItems[i], i == mainCursor);
    else if (currentScreen == SCREEN_SETTINGS) drawMenuLine(2 + i * 16, settingsItems[i], i == settingsCursor);
  }
  display.drawFastVLine(106, 0, 64, WHITE);   // разделитель
  drawNav();
  display.display();
}

void drawEdit(const char* text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  int len = strlen(text);
  display.setCursor(MENU_W / 2 - len * 3, 2);
  display.print(text);

  display.fillTriangle(10, 33, 10, 43, 2, 38, WHITE);
  display.fillTriangle(94, 33, 94, 43, 102, 38, WHITE);

  display.drawFastVLine(106, 0, 64, WHITE);
  drawNav();
  display.display();
}

void drawValue (String value){
  display.fillRect(12, 12, 80, 52, BLACK);
  display.setTextSize(3);
  int len = value.length();
  display.setCursor(MENU_W / 2 - len * 3 * 3, 26);
  display.print(value);
  display.display();
}

void handleMain() {
  if (pollButton(btnUp)) {
    mainCursor = (mainCursor + 3) % 4;
    drawMenuScreen();
  }
  if (pollButton(btnDown)) {
    mainCursor = (mainCursor + 1) % 4;
    drawMenuScreen();
  }
  if (pollButton(btnOk)) {
    switch (mainCursor) {
      case 0:
        break;
      case 1:
        break;
      case 2:
        break;
      case 3:
        settingsCursor = 0;
        currentScreen = SCREEN_SETTINGS;
        drawMenuScreen();
        break;
    }
  }
}

void handleSettings() {
  if (pollButton(btnUp)) {
    settingsCursor = (settingsCursor + 3) % 4;
    drawMenuScreen();
  }
  if (pollButton(btnDown)) {
    settingsCursor = (settingsCursor + 1) % 4;
    drawMenuScreen();
  }
  if (pollButton(btnOk)) {
    switch (settingsCursor) {
      case 0:
        currentStepValue = savedStepValue;
        currentScreen = SCREEN_EDIT;
        drawEdit(settingsItems[editItem]);
        drawValue(String(stepValues[savedStepValue]));
        break;
      case 1:
        currentFlowRate = (uint8_t)round(savedFlowRate / flowRateStep);
        currentScreen = SCREEN_EDIT;
        drawEdit(settingsItems[editItem]);
        drawValue(String(savedFlowRate, 1));
        break;
      case 2:
        currentBolusTime = (uint8_t)round(savedBolusVolume / bolusVolumeStep);
        currentScreen = SCREEN_EDIT;
        drawEdit(settingsItems[editItem]);
        drawValue(String(savedBolusVolume, 1));
        break;
      case 3:
        currentBolusTime = savedBolusTime;
        currentScreen = SCREEN_EDIT;
        drawEdit(settingsItems[editItem]);
        drawValue(String(bolusTimeValues[savedBolusTime]));
        break;
    }
  }
  if (pollButton(btnBack)) {
    currentScreen = SCREEN_MAIN;
    drawMenuScreen();
  }
}

void handleEdit() {
  if (pollButton(btnUp)) {
    switch (settingsCursor){
      case 0:{
        currentStepValue += 1;
        if (currentStepValue >= 5) currentStepValue = 0;
        drawValue(String(stepValues[currentStepValue]));
        break;
      }
      case 1:{
        currentFlowRate += 1;
        float flowRate = currentFlowRate * flowRateStep;
        if (flowRate > flowRateMax){
          currentFlowRate = 1;
          flowRate = flowRateMin;
        }
        drawValue(String(flowRate, 1));
        break;
      }
      case 2:{
        currentBolusVolume += 1;
        float bolusVolume = currentBolusVolume * bolusVolumeStep;
        if (bolusVolume > bolusVolumeMax){
          currentBolusVolume = 1;
          bolusVolume = bolusVolumeMin;
        }
        drawValue(String(bolusVolume, 1));
        break;
      }
      case 3:{
        currentBolusTime += 1;
        if (currentBolusTime >= 3) currentBolusTime = 0;
        drawValue(String(bolusTimeValues[currentBolusTime]));
        break;
      }
    }
  }
  if (pollButton(btnDown)) {
    switch (settingsCursor){
      case 0:{
        if (currentStepValue == 0) currentStepValue = 4;
        else currentStepValue -= 1;
        drawValue(String(stepValues[currentStepValue]));
        break;
      }
      case 1:{
        if (currentFlowRate == 1) currentFlowRate = 20;
        else currentFlowRate -= 1;
        float flowRate = currentFlowRate * flowRateStep;
        drawValue(String(flowRate, 1));
        break;
      }
      case 2:{
        if (currentBolusVolume == 1) currentBolusVolume = 20;
        else currentBolusVolume -= 1;
        float bolusVolume = currentBolusVolume * bolusVolumeStep;
        drawValue(String(bolusVolume, 1));
        break;
      }
      case 3:{
        if (currentBolusTime == 0) currentBolusTime = 2;
        else currentBolusTime -= 1;
        drawValue(String(bolusTimeValues[currentBolusTime]));
        break;
      }
    }
  }
  if (pollButton(btnOk)) {
    switch (settingsCursor){
      case 0:
        savedStepValue = currentStepValue;
        showSavedPopup();
        currentScreen = SCREEN_SETTINGS;
        drawMenuScreen();
        break;
      case 1:
        savedFlowRate = currentFlowRate * flowRateStep;
        showSavedPopup();
        currentScreen = SCREEN_SETTINGS;
        drawMenuScreen();
        break;
      case 2:
        savedBolusVolume = currentBolusVolume * bolusVolumeStep;
        showSavedPopup();
        currentScreen = SCREEN_SETTINGS;
        drawMenuScreen();
        break;
      case 3:
        savedBolusTime = currentBolusTime;
        showSavedPopup();
        currentScreen = SCREEN_SETTINGS;
        drawMenuScreen();
        break;
    }
  }
  if (pollButton(btnBack)) {
    currentScreen = SCREEN_SETTINGS;
    drawMenuScreen();
  }
}



void setup() {
  Serial.begin(115200);

  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 init failed");
    for (;;) {}
  }
  display.clearDisplay();
  display.display();

  initButton(btnUp);
  initButton(btnDown);
  initButton(btnOk);
  initButton(btnBack);

  drawMenuScreen();

}

void loop() {
  switch (currentScreen) {
    case SCREEN_MAIN:     handleMain();     break;
    case SCREEN_SETTINGS: handleSettings(); break;
    case SCREEN_EDIT:     handleEdit();     break;
  }

}
