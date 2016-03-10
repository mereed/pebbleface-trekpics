 /*
Copyright (C) 2016 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pebble.h"
	  
static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_CLEAR_DAY,
  RESOURCE_ID_CLEAR_NIGHT,
  RESOURCE_ID_WINDY,
  RESOURCE_ID_COLD,
  RESOURCE_ID_PARTLY_CLOUDY_DAY,
  RESOURCE_ID_PARTLY_CLOUDY_NIGHT,
  RESOURCE_ID_HAZE,
  RESOURCE_ID_CLOUD,
  RESOURCE_ID_RAIN,
  RESOURCE_ID_SNOW,
  RESOURCE_ID_HAIL,
  RESOURCE_ID_CLOUDY,
  RESOURCE_ID_STORM,
  RESOURCE_ID_FOG,
  RESOURCE_ID_NA,
};

enum key {
  ICON_KEY = 0x0,
  BLUETOOTHVIBE_KEY = 0x1,
  HOURLYVIBE_KEY = 0x2,
  TEMP_KEY = 0x3,
  BACKGROUND_KEY = 0x5,
//  STEPS_KEY = 0x6,
  FORMAT_KEY = 0x7
};

static AppSync sync;
static uint8_t sync_buffer[128];

static int background;
static int bluetoothvibe;
static int hourlyvibe;
static int format;
//static int steps_status;

static bool appStarted = false;

Window *window;
static Layer *window_layer;

GBitmap *background_image = NULL;
static BitmapLayer *background_layer;

GBitmap *batt_image = NULL;
static BitmapLayer *batt_layer;

GBitmap *nine_image = NULL;
static BitmapLayer *nine_layer;
GBitmap *three_image = NULL;
static BitmapLayer *three_layer;

BitmapLayer *icon_layer;
GBitmap *icon_bitmap = NULL;

TextLayer *layer_date_text;
TextLayer *layer_time_text;
TextLayer *layer_secs_text;

TextLayer *layer_date_text1;
TextLayer *layer_date_text2;
TextLayer *layer_date_text3;
TextLayer *layer_date_text4;
TextLayer *layer_date_text5;
TextLayer *layer_date_text6;
TextLayer *layer_date_text7;
TextLayer *layer_date_text8;
TextLayer *layer_date_text9;

TextLayer *temp_layer;

static GFont time_font;
static GFont temp_font;
static GFont batt_font;

int cur_day = -1;

int charge_percent = 0;

static int s_random = 13;
static int temp_random;

TextLayer *battery_text_layer;

Layer *minute_display_layer;

Layer *second_display_layer;

static bool status_showing = false;
static AppTimer *display_timer;


BitmapLayer *layer_conn_img;
GBitmap *img_bt_connect;
GBitmap *img_bt_disconnect;

//static TextLayer *steps_label;



/*
static void health_handler(HealthEventType event, void *context) {
  static char s_value_buffer[20];
  if (event == HealthEventMovementUpdate) {
    // display the step count
    snprintf(s_value_buffer, sizeof(s_value_buffer), "%d st.", (int)health_service_sum_today(HealthMetricStepCount));
	  
    text_layer_set_text(steps_label, s_value_buffer);
  }
}
*/
const GPathInfo SECOND_SEGMENT_PATH_POINTS = {
  3,
  (GPoint []) {
    {0, 0},
    {-6, -74}, // 70 = radius + fudge; 7 = 70*tan(6 degrees); 6 degrees per minute;
    {6,  -74},
  }
};

static GPath *second_segment_path;

static void second_display_layer_update_callback(Layer *layer, GContext* ctx) {

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  unsigned int angle = (t->tm_sec + 1) * 6;

  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  graphics_context_set_fill_color(ctx, GColorWhite);

  graphics_fill_circle(ctx, center, 71);

  graphics_context_set_fill_color(ctx, GColorBlack);

  for(; angle < 355; angle += 6) {

    gpath_rotate_to(second_segment_path, (TRIG_MAX_ANGLE / 360) * angle);

    gpath_draw_filled(ctx, second_segment_path);

  }

  graphics_fill_circle(ctx, center, 69);

}

