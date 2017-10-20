
#include "Pokitto.h"
#include "sens_levs.h"
#include "gfx.h"

Pokitto::Core game;

#define OFF_X 2
#define OFF_Y 42
#define LEVWIDTH 18
#define LEVHEIGHT 11
#define SCROLLHEIGHT 112
#define RUMBLEFRAMES 15
#define STEPSIZE 1
#define EXPLODESPRITES 15
#define EXPLODESPEED 5

byte oldLev[LEVWIDTH * LEVHEIGHT];
byte curLev[LEVWIDTH * LEVHEIGHT];
byte teleportMap[LEVWIDTH * LEVHEIGHT];
byte telArray[32];
char satTime = 0;
int numTiles;
int px,py,ps,pd;
int levelNum=0;
int maxLevels=sizeof(levels)/102;
int tileSize=12;
uint32_t frameNumber=0;
int score=0;
int hiscore=0;
bool stillExploding=0;
int telNum = 0;
byte gameMode;
unsigned int myInt=0;
byte scroller=0;
char musicName[] = "sensitiv.raw";
int lives;

uint32_t myDelay;
uint32_t tempTime;

// explosion
uint8_t expX[EXPLODESPRITES], expY[EXPLODESPRITES], expF[EXPLODESPRITES], expU[EXPLODESPRITES];
uint8_t explode = 0;

void rumbleOn(){
    DigitalOut outPin(P0_13);
    DigitalOut inPin(P0_11);
    inPin=0;
    outPin=1;
}
void rumbleOff(){
    DigitalOut outPin(P0_13);
    DigitalOut inPin(P0_11);
    inPin=0;
    outPin=0;
}

/**************************************************************************/
#define HELD 0
#define NEW 1
#define RELEASE 2
byte CompletePad, ExPad, TempPad, myPad;
bool _A[3], _B[3], _C[3], _Up[3], _Down[3], _Left[3], _Right[3];

DigitalIn _aPin(P1_9);
DigitalIn _bPin(P1_4);
DigitalIn _cPin(P1_10);
DigitalIn _upPin(P1_13);
DigitalIn _downPin(P1_3);
DigitalIn _leftPin(P1_25);
DigitalIn _rightPin(P1_7);

void UPDATEPAD(int pad, int var) {
  _C[pad] = (var >> 1)&1;
  _B[pad] = (var >> 2)&1;
  _A[pad] = (var >> 3)&1;
  _Down[pad] = (var >> 4)&1;
  _Left[pad] = (var >> 5)&1;
  _Right[pad] = (var >> 6)&1;
  _Up[pad] = (var >> 7)&1;
}

void UpdatePad(int joy_code){
  ExPad = CompletePad;
  CompletePad = joy_code;
  UPDATEPAD(HELD, CompletePad); // held
  UPDATEPAD(RELEASE, (ExPad & (~CompletePad))); // released
  UPDATEPAD(NEW, (CompletePad & (~ExPad))); // newpress
}

byte updateButtons(byte var){
   var = 0;
   if (_cPin) var |= (1<<1);
   if (_bPin) var |= (1<<2);
   if (_aPin) var |= (1<<3); // P1_9 = A
   if (_downPin) var |= (1<<4);
   if (_leftPin) var |= (1<<5);
   if (_rightPin) var |= (1<<6);
   if (_upPin) var |= (1<<7);

   return var;
}

/**************************************************************************/

void new2BitTile(int x1, int y1, int wide, int high, int tile, const uint16_t *gfx, int palNumber){

    uint16_t tileBuffer[wide*high];

    uint8_t pix;
    uint8_t quartWide=wide/4;
    uint8_t pic;

    int palNo = palLookup[palNumber]*4;
    for(int y=0; y<high; y++){
        for(int x=0; x<quartWide; x++){
            pic = gfx[(tile*quartWide*high)+(x+quartWide*y)];
            pix = (pic >> 6)&3; tileBuffer[y+high*(  x*4)]=_pal[_miniPal[palNo+pix]];
            pix = (pic >> 4)&3; tileBuffer[y+high*(1+x*4)]=_pal[_miniPal[palNo+pix]];
            pix = (pic >> 2)&3; tileBuffer[y+high*(2+x*4)]=_pal[_miniPal[palNo+pix]];
            pix = pic &3;       tileBuffer[y+high*(3+x*4)]=_pal[_miniPal[palNo+pix]];
        }
    }

    game.display.directTile(x1, y1, x1+wide, y1+high, tileBuffer);
}

