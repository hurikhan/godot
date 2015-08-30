/*************************************************************************/
/*  os_wayland.cpp                                                       */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2015 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "servers/visual/visual_server_raster.h"
#include "drivers/gles2/rasterizer_gles2.h"
#include "os_wayland.h"
#include <stdio.h>
#include <stdlib.h>
#include "print_string.h"
#include "servers/physics/physics_server_sw.h"
#include "main/main.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

//                 _     _                _ _     _                       
//  _ __ ___  __ _(_)___| |_ _ __ _   _  | (_)___| |_ ___ _ __   ___ _ __ 
// | '__/ _ \/ _` | / __| __| '__| | | | | | / __| __/ _ \ '_ \ / _ \ '__|
// | | |  __/ (_| | \__ \ |_| |  | |_| | | | \__ \ ||  __/ | | |  __/ |   
// |_|  \___|\__, |_|___/\__|_|   \__, | |_|_|___/\__\___|_| |_|\___|_|   
//           |___/                |___/                                   

const struct wl_registry_listener OS_Wayland::registry_listener = {
	OS_Wayland::registry_handler,
	OS_Wayland::registry_remover
};


void OS_Wayland::registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {

	OS_Wayland *that = static_cast<OS_Wayland *>(data);
	
	print_line(String("Wayland -- Registry Event -- ") + interface + " ID: " + itos( id ) + " Version: " + itos( version ));

	if (strcmp(interface, "wl_compositor") == 0) 
		that->compositor =  static_cast<struct wl_compositor *> ( wl_registry_bind(registry, id, &wl_compositor_interface, 1) );

	if (strcmp(interface, "wl_shell") == 0) 
		that->shell = static_cast<wl_shell *> ( wl_registry_bind(registry, id, &wl_shell_interface, 1) );

	if (strcmp(interface, "wl_seat") == 0) 
		that->seat = static_cast<wl_seat *> ( wl_registry_bind(registry, id, &wl_seat_interface, 4) );

	if (strcmp(interface, "wl_output") == 0) {
		OS_Wayland::output_data_t output_data;
		output_data.output = static_cast<wl_output *> ( wl_registry_bind( registry, id, &wl_output_interface, 2 ) );
		that->output_data.push_back( output_data );
	}

	if (strcmp(interface, "wl_shm") == 0) {
		that->shm = static_cast<wl_shm *> ( wl_registry_bind(registry, id, &wl_shm_interface, 1) );
		pointer_init_cursor_theme( that );
	}
}


void OS_Wayland::registry_remover(void *data, struct wl_registry *registry, uint32_t id) {
}


//      _          _ _                   __                  _ _     _                       
//  ___| |__   ___| | |  ___ _   _ _ __ / _| __ _  ___ ___  | (_)___| |_ ___ _ __   ___ _ __ 
// / __| '_ \ / _ \ | | / __| | | | '__| |_ / _` |/ __/ _ \ | | / __| __/ _ \ '_ \ / _ \ '__|
// \__ \ | | |  __/ | | \__ \ |_| | |  |  _| (_| | (_|  __/ | | \__ \ ||  __/ | | |  __/ |   
// |___/_| |_|\___|_|_| |___/\__,_|_|  |_|  \__,_|\___\___| |_|_|___/\__\___|_| |_|\___|_|   
                                                                                          
const struct wl_shell_surface_listener OS_Wayland::shell_surface_listener = {
	shell_surface_ping,
	shell_surface_configure,
	shell_surface_popup
};


void OS_Wayland::shell_surface_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial) {
	OS_Wayland *that = static_cast<OS_Wayland *>(data);
	wl_shell_surface_pong( that->shell_surface, serial );
}


void OS_Wayland::shell_surface_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {
	//print_line(String("Wayland -- Shell Surface -- " + itos(edges) + " " + itos(width) + " " + itos(height)));
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	if( width <= 0 && height <= 0 )
		return;

	if( that->current_videomode.resizable == false )
		return;

	that->current_videomode.width = width;
	that->current_videomode.height = height;

	that->context_gl->resize( width, height );
}


void OS_Wayland::shell_surface_popup(void *data, struct wl_shell_surface *shell_surface) {
}


//                 _     _ _     _                       
//  ___  ___  __ _| |_  | (_)___| |_ ___ _ __   ___ _ __ 
// / __|/ _ \/ _` | __| | | / __| __/ _ \ '_ \ / _ \ '__|
// \__ \  __/ (_| | |_  | | \__ \ ||  __/ | | |  __/ |   
// |___/\___|\__,_|\__| |_|_|___/\__\___|_| |_|\___|_|   

const struct wl_seat_listener OS_Wayland::seat_listener = {
	seat_handle_capabilities,
	seat_handle_name
};


