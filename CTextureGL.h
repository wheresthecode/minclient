#ifndef CTEXTUREGL_H
#define CTEXTUREGL_H

#include "CTexture.h"
#include "Engine.h"

#include <iostream>

using namespace std;

class CTextureGL : public CTexture{
   public:
    CTextureGL(unsigned char* data_, int sizeX_, int sizeY_, int channels_);
    CTextureGL();

    ~CTextureGL();

    void generateTexture();	  

   private:

};

#endif
