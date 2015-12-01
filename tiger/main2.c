/*------------------------------------------------------------------------
 *
 * OpenVG 1.0.1 Reference Implementation sample code
 * -------------------------------------------------
 *
 * Copyright (c) 2007 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and /or associated documentation files
 * (the "Materials "), to deal in the Materials without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Materials,
 * and to permit persons to whom the Materials are furnished to do so,
 * subject to the following conditions: 
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Materials. 
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR
 * THE USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 *//**
 * \file
 * \brief	Tiger sample application. Resizing the application window
 *			rerenders the tiger in the new resolution. Pressing 1,2,3
 *			or 4 sets pixel zoom factor, mouse moves inside the zoomed
 *			image.
 * \note	
    *//*-------------------------------------------------------------------*/


#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#define UNREF(X) ((void)(X))

#include "VG/openvg.h"
#include "VG/vgu.h"

#include "GLES/gl.h"
#include "EGL/egl.h"

#include "libgdl.h"
#include "libgma.h"

#include "tiger.h"

#define ORIGIN_X 0
#define ORIGIN_Y 0
#define WIDTH   640
#define HEIGHT  480


/*--------------------------------------------------------------*/

float				pixelScale = 1.0f;
float				aspectRatio;
int					initialWidth;
int					initialHeight;
int					rerender = 1;
float				mousex, mousey;
int					mouseButton = 0;
int					window;
EGLDisplay			egldisplay;
EGLConfig			eglconfig;
EGLSurface			eglsurface;
EGLContext			eglcontext;

/*--------------------------------------------------------------*/

typedef struct
{
	VGFillRule		m_fillRule;
	VGPaintMode		m_paintMode;
	VGCapStyle		m_capStyle;
	VGJoinStyle		m_joinStyle;
	float			m_miterLimit;
	float			m_strokeWidth;
	VGPaint			m_fillPaint;
	VGPaint			m_strokePaint;
	VGPath			m_path;
} PathData;

typedef struct
{
	PathData*			m_paths;
	int					m_numPaths;
} PS;
static void get_target_surface_dimensions(int *width, int *height);

