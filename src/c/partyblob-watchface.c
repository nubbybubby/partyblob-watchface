#include <pebble.h>

#define RGB_STEP 17
#define SPEED_MIN 0.25
#define SPEED_MAX 2.50
#define SETTINGS_KEY 1

static Window *s_window;
static TextLayer *s_text_layer;
static TextLayer *text_bottom_layer;

static GFont lucky_font;
static GFont lucky_font_small;

static int16_t rgb[3];
static uint8_t curr_chan;

static GBitmapSequence *s_sequence;
static GBitmap *s_firstframe_bitmap;
static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;

static AppTimer *anim_timer;
static AppTimer *rgb_timer;

typedef struct blob_settings {
  bool leading_zero_disabled;
  float blob_speed;
  float color_speed;
  char bottom_text[15];
} blob_settings;

static blob_settings settings;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char bt_buff[20];
  strftime(bt_buff, sizeof(bt_buff), settings.bottom_text, tick_time);

  static char time_buff[8];
  strftime(time_buff, sizeof(time_buff), clock_is_24h_style() ?
                                         "%H:%M" : "%I:%M", tick_time);
  char *time_buff_ptr = time_buff;
  if (settings.leading_zero_disabled == true) {
    if (time_buff[0] == '0') {
      time_buff_ptr++;
    }
  }
  
  text_layer_set_text(text_bottom_layer, bt_buff);
  text_layer_set_text(s_text_layer, time_buff_ptr);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) { update_time(); }

static void reset_rgb(void) {
  memset(rgb, 0, sizeof(rgb));
  curr_chan = 0;
  rgb[0] = 255;
  rgb[2] = 127;
}

uint8_t u16_to_u8(uint16_t val) {
  return (val > 255) ? 255 : (uint8_t)val;
}

static void rgb_handler(void *context) {
  if (!light_is_on()) {
    rgb_timer = NULL;
    return;
  }

  if (curr_chan > 2) {
    curr_chan = 0;
  }

  for (size_t i = 0; i < 3; i++) {
    if (i == curr_chan) {
      if (rgb[i] > 255) {
        rgb[i] = 255;
        curr_chan++;
      } else {
        if (curr_chan == 0) {
          rgb[2] += (RGB_STEP / 2) * settings.color_speed;
        }
        rgb[i] += RGB_STEP * settings.color_speed;
      }
    } else {
      if (rgb[i] > 0) {
        rgb[i] -= RGB_STEP * settings.color_speed;
      }
      if (rgb[i] < 0) {
        rgb[i] = 0;
      }
    }
  }

  uint32_t combined = (u16_to_u8(rgb[0]) << 16) | (u16_to_u8(rgb[1]) << 8) | u16_to_u8(rgb[2]);
  light_set_color_rgb888(combined);
  rgb_timer = app_timer_register(8.3, rgb_handler, NULL);
}

static void timer_handler(void *context) {
  uint32_t next_delay;

  if (gbitmap_sequence_get_current_frame_idx(s_sequence) == 10 && !light_is_on()) {
    anim_timer = NULL;
    gbitmap_sequence_restart(s_sequence);
    return;
  }

  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay)) {
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
    anim_timer = app_timer_register(next_delay / settings.blob_speed, timer_handler, NULL);
  }
}

static void start_timers(void) {
  if (settings.blob_speed < SPEED_MIN || settings.blob_speed > SPEED_MAX) {
    settings.blob_speed = 1.0;
  }
  
  if (settings.color_speed < SPEED_MIN || settings.color_speed > SPEED_MAX) {
    settings.color_speed = 1.0;
  }

  if (!light_is_on()) return;

  if (!anim_timer) {
    timer_handler(NULL);
  }
  if (!rgb_timer) {
    reset_rgb();
    rgb_handler(NULL);
  }
}

static void backlight_handler(bool is_backlight_on) {
  if (is_backlight_on) {
    start_timers();
  } 
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void load_settings(void) {
  if (persist_exists(SETTINGS_KEY)) {
    persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
  } else {
    /* set defaults */
    settings.leading_zero_disabled = false;
    settings.blob_speed = 1.0;
    settings.color_speed = 1.0;
    snprintf(settings.bottom_text, sizeof(settings.bottom_text), "%s\n", "%a, %b %d");
    prv_save_settings();
  }
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *bottom_text_t = dict_find(iter, MESSAGE_KEY_bottom_text);
  if (bottom_text_t) {
    strncpy(settings.bottom_text, bottom_text_t->value->cstring, sizeof(settings.bottom_text));
  }
 
  Tuple *blob_speed_t = dict_find(iter, MESSAGE_KEY_blob_speed);
  if (blob_speed_t) {
    float new_blob_speed = (float)blob_speed_t->value->int32 / 100;
    if (new_blob_speed >= SPEED_MIN && new_blob_speed <= SPEED_MAX) {
      settings.blob_speed = new_blob_speed;
    }
  }

  Tuple *color_speed_t = dict_find(iter, MESSAGE_KEY_color_speed);
  if (color_speed_t) {
    float new_color_speed = (float)color_speed_t->value->int32 / 100;
    if (new_color_speed >= SPEED_MIN && new_color_speed <= SPEED_MAX) {
      settings.color_speed = new_color_speed;
    }
  }

  Tuple *leading_zero_t = dict_find(iter, MESSAGE_KEY_leading_zero);
  if (leading_zero_t) {
    settings.leading_zero_disabled = leading_zero_t->value->int8; 
  }

  prv_save_settings();
  update_time();
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  window_set_background_color(s_window, GColorBlack);

  lucky_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LUCKIEST_GUY_50));
  lucky_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LUCKIEST_GUY_28));

  s_text_layer = text_layer_create(GRect(-50, 5, bounds.size.w * 1.5, 50));
  text_layer_set_text(s_text_layer, "16:20");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_text_layer, GColorWhite);
  text_layer_set_background_color(s_text_layer, GColorClear);
    
  text_bottom_layer = text_layer_create(GRect(-50, 178, bounds.size.w * 1.5, 50));
  text_layer_set_text(text_bottom_layer, "Tue, Jun 9");
  text_layer_set_text_alignment(text_bottom_layer, GTextAlignmentCenter);
  text_layer_set_text_color(text_bottom_layer, GColorWhite);
  text_layer_set_background_color(text_bottom_layer, GColorClear);

  text_layer_set_font(s_text_layer, lucky_font);
  text_layer_set_font(text_bottom_layer, lucky_font_small);

  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_PARTY_BLOB);
  GSize frame_size = gbitmap_sequence_get_bitmap_size(s_sequence);
  s_bitmap = gbitmap_create_blank(frame_size, GBitmapFormat8Bit);
  s_bitmap_layer = bitmap_layer_create(bounds);
  
  s_firstframe_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTY_BLOB_FIRST_FRAME);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_firstframe_bitmap);
  
  start_timers();
  update_time();

  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_bottom_layer));
}

static void prv_window_unload(Window *window) {
  backlight_service_unsubscribe();
  text_layer_destroy(s_text_layer);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  load_settings();
  backlight_service_subscribe(backlight_handler);
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