const GPathInfo MINUTE_SEGMENT_PATH_POINTS = {
  3,
  (GPoint []) {
    {0, 0},
    {-8, -80}, // 70 = radius + fudge; 7 = 70*tan(6 degrees); 6 degrees per minute;
    {8,  -80},
  }
};

static GPath *minute_segment_path;

static void minute_display_layer_update_callback(Layer *layer, GContext* ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  unsigned int angle = t->tm_min * 6;
  gpath_rotate_to(minute_segment_path, (TRIG_MAX_ANGLE / 360) * angle);

  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  graphics_context_set_fill_color(ctx, GColorDarkGray);
	
  graphics_fill_circle(ctx, center, 78);
	
  graphics_context_set_fill_color(ctx, GColorBlack);

  gpath_draw_filled(ctx, minute_segment_path);
  graphics_fill_circle(ctx, center, 74);
}
	
void theme_choice() {

	switch(background) {
		
		case 0: // blank + time

			layer_set_hidden(text_layer_get_layer(layer_time_text), false);
			layer_set_hidden(bitmap_layer_get_layer(background_layer), true);

	    break;
		
		case 1: //grid + time
		
	    if (background_image) {
		gbitmap_destroy(background_image);
		background_image = NULL;
    }

			layer_set_hidden(text_layer_get_layer(layer_time_text), false);
			background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG19);		
					
	   if (background_image != NULL) {
		bitmap_layer_set_bitmap(background_layer, background_image);
		layer_set_hidden(bitmap_layer_get_layer(background_layer), false);
		layer_mark_dirty(bitmap_layer_get_layer(background_layer));
    }
	
	    break;

		case 2:  // united fed planets badge

	    if (background_image) {
		gbitmap_destroy(background_image);
		background_image = NULL;
    }

			layer_set_hidden(text_layer_get_layer(layer_time_text), true);
			background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG2);
		 	
			
	   if (background_image != NULL) {
		bitmap_layer_set_bitmap(background_layer, background_image);
		layer_set_hidden(bitmap_layer_get_layer(background_layer), false);
		layer_mark_dirty(bitmap_layer_get_layer(background_layer));
       }
			
	    break;
		case 3:  // starfleet badge

	    if (background_image) {
		gbitmap_destroy(background_image);
		background_image = NULL;
    }

			layer_set_hidden(text_layer_get_layer(layer_time_text), true);
			background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG12);
		 	
			
	   if (background_image != NULL) {
		bitmap_layer_set_bitmap(background_layer, background_image);
		layer_set_hidden(bitmap_layer_get_layer(background_layer), false);
		layer_mark_dirty(bitmap_layer_get_layer(background_layer));
       }
			
	    break;
		
		case 4:  // random all vessels

		if(s_random == 13){
			s_random = 0;
		} else {

			temp_random = rand() % 13;

			while(temp_random == s_random){
			    temp_random = rand() % 13;
		    }

		    s_random = temp_random;

	    if (background_image) {
		gbitmap_destroy(background_image);

		background_image = NULL;
			
		layer_set_hidden(text_layer_get_layer(layer_time_text), true);

    }

		   if(s_random == 0){
			   background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG5);
         } else if(s_random == 1){
				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG7);
         } else if(s_random == 2){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG10);
         } else if(s_random == 3){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG14);
         } else if(s_random == 4){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG15);
         } else if(s_random == 5){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG16);
         } else if(s_random == 6){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG17);
		 } else if(s_random == 7){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG22);
         } else if(s_random == 8){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG24);
         } else if(s_random == 9){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG30);
         } else if(s_random == 10){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG32);
         } else if(s_random == 11){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG33);
         } else if(s_random == 12){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG39);
         } //else if(s_random == 13){
 			//	background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG);
         //} else if(s_random == 14){
 		//		background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG);
         //} else if(s_random == 15){
 		//		background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG);
        // } else if(s_random == 16){
 			//	background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG);
		// } else if(s_random == 17){
 		//		background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG);
         //}
			
	   if (background_image != NULL) {
		bitmap_layer_set_bitmap(background_layer, background_image);
		layer_set_hidden(bitmap_layer_get_layer(background_layer), false);
		layer_mark_dirty(bitmap_layer_get_layer(background_layer));
       }
	}				
	    break;
	}
}
	
 void dateformat () {
	
	  switch (format) {
  
	case 0:  //Sun 28th, Jun	
		 layer_set_hidden(text_layer_get_layer(layer_date_text), false); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), true); 

	break;
		  
	case 1: // week 01
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), false); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), true); 
	break;
		  
	case 2: // stardate
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), false); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), true); 		  
	break;
		  
	case 3: // dd/mm/yy
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), false); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), true); 		  
	break;
		  
	case 4: // mm/dd/yy
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), false); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), true); 		  
	break;		  
		  
	case 5: // week daynum
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), false); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), true); 		  
	break;
		  
	case 6: // yyyy mm dd
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), false); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), true); 		  
	break;
		  
	case 7: // dd.mm.yyyy
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), false); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), true); 		  
	break;
		  
	case 8: // yy.ww.ddd
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), false); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), true); 		  
	break;

	case 9: // time [24hr]
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text1), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text2), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text3), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text4), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text5), true); 
		 layer_set_hidden(text_layer_get_layer(layer_date_text6), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text7), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text8), true); 		  
		 layer_set_hidden(text_layer_get_layer(layer_date_text9), false); 		  
	break; 
	  }
} 