void OS_Wayland::seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t caps) {
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	if (caps & WL_SEAT_CAPABILITY_POINTER && !that->pointer) {
		print_line("Wayland -- Seat -- Pointer found");
		that->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(that->pointer, &pointer_listener, that);
	} else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && that->pointer) {
		wl_pointer_destroy(that->pointer);
		that->pointer = NULL;
		print_line("Wayland -- Seat -- Pointer destroyed");
	}

	if (caps & WL_SEAT_CAPABILITY_KEYBOARD && !that->keyboard) {
		print_line("Wayland -- Seat -- Keyboard found");
		that->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(that->keyboard, &keyboard_listener, that);
	} else if(!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && that->keyboard) {
		wl_keyboard_destroy(that->keyboard);
		that->keyboard = NULL;
		print_line("Wayland -- Seat -- Keyboard destroyed");
	}

	if (caps & WL_SEAT_CAPABILITY_TOUCH) {
		print_line("Wayland -- Seat -- Touch found");
	}
}


void OS_Wayland::seat_handle_name(void *data, struct wl_seat *seat, const char *name) {
	print_line(String("Wayland -- Seat -- Name: ") + name );
}


//             _       _              _ _     _                       
//  _ __   ___ (_)_ __ | |_ ___ _ __  | (_)___| |_ ___ _ __   ___ _ __ 
// | '_ \ / _ \| | '_ \| __/ _ \ '__| | | / __| __/ _ \ '_ \ / _ \ '__|
// | |_) | (_) | | | | | ||  __/ |    | | \__ \ ||  __/ | | |  __/ |   
// | .__/ \___/|_|_| |_|\__\___|_|    |_|_|___/\__\___|_| |_|\___|_|   
// |_|                                                                 

const struct wl_pointer_listener OS_Wayland::pointer_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis	
};


void OS_Wayland::pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	print_line("Wayland -- Pointer -- Enter");
	
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	struct wl_buffer *buffer;
	struct wl_cursor_image *image;

	image = that->pointer_data.cursors[0]->images[0];
	buffer = wl_cursor_image_get_buffer( image );
	struct wl_surface *cursor_surface = that->pointer_data.cursor_surface;
	
	wl_pointer_set_cursor( pointer, serial, cursor_surface, image->hotspot_x, image->hotspot_y );
	wl_surface_attach( cursor_surface, buffer, 0, 0 );
	wl_surface_damage( cursor_surface, 0, 0, image->width, image->height );
	wl_surface_commit( cursor_surface );
}


void OS_Wayland::pointer_handle_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface) {
	print_line("Wayland -- Pointer -- Leave");
}


void OS_Wayland::pointer_handle_motion(void *data, struct wl_pointer *pointer, uint32_t serial, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	Point2i pos;
	pos.x = wl_fixed_to_int( surface_x );
	pos.y = wl_fixed_to_int( surface_y );
	that->input->set_mouse_pos( pos );

	Point2i speed;
	speed = that->input->get_mouse_speed();

	Point2i rel;
	rel = pos - that->pointer_data.pos;

	InputEvent event;
	event.ID = ++that->event_id;
	event.type = InputEvent::MOUSE_MOTION;
	event.device = 0;				// MSC: Check what that means!
	event.mouse_motion.mod = that->keyboard_data.modifiers;
	event.mouse_motion.button_mask = that->pointer_data.button_mask;
	event.mouse_motion.x = pos.x;
	event.mouse_motion.y = pos.y;
	event.mouse_motion.global_x = pos.x;
	event.mouse_motion.global_y = pos.y;
	event.mouse_motion.speed_x = speed.x;
	event.mouse_motion.speed_y = speed.y;
	event.mouse_motion.relative_x = rel.x;
	event.mouse_motion.relative_y = rel.y;
	that->input->parse_input_event( event );

	that->pointer_data.pos = pos;
	that->pointer_data.rel = rel;
	that->pointer_data.speed= speed;
}


void OS_Wayland::pointer_handle_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	int _button = pointer_get_button( button );
	if( _button == 0 ) {
		print_line(String( "Wayland -- Pointer -- Button " + itos(button) + " is not supported"));
		return;
	}

	int button_mask = that->pointer_data.button_mask;
	if( state ) {
		switch( _button ) {
			case BUTTON_LEFT:	button_mask |= BUTTON_MASK_LEFT; break;
			case BUTTON_RIGHT:	button_mask |= BUTTON_MASK_RIGHT; break;
			case BUTTON_MIDDLE:	button_mask |= BUTTON_MASK_MIDDLE; break;
		}
	} else {
		switch( _button ) {
			case BUTTON_LEFT:	button_mask &= ~BUTTON_MASK_LEFT; break;
			case BUTTON_RIGHT:	button_mask &= ~BUTTON_MASK_RIGHT; break;
			case BUTTON_MIDDLE:	button_mask &= ~BUTTON_MASK_MIDDLE; break;
		}
	}

	bool double_click = false;
	if( state && (_button == BUTTON_LEFT) ){
		uint64_t click_time = that->get_ticks_usec() / 1000;
		uint64_t diff = click_time - that->pointer_data.last_click_time;
		Point2i pos1 = that->pointer_data.last_click_pos;
		Point2i pos2 = that->pointer_data.pos; 
		int distance = Point2(pos1).distance_to( pos2 );

		if( diff < 400 && distance < 5 )
			double_click = true;


		that->pointer_data.last_click_pos = that->pointer_data.pos;
		that->pointer_data.last_click_time = click_time;
	}
		

	InputEvent event;
	event.ID = ++that->event_id;
	event.type = InputEvent::MOUSE_BUTTON;
	event.device = 0;
	event.mouse_button.mod = that->keyboard_data.modifiers;
	event.mouse_button.button_mask = button_mask;
	event.mouse_button.x = that->pointer_data.pos.x;
	event.mouse_button.y = that->pointer_data.pos.y;
	event.mouse_button.global_x = that->pointer_data.pos.x;
	event.mouse_button.global_y = that->pointer_data.pos.y;
	event.mouse_button.button_index = _button;
	event.mouse_button.pressed = state;
	event.mouse_button.doubleclick = double_click;

	that->input->parse_input_event( event );

	that->pointer_data.button_mask = button_mask;
}