void new4BitTile(int x1, int y1, int wide, int high, int tile, int transparentColor, const uint8_t *gfx){
    uint16_t tileBuffer[wide*high];
    int pix1,pix2;
    int halfWide=wide/2;
    for(int y=0; y<high; y++){
        for(int x=0; x<halfWide; x++){
            pix1 = gfx[(tile*halfWide*high)+(x+halfWide*y)]>>4;
            pix2 = gfx[(tile*halfWide*high)+(x+halfWide*y)]&15;
            tileBuffer[y+high*(  x*2)]=_pal[pix1];
            tileBuffer[y+high*(1+x*2)]=_pal[pix2];
        }
    }
    game.display.directTile(x1, y1, x1+wide, y1+high, tileBuffer);
}


/**************************************************************************/
/**************************************************************************/


int RandMinMax(int min, int max){
    return rand() % max + min;
}

void refreshDisplay(){
    // place holder to replace a simulator command.
}

void draw4BitTile(int x1, int y1, int wide, int high, int tile, int transparentColor, const uint8_t *gfx){
    int pix1,pix2;
    int halfWide=wide/2;
    for(int y=0; y<high; y++){
        for(int x=0; x<halfWide; x++){
            pix1 = gfx[(tile*halfWide*high)+(x+halfWide*y)]>>4;
            pix2 = gfx[(tile*halfWide*high)+(x+halfWide*y)]&15;
            if(pix1 != transparentColor){game.display.directPixel(x1+(x*2),y1+y,_pal[pix1]);}
            if(pix2 != transparentColor){game.display.directPixel(1+x1+(x*2),y1+y,_pal[pix2]);}
        }
    }
}

void draw2BitTile(int x1, int y1, int wide, int high, int tile, int transparentColor, const uint8_t *gfx, int palNumber, int palReplace = -1){
    uint8_t pix;
    uint8_t quartWide=wide/4;
    uint8_t pic;

    int palNo = palLookup[palNumber]*4;
    for(int y=0; y<high; y++){
        for(int x=0; x<quartWide; x++){
            pic = gfx[(tile*quartWide*high)+(x+quartWide*y)];
            pix = (pic >> 6)&3; if(pix != transparentColor){game.display.directPixel(  x1+(x*4),y1+y,_pal[_miniPal[palNo+pix]]);}
            pix = (pic >> 4)&3; if(pix != transparentColor){game.display.directPixel(1+x1+(x*4),y1+y,_pal[_miniPal[palNo+pix]]);}
            pix = (pic >> 2)&3; if(pix != transparentColor){game.display.directPixel(2+x1+(x*4),y1+y,_pal[_miniPal[palNo+pix]]);}
            pix = pic &3;       if(pix != transparentColor){game.display.directPixel(3+x1+(x*4),y1+y,_pal[_miniPal[palNo+pix]]);}
        }
    }
}


void drawFont(int x1, int y1, int wide, int high, int tile, int transparentColor, const uint8_t *gfx, int palReplace = -1){
    uint16_t tileBuffer[(wide+1)*(high+1)];

    uint8_t pix;
    uint8_t quartWide=wide/4;
    uint8_t pic;

    for(int y=0; y<high; y++){
        for(int x=0; x<quartWide; x++){
            pic = gfx[(tile*quartWide*high)+(x+quartWide*y)];
            pix = (pic >> 6)&3; if(pix != transparentColor){tileBuffer[y+high*(  x*4)]=numbers_pal[pix];}else{tileBuffer[y+high*(  x*4)]=palReplace;}
            pix = (pic >> 4)&3; if(pix != transparentColor){tileBuffer[y+high*(1+x*4)]=numbers_pal[pix];}else{tileBuffer[y+high*(1+x*4)]=palReplace;}
            pix = (pic >> 2)&3; if(pix != transparentColor){tileBuffer[y+high*(2+x*4)]=numbers_pal[pix];}else{tileBuffer[y+high*(2+x*4)]=palReplace;}
            pix = pic &3;       if(pix != transparentColor){tileBuffer[y+high*(3+x*4)]=numbers_pal[pix];}else{tileBuffer[y+high*(3+x*4)]=palReplace;}
        }
    }

    game.display.directTile(x1, y1, x1+wide, y1+high, tileBuffer);

}

