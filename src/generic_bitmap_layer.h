#pragma once

#include <pebble.h>

typedef struct GenericBitmapLayer GenericBitmapLayer;

GenericBitmapLayer* generic_bitmap_layer_create(GRect frame);

void generic_bitmap_layer_destroy(GenericBitmapLayer* gbl);

Layer* generic_bitmap_layer_get_layer(GenericBitmapLayer* gbl);

void generic_bitmap_layer_set_resource(GenericBitmapLayer* gbl, uint32_t resource_id);