void OS_Wayland::pointer_handle_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
	//print_line(String( "Wayland -- Pointer -- Axis " + itos(axis) + " " + itos(wl_fixed_to_int(value))));
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	if( axis != 0 ) {
		print_line(String( "Wayland -- Pointer -- Axis " + itos(axis) + " is not supported"));
		return;
	}

	int _button = pointer_get_axis_direction( value );

	InputEvent event;
	event.ID = ++that->event_id;
	event.type = InputEvent::MOUSE_BUTTON;
	event.device = 0;
	event.mouse_button.mod = that->keyboard_data.modifiers;
	event.mouse_button.button_mask = that->pointer_data.button_mask;
	event.mouse_button.x = that->pointer_data.pos.x;
	event.mouse_button.y = that->pointer_data.pos.y;
	event.mouse_button.global_x = that->pointer_data.pos.x;
	event.mouse_button.global_y = that->pointer_data.pos.y;
	event.mouse_button.button_index = _button;
	event.mouse_button.pressed = true;

	that->input->parse_input_event( event );

	event.ID = ++that->event_id;
	event.mouse_button.pressed = false;
	that->input->parse_input_event( event );
}


int OS_Wayland::pointer_get_button( uint32_t button ) {

	switch(button) {
		case WL_BUTTON_LEFT:	return BUTTON_LEFT;
		case WL_BUTTON_RIGHT:	return BUTTON_RIGHT;
		case WL_BUTTON_MIDDLE:	return BUTTON_MIDDLE;
	}

	return 0;	
}


int OS_Wayland::pointer_get_axis_direction( wl_fixed_t value ) {
	int _value = wl_fixed_to_int( value );

	if( _value > 0 )
		return BUTTON_WHEEL_DOWN;
	else if( _value < 0 )
		return BUTTON_WHEEL_UP;

	return 0;
}


void OS_Wayland::pointer_init_cursor_theme( OS_Wayland *that ) {
	that->pointer_data.cursor_theme = wl_cursor_theme_load( NULL, 32, that->shm );
	that->pointer_data.cursor_surface = wl_compositor_create_surface( that->compositor );

	for(int i=0;i<CURSOR_MAX;i++) {

		static const char *cursor_file[]={
			"left_ptr",
			"xterm",
			"hand2",
			"cross",
			"watch",
			"left_ptr_watch",
			"fleur",
			"hand1",
			"X_cursor",
			"sb_v_double_arrow",
			"sb_h_double_arrow",
			"size_bdiag",
			"size_fdiag",
			"hand1",
			"sb_v_double_arrow",
			"sb_h_double_arrow",
			"question_arrow"
		};
		that->pointer_data.cursors[i] = wl_cursor_theme_get_cursor( that->pointer_data.cursor_theme, cursor_file[i] );
	}

	ERR_FAIL_COND( that->pointer_data.cursor_theme == NULL );
	ERR_FAIL_COND( that->pointer_data.cursors[0] == NULL );
	ERR_FAIL_COND( that->pointer_data.cursor_surface == NULL);
}

//  _              _                         _   _ _     _                       
// | | _____ _   _| |__   ___   __ _ _ __ __| | | (_)___| |_ ___ _ __   ___ _ __ 
// | |/ / _ \ | | | '_ \ / _ \ / _` | '__/ _` | | | / __| __/ _ \ '_ \ / _ \ '__|
// |   <  __/ |_| | |_) | (_) | (_| | | | (_| | | | \__ \ ||  __/ | | |  __/ |   
// |_|\_\___|\__, |_.__/ \___/ \__,_|_|  \__,_| |_|_|___/\__\___|_| |_|\___|_|   
//           |___/                                                               

const struct wl_keyboard_listener OS_Wayland::keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers,
	keyboard_handle_repeat_info
};


void OS_Wayland::keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	char* mapStr;

	ERR_FAIL_COND( format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 )

	mapStr = static_cast<char*> (mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0));
	ERR_FAIL_COND( mapStr == MAP_FAILED )

	that->keyboard_data.keymap = xkb_map_new_from_string( that->keyboard_data.context, mapStr, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS );
	ERR_FAIL_COND( that->keyboard_data.keymap == NULL )

	that->keyboard_data.state = xkb_state_new( that->keyboard_data.keymap );
	ERR_FAIL_COND( that->keyboard_data.state == NULL )

	print_line("Wayland -- Keyboard -- Keymap initialized");
}


void OS_Wayland::keyboard_handle_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
}


