#include <pebble.h>
#include "face_handler.h"
#include "app_message.h"

//-- static globals --
static Window *s_window;
static GFont s_res_sqr_num;
static GFont s_res_sqr_num_32;
static GFont s_res_scp;
static GFont s_res_scp_20;
static GBitmap *s_res_w01d; // day sunny
static GBitmap *s_res_w01n; // night sunny
static GBitmap *s_res_w02d; // day few clouds
static GBitmap *s_res_w02n; // night few clouds
static GBitmap *s_res_w03d; // day scattered clouds
static GBitmap *s_res_w04d; // day broken clouds
static GBitmap *s_res_w09d; // day shower rain
static GBitmap *s_res_w10d; // day rain
static GBitmap *s_res_w10n; // night rain
static GBitmap *s_res_w11d; // day thunderstorm
static GBitmap *s_res_w13d; // day snow
static GBitmap *s_res_w50d; // day mist
static GBitmap *s_res_bluetooth;
static GBitmap *s_res_bluetoothOff;
static TextLayer *s_time_hr;
static TextLayer *s_time_hr2;
static TextLayer *s_time_min;
static TextLayer *s_time_min2;
static TextLayer *s_weather_temp;
static TextLayer *s_date_year;
static TextLayer *s_date_year2;
static TextLayer *s_date_day;
static TextLayer *s_date_month;
//static TextLayer *s_day_h;
//static TextLayer *s_month_h;
static TextLayer *s_day_v;
static TextLayer *s_date_day_v;
static TextLayer *s_month_v;
static TextLayer *s_year_v;
static TextLayer *s_ampm;
static TextLayer *s_battery;
static BitmapLayer *s_weather_image;
static BitmapLayer *s_bluetooth_image;

#define SPACE  32 // ASCII ' '
#define FORCE_WEATHER_UPDATE 30

  
#define BLU    GColorFromHEX(0x0300B9)
#define GREEN  GColorFromHEX(0x006A66)
#define ORANGE GColorFromHEX(0xFF6326)
#define RED    GColorFromHEX(0xFE3F4E)
struct TCColors
{
  GColor8 font_color_pri;
  GColor8 font_color_sec;
  GColor8 font_color_ter;
};
static struct TCColors* s_colors;

//-- functions --

static void update_daily(void)
{
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  static char date_year[]  = "20";
  static char date_year2[] = "15";
  static char date_day[]   = "01";
  static char date_month[] = "01";
  //static char day_h[]      = "FRI";
  //static char month_h[]    = "JAN";
  static char day_v[]      = "F\nR\nI";
  static char date_day_v[] = "0\n1";
  static char month_v[]    = "J\nA\nN";
  static char year_v[]     = "2\n0\n1\n5";
  
  strftime(date_day,   sizeof(date_day),   "%d", tick_time);
  date_day_v[0] = date_day[0]; date_day_v[2] = date_day[1];
  strftime(date_month, sizeof(date_month), "%m", tick_time);
  
  {
    char buffer[5];
    
    strftime(buffer,  sizeof(buffer),  "%G", tick_time);
    date_year[0]  = year_v[0] = buffer[0];
    date_year[1]  = year_v[2] = buffer[1];
    date_year2[0] = year_v[4] = buffer[2];
    date_year2[1] = year_v[6] = buffer[3];
    
    strftime(buffer, sizeof(buffer), "%b", tick_time);
    month_v[0] = buffer[0]; // month_h[0]
    month_v[2] = buffer[1] - SPACE; // month_h[1]
    month_v[4] = buffer[2] - SPACE; // month_h[2]
    
    strftime(buffer, sizeof(buffer), "%a", tick_time);
    day_v[0] = buffer[0]; // day_h[0]
    day_v[2] = buffer[1] - SPACE; // day_h[1]
    day_v[4] = buffer[2] - SPACE; // day_h[2]
  }
  
  text_layer_set_text(s_date_year, date_year);
  text_layer_set_text(s_date_year2, date_year2);
  text_layer_set_text(s_date_day, date_day);
  text_layer_set_text(s_date_month, date_month);
//  text_layer_set_text(s_day_h, day_h);
//  text_layer_set_text(s_month_h, month_h);
  text_layer_set_text(s_day_v, day_v);
  text_layer_set_text(s_date_day_v, date_day_v);
  text_layer_set_text(s_month_v, month_v);
  text_layer_set_text(s_year_v, year_v);
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
  
  strftime(ampm, sizeof(ampm), "%p", tick_time);
  
  text_layer_set_text(s_time_hr, time_hr);
  text_layer_set_text(s_time_hr2, time_hr2);
  text_layer_set_text(s_time_min, time_min);
  text_layer_set_text(s_time_min2, time_min2);
  text_layer_set_text(s_ampm, ampm);
}