// print text
void print(uint16_t x, uint16_t y, const char* text, uint8_t bgCol, signed int repCol=-1){
    if(repCol==-1)repCol=bgCol;
    for(uint8_t t = 0; t < strlen(text); t++){
        uint8_t character = text[t]-32;
        drawFont(x, y, 8, 8, character, bgCol, myFont, repCol);
        x += 8;
    }
}



void loadLevel(int number) {
/*
    // store level number in file for testing
   FILE *fp;
   fp = fopen("test.txt", "w+");
   fwrite(&number, sizeof(int),1, fp);
   fclose(fp);
*/

  numTiles = 0;
  satTime = 0;
  int numTels = 0;

  int amount = (LEVWIDTH/2)*LEVHEIGHT;

  for (int t = amount-1; t >= 0; t--) {
    int s = pgm_read_byte(levels + t + ((amount+3) * number));
    curLev[t * 2] = pgm_read_byte(lookUpTile + (s >> 4));
    curLev[(t * 2) + 1] = pgm_read_byte(lookUpTile + (s & 15));
    s = t * 2;
    teleportMap[s] = 0;
    teleportMap[s+1] = 0;

    switch (curLev[s]) {
      case 1:
        numTiles++;
        break;
      case 2:
        numTiles += 2;
        break;
      case 4:
        curLev[s + 1] = 5;
        curLev[s + LEVWIDTH] = 6;
        curLev[s + LEVWIDTH+1] = 7;
        break;
      case 8:
        curLev[s + 1] = 9;
        break;
      case 10:
        curLev[s + 1] = 11;
        break;
      case 12:
        curLev[s + LEVWIDTH] = 13;
        break;
    }

    s = (t * 2) + 1;
    switch (curLev[s]) {
      case 1:
        numTiles++;
        break;
      case 2:
        numTiles += 2;
        break;
      case 4:
        curLev[s + 1] = 5;
        curLev[s + LEVWIDTH] = 6;
        curLev[s + LEVWIDTH+1] = 7;
        break;
      case 8:
        curLev[s + 1] = 9;
        break;
      case 10:
        curLev[s + 1] = 11;
        break;
      case 12:
        curLev[s + LEVWIDTH] = 13;
        break;
    }

  }


  px = pgm_read_byte(levels + amount + ((amount+3) * number))*tileSize;
  py = pgm_read_byte(levels + amount+1 + ((amount+3) * number))*tileSize;
  int telNumber = pgm_read_byte(levels + amount+2 + ((amount+3) * number));
  ps = 0; pd = 0;
  satTime = 0;

  for(int t=0; t<31; t++){
    telArray[t] = pgm_read_byte(telPath + t + (telNumber*32));
  }

    // teleport locations
    numTels=0;
    for (int y = 0; y <LEVHEIGHT; y++) {
      for (int x = 0; x <LEVWIDTH ; x++) {
        int tn = (y * LEVWIDTH) + x;
        if(curLev[tn]==16){
            teleportMap[tn] = numTels;
            numTels++;
        }
      }
    }


  // draw part of the screen here
  // bad practice I know, but so what!

    for(byte t=0; t<LEVWIDTH * LEVHEIGHT; t++){
        oldLev[t] = 255;
    }

    for (int x = LEVWIDTH-1; x >= 0 ; x--) {
        new2BitTile(OFF_X+x*tileSize, 42, tileSize, tileSize, 0, gbTiles, 0);
    }

    game.display.directRectangle(OFF_X,0,218,40,_pal[10]);
    game.display.directRectangle(OFF_X,0,218,0,_pal[4]);
    game.display.directRectangle(OFF_X,1,218,1,_pal[10]);
    game.display.directRectangle(OFF_X,2,218,2,_pal[15]);
    game.display.directRectangle(OFF_X,3,218,3,_pal[15]);
    game.display.directRectangle(OFF_X,4,218,4,_pal[10]);
    game.display.directRectangle(OFF_X,5,218,5,_pal[4]);
    game.display.directRectangle(OFF_X,36,218,36,_pal[4]);
    game.display.directRectangle(OFF_X,37,218,37,_pal[10]);
    game.display.directRectangle(OFF_X,38,218,38,_pal[15]);
    game.display.directRectangle(OFF_X,39,218,39,_pal[15]);
    game.display.directRectangle(OFF_X,40,218,40,_pal[10]);
    game.display.directRectangle(OFF_X,41,218,41,_pal[4]);


    // pause if any buttons held
    while(_Up[HELD] || _Down[HELD] || _Left[HELD] || _Right[HELD] || _A[HELD] || _B[HELD] || _C[HELD]){
      myPad = updateButtons(myPad);
      UpdatePad(myPad);
    }


}