void OS_Wayland::keyboard_handle_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
}


void OS_Wayland::keyboard_handle_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	uint32_t scancode =  keyboard_get_scancode( that, key );

	switch( scancode ) {
		case KEY_SHIFT:		that->keyboard_data.modifiers.shift = state; break;
		case KEY_CONTROL:	that->keyboard_data.modifiers.control = state; break;
		case KEY_ALT:		that->keyboard_data.modifiers.alt = state; break;
		case KEY_META:		that->keyboard_data.modifiers.meta = state; break;

		default:			that->keyboard_data.repeat_key = key;
							that->keyboard_data.repeat_time = that->get_ticks_usec() / 1000 + that->keyboard_data.repeat_delay;
	}

	InputEvent event;
	event.ID = ++that->event_id;
	event.type = InputEvent::KEY;
	event.device = 0;
	event.key.mod = that->keyboard_data.modifiers;
	event.key.pressed = state;
	event.key.echo = false;
	event.key.scancode = scancode;
	event.key.unicode = keyboard_get_unicode( that, key );

	/*
	print_line( String( "Key: " + itos(key) ) );
	print_line( String( "Scancode: " + itos(event.key.scancode) ) );
	print_line( String( "Unicode: " + itos(event.key.unicode) ) );

	char buffer[64];
	int i = xkb_keysym_get_name( xkb_state_key_get_one_sym( that->keyboard_data.state, key + 8), buffer, 64);
	print_line( String( itos(i) + " " + buffer ) );
	*/

	that->input->parse_input_event( event );

	if(!state) {
		that->keyboard_data.repeat_key = 0;
		that->keyboard_data.repeat_time = 0;
	}

	//print_line(itos(that->keyboard_data.repeat_key));
	//print_line(itos(that->keyboard_data.repeat_time));
}


void OS_Wayland::keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	OS_Wayland *that = static_cast<OS_Wayland *>(data);
	xkb_state_update_mask(that->keyboard_data.state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}


void OS_Wayland::keyboard_handle_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay) {
	print_line(String("Wayland -- Keyboard -- Key repeat info --> Rate: " + itos(rate) + " Delay: " + itos(delay)) );

	OS_Wayland *that = static_cast<OS_Wayland *>(data);
	that->keyboard_data.repeat_rate = rate;
	that->keyboard_data.repeat_delay = delay;
}


struct _TranslatePair {
	xkb_keysym_t keysym;
	uint32_t keycode;
};


static _TranslatePair _keysym_to_keycode[]={
	{	XKB_KEY_Escape,				KEY_ESCAPE	},
	{	XKB_KEY_Tab,				KEY_TAB },
	{	XKB_KEY_ISO_Left_Tab,		KEY_BACKTAB },
	{	XKB_KEY_BackSpace,			KEY_BACKSPACE },
	{	XKB_KEY_Return,				KEY_RETURN },
	{	XKB_KEY_Insert,				KEY_INSERT },
	{	XKB_KEY_Delete,				KEY_DELETE },
	{	XKB_KEY_Pause,				KEY_PAUSE },
	{	XKB_KEY_Print,				KEY_PRINT },
	{	XKB_KEY_Sys_Req,			KEY_SYSREQ },
	{	XKB_KEY_Clear,				KEY_CLEAR },
	{	XKB_KEY_Home,				KEY_HOME },
	{	XKB_KEY_End,				KEY_END },
	{	XKB_KEY_Left,				KEY_LEFT },
	{	XKB_KEY_Up,					KEY_UP },
	{	XKB_KEY_Right,				KEY_RIGHT },
	{	XKB_KEY_Down,				KEY_DOWN },
	{	XKB_KEY_Page_Up,			KEY_PAGEUP },
	{	XKB_KEY_Page_Down,			KEY_PAGEDOWN },
	{	XKB_KEY_Shift_L,			KEY_SHIFT },
	{	XKB_KEY_Shift_R,			KEY_SHIFT },
	{	XKB_KEY_Control_L,			KEY_CONTROL },
	{	XKB_KEY_Control_R,			KEY_CONTROL },
	{	XKB_KEY_Meta_L,				KEY_META },
	{	XKB_KEY_Meta_R,				KEY_META },
	{	XKB_KEY_Alt_L,				KEY_ALT },
	{	XKB_KEY_Alt_R,				KEY_ALT },
	{	XKB_KEY_Caps_Lock,			KEY_CAPSLOCK },
	{	XKB_KEY_Num_Lock,			KEY_NUMLOCK },
	{	XKB_KEY_Scroll_Lock,		KEY_SCROLLLOCK },
	{	XKB_KEY_F1,					KEY_F1 },
	{	XKB_KEY_F2,					KEY_F2 },
	{	XKB_KEY_F3,					KEY_F3 },
	{	XKB_KEY_F4,					KEY_F4 },
	{	XKB_KEY_F5,					KEY_F5 },
	{	XKB_KEY_F6,					KEY_F6 },
	{	XKB_KEY_F7,					KEY_F7 },
	{	XKB_KEY_F8,					KEY_F8 },
	{	XKB_KEY_F9,					KEY_F9 },
	{	XKB_KEY_F10,				KEY_F10 },
	{	XKB_KEY_F11,				KEY_F11 },
	{	XKB_KEY_F12,				KEY_F12 },
	{	XKB_KEY_F13,				KEY_F13 },
	{	XKB_KEY_F14,				KEY_F14 },
	{	XKB_KEY_F15,				KEY_F15 },
	{	XKB_KEY_F16,				KEY_F16 },
	{	XKB_KEY_KP_Enter,			KEY_KP_ENTER },
	{	XKB_KEY_KP_Multiply,		KEY_KP_MULTIPLY },
	{	XKB_KEY_KP_Divide,			KEY_KP_DIVIDE },
	{	XKB_KEY_KP_Subtract,		KEY_KP_SUBSTRACT },
	{	XKB_KEY_KP_Add,				KEY_KP_ADD },
	{	XKB_KEY_KP_Decimal,			KEY_KP_PERIOD },
	{	XKB_KEY_KP_0,				KEY_KP_0 },
	{	XKB_KEY_KP_1,				KEY_KP_1 },
	{	XKB_KEY_KP_2,				KEY_KP_2 },
	{	XKB_KEY_KP_3,				KEY_KP_3 },
	{	XKB_KEY_KP_4,				KEY_KP_4 },
	{	XKB_KEY_KP_5,				KEY_KP_5 },
	{	XKB_KEY_KP_6,				KEY_KP_6 },
	{	XKB_KEY_KP_7,				KEY_KP_7 },
	{	XKB_KEY_KP_8,				KEY_KP_8 },
	{	XKB_KEY_KP_9,				KEY_KP_9 },
	{	XKB_KEY_Super_L,			KEY_SUPER_L },
	{	XKB_KEY_Super_R,			KEY_SUPER_R },
	{	XKB_KEY_Menu,				KEY_MENU },
	{	XKB_KEY_Hyper_L,			KEY_HYPER_L },
	{	XKB_KEY_Hyper_R,			KEY_HYPER_R },
	{	XKB_KEY_Help,				KEY_HELP },
	{	0,							0 }
};


