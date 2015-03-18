#include "OpenGLstuff.h"

// todo: gearVR remove glEnable(GL_WRITEONLY_RENDERING_QCOM);

// GL_OES_compressed_ETC1_RGB8_texture
#ifndef GL_ETC1_RGB8_OES
  #define GL_ETC1_RGB8_OES                                        0x8D64
#endif


OpenGLstuff::OpenGLstuff(SDL_Window* mainWindow_){
  this->mainWindow = mainWindow_;

  frameBufferPointer = NULL;
  framebufferTexID = 0;

  for(int i = 0; i < MAX_SERVERS; i++){
    frameBufferPointerRect[i] = NULL;
    framebufferTexIDRect[i] = 0;
    rectCopyBuffers[i] = NULL;
  }

#ifndef ANDROID
  glCompressedTexImage2D = NULL;
  glCompressedTexSubImage2D = NULL;
#endif

  init();
}

OpenGLstuff::~OpenGLstuff(){
}

void OpenGLstuff::init(){
#ifndef ANDROID
  GLenum err = glewInit();
  if (err != GLEW_OK) {
      cerr << "GLEW init failed!" << std::endl;
      exit(EXIT_FAILURE);
  }
#endif

  int numBytesPerPixel = 4;

  if(!Engine::rectMode){
#ifdef ANDROID
    frameBufferPointer = new unsigned char[(Engine::screenWidthRT * Engine::screenHeightRT * numBytesPerPixel) / 8];
    memset(frameBufferPointer, 0, (Engine::screenWidthRT * Engine::screenHeightRT * numBytesPerPixel) / 8);
#else
    frameBufferPointer = new unsigned char[Engine::screenWidthRT * Engine::screenHeightRT * numBytesPerPixel];
    memset(frameBufferPointer, 0, Engine::screenWidthRT * Engine::screenHeightRT * numBytesPerPixel);
#endif
  } else {
    for(int i = 0; i < Engine::numServers; i++){
#ifdef ANDROID
      // android uses ETC1 nativley so we can assign 1/8 of the frame buffer
      frameBufferPointerRect[i] = new unsigned char[(Engine::rectSizeX[i] * Engine::rectSizeY[i] * numBytesPerPixel) / 8];
      memset(frameBufferPointerRect[i], 0, (Engine::rectSizeX[i] * Engine::rectSizeY[i] * numBytesPerPixel) / 8);
#else
      // everyone else decompresses into RGBA and needs the full frame buffer
      frameBufferPointerRect[i] = new unsigned char[Engine::rectSizeX[i] * Engine::rectSizeY[i] * numBytesPerPixel];
      memset(frameBufferPointerRect[i], 0, Engine::rectSizeX[i] * Engine::rectSizeY[i] * numBytesPerPixel);

      rectCopyBuffers[i] = new unsigned char[(Engine::rectSizeX[i] * Engine::rectSizeY[i] * numBytesPerPixel) / 8];
      memset(rectCopyBuffers[i], 0, (Engine::rectSizeX[i] * Engine::rectSizeY[i] * numBytesPerPixel) / 8);
#endif
    }
  }

  glViewport(0, 0, Engine::screenWidthGL, Engine::screenHeightGL);

#ifndef ANDROID
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
#endif

  float orthoMatrix[16];
  memset(orthoMatrix, 0, sizeof(orthoMatrix));
  Engine::calculateOrthoMatrix(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, orthoMatrix);

#ifndef ANDROID
  glMultMatrixf(orthoMatrix);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
#endif
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);

  generateFramebufferTexture();

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  swapBuffers();
  glClear(GL_COLOR_BUFFER_BIT);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
}

void OpenGLstuff::generateFramebufferTexture(){
  if(!Engine::rectMode){
    generateFramebufferTextureNoRect();
  } else {
    generateFramebufferTextureRect();
  }
}

void OpenGLstuff::generateFramebufferTextureNoRect(){
  glGenTextures(1, &framebufferTexID);

  glBindTexture(GL_TEXTURE_2D, framebufferTexID);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifdef ANDROID
  glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, Engine::screenWidthRT, Engine::screenHeightRT, 0,	(Engine::screenWidthRT * Engine::screenHeightRT) / 2, NULL);  
