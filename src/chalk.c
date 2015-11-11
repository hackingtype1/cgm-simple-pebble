// #include <pebble.h>
// #include <pebble_chart.h>
// #include <data-processor.h>
// #include <cgm_info.h>

// #define ANTIALIASING true

// #define HAND_MARGIN  - 1
// #define FINAL_RADIUS 68

// #define ANIMATION_DURATION 500
// #define ANIMATION_DELAY    0

// typedef struct {
//     int hours;
//     int minutes;
// } Time;

// static Window * s_main_window;
// static Layer * s_canvas_layer;

// static GPoint s_center;
// static Time s_last_time, s_anim_time;
// static int s_radius = 0, t_delta = 0, has_launched = 0, vibe_state = 1, alert_state = 0 ,s_anim_hours_60 = 0, com_alert = 1;
// // static int bgs[8] = { 0 };
// // static int bg_times[8] = { 0 };
// static int * bgs;
// static int * bg_times;
// static int num_bgs = 0;
// static bool s_animating = false;
// static bool out_msg_inflight = false;

// static GBitmap *icon_bitmap = NULL;

// static BitmapLayer * icon_layer;
// static TextLayer * bg_layer, *delta_layer, *time_delta_layer, *time_layer, *date_layer;

// static char date_app_text[] = "";
// static char last_bg[124];
// static char data_id[124];
// static char time_delta_str[124] = "";
// static char time_text[124] = "";

// enum CgmKey {
//     CGM_EGV_DELTA_KEY = 0x0,
//     CGM_EGV_KEY = 0x1,
//     CGM_TREND_KEY = 0x2,
//     CGM_ALERT_KEY = 0x3,
//     CGM_VIBE_KEY = 0x4,
//     CGM_ID = 0x5,
//     CGM_TIME_DELTA_KEY = 0x6,
// 	CGM_BGS = 0x7,
//     CGM_BG_TIMES = 0x8
// };


// enum Alerts {
//     OKAY = 0x0,
//     LOSS_MID_NO_NOISE = 0x1,
//     LOSS_HIGH_NO_NOISE = 0x2,
//     NO_CHANGE = 0x3,
//     OLD_DATA = 0x4,
// };

// static int s_color_channels[3] = { 85, 85, 85 };
// static int b_color_channels[3] = { 0, 0, 0 };

// static const uint32_t const error[] = { 100,100,100,100,100 };

// static const uint32_t CGM_ICONS[] = {
//     RESOURCE_ID_IMAGE_NONE_WHITE,	  //4 - 0
//     RESOURCE_ID_IMAGE_UPUP_WHITE,     //0 - 1
//     RESOURCE_ID_IMAGE_UP_WHITE,       //1 - 2
//     RESOURCE_ID_IMAGE_UP45_WHITE,     //2 - 3
//     RESOURCE_ID_IMAGE_FLAT_WHITE,     //3 - 4
//     RESOURCE_ID_IMAGE_DOWN45_WHITE,   //5 - 5
//     RESOURCE_ID_IMAGE_DOWN_WHITE,     //6 - 6
//     RESOURCE_ID_IMAGE_DOWNDOWN_WHITE,  //7 - 7
// };

// char *translate_error(AppMessageResult result) {
//     switch (result) {
//         case APP_MSG_OK: return "OK";
//         case APP_MSG_SEND_TIMEOUT: return "Send Timeout";
//         case APP_MSG_SEND_REJECTED: return "Send Rejected";
//         case APP_MSG_NOT_CONNECTED: return "Not Connected";
//         case APP_MSG_APP_NOT_RUNNING: return "App Not Up";
//         case APP_MSG_INVALID_ARGS: return "App Invalid Args";
//         case APP_MSG_BUSY: return "App Busy";
//         case APP_MSG_BUFFER_OVERFLOW: return "App Overflow";
//         case APP_MSG_ALREADY_RELEASED: return "App Msg Released";
//         case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "Cback Registered";
//         case APP_MSG_CALLBACK_NOT_REGISTERED: return "Cback Not Registered";
//         case APP_MSG_OUT_OF_MEMORY: return "Out of Memory";
//         case APP_MSG_CLOSED: return "Closed";
//         case APP_MSG_INTERNAL_ERROR: return "Internal Error";
//         default: return "Unknown Error";
//     }
// }

