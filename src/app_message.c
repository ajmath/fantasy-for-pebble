#include <pebble.h>
#include "strtok.h"
  
#define len(x)  (sizeof(x) / sizeof(x[0]))

#define SCORE_WINDOW 1
#define TEAM1_WINDOW 2
#define TEAM2_WINDOW 3
  
#define REQUEST_TYPE_MATCHUP 1

static char* TEAM_ID = "1095147";
static char* LEAGUE_ID = "158259";

// Key values for AppMessage Dictionary
enum {
  FANTASY_KEY_TEAM_ID = 0x0,
  FANTASY_KEY_LEAGUE_ID = 0x1,
  FANTASY_KEY_REQUEST_TYPE = 0x2,
  FANTASY_KEY_TEAM1_NAME = 0x3,
  FANTASY_KEY_TEAM2_NAME = 0x4,
  FANTASY_KEY_TEAM1_SCORE = 0x5,
  FANTASY_KEY_TEAM2_SCORE = 0x6,
  FANTASY_KEY_TEAM1_PLAYERS = 0x7,
  FANTASY_KEY_TEAM2_PLAYERS = 0x8
};

typedef struct player_struct {
  char* name;
  char* team;
  char* position;
  char* points;
} Player;

static uint8_t current_window = SCORE_WINDOW;

static Window *score_window;
static Window *team1_window;
static Window *team2_window;

static char* team1_name;
static char* team2_name;
static char* team1_score;
static char* team2_score;
static Player* team1_players;
static uint8_t num_players_team1;
static Player* team2_players;
static uint8_t num_players_team2;

static TextLayer *team1_name_layer;
static TextLayer *team1_score_layer;
static TextLayer *team2_name_layer;
static TextLayer *team2_score_layer;

// Write message to buffer & send
void request_data(uint8_t view){
  DictionaryIterator *iter;
  
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending over team_id = %s", TEAM_ID);
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, FANTASY_KEY_TEAM_ID, TEAM_ID);
  dict_write_cstring(iter, FANTASY_KEY_LEAGUE_ID, LEAGUE_ID);
  dict_write_uint8(iter, FANTASY_KEY_REQUEST_TYPE, REQUEST_TYPE_MATCHUP);
  dict_write_end(iter);
  app_message_outbox_send();
}

static char* get_cstr_from_dict(DictionaryIterator *dict, uint8_t key) {
  Tuple *tuple = dict_find(dict, key);
  if(tuple) {
    return tuple->value->cstring; 
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Text key %d not found in dictionary", key);
    return "??";
  }
}

int count_characters(const char *str, char character) {
    const char *p = str;
    int count = 0;

    do {
        if (*p == character)
            count++;
    } while (*(p++));

    return count;
}

static Player* parse_players(char* playersstr, uint8_t num_players) {
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "Tokenizing %s", playersstr);
  Player* players = malloc(num_players*sizeof(Player));
  char* playersstr_copy = malloc(strlen(playersstr));
  strcpy(playersstr_copy, playersstr);
  int i = 0;
  char* playerstr = strtok1(playersstr_copy, "|"); 
  while (playerstr) {
    char* playerstr_copy = malloc(strlen(playerstr));
    strcpy(playerstr_copy, playerstr);
    char* row_vals[4];
    int j = 0;
    char* row_val = strtok2(playerstr_copy, ",");
    while(row_val) {
      row_vals[j++] = row_val;
      row_val = strtok2(NULL, ",");
    }
    
    players[i++] = (Player){
      .name = row_vals[0],
      .position = row_vals[1],
      .team = row_vals[2],
      .points = row_vals[3]
    };
  
    playerstr = strtok1(NULL, "|");
  }
  return players;
}