#else
  // assuming texture data has been manually decompressed from ETC1 into RGBA
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Engine::screenWidthRT, Engine::screenHeightRT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#endif
  
  glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLstuff::generateFramebufferTextureRect(){
  for(int i = 0; i < Engine::numServers; i++){
    glGenTextures(1, &framebufferTexIDRect[i]);

    glBindTexture(GL_TEXTURE_2D, framebufferTexIDRect[i]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifdef ANDROID
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, Engine::rectSizeX[i], Engine::rectSizeY[i], 0,	(Engine::rectSizeX[i] * Engine::rectSizeY[i]) / 2, NULL);  
#else
    // assuming texture data has been manually decompressed from ETC1 into RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Engine::rectSizeX[i], Engine::rectSizeY[i], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#endif
  }

  glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLstuff::render(){
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);

  if(!Engine::rectMode){
    drawFrameBufferNoRect();
  } else {
    drawFrameBufferRect();
  }            

  drawFPS();
  drawHUD();

  minclient::Font::glPrintSavedLines();
  minclient::Font::resetSavedLines();
}

// Optimize: Could preallocate vertices[MAX_SERVERS] and render all things while the client state is enabled
void OpenGLstuff::drawFrameBufferRect(){
#ifndef ANDROID
  glMatrixMode(GL_PROJECTION);									       
  glPushMatrix();													             
  glLoadIdentity();												             
#else
  glEnable(GL_SCISSOR_TEST);
#endif  

  float orthoMatrix[16];
  memset(orthoMatrix, 0, sizeof(orthoMatrix));
  Engine::calculateOrthoMatrix(0.0f, Engine::screenWidthGL, 0.0f, Engine::screenHeightGL, -1.0f, 1.0f, orthoMatrix);

#ifndef ANDROID
  glMultMatrixf(orthoMatrix);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
#endif

  int horizontalOffset = (Engine::screenWidthGL  - Engine::screenWidthRT)  / 2;
  int verticalOffset   = (Engine::screenHeightGL - Engine::screenHeightRT) / 2;

  GLfloat texCoords[] = {0.0f,1.0f,  1.0f,1.0f,  1.0f,0.0f,   1.0f,0.0f,   0.0f,0.0f,  0.0f,1.0f};
  const float aspectRatio = (float)Engine::screenWidthRT / (float)Engine::screenHeightRT;

  for(int i = 0; i < Engine::numServers; i++){
    // draw ray traced rectangle 
    glBindTexture(GL_TEXTURE_2D, framebufferTexIDRect[i]);
    
    int startX = Engine::rectLeftServer[i];
    int startY = Engine::rectBottomServer[i];
#ifdef ANDROID
    glScissor(startX, startY, Engine::rectSizeX[i], Engine::rectSizeY[i]);
#endif

    GLfloat vertices[] = { 
      (float)(startX + horizontalOffset), (float)(startY + (float)Engine::rectSizeY[i] + verticalOffset),
      (float)(startX + (float)Engine::rectSizeX[i] + horizontalOffset), (float)(startY + (float)Engine::rectSizeY[i] + verticalOffset),
      (float)(startX + (float)Engine::rectSizeX[i] + horizontalOffset), (float)(startY + 0.0f + verticalOffset),
      (float)(startX + (float)Engine::rectSizeX[i] + horizontalOffset), (float)(startY + 0.0f + verticalOffset),
      (float)(startX + horizontalOffset), (float)(startY + 0.0f + verticalOffset),
      (float)(startX + horizontalOffset), (float)(startY + (float)Engine::rectSizeY[i] + verticalOffset) 
    };
/*
    if(Engine::upscale){      
      float zoomRatio;
      int newHeight;
      int newWidth;
      int newVerticalOffset;
      int newHorizontalOffset;

      if(horizontalOffset > (float)verticalOffset * aspectRatio){
        // use full vertical space
        newHeight = Engine::screenHeightGL;
        zoomRatio = newHeight / (float)Engine::screenHeightRT;
        newWidth = Engine::screenWidthRT * zoomRatio;  

        newVerticalOffset = 0;
        newHorizontalOffset = (Engine::screenWidthGL  - newWidth) / 2;
      } else {
        // use full horizontal space     
        newWidth = Engine::screenWidthGL;
        zoomRatio = newWidth / (float)Engine::screenWidthRT;
        newHeight = Engine::screenHeightRT * zoomRatio;      

        newVerticalOffset = (Engine::screenHeightGL  - newHeight) / 2;
        newHorizontalOffset = 0; 
      }

      vertices[0]  = 0.0f + newHorizontalOffset;                         vertices[1]  = newHeight + newVerticalOffset; 
      vertices[2]  = newWidth + newHorizontalOffset;                     vertices[3]  = newHeight + newVerticalOffset;  
      vertices[4]  = newWidth + newHorizontalOffset;                     vertices[5]  = 0.0f + newVerticalOffset;
      vertices[6]  = newWidth + newHorizontalOffset;                     vertices[7]  = 0.0f + newVerticalOffset;   
      vertices[8]  = 0.0f + newHorizontalOffset;                         vertices[9]  = 0.0f + newVerticalOffset;  
      vertices[10] = 0.0f + newHorizontalOffset;                         vertices[11] = newHeight + newVerticalOffset;
    }   
*/

#ifndef ANDROID
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0,vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

    glDrawArrays(GL_TRIANGLES,0,6);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
    glUseProgram(OpenGLES2stuff::programID);

    glVertexAttribPointer(OpenGLES2stuff::gPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(OpenGLES2stuff::gPositionHandle);

    glVertexAttribPointer(OpenGLES2stuff::gTexCoordHandle, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
    glEnableVertexAttribArray(OpenGLES2stuff::gTexCoordHandle);

    glUniformMatrix4fv(OpenGLES2stuff::uMatrixLocation, 1, false,      orthoMatrix);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(OpenGLES2stuff::uTextureUnitLocation, 0);

    glDrawArrays(GL_TRIANGLES,0,6);
#endif
  }

#ifndef ANDROID
  glPopMatrix();	               
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);									      
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();	
  glPopMatrix();
#else
  glDisable(GL_SCISSOR_TEST);
#endif
}

void OpenGLstuff::drawFrameBufferNoRect(){
#ifndef ANDROID
  glMatrixMode(GL_PROJECTION);									       
  glPushMatrix();													              
  glLoadIdentity();												               
#endif
  
  float orthoMatrix[16];
  memset(orthoMatrix, 0, sizeof(orthoMatrix));
  Engine::calculateOrthoMatrix(0.0f, Engine::screenWidthGL, 0.0f, Engine::screenHeightGL, -1.0f, 1.0f, orthoMatrix);

#ifndef ANDROID
  glMultMatrixf(orthoMatrix);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
#endif

  int horizontalOffset = (Engine::screenWidthGL  - Engine::screenWidthRT)  / 2;
  int verticalOffset   = (Engine::screenHeightGL - Engine::screenHeightRT) / 2;

  // draw ray traced rectangle 
  glBindTexture(GL_TEXTURE_2D, framebufferTexID);
#ifdef ANDROID
  if(!Engine::serverUseETC1){
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Engine::screenWidthRT, Engine::screenHeightRT, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameBufferPointer);
  } else {
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, Engine::screenWidthRT, Engine::screenHeightRT, 0,	(Engine::screenWidthRT * Engine::screenHeightRT) / 2, frameBufferPointer);  
  }
