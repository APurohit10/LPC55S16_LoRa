/*
 * Copyright (c) 2019, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* own modules and standard library */
#include "McuArmTools.h"
#include "application.h" /* own interface */
#include <ctype.h> /* for isupper() */
#include "platform.h"
/* SDK */
#include "fsl_lpuart.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "pin_mux.h"
#include "board.h"

/* own modules */
#include "leds.h"
#include "initPins.h"
#include "gateway.h"
#include "shutdown.h"
#include "oled.h"
#include "McuRB.h"

/* McuLib */
#include "McuWait.h"
#include "McuLED.h"
#include "McuArmTools.h"
#include "McuGenericI2C.h"
#include "McuGenericSWI2C.h"
#if PL_CONFIG_USE_GUI
  #include "lv.h"
  #include "gui.h"
#endif
#if PL_CONFIG_USE_UPS
  #include "ups.h"
#endif
#include "McuRTOS.h"
#include "FreeRTOS.h"
#include "buttons.h"
#include "RaspyUART.h"
#include "McuRTT.h"
#include "virtual_com.h"

static void AppTask(void *pv) {
#if PL_CONFIG_USE_UPS
  float voltage, prevVoltage = 0.0f, charge;
  bool isCharging = true;
  int chargeCounter = 0; /* counting up for charging, counting down for discharging */
#endif

  /* blink the LEDs on startup */
  for(int i=0;i<5;i++) {
    McuLED_On(hatRedLED);
    vTaskDelay(pdMS_TO_TICKS(100));
    McuLED_Off(hatRedLED);
    McuLED_On(hatYellowLED);
    vTaskDelay(pdMS_TO_TICKS(100));
    McuLED_Off(hatYellowLED);
    McuLED_On(hatGreenLED);
    vTaskDelay(pdMS_TO_TICKS(100));
    McuLED_Off(hatGreenLED);
#if TINYK22_HAT_VERSION < 7
    McuLED_On(hatBlueLED);
    vTaskDelay(pdMS_TO_TICKS(100));
    McuLED_Off(hatBlueLED);
#endif
  }
#if PL_CONFIG_USE_UPS
  UPS_SetIsCharging(false);
#endif
  for(;;) { /* main loop */
  #if PL_CONFIG_USE_UPS
    if (UPS_GetCharge(&charge)==0 && UPS_GetVoltage(&voltage)==0) {
      if (prevVoltage==0.0f) { /* first check */
        prevVoltage = voltage;
      }
      if (voltage-prevVoltage>0.02f) {
        chargeCounter++;
      } else if (prevVoltage-voltage>0.02f) {
        chargeCounter--;
      }
      if (!isCharging && chargeCounter>=5) {
        isCharging = true;
        UPS_SetIsCharging(true);
        prevVoltage = voltage;
        chargeCounter = 5;
      } else if (isCharging && chargeCounter<=-5) {
        isCharging = false;
        UPS_SetIsCharging(false);
        prevVoltage = voltage;
        chargeCounter = -5;
      }
    #if PL_CONFIG_USE_SHUTDOWN
      if (charge<20.0f && !isCharging) { /* low battery and not is charging => power down */
        SHUTDOWN_RequestPowerOff();
      }
    #endif
    } /* for */
#endif /* PL_CONFIG_USE_UPS */
#if PL_CONFIG_USE_RASPY_UART
    if (BTN_DownButtonIsPressed()) {
      int counter = 0;
      do {
        counter++;
        McuLED_Toggle(tinyLED);
        vTaskDelay(pdMS_TO_TICKS(100));
      } while(BTN_DownButtonIsPressed() && counter<30); /* 3 secs */
      if (counter>=30) { /* pressed for more than 3 secs */
        RASPYU_SetEventsEnabled(!RASPYU_GetEventsEnabled()); /* toggle */
        McuRTT_WriteString(0, "Toggling Raspy UART Events, current status: ");
        if (RASPYU_GetEventsEnabled()) {
          McuRTT_WriteString(0, "on\r\n");
        } else {
          McuRTT_WriteString(0, "off\r\n");
        }
      }
    }
    if (RASPYU_GetEventsEnabled()) {
      McuLED_On(tinyLED);
      vTaskDelay(pdMS_TO_TICKS(100));
      McuLED_Off(tinyLED);
      vTaskDelay(pdMS_TO_TICKS(900));
    } else {
      McuLED_Toggle(tinyLED);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
#else
    McuLED_Toggle(tinyLED);
    vTaskDelay(pdMS_TO_TICKS(1000));
#endif /* PL_CONFIG_USE_RASPY_UART */
  }
}

void APP_Run(void) {
  PL_Init();
  if (xTaskCreate(
      AppTask,  /* pointer to the task */
      "App", /* task name for kernel awareness debugging */
      600/sizeof(StackType_t), /* task stack size */
      (void*)NULL, /* optional task startup argument */
      tskIDLE_PRIORITY+2,  /* initial priority */
      (TaskHandle_t*)NULL /* optional task handle to create */
    ) != pdPASS) {
     for(;;){} /* error! probably out of memory */
  }
  vTaskStartScheduler();
  for(;;) { /* should not get here */ }
}