static void update_matchup_data(DictionaryIterator *dict) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating matchup data from JS");
  
  team1_name = get_cstr_from_dict(dict, FANTASY_KEY_TEAM1_NAME);
  text_layer_set_text(team1_name_layer, team1_name);
  
  team2_name = get_cstr_from_dict(dict, FANTASY_KEY_TEAM2_NAME);
  text_layer_set_text(team2_name_layer, team2_name);  
  
  team1_score = get_cstr_from_dict(dict, FANTASY_KEY_TEAM1_SCORE);
  text_layer_set_text(team1_score_layer, team1_score);  
  
  team2_score = get_cstr_from_dict(dict, FANTASY_KEY_TEAM2_SCORE);
  text_layer_set_text(team2_score_layer, team2_score);
  
  char* team1_players_str = get_cstr_from_dict(dict, FANTASY_KEY_TEAM1_PLAYERS);
  num_players_team1 = count_characters(team1_players_str, '|') + 1;
  team1_players = parse_players(team1_players_str, num_players_team1);
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "Parsed %d players for team1", len(team1_players));
//   free(team1_players_str);
  
  char* team2_players_str = get_cstr_from_dict(dict, FANTASY_KEY_TEAM2_PLAYERS);
  num_players_team2 = count_characters(team2_players_str, '|') + 1;
  team2_players = parse_players(team2_players_str, num_players_team2);
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "Parsed %d players for team2", len(team2_players));
//   free(team2_players_str);
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *dict, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received message from JS");
  
  Tuple *tuple = dict_find(dict, FANTASY_KEY_REQUEST_TYPE);
  if(!tuple) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "No request type found, skipping data");
      return;
  }
  switch(tuple->value->uint8) {
    case REQUEST_TYPE_MATCHUP:
      update_matchup_data(dict);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved unknown request type %d", tuple->value->uint8);
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

static void add_new_text_layer(Layer *base, TextLayer *layer, char* text, char* font_key, 
                               GTextAlignment alignment, GColor background, GColor foreground) {
  text_layer_set_font(layer, fonts_get_system_font(font_key));
  text_layer_set_text_alignment(layer, alignment);
  text_layer_set_background_color(layer, background);
  text_layer_set_text_color(layer, foreground);
  if(text) {
    text_layer_set_text(layer, text);
  }
  layer_add_child(base, text_layer_get_layer(layer));
}

static void score_window_unload(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  layer_remove_child_layers(window_layer);

  text_layer_destroy(team1_name_layer);
  text_layer_destroy(team1_score_layer);
  text_layer_destroy(team2_name_layer);
  text_layer_destroy(team2_score_layer);
}

void next_window(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Advancing to next window");
  if(current_window == SCORE_WINDOW) {
    window_stack_push(team1_window, true);
    window_stack_remove(score_window, false);
  } else if(current_window == TEAM1_WINDOW) {
    window_stack_push(team2_window, true);
    window_stack_remove(team1_window, false);
  } else if(current_window == TEAM2_WINDOW) {
    window_stack_push(score_window, true);
    window_stack_remove(team2_window, false);
  }
}

void prev_window(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Advancing to previous window");
  if(current_window == SCORE_WINDOW) {
    window_stack_push(team2_window, true);
    window_stack_remove(score_window, false);
  } else if(current_window == TEAM1_WINDOW) {
    window_stack_push(score_window, true);
    window_stack_remove(team1_window, false);
  } else if(current_window == TEAM2_WINDOW) {
    window_stack_push(team1_window, true);
    window_stack_remove(team2_window, false);
  }
}

static void window_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler)prev_window);
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler)next_window);
}

static void score_window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading score window");
  current_window = SCORE_WINDOW;
  window_set_click_config_provider(window, (ClickConfigProvider) window_config_provider);

  Layer *root_layer = window_get_root_layer(window);
  GRect root_bounds = layer_get_bounds(root_layer);
  
  uint16_t score_height = 35;
  uint16_t score_width = root_bounds.size.w / 2;
  uint16_t name_height = (root_bounds.size.h - score_height) / 2;
  char* name_font = FONT_KEY_GOTHIC_24;
  char* score_font = FONT_KEY_GOTHIC_28_BOLD;

  team1_name_layer = text_layer_create(GRect(0, 0, root_bounds.size.w, name_height));
  add_new_text_layer(root_layer, team1_name_layer, team1_name, name_font, 
                     GTextAlignmentRight, GColorBlack, GColorWhite);

  team1_score_layer = text_layer_create(GRect(root_bounds.size.w / 2, name_height, score_width, score_height));
  add_new_text_layer(root_layer, team1_score_layer, team1_score, score_font, 
                     GTextAlignmentCenter, GColorBlack, GColorWhite);
  
  team2_name_layer = text_layer_create(GRect(0, name_height+score_height, root_bounds.size.w, name_height));
  add_new_text_layer(root_layer, team2_name_layer, team2_name, name_font, 
                     GTextAlignmentLeft, GColorWhite, GColorBlack);

  team2_score_layer = text_layer_create(GRect(0, name_height, score_width, score_height));
  add_new_text_layer(root_layer, team2_score_layer, team2_score, score_font, 
                     GTextAlignmentCenter, GColorWhite, GColorBlack);
}