#else
  // Manual decompression of ETC1 into RGBA disabled.    
  // OPTIMIZE: store copy buffers somewhere else
  // unsigned char* copyBuffer = new unsigned char[(Engine::screenWidthRT * Engine::screenHeightRT * 4) / 8];
  // memcpy(copyBuffer, frameBufferPointer, (Engine::screenWidthRT * Engine::screenHeightRT * 4) / 8);

  // Etc1::convertETC1toRGBA(copyBuffer, frameBufferPointer, Engine::screenWidthRT, Engine::screenHeightRT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Engine::screenWidthRT, Engine::screenHeightRT, 0, GL_BGRA, GL_UNSIGNED_BYTE, frameBufferPointer);  
  //delete[] copyBuffer;
#endif

  GLfloat vertices[]  = {0.0f + horizontalOffset,                  (float)Engine::screenHeightRT + verticalOffset,  
                  (float)Engine::screenWidthRT + horizontalOffset, (float)Engine::screenHeightRT + verticalOffset,
                  (float)Engine::screenWidthRT + horizontalOffset, 0.0f + verticalOffset,
                  (float)Engine::screenWidthRT + horizontalOffset, 0.0f + verticalOffset,
                  0.0f + horizontalOffset,                         0.0f + verticalOffset,  
                  0.0f + horizontalOffset,                         (float)Engine::screenHeightRT + verticalOffset };

  if(Engine::upscale){
    const float aspectRatio = (float)Engine::screenWidthRT / (float)Engine::screenHeightRT;
    float zoomRatio;
    int newHeight;
    int newWidth;
    int newVerticalOffset;
    int newHorizontalOffset;

    if(horizontalOffset > (float)verticalOffset * aspectRatio){
      // use full vertical space
      newHeight = Engine::screenHeightGL;
      zoomRatio = newHeight / (float)Engine::screenHeightRT;
      newWidth = Engine::screenWidthRT * zoomRatio;  

      newVerticalOffset = 0;
      newHorizontalOffset = (Engine::screenWidthGL  - newWidth) / 2;
    } else {
      // use full horizontal space     
      newWidth = Engine::screenWidthGL;
      zoomRatio = newWidth / (float)Engine::screenWidthRT;
      newHeight = Engine::screenHeightRT * zoomRatio;      

      newVerticalOffset = (Engine::screenHeightGL  - newHeight) / 2;
      newHorizontalOffset = 0; 
    }

    vertices[0]  = 0.0f + newHorizontalOffset;                         vertices[1]  = newHeight + newVerticalOffset; 
    vertices[2]  = newWidth + newHorizontalOffset;                     vertices[3]  = newHeight + newVerticalOffset;  
    vertices[4]  = newWidth + newHorizontalOffset;                     vertices[5]  = 0.0f + newVerticalOffset;
    vertices[6]  = newWidth + newHorizontalOffset;                     vertices[7]  = 0.0f + newVerticalOffset;   
    vertices[8]  = 0.0f + newHorizontalOffset;                         vertices[9]  = 0.0f + newVerticalOffset;  
    vertices[10] = 0.0f + newHorizontalOffset;                         vertices[11] = newHeight + newVerticalOffset;
  }

  GLfloat texCoords[] = {0.0f,1.0f,  1.0f,1.0f,  1.0f,0.0f,   1.0f,0.0f,   0.0f,0.0f,  0.0f,1.0f};

