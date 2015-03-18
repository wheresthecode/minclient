#ifndef _FONT_H
#define _FONT_H

// This code and the font texture is based on one of the tutorials of NeHe productions (Jeff Molofee). http://nehe.gamedev.net/.

#include "CTextureGL.h"
#include "Engine.h"
#include "OpenGLES2stuff.h"

#include <math.h>
#include <stdio.h>

const int FONT_MAX_NUMBER_LINES = 64;
const int FONT_MAX_LENGTH_PER_LINE = 64;
const int FONT_MAX_LETTERS_ON_SCREEN = 640;

// this font works using OpenGL quads with textures
namespace minclient {
class Font{
  public:
    Font();
    ~Font();

    static void glPrint(GLint x, GLint y, const char* const string, bool swapBuffer = false);
    static void init(SDL_Window* mainWindow_); 
    static void glPrintSavedLines();
    static void resetSavedLines();

    typedef enum{
      AUTO = -666,
    } enum_FontMode;

    static void resetNumLinesPrinted();

  private:
    static CTextureGL* tex;
    static SDL_Window* mainWindow;
    static void swapBuffers();

    static int numLinesPrinted;

    static char savedLines[FONT_MAX_LENGTH_PER_LINE][FONT_MAX_NUMBER_LINES];
    static CVector2 savedLinesCoordinates[FONT_MAX_NUMBER_LINES];
    static int savedLinesNumber;

    static GLfloat savedVertices[FONT_MAX_LETTERS_ON_SCREEN][12];
    static GLfloat savedTexCoords[FONT_MAX_LETTERS_ON_SCREEN][12];
    static int savedLetters;

    static float lookUpTableLetters[256][2];
};
}

#endif
