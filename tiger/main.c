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

/*--------------------------------------------------------------*/
#define WIDTH   1280
#define HEIGHT  720
#define ORIGIN_X 0
#define ORIGIN_Y 0
#define MAXSCALE  15

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
	PathData*		m_paths;
	int			m_numPaths;
} PS;

/*--------------------------------------------------------------*/
EGLDisplay			egldisplay;
EGLConfig			eglconfig;
EGLSurface			eglsurface;
EGLContext			eglcontext;

PS* tiger 			= NULL;

float degree 			= 0.0;
float deltaX 			= 0.0;
float deltaY 	  		= 0.0;
float deltaS 			= 0.01;
float transX 			= 0.0;
float transY 			= 0.0;

short zoomMode 			= 0;// 0-stop 1 -in 2-out
short rotateMode 		= 0;// 0-stop 1 -left 2-right
short transMode 		= 0;// 0-stop 1 -left 2-right

/*--------------------------------------------------------------*/
PS* PS_construct(const char* commands, int commandCount, const float* points, int pointCount);
void PS_render(PS* ps);
void PS_destruct(PS* ps);
void mainloop(int w, int h);
void exitfunc(void);

/*--------------------------------------------------------------*/
int main(void)
{
	int width;
	int height;
	int framecount 			= 1;
	int i 				= 0;
	rotateMode=zoomMode=transMode	= 1;
	transX = WIDTH+deltaX;
	transY = HEIGHT/2+deltaY;
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

	//Setup gdl (Plane)
	gdl_plane_id_t plane 			= GDL_PLANE_ID_UPP_C;
	gdl_init(0);
	gdl_display_info_t display_info 	= { 0 };
	gdl_pixel_format_t pix_fmt 		= GDL_PF_ARGB_32;
	gdl_color_space_t color_space		= GDL_COLOR_SPACE_RGB;

	gdl_rectangle_t src_rect;
	gdl_rectangle_t dst_rect;

	dst_rect.origin.x 			= ORIGIN_X;
	dst_rect.origin.y 			= ORIGIN_Y;
	dst_rect.width 				= WIDTH;
	dst_rect.height 			= HEIGHT; 

	src_rect.origin.x 			= 0;
	src_rect.origin.y 			= 0;
	src_rect.width 				= WIDTH;
	src_rect.height 			= HEIGHT;

	gdl_plane_reset(plane);
	gdl_plane_config_begin(plane);
	gdl_plane_set_attr(GDL_PLANE_SRC_COLOR_SPACE, &color_space);
	gdl_plane_set_attr(GDL_PLANE_PIXEL_FORMAT, &pix_fmt);
	gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dst_rect);
	gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &src_rect);
	gdl_plane_config_end(GDL_FALSE);

	NativeWindowType window = 0;

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
		mainloop(width,height);

	atexit(exitfunc);
	return 0;
}

/*--------------------------------------------------------------*/
void mainloop(int w, int h)
{
	float clearColor[4] 	= {0,0,0,0};
	float scale		= w / (tigerMaxX - tigerMinX);

	if (degree >= 360)
		degree -= 360;

	vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
	vgClear(0, 0, w, h);
	vgLoadIdentity();

	//Translate ( left <-> right )
	if (transMode !=0 ) {
		if(transMode == 1) {//left
			if (transX <=0){
				transMode = 2;
			}else
				transX-=5.0;

		}else if(transMode == 2) { //right
			if( transX >= w) {
				transMode = 1;
			}else
				transX+=5.0;
		}
	}
	vgTranslate(transX,transY);

	//Rotate ( default : left )
	if (rotateMode !=0) { //rotate
		if ( rotateMode == 1 ){ //left
			vgRotate(degree);
			degree += 0.5;
		} 
		else if ( rotateMode == 2 ){ //right 
			vgRotate(degree);
			degree -= 0.5;
		} 
	}

	//Zoom
	vgScale(deltaS,deltaS);	

	if (zoomMode !=0 ) { // zoom  
		if (zoomMode == 1) {  // zoom in
			if (deltaS >= MAXSCALE){
				zoomMode=2;
			}else {
				deltaS *= 1.01;
			}
		} else if (zoomMode == 2) { //zoom out
			if (deltaS <=0.01) {
				deltaS = 0.01;
				zoomMode=1;
			}else {
				deltaS *= 0.99;
			}
		} 
	}

	//Coordiantion
	vgTranslate(-tigerMaxY/2,-tigerMaxY/2);

	//Rendering 
	PS_render(tiger);

	//Swap
	eglSwapBuffers(egldisplay, eglsurface);	//copies the OpenVG back buffer to window (uses internally glDrawPixels)
}

/*--------------------------------------------------------------*/
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

/*--------------------------------------------------------------*/
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

		//Background color ( alpha 0 )
		static int firstdraw = 1;

		if (firstdraw){
			firstdraw = 0;
			color[3] = 0.0f;
		} else
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

void exitfunc(void)
{
	PS_destruct(tiger);
	eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglTerminate(egldisplay);
}