void update_time(struct tm *tick_time) {

	static char time_text[] = "00:00";
    static char date_text[] = "mon  01st of jan";
    static char date_text1[] = "week 00";
    static char date_text2[] = "yyyy.ddd";
    static char date_text3[] = "HH:MM dd/mm";
    static char date_text4[] = "HH:MM mm/dd";
    static char date_text5[] = "Wxx Dxxx";
    static char date_text6[] = "yyyy mm dd";
    static char date_text7[] = "dd.mm.yyyy";
    static char date_text8[] = "yy.ww.ddd";
    static char date_text9[] = "00:00";
	
	
	char *time_format;

    int new_cur_day = tick_time->tm_year*1000 + tick_time->tm_yday;
    if (new_cur_day != cur_day) {
        cur_day = new_cur_day;

	switch(tick_time->tm_mday)
  {
    case 1 :
    case 21 :
    case 31 :
      strftime(date_text, sizeof(date_text), "%a %est of %b", tick_time);
      break;
    case 2 :
    case 22 :
      strftime(date_text, sizeof(date_text), "%a %end of %b", tick_time);
      break;
    case 3 :
    case 23 :
      strftime(date_text, sizeof(date_text), "%a %erd of %b", tick_time);
      break;
    default :
      strftime(date_text, sizeof(date_text), "%a %eth of %b", tick_time);
      break;
  }
	
	  text_layer_set_text(layer_date_text, date_text);
				
  }
	  strftime(date_text1, sizeof(date_text1), "Week %V", tick_time);
	  text_layer_set_text(layer_date_text1, date_text1);

	  strftime(date_text2, sizeof(date_text2), "%Y.%j", tick_time);
	  text_layer_set_text(layer_date_text2, date_text2);

	  strftime(date_text3, sizeof(date_text3), "%H:%M %d/%m", tick_time);
	  text_layer_set_text(layer_date_text3, date_text3);
			
	  strftime(date_text4, sizeof(date_text4), "%H:%M %m/%d", tick_time);
	  text_layer_set_text(layer_date_text4, date_text4); 
		
	  strftime(date_text5, sizeof(date_text5), "W%V D%j", tick_time);
	  text_layer_set_text(layer_date_text5, date_text5);

	  strftime(date_text6, sizeof(date_text6), "%Y %m %d", tick_time);
	  text_layer_set_text(layer_date_text6, date_text6);
	
	  strftime(date_text7, sizeof(date_text7), "%d.%m.%Y", tick_time);
	  text_layer_set_text(layer_date_text7, date_text7);
	
	  strftime(date_text8, sizeof(date_text8), "%y.%V.%j", tick_time);
	  text_layer_set_text(layer_date_text8, date_text8);
	
	  strftime(date_text9, sizeof(date_text9), "%H:%M", tick_time);
	  text_layer_set_text(layer_date_text9, date_text9);
	
		
	if (clock_is_24h_style()) {
        time_format = "%R";
		
    } else {
        time_format = "%I:%M";
    }

    strftime(time_text, sizeof(time_text), time_format, tick_time);

    if (!clock_is_24h_style() && (time_text[0] == '0')) {
        memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }
	
    text_layer_set_text(layer_time_text, time_text);
}

