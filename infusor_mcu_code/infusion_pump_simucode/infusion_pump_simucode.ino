#include <Adafruit_SSD1306>
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

enum Screen : uint8_t {
  SCREEN_LOADING,
  SCREEN_MAIN,
  SCREEN_SETTINGS,
  SCREEN_EDIT,
  SCREEN_MESSAGE
};

Screen currentScreen = SCREEN_MAIN;
uint8_t mainCursor     = 0;
uint8_t settingsCursor = 0;
uint8_t editItem       = 0;
int stepValues[4] = [5, 10, 25, 50, 100]; // в микролитрах
float flowRateMin = 0.1f; // мл/час
float flowRateMax = 2.0f; // мл/час
float flowRateStep = 0.1f; // мл/час
float bolusVolumeMin = 0.1f; // мл
float bolusVolumeMax = 2.0f; // мл
float bolusVolumetep = 0.1f; // мл
int bolusTimeValues[3] = [20, 40, 60]; // сек

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
  display.fillTriangle(118, 51, 118, 61, 110, 56, WHITE);
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

void drawEdit(char* text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  int len = strlen(text);
  display.setCursor(MENU_W / 2 - len * 3, 2);
  display.print(text);

  display.fillTriangle(10, 33, 10, 43, 2, 38, WHITE);
  display.fillTriangle(94, 33, 94, 43, 102, 38, WHITE);

  display.setTextSize(3);
  display.setCursor(MENU_W / 2 - len * 3 * 3, 26);
  if (editItem == 0) display.print(stepTemp, 2);
  else               display.print(bolusTemp, 2);

  display.setTextSize(1);
  display.setCursor(2, 46);  display.print("OK  - save");
  display.setCursor(2, 56);  display.print("Back - cancel");

  display.drawFastVLine(106, 0, 64, WHITE);
  drawNav();
  display.display();
}

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
