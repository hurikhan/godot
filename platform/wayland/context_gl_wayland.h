/*************************************************************************/
/*  context_gl_wayland.h                                                 */
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
#ifndef CONTEXT_GL_WAYLAND_H
#define CONTEXT_GL_WAYLAND_H

/**
	@author Mario Schlack <m4r10.5ch14ck@gmail.com>
*/

#ifdef WAYLAND_ENABLED
#if defined(OPENGL_ENABLED) || defined(LEGACYGL_ENABLED)

#include "os/os.h"
#include "drivers/gl_context/context_gl.h"
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>

struct ContextGL_Wayland_Private;

class ContextGL_Wayland : public ContextGL {

	OS::VideoMode default_video_mode;

	EGLDisplay egl_display;
	EGLConfig egl_config;
	EGLSurface egl_surface;
	EGLContext egl_context;	

	struct wl_egl_window *egl_window;
	struct wl_display *display;
	struct wl_surface *surface;

	OS::VideoMode video_mode;
	bool double_buffer;
	bool direct_render;
	int egl_major,egl_minor;
	bool opengl_3_context;

	void print_config(int number, EGLConfig config);
public:

	virtual void release_current();	
	virtual void make_current();	
	virtual void swap_buffers();
	virtual void resize( int32_t width, int32_t height );
	virtual int get_window_width();
	virtual int get_window_height();

	virtual Error initialize();

	ContextGL_Wayland(struct wl_display *p_wayland_display, struct wl_surface *p_wayland_surface ,const OS::VideoMode& p_default_video_mode,bool p_opengl_3_context);	
	~ContextGL_Wayland();
};

#endif	// OPENGL_ENABLED || LEGACYGL_ENABLE
#endif	// WAYLAND_ENABLED
#endif	// CONTEXT_GL_WAYLAND_H

