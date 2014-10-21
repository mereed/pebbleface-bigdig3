#include "pebble.h"

static Window *window;
static Layer *window_layer;

GColor background_color = GColorBlack;

static AppSync sync;
static uint8_t sync_buffer[100];

static bool appStarted = false;

static int invert;
static int hidesec;
static int hide_batt;
static int hide_date;
static int hide_weather;
static int bluetoothvibe;
static int hourlyvibe;

enum WeatherKey {
  WEATHER_TEMPERATURE_KEY = 0x1,
  INVERT_COLOR_KEY = 0x2,
  HIDE_BATT_KEY = 0x3,
  HIDE_DATE_KEY = 0x4,
  HIDE_WEATHER_KEY = 0x5,
  HIDE_SEC_KEY = 0x6,
  HOURLYVIBE_KEY = 0x7,
  BLUETOOTHVIBE_KEY = 0x8
};

TextLayer *temp_layer;

static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;

static GFont *digds;

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

#define TOTAL_TIME_DIGITS 2
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

#define TOTAL_MTIME_DIGITS 2
static GBitmap *mtime_digits_images[TOTAL_MTIME_DIGITS];
static BitmapLayer *mtime_digits_layers[TOTAL_MTIME_DIGITS];

const int MIN_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_M0,
  RESOURCE_ID_IMAGE_NUM_M1,
  RESOURCE_ID_IMAGE_NUM_M2,
  RESOURCE_ID_IMAGE_NUM_M3,
  RESOURCE_ID_IMAGE_NUM_M4,
  RESOURCE_ID_IMAGE_NUM_M5,
  RESOURCE_ID_IMAGE_NUM_M6,
  RESOURCE_ID_IMAGE_NUM_M7,
  RESOURCE_ID_IMAGE_NUM_M8,
  RESOURCE_ID_IMAGE_NUM_M9
};

#define TOTAL_DATE_DIGITS 2	
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

const int DATE_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_DATE0,
  RESOURCE_ID_IMAGE_NUM_DATE1,
  RESOURCE_ID_IMAGE_NUM_DATE2,
  RESOURCE_ID_IMAGE_NUM_DATE3,
  RESOURCE_ID_IMAGE_NUM_DATE4,
  RESOURCE_ID_IMAGE_NUM_DATE5,
  RESOURCE_ID_IMAGE_NUM_DATE6,
  RESOURCE_ID_IMAGE_NUM_DATE7,
  RESOURCE_ID_IMAGE_NUM_DATE8,
  RESOURCE_ID_IMAGE_NUM_DATE9
};

#define TOTAL_SECONDS_DIGITS 2
static GBitmap *seconds_digits_images[TOTAL_SECONDS_DIGITS];
static BitmapLayer *seconds_digits_layers[TOTAL_SECONDS_DIGITS];

const int SEC_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_S0,
  RESOURCE_ID_IMAGE_NUM_S1,
  RESOURCE_ID_IMAGE_NUM_S2,
  RESOURCE_ID_IMAGE_NUM_S3,
  RESOURCE_ID_IMAGE_NUM_S4,
  RESOURCE_ID_IMAGE_NUM_S5,
  RESOURCE_ID_IMAGE_NUM_S6,
  RESOURCE_ID_IMAGE_NUM_S7,
  RESOURCE_ID_IMAGE_NUM_S8,
  RESOURCE_ID_IMAGE_NUM_S9
};

BitmapLayer *layer_batt_img;
GBitmap *digds_batt_100;
GBitmap *digds_batt_90;
GBitmap *digds_batt_80;
GBitmap *digds_batt_70;
GBitmap *digds_batt_60;
GBitmap *digds_batt_50;
GBitmap *digds_batt_40;
GBitmap *digds_batt_30;
GBitmap *digds_batt_20;
GBitmap *digds_batt_10;
GBitmap *digds_batt_charge;
int charge_percent = 0;
InverterLayer *inverter_layer = NULL;


void set_invert_color(bool invert) {
  if (invert && inverter_layer == NULL) {
    // Add inverter layer
    Layer *window_layer = window_get_root_layer(window);

    inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
    layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));
  } else if (!invert && inverter_layer != NULL) {
    // Remove Inverter layer
    layer_remove_from_parent(inverter_layer_get_layer(inverter_layer));
    inverter_layer_destroy(inverter_layer);
    inverter_layer = NULL;
  }
  // No action required
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

void hide_batt_now(bool hide_batt) {
	
	if (hide_batt) {
		layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), true);
		
	} else {
		layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), false);
	}
}

void hide_date_now(bool hide_date) {
	
	if (hide_date) {
		layer_set_hidden(bitmap_layer_get_layer(day_name_layer), true);
		
		for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[i]), true);
			}
	} else {
		layer_set_hidden(bitmap_layer_get_layer(day_name_layer), false);
		
		for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[i]), false);
			}
	}
}

void hide_weather_now(bool hide_weather) {
	
	if (hide_weather) {
		layer_set_hidden(text_layer_get_layer(temp_layer), true);
		
	} else {
		layer_set_hidden(text_layer_get_layer(temp_layer), false);
	}
}