// /**********CHART***************/

// static ChartLayer* chart_layer;

// static void load_chart_1() {
//   //const int x[] = { 0, 5, 10, 15, 20, 25, 30 };
//   //const int y[] = { bg6, bg5, bg4, bg3, bg2, bg1, bg0};
//   //chart_layer_show_frame(chart_layer, true);
//   chart_layer_set_margin(chart_layer, 6);
//   //chart_layer_set_plot_type(chart_layer, eSCATTER);
//   chart_layer_set_data(chart_layer, bg_times, eINT, bgs, eINT, 8);
// }

// /*************************** AnimationImplementation **************************/
// static void animation_started(Animation * anim, void *context) {
//     s_animating = true;
// }

// static void animation_stopped(Animation * anim, bool stopped, void *context) {
//     s_animating = false;
// }

// static void animate(int duration, int delay, AnimationImplementation * implementation, bool handlers) {
//     Animation * anim = animation_create();
//     animation_set_duration(anim, duration);
//     animation_set_delay(anim, delay);
//     animation_set_curve(anim, AnimationCurveEaseInOut);
//     animation_set_implementation(anim, implementation);
//     if (handlers) {
//         animation_set_handlers(anim, (AnimationHandlers) {
//             .started = animation_started,
//             .stopped = animation_stopped
//         }, NULL);
//     }
//     animation_schedule(anim);
// }

// static int hours_to_minutes(int hours_out_of_12) {
//     return (int)(float)(((float)hours_out_of_12 / 12.0F) * 60.0F);
// }

// /************************************ UI **************************************/
// void send_cmd(void) {
    
//     if (s_canvas_layer) {
//         gbitmap_destroy(icon_bitmap);
//         text_layer_set_text(time_delta_layer, "check...");
//         text_layer_set_text(delta_layer, "");
//         //text_layer_set_text(bg_layer, "");
//         icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_REFRESH_WHITE);
//         bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
//     }

//     if (!out_msg_inflight) {
//         DictionaryIterator *iter;
//         app_message_outbox_begin(&iter);
    
//         if (iter == NULL) {
//             return;
//         }
    
//         static char *idptr = data_id;

//         Tuplet idval = TupletCString(5, idptr);
    
//         dict_write_tuplet(iter, &idval);
//         dict_write_end(iter);
    
//         app_message_outbox_send();
//     }
      
// }

// static void clock_refresh(struct tm * tick_time) {
//      char *time_format;
 
//     if (!tick_time) {
//         time_t now = time(NULL);
//         tick_time = localtime(&now);
//     }
    
//     if (clock_is_24h_style()) {
//         time_format = "%H:%M  %B %e";
//     } else {
//         time_format = "%I:%M  %B %e";
//     }
    
//     strftime(time_text, sizeof(time_text), time_format, tick_time);
    
//     if (time_text[0] == '0') {
//         memmove(time_text, &time_text[1], sizeof(time_text) - 1);
//     }
    
//     if (s_canvas_layer) {
//         layer_mark_dirty(s_canvas_layer);
//         text_layer_set_text(time_layer, time_text);
//     }

//     s_last_time.hours = tick_time->tm_hour;
//     APP_LOG(APP_LOG_LEVEL_DEBUG, "time hours 1: %d", s_last_time.hours);
//     s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
//     APP_LOG(APP_LOG_LEVEL_DEBUG, "time hours 2: %d", s_last_time.hours);
//     s_last_time.minutes = tick_time->tm_min;
    
// }

// void accel_tap_handler(AccelAxisType axis, int32_t direction) {
    
//     if (axis == ACCEL_AXIS_X)
//     {
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "axis: %s", "X");
//         //send_cmd();
//     } else if (axis == ACCEL_AXIS_Y)
        
//     {
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "axis: %s", "Y");
//         //send_cmd();
//     } else if (axis == ACCEL_AXIS_Z)
//     {
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "axis: %s", "Z");
//         //send_cmd();
//     }
    
