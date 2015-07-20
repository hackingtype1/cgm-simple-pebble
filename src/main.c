#include <pebble.h>

#define ANTIALIASING true

#define HAND_MARGIN  - 1
#define FINAL_RADIUS 68

#define ANIMATION_DURATION 500
#define ANIMATION_DELAY    0

typedef struct {
    int hours;
    int minutes;
} Time;

static Window * s_main_window;
static Layer * s_canvas_layer;

static GPoint s_center;
static Time s_last_time, s_anim_time;
static int s_radius = 0, t_delta = 0, has_launched = 0, vibe_state = 1, alert_state = 0 ,s_anim_hours_60 = 0, com_alert = 1;
static bool s_animating = false;

static GBitmap *icon_bitmap = NULL;

static BitmapLayer * icon_layer;
static TextLayer * bg_layer, *delta_layer, *time_delta_layer, *time_layer, *date_layer;

static char date_app_text[] = "";
static char last_bg[124];
static char data_id[124];
static char time_delta_str[124] = "";
static char time_text[124] = "";

enum CgmKey {
    CGM_EGV_DELTA_KEY = 0x0,
    CGM_EGV_KEY = 0x1,
    CGM_TREND_KEY = 0x2,
    CGM_ALERT_KEY = 0x3,
    CGM_VIBE_KEY = 0x4,
    CGM_ID = 0x5,
    CGM_TIME_DELTA_KEY = 0x6,
};

enum Alerts {
    OKAY = 0x0,
    LOSS_MID_NO_NOISE = 0x1,
    LOSS_HIGH_NO_NOISE = 0x2,
    NO_CHANGE = 0x3,
    OLD_DATA = 0x4,
};

static int s_color_channels[3] = { 85, 85, 85 };
static int b_color_channels[3] = { 170, 170, 170 };

static const uint32_t const error[] = { 100,100,100,100,100 };

static const uint32_t CGM_ICONS[] = {
    RESOURCE_ID_IMAGE_NONE_WHITE,	  //4 - 0
    RESOURCE_ID_IMAGE_UPUP_WHITE,     //0 - 1
    RESOURCE_ID_IMAGE_UP_WHITE,       //1 - 2
    RESOURCE_ID_IMAGE_UP45_WHITE,     //2 - 3
    RESOURCE_ID_IMAGE_FLAT_WHITE,     //3 - 4
    RESOURCE_ID_IMAGE_DOWN45_WHITE,   //5 - 5
    RESOURCE_ID_IMAGE_DOWN_WHITE,     //6 - 6
    RESOURCE_ID_IMAGE_DOWNDOWN_WHITE,  //7 - 7
};

char *translate_error(AppMessageResult result) {
    switch (result) {
        case APP_MSG_OK: return "OK";
        case APP_MSG_SEND_TIMEOUT: return "Send Timeout";
        case APP_MSG_SEND_REJECTED: return "Send Rejected";
        case APP_MSG_NOT_CONNECTED: return "Not Connected";
        case APP_MSG_APP_NOT_RUNNING: return "App Not Up";
        case APP_MSG_INVALID_ARGS: return "App Invalid Args";
        case APP_MSG_BUSY: return "App Busy";
        case APP_MSG_BUFFER_OVERFLOW: return "App Overflow";
        case APP_MSG_ALREADY_RELEASED: return "App Msg Released";
        case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "Cback Registered";
        case APP_MSG_CALLBACK_NOT_REGISTERED: return "Cback Not Registered";
        case APP_MSG_OUT_OF_MEMORY: return "Out of Memory";
        case APP_MSG_CLOSED: return "Closed";
        case APP_MSG_INTERNAL_ERROR: return "Internal Error";
        default: return "Unknown Error";
    }
}

/*************************** AnimationImplementation **************************/
static void animation_started(Animation * anim, void *context) {
    s_animating = true;
}

static void animation_stopped(Animation * anim, bool stopped, void *context) {
    s_animating = false;
}

static void animate(int duration, int delay, AnimationImplementation * implementation, bool handlers) {
    Animation * anim = animation_create();
    animation_set_duration(anim, duration);
    animation_set_delay(anim, delay);
    animation_set_curve(anim, AnimationCurveEaseInOut);
    animation_set_implementation(anim, implementation);
    if (handlers) {
        animation_set_handlers(anim, (AnimationHandlers) {
            .started = animation_started,
            .stopped = animation_stopped
        }, NULL);
    }
    animation_schedule(anim);
}