void hide_seconds_now(bool hidesec) {

   if(hidesec) {
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[i]), true);
			}
    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
      }
      else {
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[i]), false);
			}
    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
      }
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
   
	switch (key) {

		
	case WEATHER_TEMPERATURE_KEY:
      text_layer_set_text(temp_layer, new_tuple->value->cstring);
      break;
	case INVERT_COLOR_KEY:
      invert = new_tuple->value->uint8 != 0;
	  persist_write_bool(INVERT_COLOR_KEY, invert);
      set_invert_color(invert);
      break;
	case HIDE_SEC_KEY:
      hidesec = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_SEC_KEY, hidesec);	  
	  hide_seconds_now(hidesec);
      break; 
	case HIDE_BATT_KEY:
	  hide_batt = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_BATT_KEY, hide_batt);
	  hide_batt_now(hide_batt);
	  break;
	case HIDE_DATE_KEY:
      hide_date = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_DATE_KEY, hide_date);	  
	  hide_date_now(hide_date);
	  break;
    case HIDE_WEATHER_KEY:
      hide_weather = new_tuple->value->uint8 !=0;
	  persist_write_bool(HIDE_WEATHER_KEY, hide_weather);	  
	  hide_weather_now(hide_weather);
	  break;
    case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
      break;      
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
      break; 
  }
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;
  *bmp_image = gbitmap_create_with_resource(resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = (*bmp_image)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
  if (old_image != NULL) {
	gbitmap_destroy(old_image);
	old_image = NULL;
  }
}

void update_battery(BatteryChargeState charge_state) {

    if (charge_state.is_charging) {
        bitmap_layer_set_bitmap(layer_batt_img, digds_batt_charge);

    } else {
        if (charge_state.charge_percent <= 10) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_10);
        } else if (charge_state.charge_percent <= 20) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_20);
        } else if (charge_state.charge_percent <= 30) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_30);
		} else if (charge_state.charge_percent <= 40) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_40);
		} else if (charge_state.charge_percent <= 50) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_50);
    	} else if (charge_state.charge_percent <= 60) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_60);	
        } else if (charge_state.charge_percent <= 70) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_70);
		} else if (charge_state.charge_percent <= 80) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_80);
		} else if (charge_state.charge_percent <= 90) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_90);
		} else if (charge_state.charge_percent <= 100) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_100);					
    }
    charge_percent = charge_state.charge_percent;
    }
}

static void toggle_bluetooth_icon(bool connected) {

    if (connected) {
	  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), true);
    } else {
  	  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), false);
    }

    if (appStarted && bluetoothvibe) {
      
        vibes_long_pulse();
	}
}

void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth_icon(connected);
}

void force_update(void) {
    update_battery(battery_state_service_peek());
    toggle_bluetooth_icon(bluetooth_connection_service_peek());
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_days(struct tm *tick_time) {
  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint(2, 147));
  set_container_image(&date_digits_images[0], date_digits_layers[0], DATE_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(47, 147));
  set_container_image(&date_digits_images[1], date_digits_layers[1], DATE_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(60, 147));
}

static void update_hours(struct tm *tick_time) {

  if(appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
  }
  
  unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(0, 2));
  set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(66, 2));

  if (!clock_is_24h_style()) {
    
    if (display_hour/10 == 0) {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
    }
    else {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
    }
  }
}

static void update_minutes(struct tm *tick_time) {
  set_container_image(&mtime_digits_images[0], mtime_digits_layers[0], MIN_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(85, 110));
  set_container_image(&mtime_digits_images[1], mtime_digits_layers[1], MIN_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(108, 110));
}

static void update_seconds(struct tm *tick_time) {
  set_container_image(&seconds_digits_images[0], seconds_digits_layers[0], SEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec/10], GPoint(105, 147));
  set_container_image(&seconds_digits_images[1], seconds_digits_layers[1], SEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec%10], GPoint(118, 147));
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & DAY_UNIT) {
    update_days(tick_time);
  }
  if (units_changed & HOUR_UNIT) {
    update_hours(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
   update_minutes(tick_time);
  }	
  if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }		
}