// }

// static void tick_handler(struct tm * tick_time, TimeUnits changed) {
//     if (!has_launched) {                 
//             snprintf(time_delta_str, 12, "load...");
            
//             if (time_delta_layer) {
//                 text_layer_set_text(time_delta_layer, time_delta_str);
//             }
//     } else 
    
//     if(t_delta > 4) {
//         //accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
//         //accel_tap_service_subscribe(accel_tap_handler);
//         out_msg_inflight = false;
//         send_cmd();
//     } else
//     {   
//         if (has_launched) {
//             if (t_delta <= 0) {
//                 t_delta = 0;
//                 snprintf(time_delta_str, 12, "now"); // puts string into buffer
//             } else {
//                 snprintf(time_delta_str, 12, "%d min", t_delta); // puts string into buffer
//             }
//             if (time_delta_layer) {
//                 text_layer_set_text(time_delta_layer, time_delta_str);
//             }
//         } else {

//         }
//     }
//     t_delta++;  
//     clock_refresh(tick_time);
    
// }
// static void update_proc(Layer * layer, GContext * ctx) {

// #ifdef PBL_PLATFORM_CHALK

//     graphics_context_set_fill_color(ctx, GColorFromRGB(b_color_channels[0], b_color_channels[1], b_color_channels[2]));
//     graphics_fill_rect(ctx, GRect(0, 0, 180, 180), 0, GCornerNone);
    
//     graphics_context_set_stroke_color(ctx, GColorBlack); 
//     graphics_context_set_antialiased(ctx, ANTIALIASING);
    
//     // White clockface
//     graphics_context_set_fill_color(ctx, GColorFromRGB(s_color_channels[0], s_color_channels[1], s_color_channels[2]));
//     graphics_fill_rect(ctx, GRect(18, 40, 144, 60), 4, GCornersAll);
//     graphics_context_set_stroke_color(ctx, GColorWhite); 
//     graphics_context_set_stroke_width(ctx, 2);
//     graphics_draw_round_rect(ctx, GRect(18, 40, 144, 60), 4);

// #else
//     //Change Background
//     if (b_color_channels[0] == 170) {
//         //OKAY
//         graphics_context_set_fill_color(ctx, GColorWhite);
//         graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);
//         if(time_layer)
//             text_layer_set_text_color(time_layer, GColorBlack);
//     } else {
//         //BAD COMMS
//         graphics_context_set_fill_color(ctx, GColorBlack);
//         graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);
//         if(time_layer)
//             text_layer_set_text_color(time_layer, GColorWhite);
//     }
//     // Change clockface
//     if (s_color_channels[0] < 255){
//         //OKAY
//         graphics_context_set_stroke_color(ctx, GColorBlack);
//         graphics_context_set_fill_color(ctx, GColorClear);
        
//     } else {
//         //NOT OKAY
//         graphics_context_set_stroke_color(ctx, GColorWhite);
//         graphics_context_set_fill_color(ctx, GColorBlack);
//     }
//     graphics_fill_rect(ctx, GRect(5, 29, 134, 134), 0, GCornerNone);
//     //graphics_draw_round_rect(ctx, GRect(5, 29, 134, 134), 5);
// #endif
    
// }

// static void reset_background() {
    
//     b_color_channels[0] = 0;
//     b_color_channels[1] = 0;
//     b_color_channels[2] = 0;
//     if(time_layer)
//         text_layer_set_text_color(time_layer, GColorWhite);
//     layer_mark_dirty(s_canvas_layer);
// }

// static void process_alert() {
//     APP_LOG(APP_LOG_LEVEL_DEBUG, "Vibe State: %i", vibe_state);
//     switch (alert_state) {
        
//     case LOSS_MID_NO_NOISE:;    
//         s_color_channels[0] = 255;
//         s_color_channels[1] = 255;
//         s_color_channels[2] = 0;
        
