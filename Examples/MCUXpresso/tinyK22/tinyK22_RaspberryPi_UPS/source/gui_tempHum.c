/*
 * Copyright (c) 2019, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "platform.h"
#if (PL_CONFIG_USE_SHT31 || PL_CONFIG_USE_SHT40) && PL_CONFIG_USE_GUI
#include "gui_tempHum.h"
#include "LittlevGL/lvgl/lvgl.h"
#include "Sensor.h"
#include "McuXFormat.h"
#include "gui.h"
#include "toaster.h"

static lv_obj_t *win;
static lv_obj_t *chart_label;
static lv_obj_t *chart;
static lv_chart_series_t *temperature_ser, *humidity_ser;
#define CHART_MAX_VALUE   100
#define CHART_POINT_NUM   100
#if LV_COLOR_DEPTH==1
  #define TEMPERATURE_LABEL_COLOR   "000000"
  #define HUMIDITY_LABEL_COLOR      "000000"
#else
  #define TEMPERATURE_LABEL_COLOR   "FF0000"
  #define HUMIDITY_LABEL_COLOR      "00FF00"
#endif
static lv_task_t *refr_task;
#define REFR_TIME_MS   (1000)

/**
 * Called when the window's close button is clicked
 * @param btn pointer to the close button
 * @return LV_ACTION_RES_INV because the window is deleted in the function
 */
static void win_close_action(lv_obj_t *btn, lv_event_t event) {
  if (event==LV_EVENT_RELEASED) {
  #if PL_CONFIG_USE_GUI_KEY_NAV
    GUI_GroupPull();
  #endif
    lv_obj_del(win);
    win = NULL;
    lv_task_del(refr_task);
    refr_task = NULL;
  }
}

/**
 * Called periodically to monitor the LED.
 * @param param unused
 */
static void refresh_task(struct _lv_task_t *param) {
  unsigned char buf[48];
  float temperature, humidity;
  int16_t chart_tvalue;
  int16_t chart_hvalue;

#if PL_CONFIG_USE_TOASTER
  if (TOASTER_IsRunning()) {
    return;
  }
#endif
  temperature = SENSOR_GetTemperature();
  humidity = SENSOR_GetHumidity();
  chart_tvalue = temperature+30; /* add offset */
  if (chart_tvalue>CHART_MAX_VALUE) {
    chart_tvalue = CHART_MAX_VALUE;
  } else if (chart_tvalue<0) {
    chart_tvalue = 0;
  }
  chart_hvalue = humidity;
  if (chart_hvalue>CHART_MAX_VALUE) {
    chart_hvalue = CHART_MAX_VALUE;
  } else if (chart_hvalue<0) {
    chart_hvalue = 0;
  }
  /*Add the data to the chart*/
  lv_chart_set_next(chart, temperature_ser, chart_tvalue);
  lv_chart_set_next(chart, humidity_ser, chart_hvalue);

  McuXFormat_xsnprintf((char*)buf, sizeof(buf), "%s%s T: %.1fC%s %s%s RH: %.1f%%%s",
    LV_TXT_COLOR_CMD, TEMPERATURE_LABEL_COLOR, temperature, LV_TXT_COLOR_CMD,
    LV_TXT_COLOR_CMD, HUMIDITY_LABEL_COLOR,    humidity, LV_TXT_COLOR_CMD
    );
  lv_label_set_text(chart_label, (char*)buf);
}

void GUI_TEMPHUM_CreateView(void) {
  lv_obj_t *closeBtn;
  refr_task = lv_task_create(refresh_task, REFR_TIME_MS, LV_TASK_PRIO_LOW, NULL);

  win = lv_win_create(lv_scr_act(), NULL);
#if PL_CONFIG_USE_SHT31
  lv_win_set_title(win, "SHT31");
#elif PL_CONFIG_USE_SHT40
  lv_win_set_title(win, "SHT40");
#else
  #error "device?"
#endif
  lv_win_set_btn_size(win, 15);
  closeBtn = lv_win_add_btn(win, LV_SYMBOL_CLOSE);
  lv_obj_set_event_cb(closeBtn, win_close_action);
#if PL_CONFIG_USE_GUI_KEY_NAV
  GUI_GroupPush();
  GUI_AddObjToGroup(closeBtn);
#endif
  lv_group_focus_obj(closeBtn);
  /* Make the window content responsive */
  lv_win_set_layout(win, LV_LAYOUT_PRETTY);

  /* Create a chart with two data lines */
  chart = lv_chart_create(win, NULL);
  lv_obj_set_size(chart, LV_HOR_RES/2, LV_VER_RES/2);

  lv_obj_set_pos(chart, LV_DPI/10, LV_DPI/10);

  lv_chart_set_point_count(chart, CHART_POINT_NUM);
  lv_chart_set_range(chart, 0, CHART_MAX_VALUE);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_series_width(chart, 3);
  temperature_ser =  lv_chart_add_series(chart, /*LV_COLOR_RED*/LV_COLOR_BLACK);
  humidity_ser =  lv_chart_add_series(chart, /*LV_COLOR_GREEN*/LV_COLOR_BLACK);

  /* Set the data series to zero */
  uint16_t i;
  for(i = 0; i<CHART_POINT_NUM; i++) {
      lv_chart_set_next(chart, temperature_ser, 0);
      lv_chart_set_next(chart, humidity_ser, 0);
  }
  /* label for values: */
  chart_label = lv_label_create(win, NULL);
  lv_label_set_recolor(chart_label, true);
  lv_obj_align(chart_label, chart, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI/5, 0);

  /* Refresh the chart and label manually at first */
  refresh_task(NULL);
}
#endif /* PL_CONFIG_HAS_SGP30 */