static void bluetooth_handler(bool connected)
{
  if(connected)
  {
    APP_LOG(APP_LOG_LEVEL_INFO, "bluetooth connected");
    bitmap_layer_set_bitmap(s_bluetooth_image, s_res_bluetooth);
  }
  else
    bitmap_layer_set_bitmap(s_bluetooth_image, s_res_bluetoothOff);
}

static void battery_handler(BatteryChargeState charge_state)
{
  static char battery[] = " 100%";
  text_layer_set_text_color(s_battery, s_colors->font_color_ter);
  if(charge_state.charge_percent == 100) // full charge
  {
     battery[1] = '1';
     battery[2] = '0';
  }
  else
  {
    battery[1] = ' ';
    battery[2] = '0' + charge_state.charge_percent / 10;
  }
  
  if(charge_state.charge_percent <= 30)
    text_layer_set_text_color(s_battery, RED);
  
  if(charge_state.is_plugged)
  {
    text_layer_set_text_color(s_battery, ORANGE);
    battery[0] = 'C';
    // try to fix weird pebble battery reporting glitch
    if(charge_state.charge_percent >= 70)
    {
      battery[1] = '>';
      battery[2] = '7';
      text_layer_set_text_color(s_battery, GREEN);
    }
  }
  else // not plugged in (or charging)
    battery[0] = ' ';
  
  
  text_layer_set_text(s_battery, battery);
}

void icon_handler(void)
{
  switch(icon[0])
    {
    case '0':
      switch(icon[1])
      {
        case '1': // 01x
          APP_LOG(APP_LOG_LEVEL_INFO, "01x");
          if(icon[2] == 'd')
            bitmap_layer_set_bitmap(s_weather_image, s_res_w01d);
          else // night version
            bitmap_layer_set_bitmap(s_weather_image, s_res_w01n);
          break;
        case '2': // 02x
          APP_LOG(APP_LOG_LEVEL_INFO, "02x");
          if(icon[2] == 'd')
            bitmap_layer_set_bitmap(s_weather_image, s_res_w02d);
          else // night version
            bitmap_layer_set_bitmap(s_weather_image, s_res_w02n);
          break;
        case '3': // 03x
          APP_LOG(APP_LOG_LEVEL_INFO, "03x");
          bitmap_layer_set_bitmap(s_weather_image, s_res_w03d);
          break;
        case '4': // 04x
          APP_LOG(APP_LOG_LEVEL_INFO, "04x");
          bitmap_layer_set_bitmap(s_weather_image, s_res_w04d);
          break;
        case '9': // 09x
          APP_LOG(APP_LOG_LEVEL_INFO, "09x");
          bitmap_layer_set_bitmap(s_weather_image, s_res_w09d);
          break;
        default:
          break;
      }
      break;
    case '1':
      switch (icon[1])
      {
        case '0': // 10x
          APP_LOG(APP_LOG_LEVEL_INFO, "10x");
          if(icon[2] == 'd')
            bitmap_layer_set_bitmap(s_weather_image, s_res_w10d);
          else // night version
            bitmap_layer_set_bitmap(s_weather_image, s_res_w10n);
          break;
        case '1': // 11x
          APP_LOG(APP_LOG_LEVEL_INFO, "11x");
          bitmap_layer_set_bitmap(s_weather_image, s_res_w11d);
          break;
        case '3': // 13x
          APP_LOG(APP_LOG_LEVEL_INFO, "13x");
          bitmap_layer_set_bitmap(s_weather_image, s_res_w13d);
          break;
        default:
          break;
      }
      break;
    case '5':
      APP_LOG(APP_LOG_LEVEL_INFO, "50x");
      bitmap_layer_set_bitmap(s_weather_image, s_res_w50d);
      break;
    default:
      break;
    }
}

