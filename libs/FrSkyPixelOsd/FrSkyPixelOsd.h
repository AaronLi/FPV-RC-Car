/*
  FrSky PixelOSD library for Teensy LC/3.x/4.x, ATmega2560 or ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20210203
  Not for commercial use
*/

#ifndef __FRSKY_PIXEL_OSD__
#define __FRSKY_PIXEL_OSD__

#include "Arduino.h"

// Uncomment the #define line below to use software instead of hadrware serial for ATmega328P based boards
//#define OSD_USE_SOFTWARE_SERIAL

#define OSD_MAX_API_VERSION 2 // Maximum API version requested by this library (you want may edit this if your code requires lower API version)
#define OSD_DEFAULT_BAUD_RATE 115200 // Default baudrate that this library initiates communication with (you may want to edit it if the OSD is switched to a different baudrate prior to first call to the begin method)
#define OSD_CMD_RESPONSE_TIMEOUT 500 // Maximum waiting time for command response (you may want to increasy it of you are missing responses, but that may increase command execution time)

// Do not modify the #defines below
#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__) || defined(__MK66FX1M0__) || defined(__MK64FX512__) || defined(__IMXRT1062__)
#define TEENSY_HW
#endif

#define OSD_SERIAL_TYPE HardwareSerial

#define OSD_MAX_RESPONSE_LEN 67  
#define OSD_MAX_FONT_DATA_SIZE 54
#define OSD_MAX_FONT_METADATA_SIZE 10

class FrSkyPixelOsd
{
  public:
    enum osd_error_t : int8_t
    {
      OSD_CMD_ERR_NONE = 0,  // No error, everything went fine
      OSD_CMD_ERR_UNKNOWN_CMD = -1, // The received CMD is not a known one
      OSD_CMD_ERR_NO_COMMAND = -2, // A command identifier was expected, but none was found
      OSD_CMD_ERR_PAYLOAD_TOO_SMALL = -3, // Payload received for the current command is smaller than expected
      OSD_CMD_ERR_BUF_TOO_SMALL = -4, // The provided buffer was too small for the response. The UART transport manages the buffers automatically, so if you see this error when using it, please report it as a bug.
      OSD_CMD_ERR_PAYLOAD_INVALID = -5, // Payload received for the current command doesn't satisfy some invarint (e.g. it contains an invalid value for its type)
      OSD_CMD_ERR_INTERNAL = -6, // Something unexpected went wrong. Please, report it as a bug.
      OSD_CMD_ERR_CANT_PERFORM = -7, // Can't perform the command in the current state.
      OSD_CMD_ERR_OUT_OF_MEMORY = -8, // Ran out of memory when performing the request (e.g. running a user program)
      OSD_CMD_ERR_ALREADY_DONE = -9, // Trying to perform an operation that's already done
      // Below error code come from the library, not the OSD itself
      OSD_CMD_ERR_RESPONSE_TIMEOUT = -128 // Response did not arrive within timeout period (defined by OSD_CMD_RESPONSE_TIMEOUT #define above)
    };
 
    typedef struct __attribute__((packed))
    {
      int16_t x : 12;
      int16_t y : 12;
    } osd_point_t;

