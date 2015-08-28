#include <pebble.h>
#include "face_handler.h"

//-- static globals --
static Window *s_window;
static GFont s_res_hoog_numbers;
static GFont s_res_scp;
static TextLayer *s_time_hr;
static TextLayer *s_time_hr2;
static TextLayer *s_time_min;
static TextLayer *s_time_min2;
static TextLayer *s_ampm;
static TextLayer *s_date_year;
static TextLayer *s_date_day;
static TextLayer *s_date_month;
static TextLayer *s_day;
static TextLayer *s_month;
static TextLayer *s_battery;

#define SPACE 32;

//-- functions --

static void update_daily(void)
{
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  static char date_year[]  = "2015";
  static char date_day[]   = "01";
  static char date_month[] = "01";
  static char day[]        = "F\nR\nI";
  static char month[]      = "J\nA\nN"; // vertical month
  
  strftime(date_year,  sizeof(date_year),  "%G", tick_time);
  strftime(date_day,   sizeof(date_day),   "%d", tick_time);
  strftime(date_month, sizeof(date_month), "%m", tick_time);
  
  {
    char buffer[4];
    strftime(buffer, sizeof(buffer), "%b", tick_time);
    month[0] = buffer[0];
    month[2] = buffer[1] - SPACE;
    month[4] = buffer[2] - SPACE;
    
    strftime(buffer, sizeof(day), "%a", tick_time);
    day[0] = buffer[0];
    day[2] = buffer[1] - SPACE;
    day[4] = buffer[2] - SPACE;
  }
  
  text_layer_set_text(s_date_year, date_year);
  text_layer_set_text(s_date_day, date_day);
  text_layer_set_text(s_date_month, date_month);
  text_layer_set_text(s_day, day);
  text_layer_set_text(s_month, month);
}

static void update_minute(void)
{
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  static char time_hr[]   = "0";
  static char time_hr2[]  = "0";
  static char time_min[]  = "0";
  static char time_min2[] = "0";
  static char ampm[]      = "AM";
         char buffer[]    = "00";
  
  if(clock_is_24h_style() == true)
    strftime(buffer, sizeof(buffer), "%H", tick_time);
  else
    strftime(buffer, sizeof(buffer), "%I", tick_time);
  time_hr[0] = buffer[0]; time_hr2[0] = buffer[1];
  
  strftime(buffer, sizeof(buffer), "%M", tick_time);
  time_min[0] = buffer[0]; time_min2[0] = buffer[1];
  
  strftime(ampm,     sizeof(ampm),     "%p", tick_time);
  
  text_layer_set_text(s_time_hr, time_hr);
  text_layer_set_text(s_time_hr2, time_hr2);
  text_layer_set_text(s_time_min, time_min);
  text_layer_set_text(s_time_min2, time_min2);
  text_layer_set_text(s_ampm, ampm);
}

static void battery_handler(BatteryChargeState charge_state)
{
  //                        0    1    2    3    4     5
  static char battery[] = {' ', '1', '0', '0', '%', '\0'};
  if(charge_state.charge_percent == 100) // full charge
  {
    battery[1] = '1';
    battery[2] = '0';
  }
  else
  {
    battery[1] = ' ';
    battery[2] = (char)(charge_state.charge_percent / 10) + '0';
  }
  
  if(charge_state.is_plugged)
  {
    battery[0] = 'C';
    // try to fix weird pebble battery reporting glitch
    if(charge_state.charge_percent >= 70)
    {
      battery[1] = '>';
      battery[2] = '7';
    }
  }
  else
    battery[0] = ' ';
  
  text_layer_set_text(s_battery, battery);
}

static void update_all(void)
{
  update_minute();
  update_daily();
  battery_handler(battery_state_service_peek()); // force refresh
}

