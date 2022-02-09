/*
  FrSky PixelOSD library widget example
  (c) Pawelsky 20210203
  Not for commercial use
  
  Note that you need Teensy LC/3.x/4.x, ATmega2560 or ATmega328P based (e.g. Pro Mini, Nano, Uno) board, FrSkyPixelOsd library
  and the actual FrSky Pixel OSD hardware (https://www.frsky-rc.com/product/osd/) or simulator (https://github.com/FrSkyRC/PixelOSD/tree/master/simulator)
  
  API version >= 2 is required for this example to work (firmware v2.0.1+ or Simulator v1.0.3+ are recommended due to errors in earlier versions)
  
  To display a chargauge character #150 must have the necessary metadata (rect, offset and color) defined, or otherwise the gauge will not work.
  Best to use one of the iNav fonts, e.g. Vision (you can dowlnoad them with the FrSkyOSDApp)
*/

#include "FrSkyPixelOsd.h"

// Create OSD object, pass the reference to the serial port to use
#if defined(TEENSY_HW) || defined(__AVR_ATmega2560__)
  FrSkyPixelOsd osd(&Serial1);
#else
  FrSkyPixelOsd osd(&Serial);
#endif

FrSkyPixelOsd::osd_cmd_info_response_t response;
FrSkyPixelOsd::osd_widget_chargauge_config_t charGaugeConfig;
FrSkyPixelOsd::osd_error_t charGaugeError;
uint8_t position = 0;
char string[31];

void setup()
{
  osd.begin();  // Initialize the OSD, with the default (115200) baudrate
  sprintf(string, " PAWELSKY FRSKY PIXELOSD TEST ");
  osd.cmdDrawGridString(0, 1, string, strlen(string) + 1);

  // Widgets are available with firmware v2.0.0+, check and display an error message if lower firmware detected
  osd.cmdInfo(&response);
  if(response.versionMajor < 2)
  {
    sprintf(string, "FIRMWARE TOO LOW (NEED 2.0.0+)");
    osd.cmdDrawGridString(0, 3, string, strlen(string) + 1);
    while(1);
  }
  
  // Configure sidebar widgets
  osd.cmdWidgetSetConfigSidebar(10, 50, 75, 150, FrSkyPixelOsd::WIDGET_SIDEBAR_OPTION_LEFT, 10, 10, 1, 'P', 10, 'P', FrSkyPixelOsd::WIDGET_ID_SIDEBAR_0);
  osd.cmdWidgetSetConfigSidebar(275, 50, 75, 150, FrSkyPixelOsd::WIDGET_SIDEBAR_OPTION_REVERSE | FrSkyPixelOsd::WIDGET_SIDEBAR_OPTION_UNLABELED, 5, 10, 1, 'R', 0, 'R', FrSkyPixelOsd::WIDGET_ID_SIDEBAR_1);
  // Configure graph widget
  osd.cmdWidgetSetConfigGraph(4, 52, 172, 144, FrSkyPixelOsd::WIDGET_GRAPH_OPTION_NONE, 3, 4, 2, 1, 'S', 10, 'S', FrSkyPixelOsd::WIDGET_ID_GRAPH_0);
  // Configure char gauge widget
  // Assumes the character #150 has the necessary metadata (rect, offset and color) defined, otherwise the gauge will not work and error will be printer.
  // Use one of the iNav fonts, e.g. Vision (you can dowlnoad them with the FrSkyOSDApp)
  charGaugeError = osd.cmdWidgetSetConfigCharGauge(132, 216, 150, FrSkyPixelOsd::WIDGET_ID_CHARGAUGE_0, &charGaugeConfig);
  sprintf(string, "CHARGAUGE:%s", (charGaugeError == FrSkyPixelOsd::OSD_CMD_ERR_NONE) ? "" : " WRONG FONT METADATA");
}

void loop()
{
  // Configure AHI widget as "staircase"
  osd.cmdWidgetSetConfigAhi(90, 50, 180, 150, FrSkyPixelOsd::WIDGET_AHI_STYLE_STAIRCASE, FrSkyPixelOsd::WIDGET_AHI_OPTION_NONE, 10, 1, FrSkyPixelOsd::WIDGET_ID_AHI);
  osd.cmdDrawGridString(0, 12, string, strlen(string) + 1);
  for(int16_t i = -90 ; i <= 90; i++)
  {
    // Draw AHI, chargauge and sidebar widgets
    osd.cmdWidgetDrawAhiDeg(i, -i / 2, FrSkyPixelOsd::WIDGET_ID_AHI);
    osd.cmdWidgetDrawSidebar(i, FrSkyPixelOsd::WIDGET_ID_SIDEBAR_0);
    osd.cmdWidgetDrawSidebar(-i / 2, FrSkyPixelOsd::WIDGET_ID_SIDEBAR_1);
    if(charGaugeError == FrSkyPixelOsd::OSD_CMD_ERR_NONE) osd.cmdWidgetDrawCharGauge(abs(i) * 2, FrSkyPixelOsd::WIDGET_ID_CHARGAUGE_0);
    delay(50);
  }
  // Erase AHI, gauge and sidebar widgets
  osd.cmdWidgetErase(FrSkyPixelOsd::WIDGET_ID_AHI);
  osd.cmdWidgetErase(FrSkyPixelOsd::WIDGET_ID_SIDEBAR_0);
  osd.cmdWidgetErase(FrSkyPixelOsd::WIDGET_ID_SIDEBAR_1);
  if(charGaugeError == FrSkyPixelOsd::OSD_CMD_ERR_NONE) osd.cmdWidgetErase(FrSkyPixelOsd::WIDGET_ID_CHARGAUGE_0);
  osd.cmdDrawGridString(0, 12, "                              ", 31);

  // Configure AHI widget as "line"
  osd.cmdWidgetSetConfigAhi(185, 50, 170, 150, FrSkyPixelOsd::WIDGET_AHI_STYLE_LINE, FrSkyPixelOsd::WIDGET_AHI_OPTION_SHOW_CORNERS, 10, 2, FrSkyPixelOsd::WIDGET_ID_AHI);
  for(float i = -PI / 2 ; i <= PI / 2; i = i + (PI / 180.0))
  {
    // Draw AHI and graph widgets
    osd.cmdWidgetDrawAhiRad(i, -i / 2, FrSkyPixelOsd::WIDGET_ID_AHI);
    osd.cmdWidgetDrawGraph(sin(i) * 100, FrSkyPixelOsd::WIDGET_ID_GRAPH_0);
    delay(50);
  }
  // Erase AHI and graph widgets
  osd.cmdWidgetErase(FrSkyPixelOsd::WIDGET_ID_AHI);
  osd.cmdWidgetErase(FrSkyPixelOsd::WIDGET_ID_GRAPH_0);
}