#include "espcyd.h"

/* espcyd.cpp */

TFT_eSPI tft = TFT_eSPI();
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

SemaphoreHandle_t spiSemaphore = nullptr;
QueueHandle_t touchQueue = nullptr;
QueueHandle_t timeQueue = nullptr;


/* === Default no-op stubs for callbacks === */
static void noop_send(uint16_t, uint8_t*, uint8_t) {}
static void noop_backlight(uint8_t, uint8_t, uint32_t, uint32_t) {}

/* === Callbacks === */
send_message_cb_t send_message_cb = nullptr;
backlight_cb_t backlight_cb = nullptr;

void espcyd_set_send_message_callback(send_message_cb_t cb) 
{
    send_message_cb = cb;
}

void espcyd_set_backlight_callback(backlight_cb_t cb) 
{
    backlight_cb = cb;
}


/* Match the header exactly: volatile and explicit size */
volatile int discoveredNodeCount = 0;
volatile int selectedNodeIdx = 0;

argbNode_t discoveredNodes[MAX_ARGB_NODES] = {0};

/* CAN interface status from main.cpp */
extern volatile bool can_suspended;
extern volatile bool can_driver_installed;

/* Define task handles */
TaskHandle_t xDisplayHandle = NULL;
TaskHandle_t xTouchHandle = NULL;

/* Variables for the color picker routines - from main.cpp*/
cydDisplayMode_t currentMode = MODE_HOME; /**< Current display mode */

/* can tx function from main.cpp */
// extern void send_message(uint16_t msgid, uint8_t *data, uint8_t dlc);

/* hardware pwm function from main.cpp */
// extern void handleHardwareBlink(uint8_t submodIdx, uint8_t pin, uint32_t freq, uint32_t duty = (LEDC_13BIT_50PCT));

/* setup menu labels */
const char* const menuLabels[] = {
    "HOME", 
    "COLOR PICKER", 
    "NODE SELECT", 
    "SYSTEM INFO", 
    "HAMBURGER MENU"
};

/* zero init last touch timestamp */
uint32_t tsLastTouch = 0;

/* node ID for the data payload from main CPP */
extern volatile uint8_t myNodeID[4];
extern bool wifi_connected;

/* define screen off and dim flags */
bool screenOff = false; /* clear the screen off flag */
bool screenDim = false; /* clear the screen dim flag */

void initCYD() {
    spiSemaphore = xSemaphoreCreateBinary(); /* semaphore to control SPI access */
    xSemaphoreGive(spiSemaphore); /* unlock SPI access */
    
    touchQueue = xQueueCreate(5, sizeof(touchData_t));
    timeQueue = xQueueCreate(1, 10 * sizeof(char));

    Serial.println("CYD: Init");

    /* Clear the discovered nodes array to prevent garbage data on UI */
    memset(discoveredNodes, 0, sizeof(discoveredNodes));

    /* Power on the backlight */
    pinMode(CYD_BACKLIGHT, OUTPUT);
    digitalWrite(CYD_BACKLIGHT, HIGH);

    tft.begin(); /* initialize the display */
    tft.setRotation(1);
    /* Clear the screen before writing to it */
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);

    /* Setup the touchscreen */
    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); /* configure SPI interface for touchscreen */
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(1);
  
    /* Start the tasks */
    xTaskCreate(TaskReadTouch, "TouchTask", 4096, NULL, 2, &xTouchHandle);          /* assign task handles */
    xTaskCreate(TaskUpdateDisplay, "DisplayTask", 6144, NULL, 1, &xDisplayHandle);
}



/**
 * @brief Logic to register or update a discovered ARGB node
 * @param id The 32-bit Node ID extracted from the CAN frame
 */
void registerARGBNode(uint32_t id) {
    int emptySlot = -1;

    for (int i = 0; i < 5; i++) {
        /* Case 1: Node already exists in our table */
        if (discoveredNodes[i].id == id) {
            discoveredNodes[i].lastSeen = millis();
            discoveredNodes[i].active = true;
            return;
        }

        /* Keep track of the first available empty slot */
        if (emptySlot == -1 && discoveredNodes[i].id == 0) {
            emptySlot = i;
        }
    }

    /* Case 2: New node discovered and we have room */
    if (emptySlot != -1) {
        discoveredNodes[emptySlot].id = id;
        discoveredNodes[emptySlot].active = true;
        discoveredNodes[emptySlot].lastSeen = millis();
        
        Serial.printf("UI: Registered New ARGB Node [0x%08X] at slot %d\n", id, emptySlot);
    } else {
        /* Case 3: Project limit reached (5 nodes) */
        Serial.println("UI Warning: Discovered node ignored, table full.");
    }
}