void hourvibe (struct tm *tick_time) {

  if(appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
  }
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
	
if (units_changed & MINUTE_UNIT) {
	theme_choice(); 
  }
  
}

// Hides seconds
void hide_status() {
	status_showing = false;
	layer_set_hidden((second_display_layer), true);

    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
	
}

// Shows battery
void show_status() {
	status_showing = true;
	//SHOW SECONDS RING
	
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(SECOND_UNIT, handle_minute_tick);
	
	layer_set_hidden((second_display_layer), false);
		
	// 40 Sec timer then call "hide_status". Cancels previous timer if another show_status is called within the 40000ms
	app_timer_cancel(display_timer);
	display_timer = app_timer_register(40000, hide_status, NULL);
}

// Shake/Tap Handler. On shake/tap... call "show_status"
void tap_handler(AccelAxisType axis, int32_t direction) {
	show_status();	
}


static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	if(new_tuple==NULL || new_tuple->value==NULL) {
		return;
	}
	
	switch (key) {
	  
	case TEMP_KEY:
      text_layer_set_text(temp_layer, new_tuple->value->cstring);
      break;

	case ICON_KEY:
      if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }
      icon_bitmap = gbitmap_create_with_resource(WEATHER_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
    break;
	  
    case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
    break;      
	  
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
    break;	  
	
	case BACKGROUND_KEY:
      background = new_tuple->value->uint8;
	  persist_write_bool(BACKGROUND_KEY, background);	  
	  theme_choice(); 
	break; 
	  
      break;
	
/*    case STEPS_KEY:
        steps_status = new_tuple->value->uint8 != 0;
	  persist_write_bool(STEPS_KEY, steps_status);	
	  
	  if (!steps_status) {
		  		  
		health_service_events_subscribe(health_handler, NULL);

    // force initial steps display
		health_handler(HealthEventMovementUpdate, NULL);
		layer_set_hidden(text_layer_get_layer(steps_label), false); 
            
        } else {
        health_service_events_unsubscribe();		
		layer_set_hidden(text_layer_get_layer(steps_label), true); 

	  }
	  break;    
*/
	case FORMAT_KEY:
      	format = new_tuple->value->uint8;
	  	persist_write_bool(FORMAT_KEY, format);
	  	dateformat();

	break;
  }
}

void update_battery_state(BatteryChargeState charge_state) {
	static char battery_text[] = "x100";

    if (charge_state.is_charging) {
  window_set_background_color(window, GColorWhite);
		snprintf(battery_text, sizeof(battery_text), "+%d", charge_state.charge_percent);

    } else {
	    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);       
	
        if (charge_state.charge_percent <= 10) {
  window_set_background_color(window, GColorBulgarianRose );

        } else if (charge_state.charge_percent <= 20) {
  window_set_background_color(window, GColorRed);
		  
		} else if (charge_state.charge_percent <= 30) {
  window_set_background_color(window, GColorRed);
			  
		} else if (charge_state.charge_percent <= 40) {
  window_set_background_color(window, GColorOrange );

		} else if (charge_state.charge_percent <= 50) {
  window_set_background_color(window, GColorPurple );

        } else if (charge_state.charge_percent <= 60) {
  window_set_background_color(window, GColorIndigo );

		} else if (charge_state.charge_percent <= 70) {
  window_set_background_color(window, GColorBlue );

		} else if (charge_state.charge_percent <= 80) {
  window_set_background_color(window, GColorGreen);

		} else if (charge_state.charge_percent <= 90) {
  window_set_background_color(window, GColorGreen);
			  

		} else if (charge_state.charge_percent <= 100) {
  window_set_background_color(window, GColorDarkGreen );

		} else {
  window_set_background_color(window, GColorGreen);
			  text_layer_set_text_color(battery_text_layer, GColorBlack);

        } 
    } 
    charge_percent = charge_state.charge_percent;  
	text_layer_set_text(battery_text_layer, battery_text);

} 


