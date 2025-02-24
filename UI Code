#include <SPI.h>
#include <ILI9341_t3.h>


// TFT Display Pins
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_MOSI 11
#define TFT_MISO 12
#define TFT_SCK 13
#define TFT_LED 6 // Optional PWM control
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_LED);

// Button Pins
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_LATERAL 4
#define BTN_SELECT 5

int menuIndex = 0;
int subMenuIndex = 0;
int menuItems = 3;
int subMenuItems = 2; // Adjust per submenu
bool inSubMenu = false; // Track if we're in a submenu



void setup() {
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LATERAL, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);
    pinMode(TFT_LED, OUTPUT);
    analogWrite(TFT_LED, 255); // Set backlight to full brightness
    
    SPI.begin();
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(ILI9341_BLACK);
    drawMenu();
}

void loop() {
    handleButtons();
}

void handleButtons() {
    if (digitalRead(BTN_UP) == LOW) {
        if (inSubMenu) {
            subMenuIndex = (subMenuIndex - 1 + subMenuItems) % subMenuItems;
        } else {
            menuIndex = (menuIndex - 1 + menuItems) % menuItems;
        }
        drawMenu();
        delay(200);
    }
    if (digitalRead(BTN_DOWN) == LOW) {
        if (inSubMenu) {
            subMenuIndex = (subMenuIndex + 1) % subMenuItems;
        } else {
            menuIndex = (menuIndex + 1) % menuItems;
        }
        drawMenu();
        delay(200);
    }
    if (digitalRead(BTN_LATERAL) == LOW) {
        if (inSubMenu && menuIndex == 0) {
            //frequency += 0.005; // Adjust step
        }
        drawMenu();
        delay(200);
    }
    if (digitalRead(BTN_SELECT) == LOW) {
        if (!inSubMenu) {
            inSubMenu = true;
            subMenuIndex = 0;
        } else {
            inSubMenu = false; // Go back to main menu
        }
        drawMenu();
        delay(200);
    }
}
float frequency = 144.000; // Example starting frequency
;

void drawMenu() {
  
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    
    if (!inSubMenu){
    tft.setCursor(70, 20);
    tft.print("Ham Radio Menu");
    
    tft.setCursor(20, 60);
    tft.print(menuIndex == 0 ? "> Frequency Select" : "  Frequency Select");
    
    tft.setCursor(20, 100);
    tft.print(menuIndex == 2 ? "> Settings" : "  Settings");

    tft.setCursor(20, 140);
    tft.print("  Battery Status"); // This section is commented out for now

    } else {
        // Submenu
        tft.setCursor(20, 20);
        if (menuIndex == 0) {
            tft.print("Adjust Frequency");
            tft.setCursor(20, 60);
            tft.print(subMenuIndex == 0 ? "> Increase" : "  Increase");
            tft.setCursor(20, 100);
            tft.print(subMenuIndex == 1 ? "> Decrease" : "  Decrease");
        } else if (menuIndex == 2) {
            tft.print("Settings Menu");
            tft.setCursor(20, 60);
            tft.print(subMenuIndex == 0 ? "> Option 1" : "  Option 1");
            tft.setCursor(20, 100);
            tft.print(subMenuIndex == 1 ? "> Option 2" : "  Option 2");
        }
    }
    // Display frequency
    tft.setCursor(60, 180);
    tft.print("Freq: ");
    tft.print(frequency, 3);
    tft.print("KM/S");

    

    // Frequency settings


    tft.setCursor(210,220);
    tft.print("Pico-Vox");
}