uint32_t OS_Wayland::keyboard_get_scancode( OS_Wayland *that, uint32_t key ) {

	uint32_t keysym = xkb_state_key_get_one_sym( that->keyboard_data.state, key + 8);

	for( int index = 0; _keysym_to_keycode[index].keysym != 0; index++ ) {
		if( _keysym_to_keycode[index].keysym == keysym) {
			return _keysym_to_keycode[index].keycode;
		}
	}
	return key + 8;
}


uint32_t OS_Wayland::keyboard_get_unicode( OS_Wayland *that, uint32_t key ) {
	return xkb_state_key_get_utf32( that->keyboard_data.state, key + 8 );
}


void OS_Wayland::keyboard_repeat_key( OS_Wayland *that ) {

	if( that->keyboard_data.repeat_key == 0)
		return;

	if( that->keyboard_data.repeat_time <= that->get_ticks_usec() / 1000 ) {

		InputEvent event;
		event.ID = ++that->event_id;
		event.type = InputEvent::KEY;
		event.device = 0;
		event.key.mod = that->keyboard_data.modifiers;
		event.key.pressed = true;
		event.key.echo = true;
		event.key.scancode = keyboard_get_scancode( that, that->keyboard_data.repeat_key );
		event.key.unicode = keyboard_get_unicode( that, that->keyboard_data.repeat_key );

		that->input->parse_input_event( event );

		that->keyboard_data.repeat_time = (that->get_ticks_usec() / 1000) + (1000 / that->keyboard_data.repeat_rate);
	}
}

//             _               _     _ _     _
//  ___  _   _| |_ _ __  _   _| |_  | (_)___| |_ ___ _ __   ___ _ __
// / _ \| | | | __| '_ \| | | | __| | | / __| __/ _ \ '_ \ / _ \ '__|
// |(_) | |_| | |_| |_) | |_| | |_  | | \__ \ ||  __/ | | |  __/ |
// \___/ \__,_|\__| .__/ \__,_|\__| |_|_|___/\__\___|_| |_|\___|_|
//                |_|

const struct wl_output_listener OS_Wayland::output_listener = {
	output_handle_geometry,
	output_handle_mode,
	output_handle_done,
	output_handle_scale
};


void OS_Wayland::output_handle_geometry(void *data, wl_output *output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make, const char *model, int32_t transform) {
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	for( int i=0; i < that->output_data.size(); i++)
		if( that->output_data[i].output == output ) {
			that->output_data[i].x = x;
			that->output_data[i].y = y;
			that->output_data[i].physical_width = physical_width;
			that->output_data[i].physical_height = physical_height;
			that->output_data[i].subpixel = subpixel;
			that->output_data[i].make = make;
			that->output_data[i].model = model;
			that->output_data[i].transform = transform;
		}

	print_line(String("Wayland -- Output -- Geometry +- x: ") + itos(x) + " y: " + itos(y));
	print_line(String("                              +- width: ") + itos(physical_width) + "mm height: " + itos(physical_height) + "mm");
	print_line(String("                              +- subpixel: ") + itos(subpixel) );
	print_line(String("                              +- make: ") + make);
	print_line(String("                              +- model: ") + model);
	print_line(String("                              +- transform: ") + itos(transform) );
}