//         if (vibe_state > 0)
//             vibes_long_pulse();
            
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "Alert key: %i", LOSS_MID_NO_NOISE);
// #ifdef PBL_PLATFORM_BASALT
//         text_layer_set_text_color(bg_layer, GColorBlack);
//         text_layer_set_text_color(delta_layer, GColorBlack);
//         text_layer_set_text_color(time_delta_layer, GColorBlack);
//         bitmap_layer_set_compositing_mode(icon_layer, GCompOpClear);
// #else
//         text_layer_set_text_color(bg_layer, GColorWhite);
//         text_layer_set_text_color(delta_layer, GColorWhite);
//         text_layer_set_text_color(time_delta_layer, GColorWhite);
//         bitmap_layer_set_compositing_mode(icon_layer, GCompOpOr);
// #endif  

  	             
// #ifdef PBL_PLATFORM_BASALT
//         //chart_layer_set_plot_color(chart_layer, GColorBlack); 
//   	    //chart_layer_set_canvas_color(chart_layer, GColorFromRGB(s_color_channels[0], s_color_channels[1], s_color_channels[2]));
// #else   
//         //chart_layer_set_plot_color(chart_layer, GColorWhite);    
//         //chart_layer_set_canvas_color(chart_layer, GColorBlack);
// #endif                      

//         break;
        
//     case LOSS_HIGH_NO_NOISE:;
//         s_color_channels[0] = 255;
//         s_color_channels[1] = 0;
//         s_color_channels[2] = 0;
        
//        if (vibe_state > 0)
//             vibes_long_pulse();
            
//         //chart_layer_set_plot_color(chart_layer, GColorWhite);          
// #ifdef PBL_PLATFORM_BASALT
//   	    //chart_layer_set_canvas_color(chart_layer, GColorFromRGB(s_color_channels[0], s_color_channels[1], s_color_channels[2]));
// #else      
//         //chart_layer_set_canvas_color(chart_layer, GColorBlack);
// #endif
        
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "Alert key: %i", LOSS_HIGH_NO_NOISE);
//         text_layer_set_text_color(bg_layer, GColorWhite);
//         text_layer_set_text_color(delta_layer, GColorWhite);
//         text_layer_set_text_color(time_delta_layer, GColorWhite);
//         bitmap_layer_set_compositing_mode(icon_layer, GCompOpOr);
//         break;
        
//     case OKAY:;
//         s_color_channels[0] = 0;
//         s_color_channels[1] = 255;
//         s_color_channels[2] = 0;
        
//         if (vibe_state > 1)
//             vibes_double_pulse();
            
//         //chart_layer_set_plot_color(chart_layer, GColorBlack);          
// #ifdef PBL_PLATFORM_BASALT
//   	    //chart_layer_set_canvas_color(chart_layer, GColorFromRGB(s_color_channels[0], s_color_channels[1], s_color_channels[2]));
// #else      
//         //chart_layer_set_canvas_color(chart_layer, GColorWhite);
// #endif
        
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "Alert key: %i", OKAY);
//         text_layer_set_text_color(bg_layer, GColorBlack);
//         text_layer_set_text_color(delta_layer, GColorBlack);
//         text_layer_set_text_color(time_delta_layer, GColorBlack);
//         bitmap_layer_set_compositing_mode(icon_layer, GCompOpClear);
//         break;
        
//     case OLD_DATA:;
//         VibePattern pat = {
//             .durations = error,
//             .num_segments = ARRAY_LENGTH(error),
//         };
        
//         vibes_enqueue_custom_pattern(pat);
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "Alert key: %i", OLD_DATA);
        
//         s_color_channels[0] = 0;
//         s_color_channels[1] = 255;
//         s_color_channels[2] = 255;
// #ifdef PBL_PLATFORM_BASALT
//         text_layer_set_text_color(time_delta_layer, GColorRed);
//         bitmap_layer_set_compositing_mode(icon_layer, GCompOpOr);
// #else
// #endif

//   	    //chart_layer_set_plot_color(chart_layer, GColorBlack);          
// #ifdef PBL_PLATFORM_BASALT
//   	    //chart_layer_set_canvas_color(chart_layer, GColorFromRGB(s_color_channels[0], s_color_channels[1], s_color_channels[2]));
// #else      
//         //chart_layer_set_canvas_color(chart_layer, GColorWhite);
// #endif

