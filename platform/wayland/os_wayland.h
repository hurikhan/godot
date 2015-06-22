/*************************************************************************/
/*  os_wayland.h                                                         */
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
#ifndef OS_WAYLAND_H
#define OS_WAYLAND_H


#include "os/input.h"
#include "os/keyboard.h"
#include "drivers/unix/os_unix.h"
#include "context_gl_wayland.h"
#include "servers/visual_server.h"
#include "servers/visual/visual_server_wrap_mt.h"
#include "servers/visual/rasterizer.h"
#include "servers/physics_server.h"
#include "servers/audio/audio_server_sw.h"
#include "servers/audio/sample_manager_sw.h"
#include "servers/spatial_sound/spatial_sound_server_sw.h"
#include "servers/spatial_sound_2d/spatial_sound_2d_server_sw.h"
#include "drivers/rtaudio/audio_driver_rtaudio.h"
#include "drivers/alsa/audio_driver_alsa.h"
#include "drivers/pulseaudio/audio_driver_pulseaudio.h"
#include "servers/physics_2d/physics_2d_server_sw.h"

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

/*
	@author Mario Schlack <m4r10.5ch14ck@gmail.com>
*/

#define WL_BUTTON_LEFT 0x110
#define WL_BUTTON_RIGHT 0x111
#define WL_BUTTON_MIDDLE 0x112

class OS_Wayland : public OS_Unix {

#if defined(OPENGL_ENABLED) || defined(LEGACYGL_ENABLED)
	ContextGL_Wayland *context_gl;
#endif
	Rasterizer *rasterizer;
	VisualServer *visual_server;
	VideoMode current_videomode;
	List<String> args;
	MainLoop *main_loop;	
	PhysicsServer *physics_server;
	Physics2DServer *physics_2d_server;

	MouseMode mouse_mode;
	Point2i center;
	
	virtual void delete_main_loop();

	AudioServerSW *audio_server;
	SampleManagerMallocSW *sample_manager;
	SpatialSoundServerSW *spatial_sound_server;
	SpatialSound2DServerSW *spatial_sound_2d_server;

	bool force_quit;
	bool minimized;

	InputDefault *input;

#ifdef RTAUDIO_ENABLED
	AudioDriverRtAudio driver_rtaudio;
#endif

#ifdef ALSA_ENABLED
	AudioDriverALSA driver_alsa;
#endif

#ifdef PULSEAUDIO_ENABLED
	AudioDriverPulseAudio driver_pulseaudio;
#endif

	int audio_driver_index;

	uint32_t event_id;

	struct wl_display *display;
 	struct wl_compositor *compositor;

	// registry
	struct wl_registry *registry;

	static const struct wl_registry_listener registry_listener; 	
	static void registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
	static void registry_remover(void *data, struct wl_registry *registry, uint32_t id); 


	// shell surface
	struct wl_surface *surface;
	struct wl_shell *shell;
	struct wl_shell_surface *shell_surface;