static void update_weather(int tm_min)
{
  if(tm_min % 30 == 0)
    pull_weather();
}

static void update_all(void)
{
  update_minute();
  update_daily();
  update_weather(FORCE_WEATHER_UPDATE);
  battery_handler(battery_state_service_peek()); // force refresh
  bluetooth_handler(bluetooth_connection_service_peek());
}

static void time_handler(struct tm *tick_time, TimeUnits units_changed)
{
  static int current_day = -1;
  
  update_minute();
  update_weather(tick_time->tm_min);
  // saves computation of day, month, year, from being calculated every minute
  if(current_day != tick_time->tm_yday)
  {
    current_day = tick_time->tm_yday;
    update_daily();
  }
}

static void initialise_ui(void)
{
  GFont bigNumbers, ltlNumbers, regText, bigText;
  
  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, true);
  #endif
  
  //-- colors --
  s_colors = (struct TCColors*)malloc(sizeof(struct TCColors));
  s_colors->font_color_pri = BLU;
  s_colors->font_color_sec = GColorBlack;
  s_colors->font_color_ter = GColorBlack;
  
  
  //-- load font resources --
  s_res_sqr_num = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SQUARENUM_64));
  s_res_sqr_num_32 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SQUARENUM_32));
  s_res_scp = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SCP_12));
  s_res_scp_20 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SCP_20));
  
  bigNumbers = s_res_sqr_num;
  ltlNumbers = s_res_sqr_num_32;
  regText = s_res_scp;
  bigText = s_res_scp_20;
  //-- load image resources --
  s_res_w01d = gbitmap_create_with_resource(RESOURCE_ID_w01d);
  s_res_w01n = gbitmap_create_with_resource(RESOURCE_ID_w01n);
  s_res_w02d = gbitmap_create_with_resource(RESOURCE_ID_w02d);
  s_res_w02n = gbitmap_create_with_resource(RESOURCE_ID_w02n);
  s_res_w03d = gbitmap_create_with_resource(RESOURCE_ID_w03d);
  s_res_w04d = gbitmap_create_with_resource(RESOURCE_ID_w04d);
  s_res_w09d = gbitmap_create_with_resource(RESOURCE_ID_w09d);
  s_res_w10d = gbitmap_create_with_resource(RESOURCE_ID_w10d);
  s_res_w10n = gbitmap_create_with_resource(RESOURCE_ID_w10n);
  s_res_w11d = gbitmap_create_with_resource(RESOURCE_ID_w11d);
  s_res_w13d = gbitmap_create_with_resource(RESOURCE_ID_w13d);
  s_res_w50d = gbitmap_create_with_resource(RESOURCE_ID_w50d);
  s_res_bluetooth = gbitmap_create_with_resource(RESOURCE_ID_bluetooth_icon);
  s_res_bluetoothOff = gbitmap_create_with_resource(RESOURCE_ID_bluetooth_off);
  
  //-- text layers --
  // s_time_hr
  s_time_hr = text_layer_create(GRect(40, -10, 50, 90));
  text_layer_set_background_color(s_time_hr, GColorClear);
  text_layer_set_text_color(s_time_hr, s_colors->font_color_pri);
  text_layer_set_text_alignment(s_time_hr, GTextAlignmentRight);
  text_layer_set_font(s_time_hr, bigNumbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_time_hr);
  
  // s_time_hr2
  s_time_hr2 = text_layer_create(GRect(40, -10, 100, 90));
  text_layer_set_background_color(s_time_hr2, GColorClear);
  text_layer_set_text_color(s_time_hr2, s_colors->font_color_pri);
  text_layer_set_text_alignment(s_time_hr2, GTextAlignmentRight);
  text_layer_set_font(s_time_hr2, bigNumbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_time_hr2);
  
  // s_time_min
  s_time_min = text_layer_create(GRect(40, 43, 50, 90));
  text_layer_set_background_color(s_time_min, GColorClear);
  text_layer_set_text_color(s_time_min, s_colors->font_color_pri);
  text_layer_set_text_alignment(s_time_min, GTextAlignmentRight);
  text_layer_set_font(s_time_min, bigNumbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_time_min);
  
  // s_time_min2
  s_time_min2 = text_layer_create(GRect(40, 43, 100, 90));
  text_layer_set_background_color(s_time_min2, GColorClear);
  text_layer_set_text_color(s_time_min2, s_colors->font_color_pri);
  text_layer_set_text_alignment(s_time_min2, GTextAlignmentRight);
  text_layer_set_font(s_time_min2, bigNumbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_time_min2);
  
  // s_weather_temp
  s_weather_temp = text_layer_create(GRect(-2, 30, 46, 27));
  text_layer_set_background_color(s_weather_temp, GColorClear);
  text_layer_set_text_color(s_weather_temp, s_colors->font_color_pri);
  text_layer_set_text_alignment(s_weather_temp, GTextAlignmentCenter);
  text_layer_set_font(s_weather_temp, bigText);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_weather_temp);
  
  // s_date_year = first part of a year (20)
  s_date_year = text_layer_create(GRect(42, 130, 46, 37));
  text_layer_set_background_color(s_date_year, GColorClear);
  text_layer_set_text_color(s_date_year, s_colors->font_color_sec);
  text_layer_set_text_alignment(s_date_year, GTextAlignmentRight);
  text_layer_set_font(s_date_year, ltlNumbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_date_year);
  
  // s_date_year2 = last part of a year (15)
  s_date_year2 = text_layer_create(GRect(93, 130, 46, 37));
  text_layer_set_background_color(s_date_year2, GColorClear);
  text_layer_set_text_color(s_date_year2, s_colors->font_color_sec);
  text_layer_set_text_alignment(s_date_year2, GTextAlignmentRight);
  text_layer_set_font(s_date_year2, ltlNumbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_date_year2);
  
  // s_date_day = 01, 02, 03, ..., 31
  s_date_day = text_layer_create(GRect(93, 105, 46, 37));
  text_layer_set_background_color(s_date_day, GColorClear);
  text_layer_set_text_color(s_date_day, s_colors->font_color_sec);
  text_layer_set_text_alignment(s_date_day, GTextAlignmentRight);
  text_layer_set_font(s_date_day, ltlNumbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_date_day);
  
  // s_date_month = 01, 02, 03, ..., 12
  s_date_month = text_layer_create(GRect(42, 105, 46, 37));
  text_layer_set_background_color(s_date_month, GColorClear);
  text_layer_set_text_color(s_date_month, s_colors->font_color_sec);
  text_layer_set_text_alignment(s_date_month, GTextAlignmentRight);
  text_layer_set_font(s_date_month, ltlNumbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_date_month);
  
  /* looks ugly
  // s_day_h = horizontal day listing
  s_day_h = text_layer_create(GRect(0, 112, 46, 37));
  text_layer_set_background_color(s_day_h, GColorClear);
  text_layer_set_text_color(s_day_h, s_colors->font_color_pri);
  text_layer_set_text_alignment(s_day_h, GTextAlignmentRight);
  text_layer_set_font(s_day_h, bigText);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_day_h);
  
  // s_month_h = horizontal month listing
  s_month_h = text_layer_create(GRect(0, 135, 46, 37));
  text_layer_set_background_color(s_month_h, GColorClear);
  text_layer_set_text_color(s_month_h, s_colors->font_color_pri);
  text_layer_set_text_alignment(s_month_h, GTextAlignmentRight);
  text_layer_set_font(s_month_h, bigText);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_month_h);
  */
  
  // s_ampm
  s_ampm = text_layer_create(GRect(50, -4, 14, 13));
  text_layer_set_background_color(s_ampm, GColorClear);
  text_layer_set_text_color(s_ampm, s_colors->font_color_ter);
  text_layer_set_font(s_ampm, regText);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_ampm);
  
  // s_day_v = vertical day listing
  s_day_v = text_layer_create(GRect(40, 7, 8, 36));
  text_layer_set_background_color(s_day_v, GColorClear);
  text_layer_set_text_color(s_day_v, s_colors->font_color_ter);
  text_layer_set_text_alignment(s_day_v, GTextAlignmentCenter);
  text_layer_set_font(s_day_v, regText);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_day_v);
  
  // s_date_day_v = same as s_date_day, but vertical, below s_day
  s_date_day_v = text_layer_create(GRect(40, 48, 8, 24));
  text_layer_set_background_color(s_date_day_v, GColorClear);
  text_layer_set_text_color(s_date_day_v, s_colors->font_color_ter);
  text_layer_set_text_alignment(s_date_day_v, GTextAlignmentCenter);
  text_layer_set_font(s_date_day_v, regText);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_date_day_v);
  
  // s_month_v = vertical month listing
  s_month_v = text_layer_create(GRect(40, 75, 8, 36));
  text_layer_set_background_color(s_month_v, GColorClear);
  text_layer_set_text_color(s_month_v, s_colors->font_color_ter);
  text_layer_set_text_alignment(s_month_v, GTextAlignmentCenter);
  text_layer_set_font(s_month_v, regText);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_month_v);
  
  // s_year_v = vertical year listing
  s_year_v = text_layer_create(GRect(40, 115, 8, 48));
  text_layer_set_background_color(s_year_v, GColorClear);
  text_layer_set_text_color(s_year_v, s_colors->font_color_ter);
  text_layer_set_text_alignment(s_year_v, GTextAlignmentCenter);
  text_layer_set_font(s_year_v, regText);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_year_v);
  
  // s_battery
  s_battery = text_layer_create(GRect(101, -4, 36, 13));
  text_layer_set_background_color(s_battery, GColorClear);
  text_layer_set_text_color(s_battery, s_colors->font_color_ter);
  text_layer_set_font(s_battery, regText);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_battery);
  
  //-- bitmaps --
  
  // s_weather_image
  s_weather_image = bitmap_layer_create(GRect(0, -3, 40, 40));
  bitmap_layer_set_compositing_mode(s_weather_image, GCompOpSet);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_weather_image);
  
  // s_bluetooth_image
  s_bluetooth_image = bitmap_layer_create(GRect(83, -3, 20, 20));
  bitmap_layer_set_compositing_mode(s_bluetooth_image, GCompOpSet);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_bluetooth_image);
  
  //-- subscribing to events --
  tick_timer_service_subscribe(MINUTE_UNIT, time_handler);
  battery_state_service_subscribe(battery_handler);
  message_init(s_weather_temp); // start weather service
  bluetooth_connection_service_subscribe(bluetooth_handler);
  //-- setting face --
  update_all();
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(s_time_hr);
  text_layer_destroy(s_time_hr2);
  text_layer_destroy(s_time_min);
  text_layer_destroy(s_time_min2);
  text_layer_destroy(s_weather_temp);
  text_layer_destroy(s_date_year);
  text_layer_destroy(s_date_year2);
  text_layer_destroy(s_date_day);
  text_layer_destroy(s_date_month);
  //text_layer_destroy(s_day_h);
  //text_layer_destroy(s_month_h);
  text_layer_destroy(s_day_v);
  text_layer_destroy(s_date_day_v);
  text_layer_destroy(s_month_v);
  text_layer_destroy(s_year_v);
  text_layer_destroy(s_ampm);
  text_layer_destroy(s_battery);
  fonts_unload_custom_font(s_res_sqr_num);
  fonts_unload_custom_font(s_res_sqr_num_32);
  fonts_unload_custom_font(s_res_scp);
  fonts_unload_custom_font(s_res_scp_20);
  bitmap_layer_destroy(s_weather_image);
  bitmap_layer_destroy(s_bluetooth_image);
  gbitmap_destroy(s_res_w01d);
  gbitmap_destroy(s_res_w01n);
  gbitmap_destroy(s_res_w02d);
  gbitmap_destroy(s_res_w02n);
  gbitmap_destroy(s_res_w03d);
  gbitmap_destroy(s_res_w04d);
  gbitmap_destroy(s_res_w09d);
  gbitmap_destroy(s_res_w10d);
  gbitmap_destroy(s_res_w10n);
  gbitmap_destroy(s_res_w11d);
  gbitmap_destroy(s_res_w13d);
  gbitmap_destroy(s_res_w50d);
  gbitmap_destroy(s_res_bluetooth);
  gbitmap_destroy(s_res_bluetoothOff);
  
  free(s_colors);
  message_deinit(); // stop weather services
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_main_face(void)
{
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
}

void hide_main_face(void) {
  window_stack_remove(s_window, true);
}