//         text_layer_set_text_color(bg_layer, GColorBlack);
//         text_layer_set_text_color(delta_layer, GColorBlack);
        
//         bitmap_layer_set_compositing_mode(icon_layer, GCompOpOr);
//         break;
     
//      case NO_CHANGE:;
//         break;   
//     }
        
// }

// static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
//     APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
//     text_layer_set_text(time_delta_layer, "check...");
//     // Get the first pair
//     Tuple *new_tuple = dict_read_first(iterator);
//     reset_background();
//     CgmData* cgm_data = cgm_data_create(1,2,"3m","199","+3mg/dL","Evan");
   
//     // Process all pairs present
//     while(new_tuple != NULL) {
//         // Process this pair's key
        
//         switch (new_tuple->key) {
                
//             case CGM_ID:;
//                 strncpy(data_id, new_tuple->value->cstring, 124);
//                 break;
                
//             case CGM_EGV_DELTA_KEY:;
//                 text_layer_set_text(delta_layer, new_tuple->value->cstring);
//                 break;
                
//             case CGM_EGV_KEY:;
//                 cgm_data_set_egv(cgm_data, new_tuple->value->cstring);
//                 //cgm_data_set_egv(cgm_data, "999");
//                 text_layer_set_text(bg_layer, cgm_data_get_egv(cgm_data));
//                 //text_layer_set_text(bg_layer, new_tuple->value->cstring);
//                 //bgs[6] = atoi(new_tuple->value->cstring);
//                 strncpy(last_bg, new_tuple->value->cstring, 124);
//                 break;
                
//             case CGM_TREND_KEY:;
//                 if (icon_bitmap) {
//                     gbitmap_destroy(icon_bitmap);
//                 }
//                 icon_bitmap = gbitmap_create_with_resource(CGM_ICONS[new_tuple->value->uint8]);
//                 bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
//                 break;
                
//             case CGM_ALERT_KEY:;
//                 alert_state = new_tuple->value->uint8;
//                 break;
                
//             case CGM_VIBE_KEY:
//                 vibe_state = new_tuple->value->int16;             
//                 break;
                
//             case CGM_TIME_DELTA_KEY:;
//                 t_delta = new_tuple->value->int16;
//                 if (t_delta <= 0) {
//                 t_delta = 0;
//                     snprintf(time_delta_str, 12, "now"); // puts string into buffer
//                 } else {
//                     snprintf(time_delta_str, 12, "%d min", t_delta); // puts string into buffer
//                 }
//                 if (time_delta_layer) {
//                     text_layer_set_text(time_delta_layer, time_delta_str);
//                 }
//                 break;
//             case CGM_BGS:;
//                 ProcessingState* state = data_processor_create(new_tuple->value->cstring, ',');
//                 uint8_t num_strings = data_processor_count(state);
//                 num_bgs = num_strings;
//                 APP_LOG(APP_LOG_LEVEL_DEBUG, "BG num: %i", num_strings);
//                 bgs = (int*)malloc(num_strings*sizeof(int));
//                 for (uint8_t n = 0; n < num_strings; n += 1) {
//                     bgs[n] = data_processor_get_int(state);
//                     APP_LOG(APP_LOG_LEVEL_DEBUG, "BG Split: %i", bgs[n]);
//                 }			
//                 break;
//             case CGM_BG_TIMES:;
//                 ProcessingState* state_t = data_processor_create(new_tuple->value->cstring, ',');
//                 uint8_t num_strings_t = data_processor_count(state_t);
//                 APP_LOG(APP_LOG_LEVEL_DEBUG, "BG_t num: %i", num_strings_t);
//                 bg_times = (int*)malloc(num_strings_t*sizeof(int));
//                 for (uint8_t n = 0; n < num_strings_t; n += 1) {
//                     bg_times[n] = data_processor_get_int(state_t);
//                     APP_LOG(APP_LOG_LEVEL_DEBUG, "BG_t Split: %i", bg_times[n]);
//                 }			
//                 break;
//         }
        
//         // Get next pair, if any
//         new_tuple = dict_read_next(iterator);
//     }
    

