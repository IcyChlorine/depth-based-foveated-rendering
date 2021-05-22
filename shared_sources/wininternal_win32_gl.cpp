/*-----------------------------------------------------------------------
    Copyright (c) 2013, NVIDIA. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to tlorach@nvidia.com (Tristan Lorach)
*/ //--------------------------------------------------------------------
#include <GL/glew.h>
#include <GL/wglew.h>

#include <windows.h>
#include <windowsx.h>
#include "resources.h"

#define DECL_WININTERNAL
#include "main.h"
#include "nv_helpers/misc.hpp" // for bmp screenshot

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

extern LRESULT CALLBACK WindowProc( HWND   m_hWnd, 
                             UINT   msg, 
                             WPARAM wParam, 
                             LPARAM lParam );
extern HINSTANCE   g_hInstance;
extern std::vector<NVPWindow *> g_windows;


struct WINinternalGL : WINinternal
{
  HGLRC m_hRC;

  WINinternalGL(NVPWindow *win) : WINinternal (win)
    , m_hRC(NULL)
  {  }

  virtual bool initBase(const NVPWindow::ContextFlags * cflags, NVPWindow* sourcewindow);
  virtual int  sysExtensionSupported( const char* name );
  virtual void swapInterval(int i)  { wglSwapIntervalEXT(i); }
  virtual void swapBuffers()        { SwapBuffers( m_hDC ); }
  virtual void* getHGLRC()          { return m_hRC; }
  virtual NVPWindow::NVPproc sysGetProcAddress( const char* name ) 
                                    { return (NVPWindow::NVPproc)wglGetProcAddress(name); }
  virtual void makeContextCurrent() { wglMakeCurrent(m_hDC, m_hRC); }
  virtual void makeContextNonCurrent() { wglMakeCurrent(0, 0); }

  virtual void screenshot(const char* filename, int x, int y, int width, int height, unsigned char* data);


  static WINinternal* alloc(NVPWindow *win);
};

//------------------------------------------------------------------------------
// create this specific WINinternal
//------------------------------------------------------------------------------
WINinternal* newWINinternalGL(NVPWindow *win)
{
    return new WINinternalGL(win);
}

