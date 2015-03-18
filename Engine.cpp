#include "Engine.h"

TCPsocket Engine::socketDescriptor[MAX_SERVERS];
IPaddress Engine::serverIP[MAX_SERVERS];

int Engine::rectLeftServer[MAX_SERVERS];
int Engine::rectTopServer[MAX_SERVERS];
int Engine::rectRightServer[MAX_SERVERS];
int Engine::rectBottomServer[MAX_SERVERS];
int Engine::rectSizeX[MAX_SERVERS];
int Engine::rectSizeY[MAX_SERVERS];

bool Engine::rectMode = false;

int Engine::numAccelerometerHits = 0;

#if 0
  int Engine::screenWidthRT = 2560;  // needs to be divisable by RENDERTILE_SIZE
  int Engine::screenHeightRT = 1440;  // needs to be divisable by RENDERTILE_SIZE
  int Engine::screenWidthGL = 2560;
  int Engine::screenHeightGL = 1440;
#else
  #if 0
    int Engine::screenWidthRT = 1920;  
    int Engine::screenHeightRT = 1080;  
    int Engine::screenWidthGL = 1920;
    int Engine::screenHeightGL = 1080;
  #else
    #if 1
      int Engine::screenWidthRT = 1280;  
      int Engine::screenHeightRT = 720;  
      int Engine::screenWidthGL = 1280;
      int Engine::screenHeightGL = 720;
    #else
      #if 0
        int Engine::screenWidthRT = 240;  
        int Engine::screenHeightRT = 240;  
        int Engine::screenWidthGL = 240;
        int Engine::screenHeightGL = 240;
      #else
        int Engine::screenWidthRT = 512;  
        int Engine::screenHeightRT = 256;  
        int Engine::screenWidthGL = 512;
        int Engine::screenHeightGL = 256;
      #endif
    #endif
  #endif
#endif

int Engine::serverFrameBuffers = 0; 

int Engine::clicksPerSecond = 0;

// keys & mouse
bool Engine::upKey = false;
bool Engine::downKey = false; 
bool Engine::leftKey = false; 
bool Engine::rightKey = false;
//bool Engine::leftButton = false;
//bool Engine::rightButton = false;
bool Engine::jumpKey = false;
bool Engine::crouchKey = false;

bool Engine::upscale = true;

CVector2 Engine::relativeMouseMotion;
  
bool Engine::forceScreenResolution = false;
bool Engine::done = false;

int Engine::fontSize = 20;

bool Engine::fullscreen = false;

float Engine::angle = 90.0f;

float Engine::currentTime = 0.0f;
float Engine::currentTimeLastFrame = 0.0f;
float Engine::timeWhenMapWasLoaded = 0.0f;
float Engine::sensitivity = 3.0f;
float Engine::frameTime = 0.0f;
float Engine::framesPerSecond = 0.0f;		

char* Engine::startMap = NULL;

char* Engine::serverName[MAX_SERVERS];
int Engine::serverPort[MAX_SERVERS];
int Engine::numServers = 0;

float Engine::frameInterval = 0.3f;
int Engine::numFramesRendered = 0;

float Engine::lastTime = 0.0f;				// This will hold the time from the last frame	

char Engine::strFrameRate[32]; 

bool Engine::serverUseETC1 = false;

CVector2 Engine::buttonLeftCenter;
CVector2 Engine::buttonRightCenter;
CVector2 Engine::buttonCenter;

CVector2 Engine::buttonPressed;

char Engine::debugMessage[64];

void Engine::init(){
  for(int i = 0; i < MAX_SERVERS; i++){
    serverName[i] = NULL;
    serverPort[i] = PORT_TO_SERVER;

    rectLeftServer[i] = 0;
    rectTopServer[i] = 0;
    rectRightServer[i] = 0;
    rectBottomServer[i] = 0;

    rectSizeX[i] = rectSizeY[i] = 0;
  }

  sprintf(strFrameRate, "0fps");

  debugMessage[0] = 0;

  parseConfigFile();	

  buttonPressed = CVector2(-666.6f, -666.6f);
  buttonCenter = CVector2(520.0f, 175.0f);

  for(int i = 0; i < numServers; i++){
    rectSizeX[i] = rectRightServer[i] - rectLeftServer[i] + 1;
    rectSizeY[i] = rectTopServer[i] - rectBottomServer[i] + 1;    
  }

  if(numServers >= 1){
    // only check server 0
    if(rectSizeX[0] > 0 && rectSizeX[0] < Engine::screenWidthRT && rectSizeY[0] > 0 && rectSizeY[0] <= Engine::screenHeightRT){
      const int RENDERTILE_SIZE = 4;
      int remainingPixelsX = rectSizeX[0] % RENDERTILE_SIZE;
      int remainingPixelsY = rectSizeY[0] % RENDERTILE_SIZE;

      if(remainingPixelsX == 0 && remainingPixelsY == 0){
        Engine::rectMode = true;
        
        for(int i = 0; i < numServers; i++){
          cout << "Rendering Rect enabled: X (" << Engine::rectLeftServer[i] << " - " << Engine::rectRightServer[i] << ")   Y (" << Engine::rectBottomServer[i] << " - " << Engine::rectTopServer[i] << "). " 
            << rectSizeX[i] << "x" << rectSizeY[i] << endl;
        }
      }
    }
  }

  // seed the randomizer
  srand(currentTime);	

  cout << "Engine initialized." << endl;
}