#ifndef ANDROID
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glVertexPointer(2, GL_FLOAT, 0,vertices);
  glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

  glDrawArrays(GL_TRIANGLES,0,6);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
  glUseProgram(OpenGLES2stuff::programID);

  glVertexAttribPointer(OpenGLES2stuff::gPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  glEnableVertexAttribArray(OpenGLES2stuff::gPositionHandle);

  glVertexAttribPointer(OpenGLES2stuff::gTexCoordHandle, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
  glEnableVertexAttribArray(OpenGLES2stuff::gTexCoordHandle);

  glUniformMatrix4fv(OpenGLES2stuff::uMatrixLocation, 1, false, orthoMatrix);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(OpenGLES2stuff::uTextureUnitLocation, 0);
   
  glDrawArrays(GL_TRIANGLES,0,6); 
#endif

#ifndef ANDROID
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
#endif
}


void OpenGLstuff::drawHUD(){
  Engine::buttonLeftCenter.x = 150.0f;
  Engine::buttonLeftCenter.y = 100.0f;

  Engine::buttonRightCenter.x = 400.0f;
  Engine::buttonRightCenter.y = 100.0f;

  // lenovo 1920x1080 screen
  if (Engine::screenHeightGL == 1920 && Engine::screenWidthGL == 1080){
    Engine::buttonLeftCenter.x = 175.0f;
    Engine::buttonLeftCenter.y = 175.0f;

    Engine::buttonRightCenter.x = Engine::screenWidthGL - 200.0f;
    Engine::buttonRightCenter.y = 175.0f;
  }

  if (Engine::screenWidthGL > 512){
    minclient::Font::glPrint(Engine::buttonLeftCenter.x, Engine::buttonLeftCenter.y + Engine::fontSize, " o ", false);
    minclient::Font::glPrint(Engine::buttonLeftCenter.x, Engine::buttonLeftCenter.y, "ooo", false);
    minclient::Font::glPrint(Engine::buttonLeftCenter.x, Engine::buttonLeftCenter.y - Engine::fontSize, " o ", false);
  }
}

void OpenGLstuff::swapBuffers(){
  SDL_GL_SwapWindow(mainWindow);
}

void OpenGLstuff::drawFPS(){
  const int width = Engine::screenWidthGL;
  int height = Engine::screenHeightGL;

  int widthToPrint = width - width/10;
  if(width < 512)
    widthToPrint = width - width/4;

  if(height <= 256 && height > 240)
    height = height - 10;

  minclient::Font::glPrint(widthToPrint, height - height/24, Engine::strFrameRate, 0);

  if(Engine::debugMessage[0] != 0){
    minclient::Font::glPrint(10, height - (2 * height) / 24, Engine::debugMessage, 0);
  }
}

bool OpenGLstuff::isETCSupported(){
  int count = 0;
  int GLEW_OES_compressed_ETC1_RGB8_texture = 0;

  glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count);

  if(count > 0){
    GLint* formats = (GLint*)calloc(count, sizeof(GLint));
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats);

    int index;
    for(index = 0; index < count; index++){
      switch(formats[index]){
        case GL_ETC1_RGB8_OES:
          GLEW_OES_compressed_ETC1_RGB8_texture = 1;
          break;
      }
    }

    free(formats);
  }

  if(GLEW_OES_compressed_ETC1_RGB8_texture)
    return true;
  else
    return false;
}