static void team1_window_unload(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  layer_remove_child_layers(window_layer);
}

static void team_window_load(Player *players, int8_t num_players, Window* window) {
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "team_window_load - num_players = %d", num_players);
  Layer *root_layer = window_get_root_layer(window);
  GRect root_bounds = layer_get_bounds(root_layer);
  
  uint16_t row_height = (uint16_t)((float)root_bounds.size.h / (float)num_players);
  uint16_t name_width = root_bounds.size.w * 0.5;
  uint16_t pos_width = root_bounds.size.w * 0.15;
  uint16_t team_width = root_bounds.size.w * 0.15;
  uint16_t points_width = root_bounds.size.w * 0.2;
  
  for(int i = 0; i < num_players; i++) {
    Layer *player_layer = layer_create(GRect(0, i*row_height, root_bounds.size.w, row_height)); 
    
    Player player = players[i];
    
//     APP_LOG(APP_LOG_LEVEL_DEBUG, "Adding player %s", player.name);
    
    TextLayer* name_layer = text_layer_create(GRect(0, 0, name_width, row_height));
    text_layer_set_font(name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text(name_layer, player.name);
    text_layer_set_text_alignment(name_layer, GTextAlignmentLeft);
    layer_add_child(player_layer, text_layer_get_layer(name_layer));
    
    TextLayer* pos_layer = text_layer_create(GRect(name_width, 0, pos_width, row_height));
    text_layer_set_font(pos_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text(pos_layer, player.position);
    text_layer_set_text_alignment(pos_layer, GTextAlignmentCenter);
    layer_add_child(player_layer, text_layer_get_layer(pos_layer));
    
    TextLayer* team_layer = text_layer_create(GRect(name_width + pos_width, 0, team_width, row_height));
    text_layer_set_font(team_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text(team_layer, player.team);
    text_layer_set_text_alignment(team_layer, GTextAlignmentCenter);
    layer_add_child(player_layer, text_layer_get_layer(team_layer));
    
    TextLayer* points_layer = text_layer_create(GRect(name_width + pos_width + team_width, 0, points_width, row_height));
    text_layer_set_font(points_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text(points_layer, player.points);
    text_layer_set_text_alignment(points_layer, GTextAlignmentRight);
    layer_add_child(player_layer, text_layer_get_layer(points_layer));
    
    layer_add_child(root_layer, player_layer);
  }
}

static void team1_window_load(Window *window) {
  //Assume data has been retrieved before this window is loaded
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading team1 window");
  current_window = TEAM1_WINDOW;
  
  window_set_click_config_provider(window, (ClickConfigProvider) window_config_provider);
  
  team_window_load(team1_players, 9, window);
}

static void team2_window_unload(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  layer_remove_child_layers(window_layer);
}

static void team2_window_load(Window *window) { 
  //Assume data has been retrieved before this window is loaded
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading team2 window");
  current_window = TEAM2_WINDOW;
  
  window_set_click_config_provider(window, (ClickConfigProvider) window_config_provider);
  
  team_window_load(team2_players, 9, window);
}

static WindowHandlers score_window_handlers = {
  .load = score_window_load,
  .unload = score_window_unload
};
static WindowHandlers team1_window_handlers = {
  .load = team1_window_load,
  .unload = team1_window_unload
};
static WindowHandlers team2_window_handlers = {
  .load = team2_window_load,
  .unload = team2_window_unload
};

void init(void) {
  // Register AppMessage handlers
  app_message_register_inbox_received(in_received_handler); 
  app_message_register_inbox_dropped(in_dropped_handler); 
  app_message_register_outbox_failed(out_failed_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  request_data(REQUEST_TYPE_MATCHUP);
  
  score_window = window_create();
  window_set_window_handlers(score_window, score_window_handlers);

  team1_window = window_create();
  window_set_window_handlers(team1_window, team1_window_handlers);
  
  team2_window = window_create();
  window_set_window_handlers(team2_window, team2_window_handlers);
  
  window_stack_push(score_window, true);
}

void deinit(void) {
  app_message_deregister_callbacks();
  window_destroy(score_window);
  window_destroy(team1_window);
  window_destroy(team2_window);
}

int main( void ) {
  init();
  app_event_loop();
  deinit();
}