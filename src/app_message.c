#include <pebble.h>
  
#define SCORE_WINDOW 1
#define TEAM1_WINDOW 2
#define TEAM2_WINDOW 3

static char* TEAM_ID = "1095147";
static char* LEAGUE_ID = "158259";

// Key values for AppMessage Dictionary
enum {
  FANTASY_KEY_TEAM_ID = 0x0,	
  FANTASY_KEY_LEAGUE_ID = 0x1,
  FANTASY_KEY_WINDOW = 0x2,
  FANTASY_KEY_TEAM1_NAME = 0x3,
  FANTASY_KEY_TEAM2_NAME = 0x4,
  FANTASY_KEY_TEAM1_SCORE = 0x5,
  FANTASY_KEY_TEAM2_SCORE = 0x6,
  FANTASY_KEY_NAME = 0x7,
  FANTASY_KEY_SCORE = 0x8,
  FANTASY_KEY_PLAYERS = 0x9
};

static Window *score_window;
// static Window *team1_window;
// static Window *team2_window;

static TextLayer *team1_name_layer;
static TextLayer *team1_score_layer;
static TextLayer *team2_name_layer;
static TextLayer *team2_score_layer;

// Write message to buffer & send
void request_data(uint8_t view){
  DictionaryIterator *iter;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending over team_id = %s", TEAM_ID);
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, FANTASY_KEY_TEAM_ID, TEAM_ID);
  dict_write_cstring(iter, FANTASY_KEY_LEAGUE_ID, LEAGUE_ID);
  dict_write_uint8(iter, FANTASY_KEY_WINDOW, view);
  dict_write_end(iter);
  app_message_outbox_send();
}
static void set_text_layer_from_dict(DictionaryIterator *dict, uint8_t key, TextLayer *text_layer) {
  Tuple *tuple = dict_find(dict, key);
  if(tuple) {
    text_layer_set_text(text_layer, tuple->value->cstring);  
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Text key %d not found in dictionary", key);
  }
}

static void update_score_window_data(DictionaryIterator *dict) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating score window from JS");
  set_text_layer_from_dict(dict, FANTASY_KEY_TEAM1_NAME, team1_name_layer);
  set_text_layer_from_dict(dict, FANTASY_KEY_TEAM1_SCORE, team1_score_layer);
  set_text_layer_from_dict(dict, FANTASY_KEY_TEAM2_NAME, team2_name_layer);
  set_text_layer_from_dict(dict, FANTASY_KEY_TEAM2_SCORE, team2_score_layer);
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *dict, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received message from JS");
  
  Tuple *tuple = dict_find(dict, FANTASY_KEY_WINDOW);
  if(!tuple) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "No window key found, skipping data");
      return;
  }
  switch(tuple->value->uint8) {
    case SCORE_WINDOW:
      update_score_window_data(dict);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved unknown window key %d", tuple->value->uint8);
  }
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Dropped message from JS");
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send message to JS");
}

static void score_window_unload(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  layer_remove_child_layers(window_layer);

  text_layer_destroy(team1_name_layer);
  text_layer_destroy(team1_score_layer);
  text_layer_destroy(team2_name_layer);
  text_layer_destroy(team2_score_layer);
}

static void score_window_load(Window *window) {
//   window_set_click_config_provider(window, (ClickConfigProvider) font_window_config_provider);

  Layer *root_layer = window_get_root_layer(window);

  GRect root_bounds = layer_get_bounds(root_layer);
  
  uint16_t score_height = 35;
  uint16_t score_width = root_bounds.size.w / 2;
  uint16_t name_height = (root_bounds.size.h - score_height) / 2;

  // Team1 name
  team1_name_layer = text_layer_create(GRect(0, 0, root_bounds.size.w, name_height));
  text_layer_set_font(team1_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
//   text_layer_set_text(team1_name_layer, "Something about your mom");
  text_layer_set_text_alignment(team1_name_layer, GTextAlignmentRight);
  text_layer_set_background_color(team1_name_layer, GColorBlack);
  text_layer_set_text_color(team1_name_layer, GColorWhite);
  layer_add_child(root_layer, text_layer_get_layer(team1_name_layer));

  // Team1 score
  team1_score_layer = text_layer_create(GRect(root_bounds.size.w / 2, name_height, score_width, score_height));
  text_layer_set_font(team1_score_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
//   text_layer_set_text(team1_score_layer, "120.92");
  text_layer_set_text_alignment(team1_score_layer, GTextAlignmentCenter);
  text_layer_set_background_color(team1_score_layer, GColorBlack);
  text_layer_set_text_color(team1_score_layer, GColorWhite);
  layer_add_child(root_layer, text_layer_get_layer(team1_score_layer));
  
  // Team2 name
  team2_name_layer = text_layer_create(GRect(0, name_height+score_height, root_bounds.size.w, name_height));
  text_layer_set_font(team2_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
//   text_layer_set_text(team2_name_layer, "Hemingway squandered half his life hanging around Picasso, trying to nail his leftovers");
  text_layer_set_text_alignment(team2_name_layer, GTextAlignmentLeft);
  text_layer_set_background_color(team2_name_layer, GColorWhite);
  text_layer_set_text_color(team2_name_layer, GColorBlack);
  layer_add_child(root_layer, text_layer_get_layer(team2_name_layer));

  // Team2 score
  team2_score_layer = text_layer_create(GRect(0, name_height, score_width, score_height));
  text_layer_set_font(team2_score_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
//   text_layer_set_text(team2_score_layer, "93.33");
  text_layer_set_background_color(team2_score_layer, GColorWhite);
  text_layer_set_text_alignment(team2_score_layer, GTextAlignmentCenter);
  text_layer_set_text_color(team2_score_layer, GColorBlack);
  layer_add_child(root_layer, text_layer_get_layer(team2_score_layer));
  
 request_data(SCORE_WINDOW);
}

static WindowHandlers score_window_handlers = {
  .load = score_window_load,
  .unload = score_window_unload
};

void init(void) {
  // Register AppMessage handlers
  app_message_register_inbox_received(in_received_handler); 
  app_message_register_inbox_dropped(in_dropped_handler); 
  app_message_register_outbox_failed(out_failed_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  score_window = window_create();
  window_set_window_handlers(score_window, score_window_handlers);
  window_stack_push(score_window, true);
}

void deinit(void) {
  app_message_deregister_callbacks();
  window_destroy(score_window);
//   window_destroy(team1_window);
//   window_destroy(team2_window);
}

int main( void ) {
  init();
  app_event_loop();
  deinit();
}