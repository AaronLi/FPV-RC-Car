/*
  FrSky PixelOSD library for Teensy LC/3.x/4.x, ATmega2560 or ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20210203
  Not for commercial use
*/

#include "FrSkyPixelOsd.h"

void FrSkyPixelOsd::sendByte(uint8_t *crc, uint8_t byte)
{
  osdSerial->write(byte);
  updateCrc(crc, byte);
}

uint8_t FrSkyPixelOsd::getUvarintLen(uint32_t value)
{
  if(value > 268453455) return 5;    // more than 28 bits
  else if(value > 2097151) return 4; // more than 21 bits
  else if(value > 16383) return 3;   // more than 14 bits
  else if(value > 127) return 2;     // more than 7 bits
  else return 1;                     // less than 8 bits
}

void FrSkyPixelOsd::sendUvarint(uint8_t *crc, uint32_t value)
{
  while(value >= 0x80)
  {
    sendByte(crc, (value & 0x7F) | 0x80);
    value >>= 7;
  }
  sendByte(crc, value & 0x7F);
}

void FrSkyPixelOsd::sendPayload(uint8_t *crc, const void *payload, uint32_t payloadLen)
{
  if(payload != NULL)
  {
    for(uint32_t i = 0; i < payloadLen; i++)
    {
      sendByte(crc, ((uint8_t*)payload)[i]);
    }
  }
}

bool FrSkyPixelOsd::decodeUvarint(uint8_t *val, uint8_t byte)
{
  // Simplified - will only work with uvarint < 128, but we do not expect anything bigger in responses
  *val = byte;
  return (byte < 0x80);
}

void FrSkyPixelOsd::updateCrc(uint8_t *crc, uint8_t data)
{
  if(crc != NULL)
  {
    *crc ^= data;
    for (uint8_t i = 0; i < 8; ++i)
    {
      if (*crc & 0x80) *crc = (*crc << 1) ^ 0xD5; else *crc <<= 1;
    }
  }
}

FrSkyPixelOsd::FrSkyPixelOsd(OSD_SERIAL_TYPE *serial)
{
  osdSerial = serial;
}

FrSkyPixelOsd::FrSkyPixelOsd()
{
  // Intentionally left empty
}