//     // Redraw
//     if (s_canvas_layer) {
//         //load_chart_1();
//         time_t t = time(NULL);
//         struct tm * time_now = localtime(&t);
//         clock_refresh(time_now);
//         layer_mark_dirty(s_canvas_layer);
//         chart_layer_set_canvas_color(chart_layer, GColorClear);
//         chart_layer_set_margin(chart_layer, 7);
//         chart_layer_set_data(chart_layer, bg_times, eINT, bgs, eINT, num_bgs);
//     }
//     //Process Alerts
//     process_alert();
//     has_launched = 1;
//     com_alert = 1;
//     accel_tap_service_unsubscribe();
//     out_msg_inflight = false;

// }

// static void inbox_dropped_callback(AppMessageResult reason, void *context) {
//     VibePattern pat = {
//         .durations = error,
//         .num_segments = ARRAY_LENGTH(error),
//     };
    
//     if (com_alert == 1) {
//         vibes_enqueue_custom_pattern(pat);
//         com_alert = 0;
//     }
    
//     b_color_channels[0] = 255;
//     b_color_channels[1] = 0;
//     b_color_channels[2] = 0;
    
//     text_layer_set_text(time_layer, "in-com err");
//     layer_mark_dirty(s_canvas_layer);
//     out_msg_inflight = false;
// }

// static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
//     APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %i - %s", reason, translate_error(reason));
//     VibePattern pat = {
//         .durations = error,
//         .num_segments = ARRAY_LENGTH(error),
//     };
//     if (com_alert == 1) {
//         vibes_enqueue_custom_pattern(pat);
//         com_alert = 0;
//     }
    
//     b_color_channels[0] = 255;
//     b_color_channels[1] = 0;
//     b_color_channels[2] = 0;
//     if(time_layer)
//         text_layer_set_text_color(time_layer, GColorWhite);
//     text_layer_set_text(time_layer, "out-com err");
//     layer_mark_dirty(s_canvas_layer);
//     out_msg_inflight = false;
// }

// static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
//     APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
//     out_msg_inflight = true;
// }

// static void window_load(Window * window) {
    
//     Layer * window_layer = window_get_root_layer(window);
//     GRect window_bounds = layer_get_bounds(window_layer);
    
// #ifdef PBL_PLATFORM_BASALT
//     //s_center = grect_center_point(&window_bounds);
//     //int offset = 0;
//     int offset = 24/2;
//     GRect inner_bounds = GRect(0, 24, 144, 144);
//     s_center = grect_center_point(&inner_bounds);
// #else
//     int offset = 24/2;
//     GRect inner_bounds = GRect(0, 24, 144, 144);
//     s_center = grect_center_point(&inner_bounds);
// #endif
//     s_canvas_layer = layer_create(window_bounds);
//     layer_set_update_proc(s_canvas_layer, update_proc);
//     layer_add_child(window_layer, s_canvas_layer);
    
//     icon_layer = bitmap_layer_create(GRect(106 + 18, 50+offset, 30, 30));
//     bitmap_layer_set_background_color(icon_layer, GColorClear);
//     bitmap_layer_set_compositing_mode(icon_layer, GCompOpClear);
//     layer_add_child(s_canvas_layer, bitmap_layer_get_layer(icon_layer));

//     bg_layer = text_layer_create(GRect(4 + 20, 34+offset, 90, 75));
//     text_layer_set_text_color(bg_layer, GColorBlack);
//     text_layer_set_background_color(bg_layer, GColorClear);
//     text_layer_set_font(bg_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HN_BOLD_50)));
//     text_layer_set_text_alignment(bg_layer, GTextAlignmentRight);
//     layer_add_child(s_canvas_layer, text_layer_get_layer(bg_layer));
  
//     delta_layer = text_layer_create(GRect(61 + 20, 38, 72, 25));
//     text_layer_set_text_color(delta_layer, GColorBlack);
//     text_layer_set_background_color(delta_layer, GColorClear);
//     text_layer_set_font(delta_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
//     text_layer_set_text_alignment(delta_layer, GTextAlignmentCenter);
//     layer_add_child(s_canvas_layer, text_layer_get_layer(delta_layer));

