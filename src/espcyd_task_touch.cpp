#include "espcyd.h"

/* ========================================================================= */
/* External Variables
/* ========================================================================= */

/* CAN interface status from main.cpp */
extern volatile bool can_suspended;
extern volatile bool can_driver_installed;

/* externs from main.cpp */
extern volatile uint8_t myNodeID[4];
extern bool wifi_connected;


/* Dim screen based on inactivity */
void cydScreenDimmer() 
{
  const uint32_t currentTime = millis();

  if (!screenOff) {
    if (currentTime - tsLastTouch > SCREEN_OFF_MS) { 
        /* Turn screen to minimum brightness */
        backlight_cb(CYD_BACKLIGHT_IDX, CYD_BACKLIGHT, CYD_BACKLIGHT_PWM_HZ, LEDC_13BIT_10PCT);
        screenOff = true;
        Serial.println("[CYD] Screen to min. brightness.");
    } else if ((currentTime - tsLastTouch > SCREEN_DIM_MS) && !screenDim) {
        /* Dim to 50% */
        backlight_cb(CYD_BACKLIGHT_IDX, CYD_BACKLIGHT, CYD_BACKLIGHT_PWM_HZ, LEDC_13BIT_50PCT); 
        screenDim = true;
        Serial.println("[CYD] Screen dimmed.");
    }
 }
}

/* ========================================================================= */
/* FreeRTOS Touch Task 
/* ========================================================================= */

void TaskReadTouch(void * pvParameters) 
{
  touchData_t currentTouch;
  Serial.println("[CYD] Touch Task Started");

  for(;;) {
    /* Only process touch if CAN is healthy and not suspended */
    if (can_driver_installed && !can_suspended) {
    //   digitalWrite(LED_RED, digitalRead(LED_RED) ^ 1); /* Toggle RED LED */

      /* Try to take the mutex (wait up to 10ms if busy) */
      if (touchscreen.tirqTouched() && touchscreen.touched()) {
        if (spiSemaphore != NULL && xSemaphoreTake(spiSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
          TS_Point p = touchscreen.getPoint();
          currentTouch.x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
          currentTouch.y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
          currentTouch.z = p.z;
          
          if (p.z > 800) { /* Only queue if the press is firm enough */
            xQueueSend(touchQueue, &currentTouch, 0);
          }

          /* Always give the mutex back! */
          xSemaphoreGive(spiSemaphore);

          /* If the screen is dimmed or off, turn it back on */
          if (screenDim || screenOff) {
              screenDim = false;
              screenOff = false;
              /* Turn screen back on */
              backlight_cb(CYD_BACKLIGHT_IDX, CYD_BACKLIGHT, CYD_BACKLIGHT_PWM_HZ, LEDC_13BIT_100PCT);
          }
        }
      }
    } else {
      /* Optional: Clear queue if bus drops to prevent latent actions */
      xQueueReset(touchQueue);
    }
    vTaskDelay(pdMS_TO_TICKS(20)); /* High polling rate for touch */
  }
}