uint32_t FrSkyPixelOsd::begin(uint32_t baudRate)
{
  FrSkyPixelOsd::osd_cmd_info_response_t response;
  
  // Initialize serial (Hardware or Software)
  osdSerial->begin(osdBaudrate);
  // Wait for the OSD to be responsive
  while(cmdInfo(&response) != OSD_CMD_ERR_NONE) delay(100);
  // Change baudrate if different than default requested
  if(baudRate != osdBaudrate)
  {
    cmdSetDataRate(baudRate, &osdBaudrate);
    osdSerial->end();
    osdSerial->begin(osdBaudrate);
  }
  // Reset the OSD
  cmdDrawingReset();
  cmdClearScreen();
  
  return osdBaudrate;
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::receive(FrSkyPixelOsd::osd_command_t expectedCmdId, const void *payload, void *response, uint32_t timeout)
{
  FrSkyPixelOsd::osd_error_t result = OSD_CMD_ERR_RESPONSE_TIMEOUT;
  uint8_t state = 0;
  uint8_t crc = 0;
  uint8_t frameLen = 0;
  uint8_t frameIdx = 0;
  uint8_t frame[OSD_MAX_RESPONSE_LEN];
  uint32_t currentTime = millis();
  uint32_t endTime = currentTime + timeout;
  
  while(currentTime < endTime)
  {
    if(osdSerial->available())
    {
      uint8_t data = osdSerial->read();
      if((state == 0) && (data == '$'))  // First header character '$'
      {
        state = 1;
      }
      else if((state == 1) && (data == 'A')) // Second header character 'A'
      {
        state = 2;
        frameLen = 0;
        frameIdx = 0;
        crc = 0;
      }
      else if(state == 2) // Message length as uvarint
      {
        updateCrc(&crc, data);
        state = 0;
        if(decodeUvarint(&frameLen, data) == true)
        {
          if(frameLen <= OSD_MAX_RESPONSE_LEN) state = 3;
        }
      }
      else if(state == 3) // Message data
      {
        updateCrc(&crc, data);
        frame[frameIdx++] = data;
        if(frameIdx >= frameLen) state = 4;
      }
      else if(state == 4) // Message CRC
      {
        if(crc == data) state = 5; else state = 0;
      }
      else state = 0;
    
      // Message has been decoded
      if(state == 5)
      {
        if((FrSkyPixelOsd::osd_command_t)(frame[0]) == expectedCmdId)
        {
          result = OSD_CMD_ERR_NONE;
          if((expectedCmdId == FrSkyPixelOsd::CMD_INFO) && (frame[1] == 'A') && (frame[2] == 'G') && (frame[3] == 'H')) { if(response != NULL) memcpy(response, frame + 1, sizeof(osd_cmd_info_response_t)); }
          else if(expectedCmdId == FrSkyPixelOsd::CMD_READ_FONT) { if(response != NULL) memcpy(response, frame + 3, sizeof(osd_chr_data_t)); }
          else if(expectedCmdId == FrSkyPixelOsd::CMD_WRITE_FONT) { if(response != NULL) memcpy(response, frame + 3, sizeof(osd_chr_data_t)); }
          else if(expectedCmdId == FrSkyPixelOsd::CMD_GET_CAMERA) { if(response != NULL) *((uint8_t*)response) = frame[1]; }
          else if(expectedCmdId == FrSkyPixelOsd::CMD_GET_ACTIVE_CAMERA) { if(response != NULL) *((uint8_t*)response) = frame[1]; }
          else if(expectedCmdId == FrSkyPixelOsd::CMD_GET_OSD_ENABLED) { if(response != NULL) *((bool*)response) = (frame[1] > 0) ? true : false; }
          else if(expectedCmdId == FrSkyPixelOsd::CMD_GET_SETTINGS) { if(response != NULL) memcpy(response, frame + 1, sizeof(osd_cmd_settings_response_t)); }
          else if(expectedCmdId == FrSkyPixelOsd::CMD_SET_SETTINGS) { if(response != NULL) memcpy(response, frame + 1, sizeof(osd_cmd_settings_response_t)); }
          else if(expectedCmdId == FrSkyPixelOsd::CMD_SAVE_SETTINGS) { if(response != NULL) *((uint8_t*)response) = frame[1]; }
          else if((expectedCmdId == FrSkyPixelOsd::CMD_WIDGET_SET_CONFIG) && (payload != NULL) && (*((osd_widget_id_t*)payload) == (osd_widget_id_t)frame[1]))
          {
            // Check the type of response...
            osd_widget_id_t id = (osd_widget_id_t)frame[1];
            uint8_t responseSize = 0;
            if(id == FrSkyPixelOsd::WIDGET_ID_AHI) responseSize = sizeof(osd_widget_ahi_config_t);
            else if((id == FrSkyPixelOsd::WIDGET_ID_SIDEBAR_0) || (id == FrSkyPixelOsd::WIDGET_ID_SIDEBAR_0)) responseSize = sizeof(osd_widget_sidebar_config_t);
            else if((id == FrSkyPixelOsd::WIDGET_ID_GRAPH_0) || (id == FrSkyPixelOsd::WIDGET_ID_GRAPH_1) ||
                    (id == FrSkyPixelOsd::WIDGET_ID_GRAPH_2) || (id == FrSkyPixelOsd::WIDGET_ID_GRAPH_3)) responseSize = sizeof(osd_widget_graph_config_t);
            else if((id == FrSkyPixelOsd::WIDGET_ID_CHARGAUGE_0) || (id == FrSkyPixelOsd::WIDGET_ID_CHARGAUGE_1) ||
                    (id == FrSkyPixelOsd::WIDGET_ID_CHARGAUGE_2) || (id == FrSkyPixelOsd::WIDGET_ID_CHARGAUGE_3)) responseSize = sizeof(osd_widget_chargauge_config_t);
            // ...and populate if recognized
            if((responseSize > 0) && (response != NULL)) memcpy(response, frame + 2, responseSize);
          }
          else if(expectedCmdId == FrSkyPixelOsd::CMD_SET_DATA_RATE) { if(response != NULL) memcpy(response, frame + 1, sizeof(uint32_t)); }
          else result = OSD_CMD_ERR_RESPONSE_TIMEOUT;
          
          if(result != OSD_CMD_ERR_RESPONSE_TIMEOUT) break;
        }
        else if (((FrSkyPixelOsd::osd_command_t)(frame[0]) == FrSkyPixelOsd::CMD_ERROR) && ((FrSkyPixelOsd::osd_command_t)(frame[1]) == expectedCmdId))
        {
          result = (FrSkyPixelOsd::osd_error_t)(frame[2]);
          break;
        }
        state = 0;
      }
    }
    currentTime = millis();
  }
  return result;
}

void FrSkyPixelOsd::sendCmd(FrSkyPixelOsd::osd_command_t id, const void *payload, uint32_t payloadLen, const void *varPayload, uint32_t varPayloadLen, bool sendVarPayloadLen)
{
  uint8_t crc = 0;
  uint8_t messageLen;
  if(payload == NULL) payloadLen = 0;             // Ensure the payload size is zeroed when payload is not specified  
  messageLen = payloadLen + 1;                    // Message length (payload + 1 byte for command ID
  if(varPayload != NULL)
  {
    // If variable payload is given increase message length by size of variable payload and (if requested) Uvarint carrying the size
    messageLen += varPayloadLen;
    if(sendVarPayloadLen == true) messageLen += getUvarintLen(varPayloadLen);
  }
  osdSerial->print("$A");                         // Message header
  sendUvarint(&crc, messageLen);                  // Message length as Uvarint (1 byte for command ID plus payload length if any, plus variable payload len and variable payload size len if any)
  sendByte(&crc, id);                             // Command ID
  sendPayload(&crc, payload, payloadLen);         // Command payload (if any)
  if(varPayload != NULL)                            
  {
    if(sendVarPayloadLen == true) sendUvarint(&crc, varPayloadLen); // Command variable payload (if any) length as uvariant
    sendPayload(&crc, varPayload, varPayloadLen); // Command variable payload (if any)
  }
  osdSerial->write(crc);                          // Message CRC
  osdSerial->flush();                             // Ensure the whole message is sent
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdInfo(FrSkyPixelOsd::osd_cmd_info_response_t *response)
{
  uint8_t payload = OSD_MAX_API_VERSION;
  sendCmd(FrSkyPixelOsd::CMD_INFO, &payload, sizeof(payload));
  return receive(FrSkyPixelOsd::CMD_INFO, NULL, response);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdReadFont(uint16_t character, FrSkyPixelOsd::osd_chr_data_t *response)
{
  sendCmd(FrSkyPixelOsd::CMD_READ_FONT, &character, sizeof(character));
  return receive(FrSkyPixelOsd::CMD_READ_FONT, NULL, response);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdWriteFont(uint16_t character, const FrSkyPixelOsd::osd_chr_data_t *font, FrSkyPixelOsd::osd_chr_data_t *response)
{
  FrSkyPixelOsd::osd_cmd_font_t payload = { .chr = character, .data = *font };
  sendCmd(FrSkyPixelOsd::CMD_WRITE_FONT, &payload, sizeof(payload));
  return receive(FrSkyPixelOsd::CMD_WRITE_FONT, NULL, response);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdGetCamera(uint8_t *response)
{
  sendCmd(FrSkyPixelOsd::CMD_GET_CAMERA);
  return receive(FrSkyPixelOsd::CMD_GET_CAMERA, NULL, response);
}

void FrSkyPixelOsd::cmdSetCamera(uint8_t camera)
{
  sendCmd(FrSkyPixelOsd::CMD_SET_CAMERA, &camera, sizeof(camera));
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdGetActiveCamera(uint8_t *response)
{
  sendCmd(FrSkyPixelOsd::CMD_GET_ACTIVE_CAMERA);
  return receive(FrSkyPixelOsd::CMD_GET_ACTIVE_CAMERA, NULL, response);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdGetOsdEnabled(bool *response)
{
  sendCmd(FrSkyPixelOsd::CMD_GET_OSD_ENABLED);
  return receive(FrSkyPixelOsd::CMD_GET_OSD_ENABLED, NULL, response);
}

void FrSkyPixelOsd::cmdSetOsdEnabled(bool enabled)
{
  uint8_t payload = (enabled == true) ? 1 : 0;
  sendCmd(FrSkyPixelOsd::CMD_SET_OSD_ENABLED, &payload, sizeof(payload));
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdGetSettings(uint8_t version, FrSkyPixelOsd::osd_cmd_settings_response_t *response)
{
  sendCmd(FrSkyPixelOsd::CMD_GET_SETTINGS, &version, sizeof(version));
  return receive(FrSkyPixelOsd::CMD_GET_SETTINGS, NULL, response);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdSetSettings(uint8_t version, int8_t brightness, int8_t horizontalOffset, int8_t verticalOffset, FrSkyPixelOsd::osd_cmd_settings_response_t *response)
{
  FrSkyPixelOsd::osd_cmd_settings_response_t payload = { .version = version, .brightness = brightness, .horizontalOffset = horizontalOffset, .verticalOffset = verticalOffset };
  sendCmd(FrSkyPixelOsd::CMD_SET_SETTINGS, &payload, sizeof(payload));
  return receive(FrSkyPixelOsd::CMD_SET_SETTINGS, NULL, response);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdSaveSettings(uint8_t *response)
{
  return receive(FrSkyPixelOsd::CMD_SAVE_SETTINGS, NULL, response);
}

void FrSkyPixelOsd::cmdTransactionBegin()
{
  sendCmd(FrSkyPixelOsd::CMD_TRANSACTION_BEGIN);
}

void FrSkyPixelOsd::cmdTransactionCommit()
{
  sendCmd(FrSkyPixelOsd::CMD_TRANSACTION_COMMIT);
}

void FrSkyPixelOsd::cmdTransactionBeginProfiled(int16_t x, int16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_TRANSACTION_BEGIN_PROFILED, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdTransactionBeginResetDrawing()
{
  sendCmd(FrSkyPixelOsd::CMD_TRANSACTION_BEGIN_RESET_DRAWING);
}

void FrSkyPixelOsd::cmdSetStrokeColor(FrSkyPixelOsd::osd_color_t color)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_STROKE_COLOR, &color, sizeof(color));
}

void FrSkyPixelOsd::cmdSetFillColor(FrSkyPixelOsd::osd_color_t color)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_FILL_COLOR, &color, sizeof(color));
}

void FrSkyPixelOsd::cmdSetStrokeAndFillColor(FrSkyPixelOsd::osd_color_t color)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_STROKE_AND_FILL_COLOR, &color, sizeof(color));
}

void FrSkyPixelOsd::cmdSetColorInversion(bool enabled)
{
  uint8_t payload = (enabled == true) ? 1 : 0;
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_COLOR_INVERSION, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdSetPixel(int16_t x, int16_t y, FrSkyPixelOsd::osd_color_t color)
{
  FrSkyPixelOsd::osd_cmd_set_pixel_data_t payload = { .point = { .x = x, .y = y }, .color = color };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_PIXEL, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdSetPixelToStrokeColor(int16_t x, int16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_PIXEL_TO_STROKE_COLOR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdSetPixelToFillColor(int16_t x, int16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_PIXEL_TO_FILL_COLOR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdSetStrokeWidth(uint8_t width)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_STROKE_WIDTH, &width, sizeof(width));
}

void FrSkyPixelOsd::cmdSetLineOutlineType(FrSkyPixelOsd::osd_outline_t outline)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_LINE_OUTLINE_TYPE, &outline, sizeof(outline));
}

void FrSkyPixelOsd::cmdSetLineOutlineColor(FrSkyPixelOsd::osd_color_t color)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_LINE_OUTLINE_COLOR, &color, sizeof(color));
}

void FrSkyPixelOsd::cmdClipToRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_CLIP_TO_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdClearScreen()
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_CLEAR_SCREEN);
}
  
void FrSkyPixelOsd::cmdClearRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_CLEAR_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdDrawingReset()
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_RESET);
}

