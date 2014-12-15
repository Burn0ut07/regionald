#include <pebble.h>

#include "strings.h"

#define btos(x) ((x)?"true":"false")

static Window* window;
static MenuLayer* menu_layer;

enum {
    REGION_LIST,
    REGION_UPDATE,
    REGION_SELECTED,
    REGION_LIST_BEGIN,
    SERVICE_ERROR
};

#define MAX_REGIONS (19)

typedef struct {
    bool selected;
    String short_name;
    String full_name;
} RegionItem;

static RegionItem region_list_items[MAX_REGIONS];
static int region_count = -1;
static bool fetching_regions = true;
static char info_cell_message[100] = "Fetching regions...";

bool send_region_update(const char* const new_region) {
    DictionaryIterator *iter;

    app_message_outbox_begin(&iter);
    dict_write_cstring(iter, REGION_UPDATE, new_region);

    dict_write_end(iter);
    AppMessageResult result = app_message_outbox_send();
    if (result == APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Message ok!");
        return true;
    }
    if (result == APP_MSG_BUSY) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Message busy!");
    }
    return false;
}

static void region_list_update(String unparsed_region_list, bool append) {
    if (!append) {
        // If we are not appending then this is a new region list. so reset count;
        region_count = -1;
    }
    String* parts = string_split(unparsed_region_list, ',');

    bool set_short_name = true;
    // region_count is set to last valid region index, so start adding regions in next index
    int index = region_count + 1; 
    for(String* part_ref = parts; *part_ref != NULL && index < MAX_REGIONS; part_ref++) {
        if (set_short_name) {
            String short_name = region_list_items[index].short_name;
            if (short_name != NULL) {
                free(short_name);
            }
            region_list_items[index].short_name = *part_ref;
            set_short_name = false;
        } else {
            String full_name = region_list_items[index].full_name;
            if (full_name != NULL) {
                free(full_name);
            }
            region_list_items[index].full_name = *part_ref;
            index++;
            region_count++;
            set_short_name = true;
        }
    }

    free(parts);
}

static void region_selected_update(String selected_region_short_name) {
    for(int index = 0; index <= region_count; index++) {
        if (strncmp(region_list_items[index].short_name, selected_region_short_name, 2) == 0) {
            region_list_items[index].selected = true;
        } else {
            region_list_items[index].selected = false;
        }
    }
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Receive handler");
    Tuple *region_list_tuple = dict_find(received, REGION_LIST);
    Tuple *region_selected_tuple = dict_find(received, REGION_SELECTED);
    Tuple *region_list_begin = dict_find(received, REGION_LIST_BEGIN);
    Tuple *service_error_tuple = dict_find(received, SERVICE_ERROR);

    if (service_error_tuple) {
        strncpy(info_cell_message, service_error_tuple->value->cstring, 100);
        menu_layer_reload_data(menu_layer);
    }
    if (region_list_tuple) {
        fetching_regions = true;
        region_list_update(region_list_tuple->value->cstring, region_list_begin == NULL);
    }
    if (region_selected_tuple) {
        fetching_regions = false;
        region_selected_update(region_selected_tuple->value->cstring);
        menu_layer_reload_data(menu_layer);
    }
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Message dropped! Reason: %s", app_message_enum_to_string(reason));
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Message not sent! Reason: %d", app_message_enum_to_string(reason));
}

static int16_t get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Regions");
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    return 44;
}

static void draw_row_callback(GContext* ctx, Layer *cell_layer, MenuIndex *cell_index, void *data) {
    if (fetching_regions) {
        menu_cell_basic_draw(ctx, cell_layer, info_cell_message, NULL, NULL);
        return;
    }

    const int index = cell_index->row;
    RegionItem item = region_list_items[index];    

    if (!item.selected) {
        menu_cell_basic_draw(ctx, cell_layer, item.full_name, NULL, NULL);
    } else {
        menu_cell_basic_draw(ctx, cell_layer, item.full_name, "selected", NULL);
    }
}

static uint16_t get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return fetching_regions ? 1 : region_count + 1;
}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    if (fetching_regions) {
        return;
    }

    const int index = cell_index->row;
    DictionaryIterator *iter;

    if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
        return;
    }
    if (dict_write_cstring(iter, REGION_UPDATE, region_list_items[index].short_name) != DICT_OK) {
        return;
    }
    app_message_outbox_send();
}

static void window_load(Window* window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect window_frame = layer_get_frame(window_layer);
    menu_layer = menu_layer_create(window_frame);
    menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
        .get_header_height = (MenuLayerGetHeaderHeightCallback) get_header_height_callback,
        .draw_header = (MenuLayerDrawHeaderCallback) draw_header_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback) get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback) draw_row_callback,
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback) get_num_rows_callback,
        .select_click = (MenuLayerSelectCallback) select_callback
    });
    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

void init(void) {
    for(int index = 0; index < MAX_REGIONS; index++) {
        region_list_items[index].short_name = NULL;
        region_list_items[index].full_name = NULL;
        region_list_items[index].selected = false;
    }

    window = window_create();

    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
    });
    window_stack_push(window, true);

    // Register AppMessage handlers
    app_message_register_inbox_received(in_received_handler); 
    app_message_register_inbox_dropped(in_dropped_handler); 
    app_message_register_outbox_failed(out_failed_handler);

    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void deinit(void) {
    app_message_deregister_callbacks();
    menu_layer_destroy(menu_layer);
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
