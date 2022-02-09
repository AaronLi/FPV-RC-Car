/*
  FrSky PixelOSD library cube example
  (c) Pawelsky 20210203
  Not for commercial use
  
  Cube animation code based on http://colinord.blogspot.com/2015/01/arduino-oled-module-with-3d-demo.html
  
  Note that you need Teensy LC/3.x/4.x, ATmega2560 or ATmega328P based (e.g. Pro Mini, Nano, Uno) board, FrSkyPixelOsd library
  and the actual FrSky Pixel OSD hardware (https://www.frsky-rc.com/product/osd/) or simulator (https://github.com/FrSkyRC/PixelOSD/tree/master/simulator)
*/

#include "FrSkyPixelOsd.h"

// Create OSD object, pass the reference to the serial port to use
#if defined(TEENSY_HW) || defined(__AVR_ATmega2560__)
  FrSkyPixelOsd osd(&Serial1);
#else
  FrSkyPixelOsd osd(&Serial); // Create OSD object, pass the reference to the serial port to use
#endif

float rot, rotx, roty, rotz, rotxx, rotyy, rotxxx, rotyyy;
int wireframe[8][2];

int originx = 180;
int originy = 144;

int cubeVertex[8][3] =
{
  { -50, -50,  50 },
  {  50, -50,  50 },
  {  50,  50,  50 },
  { -50,  50,  50 },
  { -50, -50, -50 },
  {  50, -50, -50 },
  {  50,  50, -50 },
  { -50,  50, -50 }
};

void setup()
{
  osd.begin(); // Initialize the OSD with the dafault baudrate (115200)
  osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
  osd.cmdSetStrokeWidth(3);
  char string[] = "PAWELSKY FRSKY PIXELOSD TEST";
  osd.cmdDrawGridString(1, 1, string, sizeof(string));
}

void drawWireframe(void)
{
  osd.cmdTransactionBegin();
  osd.cmdClearRect(90, 50, 180, 180);
  osd.cmdMoveToPoint(wireframe[0][0], wireframe[0][1]);
  osd.cmdStrokeLineToPoint(wireframe[1][0], wireframe[1][1]);
  osd.cmdStrokeLineToPoint(wireframe[2][0], wireframe[2][1]);
  osd.cmdStrokeLineToPoint(wireframe[3][0], wireframe[3][1]);
  osd.cmdStrokeLineToPoint(wireframe[0][0], wireframe[0][1]);

  osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_GREY);
  osd.cmdMoveToPoint(wireframe[1][0], wireframe[1][1]);
  osd.cmdStrokeLineToPoint(wireframe[3][0], wireframe[3][1]);
  osd.cmdMoveToPoint(wireframe[0][0], wireframe[0][1]);
  osd.cmdStrokeLineToPoint(wireframe[2][0], wireframe[2][1]);
  osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);

  osd.cmdMoveToPoint(wireframe[4][0], wireframe[4][1]);
  osd.cmdStrokeLineToPoint(wireframe[5][0], wireframe[5][1]);
  osd.cmdStrokeLineToPoint(wireframe[6][0], wireframe[6][1]);
  osd.cmdStrokeLineToPoint(wireframe[7][0], wireframe[7][1]);
  osd.cmdStrokeLineToPoint(wireframe[4][0], wireframe[4][1]);

  osd.cmdMoveToPoint(wireframe[0][0], wireframe[0][1]);
  osd.cmdStrokeLineToPoint(wireframe[4][0], wireframe[4][1]);
  osd.cmdMoveToPoint(wireframe[1][0], wireframe[1][1]);
  osd.cmdStrokeLineToPoint(wireframe[5][0], wireframe[5][1]);
  osd.cmdMoveToPoint(wireframe[2][0], wireframe[2][1]);
  osd.cmdStrokeLineToPoint(wireframe[6][0], wireframe[6][1]);
  osd.cmdMoveToPoint(wireframe[3][0], wireframe[3][1]);
  osd.cmdStrokeLineToPoint(wireframe[7][0], wireframe[7][1]);
  osd.cmdTransactionCommit();
}

void loop(void)
{
  for (uint16_t angle = 0; angle <= 360; angle = angle + 2)
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      rot = angle * 0.0174532;
      rotz = cubeVertex[i][2] * cos(rot) - cubeVertex[i][0] * sin(rot);
      rotx = cubeVertex[i][2] * sin(rot) + cubeVertex[i][0] * cos(rot);
      roty = cubeVertex[i][1];
      rotyy = roty * cos(rot) - rotz * sin(rot);
      rotxx = rotx;
      rotxxx = rotxx * cos(rot) - rotyy * sin(rot);
      rotyyy = rotxx * sin(rot) + rotyy * cos(rot);
      rotxxx = rotxxx + originx;
      rotyyy = rotyyy + originy;
      wireframe[i][0] = rotxxx;
      wireframe[i][1] = rotyyy;
    }
    drawWireframe();
  }
}