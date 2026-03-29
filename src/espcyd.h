#pragma once

/* === Framework includes === */
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <WiFi.h>

/* === ESP32 native IDF includes === */
#include "driver/twai.h" /**< Required for twai_status_info_t and twai_get_status_info() */
#include "time.h"

/* My project includes */
#include "canbus_project.h"

/* my colors */
#if defined(ARGB_LED) || defined(ESP32CYD)
#include "colorpalette.h"
#endif

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd/ or https://RandomNerdTutorials.com/esp32-tft/   */
#include <TFT_eSPI.h>

/** Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen
*   Note: this library doesn't require further configuration */
#include <XPT2046_Touchscreen.h>

/* === Macros and Constants === */

/** Touchscreen pins, this is setup in build_flags in platformio.ini */
#define XPT2046_IRQ 36   /* T_IRQ */
#define XPT2046_MOSI 32  /* T_DIN */
#define XPT2046_MISO 39  /* T_OUT */
#define XPT2046_CLK 25   /* T_CLK */
#define XPT2046_CS 33    /* T_CS */

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 320
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 240
#endif

#define FONT_SIZE 2

/** Cheap yellow display pin assignments */
#define LED_RED           (4U)               
#define LED_BLUE          (17U)
#define LED_GREEN         (16U)
#define CYD_BACKLIGHT     (21U) 
#define CYD_LDR           (34U)
#define CYD_SPEAKER       (26U)

/** Mamiumum number of ARGB nodes for the color picker */
#define MAX_ARGB_NODES    8

/** Submodule index for backlight */
#define CYD_BACKLIGHT_IDX 1 

/** PWM frequency for backlight pwm in Hz */
#define CYD_BACKLIGHT_PWM_HZ  1000

#define SCREEN_DIM_MS 10000 /**< 10 seconds screen dims */
#define SCREEN_OFF_MS 60000 /**< 1 minute screen off */


/* === Callback functions === */
typedef void (*send_message_cb_t)(uint16_t id, uint8_t *data, uint8_t dlc);
typedef void (*backlight_cb_t)(uint8_t idx, uint8_t pin, uint32_t freq, uint32_t duty);

/* === Callback pointers (extern declarations) === */
extern send_message_cb_t send_message_cb;
extern backlight_cb_t    backlight_cb;

extern void espcyd_set_send_message_callback(send_message_cb_t cb);
extern void espcyd_set_backlight_callback(backlight_cb_t cb);

/* === Typedefs === */

typedef enum  { 
    MODE_HOME = 0 , 
    MODE_COLOR_PICKER = 1, 
    MODE_NODE_SEL = 2, 
    MODE_SYSTEM_INFO = 3, 
    MODE_HAMBURGER_MENU = 4
} cydDisplayMode_t; /**< UI state - current display mode */

typedef struct {
  int x;
  int y;
  int z;
} touchData_t;

typedef struct {
    int x, y, w, h;
    char label[10];
    uint16_t canID;
    uint16_t color;
} keyPadButtons_t;

typedef struct {
    const char* label;
    uint16_t color;
    void (*drawIcon)(int x, int y);
} gridItem_t; /**< Container for button metadata to be used in drawUnifiedGrid */

typedef struct {
    uint32_t id;       /**< 32-bit Node ID */
    uint32_t lastSeen; /**< Heartbeat timestamp */
    int lastColorIdx;  /**< Last color index sent to this node */
    bool active;       /**< Status flag */
} argbNode_t; /**< Describe an ARGB node */


/* === Global Variables === */
extern TFT_eSPI tft;
extern String wifiIP; /**< Refers to the String defined in main.cpp */

/* Task handles */
extern TaskHandle_t xDisplayHandle; /* task in espcyd_task_display.cpp */
extern TaskHandle_t xTouchHandle;   /* task in espcyd_task_touch.cpp */

/* Library global variables for inter-task communication */
extern SPIClass touchscreenSPI;
extern XPT2046_Touchscreen touchscreen;

extern SemaphoreHandle_t spiSemaphore;
extern QueueHandle_t touchQueue;
extern QueueHandle_t timeQueue;

extern uint32_t tsLastTouch; /**< Timestamp of last cyd touch event */
extern bool screenOff; /**< True if screen is off */
extern bool screenDim; /**< True if screen is dimmed */

/* Menu items for the hamburger menu */
extern const char* const menuLabels[];

/* UI state variables */
extern keyPadButtons_t buttons[4];
extern cydDisplayMode_t currentMode;

extern volatile int   discoveredNodeCount; /**< Track active count in the array */
extern volatile int   selectedNodeIdx;
extern argbNode_t     discoveredNodes[MAX_ARGB_NODES]; /**< Size must be explicit here */

/* ========================================================================= */
/* Local functions and prototypes 
/* ========================================================================= */

/**
 * @brief Converts a NeoPixelBus RgbColor to a 16-bit RGB565 value for the TFT.
 * @param color The RgbColor to convert.
 * @return uint16_t The color in RGB565 format.
 */
static inline uint16_t colorTo565(PaletteColor color) 
{
    /* Scale 8-bit color channels to 5-6-5 bit depths */
    return ((color.R & 0xF8) << 8) | ((color.G & 0xFC) << 3) | (color.B >> 3);
}


/* Main functions */
void initCYD();

/* === Draw functions === */

/* Main screens */
void drawColorPicker();
void drawSplashScreen(const char* message);
void drawKeypad();
void drawSystemInfo();
void drawNodeSelector();

/* Icons and elements */
void drawInfoIcon(int x, int y);
void drawNetworkIcon(int x, int y);
void drawPaletteIcon(int x, int y);
void drawHomeIcon(int x, int y);
void drawWiFiStatus(int32_t rssi);
void drawHamburgerIcon();
void drawLightbarIcon(int x, int y);
void drawSeatWarmerIcon(int x, int y);
void drawWaterPumpIcon(int x, int y);
void drawDefrosterIcon(int x, int y);
void drawPickerIcon();
void drawHamburgerMenu();

/* Header and footer */
void drawFooter();
void drawHeader(const char* title);

/* Toolbox functions*/
void drawUnifiedGrid(const char* title, gridItem_t* items);
void refreshCurrentScreen();

/* === Display and Touch screen functions === */

/* Touch functions */
void TaskReadTouch(void * pvParameters);
void cydScreenDimmer();

/* Display functions */
void TaskUpdateDisplay(void * pvParameters);