void handle_bluetooth(bool connected) {
    if (connected) {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    } else {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_disconnect);
    }

    if (appStarted && bluetoothvibe) {
      
        vibes_long_pulse();
	}
}

void force_update(void) {
    handle_bluetooth(bluetooth_connection_service_peek());
    time_t now = time(NULL);
    update_time(localtime(&now));
}


static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	  GRect bounds = layer_get_bounds(window_layer);

  // Init the layer for the minute display
  minute_display_layer = layer_create(bounds);
  layer_set_update_proc(minute_display_layer, minute_display_layer_update_callback);
  layer_add_child(window_layer, minute_display_layer);

  // Init the minute segment path
  minute_segment_path = gpath_create(&MINUTE_SEGMENT_PATH_POINTS);
  gpath_move_to(minute_segment_path, grect_center_point(&bounds));
	
  // Init the layer for the second display
  second_display_layer = layer_create(bounds);
  layer_set_update_proc(second_display_layer, second_display_layer_update_callback);
  layer_add_child(window_layer, second_display_layer);

  // Init the second segment path
  second_segment_path = gpath_create(&SECOND_SEGMENT_PATH_POINTS);
  gpath_move_to(second_segment_path, grect_center_point(&bounds));

  // draw a background if displayed
  background_image = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BG19 );
  background_layer = bitmap_layer_create( bounds );
  bitmap_layer_set_bitmap( background_layer, background_image );
  GCompOp compositing_mod_back = GCompOpSet;
  bitmap_layer_set_compositing_mode(background_layer, compositing_mod_back);	
  layer_add_child( window_layer, bitmap_layer_get_layer( background_layer ) );
	
	// resources
	img_bt_connect     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
    img_bt_disconnect  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_OFF);
		
	// draw the bluetooth image
#ifdef PBL_PLATFORM_CHALK
	layer_conn_img  = bitmap_layer_create(GRect(74, 0,  32,  26));
#else
	layer_conn_img  = bitmap_layer_create(GRect(57, -3,  32,  26));
#endif	
    bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
	GCompOp compositing_mode = GCompOpSet;
    bitmap_layer_set_compositing_mode(layer_conn_img, compositing_mode);
    layer_add_child(window_layer, bitmap_layer_get_layer(layer_conn_img)); 
	
	
	// resources

	time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_78));
    batt_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_16));
    temp_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_25));
	

// setup time layer
#ifdef PBL_PLATFORM_CHALK
    layer_time_text = text_layer_create(GRect(  0,   34, 180,  80));
#else
	layer_time_text = text_layer_create(GRect(0, 30, 144, 80));
#endif
    text_layer_set_background_color(layer_time_text, GColorClear);
    text_layer_set_font(layer_time_text, time_font);
    text_layer_set_text_alignment(layer_time_text, GTextAlignmentCenter);
    text_layer_set_text_color(layer_time_text, GColorWhite);
    layer_add_child(window_layer, text_layer_get_layer(layer_time_text));

// setup date layer
	
#ifdef PBL_PLATFORM_CHALK
    layer_date_text = text_layer_create(GRect( 0, 114, 180,  30));
#else
    layer_date_text = text_layer_create(GRect(0, 109, 144, 30));
#endif
    text_layer_set_background_color(layer_date_text, GColorClear);
    text_layer_set_font(layer_date_text, temp_font);
    text_layer_set_text_alignment(layer_date_text, GTextAlignmentCenter);
    text_layer_set_text_color(layer_date_text, GColorWhite);
    layer_add_child(window_layer, text_layer_get_layer(layer_date_text));

// Setup date format layers
	
#ifdef PBL_PLATFORM_CHALK
    layer_date_text1 = text_layer_create(GRect( 0, 114, 180,  30));
#else
    layer_date_text1 = text_layer_create(GRect(0, 110, 144, 30));
#endif
  text_layer_set_text_alignment(layer_date_text1, GTextAlignmentCenter);
  text_layer_set_text_color(layer_date_text1, GColorWhite  );
  text_layer_set_background_color(layer_date_text1, GColorClear);
  text_layer_set_font(layer_date_text1, temp_font);	  
  layer_add_child( window_layer, text_layer_get_layer( layer_date_text1 ) );
  
