#ifndef _OPENGLES2STUFF_H
#define _OPENGLES2STUFF_H

// loadShader() and createProgram() from Google Android NDK
// license: Apache 2.0, https://developer.android.com/license.html
#define GL_GLEXT_PROTOTYPES

#ifdef _WIN32
  #include <windows.h>
#endif

#ifdef ANDROID
  #include <GLES2/gl2.h>
  #include <GLES2/gl2ext.h>
  #include <EGL/egl.h>
  #include <EGL/eglext.h>
  #include <android/log.h>
#else
  #include <GL/glew.h>
#ifdef __APPLE__
  #include <OpenGL/glext.h>
  #include <OpenGL/gl.h>
#else
  #include <GL/glext.h>   // used for GL_BGRA
  #include <GL/gl.h>
#endif
#endif

#include "Engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <SDL.h>

using namespace std;

static const char gVertexShader[] = 
    "uniform mediump mat4 u_Matrix;\n"
    "\n"
    "attribute mediump vec4 a_Position;\n"
    "attribute mediump vec2 a_TextureCoordinates;\n"
    "\n"
    "varying mediump vec2 v_TextureCoordinates;\n"
    "\n"
    "void main(){\n"
    "  v_TextureCoordinates = a_TextureCoordinates;\n"
    "  gl_Position = u_Matrix * a_Position;\n"
    "}\n";

static const char gFragmentShader[] = 
    "uniform sampler2D u_TextureUnit;\n"
    "varying mediump vec2 v_TextureCoordinates;\n"
    "\n"
    "void main(){\n"
    "  gl_FragColor = texture2D(u_TextureUnit, v_TextureCoordinates);\n"
    "}";

class OpenGLES2stuff{
  public:
    OpenGLES2stuff();
    ~OpenGLES2stuff();

    static void init();
    static unsigned int programID;
    static int gPositionHandle;
    static int gTexCoordHandle;
    static int uMatrixLocation;
    static int uTextureUnitLocation;

  private:
    static GLuint loadShader(GLenum shaderType, const char* pSource);  
    static GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);

    static vector<GLuint> programIDs;
    static vector<GLuint> shaderIDs;
};

#endif
