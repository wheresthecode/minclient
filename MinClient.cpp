#include "MinClient.h"

int main(int argc, char *argv[]){
  Engine::init();

  Camera camera;
  SDLstuff sdlstuff;
  OpenGLstuff openglstuff(sdlstuff.getMainWindow());	
  OpenGLES2stuff::init();

  minclient::Font::init(sdlstuff.getMainWindow());

  sdlstuff.tweakFontSize();
  sdlstuff.printResolution();

  openglstuff.printCompressedTextureAvailability();

  NetworkStuff networkStuff(&camera, &openglstuff);

  // main game loop
  while(!Engine::done)  {
    doGameLoop(&sdlstuff, &camera, &openglstuff, &networkStuff);
  }

  sdlstuff.closeSDL_Net();

  sdlstuff.ungrabKeyAndMouse();

  cout << "MinClient: Leaving now..." << endl;  

  // FIXME: without the exit here it hangs. need to find out what causes this.
  exit(1);

  return 0;
}

void doGameLoop(SDLstuff* sdlstuff, Camera* camera, OpenGLstuff* openglstuff, NetworkStuff* networkStuff){	
  sdlstuff->checkEvents();	
  Engine::calculateFrameRate();
  camera->update();

#ifdef NETWORK_ON
  networkStuff->sendMessageToRenderServers();  
  networkStuff->receiveMessageFromRenderServer();
#endif

  openglstuff->render();
  openglstuff->swapBuffers();      

  Engine::increaseNumFramesRendered();
}
