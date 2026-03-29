#include "espcyd.h"

/* externs from main.cpp */
extern volatile uint8_t myNodeID[4];
extern bool wifi_connected;

/** Task 2: Update Display */


void TaskUpdateDisplay(void * pvParameters)
{
  touchData_t receivedTouch;
  char receivedTime[10];
  uint32_t lastPressTime = 0; 
  const uint32_t debounceDelay = 750;  /**< Milliseconds to wait between valid presses */
  
  /* flags to keep track of what has been drawn */
  static bool buttons_drawn = false;
  static bool ip_drawn = false;
  static bool footer_drawn = false;

  static uint32_t lastCANCheck = 0;    /**< Tracks the last time we checked the bus */
  static bool lastCANState = false;    /**< Tracks the previous state to detect changes */  
  static bool ui_initialized = false;  /**< Flag to track if the UI has been initialized */
  static int32_t lastRSSI = 0;         /**< WiFi RSSI value for signal strength indicator */

  /* Variables for local time polling */
  static uint32_t lastTimeUpdate = 0;
  struct tm timeinfo;
  char timeString[10];

  Serial.println("[CYD] Display Task Started");
//   digitalWrite(LED_BLUE, LOW); /* Turn on the blue LED */

  for(;;) {
    uint32_t currentMillis = millis();

    /* STATE 1: Waiting for CAN Introduction Acknowledgement */
    if (!ui_initialized) {
        if (xSemaphoreTake(spiSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
            drawKeypad();
            /* Draw footer immediately upon entry to main UI */
            if (wifi_connected) drawFooter();
            ui_initialized = true;
            xSemaphoreGive(spiSemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        continue; 
    }

    /* STATE 2: Normal UI Operation */
    /* 1000ms Refresh Loop */
    if (currentMillis - lastTimeUpdate >= 1000) { /* The one-second loop */

        /* Check for stale nodes */
        lastTimeUpdate = currentMillis;

        /* Check if we need to dim the screen */
        cydScreenDimmer();

        bool stateChanged = false;
        for (int i = 0; i < 5; i++) {
            if (discoveredNodes[i].id != 0) {
                /** * If node was active but hasn't been seen for > 30s, 
                 * mark as inactive and trigger a UI refresh.
                 */
                if (discoveredNodes[i].active && (currentMillis - discoveredNodes[i].lastSeen > 30000)) {
                    discoveredNodes[i].active = false;
                    stateChanged = true;
                    Serial.printf("Node 0x%08X timed out.\n", discoveredNodes[i].id);
                }
            }
        }

        if (xSemaphoreTake(spiSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
            /* Refresh current screen if a node dropped or if in System Info */
            if ((currentMode == MODE_SYSTEM_INFO) || (stateChanged && currentMode == MODE_NODE_SEL)) {
                refreshCurrentScreen();
            }
            xSemaphoreGive(spiSemaphore);
        }


    } /* End 1000ms refresh loop */
    
    /* Check for Touch Data */
    if (xQueueReceive(touchQueue, &receivedTouch, 0)) {
        uint32_t currentTime = millis();
        
        if (currentTime - lastPressTime > debounceDelay) {
            lastPressTime = currentTime; // Move debounce lock to the start
            tsLastTouch = currentTime; /* Keep track of last touch for screen dimming */

            /**
             * @section Header Processing
             * Handle global navigation (Mode switching, Node cycling, Hamburger)
             */
            if (receivedTouch.y < 45) {
                bool handled = false;

                /* Left Target: Mode Toggle (x=0 to 80) */
                if (receivedTouch.x < 80) {
                    currentMode = (currentMode == MODE_HOME) ? MODE_COLOR_PICKER : MODE_HOME;
                    handled = true;
                } 
                /* Right Target: Hamburger Menu (x=240 to 320) */
                else if (receivedTouch.x > 240) {
                    currentMode = MODE_HAMBURGER_MENU;
                    handled = true;
                }
                /* Center Target: Cycle Nodes (x=80 to 240) */
                else if (receivedTouch.x >= 80 && receivedTouch.x <= 240) {
                    selectedNodeIdx = (selectedNodeIdx + 1) % 5;
                    if(discoveredNodes[selectedNodeIdx].id == 0) selectedNodeIdx = 0;
                    handled = true;
                }

                if (handled) {
                    if (xSemaphoreTake(spiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
                        refreshCurrentScreen(); // Uses the helper function below
                        xSemaphoreGive(spiSemaphore);
                    }
                    continue;
                }
            }

            /**
             * @section Content Processing
             * Use switch-case for mode-specific screen interactions
             */
            switch (currentMode) {
                case MODE_HOME: {
                    for (int i = 0; i < 4; i++) {
                        if (receivedTouch.x >= buttons[i].x && receivedTouch.x <= (buttons[i].x + buttons[i].w) &&
                            receivedTouch.y >= buttons[i].y && receivedTouch.y <= (buttons[i].y + buttons[i].h)) {
                            
                            /* Visual Feedback */
                            if (xSemaphoreTake(spiSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
                                tft.drawRoundRect(buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h, 8, TFT_RED);
                                xSemaphoreGive(spiSemaphore);
                            }

                            uint8_t canData[5];
                            memcpy(canData, (void*)myNodeID, 4);
                            canData[4] = (uint8_t)buttons[i].canID;
                            send_message_cb(SW_MOM_PRESS_ID, canData, SW_MOM_PRESS_DLC);

                            vTaskDelay(pdMS_TO_TICKS(150)); 

                            if (xSemaphoreTake(spiSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
                                tft.drawRoundRect(buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h, 8, TFT_WHITE);
                                xSemaphoreGive(spiSemaphore);
                            }
                        }
                    }
                    break;
                }

                case MODE_COLOR_PICKER: {
                    /* Ensure touch is within the palette grid area */
                    if (receivedTouch.y >= 45 && receivedTouch.y < 225) {
                        int col = receivedTouch.x / 40;
                        int row = (receivedTouch.y - 45) / 45;
                        
                        /* Clamp values to grid bounds */
                        if (col > 7) col = 7;
                        if (row > 3) row = 3;

                        int colorIdx = (row * 8) + col;

                        if (colorIdx >= 0 && colorIdx < 32) {
                            /* Update local state */
                            discoveredNodes[selectedNodeIdx].lastColorIdx = colorIdx;

                            /* Construct and send CAN message */
                            uint32_t targetID = discoveredNodes[selectedNodeIdx].id;
                            uint8_t canData[6];
                            canData[0] = (targetID >> 24) & 0xFF;
                            canData[1] = (targetID >> 16) & 0xFF;
                            canData[2] = (targetID >> 8) & 0xFF;
                            canData[3] = targetID & 0xFF;
                            canData[4] = 0; // LED Strip/Index
                            canData[5] = (uint8_t)colorIdx;

                            send_message_cb(SET_ARGB_STRIP_COLOR_ID, canData, SET_ARGB_STRIP_COLOR_DLC);

                            /* Trigger immediate redraw for the selection highlight */
                            if (xSemaphoreTake(spiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
                                drawColorPicker(); 
                                xSemaphoreGive(spiSemaphore);
                            }
                        }
                    }
                    break;
                }

                case MODE_NODE_SEL: {
                    int clickedIdx = (receivedTouch.y - 45) / 38;
                    if (clickedIdx >= 0 && clickedIdx < 5 && discoveredNodes[clickedIdx].id != 0) {
                        selectedNodeIdx = clickedIdx;
                        if (xSemaphoreTake(spiSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
                            drawNodeSelector();
                            xSemaphoreGive(spiSemaphore);
                        }
                    }
                    break;
                }

            case MODE_HAMBURGER_MENU: {
                for (int i = 0; i < 4; i++) {
                    if (receivedTouch.x >= buttons[i].x && receivedTouch.x <= (buttons[i].x + buttons[i].w) &&
                        receivedTouch.y >= buttons[i].y && receivedTouch.y <= (buttons[i].y + buttons[i].h)) {
                        
                        currentMode = (cydDisplayMode_t)i; // 0=Home, 1=Picker, 2=Nodes, 3=System
                        
                        if (xSemaphoreTake(spiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
                            refreshCurrentScreen();
                            xSemaphoreGive(spiSemaphore);
                        }
                        break;
                    }
                }
                break;
            }

            case MODE_SYSTEM_INFO: {
                /* Optional: Add a button here to reset CAN counters or Reconnect WiFi */
                break;
            }
            } /* end switch(currentMode) */
        } /* end debounce */
    } /* end queue receive */

    /* 3. Explicitly yield to the IDLE task */
    vTaskDelay(pdMS_TO_TICKS(5));
  } /* closing for(;;) */
} /* closing TaskUpdateDisplay() */