static void init(void) {
  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));
  memset(&mtime_digits_layers, 0, sizeof(mtime_digits_layers));
  memset(&mtime_digits_images, 0, sizeof(mtime_digits_images));
  memset(&seconds_digits_layers, 0, sizeof(seconds_digits_layers));
  memset(&seconds_digits_images, 0, sizeof(seconds_digits_images));
  memset(&date_digits_layers, 0, sizeof(date_digits_layers));
  memset(&date_digits_images, 0, sizeof(date_digits_images));

  const int inbound_size = 100;
  const int outbound_size = 100;
  app_message_open(inbound_size, outbound_size);  

  window = window_create();
  if (window == NULL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }
  window_stack_push(window, true /* Animated */);
  window_layer = window_get_root_layer(window);
  
  background_color  = GColorBlack;
  window_set_background_color(window, background_color);
	
 // resources
	
    digds_batt_100   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_090_100);
    digds_batt_90   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_080_090);
    digds_batt_80   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_070_080);
    digds_batt_70   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_060_070);
    digds_batt_60   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_050_060);
    digds_batt_50   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_040_050);
    digds_batt_40   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_030_040);
    digds_batt_30    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_020_030);
    digds_batt_20    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_010_020);
    digds_batt_10    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_000_010);
    digds_batt_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_CHARGING);

    digds = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITALDS_29));
	
  bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  GRect frame = (GRect) {
    .origin = { .x = 79, .y = 147 },
    .size = bluetooth_image->bounds.size
  };
  bluetooth_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));
  
  // Create time and date layers
  GRect dummy_frame = { {0, 0}, {0, 0} };
  day_name_layer = bitmap_layer_create(dummy_frame);
  layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));
  
  for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }
	for (int i = 0; i < TOTAL_MTIME_DIGITS; ++i) {
    mtime_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(mtime_digits_layers[i]));
  }
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    seconds_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(seconds_digits_layers[i]));
  }
   for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
  }
	
  layer_batt_img  = bitmap_layer_create(GRect(131, 0, 12, 168));
  bitmap_layer_set_bitmap(layer_batt_img, digds_batt_100);
  layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));	
	
  Layer *weather_holder = layer_create(GRect(0, 0, 144, 168 ));
  layer_add_child(window_layer, weather_holder);
	
  temp_layer = text_layer_create(GRect(1, 115, 60, 40));
  text_layer_set_text_color(temp_layer, GColorWhite);
  text_layer_set_background_color(temp_layer, GColorClear);
  text_layer_set_font(temp_layer, (digds));	 
  text_layer_set_text_alignment(temp_layer, GTextAlignmentLeft);
  layer_add_child(weather_holder, text_layer_get_layer(temp_layer));

  Tuplet initial_values[] = {
	TupletCString(WEATHER_TEMPERATURE_KEY, ""),
    TupletInteger(INVERT_COLOR_KEY, persist_read_bool(INVERT_COLOR_KEY)),
    TupletInteger(HIDE_BATT_KEY, persist_read_bool(HIDE_BATT_KEY)),
	TupletInteger(HIDE_DATE_KEY, persist_read_bool(HIDE_DATE_KEY)),
	TupletInteger(HIDE_WEATHER_KEY, persist_read_bool(HIDE_WEATHER_KEY)),
    TupletInteger(HIDE_SEC_KEY, persist_read_bool(HIDE_SEC_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
  };
  
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, NULL, NULL);
   
  appStarted = true;
  
  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);
  battery_state_service_subscribe(&update_battery);

  // draw first frame
  force_update();

}

static void deinit(void) {

  app_sync_deinit(&sync);
  
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();

  fonts_unload_custom_font(digds);

  layer_remove_from_parent(text_layer_get_layer(temp_layer));
  text_layer_destroy(temp_layer);
	
  layer_remove_from_parent(bitmap_layer_get_layer(bluetooth_layer));
  bitmap_layer_destroy(bluetooth_layer);
  gbitmap_destroy(bluetooth_image);
  bluetooth_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_batt_img));
  bitmap_layer_destroy(layer_batt_img);	
  gbitmap_destroy(digds_batt_100);
  gbitmap_destroy(digds_batt_90);
  gbitmap_destroy(digds_batt_80);
  gbitmap_destroy(digds_batt_70);
  gbitmap_destroy(digds_batt_60);
  gbitmap_destroy(digds_batt_50);
  gbitmap_destroy(digds_batt_40);
  gbitmap_destroy(digds_batt_30);
  gbitmap_destroy(digds_batt_20);
  gbitmap_destroy(digds_batt_10);
  gbitmap_destroy(digds_batt_charge);		
	
  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
  day_name_image = NULL;
	
	for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    time_digits_images[i] = NULL;
    bitmap_layer_destroy(time_digits_layers[i]);
	time_digits_layers[i] = NULL; 
    }

	for (int i = 0; i < TOTAL_MTIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(mtime_digits_layers[i]));
    gbitmap_destroy(mtime_digits_images[i]);
    mtime_digits_images[i] = NULL;
    bitmap_layer_destroy(mtime_digits_layers[i]);
	mtime_digits_layers[i] = NULL;
    }

	for (int i = 0; i < TOTAL_SECONDS_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(seconds_digits_layers[i]));
    gbitmap_destroy(seconds_digits_images[i]);
    seconds_digits_images[i] = NULL;
    bitmap_layer_destroy(seconds_digits_layers[i]);
	seconds_digits_layers[i] = NULL;
    }

	for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
	layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    gbitmap_destroy(date_digits_images[i]);
    date_digits_layers[i] = NULL;
    bitmap_layer_destroy(date_digits_layers[i]);
	date_digits_layers[i] = NULL;
	}
	
  layer_remove_from_parent(window_layer);
  layer_destroy(window_layer);
	
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}