PS* PS_construct(const char* commands, int commandCount, const float* points, int pointCount)
{
	PS* ps = (PS*)malloc(sizeof(PS));
	int p = 0;
	int c = 0;
	int i = 0;
	int paths = 0;
	int maxElements = 0;
	unsigned char* cmd;
	UNREF(pointCount);

	while(c < commandCount)
	{
		int elements, e;
		c += 4;
		p += 8;
		elements = (int)points[p++];
		assert(elements > 0);
		if(elements > maxElements)
			maxElements = elements;
		for(e=0;e<elements;e++)
		{
			switch(commands[c])
			{
				case 'M': p += 2; break;
				case 'L': p += 2; break;
				case 'C': p += 6; break;
				case 'E': break;
				default:
					  assert(0);		//unknown command
			}
			c++;
		}
		paths++;
	}

	ps->m_numPaths = paths;
	ps->m_paths = (PathData*)malloc(paths * sizeof(PathData));
	cmd = (unsigned char*)malloc(maxElements);

	i = 0;
	p = 0;
	c = 0;
	while(c < commandCount)
	{
		int elements, startp, e;
		float color[4];

		//fill type
		int paintMode = 0;
		ps->m_paths[i].m_fillRule = VG_NON_ZERO;
		switch( commands[c] )
		{
			case 'N':
				break;
			case 'F':
				ps->m_paths[i].m_fillRule = VG_NON_ZERO;
				paintMode |= VG_FILL_PATH;
				break;
			case 'E':
				ps->m_paths[i].m_fillRule = VG_EVEN_ODD;
				paintMode |= VG_FILL_PATH;
				break;
			default:
				assert(0);		//unknown command
		}
		c++;

		//stroke
		switch( commands[c] )
		{
			case 'N':
				break;
			case 'S':
				paintMode |= VG_STROKE_PATH;
				break;
			default:
				assert(0);		//unknown command
		}
		ps->m_paths[i].m_paintMode = (VGPaintMode)paintMode;
		c++;

		//line cap
		switch( commands[c] )
		{
			case 'B':
				ps->m_paths[i].m_capStyle = VG_CAP_BUTT;
				break;
			case 'R':
				ps->m_paths[i].m_capStyle = VG_CAP_ROUND;
				break;
			case 'S':
				ps->m_paths[i].m_capStyle = VG_CAP_SQUARE;
				break;
			default:
				assert(0);		//unknown command
		}
		c++;

		//line join
		switch( commands[c] )
		{
			case 'M':
				ps->m_paths[i].m_joinStyle = VG_JOIN_MITER;
				break;
			case 'R':
				ps->m_paths[i].m_joinStyle = VG_JOIN_ROUND;
				break;
			case 'B':
				ps->m_paths[i].m_joinStyle = VG_JOIN_BEVEL;
				break;
			default:
				assert(0);		//unknown command
		}
		c++;

		//the rest of stroke attributes
		ps->m_paths[i].m_miterLimit = points[p++];
		ps->m_paths[i].m_strokeWidth = points[p++];

		//paints
		color[0] = points[p++];
		color[1] = points[p++];
		color[2] = points[p++];
		color[3] = 1.0f;
		ps->m_paths[i].m_strokePaint = vgCreatePaint();
		vgSetParameteri(ps->m_paths[i].m_strokePaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
		vgSetParameterfv(ps->m_paths[i].m_strokePaint, VG_PAINT_COLOR, 4, color);

		color[0] = points[p++];
		color[1] = points[p++];
		color[2] = points[p++];
		color[3] = 1.0f;
		ps->m_paths[i].m_fillPaint = vgCreatePaint();
		vgSetParameteri(ps->m_paths[i].m_fillPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
		vgSetParameterfv(ps->m_paths[i].m_fillPaint, VG_PAINT_COLOR, 4, color);

		//read number of elements

		elements = (int)points[p++];
		assert(elements > 0);
		startp = p;
		for(e=0;e<elements;e++)
		{
			switch( commands[c] )
			{
				case 'M':
					cmd[e] = VG_MOVE_TO | VG_ABSOLUTE;
					p += 2;
					break;
				case 'L':
					cmd[e] = VG_LINE_TO | VG_ABSOLUTE;
					p += 2;
					break;
				case 'C':
					cmd[e] = VG_CUBIC_TO | VG_ABSOLUTE;
					p += 6;
					break;
				case 'E':
					cmd[e] = VG_CLOSE_PATH;
					break;
				default:
					assert(0);		//unknown command
			}
			c++;
		}

		ps->m_paths[i].m_path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, (unsigned int)VG_PATH_CAPABILITY_ALL);
		vgAppendPathData(ps->m_paths[i].m_path, elements, cmd, points + startp);
		i++;
	}
	free(cmd);
	return ps;
}

void PS_destruct(PS* ps)
{
	int i;
	assert(ps);
	for(i=0;i<ps->m_numPaths;i++)
	{
		vgDestroyPaint(ps->m_paths[i].m_fillPaint);
		vgDestroyPaint(ps->m_paths[i].m_strokePaint);
		vgDestroyPath(ps->m_paths[i].m_path);
	}
	free(ps->m_paths);
	free(ps);
}

void PS_render(PS* ps)
{
	int i;
	assert(ps);
	vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);

	for(i=0;i<ps->m_numPaths;i++)
	{
		vgSeti(VG_FILL_RULE, ps->m_paths[i].m_fillRule);
		vgSetPaint(ps->m_paths[i].m_fillPaint, VG_FILL_PATH);

		if(ps->m_paths[i].m_paintMode & VG_STROKE_PATH)
		{
			vgSetf(VG_STROKE_LINE_WIDTH, ps->m_paths[i].m_strokeWidth);
			vgSeti(VG_STROKE_CAP_STYLE, ps->m_paths[i].m_capStyle);
			vgSeti(VG_STROKE_JOIN_STYLE, ps->m_paths[i].m_joinStyle);
			vgSetf(VG_STROKE_MITER_LIMIT, ps->m_paths[i].m_miterLimit);
			vgSetPaint(ps->m_paths[i].m_strokePaint, VG_STROKE_PATH);
		}

		vgDrawPath(ps->m_paths[i].m_path, ps->m_paths[i].m_paintMode);
	}
}

