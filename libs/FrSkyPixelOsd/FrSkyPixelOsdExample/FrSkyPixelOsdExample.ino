/*
  FrSky PixelOSD library example
  (c) Pawelsky 20210203
  Not for commercial use
  
  Note that you need Teensy LC/3.x/4.x, ATmega2560 or ATmega328P based (e.g. Pro Mini, Nano, Uno) board, FrSkyPixelOsd library
  and the actual FrSky Pixel OSD hardware (https://www.frsky-rc.com/product/osd/) or simulator (https://github.com/FrSkyRC/PixelOSD/tree/master/simulator)
*/

#include "FrSkyPixelOsd.h"

// Create OSD object, pass the reference to the serial port to use
#if defined(TEENSY_HW) || defined(__AVR_ATmega2560__)
  FrSkyPixelOsd osd(&Serial1);
#else
  FrSkyPixelOsd osd(&Serial);
#endif

FrSkyPixelOsd::osd_cmd_info_response_t response;
FrSkyPixelOsd::osd_cmd_settings_response_t settings;
char string[31] = " PAWELSKY FRSKY PIXELOSD TEST ";
uint8_t strLen;
uint32_t baudrate;

void setup()
{
  baudrate = osd.begin(57600);  // Initialize the OSD, set the desired baudrate for communication (the default is 115200)
  delay(100); // Small delay to allow the OSD to physically switch to desired baudrate
  osd.cmdDrawGridString(0, 1, string, sizeof(string));
}

void loop()
{
  uint8_t character = 0;
  
  // Print out basic OSD info
  osd.cmdTransactionBegin();
  osd.cmdClearRect(0, 36, 380, 252);
  osd.cmdInfo(&response);
  strLen = sprintf(string, "UART:%lu VERSION:%u.%u.%u", 
                   baudrate, response.versionMajor, response.versionMinor, response.versionPatch);
  osd.cmdDrawGridString(0, 3, string, strLen + 1);
  strLen = sprintf(string, "GRID:%uX%u PIXELS:%uX%u",
                   response.gridColums,response.gridRows, response.pixelsWidth, response.pixelsHeight);
  osd.cmdDrawGridString(0, 4, string, strLen + 1);
  strLen = sprintf(string, "TV STANDARD:%s CAMERA:%u", 
                   response.tvStandard == 2 ? "PAL" : response.tvStandard == 1 ? "NTSC" : "OTHER", response.hasDetectedCamera);
  osd.cmdDrawGridString(0, 5, string, strLen + 1);
  strLen = sprintf(string, "MAX FRAME:%u MAX CONTEXT:%u",
                   response.maxFrameSize, response.contextStackSize);
  osd.cmdDrawGridString(0, 6, string, strLen + 1);
  osd.cmdTransactionCommit();
  delay(1000);

  osd.cmdTransactionBegin();
  osd.cmdClearRect(0, 36, 380, 252);
  osd.cmdGetSettings(2, &settings);
  strLen = sprintf(string, "SETTINGS VER: %u", settings.version);
  osd.cmdDrawGridString(0, 3, string, strLen + 1);
  if(settings.version >= 2)
  {
    strLen = sprintf(string, "BRIGHTNESS: %d", settings.brightness);
    osd.cmdDrawGridString(0, 5, string, strLen + 1);
    strLen = sprintf(string, "OFFSET H:%d V:%d", settings.horizontalOffset, settings.verticalOffset);
    osd.cmdDrawGridString(0, 4, string, strLen + 1);
  }
  osd.cmdTransactionCommit();
  delay(1000);

  // Display first 256 characters
  osd.cmdTransactionBegin();
  osd.cmdClearRect(0, 36, 380, 252);
  for(uint16_t y = 0; y < 8; y++)
  {
    for(uint16_t x = 0; x < 32; x++)
    {
      osd.cmdDrawGridChar(x, y + 4, character++);
    }
  }
  osd.cmdTransactionCommit();
  delay(1000);

  // Display some geometric shapes
  osd.cmdTransactionBegin();
  osd.cmdClearRect(0, 36, 380, 252);
  osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_BLACK);
  osd.cmdStrokeTriangle(10, 170, 35, 120, 60, 170);
  osd.cmdSetFillColor(FrSkyPixelOsd::COLOR_WHITE);
  osd.cmdFillTriangle(65, 170, 90, 120, 115, 170);
  osd.cmdStrokeRect(120, 120, 50, 50);
  osd.cmdSetFillColor(FrSkyPixelOsd::COLOR_GREY);
  osd.cmdFillRect(175, 120, 50, 50);
  osd.cmdStrokeEllipseInRect(230, 120, 50, 50);
  osd.cmdSetFillColor(FrSkyPixelOsd::COLOR_BLACK);
  osd.cmdFillEllipseInRect(285, 120, 50, 50);
  osd.cmdTransactionCommit();
  delay(1000);
}