void Engine::parseConfigFile(){
  // instead of parsing a real file we just hardcode everything
  forceScreenResolution = false;
  fullscreen = false;

  serverFrameBuffers = 1; 
  startMap = (char*)"dan3";
  
  serverName[0] = (char*)"192.168.0.100";
  serverPort[0] = 2000;

  // config file available?
  FILE* in = fopen(CONFIG_FILE, "rb");
  if(!in){
    cout << "Could not open config file " << CONFIG_FILE << endl;
    cout << "Using default values. Trying to create new config file." << endl;

    FILE* out = fopen(CONFIG_FILE, "wb");
    if(!out)
      return;

    fprintf(out, "serverUseETC1   true\n");

    fprintf(out, "server0 %s %i\n\n", serverName[0], serverPort[0]);
    fprintf(out, "#server1 192.168.0.3 2000\n");
    fprintf(out, "#server2 192.168.0.4 2000\n");
    fprintf(out, "#server3 192.168.0.5 2000\n\n");

    // settings for rendering in 1024x1024
    fprintf(out, "##### 4 servers\n");
    fprintf(out, "#rectLeftServer0 0\n");
    fprintf(out, "#rectRightServer0 255\n");
    fprintf(out, "#rectBottomServer0 0\n");
    fprintf(out, "#rectTopServer0 1023\n\n");

    fprintf(out, "#rectLeftServer1 256\n");
    fprintf(out, "#rectRightServer1 511\n");
    fprintf(out, "#rectBottomServer1 0\n");
    fprintf(out, "#rectTopServer1 1023\n\n");

    fprintf(out, "#rectLeftServer2 512\n");
    fprintf(out, "#rectRightServer2 767\n");
    fprintf(out, "#rectBottomServer2 0\n");
    fprintf(out, "#rectTopServer2 1023\n\n");

    fprintf(out, "#rectLeftServer3 768\n");
    fprintf(out, "#rectRightServer3 1023\n");
    fprintf(out, "#rectBottomServer3 0\n");
    fprintf(out, "#rectTopServer3 1023\n\n");

    fprintf(out, "\n##### 2 servers\n");
    fprintf(out, "#rectLeftServer0 0\n");
    fprintf(out, "#rectRightServer0 511\n");    
    fprintf(out, "#rectBottomServer0 0\n\n");
    fprintf(out, "#rectTopServer0 1023\n");   

    fprintf(out, "#rectLeftServer1 512\n");
    fprintf(out, "#rectRightServer1 1023\n");
    fprintf(out, "#rectBottomServer1 0\n\n"); 
    fprintf(out, "#rectTopServer1 1023\n\n");   

    fprintf(out, "startMap dan3\n");

    fclose(out);
    return;
  }

  char* firstWord = NULL; 
  char* secondWord = NULL; 
  char* thirdWord = NULL;
  bool fileIsOver = false;
  int i; 
  char buffer[1024];
  char c = fgetc(in);	
  while(!feof(in) && !fileIsOver){
    for(i = 0; (c != 0x0A) && (i < 1023) && !fileIsOver; i++){
      buffer[i] = c;
      c = fgetc(in);
      if(feof(in))
        fileIsOver = true;
    }
    c = fgetc(in);

    if(feof(in))
      fileIsOver = true;
    buffer[i] = '\0';
    CTextParser::eliminateEmptySpaceBeforeContent(buffer);
    
    // don't add empty lines
    if(buffer[0] == '\0')
      continue;

    // don't add comment lines
    if(buffer[0] == '/')
      continue;
    
    CTextParser::eliminateReturnAtEndOfLine(buffer);
    
    // don't add empty lines (after killing return character)
    if(buffer[0] == '\0')
      continue;

    firstWord = CTextParser::getFirstWord(buffer);
    secondWord = CTextParser::getSecondWord(buffer);
    thirdWord =  CTextParser::getThirdWord(buffer);
    
    if(!secondWord)
      continue;


    // 
    // serverUseETC1
    // 
    else if(!strcmp(firstWord, "serverUseETC1")){
      if(!strcmp(secondWord, "true")){
        serverUseETC1 = true;
      }	else{
        serverUseETC1 = false;
      }
    }

    // 
    // startMap
    // 
    else if(!strcmp(firstWord, "startMap")){
      startMap = new char[strlen(secondWord) + 1];
      strcpy(startMap, secondWord);
    }

    // 
    // server0
    // server1
    // ...
    //
    for(int i = numServers; i < MAX_SERVERS; i++){
      char searchString[128];
      sprintf(searchString, "server%i", i);

      if(!strcmp(firstWord, searchString)){
        serverName[i] = new char[strlen(secondWord) + 1];
        strcpy(serverName[i], secondWord);

        if(thirdWord){
          serverPort[i] = atoi(thirdWord);
        }

        numServers = i + 1;
      } else {				
        break;
      }
    }

    // 
    // rectLeftServer0, rectLeftServer1, ...
    //
    for(int i = 0; i < MAX_SERVERS; i++){
      char searchString[128];
      sprintf(searchString, "rectLeftServer%i", i);

      if(!strcmp(firstWord, searchString)){
        rectLeftServer[i] = atoi(secondWord);
      }
    }

    // 
    // rectTopServer0, rectTopServer1, ...
    //
    for(int i = 0; i < MAX_SERVERS; i++){
      char searchString[128];
      sprintf(searchString, "rectTopServer%i", i);

      if(!strcmp(firstWord, searchString)){
        rectTopServer[i] = atoi(secondWord);
      }
    }

    // 
    // rectRightServer0, rectRightServer1, ...
    //
    for(int i = 0; i < MAX_SERVERS; i++){
      char searchString[128];
      sprintf(searchString, "rectRightServer%i", i);

      if(!strcmp(firstWord, searchString)){
        rectRightServer[i] = atoi(secondWord);
      }
    }

    // 
    // rectBottomServer0, rectBottomServer1, ...
    //
    for(int i = 0; i < MAX_SERVERS; i++){
      char searchString[128];
      sprintf(searchString, "rectBottomServer%i", i);

      if(!strcmp(firstWord, searchString)){
        rectBottomServer[i] = atoi(secondWord);
      }
    }

    delete [] firstWord;
    delete [] secondWord;
  }

  fclose(in);

  // there has been at least one defined on top
  if(numServers == 0)
    numServers = 1;
}