void renderLevel(bool drawPlayer = 1, bool fullLevel=0){

    signed int xStart=(px / tileSize)+4;
    signed int yStart=(py / tileSize)+4;
    signed int xEnd=(px / tileSize)-4;
    signed int yEnd=(py / tileSize)-4;
    if(xEnd<0){xStart+=-xEnd; xEnd=0;}
    if(yEnd<0){yStart+=-yEnd; yEnd=0;}
    if(xStart>LEVWIDTH-1){xStart=LEVWIDTH-1; xEnd=xStart-9;}
    if(yStart>LEVHEIGHT-1){yStart=LEVHEIGHT-1; yEnd=yStart-9;}

//  if(fullLevel){
        xStart=LEVWIDTH-1;
        yStart=LEVHEIGHT-1;
        xEnd=0;
        yEnd=1;
//  }


//        oldLev[t] = curLev[t];


 //   int transp=-1;
    for (int y = yStart; y >= yEnd; y--) {
      for (int x = xStart; x >= xEnd; x--) {
        int tn = (y * LEVWIDTH) + x;
            if(oldLev[tn] != curLev[tn]){
                oldLev[tn] = curLev[tn];
                if(curLev[tn]==0){
                    // water tile with shaddow
                    uint8_t tile = 0;
                    if(y>0){
                        if(curLev[((y-1) * LEVWIDTH) + (x)]){ tile += 1; }
                        if(x>0){if(curLev[((y-1) * LEVWIDTH) + (x-1)]){ tile += 2; }}
                    }
                    if(x>0){if(curLev[((y) * LEVWIDTH) + (x-1)]){ tile += 4; }}
                    new2BitTile(OFF_X+x*tileSize, OFF_Y+y*tileSize, tileSize, tileSize, shaddow[tile], gbTiles, shaddow[tile]);
                }else{
                    // all other tiles
                    new2BitTile(OFF_X+x*tileSize, OFF_Y+y*tileSize, tileSize, tileSize, curLev[tn], gbTiles, curLev[tn]);
                }
            }
      }


    }

        // explosions
        stillExploding=0;
        for (int t = 0; t < EXPLODESPRITES; t++) {
          if (expU[t] == 1) {
            oldLev[expX[t] + LEVWIDTH * expY[t]] = 255;

            stillExploding=1;
            if(expF[t]<=1){
                new2BitTile(OFF_X+(expX[t]*tileSize), OFF_Y+expY[t]*tileSize, tileSize, tileSize, 1, gbTiles, 1);
            }
            draw4BitTile(OFF_X+(expX[t]*tileSize), OFF_Y+expY[t]*tileSize, tileSize, tileSize, expF[t], 8, explode_tiles);
            if (frameNumber % EXPLODESPEED == 0) {
              expF[t]++;
              if (expF[t] == 8) {
                expU[t] = 0;
              }
            }
          }
        }

        // player sprite
        if(drawPlayer){
            uint16_t tileBuffer[10];
            for(byte y=0; y<8; y++){
                int offX = ball[y][0];
                int wide = ball[y][1];
                for(byte x=0; x<wide; x++){
                    tileBuffer[x]=_pal[ball[y][x+2]];
                }
                game.display.directTile(OFF_X+px+offX+2, OFF_Y+py+y+2, OFF_X+px+wide+offX+2, OFF_Y+py+3+y , tileBuffer);
            }// y
        }//drawplayer


    print(8, 14, "SCORE  LEV LIVES  HISCORE",0,_pal[10]);
    char text[] = "      ";
    sprintf(text, "%05d",score);
    print(8, 24, text,0,_pal[10]);
    sprintf(text, "%03d",levelNum+1);
    print(64, 24, text,0,_pal[10]);
    sprintf(text, "%02d",lives);
    print(104, 24, text,0,_pal[10]);
    sprintf(text, "%05d",hiscore);
    print(160, 24, text,0,_pal[10]);

}