void FrSkyPixelOsd::cmdDrawChar(int16_t x, int16_t y, uint16_t character, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_char_data_t payload = { .point = { .x = x, .y = y }, .chr = character, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_CHAR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdDrawCharMask(int16_t x, int16_t y, uint16_t character, FrSkyPixelOsd::osd_bitmap_opts_t options, FrSkyPixelOsd::osd_color_t color)
{
  FrSkyPixelOsd::osd_cmd_draw_char_mask_data_t payload = { .point = { .x = x, .y = y }, .chr = character, .opts = options, .color = color };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_CHAR_MASK, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdDrawString(int16_t x, int16_t y, const char *string, uint32_t stringLen, FrSkyPixelOsd::FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_string_data_t payload = { .point = { .x = x, .y = y }, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_STRING, &payload, sizeof(payload), string, stringLen);
}

void FrSkyPixelOsd::cmdDrawStringMask(int16_t x, int16_t y, const char *string, uint32_t stringLen, FrSkyPixelOsd::osd_bitmap_opts_t options, FrSkyPixelOsd::osd_color_t color)
{
  FrSkyPixelOsd::osd_cmd_draw_string_mask_data_t payload = { .point = { .x = x, .y = y }, .opts = options, .color = color };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_STRING_MASK, &payload, sizeof(payload), string, stringLen);
}

void FrSkyPixelOsd::cmdDrawBitmap(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t *bitmap, uint32_t bitmapLen, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_bitmap_data_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height }, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_BITMAP, &payload, sizeof(payload), bitmap, bitmapLen);
}