	static const struct wl_shell_surface_listener shell_surface_listener;
	static void shell_surface_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial);
	static void shell_surface_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height);
	static void shell_surface_popup(void *data, struct wl_shell_surface *shell_surface);


	// seat
	struct wl_seat *seat;

	static const struct wl_seat_listener seat_listener;
	static void seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t caps);
	static void seat_handle_name(void *data, struct wl_seat *seat, const char *name);


	// pointer
	struct wl_pointer *pointer;

	static const struct wl_pointer_listener pointer_listener;
	static void pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
	static void pointer_handle_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);
	static void pointer_handle_motion(void *data, struct wl_pointer *pointer, uint32_t serial, wl_fixed_t surface_x, wl_fixed_t surface_y);
	static void pointer_handle_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
	static void pointer_handle_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value);

	static int pointer_get_button( uint32_t button );
	static int pointer_get_axis_direction( wl_fixed_t value );

	struct {
		Point2i pos;
		Point2i rel;
		Point2i speed;
		Point2i last_click_pos;
		uint64_t last_click_time;		
		int button_mask;
	} pointer_data;


	// keyboard
	struct wl_keyboard *keyboard;

	static const struct wl_keyboard_listener keyboard_listener;
	static void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size);
	static void keyboard_handle_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
	static void keyboard_handle_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface);
	static void keyboard_handle_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
	static void keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
	static void keyboard_handle_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay);

	static uint32_t keyboard_get_scancode( OS_Wayland *that, uint32_t key );
	static uint32_t keyboard_get_unicode( OS_Wayland *that, uint32_t key );
	static void keyboard_repeat_key( OS_Wayland *that );

	struct {
		struct xkb_context *context;
		struct xkb_keymap *keymap;
		struct xkb_state *state;
		InputModifierState modifiers;
		int32_t repeat_rate;
		int32_t repeat_delay;
		int32_t repeat_key;
		int32_t repeat_time;
	} keyboard_data;


	// output
	Vector<struct wl_output *> output_vector;
	static const struct wl_output_listener output_listener;
	static void output_handle_geometry(void *data, struct wl_output *output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make, const char *model, int32_t transform);
	static void output_handle_mode(void *data, struct wl_output *output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
	static void output_handle_done(void *data, struct wl_output *output);
	static void output_handle_scale(void *data, struct wl_output *output, int32_t factor);


protected:

	virtual int get_video_driver_count() const;
	virtual const char * get_video_driver_name(int p_driver) const;	
	virtual VideoMode get_default_video_mode() const;

	virtual int get_audio_driver_count() const;
	virtual const char * get_audio_driver_name(int p_driver) const;

	virtual void initialize(const VideoMode& p_desired,int p_video_driver,int p_audio_driver);	
	virtual void finalize();

	virtual void set_main_loop( MainLoop * p_main_loop );    

public:

	virtual String get_name();

	virtual void set_cursor_shape(CursorShape p_shape);
	
	void set_mouse_mode(MouseMode p_mode);
	MouseMode get_mouse_mode() const;

	virtual void warp_mouse_pos(const Point2& p_to);
	virtual Point2 get_mouse_pos() const;
	virtual int get_mouse_button_state() const;
	virtual void set_window_title(const String& p_title);

	virtual void set_icon(const Image& p_icon);

	virtual MainLoop *get_main_loop() const;
	
	virtual bool can_draw() const;

	virtual void set_clipboard(const String& p_text);
	virtual String get_clipboard() const;

	virtual void release_rendering_thread();
	virtual void make_rendering_thread();
	virtual void swap_buffers();

	virtual String get_system_dir(SystemDir p_dir) const;

	virtual Error shell_open(String p_uri);

	virtual void set_video_mode(const VideoMode& p_video_mode,int p_screen=0);
	virtual VideoMode get_video_mode(int p_screen=0) const;
	virtual void get_fullscreen_mode_list(List<VideoMode> *p_list,int p_screen=0) const;


	virtual int get_screen_count() const;
	virtual int get_current_screen() const;
	virtual void set_current_screen(int p_screen);
	virtual Point2 get_screen_position(int p_screen=0) const;
	virtual Size2 get_screen_size(int p_screen=0) const;
	virtual Point2 get_window_position() const;
	virtual void set_window_position(const Point2& p_position);
	virtual Size2 get_window_size() const;
	virtual void set_window_size(const Size2 p_size);
	virtual void set_window_fullscreen(bool p_enabled);
	virtual bool is_window_fullscreen() const;
	virtual void set_window_resizable(bool p_enabled);
	virtual bool is_window_resizable() const;
	virtual void set_window_minimized(bool p_enabled);
	virtual bool is_window_minimized() const;
	virtual void set_window_maximized(bool p_enabled);
	virtual bool is_window_maximized() const;

	virtual void move_window_to_foreground();

	void run();

	OS_Wayland();
};

#endif