#ifdef PBL_PLATFORM_CHALK
  layer_date_text2 = text_layer_create(GRect( 0, 121, 180,  30 ));	
#else
    layer_date_text2 = text_layer_create(GRect(0, 110, 144, 30));
#endif
  text_layer_set_text_alignment(layer_date_text2, GTextAlignmentCenter);
  text_layer_set_text_color(layer_date_text2, GColorWhite  );
  text_layer_set_background_color(layer_date_text2, GColorClear);
  text_layer_set_font(layer_date_text2, temp_font);	  
  layer_add_child( window_layer, text_layer_get_layer( layer_date_text2 ) );
  
#ifdef PBL_PLATFORM_CHALK
  layer_date_text3 = text_layer_create(GRect( 0, 121, 180,  30 ));	
#else
    layer_date_text3 = text_layer_create(GRect(0, 110, 144, 30));
#endif
  text_layer_set_text_alignment(layer_date_text3, GTextAlignmentCenter);
  text_layer_set_text_color(layer_date_text3, GColorWhite  );
  text_layer_set_background_color(layer_date_text3, GColorClear);
  text_layer_set_font(layer_date_text3, temp_font);	  
  layer_add_child( window_layer, text_layer_get_layer( layer_date_text3 ) );
  
#ifdef PBL_PLATFORM_CHALK
  layer_date_text4 = text_layer_create(GRect( 0, 121, 180,  30 ));	
#else
    layer_date_text4 = text_layer_create(GRect(0, 110, 144, 30));
#endif
  text_layer_set_text_alignment(layer_date_text4, GTextAlignmentCenter);
  text_layer_set_text_color(layer_date_text4, GColorWhite  );
  text_layer_set_background_color(layer_date_text4, GColorClear);
  text_layer_set_font(layer_date_text4, temp_font);	  
  layer_add_child( window_layer, text_layer_get_layer( layer_date_text4 ) );
  
#ifdef PBL_PLATFORM_CHALK
  layer_date_text5 = text_layer_create(GRect( 0, 121, 180,  30 ));	
#else
    layer_date_text5 = text_layer_create(GRect(0, 106, 144, 30));
#endif
  text_layer_set_text_alignment(layer_date_text5, GTextAlignmentCenter);
  text_layer_set_text_color(layer_date_text5, GColorWhite  );
  text_layer_set_background_color(layer_date_text5, GColorClear);
  text_layer_set_font(layer_date_text5, temp_font);	  
  layer_add_child( window_layer, text_layer_get_layer( layer_date_text5 ) );
  
#ifdef PBL_PLATFORM_CHALK
  layer_date_text6 = text_layer_create(GRect( 0, 121, 180,  30 ));	
#else
    layer_date_text6 = text_layer_create(GRect(0, 110, 144, 30));
#endif
  text_layer_set_text_alignment(layer_date_text6, GTextAlignmentCenter);
  text_layer_set_text_color(layer_date_text6, GColorWhite  );
  text_layer_set_background_color(layer_date_text6, GColorClear);
  text_layer_set_font(layer_date_text6, temp_font);	  
  layer_add_child( window_layer, text_layer_get_layer( layer_date_text6 ) );
  	
#ifdef PBL_PLATFORM_CHALK
  layer_date_text7 = text_layer_create(GRect( 0, 121, 180,  30 ));	
#else
    layer_date_text7 = text_layer_create(GRect(0, 110, 144, 30));
#endif
  text_layer_set_text_alignment(layer_date_text7, GTextAlignmentCenter);
  text_layer_set_text_color(layer_date_text7, GColorWhite  );
  text_layer_set_background_color(layer_date_text7, GColorClear);
  text_layer_set_font(layer_date_text7, temp_font);	  
  layer_add_child( window_layer, text_layer_get_layer( layer_date_text7 ) );

#ifdef PBL_PLATFORM_CHALK
  layer_date_text8 = text_layer_create(GRect( 0, 121, 180,  30 ));	
#else
    layer_date_text8 = text_layer_create(GRect(0, 110, 144, 30));