static int hours_to_minutes(int hours_out_of_12) {
    return (int)(float)(((float)hours_out_of_12 / 12.0F) * 60.0F);
}

/************************************ UI **************************************/
void send_cmd(void) {
    
    if (s_canvas_layer) {
        gbitmap_destroy(icon_bitmap);
        text_layer_set_text(time_delta_layer, "checking...");
        text_layer_set_text(delta_layer, "");
        text_layer_set_text(bg_layer, "");
        icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_REFRESH_WHITE);
        bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
    }

    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    if (iter == NULL) {
        return;
    }
    
    static char *idptr = data_id;

    Tuplet idval = TupletCString(5, idptr);
    
    dict_write_tuplet(iter, &idval);
    dict_write_end(iter);
    
    app_message_outbox_send();
      
}

static void clock_refresh(struct tm * tick_time) {
     char *time_format;
// #ifdef PBL_PLATFORM_APLITE
 
    if (!tick_time) {
        time_t now = time(NULL);
        tick_time = localtime(&now);
    }
    
    if (clock_is_24h_style()) {
        time_format = "%H:%M  %m/%d";
    } else {
        time_format = "%I:%M  %m/%d";
    }
    
    strftime(time_text, sizeof(time_text), time_format, tick_time);
    
    if (time_text[0] == '0') {
        memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }
    
    if (s_canvas_layer) {
        layer_mark_dirty(s_canvas_layer);
        text_layer_set_text(time_layer, time_text);
    }
// #else
    
//     if (!tick_time) {
//         time_t now = time(NULL);
//         tick_time = localtime(&now);
//     }
    
//     time_format = "%B %e";
    
//     strftime(time_text, sizeof(time_text), time_format, tick_time);
    
//     if (time_text[0] == '0') {
//         memmove(time_text, &time_text[1], sizeof(time_text) - 1);
//     }
    
//     if (s_canvas_layer) {
//         layer_mark_dirty(s_canvas_layer);
//         text_layer_set_text(time_layer, time_text);
//     }
    

// #endif
    s_last_time.hours = tick_time->tm_hour;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "time hours 1: %d", s_last_time.hours);
    s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "time hours 2: %d", s_last_time.hours);
    s_last_time.minutes = tick_time->tm_min;
    
}

void accel_tap_handler(AccelAxisType axis, int32_t direction) {
    
    if (axis == ACCEL_AXIS_X)
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "axis: %s", "X");
        send_cmd();
    } else if (axis == ACCEL_AXIS_Y)
        
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "axis: %s", "Y");
        send_cmd();
    } else if (axis == ACCEL_AXIS_Z)
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "axis: %s", "Z");
        send_cmd();
    }
    
}

static void tick_handler(struct tm * tick_time, TimeUnits changed) {
    if (!has_launched) {                 
            snprintf(time_delta_str, 12, "loading");
            
            if (time_delta_layer) {
                text_layer_set_text(time_delta_layer, time_delta_str);
            }
    } else 
    
    if(t_delta > 3) {
        accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
        accel_tap_service_subscribe(accel_tap_handler);
        send_cmd();
    } else
    {   
        if (has_launched) {
            if (t_delta <= 0) {
                t_delta = 0;
                snprintf(time_delta_str, 12, "now"); // puts string into buffer
            } else {
                snprintf(time_delta_str, 12, "%dm ago", t_delta); // puts string into buffer
            }
            if (time_delta_layer) {
                text_layer_set_text(time_delta_layer, time_delta_str);
            }
        } else {

        }
    }
    t_delta++;  
    clock_refresh(tick_time);
    
}
static void update_proc(Layer * layer, GContext * ctx) {

#ifdef PBL_PLATFORM_BASALT

    graphics_context_set_fill_color(ctx, GColorFromRGB(b_color_channels[0], b_color_channels[1], b_color_channels[2]));
    graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);
    
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_stroke_width(ctx, 8);
    
    graphics_context_set_antialiased(ctx, ANTIALIASING);
    
    // White clockface
    graphics_context_set_fill_color(ctx, GColorFromRGB(s_color_channels[0], s_color_channels[1], s_color_channels[2]));
    graphics_fill_circle(ctx, s_center, s_radius);
    
    // Draw outline
    graphics_draw_circle(ctx, s_center, s_radius);
    
    graphics_context_set_stroke_color(ctx, GColorFromRGB(s_color_channels[0], s_color_channels[1], s_color_channels[2]));
    graphics_context_set_stroke_width(ctx, 2);
    
    graphics_context_set_antialiased(ctx, ANTIALIASING);
    
    // Don't use current time while animating
    Time mode_time = (s_animating) ? s_anim_time : s_last_time;
    
    // Adjust for minutes through the hour
    float minute_angle = TRIG_MAX_ANGLE * mode_time.minutes / 60;
    float hour_angle;
    if (s_animating) {
        // Hours out of 60 for smoothness
        hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 60;
    }
    else {
        hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 12;
    }
    hour_angle += (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);
    
    // Plot hands
    GPoint minute_hand = (GPoint) {
        .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * mode_time.minutes / 60) * (int32_t)(s_radius + 3) / TRIG_MAX_RATIO) + s_center.x,
        .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * mode_time.minutes / 60) * (int32_t)(s_radius + 3) / TRIG_MAX_RATIO) + s_center.y,
    };
    GPoint hour_hand = (GPoint) {
        .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)(s_radius - (2 * HAND_MARGIN)) / TRIG_MAX_RATIO) + s_center.x,
        .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(s_radius - (2 * HAND_MARGIN)) / TRIG_MAX_RATIO) + s_center.y,
    };
    
    // Draw hands with positive length only
    if (s_radius > 2 * HAND_MARGIN) {
        graphics_context_set_stroke_width(ctx, 8);
        graphics_draw_line(ctx, s_center, hour_hand);
    }
    if (s_radius > HAND_MARGIN) {
        graphics_context_set_stroke_width(ctx, 4);
        graphics_draw_line(ctx, s_center, minute_hand);
    }