static void time_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_minute();
  update_daily();
}

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, true);
  #endif
  
  //-- load font resources --
  s_res_hoog_numbers = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_hoog_numbers_80));
  s_res_scp = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SCP_12));
  
  //-- text layers --
  // s_time_hr
  s_time_hr = text_layer_create(GRect(50, -15, 50, 90));
  text_layer_set_background_color(s_time_hr, GColorClear);
  text_layer_set_text_color(s_time_hr, GColorWhite);
  text_layer_set_text_alignment(s_time_hr, GTextAlignmentRight);
  text_layer_set_font(s_time_hr, s_res_hoog_numbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_time_hr);
  
  // s_time_hr2
  s_time_hr2 = text_layer_create(GRect(50, -15, 100, 90));
  text_layer_set_background_color(s_time_hr2, GColorClear);
  text_layer_set_text_color(s_time_hr2, GColorWhite);
  text_layer_set_text_alignment(s_time_hr2, GTextAlignmentRight);
  text_layer_set_font(s_time_hr2, s_res_hoog_numbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_time_hr2);
  
  // s_time_min
  s_time_min = text_layer_create(GRect(50, 40, 50, 90));
  text_layer_set_background_color(s_time_min, GColorClear);
  text_layer_set_text_color(s_time_min, GColorWhite);
  text_layer_set_text_alignment(s_time_min, GTextAlignmentRight);
  text_layer_set_font(s_time_min, s_res_hoog_numbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_time_min);
  
  // s_time_min2
  s_time_min2 = text_layer_create(GRect(50, 40, 100, 90));
  text_layer_set_background_color(s_time_min2, GColorClear);
  text_layer_set_text_color(s_time_min2, GColorWhite);
  text_layer_set_text_alignment(s_time_min2, GTextAlignmentRight);
  text_layer_set_font(s_time_min2, s_res_hoog_numbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_time_min2);
  
  // s_ampm
  s_ampm = text_layer_create(GRect(50, 0, 16, 12));
  text_layer_set_background_color(s_ampm, GColorClear);
  text_layer_set_text_color(s_ampm, GColorWhite);
  text_layer_set_font(s_ampm, s_res_scp);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_ampm);
  
  // s_date_year = eg. 2015
  s_date_year = text_layer_create(GRect(0, 148, 28, 12));
  text_layer_set_background_color(s_date_year, GColorBlack);
  text_layer_set_text_color(s_date_year, GColorWhite);
  text_layer_set_text_alignment(s_date_year, GTextAlignmentCenter);
  text_layer_set_font(s_date_year, s_res_scp);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_date_year);
  
  // s_date_day = 01, 02, 03, ..., 31
  s_date_day = text_layer_create(GRect(0, 136, 28, 12));
  text_layer_set_background_color(s_date_day, GColorClear);
  text_layer_set_text_color(s_date_day, GColorWhite);
  text_layer_set_text_alignment(s_date_day, GTextAlignmentRight);
  text_layer_set_font(s_date_day, s_res_scp);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_date_day);
  
  // s_date_month = 01, 02, 03, ..., 12
  s_date_month = text_layer_create(GRect(0, 136, 28, 12));
  text_layer_set_background_color(s_date_month, GColorClear);
  text_layer_set_text_color(s_date_month, GColorWhite);
  text_layer_set_font(s_date_month, s_res_scp);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_date_month);
  
  // s_day = vertical day listing
  s_day = text_layer_create(GRect(40, 12, 24, 36));
  text_layer_set_background_color(s_day, GColorClear);
  text_layer_set_text_color(s_day, GColorWhite);
  text_layer_set_font(s_day, s_res_scp);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_day);
  
  // s_month = vertical month listing
  s_month = text_layer_create(GRect(40, 67, 24, 36));
  text_layer_set_background_color(s_month, GColorClear);
  text_layer_set_text_color(s_month, GColorWhite);
  text_layer_set_font(s_month, s_res_scp);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_month);
    
  // s_battery
  s_battery = text_layer_create(GRect(92, 0, 48, 12));
  text_layer_set_background_color(s_battery, GColorClear);
  text_layer_set_text_color(s_battery, GColorWhite);
  text_layer_set_text_alignment(s_battery, GTextAlignmentRight);
  text_layer_set_font(s_battery, s_res_scp);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_battery);
  
  // setting clocks
  tick_timer_service_subscribe(MINUTE_UNIT, time_handler);
  battery_state_service_subscribe(battery_handler);
  update_all(); // get correct time at init
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(s_time_hr);
  text_layer_destroy(s_time_hr2);
  text_layer_destroy(s_time_min);
  text_layer_destroy(s_time_min2);
  text_layer_destroy(s_ampm);
  text_layer_destroy(s_date_year);
  text_layer_destroy(s_date_day);
  text_layer_destroy(s_date_month);
  text_layer_destroy(s_day);
  text_layer_destroy(s_month);
  text_layer_destroy(s_battery);
  fonts_unload_custom_font(s_res_hoog_numbers);
  fonts_unload_custom_font(s_res_scp);
}
// END AUTO-GENERATED UI CODE

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_main_face(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
}

void hide_main_face(void) {
  window_stack_remove(s_window, true);
}
