#include <pebble.h>

#include "generic_bitmap_layer.h"

static Window *window;
static GenericBitmapLayer *generic_layer;

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  generic_bitmap_layer_set_resource(generic_layer, RESOURCE_ID_APNG);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  generic_bitmap_layer_set_resource(generic_layer, RESOURCE_ID_PDCS);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  generic_bitmap_layer_set_resource(generic_layer, RESOURCE_ID_PDCI);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  generic_layer = generic_bitmap_layer_create(bounds);
  generic_bitmap_layer_set_resource(generic_layer, RESOURCE_ID_PNG);
  layer_add_child(window_layer, generic_bitmap_layer_get_layer(generic_layer));
}

static void window_unload(Window *window) {
  generic_bitmap_layer_destroy(generic_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