#else
    //Change Background
    if (b_color_channels[0] == 170) {
        //OKAY
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);
        if(time_layer)
            text_layer_set_text_color(time_layer, GColorBlack);
    } else {
        //BAD COMMS
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);
        if(time_layer)
            text_layer_set_text_color(time_layer, GColorWhite);
    }
    // Change clockface
    if (s_color_channels[0] < 255){
        //OKAY
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_context_set_fill_color(ctx, GColorClear);
        
    } else {
        //NOT OKAY
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_fill_color(ctx, GColorBlack);
    }
    graphics_fill_circle(ctx, s_center, s_radius - 2);
    graphics_draw_circle(ctx, s_center, s_radius - 2);
#endif
    
}

static void reset_background() {
    
    b_color_channels[0] = 170;
    b_color_channels[1] = 170;
    b_color_channels[2] = 170;
    if(time_layer)
        text_layer_set_text_color(time_layer, GColorBlack);
    layer_mark_dirty(s_canvas_layer);
}

static void process_alert() {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Vibe State: %i", vibe_state);
    switch (alert_state) {
        
    case LOSS_MID_NO_NOISE:;    
        s_color_channels[0] = 255;
        s_color_channels[1] = 255;
        s_color_channels[2] = 0;
        
        if (vibe_state > 0)
            vibes_long_pulse();
            
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Alert key: %i", LOSS_MID_NO_NOISE);
#ifdef PBL_PLATFORM_BASALT
        text_layer_set_text_color(bg_layer, GColorBlack);
        text_layer_set_text_color(delta_layer, GColorBlack);
        text_layer_set_text_color(time_delta_layer, GColorBlack);
        bitmap_layer_set_compositing_mode(icon_layer, GCompOpClear);
#else
        text_layer_set_text_color(bg_layer, GColorWhite);
        text_layer_set_text_color(delta_layer, GColorWhite);
        text_layer_set_text_color(time_delta_layer, GColorWhite);
        bitmap_layer_set_compositing_mode(icon_layer, GCompOpOr);
#endif                        

        break;
        
    case LOSS_HIGH_NO_NOISE:;
        s_color_channels[0] = 255;
        s_color_channels[1] = 0;
        s_color_channels[2] = 0;
        
       if (vibe_state > 0)
            vibes_long_pulse();
        
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Alert key: %i", LOSS_HIGH_NO_NOISE);
        text_layer_set_text_color(bg_layer, GColorWhite);
        text_layer_set_text_color(delta_layer, GColorWhite);
        text_layer_set_text_color(time_delta_layer, GColorWhite);
        bitmap_layer_set_compositing_mode(icon_layer, GCompOpOr);
        break;
        
    case OKAY:;
        s_color_channels[0] = 0;
        s_color_channels[1] = 255;
        s_color_channels[2] = 0;
        
        if (vibe_state > 1)
            vibes_double_pulse();
        
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Alert key: %i", OKAY);
        text_layer_set_text_color(bg_layer, GColorBlack);
        text_layer_set_text_color(delta_layer, GColorBlack);
        text_layer_set_text_color(time_delta_layer, GColorBlack);
        bitmap_layer_set_compositing_mode(icon_layer, GCompOpClear);
        break;
        
    case OLD_DATA:;
        VibePattern pat = {
            .durations = error,
            .num_segments = ARRAY_LENGTH(error),
        };
        
        vibes_enqueue_custom_pattern(pat);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Alert key: %i", OLD_DATA);
        
        s_color_channels[0] = 0;
        s_color_channels[1] = 255;
        s_color_channels[2] = 255;
#ifdef PBL_PLATFORM_BASALT
        text_layer_set_text_color(time_delta_layer, GColorRed);
#else
#endif
        text_layer_set_text_color(bg_layer, GColorBlack);
        text_layer_set_text_color(delta_layer, GColorBlack);
        
        bitmap_layer_set_compositing_mode(icon_layer, GCompOpOr);
        break;
     
     case NO_CHANGE:;
        break;   
    }
        
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
    text_layer_set_text(time_delta_layer, "checking...");
    // Get the first pair
    Tuple *new_tuple = dict_read_first(iterator);
    reset_background();
    
    // Process all pairs present
    while(new_tuple != NULL) {
        // Process this pair's key
        
        switch (new_tuple->key) {
                
            case CGM_ID:;
                strncpy(data_id, new_tuple->value->cstring, 124);
                break;
                
            case CGM_EGV_DELTA_KEY:;
                text_layer_set_text(delta_layer, new_tuple->value->cstring);
                break;
                
            case CGM_EGV_KEY:;
                text_layer_set_text(bg_layer, new_tuple->value->cstring);
                strncpy(last_bg, new_tuple->value->cstring, 124);
                break;
                
            case CGM_TREND_KEY:;
                if (icon_bitmap) {
                    gbitmap_destroy(icon_bitmap);
                }
                icon_bitmap = gbitmap_create_with_resource(CGM_ICONS[new_tuple->value->uint8]);
                bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
                break;
                
            case CGM_ALERT_KEY:;
                alert_state = new_tuple->value->uint8;
                break;
                
            case CGM_VIBE_KEY:
                vibe_state = new_tuple->value->int16;             
                break;
                
            case CGM_TIME_DELTA_KEY:;
                t_delta = new_tuple->value->int16;
                if (t_delta <= 0) {
                t_delta = 0;
                    snprintf(time_delta_str, 12, "now"); // puts string into buffer
                } else {
                    snprintf(time_delta_str, 12, "%dm ago", t_delta); // puts string into buffer
                }
                if (time_delta_layer) {
                    text_layer_set_text(time_delta_layer, time_delta_str);
                }
                break;
                
        }
        
        // Get next pair, if any
        new_tuple = dict_read_next(iterator);
    }
    // Redraw
    if (s_canvas_layer) {
        time_t t = time(NULL);
        struct tm * time_now = localtime(&t);
        clock_refresh(time_now);
        layer_mark_dirty(s_canvas_layer);
    }
    //Process Alerts
    process_alert();
    has_launched = 1;
    com_alert = 1;
    accel_tap_service_unsubscribe();

}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    VibePattern pat = {
        .durations = error,
        .num_segments = ARRAY_LENGTH(error),
    };
    
    if (com_alert == 1) {
        vibes_enqueue_custom_pattern(pat);
        com_alert = 0;
    }
    
    b_color_channels[0] = 255;
    b_color_channels[1] = 0;
    b_color_channels[2] = 0;
    
    text_layer_set_text(time_layer, "comm error");
    layer_mark_dirty(s_canvas_layer);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %i - %s", reason, translate_error(reason));
    VibePattern pat = {
        .durations = error,
        .num_segments = ARRAY_LENGTH(error),
    };
    if (com_alert == 1) {
        vibes_enqueue_custom_pattern(pat);
        com_alert = 0;
    }
    
    b_color_channels[0] = 255;
    b_color_channels[1] = 0;
    b_color_channels[2] = 0;
    if(time_layer)
        text_layer_set_text_color(time_layer, GColorWhite);
    text_layer_set_text(time_layer, "comm error");
    layer_mark_dirty(s_canvas_layer);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void window_load(Window * window) {
    
    Layer * window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
#ifdef PBL_PLATFORM_BASALT
    //s_center = grect_center_point(&window_bounds);
    //int offset = 0;
    int offset = 24/2;
    GRect inner_bounds = GRect(0, 24, 144, 144);
    s_center = grect_center_point(&inner_bounds);
#else
    int offset = 24/2;
    GRect inner_bounds = GRect(0, 24, 144, 144);
    s_center = grect_center_point(&inner_bounds);
#endif
    s_canvas_layer = layer_create(window_bounds);
    layer_set_update_proc(s_canvas_layer, update_proc);
    layer_add_child(window_layer, s_canvas_layer);
    
    icon_layer = bitmap_layer_create(GRect(102, 67+offset, 30, 30));
    bitmap_layer_set_background_color(icon_layer, GColorClear);
    bitmap_layer_set_compositing_mode(icon_layer, GCompOpClear);
    layer_add_child(s_canvas_layer, bitmap_layer_get_layer(icon_layer));

    bg_layer = text_layer_create(GRect(5, 42+offset, 104, 76));
    text_layer_set_text_color(bg_layer, GColorBlack);
    text_layer_set_background_color(bg_layer, GColorClear);
    text_layer_set_font(bg_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CALIBRI_60)));
    text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
    layer_add_child(s_canvas_layer, text_layer_get_layer(bg_layer));
  
    delta_layer = text_layer_create(GRect(26, 111+offset, 90, 48));
    text_layer_set_text_color(delta_layer, GColorBlack);
    text_layer_set_background_color(delta_layer, GColorClear);
    text_layer_set_font(delta_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CALIBRI_19)));
    text_layer_set_text_alignment(delta_layer, GTextAlignmentCenter);
    layer_add_child(s_canvas_layer, text_layer_get_layer(delta_layer));

    time_delta_layer = text_layer_create(GRect(26, 30+offset, 90, 48));
    text_layer_set_text_color(time_delta_layer, GColorBlack);
    text_layer_set_background_color(time_delta_layer, GColorClear);
    text_layer_set_font(time_delta_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CALIBRI_19)));
    text_layer_set_text_alignment(time_delta_layer, GTextAlignmentCenter);
    layer_add_child(s_canvas_layer, text_layer_get_layer(time_delta_layer));
    
    time_layer = text_layer_create(GRect(0, 0, 144, 26));
    text_layer_set_text_color(time_layer, GColorBlack);
    text_layer_set_background_color(time_layer, GColorClear);