#endif
  text_layer_set_text_alignment(layer_date_text8, GTextAlignmentCenter);
  text_layer_set_text_color(layer_date_text8, GColorWhite  );
  text_layer_set_background_color(layer_date_text8, GColorClear);
  text_layer_set_font(layer_date_text8, temp_font);	  
  layer_add_child( window_layer, text_layer_get_layer( layer_date_text8 ) );
	
#ifdef PBL_PLATFORM_CHALK
  layer_date_text9 = text_layer_create(GRect( 0, 126, 180,  30 ));	
#else
    layer_date_text9 = text_layer_create(GRect(0, 114, 144, 30));
#endif
  text_layer_set_text_alignment(layer_date_text9, GTextAlignmentCenter);
  text_layer_set_text_color(layer_date_text9, GColorWhite  );
  text_layer_set_background_color(layer_date_text9, GColorClear);
  text_layer_set_font(layer_date_text9, temp_font);	  
  layer_add_child( window_layer, text_layer_get_layer( layer_date_text9 ) );
	
  // draw a background if displayed
  batt_image = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BATT );
#ifdef PBL_PLATFORM_CHALK
  batt_layer = bitmap_layer_create( GRect(74,158,32,26) );
#else
  batt_layer = bitmap_layer_create( GRect(57,145,32,26) );
#endif
  bitmap_layer_set_bitmap( batt_layer, batt_image );
  GCompOp compositing_batt = GCompOpSet;
  bitmap_layer_set_compositing_mode(batt_layer, compositing_batt);	
  layer_add_child( window_layer, bitmap_layer_get_layer( batt_layer ) );
		
  // set up battery text layer
#ifdef PBL_PLATFORM_CHALK
  battery_text_layer = text_layer_create(GRect(72, 160, 40, 20));	
#else
  battery_text_layer = text_layer_create(GRect(56, 148, 40, 20));	
#endif
  text_layer_set_background_color(battery_text_layer, GColorClear);
  text_layer_set_text_color(battery_text_layer, GColorBlack);
  text_layer_set_font(battery_text_layer, batt_font);
  text_layer_set_text_alignment(battery_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(battery_text_layer));		
	
  // draw 9 marker
  nine_image = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_9MARK );
#ifdef PBL_PLATFORM_CHALK
  nine_layer = bitmap_layer_create( GRect(-2,86,26,11) );
#else
  nine_layer = bitmap_layer_create(GRect(-20, 78, 26, 11));
#endif
  bitmap_layer_set_bitmap( nine_layer, nine_image );
  GCompOp compositing_nine = GCompOpSet;
  bitmap_layer_set_compositing_mode(nine_layer, compositing_nine);	
  layer_add_child( window_layer, bitmap_layer_get_layer( nine_layer ) );
	
  // draw 3 marker
  three_image = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_3MARK );
#ifdef PBL_PLATFORM_CHALK
  three_layer = bitmap_layer_create( GRect(158,86,26,11) );
#else
  three_layer = bitmap_layer_create(GRect(138, 78, 26, 11));
#endif
  bitmap_layer_set_bitmap( three_layer, three_image );
  GCompOp compositing_three = GCompOpSet;
  bitmap_layer_set_compositing_mode(three_layer, compositing_three);	
  layer_add_child( window_layer, bitmap_layer_get_layer( three_layer ) );
/*	
// setup steps layer
	
  steps_label = text_layer_create( GRect(  43, 115, 95,  27 ));
  text_layer_set_background_color(steps_label, GColorBlack);
  text_layer_set_text_color(steps_label, GColorWhite  );
  text_layer_set_text_alignment(steps_label, GTextAlignmentCenter);
  text_layer_set_font(steps_label, temp_font);
  layer_add_child(window_layer, text_layer_get_layer(steps_label));
  layer_set_hidden(text_layer_get_layer(steps_label), true);
*/
//setup weather layers
	
#ifdef PBL_PLATFORM_CHALK
  icon_layer = bitmap_layer_create(GRect(97,  27, 24, 24));
#else
  icon_layer = bitmap_layer_create(GRect(74, 23, 24, 24));
#endif
  GCompOp compositing_weather = GCompOpSet;
  bitmap_layer_set_compositing_mode(icon_layer, compositing_weather);	
  layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));

	