void OS_Wayland::output_handle_mode(void *data, wl_output *output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
	
	OS_Wayland *that = static_cast<OS_Wayland *>(data);

	for( int i=0; i < that->output_data.size(); i++)
		if( that->output_data[i].output == output ) {
			that->output_data[i].flags = flags;
			that->output_data[i].width = width;
			that->output_data[i].height = height;
			that->output_data[i].refresh = refresh;
		}

	print_line(String("Wayland -- Output -- Mode -- ") + itos(width) + "*" + itos(height) + "@" + itos(refresh) + "mHz");
}


void OS_Wayland::output_handle_done(void *data, wl_output *output) {
	print_line("Wayland -- Output -- Done");
}


void OS_Wayland::output_handle_scale(void *data, wl_output *output, int32_t factor) {
	print_line(String("Wayland -- Output -- Scale -- scale: ") + itos(factor) );
}


// ============================================================================================================================================================================

void OS_Wayland::initialize(const VideoMode& p_desired,int p_video_driver,int p_audio_driver) {
	
	if (get_render_thread_mode()==RENDER_SEPARATE_THREAD) {
		print_line("Not implemented...\n"); 	// FIXME
		exit(1);
	}

	keyboard_data.context = xkb_context_new( XKB_CONTEXT_NO_FLAGS );
	ERR_FAIL_COND( keyboard_data.context == NULL )
	print_line("Wayland -- Keyboard Context created");

	display = wl_display_connect( NULL );
	ERR_FAIL_COND( display == NULL );
	print_line("Wayland -- Display connected");

	registry = wl_display_get_registry( display );
	ERR_FAIL_COND( registry == NULL );
	print_line("Wayland -- Get registry");

	wl_registry_add_listener( registry, &registry_listener, this );
	wl_display_roundtrip( display );
	ERR_FAIL_COND( compositor == NULL);
	ERR_FAIL_COND( shell == NULL );
	ERR_FAIL_COND( seat == NULL );
	ERR_FAIL_COND( output_data.size() == 0 );
	ERR_FAIL_COND( shm == NULL );

	surface = wl_compositor_create_surface( compositor );
	ERR_FAIL_COND( surface == NULL );
	print_line("Wayland -- Surface created");

	shell_surface = wl_shell_get_shell_surface(shell, surface);
	ERR_FAIL_COND(shell_surface == NULL);
	print_line("Wayland -- Shell Surface created");

	wl_shell_surface_set_toplevel(shell_surface);
	wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, this);
	wl_seat_add_listener(seat, &seat_listener, this);
	
	for( int i = 0; i < output_data.size(); i++ )
		wl_output_add_listener( output_data[i].output, &output_listener, this );


	context_gl = memnew( ContextGL_Wayland( display, surface, current_videomode, false ) );
	context_gl->initialize();

#if defined(OPENGL_ENABLED) || defined(LEGACYGL_ENABLED)
	rasterizer = memnew( RasterizerGLES2 );
#endif

	visual_server = memnew( VisualServerRaster( rasterizer ) );
	visual_server->init();

	AudioDriverManagerSW::get_driver(p_audio_driver)->set_singleton();

	audio_driver_index=p_audio_driver;
	if (AudioDriverManagerSW::get_driver(p_audio_driver)->init()!=OK) {

		bool success=false;
		audio_driver_index=-1;
		for(int i=0;i<AudioDriverManagerSW::get_driver_count();i++) {
			if (i==p_audio_driver)
				continue;
			if (AudioDriverManagerSW::get_driver(i)->init()==OK) {
				success=true;
				print_line("Audio Driver Failed: "+String(AudioDriverManagerSW::get_driver(p_audio_driver)->get_name()));
				print_line("Using alternate audio driver: "+String(AudioDriverManagerSW::get_driver(i)->get_name()));
				audio_driver_index=i;
				break;
			}
		}
		if (!success) {
			ERR_PRINT("Initializing audio failed.");
		}

	}

	sample_manager = memnew( SampleManagerMallocSW );
	audio_server = memnew( AudioServerSW(sample_manager) );
	audio_server->init();
	spatial_sound_server = memnew( SpatialSoundServerSW );
	spatial_sound_server->init();
	spatial_sound_2d_server = memnew( SpatialSound2DServerSW );
	spatial_sound_2d_server->init();

	physics_server = memnew( PhysicsServerSW );
	physics_server->init();
	physics_2d_server = memnew( Physics2DServerSW );
	//physics_2d_server = Physics2DServerWrapMT::init_server<Physics2DServerSW>();
	physics_2d_server->init();

	input = memnew( InputDefault );
}