//------------------------------------------------------------------------------
// OGL callback
//------------------------------------------------------------------------------
#ifdef _DEBUG
static void APIENTRY myOpenGLCallback(  GLenum source,
                        GLenum type,
                        GLuint id,
                        GLenum severity,
                        GLsizei length,
                        const GLchar* message,
                        const GLvoid* userParam)
{

  NVPWindow* window = (NVPWindow*)userParam;

  GLenum filter = window->m_debugFilter;
  GLenum severitycmp = severity;
  // minor fixup for filtering so notification becomes lowest priority
  if (GL_DEBUG_SEVERITY_NOTIFICATION == filter){
    filter = GL_DEBUG_SEVERITY_LOW_ARB+1;
  }
  if (GL_DEBUG_SEVERITY_NOTIFICATION == severitycmp){
    severitycmp = GL_DEBUG_SEVERITY_LOW_ARB+1;
  }

  if (!filter|| severitycmp <= filter )
  {
  
    //static std::map<GLuint, bool> ignoreMap;
    //if(ignoreMap[id] == true)
    //    return;
    char *strSource = "0";
    char *strType = strSource;
    switch(source)
    {
    case GL_DEBUG_SOURCE_API_ARB:
        strSource = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        strSource = "WINDOWS";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        strSource = "SHADER COMP.";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        strSource = "3RD PARTY";
        break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
        strSource = "APP";
        break;
    case GL_DEBUG_SOURCE_OTHER_ARB:
        strSource = "OTHER";
        break;
    }
    switch(type)
    {
    case GL_DEBUG_TYPE_ERROR_ARB:
        strType = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        strType = "Deprecated";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        strType = "Undefined";
        break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        strType = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        strType = "Performance";
        break;
    case GL_DEBUG_TYPE_OTHER_ARB:
        strType = "Other";
        break;
    }
    switch(severity)
    {
    case GL_DEBUG_SEVERITY_HIGH_ARB:
        LOGE("ARB_debug : %s High - %s - %s : %s\n", window->m_debugTitle.c_str(), strSource, strType, message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        LOGW("ARB_debug : %s Medium - %s - %s : %s\n", window->m_debugTitle.c_str(), strSource, strType, message);
        break;
    case GL_DEBUG_SEVERITY_LOW_ARB:
        LOGI("ARB_debug : %s Low - %s - %s : %s\n", window->m_debugTitle.c_str(), strSource, strType, message);
        break;
    default:
        //LOGI("ARB_debug : comment - %s - %s : %s\n", strSource, strType, message);
        break;
    }
  }
}

//------------------------------------------------------------------------------
void checkGL( char* msg )
{
	GLenum errCode;
	//const GLubyte* errString;
	errCode = glGetError();
	if (errCode != GL_NO_ERROR) {
		//printf ( "%s, ERROR: %s\n", msg, gluErrorString(errCode) );
        LOGE("%s, ERROR: 0x%x\n", msg, errCode );
	}
}
#endif
//------------------------------------------------------------------------------
bool WINinternalGL::initBase(const NVPWindow::ContextFlags* cflags, NVPWindow* sourcewindow)
{
    GLuint PixelFormat;
    
    NVPWindow::ContextFlags  settings;
    if (cflags){
      settings = *cflags;
    }

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = settings.depth;
    pfd.cStencilBits = settings.stencil;

    if( settings.stereo )
    {
      pfd.dwFlags |= PFD_STEREO;
    }

    if(settings.MSAA > 1)
    {
        m_hDC = GetDC(m_hWndDummy);
        PixelFormat = ChoosePixelFormat( m_hDC, &pfd );
        SetPixelFormat( m_hDC, PixelFormat, &pfd);
        m_hRC = wglCreateContext( m_hDC );
        wglMakeCurrent( m_hDC, m_hRC );
        glewInit();
        ReleaseDC(m_hWndDummy, m_hDC);
        m_hDC = GetDC( m_hWnd );
        int attri[] = {
			WGL_DRAW_TO_WINDOW_ARB, true,
	        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		    WGL_SUPPORT_OPENGL_ARB, true,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
	        WGL_DOUBLE_BUFFER_ARB, true,
	        WGL_DEPTH_BITS_ARB, settings.depth,
	        WGL_STENCIL_BITS_ARB, settings.stencil,
            WGL_SAMPLE_BUFFERS_ARB, 1,
			WGL_SAMPLES_ARB,
			settings.MSAA,
            0,0
        };
        GLuint nfmts;
        int fmt;
	    if(!wglChoosePixelFormatARB( m_hDC, attri, NULL, 1, &fmt, &nfmts )){
            wglDeleteContext(m_hRC);
            return false;
        }
        wglDeleteContext(m_hRC);
        DestroyWindow(m_hWndDummy);
        m_hWndDummy = NULL;
        if(!SetPixelFormat( m_hDC, fmt, &pfd))
            return false;
    } else {
        m_hDC = GetDC( m_hWnd );
        PixelFormat = ChoosePixelFormat( m_hDC, &pfd );
        SetPixelFormat( m_hDC, PixelFormat, &pfd);
    }
    m_hRC = wglCreateContext( m_hDC );
    wglMakeCurrent( m_hDC, m_hRC );
    // calling glewinit NOW because the inside glew, there is mistake to fix...
    // This is the joy of using Core. The query glGetString(GL_EXTENSIONS) is deprecated from the Core profile.
    // You need to use glGetStringi(GL_EXTENSIONS, <index>) instead. Sounds like a "bug" in GLEW.
    glewInit();
#define GLCOMPAT
    if(!wglCreateContextAttribsARB)
        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if(wglCreateContextAttribsARB)
    {
        HGLRC hRC = NULL;
        std::vector<int> attribList;
        #define ADDATTRIB(a,b) { attribList.push_back(a); attribList.push_back(b); }
        int maj= settings.major;
        int min= settings.minor;
        ADDATTRIB(WGL_CONTEXT_MAJOR_VERSION_ARB, maj)
        ADDATTRIB(WGL_CONTEXT_MINOR_VERSION_ARB, min)
        if(settings.core)
            ADDATTRIB(WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB)
        else
            ADDATTRIB(WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB)
        int ctxtflags = 0;
        if(settings.debug)
            ctxtflags |= WGL_CONTEXT_DEBUG_BIT_ARB;
        if(settings.robust)
            ctxtflags |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;
        if(settings.forward) // use it if you want errors when compat options still used
            ctxtflags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
        ADDATTRIB(WGL_CONTEXT_FLAGS_ARB, ctxtflags);
        ADDATTRIB(0, 0)
        int *p = &(attribList[0]);
        if (!(hRC = wglCreateContextAttribsARB(m_hDC, 0, p )))
        {
            LOGE("wglCreateContextAttribsARB() failed for OpenGL context.\n");
            return false;
        }
        if (!wglMakeCurrent(m_hDC, hRC)) { 
            LOGE("wglMakeCurrent() failed for OpenGL context.\n"); 
        } else {
            wglDeleteContext( m_hRC );
            m_hRC = hRC;
#ifdef _DEBUG
            if(!__glewDebugMessageCallbackARB)
            {
                __glewDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)wglGetProcAddress("glDebugMessageCallbackARB");
                __glewDebugMessageControlARB  = (PFNGLDEBUGMESSAGECONTROLARBPROC) wglGetProcAddress("glDebugMessageControlARB");
            }
            if(__glewDebugMessageCallbackARB)
            {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
                glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
                glDebugMessageCallbackARB(myOpenGLCallback, sourcewindow);
            }
#endif
        }
    }
    glewInit();
    LOGOK("Loaded Glew\n");
    LOGOK("initialized OpenGL basis\n");
    return true;
}


