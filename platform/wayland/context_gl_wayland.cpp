/*************************************************************************/
/*  context_gl_wayland.cpp                                               */
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
#include "context_gl_wayland.h"

#ifdef WAYLAND_ENABLED
#if defined(OPENGL_ENABLED) || defined(LEGACYGL_ENABLED)
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void ContextGL_Wayland::print_config(int number, EGLConfig config) {
	int buffer, R, G, B, A;

	eglGetConfigAttrib( egl_display, config, EGL_BUFFER_SIZE, &buffer );
	eglGetConfigAttrib( egl_display, config, EGL_RED_SIZE, &R );
	eglGetConfigAttrib( egl_display, config, EGL_GREEN_SIZE, &G );
	eglGetConfigAttrib( egl_display, config, EGL_BLUE_SIZE, &B );
	eglGetConfigAttrib( egl_display, config, EGL_ALPHA_SIZE, &A );

	print_line("EGL -- config[" + itos(number) + "]: "
		+ itos(R) + "-"
		+ itos(G) + "-"
		+ itos(B) + "-"
		+ itos(A) + " "
		+ itos(buffer) + " bytes"
	);
}

void ContextGL_Wayland::release_current() {
	;
}

void ContextGL_Wayland::make_current() {
	eglMakeCurrent( egl_display, egl_surface, egl_surface, egl_context );
}

void ContextGL_Wayland::swap_buffers() {
	eglSwapBuffers( egl_display, egl_surface );
}

Error ContextGL_Wayland::initialize() {
	// Create display
	egl_display = eglGetDisplay( (EGLNativeDisplayType) display);
	ERR_FAIL_COND_V( egl_display == EGL_NO_DISPLAY, ERR_UNCONFIGURED );
	print_line("EGL -- Display created");

	// Initialize
	bool egl_init = eglInitialize(egl_display, &egl_major, &egl_minor);
	ERR_FAIL_COND_V( !egl_init, ERR_UNCONFIGURED );
	print_line("EGL -- Version: " + itos(egl_major) + "." + itos(egl_minor) );

	// Query Strings
	print_line("EGL -- EGL_CLIENT_APIS: " + String(eglQueryString(egl_display, EGL_CLIENT_APIS)));
	print_line("EGL -- EGL_EXTENSIONS: " + String(eglQueryString(egl_display, EGL_EXTENSIONS)));
	print_line("EGL -- EGL_VENDOR: " + String(eglQueryString(egl_display, EGL_VENDOR)));
	print_line("EGL -- EGL_VERSION: " + String(eglQueryString(egl_display, EGL_VERSION)));

	// Get config count
	int count;
	eglGetConfigs(egl_display, NULL, 0, &count);
	ERR_FAIL_COND_V( count == 0, ERR_UNCONFIGURED );
	print_line("EGL -- " + itos(count) + " configs found");

	// Get configs
	EGLConfig *configs = NULL;
	configs = (EGLConfig*) memalloc( count * (sizeof(*configs)));

	EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	int n;
	eglChooseConfig(egl_display, config_attribs, configs, count, &n);

	for( int i = 0; i < n; i++ ) {
		print_config(i, configs[i]);
	}

	egl_config = configs[0];	// Take the first one		

	memfree(configs);

	// Create Context
	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	egl_context = eglCreateContext( egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
	ERR_FAIL_COND_V( egl_context == EGL_NO_CONTEXT, ERR_UNCONFIGURED );
	print_line("EGL -- Context created" );

	// Create Window
	egl_window = wl_egl_window_create(surface, video_mode.width, video_mode.height );
	ERR_FAIL_COND_V( egl_window == EGL_NO_SURFACE, ERR_UNCONFIGURED );
	print_line(String("EGL -- Window created " + itos(video_mode.width) + "x" + itos(video_mode.height)));

	egl_surface = eglCreateWindowSurface(egl_display, egl_config, egl_window, NULL);
	ERR_FAIL_COND_V( egl_surface == EGL_NO_SURFACE, ERR_UNCONFIGURED );
	print_line("EGL -- Surface created" );

	eglMakeCurrent( egl_display, egl_surface, egl_surface, egl_context );
	eglSwapBuffers( egl_display, egl_surface );
		
	return OK;
}

int ContextGL_Wayland::get_window_width() {
	return video_mode.width;
}

int ContextGL_Wayland::get_window_height() {
	return video_mode.height;
}


ContextGL_Wayland::ContextGL_Wayland(struct wl_display *p_wayland_display, struct wl_surface *p_wayland_surface ,const OS::VideoMode& p_default_video_mode,bool p_opengl_3_context) {
	default_video_mode = p_default_video_mode;

	display = p_wayland_display;
	surface = p_wayland_surface;
	
	video_mode = p_default_video_mode;
	opengl_3_context = p_opengl_3_context;
	double_buffer = false;
	direct_render = false;
	egl_major = egl_minor = 0;
}

ContextGL_Wayland::~ContextGL_Wayland() {
	if( egl_display != EGL_NO_DISPLAY )
		eglTerminate( egl_display );
}

#endif
#endif