void FrSkyPixelOsd::cmdDrawBitmapMask(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t *bitmap, uint32_t bitmapLen, FrSkyPixelOsd::osd_bitmap_opts_t options, FrSkyPixelOsd::osd_color_t color)
{
  FrSkyPixelOsd::osd_cmd_draw_bitmap_mask_data_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height }, .opts = options, .color = color };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_BITMAP_MASK, &payload, sizeof(payload), bitmap, bitmapLen);
}

void FrSkyPixelOsd::cmdMoveToPoint(int16_t x, int16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_MOVE_TO_POINT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdStrokeLineToPoint(int16_t x, int16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_STROKE_LINE_TO_POINT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdStrokeTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3)
{
  FrSkyPixelOsd::osd_triangle_t payload = { .point1 = { .x = x1, .y = y1 }, .point2 = { .x = x2, .y = y2 }, .point3 = { .x = x3, .y = y3 } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_STROKE_TRIANGLE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3)
{
  FrSkyPixelOsd::osd_triangle_t payload = { .point1 = { .x = x1, .y = y1 }, .point2 = { .x = x2, .y = y2 }, .point3 = { .x = x3, .y = y3 } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_TRIANGLE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillStrokeTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3)
{
  FrSkyPixelOsd::osd_triangle_t payload = { .point1 = { .x = x1, .y = y1 }, .point2 = { .x = x2, .y = y2 }, .point3 = { .x = x3, .y = y3 } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_STROKE_TRIANGLE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdStrokeRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_STROKE_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillStrokeRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_STROKE_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdStrokeEllipseInRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_STROKE_ELLIPSE_IN_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillEllipseInRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_ELLIPSE_IN_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillStrokeEllipseInRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_STROKE_ELLIPSE_IN_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmReset()
{
  sendCmd(FrSkyPixelOsd::CMD_CTM_RESET);
}

void FrSkyPixelOsd::cmdCtmSet(float m11, float m12, float m21, float m22, float m31, float m32)
{
  FrSkyPixelOsd::osd_cmt_t payload = { .m11 = m11, .m12 = m12, .m21 = m21, .m22 = m22, .m31 = m31, .m32 = m32 };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SET, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmTranstale(float tx, float ty)
{
  FrSkyPixelOsd::osd_transformation_t payload = { .x = tx, .y = ty };
  sendCmd(FrSkyPixelOsd::CMD_CTM_TRANSLATE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmScale(float sx, float sy)
{
  FrSkyPixelOsd::osd_transformation_t payload = { .x = sx, .y = sy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SCALE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmRotateRad(float angle)
{
  sendCmd(FrSkyPixelOsd::CMD_CTM_ROTATE, &angle, sizeof(angle));
}

void FrSkyPixelOsd::cmdCtmRotateDeg(int16_t angle)
{
  cmdCtmRotateRad((float)(angle * 71 / 4068.0));
}

void FrSkyPixelOsd::cmdCtmRotateAboutRad(float angle, float cx, float cy)
{
  FrSkyPixelOsd::osd_cmd_rotate_about_data_t payload = { .angle = angle, .cx = cx, .cy = cy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_ROTATE_ABOUT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmRotateAboutDeg(int16_t angle, float cx, float cy)
{
  cmdCtmRotateAboutRad((float)(angle * 71 / 4068.0), cx, cy);
}

void FrSkyPixelOsd::cmdCtmShear(float sx, float sy)
{
  FrSkyPixelOsd::osd_transformation_t payload = { .x = sx, .y = sy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SHEAR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmShearAbout(float sx, float sy, float cx, float cy)
{
  FrSkyPixelOsd::osd_cmd_shear_about_data_t payload = { .sx = sx, .sy = sy, .cx = cx, .cy = cy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SHEAR_ABOUT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmMultiply(float m11, float m12, float m21, float m22, float m31, float m32)
{
  FrSkyPixelOsd::osd_cmt_t payload = { .m11 = m11, .m12 = m12, .m21 = m21, .m22 = m22, .m31 = m31, .m32 = m32 };
  sendCmd(FrSkyPixelOsd::CMD_CTM_MULTIPLY, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmTranslateRev(float tx, float ty)
{
  FrSkyPixelOsd::osd_transformation_t payload = { .x = tx, .y = ty };
  sendCmd(FrSkyPixelOsd::CMD_CTM_TRANSLATE_REV, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmScaleRev(float sx, float sy)
{
  FrSkyPixelOsd::osd_transformation_t payload = { .x = sx, .y = sy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SCALE_REV, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmRotateRevRad(float angle)
{
  sendCmd(FrSkyPixelOsd::CMD_CTM_ROTATE_REV, &angle, sizeof(angle));
}

void FrSkyPixelOsd::cmdCtmRotateRevDeg(int16_t angle)
{
  cmdCtmRotateRevRad((float)(angle * 71 / 4068.0));
}

void FrSkyPixelOsd::cmdCtmRotateAboutRevRad(float angle, float cx, float cy)
{
  FrSkyPixelOsd::osd_cmd_rotate_about_data_t payload = { .angle = angle, .cx = cx, .cy = cy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_ROTATE_ABOUT_REV, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmRotateAboutRevDeg(int16_t angle, float cx, float cy)
{
  cmdCtmRotateAboutRevRad((float)(angle * 71 / 4068.0), cx, cy);
}
void FrSkyPixelOsd::cmdCtmShearRev(float sx, float sy)
{
  FrSkyPixelOsd::osd_transformation_t payload = { .x = sx, .y = sy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SHEAR_REV, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmShearAboutRev(float sx, float sy, float cx, float cy)
{
  FrSkyPixelOsd::osd_cmd_shear_about_data_t payload = { .sx = sx, .sy = sy, .cx = cx, .cy = cy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SHEAR_ABOUT_REV, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmMultiplyRev(float m11, float m12, float m21, float m22, float m31, float m32)
{
  FrSkyPixelOsd::osd_cmt_t payload = { .m11 = m11, .m12 = m12, .m21 = m21, .m22 = m22, .m31 = m31, .m32 = m32 };
  sendCmd(FrSkyPixelOsd::CMD_CTM_MULTIPLY_REV, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmI16Translate(int16_t tx, int16_t ty)
{
  FrSkyPixelOsd::osd_transformation_i16_t payload = { .x = tx, .y = ty };
  sendCmd(FrSkyPixelOsd::CMD_CTM_I16TRANSLATE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmU16Rotate(uint16_t angle)
{
  sendCmd(FrSkyPixelOsd::CMD_CTM_U16ROTATE, &angle, sizeof(angle));
}

void FrSkyPixelOsd::cmdCtmI16TranslateRev(int16_t tx, int16_t ty)
{
  FrSkyPixelOsd::osd_transformation_i16_t payload = { .x = tx, .y = ty };
  sendCmd(FrSkyPixelOsd::CMD_CTM_I16TRANSLATE_REV, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmU16RotateRev(uint16_t angle)
{
  sendCmd(FrSkyPixelOsd::CMD_CTM_U16ROTATE_REV, &angle, sizeof(angle));
}

void FrSkyPixelOsd::cmdContextPush()
{
  sendCmd(FrSkyPixelOsd::CMD_CONTEXT_PUSH);
}

void FrSkyPixelOsd::cmdContextPop()
{
  sendCmd(FrSkyPixelOsd::CMD_CONTEXT_POP);
}

void FrSkyPixelOsd::cmdDrawGridChar(uint8_t column, uint8_t row, uint16_t character, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_grid_char_data_t payload = { .column = column, .row = row, .chr = character, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAW_GRID_CHR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdDrawGridString(uint8_t column, uint8_t row, const char *string, uint32_t stringLen, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_grid_string_data_t payload = { .column = column, .row = row, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAW_GRID_STR, &payload, sizeof(payload), string, stringLen);
}

void FrSkyPixelOsd::cmdDrawGridChar2(uint8_t column, uint8_t row, uint16_t character, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_grid_chr2_data_t payload = { .column = column, .row = row, .chr = character, .opts = options, .asMask = 0, .color = 0 };
  sendCmd(FrSkyPixelOsd::CMD_DRAW_GRID_CHR_2, &payload, sizeof(payload));
}
  
void FrSkyPixelOsd::cmdDrawGridCharMask2(uint8_t column, uint8_t row, uint16_t character, FrSkyPixelOsd::osd_bitmap_opts_t options, osd_color_t color)
{
  FrSkyPixelOsd::osd_cmd_draw_grid_chr2_data_t payload = { .column = column, .row = row, .chr = character, .opts = options, .asMask = 1, .color = color };
  sendCmd(FrSkyPixelOsd::CMD_DRAW_GRID_CHR_2, &payload, sizeof(payload));
}
  
void FrSkyPixelOsd::cmdDrawGridString2(uint8_t column, uint8_t row, const char *string, uint32_t stringLen, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  if((string != NULL) && (stringLen > 0))
  {
    FrSkyPixelOsd::osd_cmd_draw_grid_str2_data_t payload = { .column = column, .row = row, .opts = options, .strSize = 0 };
    if(stringLen <= 15)
    {
      payload.strSize = stringLen;
      sendCmd(FrSkyPixelOsd::CMD_DRAW_GRID_STR_2, &payload, sizeof(payload), string, stringLen, false);
    }
    else
    {
      sendCmd(FrSkyPixelOsd::CMD_DRAW_GRID_STR_2, &payload, sizeof(payload), string, stringLen);
    }
  }
}
  
FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdWidgetSetConfigAhi(int16_t x, int16_t y, int16_t width, int16_t height, FrSkyPixelOsd::osd_widget_ahi_style_t style, uint8_t options, uint8_t crosshairMargin, uint8_t strokeWidth, osd_widget_id_t id, osd_widget_ahi_config_t *response)
{
  FrSkyPixelOsd::osd_widget_ahi_config_t payload = { .rect = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } },
                                                 .style = style, .options = options, .crosshairMargin = crosshairMargin, .strokeWidth = strokeWidth };
  sendCmd(FrSkyPixelOsd::CMD_WIDGET_SET_CONFIG, &id, sizeof(id), &payload, sizeof(payload), false);
  return receive(FrSkyPixelOsd::CMD_WIDGET_SET_CONFIG, &id, response);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdWidgetSetConfigSidebar(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t options, uint8_t divisions, uint16_t countsPerStep,
                                              uint16_t scale, uint16_t symbol, uint16_t divisor, uint16_t dividedSymbol, osd_widget_id_t id, osd_widget_sidebar_config_t *response)
{
  FrSkyPixelOsd::osd_widget_sidebar_config_t payload = { .rect = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } },
                                                     .options = options, .divisions = divisions, .countsPerStep = countsPerStep, 
                                                     .unit = { .scale = scale, .symbol = symbol, .divisor = divisor, .dividedSymbol = dividedSymbol } };
  sendCmd(FrSkyPixelOsd::CMD_WIDGET_SET_CONFIG, &id, sizeof(id), &payload, sizeof(payload), false);
  return receive(FrSkyPixelOsd::CMD_WIDGET_SET_CONFIG, &id, response);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdWidgetSetConfigGraph(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t options, uint8_t yLabelCount, uint8_t yLabelWidth, uint8_t initialScale,
                                            uint16_t scale, uint16_t symbol, uint16_t divisor, uint16_t dividedSymbol, osd_widget_id_t id, osd_widget_graph_config_t *response)
{
  FrSkyPixelOsd::osd_widget_graph_config_t payload = { .rect = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } },
                                                   .options = options, .yLabelCount = yLabelCount, .yLabelWidth = yLabelWidth, .initialScale = initialScale, 
                                                   .unit = { .scale = scale, .symbol = symbol, .divisor = divisor, .dividedSymbol = dividedSymbol } };
  sendCmd(FrSkyPixelOsd::CMD_WIDGET_SET_CONFIG, &id, sizeof(id), &payload, sizeof(payload), false);
  return receive(FrSkyPixelOsd::CMD_WIDGET_SET_CONFIG, &id, response);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdWidgetSetConfigCharGauge(int16_t x, int16_t y, uint16_t character, osd_widget_id_t id, osd_widget_chargauge_config_t *response)
{
  FrSkyPixelOsd::osd_widget_chargauge_config_t payload = { .point = { .x = x, .y = y }, .chr = character };
  sendCmd(FrSkyPixelOsd::CMD_WIDGET_SET_CONFIG, &id, sizeof(id), &payload, sizeof(payload), false);
  return receive(FrSkyPixelOsd::CMD_WIDGET_SET_CONFIG, &id, response);
}

void FrSkyPixelOsd::cmdWidgetDrawAhiRad(float pitch, float roll, osd_widget_id_t id)
{
  cmdWidgetDrawAhiDeg((int16_t)round((pitch * 4068.0) / 71.0), (int16_t)round((roll * 4068.0) / 71.0), id);
}

void FrSkyPixelOsd::cmdWidgetDrawAhiDeg(int16_t pitch, int16_t roll, osd_widget_id_t id)
{
  FrSkyPixelOsd::osd_widget_ahi_pitch_roll_t payload = { .pitch = (uint16_t)map(pitch, -180, 180, -2048, 2047), .roll = (uint16_t)map(roll, -180, 180, -2048, 2047) };
  sendCmd(FrSkyPixelOsd::CMD_WIDGET_DRAW, &id, sizeof(id), &payload, sizeof(payload), false);
}

void FrSkyPixelOsd::cmdWidgetDrawSidebar(int32_t value, osd_widget_id_t id)
{
  FrSkyPixelOsd::osd_widget_sidebar_draw_t payload = { .value = value };
  sendCmd(FrSkyPixelOsd::CMD_WIDGET_DRAW, &id, sizeof(id), &payload, sizeof(payload), false);
}

void FrSkyPixelOsd::cmdWidgetDrawGraph(int32_t value, osd_widget_id_t id)
{
  FrSkyPixelOsd::osd_widget_graph_draw_t payload = { .value = value };
  sendCmd(FrSkyPixelOsd::CMD_WIDGET_DRAW, &id, sizeof(id), &payload, sizeof(payload), false);
}
  
void FrSkyPixelOsd::cmdWidgetDrawCharGauge(uint8_t value, osd_widget_id_t id)
{
  FrSkyPixelOsd::osd_widget_chargauge_draw_t payload = { .value = value };
  sendCmd(FrSkyPixelOsd::CMD_WIDGET_DRAW, &id, sizeof(id), &payload, sizeof(payload), false);
}

void FrSkyPixelOsd::cmdWidgetErase(osd_widget_id_t id)
{
  sendCmd(FrSkyPixelOsd::CMD_WIDGET_ERASE, &id, sizeof(id));
}

void FrSkyPixelOsd::cmdReboot(bool toBootloader)
{
  uint8_t payload = (toBootloader == true) ? 1 : 0;
  sendCmd(FrSkyPixelOsd::CMD_REBOOT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdWriteFlash()
{
  sendCmd(FrSkyPixelOsd::CMD_WRITE_FLASH);
}

FrSkyPixelOsd::osd_error_t FrSkyPixelOsd::cmdSetDataRate(uint32_t dataRate, uint32_t *response)
{
  sendCmd(FrSkyPixelOsd::CMD_SET_DATA_RATE, &dataRate, sizeof(dataRate));
  return receive(FrSkyPixelOsd::CMD_SET_DATA_RATE, NULL, response);
}

uint8_t FrSkyPixelOsd::setFontMetadata(uint8_t metadataType, const void *metadataContent, uint8_t metadataSize, uint8_t position, uint8_t *metadata)
{
  uint8_t size = 0;
  if((metadata != NULL) && (metadataContent != NULL) && (metadataSize > 0) && (position + metadataSize < OSD_MAX_FONT_METADATA_SIZE))
  {
    *(metadata + position) = metadataType;
    memcpy(metadata + position + 1, metadataContent, metadataSize);
    size = metadataSize + 1;
  }
  return size;
}

uint8_t FrSkyPixelOsd::setFontMetadataSize(uint8_t size, uint8_t position, uint8_t*metadata)
{
  FrSkyPixelOsd::osd_font_char_metadata_size_t content = { .sz = size };
  return setFontMetadata('s', &content, sizeof(content), position, metadata);
}

uint8_t FrSkyPixelOsd::setFontMetadataOffset(int8_t x, int8_t y, uint8_t position, uint8_t *metadata)
{
  FrSkyPixelOsd::osd_font_char_metadata_offset_t content = { .x = x, .y = y };
  return setFontMetadata('o', &content, sizeof(content), position, metadata);
}

uint8_t FrSkyPixelOsd::setFontMetadataRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t position, uint8_t *metadata)
{
  FrSkyPixelOsd::osd_font_char_metadata_rect_t content = { .x = x, .y = y, .w = width, .h = height };
  return setFontMetadata('r', &content, sizeof(content), position, metadata);
}

uint8_t FrSkyPixelOsd::setFontMetadataColors(uint8_t color0, uint8_t color1, uint8_t color2, uint8_t color3, uint8_t position, uint8_t *metadata)
{
  FrSkyPixelOsd::osd_font_char_metadata_colors_t content = { .color0 = color0, .color1 = color1, .color2 = color2, .color3 = color3 };
  return setFontMetadata('c', &content, sizeof(content), position, metadata);
}

void FrSkyPixelOsd::clearFontMetadata(uint8_t fromPosition, uint8_t *metadata)
{
  if((metadata != NULL) && (fromPosition < OSD_MAX_FONT_METADATA_SIZE)) memset(metadata, 0x55, OSD_MAX_FONT_METADATA_SIZE - fromPosition);
}