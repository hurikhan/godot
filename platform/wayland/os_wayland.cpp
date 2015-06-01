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
	
	print_line(String("Wayland -- Registry Event -- ") + interface + " id " + itos( id ) );

	if (strcmp(interface, "wl_compositor") == 0) 
		that->compositor =  static_cast<struct wl_compositor *> ( wl_registry_bind(registry, id, &wl_compositor_interface, 1) );
	if (strcmp(interface, "wl_shell") == 0) 
		that->shell = static_cast<wl_shell *> ( wl_registry_bind(registry, id, &wl_shell_interface, 1) );
	if (strcmp(interface, "wl_seat") == 0) 
		that->seat = static_cast<wl_seat *> ( wl_registry_bind(registry, id, &wl_seat_interface, 1) );
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
	} else if (caps & WL_SEAT_CAPABILITY_POINTER && that->pointer) {
		wl_pointer_destroy(that->pointer);
		that->pointer = NULL;
		print_line("Wayland -- Seat -- Pointer destroyed");
	}

	if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		print_line("Wayland -- Seat -- Keyboard found");
	}

	if (caps & WL_SEAT_CAPABILITY_TOUCH) {
		print_line("Wayland -- Seat -- Touch found");
	}

			
}

void OS_Wayland::seat_handle_name(void *data, struct wl_seat *seat, const char *name) {
}

//             _       _              _ _     _                       
// _ __   ___ (_)_ __ | |_ ___ _ __  | (_)___| |_ ___ _ __   ___ _ __ 
//| '_ \ / _ \| | '_ \| __/ _ \ '__| | | / __| __/ _ \ '_ \ / _ \ '__|
//| |_) | (_) | | | | | ||  __/ |    | | \__ \ ||  __/ | | |  __/ |   
//| .__/ \___/|_|_| |_|\__\___|_|    |_|_|___/\__\___|_| |_|\___|_|   
//|_|                                                                 

const struct wl_pointer_listener OS_Wayland::pointer_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis	
};

void OS_Wayland::pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	print_line("Wayland -- Pointer -- Enter");
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
	event.ID = serial;
	event.type = InputEvent::MOUSE_MOTION;
	event.device = 0;				// MSC: Check what that means!
	event.mouse_motion.mod = InputModifierState();	// FIXME: create get_key_modifier method
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

	int _button = get_pointer_button( button );
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
		print_line( "Left Click" );

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
        event.ID = serial;
        event.type = InputEvent::MOUSE_BUTTON;
	event.device = 0;
	event.mouse_button.mod = InputModifierState();		// FIXME
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

	int _button = get_pointer_axis_direction( value );

	InputEvent event;
        event.ID = 0;
        event.type = InputEvent::MOUSE_BUTTON;
	event.device = 0;
	event.mouse_button.mod = InputModifierState();
	event.mouse_button.button_mask = that->pointer_data.button_mask;
	event.mouse_button.x = that->pointer_data.pos.x;
	event.mouse_button.y = that->pointer_data.pos.y;
	event.mouse_button.global_x = that->pointer_data.pos.x;
	event.mouse_button.global_y = that->pointer_data.pos.y;
	event.mouse_button.button_index = _button;
	event.mouse_button.pressed = true;

	that->input->parse_input_event( event );

	event.ID = 1;
	event.mouse_button.pressed = false;

	that->input->parse_input_event( event );
}

int OS_Wayland::get_pointer_button( uint32_t button ) {

	switch(button) {
		case WL_BUTTON_LEFT:	return BUTTON_LEFT;
		case WL_BUTTON_RIGHT:	return BUTTON_RIGHT;
		case WL_BUTTON_MIDDLE:	return BUTTON_MIDDLE;
	}

	return 0;	
}

int OS_Wayland::get_pointer_axis_direction( wl_fixed_t value ) {
	int _value = wl_fixed_to_int( value );

	if( _value > 0 )
		return BUTTON_WHEEL_DOWN;
	else if( _value < 0 )
		return BUTTON_WHEEL_UP;

	return 0;
}

// ============================================================================================================================================================================

void OS_Wayland::initialize(const VideoMode& p_desired,int p_video_driver,int p_audio_driver) {
	
	if (get_render_thread_mode()==RENDER_SEPARATE_THREAD) {
		print_line("Not implemented...\n"); 	// FIXME
		exit(1);
	}

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

	surface = wl_compositor_create_surface( compositor );
	ERR_FAIL_COND( surface == NULL );
	print_line("Wayland -- Surface created");

	shell_surface = wl_shell_get_shell_surface(shell, surface);
	ERR_FAIL_COND(shell_surface == NULL);
	print_line("Wayland -- Shell Surface created");

	wl_shell_surface_set_toplevel(shell_surface);
	wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, this);
	wl_seat_add_listener(seat, &seat_listener, this);

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
	return 0;	// FIXME
}

Point2 OS_Wayland::get_mouse_pos() const {
	return Point2i(0, 0);	// FIXME
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
	return 0;	// FIXME
}

int OS_Wayland::get_current_screen() const {
	return 0;	// FIXME
}

void OS_Wayland::set_current_screen(int p_screen) {
	;	// FIXME
}

Point2 OS_Wayland::get_screen_position(int p_screen) const {
	Point2i position = Point2i(0, 0);	// FIXME
	return position;
}

Size2 OS_Wayland::get_screen_size(int p_screen) const {
	Size2i size = Point2i(0, 0);	// FIXME
	return size;
}
	

Point2 OS_Wayland::get_window_position() const {
	return Point2i(0, 0);	// FIXME
}

void OS_Wayland::set_window_position(const Point2& p_position) {
	;	// FIXME
}

Size2 OS_Wayland::get_window_size() const {
	return Size2i(0, 0);	// FIXME
}

void OS_Wayland::set_window_size(const Size2 p_size) {
	;	// FIXME
}

void OS_Wayland::set_window_fullscreen(bool p_enabled) {
	;	// FIXME
}

bool OS_Wayland::is_window_fullscreen() const {
	return current_videomode.fullscreen;
}

void OS_Wayland::set_window_resizable(bool p_enabled) {
	;	// FIXME
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
	;	// FIXME
}

bool OS_Wayland::is_window_maximized() const {
	return false;	// FIXME
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
};

void OS_Wayland::set_clipboard(const String& p_text) {
	;	// FIXME
}

String OS_Wayland::get_clipboard() const {
	String ret;	// FIXME
	return ret;
};

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
	
		wl_display_dispatch(display);
		
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
	mouse_mode=MOUSE_MODE_VISIBLE;

	display = NULL;
	compositor = NULL;
	surface = NULL;
	shell = NULL;
	shell_surface = NULL;
	registry = NULL;
	seat = NULL;
	pointer = NULL;
	pointer_data.button_mask = 0;
};

void OS_Wayland::set_cursor_shape(OS::CursorShape) {
	;	// FIXME
}
