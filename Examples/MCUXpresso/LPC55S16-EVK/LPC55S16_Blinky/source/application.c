/*
 * Copyright (c) 2021, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "application.h"
#include "board.h"
#include "McuLib.h"
#include "McuWait.h"
#include "McuLED.h"
#include "McuRTOS.h"

#include "fsl_iocon.h"
#include "pin_mux.h"
#define IOCON_PIO_DIGITAL_EN 0x0100u  /*!<@brief Enables digital function */
#define IOCON_PIO_FUNC0 0x00u         /*!<@brief Selects pin function 0 */
#define IOCON_PIO_INV_DI 0x00u        /*!<@brief Input function is not inverted */
#define IOCON_PIO_MODE_PULLUP 0x20u   /*!<@brief Selects pull-up function */
#define IOCON_PIO_OPENDRAIN_DI 0x00u  /*!<@brief Open drain is disabled */
#define IOCON_PIO_SLEW_STANDARD 0x00u /*!<@brief Standard mode, output slew rate control is enabled */

#define LED_GPIO    GPIO
#define LED_PORT    1U

/* Blue: P1_6 */
#define LED_BLUE_GPIO    GPIO
#define LED_BLUE_PORT    1U
#define LED_BLUE_PIN     6U
/* Red: P1_4 */
#define LED_RED_GPIO     GPIO
#define LED_RED_PORT     1U
#define LED_RED_PIN      4U
/* Green: P1_7 */
#define LED_GREEN_GPIO   GPIO
#define LED_GREEN_PORT   1U
#define LED_GREEN_PIN    7U

static McuLED_Handle_t ledRed, ledGreen, ledBlue;
static SemaphoreHandle_t mutex;

static void Init(void) {
  McuLib_Init();
//  McuRTOS_Init();
  McuWait_Init();
  McuLED_Init();
}

static void AppTask(void *pv) {
  for(;;) {
    McuLED_Toggle(ledRed);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

static TimerHandle_t timerHndl;
#define TIMER_PERIOD_MS 100

static void vTimerCallback(TimerHandle_t pxTimer) {
  /* TIMER_PERIOD_MS ms timer */
}

void APP_Run(void) {
  McuLED_Config_t config;

  Init(); /* init modules */

  CLOCK_EnableClock(kCLOCK_Iocon);
  GPIO_PortInit(LED_GPIO, LED_PORT); /* ungate clock for port */

  const uint32_t port1_pin4_config = (/* Pin is configured as PIO1_4 */
                                      IOCON_PIO_FUNC0 |
                                      /* Selects pull-up function */
                                      IOCON_PIO_MODE_PULLUP |
                                      /* Standard mode, output slew rate control is enabled */
                                      IOCON_PIO_SLEW_STANDARD |
                                      /* Input function is not inverted */
                                      IOCON_PIO_INV_DI |
                                      /* Enables digital function */
                                      IOCON_PIO_DIGITAL_EN |
                                      /* Open drain is disabled */
                                      IOCON_PIO_OPENDRAIN_DI);
  /* PORT1 PIN4 (coords: 1) is configured as PIO1_4 */
  IOCON_PinMuxSet(IOCON, LED_RED_PORT, LED_RED_PIN, port1_pin4_config);
  IOCON_PinMuxSet(IOCON, LED_GREEN_PORT, LED_GREEN_PIN, port1_pin4_config);
  IOCON_PinMuxSet(IOCON, LED_BLUE_PORT, LED_BLUE_PIN, port1_pin4_config);

  McuLED_GetDefaultConfig(&config);
  config.isLowActive = false;
  config.isOnInit = false;
  config.hw.port = LED_RED_PORT;
  config.hw.gpio = LED_RED_GPIO;
  config.hw.pin = LED_RED_PIN;
  ledRed = McuLED_InitLed(&config);

  config.hw.port = LED_GREEN_PORT;
  config.hw.gpio = LED_GREEN_GPIO;
  config.hw.pin = LED_GREEN_PIN;
  ledGreen = McuLED_InitLed(&config);

  config.hw.port = LED_BLUE_PORT;
  config.hw.gpio = LED_BLUE_GPIO;
  config.hw.pin = LED_BLUE_PIN;
  ledBlue = McuLED_InitLed(&config);
  for(;;) {
    McuLED_On(ledRed);
    McuLED_Off(ledRed);
    McuLED_On(ledGreen);
    McuLED_Off(ledGreen);
    McuLED_On(ledBlue);
    McuLED_Off(ledBlue);

    McuLED_Toggle(ledRed);
    McuWait_Waitms(100);
    McuLED_Toggle(ledRed);
    McuWait_Waitms(100);
    McuLED_Toggle(ledGreen);
    McuWait_Waitms(100);
    McuLED_Toggle(ledGreen);
    McuWait_Waitms(100);
    McuLED_Toggle(ledBlue);
    McuWait_Waitms(100);
    McuLED_Toggle(ledBlue);
    McuWait_Waitms(100);
  } /* for */
  if (xTaskCreate(
      AppTask,  /* pointer to the task */
      "App", /* task name for kernel awareness debugging */
      300/sizeof(StackType_t), /* task stack size */
      (void*)NULL, /* optional task startup argument */
      tskIDLE_PRIORITY+2,  /* initial priority */
      (TaskHandle_t*)NULL /* optional task handle to create */
    ) != pdPASS) {
     for(;;){} /* error! probably out of memory */
  }
  timerHndl = xTimerCreate("timer0", pdMS_TO_TICKS(TIMER_PERIOD_MS), pdTRUE, (void *)0, vTimerCallback);
  if (timerHndl==NULL) {
    for(;;); /* failure! */
  }
  if (xTimerStart(timerHndl, 0)!=pdPASS) {
    for(;;); /* failure! */
  }
  mutex = xSemaphoreCreateMutex();
  if (mutex!=NULL) {
    vQueueAddToRegistry(mutex, "Mutex");
  }
  vTaskStartScheduler();
  for(;;) { /* should not get here */ }
}