PS* tiger = NULL;

/*--------------------------------------------------------------*/
#if 0

char kbdBuffer[256];
void mainloop(void);
const char * STR(const char *format, ...)
{
	static char string[1024];
	va_list arg;
	va_start( arg, format );
	vsprintf( string, format, arg );
	va_end( arg );
	return string;
}
void kbdfunc(unsigned char key, int x, int y)
{
	UNREF(x);
	UNREF(y);
	kbdBuffer[key] = 1;

	if( key == 27 )
		exit(0);	//esc or alt-F4 exits
	if( key == '1' ) pixelScale = 1.0f;
	if( key == '2' ) pixelScale = 2.0f;
	if( key == '3' ) pixelScale = 4.0f;
	if( key == '4' ) pixelScale = 16.0f;
}
void kbdupfunc(unsigned char key, int x, int y)
{
	UNREF(x);
	UNREF(y);
	kbdBuffer[key] = 0;
}
void specialfunc(int key, int x, int y)
{
	UNREF(x);
	UNREF(y);
	if( key == GLUT_KEY_F4 && glutGetModifiers() == GLUT_ACTIVE_ALT )
		exit(0);	//esc or alt-F4 exits
	kbdBuffer[key] = 1;
}
void specialupfunc(int key, int x, int y)
{
	UNREF(x);
	UNREF(y);
	kbdBuffer[key] = 0;
}
void mousefunc(int x, int y)
{
	mousex = (float)x;
	mousey = (float)(glutGet(GLUT_WINDOW_HEIGHT) - y);
}
void mouseButtonFunc(int button, int state, int x, int y)
{
	UNREF(x);
	UNREF(y);
	if( button == GLUT_LEFT_BUTTON )
		mouseButton = (state == GLUT_DOWN);
}
void idlefunc(void) { glutPostRedisplay(); }
#endif
void reshape(int w, int h)
{
	eglSwapBuffers(egldisplay, eglsurface);	//resizes the rendering surface
	glViewport(0,0,w,h);
	//	glutSetWindowTitle(STR("OpenVG Tiger sample (%dx%d)",w,h));
	rerender = 1;
}

void exitfunc(void)
{
	PS_destruct(tiger);
	eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglTerminate(egldisplay);
}

/*--------------------------------------------------------------*/

void mainloop(int w, int h)
{
	if(rerender)
	{
		static degree=0;
		if (degree >= 360)
		{
			degree = 0;
		}
#if 0
		int w = glutGet(GLUT_WINDOW_WIDTH);
		int h = glutGet(GLUT_WINDOW_HEIGHT);
#endif

		float clearColor[4] = {1,1,1,1};
		float scale = w / (tigerMaxX - tigerMinX);

		vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
		vgClear(0, 0, w, h);

		vgLoadIdentity();
		vgScale(scale, scale);
		vgTranslate(-tigerMinX, -tigerMinY + 0.5f * (h / scale - (tigerMaxY - tigerMinY)));
		vgRotate(degree);
		degree++;

		PS_render(tiger);
		rerender = 1;
	}

	//clear screen and set position and zoom for glDrawPixels called from eglSwapBuffers
#if 0
	glColor4f(0,0,0,0xff);
	glClear(GL_COLOR_BUFFER_BIT);
#endif

#if 0
	if(glWindowPos2f)
		glWindowPos2f(mousex * (1.0f - pixelScale), mousey * (1.0f - pixelScale));
	glPixelZoom(pixelScale, pixelScale);
#endif
	eglSwapBuffers(egldisplay, eglsurface);	//copies the OpenVG back buffer to window (uses internally glDrawPixels)
}

/*--------------------------------------------------------------*/