    typedef struct __attribute__((packed))
    {
      int16_t width : 12;
      int16_t height : 12;
    } osd_size_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_size_t size;
    } osd_rect_t;

    typedef struct __attribute__((packed))
    {
      uint8_t data[OSD_MAX_FONT_DATA_SIZE];
      uint8_t metadata[OSD_MAX_FONT_METADATA_SIZE];
    } osd_chr_data_t;

    enum osd_bitmap_opts_t : uint8_t
    {
      BITMAP_OPT_NONE = 0,
      BITMAP_OPT_INVERSE = 1 << 0,
      BITMAP_OPT_SOLID_BACKGROUND = 1 << 1,
      BITMAP_OPT_ERASE_TRANSPARENT = 1 << 2
    };

    enum osd_color_t : uint8_t
    {
      COLOR_BLACK = 0,
      COLOR_TRANSPARENT = 1,
      COLOR_WHITE = 2,
      COLOR_GREY = 3
    };

    enum osd_outline_t : uint8_t
    {
      OUTLINE_TYPE_NONE = 0,
      OUTLINE_TYPE_TOP = 1 << 0,
      OUTLINE_TYPE_RIGHT = 1 << 1,
      OUTLINE_TYPE_BOTTOM = 1 << 2,
      OUTLINE_TYPE_LEFT = 1 << 3
    };

    enum osd_tv_std_t : uint8_t
    {
      TV_STD_NTSC = 1,
      TV_STD_PAL = 2
    };

    typedef struct __attribute__((packed))
    {
      uint8_t magic[3];
      uint8_t versionMajor;
      uint8_t versionMinor;
      uint8_t versionPatch;
      uint8_t gridRows;
      uint8_t gridColums;
      uint16_t pixelsWidth;
      uint16_t pixelsHeight;
      osd_tv_std_t tvStandard;
      uint8_t hasDetectedCamera;
      uint16_t maxFrameSize;
      uint8_t contextStackSize;
    } osd_cmd_info_response_t;

    typedef struct __attribute__((packed))
    {
      uint8_t version;
      int8_t brightness;
      int8_t horizontalOffset;
      int8_t verticalOffset;
    } osd_cmd_settings_response_t;

    typedef struct __attribute__((packed))
    {
        uint16_t scale;
        uint16_t symbol;
        uint16_t divisor;
        uint16_t dividedSymbol;
    } osd_unit_t;
    
    typedef enum : uint8_t
    {
      WIDGET_ID_AHI = 0,
      WIDGET_ID_SIDEBAR_0 = 1,
      WIDGET_ID_SIDEBAR_1 = 2,
      WIDGET_ID_GRAPH_0 = 3,
      WIDGET_ID_GRAPH_1 = 4,
      WIDGET_ID_GRAPH_2 = 5,
      WIDGET_ID_GRAPH_3 = 6,
      WIDGET_ID_CHARGAUGE_0 = 7,
      WIDGET_ID_CHARGAUGE_1 = 8,
      WIDGET_ID_CHARGAUGE_2 = 9,
      WIDGET_ID_CHARGAUGE_3 = 10
   } osd_widget_id_t;
    
    typedef enum : uint8_t
    {
      WIDGET_AHI_STYLE_STAIRCASE = 0,
      WIDGET_AHI_STYLE_LINE = 1
    } osd_widget_ahi_style_t;

    typedef enum : uint8_t
    {
      WIDGET_AHI_OPTION_NONE = 0,
      WIDGET_AHI_OPTION_SHOW_CORNERS = 1 << 0
    } osd_widget_ahi_options_t;

    typedef struct __attribute__((packed)) 
    {
      osd_rect_t rect;
      osd_widget_ahi_style_t style : 8;
      uint8_t options;
      uint8_t crosshairMargin;
      uint8_t strokeWidth;
    } osd_widget_ahi_config_t;

    typedef enum : uint8_t
    {
      WIDGET_SIDEBAR_OPTION_NONE = 0,
      WIDGET_SIDEBAR_OPTION_LEFT = 1 << 0,
      WIDGET_SIDEBAR_OPTION_REVERSE = 1 << 1,
      WIDGET_SIDEBAR_OPTION_UNLABELED = 1 << 2,
      WIDGET_SIDEBAR_OPTION_STATIC = 1 << 3
    } osd_widget_sidebar_options_t;

    typedef struct __attribute__((packed)) 
    {
      osd_rect_t rect;
      uint8_t options;
      uint8_t divisions;
      uint16_t countsPerStep;
      osd_unit_t unit;
    } osd_widget_sidebar_config_t;
   
    typedef enum : uint8_t
    {
      WIDGET_GRAPH_OPTION_NONE = 0,
      WIDGET_GRAPH_OPTION_BATCHED = 1 << 0
    } osd_widget_graph_options_t;

    typedef struct __attribute__((packed)) 
    {
      osd_rect_t rect;
      uint8_t options;
      uint8_t yLabelCount;
      uint8_t yLabelWidth;
      uint8_t initialScale;
      osd_unit_t unit;
    } osd_widget_graph_config_t;

    typedef struct __attribute__((packed)) 
    {
      osd_point_t point;
      uint16_t chr;
    } osd_widget_chargauge_config_t;

    FrSkyPixelOsd(OSD_SERIAL_TYPE *serial);
    uint32_t begin(uint32_t baudRate = OSD_DEFAULT_BAUD_RATE);
    osd_error_t cmdInfo(osd_cmd_info_response_t *response);
    osd_error_t cmdReadFont(uint16_t character, osd_chr_data_t *response);
    osd_error_t cmdWriteFont(uint16_t character, const osd_chr_data_t *font, osd_chr_data_t *response = NULL);
    osd_error_t cmdGetCamera(uint8_t *response);
    void cmdSetCamera(uint8_t camera = 0);
    osd_error_t cmdGetActiveCamera(uint8_t *response);
    osd_error_t cmdGetOsdEnabled(bool *response);
    void cmdSetOsdEnabled(bool enabled);
    osd_error_t cmdGetSettings(uint8_t version, osd_cmd_settings_response_t *response); // API >= 2
    osd_error_t cmdSetSettings(uint8_t version, int8_t brightness, int8_t horizontalOffset, int8_t verticalOffset, osd_cmd_settings_response_t *response = NULL); // API >= 2
    osd_error_t cmdSaveSettings(uint8_t *response = NULL); // API >= 2
    void cmdTransactionBegin();
    void cmdTransactionCommit();
    void cmdTransactionBeginProfiled(int16_t x, int16_t y);
    void cmdTransactionBeginResetDrawing();
    void cmdSetStrokeColor(osd_color_t color);
    void cmdSetFillColor(osd_color_t color);
    void cmdSetStrokeAndFillColor(osd_color_t color);
    void cmdSetColorInversion(bool enabled);
    void cmdSetPixel(int16_t x, int16_t y, osd_color_t color);
    void cmdSetPixelToStrokeColor(int16_t x, int16_t y);
    void cmdSetPixelToFillColor(int16_t x, int16_t y);
    void cmdSetStrokeWidth(uint8_t width);
    void cmdSetLineOutlineType(osd_outline_t outline);
    void cmdSetLineOutlineColor(osd_color_t color);
    void cmdClipToRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void cmdClearScreen();
    void cmdClearRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void cmdDrawingReset();
    void cmdDrawChar(int16_t x, int16_t y, uint16_t character, osd_bitmap_opts_t options = BITMAP_OPT_NONE);
    void cmdDrawCharMask(int16_t x, int16_t y, uint16_t character, osd_bitmap_opts_t options, osd_color_t color);
    void cmdDrawString(int16_t x, int16_t y, const char *string, uint32_t stringLen, osd_bitmap_opts_t options = BITMAP_OPT_NONE); // String should be NULL terminated, strLen should include the NULL terminating character
    void cmdDrawStringMask(int16_t x, int16_t y, const char *string, uint32_t stringLen, osd_bitmap_opts_t options, osd_color_t color); // String should be NULL terminated, strLen should include the NULL terminating character
    void cmdDrawBitmap(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t *bitmap, uint32_t bitmapLenLen, osd_bitmap_opts_t options = BITMAP_OPT_NONE);
    void cmdDrawBitmapMask(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t *bitmap, uint32_t bitmapLenLen, osd_bitmap_opts_t options, osd_color_t color);
    void cmdMoveToPoint(int16_t x, int16_t y);
    void cmdStrokeLineToPoint(int16_t x, int16_t y);
    void cmdStrokeTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3);
    void cmdFillTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3);
    void cmdFillStrokeTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3);
    void cmdStrokeRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void cmdFillRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void cmdFillStrokeRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void cmdStrokeEllipseInRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void cmdFillEllipseInRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void cmdFillStrokeEllipseInRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void cmdCtmReset();
    void cmdCtmSet(float m11, float m12, float m21, float m22, float m31, float m32);
    void cmdCtmTranstale(float tx, float ty);
    void cmdCtmScale(float sx, float sy);
    void cmdCtmRotateRad(float angle);
    void cmdCtmRotateDeg(int16_t angle);
    void cmdCtmRotateAboutRad(float angle, float cx, float cy);
    void cmdCtmRotateAboutDeg(int16_t angle, float cx, float cy);
    void cmdCtmShear(float sx, float sy);
    void cmdCtmShearAbout(float sx, float sy, float cx, float cy);
    void cmdCtmMultiply(float m11, float m12, float m21, float m22, float m31, float m32);
    void cmdCtmTranslateRev(float tx, float ty); // API >= 2
    void cmdCtmScaleRev(float sx, float sy); // API >= 2
    void cmdCtmRotateRevRad(float angle); // API >= 2
    void cmdCtmRotateRevDeg(int16_t angle); // API >= 2
    void cmdCtmRotateAboutRevRad(float angle, float cx, float cy); // API >= 2
    void cmdCtmRotateAboutRevDeg(int16_t angle, float cx, float cy); // API >= 2
    void cmdCtmShearRev(float sx, float sy); // API >= 2
    void cmdCtmShearAboutRev(float sx, float sy, float cx, float cy); // API >= 2
    void cmdCtmMultiplyRev(float m11, float m12, float m21, float m22, float m31, float m32); // API >= 2
    void cmdCtmI16Translate(int16_t tx, int16_t ty); // API >= 2
    void cmdCtmU16Rotate(uint16_t angle); // API >= 2
    void cmdCtmI16TranslateRev(int16_t tx, int16_t ty); // API >= 2
    void cmdCtmU16RotateRev(uint16_t angle); // API >= 2
    void cmdContextPush();
    void cmdContextPop();
    void cmdDrawGridChar(uint8_t column, uint8_t row, uint16_t character, osd_bitmap_opts_t options = BITMAP_OPT_NONE);
    void cmdDrawGridString(uint8_t column, uint8_t row, const char *string, uint32_t stringLen, osd_bitmap_opts_t options = BITMAP_OPT_NONE); // String should be NULL terminated, strLen should include the NULL terminating character
    void cmdDrawGridChar2(uint8_t column, uint8_t row, uint16_t character, osd_bitmap_opts_t options = BITMAP_OPT_NONE); // API >= 2
    void cmdDrawGridCharMask2(uint8_t column, uint8_t row, uint16_t character, osd_bitmap_opts_t options, osd_color_t color); // API >= 2
    void cmdDrawGridString2(uint8_t column, uint8_t row, const char *string, uint32_t stringLen, osd_bitmap_opts_t options = BITMAP_OPT_NONE); // API >= 2, string should be not NULL, strLen should not include the NULL terminating character
    osd_error_t cmdWidgetSetConfigAhi(int16_t x, int16_t y, int16_t width, int16_t height, osd_widget_ahi_style_t style, uint8_t options, uint8_t crosshairMargin, uint8_t strokeWidth,
                                      osd_widget_id_t id = WIDGET_ID_AHI, osd_widget_ahi_config_t *response = NULL); // API >= 2
    osd_error_t cmdWidgetSetConfigSidebar(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t options, uint8_t divisions, uint16_t countsPerStep,
                                          uint16_t scale, uint16_t symbol, uint16_t divisor, uint16_t dividedSymbol, osd_widget_id_t id = WIDGET_ID_SIDEBAR_0, osd_widget_sidebar_config_t *response = NULL); // API >= 2
    osd_error_t cmdWidgetSetConfigGraph(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t options, uint8_t yLabelCount, uint8_t yLabelWidth, uint8_t initialScale,
                                        uint16_t scale, uint16_t symbol, uint16_t divisor, uint16_t dividedSymbol, osd_widget_id_t id = WIDGET_ID_GRAPH_0, osd_widget_graph_config_t *response = NULL); // API >= 2
    osd_error_t cmdWidgetSetConfigCharGauge(int16_t x, int16_t y, uint16_t character, osd_widget_id_t id = WIDGET_ID_CHARGAUGE_0, osd_widget_chargauge_config_t *response = NULL); // API >= 2, requires character metadata to be set as per specification
    void cmdWidgetDrawAhiRad(float pitch, float roll, osd_widget_id_t id = WIDGET_ID_AHI); // API >= 2
    void cmdWidgetDrawAhiDeg(int16_t pitch, int16_t roll, osd_widget_id_t id = WIDGET_ID_AHI); // API >= 2
    void cmdWidgetDrawSidebar(int32_t value, osd_widget_id_t id = WIDGET_ID_SIDEBAR_0); // API >= 2
    void cmdWidgetDrawGraph(int32_t value, osd_widget_id_t id = WIDGET_ID_GRAPH_0); // API >= 2
    void cmdWidgetDrawCharGauge(uint8_t value, osd_widget_id_t id = WIDGET_ID_CHARGAUGE_0); // API >= 2
    void cmdWidgetErase(osd_widget_id_t id); // API >= 2
    void cmdReboot(bool toBootloader = false);
    void cmdWriteFlash();
    uint8_t setFontMetadataSize(uint8_t size, uint8_t position, uint8_t *metadata);
    uint8_t setFontMetadataOffset(int8_t x, int8_t y, uint8_t position, uint8_t *metadata);
    uint8_t setFontMetadataRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t position, uint8_t *metadata);
    uint8_t setFontMetadataColors(uint8_t color0, uint8_t color1, uint8_t color2, uint8_t color3, uint8_t position, uint8_t *metadata);
    void clearFontMetadata(uint8_t fromPosition, uint8_t *metadata);

  private:
    typedef uint32_t osd_uvarint_t;
    typedef struct __attribute__((packed))
    {
      osd_point_t point1;
      osd_point_t point2;
      osd_point_t point3;
    } osd_triangle_t;

    typedef struct __attribute__((packed))
    {
      uint16_t chr;
      osd_chr_data_t data;
    } osd_cmd_font_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_color_t color;
    } osd_cmd_set_pixel_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      uint16_t chr;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_char_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      uint16_t chr;
      osd_bitmap_opts_t opts;
      osd_color_t color;
    } osd_cmd_draw_char_mask_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_string_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_bitmap_opts_t opts;
      osd_color_t color;
    } osd_cmd_draw_string_mask_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_size_t size;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_bitmap_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_size_t size;
      osd_bitmap_opts_t opts;
      osd_color_t color;
    } osd_cmd_draw_bitmap_mask_data_t;

    typedef struct __attribute__((packed))
    {
        float m11;
        float m12;
        float m21;
        float m22;
        float m31;
        float m32;
    } osd_cmt_t;

    typedef struct __attribute__((packed))
    {
        float x;
        float y;
    } osd_transformation_t;

    typedef struct __attribute__((packed))
    {
        int16_t x;
        int16_t y;
    } osd_transformation_i16_t;

    typedef struct __attribute__((packed))
    {
      float angle;
      float cx;
      float cy;
    } osd_cmd_rotate_about_data_t;

    typedef struct __attribute__((packed))
    {
      float sx;
      float sy;
      float cx;
      float cy;
    } osd_cmd_shear_about_data_t;

    typedef struct __attribute__((packed))
    {
      uint8_t column;
      uint8_t row;
      uint16_t chr;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_grid_char_data_t;

    typedef struct __attribute__((packed))
    {
      uint8_t column;
      uint8_t row;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_grid_string_data_t;

    typedef struct __attribute__((packed))
    {
      uint8_t column : 5;
      uint8_t row : 4;
      uint16_t chr : 9;
      uint8_t opts : 3;
      uint8_t asMask : 1;
      uint8_t color : 2;
    } osd_cmd_draw_grid_chr2_data_t; // API >= 2

    typedef struct __attribute__((packed))
    {
      uint8_t column : 5;
      uint8_t row : 4;
      uint8_t opts : 3;
      uint8_t strSize : 4;
    } osd_cmd_draw_grid_str2_data_t; // API >= 2

    typedef struct __attribute__((packed)) 
    {
      uint16_t pitch : 12;
      uint16_t roll : 12;
    } osd_widget_ahi_pitch_roll_t;

    typedef struct __attribute__((packed)) 
    {
      int32_t value : 24;
    } osd_widget_sidebar_draw_t;

    typedef struct __attribute__((packed)) 
    {
      int32_t value : 24;
    } osd_widget_graph_draw_t;

    typedef struct __attribute__((packed))
    {
      uint8_t value;
    } osd_widget_chargauge_draw_t;

    typedef struct __attribute__((packed))
    {
      uint8_t sz;
    } osd_font_char_metadata_size_t;

    typedef struct __attribute__((packed))
    {
        int8_t x;
        int8_t y;
    } osd_font_char_metadata_offset_t;

    typedef struct __attribute__((packed))
    {
      uint8_t x;
      uint8_t y;
      uint8_t w;
      uint8_t h;
    } osd_font_char_metadata_rect_t;

    typedef struct __attribute__((packed))
    {
      uint8_t color0 : 2;
      uint8_t color1 : 2;
      uint8_t color2 : 2;
      uint8_t color3 : 2;
    } osd_font_char_metadata_colors_t;

    typedef struct __attribute__((packed))
    {
      uint8_t id;
      int8_t error;
    } osd_cmd_error_response_t;

    enum osd_command_t : uint8_t
    {
      CMD_ERROR = 0,
      CMD_INFO = 1,
      CMD_READ_FONT = 2,
      CMD_WRITE_FONT = 3,
      CMD_GET_CAMERA = 4,
      CMD_SET_CAMERA = 5,
      CMD_GET_ACTIVE_CAMERA = 6,
      CMD_GET_OSD_ENABLED = 7,
      CMD_SET_OSD_ENABLED = 8,
      CMD_GET_SETTINGS = 9, // API >= 2
      CMD_SET_SETTINGS = 10, // API >= 2
      CMD_SAVE_SETTINGS = 11, // API >= 2
      CMD_TRANSACTION_BEGIN = 16,
      CMD_TRANSACTION_COMMIT = 17,
      CMD_TRANSACTION_BEGIN_PROFILED = 18,
      CMD_TRANSACTION_BEGIN_RESET_DRAWING = 19,
      CMD_DRAWING_SET_STROKE_COLOR = 22,
      CMD_DRAWING_SET_FILL_COLOR = 23,
      CMD_DRAWING_SET_STROKE_AND_FILL_COLOR = 24,
      CMD_DRAWING_SET_COLOR_INVERSION = 25,
      CMD_DRAWING_SET_PIXEL = 26,
      CMD_DRAWING_SET_PIXEL_TO_STROKE_COLOR = 27,
      CMD_DRAWING_SET_PIXEL_TO_FILL_COLOR = 28,
      CMD_DRAWING_SET_STROKE_WIDTH = 29,
      CMD_DRAWING_SET_LINE_OUTLINE_TYPE = 30,
      CMD_DRAWING_SET_LINE_OUTLINE_COLOR = 31,
      CMD_DRAWING_CLIP_TO_RECT = 40,
      CMD_DRAWING_CLEAR_SCREEN = 41,
      CMD_DRAWING_CLEAR_RECT = 42,
      CMD_DRAWING_RESET = 43,
      CMD_DRAWING_DRAW_BITMAP = 44,
      CMD_DRAWING_DRAW_BITMAP_MASK = 45,
      CMD_DRAWING_DRAW_CHAR = 46,
      CMD_DRAWING_DRAW_CHAR_MASK = 47,
      CMD_DRAWING_DRAW_STRING = 48,
      CMD_DRAWING_DRAW_STRING_MASK = 49,
      CMD_DRAWING_MOVE_TO_POINT = 50,
      CMD_DRAWING_STROKE_LINE_TO_POINT = 51,
      CMD_DRAWING_STROKE_TRIANGLE = 52,
      CMD_DRAWING_FILL_TRIANGLE = 53,
      CMD_DRAWING_FILL_STROKE_TRIANGLE = 54,
      CMD_DRAWING_STROKE_RECT = 55,
      CMD_DRAWING_FILL_RECT = 56,
      CMD_DRAWING_FILL_STROKE_RECT = 57,
      CMD_DRAWING_STROKE_ELLIPSE_IN_RECT = 58,
      CMD_DRAWING_FILL_ELLIPSE_IN_RECT = 59,
      CMD_DRAWING_FILL_STROKE_ELLIPSE_IN_RECT = 60,
      CMD_CTM_RESET = 80,
      CMD_CTM_SET = 81,
      CMD_CTM_TRANSLATE = 82,
      CMD_CTM_SCALE = 83,
      CMD_CTM_ROTATE = 84,
      CMD_CTM_ROTATE_ABOUT = 85,
      CMD_CTM_SHEAR = 86,
      CMD_CTM_SHEAR_ABOUT = 87,
      CMD_CTM_MULTIPLY = 88,
      CMD_CTM_TRANSLATE_REV = 89, // API >= 2
      CMD_CTM_SCALE_REV = 90, // API >= 2
      CMD_CTM_ROTATE_REV = 91, // API >= 2
      CMD_CTM_ROTATE_ABOUT_REV = 92, // API >= 2
      CMD_CTM_SHEAR_REV = 93, // API >= 2
      CMD_CTM_SHEAR_ABOUT_REV = 94, // API >= 2
      CMD_CTM_MULTIPLY_REV = 95, // API >= 2
      CMD_CTM_I16TRANSLATE = 96, // API >= 2
      CMD_CTM_U16ROTATE = 97, // API >= 2
      CMD_CTM_I16TRANSLATE_REV = 98, // API >= 2
      CMD_CTM_U16ROTATE_REV = 99, // API >= 2
      CMD_CONTEXT_PUSH = 100,
      CMD_CONTEXT_POP = 101,
      CMD_DRAW_GRID_CHR = 110,
      CMD_DRAW_GRID_STR = 111,
      CMD_DRAW_GRID_CHR_2 = 112, // API >= 2
      CMD_DRAW_GRID_STR_2 = 113, // API >= 2
      CMD_WIDGET_SET_CONFIG = 115, // API >= 2
      CMD_WIDGET_DRAW = 116, // API >= 2
      CMD_WIDGET_ERASE = 117, // API >= 2
      CMD_REBOOT = 120,
      CMD_WRITE_FLASH = 121,
      CMD_SET_DATA_RATE = 122
    };
 
    FrSkyPixelOsd();
    void updateCrc(uint8_t *crc, uint8_t data);
    void sendHeader();
    void sendByte(uint8_t *crc, uint8_t byte);
    uint8_t getUvarintLen(uint32_t value);
    bool decodeUvarint(uint8_t *val, uint8_t byte);
    void sendUvarint(uint8_t *crc, uint32_t value);
    void sendPayload(uint8_t *crc, const void *payload, uint32_t payloadLen);
    void sendCrc(uint8_t crc);
    void sendCmd(FrSkyPixelOsd::osd_command_t id, const void *payload = NULL, uint32_t payloadLen = 0, const void *varPayload = NULL, uint32_t varPayloadLen = 0, bool sendVarPayloadLen = true);
    osd_error_t receive(FrSkyPixelOsd::osd_command_t expectedCmdId, const void *payload, void *response, uint32_t timeout = OSD_CMD_RESPONSE_TIMEOUT);
    osd_error_t cmdSetDataRate(uint32_t dataRate, uint32_t *response = NULL);
    uint8_t setFontMetadata(uint8_t metadataType, const void *metadataContent, uint8_t metadataSize, uint8_t position, uint8_t *metadata);

    OSD_SERIAL_TYPE *osdSerial;
    uint32_t osdBaudrate = OSD_DEFAULT_BAUD_RATE;
};

#endif // __FRSKY_PIXEL_OSD__