void OS_Wayland::finalize() {

	delete_main_loop();

	spatial_sound_server->finish();
	memdelete(spatial_sound_server);

	spatial_sound_2d_server->finish();
	memdelete(spatial_sound_2d_server);

	audio_server->finish();
	memdelete(audio_server);
	memdelete(sample_manager);

	visual_server->finish();
	memdelete(visual_server);
	memdelete(rasterizer);

	physics_server->finish();
	memdelete(physics_server);

	physics_2d_server->finish();
	memdelete(physics_2d_server);

	memdelete(input);

#if defined(OPENGL_ENABLED) || defined(LEGACYGL_ENABLED)
	memdelete(context_gl);
#endif

	wl_display_disconnect(display);

	xkb_state_unref( keyboard_data.state );
	xkb_keymap_unref( keyboard_data.keymap );
	xkb_context_unref( keyboard_data.context );
}


int OS_Wayland::get_video_driver_count() const {
	return 1;
}


const char * OS_Wayland::get_video_driver_name(int p_driver) const {
	return "GLES2";
}


OS::VideoMode OS_Wayland::get_default_video_mode() const {
	return OS::VideoMode(800,600,false);
}


int OS_Wayland::get_audio_driver_count() const {

    return AudioDriverManagerSW::get_driver_count();
}


const char *OS_Wayland::get_audio_driver_name(int p_driver) const {

    AudioDriverSW* driver = AudioDriverManagerSW::get_driver(p_driver);
    ERR_FAIL_COND_V( !driver, "" );
    return AudioDriverManagerSW::get_driver(p_driver)->get_name();
}


void OS_Wayland::set_mouse_mode(MouseMode p_mode) {
	;	// FIXME
}


void OS_Wayland::warp_mouse_pos(const Point2& p_to) {
	;	// FIXME
}


OS::MouseMode OS_Wayland::get_mouse_mode() const {
	return mouse_mode;
}


int OS_Wayland::get_mouse_button_state() const {
	return 0;	// FIXME -- deprecated?
}


Point2 OS_Wayland::get_mouse_pos() const {
	return Point2i(0, 0);	// FIXME -- deprecated?
}


void OS_Wayland::set_window_title(const String& p_title) {
	;	// FIXME
}


void OS_Wayland::set_video_mode(const VideoMode& p_video_mode,int p_screen) {
	;	// FIXME -- deprecated?
}


OS::VideoMode OS_Wayland::get_video_mode(int p_screen) const {
	return current_videomode;
}


void OS_Wayland::get_fullscreen_mode_list(List<VideoMode> *p_list,int p_screen) const {
	;	// FIXME -- deprecated?
}


int OS_Wayland::get_screen_count() const {
	return output_data.size(); 
}


int OS_Wayland::get_current_screen() const {
	WARN_PRINT("Window positioning is unknown for a Wayland client");
	return 0;
}


void OS_Wayland::set_current_screen(int p_screen) {
	WARN_PRINT("Window positioning is unknown for a Wayland client");
}


Point2 OS_Wayland::get_screen_position(int p_screen) const {
	if( p_screen >= output_data.size() || p_screen < 0 )
		return Point2i(0,0);

	int32_t x = output_data[p_screen].x;
	int32_t y = output_data[p_screen].y;

	Point2i position = Point2i(x, y);
	return position;
}


Size2 OS_Wayland::get_screen_size(int p_screen) const {
	if( p_screen >= output_data.size() || p_screen < 0 )
		return Point2i(0,0);

	int32_t width = output_data[p_screen].width;
	int32_t height = output_data[p_screen].height;

	Size2i size = Point2i(width, height);
	return size;
}


Point2 OS_Wayland::get_window_position() const {
	WARN_PRINT("Window positioning is unknown for a Wayland client");
	return Point2i(0, 0);
}


void OS_Wayland::set_window_position(const Point2& p_position) {
	WARN_PRINT("Window positioning is unknown for a Wayland client");
}


Size2 OS_Wayland::get_window_size() const {
	return Size2i(current_videomode.width, current_videomode.height);
}


void OS_Wayland::set_window_size(const Size2 p_size) {
	shell_surface_configure(this, shell_surface, 0, p_size.width, p_size.height );
}


void OS_Wayland::set_window_fullscreen(bool p_enabled) {

	if( current_videomode.resizable == false )
		return;

	if( p_enabled ) {
		wl_shell_surface_set_fullscreen( shell_surface, 0, 0, NULL );
		current_videomode.fullscreen = true;
		maximized = false;
	}
	else {
		wl_shell_surface_set_toplevel( shell_surface );
		current_videomode.fullscreen = false;
		maximized = false;
	}
}


bool OS_Wayland::is_window_fullscreen() const {
	return current_videomode.fullscreen;
}


void OS_Wayland::set_window_resizable(bool p_enabled) {
	current_videomode.resizable = p_enabled;
}


bool OS_Wayland::is_window_resizable() const {
	return current_videomode.resizable;
}


void OS_Wayland::set_window_minimized(bool p_enabled) {
	;	// FIXME
}


bool OS_Wayland::is_window_minimized() const {
	return false;	// FIXME
}


void OS_Wayland::set_window_maximized(bool p_enabled) {
	
	if( current_videomode.resizable == false )
		return;

	if( p_enabled ) {
		wl_shell_surface_set_maximized( shell_surface, NULL );
		maximized = true;
		current_videomode.fullscreen = false;
	}
	else {
		wl_shell_surface_set_toplevel( shell_surface );
		maximized = false;
		current_videomode.fullscreen = false;
	}		
}