bool OpenGLstuff::isPVRTCSupported(){
  bool PVRTCSupported = false;

  const GLubyte* pExtensions = glGetString(GL_EXTENSIONS);
  PVRTCSupported = (strstr((char*)pExtensions, "GL_IMG_texture_compression_pvrtc") != NULL);

  return PVRTCSupported;
}

bool OpenGLstuff::isFrontBufferSupported(){
  bool frontBufferSupported = false;

  const GLubyte* pExtensions = glGetString(GL_EXTENSIONS);
  frontBufferSupported = (strstr((char*)pExtensions, "egl_GVR_FrontBuffer") != NULL);

  return frontBufferSupported;
}

bool OpenGLstuff::isPVRTC2Supported(){
  bool PVRTC2Supported = false;

  const GLubyte* pExtensions = glGetString(GL_EXTENSIONS);
  PVRTC2Supported = (strstr((char*)pExtensions, "GL_IMG_texture_compression_pvrtc2") != NULL);

  return PVRTC2Supported;
}

bool OpenGLstuff::isS3TCSupported(){
  bool S3TCSupported = false;

  const GLubyte* pExtensions = glGetString(GL_EXTENSIONS);
  S3TCSupported = (strstr( (char*)pExtensions, "GL_EXT_texture_compression_s3tc" ) != NULL);
  
  return S3TCSupported;
}

bool OpenGLstuff::isDXT1Supported(){
  bool DXT1Supported = false;

  const GLubyte* pExtensions = glGetString(GL_EXTENSIONS);
  DXT1Supported = (strstr( (char*)pExtensions, "GL_EXT_texture_compression_dxt1" ) != NULL);
  
  return DXT1Supported;
}

void OpenGLstuff::printCompressedTextureAvailability(){
  char output[64];
  strcpy(output, "Comp. tex: ");
  if(isETCSupported()){
    strcat(output, "ETC1 ");
  }
  if(isPVRTCSupported()){
    strcat(output, "PVRTC ");
  }
  if(isPVRTC2Supported()){
    strcat(output, "PVRTC2 ");
  }
  if(isS3TCSupported()){
    strcat(output, "S3TC ");
  }
  if(isDXT1Supported()){
    strcat(output, "DXT1 ");
  }

  minclient::Font::glPrint(10, minclient::Font::AUTO, output, true);

  if (isFrontBufferSupported()){
    strcpy(output, "FrontBuffer EXT supported.");
    minclient::Font::glPrint(10, minclient::Font::AUTO, output, true);
  }

  SDL_Delay(1000);

  // here we check for ETC1 support. some mobile devices support it, but don't list it.
  // so if we don't find it, we will just try to continue.
  if(Engine::serverUseETC1){
    if(!isETCSupported()){
      minclient::Font::glPrint(10, minclient::Font::AUTO, "Trying to use ETC1, but not supported in hardware.", true);
#ifdef ANDROID      
      //SDL_Delay(2000);
      //exit(1);
#endif
    }
  }
}
