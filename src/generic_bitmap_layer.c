#include <pebble.h>

#include "generic_bitmap_layer.h"

struct GenericBitmapLayer {
	Layer 	*layer;
	
	GBitmap 				*bitmap;

#ifdef PBL_SDK_3
	GBitmapSequence 		*sequence;

	GDrawCommandImage 		*command_image;
	
	GDrawCommandSequence 	*command_sequence;
	GDrawCommandFrame 		*frame;
	int 					frame_index;

	AppTimer 		*animation_timer;
#endif
};

static const uint8_t PNG_SIG[8]  = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
static const uint8_t ACTL_SIG[4] = {'a','c','T','L'};
static const uint8_t PDCI_SIG[4] = {'P','D','C','I'};
static const uint8_t PDCS_SIG[4] = {'P','D','C','S'};

static GRect grect_center_rect(const GRect *rect_a, const GRect *rect_b) {
  return (GRect) {
    .origin = {
      .x = rect_a->origin.x + (rect_a->size.w - rect_b->size.w) / 2,
      .y = rect_a->origin.y + (rect_a->size.h - rect_b->size.h) / 2,
    },
    .size = rect_b->size,
  };
}

static void layer_update_proc(Layer *layer, GContext *ctx) {
	GenericBitmapLayer *self = *((GenericBitmapLayer**)layer_get_data(layer));
	GRect bounds = layer_get_bounds(layer);

	if(self->bitmap) {
		GRect bitmap_bounds = gbitmap_get_bounds(self->bitmap);
#ifdef PBL_COLOR
		graphics_context_set_compositing_mode(ctx,GCompOpSet);
#endif
		bounds.origin = GPointZero;
		graphics_draw_bitmap_in_rect(ctx, 
			self->bitmap, 
			grect_center_rect(&bounds, &bitmap_bounds));
  	}
#ifdef PBL_SDK_3
  	else if(self->command_image){
  		GSize img_size = gdraw_command_image_get_bounds_size(self->command_image);
  		gdraw_command_image_draw(ctx, 
  			self->command_image, 
  			GPoint((bounds.size.w - img_size.w) / 2, (bounds.size.h - img_size.h) / 2));
  	}
	else if(self->frame){
		GSize img_size = gdraw_command_sequence_get_bounds_size(self->command_sequence);
		gdraw_command_frame_draw(ctx, 
			self->command_sequence, 
			self->frame, 
			GPoint((bounds.size.w - img_size.w) / 2, (bounds.size.h - img_size.h) / 2));
	}
#endif
}

#ifdef PBL_SDK_3
static void animation_timer_handler(void *context) {
  	GenericBitmapLayer* self = (GenericBitmapLayer*)context;
	self->animation_timer = NULL;
	if(self->sequence){
		uint32_t next_delay;
		if(gbitmap_sequence_update_bitmap_next_frame(self->sequence, self->bitmap, &next_delay)) {
	 	 	layer_mark_dirty(self->layer);
	  		self->animation_timer = app_timer_register(next_delay, animation_timer_handler, context);
	  	}
	}
	else if(self->command_sequence){
		self->frame = gdraw_command_sequence_get_frame_by_index(self->command_sequence, self->frame_index);
		if(self->frame){
			int num_frames = gdraw_command_sequence_get_num_frames(self->command_sequence);
  			self->frame_index++;
  			if (self->frame_index == num_frames) {
    			self->frame_index = 0;
  			}
  			layer_mark_dirty(self->layer);
  			self->animation_timer = app_timer_register(gdraw_command_frame_get_duration(self->frame), animation_timer_handler, context);
		}
	} 
}
#endif

static void cleanup(GenericBitmapLayer* gbl) {
	if(gbl->bitmap)
		gbitmap_destroy(gbl->bitmap);
	gbl->bitmap = NULL;

#ifdef PBL_SDK_3
	if(gbl->sequence)
		gbitmap_sequence_destroy(gbl->sequence);
	gbl->sequence = NULL;

	if(gbl->command_image)
		gdraw_command_image_destroy(gbl->command_image);
	gbl->command_image = NULL;

	if(gbl->command_sequence)
		gdraw_command_sequence_destroy(gbl->command_sequence);
	gbl->command_sequence = NULL;

	if(gbl->animation_timer)
		app_timer_cancel(gbl->animation_timer);
	gbl->animation_timer = NULL;
#endif
}

void generic_bitmap_layer_set_resource(GenericBitmapLayer* gbl, uint32_t animation_id){
	if(gbl){

		cleanup(gbl);
		
		ResHandle rh = resource_get_handle(animation_id);
		if(resource_size(rh) < 8)
			return;

#ifdef PBL_SDK_3
		uint8_t buffer[8];
		resource_load_byte_range(rh, 0, buffer, 8);
		if(memcmp(buffer, PNG_SIG, 8) == 0){
			resource_load_byte_range(rh, 8 + (4 + 4 + 0x0D + 4) + 4, buffer, 4);
			if(memcmp(buffer, ACTL_SIG, 4) == 0){
				gbl->sequence = gbitmap_sequence_create_with_resource(animation_id);
				gbitmap_sequence_set_play_count(gbl->sequence, PLAY_COUNT_INFINITE);
				gbl->bitmap = gbitmap_create_blank(gbitmap_sequence_get_bitmap_size(gbl->sequence), GBitmapFormat8Bit);
				gbl->animation_timer = app_timer_register(1, animation_timer_handler, gbl);
			}
			else {
				gbl->bitmap = gbitmap_create_with_resource(animation_id);
			}
		}
		else if(memcmp(buffer, PDCI_SIG, 4) == 0){
			gbl->command_image = gdraw_command_image_create_with_resource(animation_id);
		}
		else if(memcmp(buffer, PDCS_SIG, 4) == 0){
			gbl->command_sequence = gdraw_command_sequence_create_with_resource(animation_id);
			gbl->animation_timer = app_timer_register(1, animation_timer_handler, gbl);
		}
		else
#endif 
		{
			gbl->bitmap = gbitmap_create_with_resource(animation_id);
		}

		layer_mark_dirty(gbl->layer);
	}
}

Layer* generic_bitmap_layer_get_layer(GenericBitmapLayer* gbl){
	return gbl ? gbl->layer : NULL;
}

GenericBitmapLayer* generic_bitmap_layer_create(GRect frame){
	GenericBitmapLayer* self = malloc(sizeof(GenericBitmapLayer));
	memset(self, 0, sizeof(GenericBitmapLayer));

	self->layer = layer_create_with_data(frame, sizeof(GenericBitmapLayer **));
  	*(GenericBitmapLayer **) layer_get_data(self->layer) = self;
	layer_set_update_proc(self->layer, layer_update_proc);

	return self;
}

void generic_bitmap_layer_destroy(GenericBitmapLayer* gbl){
	if(gbl){
		layer_destroy(gbl->layer);
		cleanup(gbl);
		free(gbl);
	}
}