void explodeHere(){
    for (int t = 0; t < EXPLODESPRITES; t++) {
        if (expU[t] == 0) {
            expX[t] = px / tileSize;
            expY[t] = py / tileSize;
            expF[t] = 0;
            expU[t] = 1;
            score++;
            break;
        }
    }
}

void checkTile(int x, int y) {
  int t = curLev[x + LEVWIDTH * y];
  switch (t) {
    case 1:
      curLev[x + LEVWIDTH * y] = 0;
      oldLev[x + LEVWIDTH * y] = 255;
      oldLev[(x+1) + LEVWIDTH * y] = 255;
      oldLev[(x+1) + LEVWIDTH * (y+1)] = 255;
      oldLev[x + LEVWIDTH * (y+1)] = 255;
      numTiles--;
      explodeHere();
      break;
    case 2:
      curLev[x + LEVWIDTH * y] = 1;
      numTiles--;
      score++;
      break;
  }
  if(score > 9999) {
    score = 9999;
  }
  if(score > hiscore) {
    hiscore = score;
  }
}


void movePlayer() {
    char x = px/tileSize;
    char y = py/tileSize;
      oldLev[(x-1) + LEVWIDTH * y] = 255;
      oldLev[x + LEVWIDTH * y] = 255;
      oldLev[(x+1) + LEVWIDTH * y] = 255;
      oldLev[x + LEVWIDTH * (y+1)] = 255;

  if (ps == 0) { // not moving
    // sneeky exit check
    if (curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)] == 10 || curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)] == 11) {
      if (numTiles == 0) {
        levelNum++;
        if (levelNum >= maxLevels) {
          levelNum = 0;
        }
        // make sure explosions have finished :-)
        for (int t = 0; t < EXPLODESPRITES; t++){expU[t] = 0;}
        renderLevel(); refreshDisplay();
        loadLevel(levelNum);
        renderLevel(0,1);
      }
    }
    // sneeky water check
    int p = curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)];
    if (p == 0 || (p>=24 && p<=32)) {
      lives--;
      rumbleOn();
      if (lives >= 0) {
        explodeHere();
        // make sure explosions have finished :-)
        for(frameNumber=0; frameNumber<30; frameNumber++){
            if(frameNumber>=RUMBLEFRAMES){rumbleOff();}
            renderLevel(0); // 0 to hide player
            refreshDisplay();
        }
        loadLevel(levelNum);
        renderLevel(0,1);
      } else {
        explodeHere();
        // make sure explosions have finished :-)
        for(frameNumber=0; frameNumber<30; frameNumber++){
            if(frameNumber>=RUMBLEFRAMES){rumbleOff();}
            renderLevel(0); // 0 to hide player
            refreshDisplay();
        }
        gameMode = 0; // titlescreen
      }
    }


    pd = 0;
//    if (!_B[HELD]) {
      if (_Up[HELD]) {
        if (curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)] != 14 && curLev[(px / tileSize) + LEVWIDTH * ((py-tileSize) / tileSize)] != 14) {
          pd = 1;
          checkTile(px / tileSize, py / tileSize);
        }
      }
      if (_Down[HELD]) {
        if (curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)] != 14 && curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)] != 14) {
          pd = 2;
          checkTile(px / tileSize, py / tileSize);
        }
      }
      if (_Left[HELD]) {
        if (curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)] != 15 && curLev[((px-tileSize) / tileSize) + LEVWIDTH * (py / tileSize)] != 15) {
          pd = 3;
          checkTile(px / tileSize, py / tileSize);
        }
      }
      if (_Right[HELD]) {
        if (curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)] != 15 && curLev[((px+tileSize) / tileSize) + LEVWIDTH * (py / tileSize)] != 15) {
          pd = 4;
          checkTile(px / tileSize, py / tileSize);
        }
      }
      if (_A[NEW]) {
        if (curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)] == 16) { // teleport
          int t = (px / tileSize) + LEVWIDTH * (py / tileSize);
          telNum = teleportMap[t];
          px = (telArray[telNum]%LEVWIDTH)*tileSize;
          py = (telArray[telNum]/LEVWIDTH)*tileSize;
          renderLevel(1,1);
        }
      }

  }

  switch (pd) {
    case 0:
      break;
    case 1:
      py-=STEPSIZE;
      ps+=STEPSIZE;
      break;
    case 2:
      py+=STEPSIZE;
      ps+=STEPSIZE;
      break;
    case 3:
      px-=STEPSIZE;
      ps+=STEPSIZE;
      break;
    case 4:
      px+=STEPSIZE;
      ps+=STEPSIZE;
      break;
  }
  if (ps == tileSize) {
    ps = 0; pd = 0;
    satTime = 0;
  }

}