bool OS_Wayland::is_window_maximized() const {
	return maximized;
}


MainLoop *OS_Wayland::get_main_loop() const {

	return main_loop;
}


void OS_Wayland::delete_main_loop() {

	if (main_loop)
		memdelete(main_loop);
	main_loop=NULL;
}


void OS_Wayland::set_main_loop( MainLoop * p_main_loop ) {

	main_loop=p_main_loop;
	input->set_main_loop(p_main_loop);
}


bool OS_Wayland::can_draw() const {

	return !minimized;
}


void OS_Wayland::set_clipboard(const String& p_text) {
	;	// FIXME
}


String OS_Wayland::get_clipboard() const {
	String ret;	// FIXME
	return ret;
}


String OS_Wayland::get_name() {
	return "Wayland";
}


Error OS_Wayland::shell_open(String p_uri) {

	Error ok;
	List<String> args;
	args.push_back(p_uri);
	ok = execute("/usr/bin/xdg-open",args,false);
	if (ok==OK)
		return OK;
	ok = execute("gnome-open",args,false);
	if (ok==OK)
		return OK;
	ok = execute("kde-open",args,false);
	return ok;
}


String OS_Wayland::get_system_dir(SystemDir p_dir) const {

	String xdgparam;

	switch(p_dir) {
		case SYSTEM_DIR_DESKTOP: {

			xdgparam="DESKTOP";
		} break;
		case SYSTEM_DIR_DCIM: {

			xdgparam="PICTURES";

		} break;
		case SYSTEM_DIR_DOCUMENTS: {

			xdgparam="DOCUMENTS";

		} break;
		case SYSTEM_DIR_DOWNLOADS: {

			xdgparam="DOWNLOAD";

		} break;
		case SYSTEM_DIR_MOVIES: {

			xdgparam="VIDEOS";

		} break;
		case SYSTEM_DIR_MUSIC: {

			xdgparam="MUSIC";

		} break;
		case SYSTEM_DIR_PICTURES: {

			xdgparam="PICTURES";

		} break;
		case SYSTEM_DIR_RINGTONES: {

			xdgparam="MUSIC";

		} break;
	}

	String pipe;
	List<String> arg;
	arg.push_back(xdgparam);
	Error err = const_cast<OS_Wayland*>(this)->execute("/usr/bin/xdg-user-dir",arg,true,NULL,&pipe);
	if (err!=OK)
		return ".";
	return pipe.strip_edges();
}


void OS_Wayland::move_window_to_foreground() {
	;	// FIXME
}


void OS_Wayland::release_rendering_thread() {
	context_gl->release_current();
}


void OS_Wayland::make_rendering_thread() {
	context_gl->make_current();
}


void OS_Wayland::swap_buffers() {

	context_gl->swap_buffers();
}


void OS_Wayland::set_icon(const Image& p_icon) {
	;	// FIXME
}


void OS_Wayland::run() {

	force_quit = false;
	
	if (!main_loop)
		return;
		
	main_loop->init();
		
	while (!force_quit) {
	
		wl_display_roundtrip( display );	// FIXME: Replace wl_display_roundtrip

		keyboard_repeat_key( this );
		
		if (Main::iteration()==true)
			break;
	};
	
	main_loop->finish();
}


OS_Wayland::OS_Wayland() {

#ifdef RTAUDIO_ENABLED
	AudioDriverManagerSW::add_driver(&driver_rtaudio);
#endif

#ifdef PULSEAUDIO_ENABLED
	AudioDriverManagerSW::add_driver(&driver_pulseaudio);
#endif

#ifdef ALSA_ENABLED
	AudioDriverManagerSW::add_driver(&driver_alsa);
#endif

	minimized = false;
	maximized = false;
	mouse_mode=MOUSE_MODE_VISIBLE;

	event_id = 0;

	display = NULL;
	compositor = NULL;
	surface = NULL;
	shell = NULL;
	shell_surface = NULL;
	registry = NULL;
	seat = NULL;
	pointer = NULL;
	pointer_data.button_mask = 0;

	keyboard = NULL;
	keyboard_data.context = NULL;
	keyboard_data.keymap = NULL;
	keyboard_data.state = NULL;
	keyboard_data.modifiers.alt = false;
	keyboard_data.modifiers.command = false;
	keyboard_data.modifiers.control = false;
	keyboard_data.modifiers.meta = false;
	keyboard_data.modifiers.shift = false;
	keyboard_data.repeat_rate = 0;
	keyboard_data.repeat_delay = 0;
	keyboard_data.repeat_key = 0;
	keyboard_data.repeat_time = 0;
}


void OS_Wayland::set_cursor_shape(OS::CursorShape p_shape) {
	struct wl_buffer *buffer;
	struct wl_cursor_image *image;

	image = pointer_data.cursors[p_shape]->images[0];
	buffer = wl_cursor_image_get_buffer( image );

	wl_surface_attach( pointer_data.cursor_surface, buffer, 0, 0 );
	wl_surface_damage( pointer_data.cursor_surface, 0, 0, image->width, image->height );
	wl_surface_commit( pointer_data.cursor_surface );
}