#ifdef PBL_PLATFORM_CHALK
    temp_layer  = text_layer_create(GRect(60,  22, 40,  30 ));
#else
    temp_layer  = text_layer_create(GRect(42, 19, 40, 30));
#endif
	text_layer_set_background_color(temp_layer, GColorClear);
    text_layer_set_font(temp_layer, temp_font);
    text_layer_set_text_alignment(temp_layer, GTextAlignmentCenter);
    text_layer_set_text_color(temp_layer, GColorWhite);
	layer_add_child(window_layer, text_layer_get_layer(temp_layer));
		
	
    appStarted = true;

  //comment this out if you don't want seconds on start
  //show_status();
  hide_status();
	
  // draw first frame
  force_update();
	
}

static void window_unload(Window *window) {

  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);

	if (background_image != NULL) {
		gbitmap_destroy(background_image);
    }

  layer_remove_from_parent(bitmap_layer_get_layer(layer_conn_img));
  bitmap_layer_destroy(layer_conn_img);
  gbitmap_destroy(img_bt_connect);
  gbitmap_destroy(img_bt_disconnect);
  img_bt_connect = NULL;
  img_bt_disconnect = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(icon_layer));
  bitmap_layer_destroy(icon_layer);
  gbitmap_destroy(icon_bitmap);
  icon_bitmap = NULL;

  layer_remove_from_parent(bitmap_layer_get_layer(nine_layer));
  bitmap_layer_destroy(nine_layer);
  gbitmap_destroy(nine_image);
  nine_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(three_layer));
  bitmap_layer_destroy(three_layer);
  gbitmap_destroy(three_image);
  three_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(batt_layer));
  bitmap_layer_destroy(batt_layer);
  gbitmap_destroy(batt_image);
  batt_image = NULL;
  
  layer_remove_from_parent(minute_display_layer);
  layer_destroy(minute_display_layer);
  gpath_destroy(minute_segment_path);
	
  layer_remove_from_parent(second_display_layer);
  layer_destroy(second_display_layer);
  gpath_destroy(second_segment_path);
	
  text_layer_destroy( layer_time_text );
  text_layer_destroy( layer_date_text );
  text_layer_destroy( layer_date_text1 );
  text_layer_destroy( layer_date_text2 );
  text_layer_destroy( layer_date_text3 );
  text_layer_destroy( layer_date_text4 );
  text_layer_destroy( layer_date_text5 );
  text_layer_destroy( layer_date_text6 );
  text_layer_destroy( layer_date_text7 );
  text_layer_destroy( layer_date_text8 );
  text_layer_destroy( layer_date_text9 );

  text_layer_destroy( layer_secs_text );
  text_layer_destroy( temp_layer );
  text_layer_destroy( battery_text_layer );
//  text_layer_destroy( steps_label );
	
  fonts_unload_custom_font(time_font);
  fonts_unload_custom_font(batt_font);
  fonts_unload_custom_font(temp_font);
	
  layer_remove_from_parent(window_layer);
  layer_destroy(window_layer);
	
}

void main_window_push() {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);
}

static void init() {
	main_window_push();

   Tuplet initial_values[] = {
	TupletCString(TEMP_KEY, ""),
    TupletInteger(ICON_KEY, (uint8_t) 14),
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
    TupletInteger(BACKGROUND_KEY, persist_read_bool(BACKGROUND_KEY)),	
//    TupletInteger(STEPS_KEY, persist_read_bool(STEPS_KEY)),	
    TupletInteger(FORMAT_KEY, persist_read_bool(FORMAT_KEY)),	
  };
  
	app_message_open(128, 128);

	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, NULL, NULL);
   
  tick_timer_service_subscribe(SECOND_UNIT, handle_minute_tick);
  battery_state_service_subscribe(&update_battery_state);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  accel_tap_service_subscribe(tap_handler);

// update the battery on launch
    update_battery_state(battery_state_service_peek());
}

void handle_deinit(void) {
	
  app_sync_deinit(&sync);

  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
	
  window_destroy(window);

}

int main(void) {
    init();
    app_event_loop();
    handle_deinit();
}