void drawTitleScreen(){
    game.display.directRectangle(0,0,220,176,_pal[0]); // clear the screen
    draw4BitTile(4, 41, 212, 23, 0, -1, title);

    print(40, 80, "Remade for Pokitto",0,_pal[0]);
    print(40, 96, "    By Spinal",0,_pal[0]);

    print(8, 136, "    Original on C64 by",0,_pal[0]);
    print(8, 152, "   Oliva Kirwa (C) 1990",0,_pal[0]);

    for(int x=0; x<220; x++){
        game.display.directPixel(x,SCROLLHEIGHT,_pal[4]);
        game.display.directPixel(x,SCROLLHEIGHT+1,_pal[10]);
        game.display.directPixel(x,SCROLLHEIGHT+2,_pal[15]);
        game.display.directPixel(x,SCROLLHEIGHT+11,_pal[15]);
        game.display.directPixel(x,SCROLLHEIGHT+12,_pal[10]);
        game.display.directPixel(x,SCROLLHEIGHT+13,_pal[4]);
    }

    gameMode=1;
}

void titleScreen(){
    char text1[34];
    memcpy(text1, &text[myInt],33);
    text1[32]=0;
    //titleprint(-scroller, 6 , text1);
    print(-scroller, SCROLLHEIGHT+3, text1,0,_pal[14]);
    if(frameNumber%3==0){
        scroller++;
        if(scroller==8){
        scroller=0;
        myInt++;
        if(myInt==strlen(text)){myInt=0;}
        }
    }

    if(_A[NEW]){
        lives = 5;
        score = 0;
        levelNum = 0;
        gameMode = 10;
        game.display.directRectangle(0,0,220,176,0x0000); // clear the screen
    }

}

void playLevel(){

    movePlayer();

    // sit still too long at your peril!
    if (pd == 0) {
        satTime++;
        int t=curLev[(px / tileSize) + LEVWIDTH * (py / tileSize)];
        char satCount = 16;
        if(t==2) satCount = 8;

        if(satTime == satCount) {
            checkTile(px / tileSize, py / tileSize);
            satTime = 0;
        }
    }

    renderLevel();

}


int main(){
    rumbleOff(); // just in case

    game.begin();
    game.display.width = 220; // full size
    game.display.height = 174;

    game.sound.ampEnable(true);
    game.sound.playMusicStream(musicName);

    gameMode = 0; // titleScreen
    tempTime = game.getTime();

    while (game.isRunning()) {

          // if it is time to update the screen
 //       if (game.update(true)){

            game.sound.updateStream();

            myPad = updateButtons(myPad);
            UpdatePad(myPad);

            frameNumber++;
            char oldMode = gameMode;
            switch(gameMode){
                case 0:
                    drawTitleScreen();
                    break;
                case 1:
                    levelNum = 0;
                    myDelay = 10;
                    titleScreen();
                    break;
                case 10:
                    // start new game.
                    loadLevel(levelNum);
                    renderLevel(0,1);
                    gameMode = 20;
                    break;
                case 20:
                    // play levels
                    myDelay = 30;
                    playLevel();
                    break;
            }

        // timing loop
        while(game.getTime()-tempTime < myDelay){
            refreshDisplay();
        };
        tempTime = game.getTime();
//  } // update
  }
    return 1;
}