void Engine::calculateFrameRate(){
  float totalCurrentTime = SDL_GetTicks() * 0.001f;
  float localCurrentTime = totalCurrentTime - timeWhenMapWasLoaded;
              
  // save old time for stereoscopic rendering where both frames need to have the same time
  currentTimeLastFrame = currentTime;
  currentTime = localCurrentTime;                              

  frameInterval = currentTime - frameTime;
  frameTime = currentTime;

  framesPerSecond++;

  if( currentTime - lastTime > 1.0f ){
    lastTime = currentTime;
    
    sprintf(strFrameRate, "%dfps", int(framesPerSecond));

    framesPerSecond = 0;
    clicksPerSecond = 0;
  }

  updateGlobalSinus();
}


void Engine::updateGlobalSinus(){
  if(angle > 360.0f)
    angle = 0.0f;

  angle += frameInterval / 10.0f;
}

// code snippet calculateOrthoMatrix() from http://stackoverflow.com/questions/9690006/glfrustrum-variant-in-opengl-es-2-0-how-fix-blank-screen
// author: Aristarhys
// license: http://creativecommons.org/licenses/by-sa/3.0/
void Engine::calculateOrthoMatrix(float left, float right, float bottom, float top, float near_, float far_, float result[16]){
  result[0] = 2.0f / (right - left);
  result[1] = 0.0f;
  result[2] = 0.0f;
  result[3] = 0.0f;

  // second column
  result[4] = 0.0f;
  result[5] = 2.0f / (top - bottom);
  result[6] = 0.0f;
  result[7] = 0.0f;

  // third column
  result[8] = 0.0f;
  result[9] = 0.0f;
  result[10] = -2.0f / (far_ - near_);
  result[11] = 0.0f;

  // fourth column
  result[12] = -(right + left) / (right - left);
  result[13] = -(top + bottom) / (top - bottom);
  result[14] = -(far_ + near_) / (far_ - near_);
  result[15] = 1;
}