int main(void)
{
#if 0
	int c = 1;
	char empty[] = "tiger.exe";
	char* param = (char*)empty;
#endif

#if 0
	memset(kbdBuffer,0,256*sizeof(char));
	//open a window using GLUT
	glutInit( &c, &param );
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(initialWidth, initialHeight);
	window = glutCreateWindow("OpenVG Tiger sample");
#endif

	int width;
	int height;
	int framecount = 1;
	int init_gdl = 1;

	static const EGLint s_configAttribs[] =
	{
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE, 	8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE, 	8,
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,	EGL_OPENVG_BIT,
		EGL_NONE
	};
	EGLint numconfigs;

	aspectRatio = (tigerMaxY - tigerMinY) / (tigerMaxX - tigerMinX);
	initialWidth = 1920;
	initialHeight = (int)(aspectRatio * initialWidth);

	//Setup gdl (Plane)

	//	gdl_ret_t gdl_rc = GDL_SUCCESS;
	gdl_plane_id_t plane = GDL_PLANE_ID_UPP_C;
	gdl_init(0);
	gdl_display_info_t display_info = { 0 };
	gdl_pixel_format_t pix_fmt = GDL_PF_ARGB_32;
	gdl_color_space_t color_space = GDL_COLOR_SPACE_RGB;

	gdl_rectangle_t src_rect;
	gdl_rectangle_t dst_rect;

	dst_rect.origin.x = ORIGIN_X;
	dst_rect.origin.y = ORIGIN_Y;
	dst_rect.width = WIDTH;
	dst_rect.height = HEIGHT; 

	src_rect.origin.x = 0;
	src_rect.origin.y = 0;
	src_rect.width = WIDTH;
	src_rect.height = HEIGHT;

	gdl_plane_reset(plane);
	gdl_plane_config_begin(plane);
	gdl_plane_set_attr(GDL_PLANE_SRC_COLOR_SPACE, &color_space);
	gdl_plane_set_attr(GDL_PLANE_PIXEL_FORMAT, &pix_fmt);
	gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dst_rect);
	gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &src_rect);
	gdl_plane_config_end(GDL_FALSE);

	NativeWindowType window = 0;//(NativeWindowType)plane;
#if 0
	// Setup GL
	float aspect = 0.0f;

	get_target_surface_dimensions(&width, &height);

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	aspect = (float)width / (float)height;
	glOrthof(-100.0 * aspect, 100.0 * aspect, -100.0, 100.0, 1.0, -1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.0f, 0.0f, 1.0f, 0.8f);
	glEnableClientState(GL_VERTEX_ARRAY);
#endif

	//Setup EGL
	egldisplay = eglGetDisplay((EGLint)EGL_DEFAULT_DISPLAY);
	eglInitialize(egldisplay, NULL, NULL);
	eglBindAPI(EGL_OPENVG_API);
	eglGetConfigs(egldisplay, NULL, 0, &numconfigs);
	eglGetConfigs(egldisplay, eglconfig, 50, &numconfigs);
	eglChooseConfig(egldisplay, s_configAttribs, &eglconfig, 1, &numconfigs);
	eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, window, NULL);
	eglcontext = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT, NULL);
	eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglcontext);
	eglSwapInterval(egldisplay, 1);
	eglQuerySurface(egldisplay, eglsurface, EGL_WIDTH, &width);
	eglQuerySurface(egldisplay, eglsurface, EGL_HEIGHT, &height);


	tiger = PS_construct(tigerCommands, tigerCommandCount, tigerPoints, tigerPointCount);

	// Render
	while (1)
	{
		mainloop(width,height);
	}

#if 0
	glutDisplayFunc(mainloop);
	glutKeyboardFunc(kbdfunc);
	glutPassiveMotionFunc(mousefunc);
	glutMotionFunc(mousefunc);
	glutMouseFunc(mouseButtonFunc);
	glutSpecialFunc(specialfunc);
	glutKeyboardUpFunc(kbdupfunc);
	glutSpecialUpFunc(specialupfunc);
	glutReshapeFunc(reshape);
	glutIdleFunc(idlefunc);
#endif
	atexit(exitfunc);
	//	glutMainLoop();
	return 0;
}

/*--------------------------------------------------------------*/
static void get_target_surface_dimensions(int *width, int *height)
{
	if (width != NULL)
	{
		*width = 1920;
	}

	if (height != NULL)
	{
		*height = 1080; 
	}
}