#ifdef PBL_PLATFORM_BASALT
    text_layer_set_font(time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CALIBRI_19)));
#else
    text_layer_set_font(time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CALIBRI_BOLD_24)));
#endif    
    
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    layer_add_child(s_canvas_layer, text_layer_get_layer(time_layer));
    //***********************//

}

static void window_unload(Window * window) {
    layer_destroy(s_canvas_layer);
}

/*********************************** App **************************************/
static int anim_percentage(AnimationProgress dist_normalized, int max) {
    return (int)(float)(((float)dist_normalized / (float)ANIMATION_NORMALIZED_MAX) * (float)max);
}

static void radius_update(Animation * anim, AnimationProgress dist_normalized) {
    s_radius = anim_percentage(dist_normalized, FINAL_RADIUS);   
    layer_mark_dirty(s_canvas_layer);
}

static void hands_update(Animation * anim, AnimationProgress dist_normalized) {
    s_anim_time.hours = anim_percentage(dist_normalized, hours_to_minutes(s_last_time.hours));
    s_anim_time.minutes = anim_percentage(dist_normalized, s_last_time.minutes);  
    layer_mark_dirty(s_canvas_layer);
}

static void init() {
    //srand(time(NULL));    
    time_t t = time(NULL);
    struct tm * time_now = localtime(&t);
    tick_handler(time_now, MINUTE_UNIT);
    
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(s_main_window, true);
    
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
    accel_tap_service_subscribe(accel_tap_handler);
    
    // Prepare animations
    AnimationImplementation radius_impl = {
        .update = radius_update
    };
    animate(ANIMATION_DURATION, ANIMATION_DELAY, &radius_impl, false);
    
    AnimationImplementation hands_impl = {
        .update = hands_update
    };
    animate(2.5 * ANIMATION_DURATION, ANIMATION_DELAY, &hands_impl, true);
    
    // Registering callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
 
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
    window_destroy(s_main_window);
    persist_write_string(0, data_id);
}

int main() {
    init();
    app_event_loop();
    deinit();
}