// from GLFW 3.0
static int stringInExtensionString(const char* string, const char* exts)
{
  const GLubyte* extensions = (const GLubyte*) exts;
  const GLubyte* start;
  GLubyte* where;
  GLubyte* terminator;

  // It takes a bit of care to be fool-proof about parsing the
  // OpenGL extensions string. Don't be fooled by sub-strings,
  // etc.
  start = extensions;
  for (;;)
  {
    where = (GLubyte*) strstr((const char*) start, string);
    if (!where)
      return GL_FALSE;

    terminator = where + strlen(string);
    if (where == start || *(where - 1) == ' ')
    {
      if (*terminator == ' ' || *terminator == '\0')
        break;
    }

    start = terminator;
  }

  return GL_TRUE;
}

int WINinternalGL::sysExtensionSupported( const char* name )
{
  // we are not using the glew query, as glew will only report
  // those extension it knows about, not what the actual driver may support

  int i;
  GLint count;

  // Check if extension is in the modern OpenGL extensions string list
  // This should be safe to use since GL 3.0 is around for a long time :)

  glGetIntegerv(GL_NUM_EXTENSIONS, &count);

  for (i = 0;  i < count;  i++)
  {
    const char* en = (const char*) glGetStringi(GL_EXTENSIONS, i);
    if (!en)
    {
      return GL_FALSE;
    }

    if (strcmp(en, name) == 0)
      return GL_TRUE;
  }

  // Check platform specifc gets

  const char* exts = NULL;
  WINinternalGL* wininternal = static_cast<WINinternalGL*>(g_windows[0]->m_internal);

  if (WGLEW_ARB_extensions_string){
    exts = wglGetExtensionsStringARB(wininternal->m_hDC);
  }
  if (!exts && WGLEW_EXT_extensions_string){
    exts = wglGetExtensionsStringEXT();
  }
  if (!exts) {
    return FALSE;
  }
  
  return stringInExtensionString(name,exts);
}

//------------------------------------------------------------------------------
// screen shot
//------------------------------------------------------------------------------
void WINinternalGL::screenshot(const char* filename, int x, int y, int width, int height, unsigned char* data)
{
    glFinish();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glReadPixels(x,y,width,height, GL_BGRA, GL_UNSIGNED_BYTE, data);

    if(filename)
        nv_helpers::saveBMP(filename, width, height, data);
}