//     time_delta_layer = text_layer_create(GRect(0 + 20, 38, 60, 25));
//     text_layer_set_text_color(time_delta_layer, GColorBlack);
//     text_layer_set_background_color(time_delta_layer, GColorClear);
//     text_layer_set_font(time_delta_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
//     text_layer_set_text_alignment(time_delta_layer, GTextAlignmentCenter);
//     layer_add_child(s_canvas_layer, text_layer_get_layer(time_delta_layer));
    
//     time_layer = text_layer_create(GRect(40, 0, 100, 40));
//     text_layer_set_text_color(time_layer, GColorWhite);
//     text_layer_set_background_color(time_layer, GColorClear);
// #ifdef PBL_PLATFORM_BASALT
//     text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
// #else
//     text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
// #endif    
    
//     text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
//     layer_add_child(s_canvas_layer, text_layer_get_layer(time_layer));
    
//     chart_layer = chart_layer_create((GRect) { 
//       	.origin = { 18, 102},
// 		.size = { 146, 45 } });
//   	chart_layer_set_plot_color(chart_layer, GColorWhite);
// #ifdef PBL_PLATFORM_BASALT
//   	chart_layer_set_canvas_color(chart_layer, GColorBlack);
// #else      
//     chart_layer_set_canvas_color(chart_layer, GColorBlack);
// #endif
//   	chart_layer_show_points_on_line(chart_layer, true);
// 	chart_layer_animate(chart_layer, false);
//   	layer_add_child(window_layer, chart_layer_get_layer(chart_layer));
//     //***********************//
	
// 	//test
// // 	text_layer_set_text(bg_layer, "999");
// // 	icon_bitmap = gbitmap_create_with_resource(CGM_ICONS[2]);
// //     bitmap_layer_set_bitmap(icon_layer, icon_bitmap);

	

// }

// static void window_unload(Window * window) {
//     layer_destroy(s_canvas_layer);
// }

// /*********************************** App **************************************/
// static int anim_percentage(AnimationProgress dist_normalized, int max) {
//     return (int)(float)(((float)dist_normalized / (float)ANIMATION_NORMALIZED_MAX) * (float)max);
// }

// static void radius_update(Animation * anim, AnimationProgress dist_normalized) {
//     s_radius = anim_percentage(dist_normalized, FINAL_RADIUS);   
//     layer_mark_dirty(s_canvas_layer);
// }

// static void hands_update(Animation * anim, AnimationProgress dist_normalized) {
//     s_anim_time.hours = anim_percentage(dist_normalized, hours_to_minutes(s_last_time.hours));
//     s_anim_time.minutes = anim_percentage(dist_normalized, s_last_time.minutes);  
//     layer_mark_dirty(s_canvas_layer);
// }

// static void init() {
//     //srand(time(NULL));    
//     time_t t = time(NULL);
//     struct tm * time_now = localtime(&t);
//     tick_handler(time_now, MINUTE_UNIT);
    
//     s_main_window = window_create();
//     window_set_window_handlers(s_main_window, (WindowHandlers) {
//         .load = window_load,
//         .unload = window_unload,
//     });
//     window_stack_push(s_main_window, true);
    
//     tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
//     accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
//     accel_tap_service_subscribe(accel_tap_handler);
    
//     // Prepare animations
//     AnimationImplementation radius_impl = {
//         .update = radius_update
//     };
//     animate(ANIMATION_DURATION, ANIMATION_DELAY, &radius_impl, false);
    
//     AnimationImplementation hands_impl = {
//         .update = hands_update
//     };
//     animate(2.5 * ANIMATION_DURATION, ANIMATION_DELAY, &hands_impl, true);
    
//     // Registering callbacks
//     app_message_register_inbox_received(inbox_received_callback);
//     app_message_register_inbox_dropped(inbox_dropped_callback);
//     app_message_register_outbox_failed(outbox_failed_callback);
//     app_message_register_outbox_sent(outbox_sent_callback);
 
//     app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
// }

// static void deinit() {
//     window_destroy(s_main_window);
//     persist_write_string(0, data_id);
// }

// int main() {
//     init();
//     app_event_loop();
//     deinit();
// }
