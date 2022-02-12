#include <Arduino.h>
#include "wiring_private.h"

#define CRSF_PAYLOAD_SIZE_MAX   32 // !!TODO needs checking
#define CRSF_FRAME_SIZE_MAX     (CRSF_PAYLOAD_SIZE_MAX + 4)

Uart Serial2(&sercom3, SCL, SDA, SERCOM_RX_PAD_0, UART_TX_PAD_2);

enum {
    CRSF_FRAME_GPS_PAYLOAD_SIZE = 15,
    CRSF_FRAME_BATTERY_SENSOR_PAYLOAD_SIZE = 8,
    CRSF_FRAME_LINK_STATISTICS_PAYLOAD_SIZE = 10,
    CRSF_FRAME_RC_CHANNELS_PAYLOAD_SIZE = 22, // 11 bits per channel * 16 channels = 22 bytes.
    CRSF_FRAME_ATTITUDE_PAYLOAD_SIZE = 6,
    CRSF_FRAME_LENGTH_ADDRESS = 1, // length of ADDRESS field
    CRSF_FRAME_LENGTH_FRAMELENGTH = 1, // length of FRAMELENGTH field
    CRSF_FRAME_LENGTH_TYPE = 1, // length of TYPE field
    CRSF_FRAME_LENGTH_CRC = 1, // length of CRC field
    CRSF_FRAME_LENGTH_TYPE_CRC = 2 // length of TYPE and CRC fields combined
};

struct crsfPayloadRcChannelsPacked_s {
    // 176 bits of data (11 bits per channel * 16 channels) = 22 bytes.
    unsigned int chan0 : 11;
    unsigned int chan1 : 11;
    unsigned int chan2 : 11;
    unsigned int chan3 : 11;
    unsigned int chan4 : 11;
    unsigned int chan5 : 11;
    unsigned int chan6 : 11;
    unsigned int chan7 : 11;
    unsigned int chan8 : 11;
    unsigned int chan9 : 11;
    unsigned int chan10 : 11;
    unsigned int chan11 : 11;
    unsigned int chan12 : 11;
    unsigned int chan13 : 11;
    unsigned int chan14 : 11;
    unsigned int chan15 : 11;
} __attribute__ ((__packed__));

typedef struct crsfPayloadRcChannelsPacked_s crsfPayloadRcChannelsPacked_t;

typedef struct crsfFrameDef_s {
    uint8_t deviceAddress;
    uint8_t frameLength;
    uint8_t type;
    uint8_t payload[CRSF_PAYLOAD_SIZE_MAX + 1]; // +1 for CRC at end of payload
} crsfFrameDef_t;

typedef union crsfFrame_u {
    uint8_t bytes[CRSF_FRAME_SIZE_MAX];
    crsfFrameDef_t frame;
} crsfFrame_t;


void SERCOM3_Handler(){
  Serial2.IrqHandler();
}

uint8_t crc8_dvb_s2(uint8_t crc, unsigned char a)

    {
        crc ^= a;
        for (int ii = 0; ii < 8; ++ii) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0xD5;
            } else {
                crc = crc << 1;
            }
        }
        return crc;
    }

crsfFrame_t frame;
uint8_t currentIndex = 0;
uint32_t frame_start_time = micros();

uint8_t crsfFrameCRC(void)
{
    // CRC includes type and payload
    uint8_t crc = crc8_dvb_s2(0, frame.frame.type);
    for (int ii = 0; ii < frame.frame.frameLength - CRSF_FRAME_LENGTH_TYPE_CRC; ++ii) {
        crc = crc8_dvb_s2(crc, frame.frame.payload[ii]);
    }
    return crc;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(400000);
  pinPeripheral(SDA, PIO_SERCOM);
  pinPeripheral(SCL, PIO_SERCOM);
  Serial.println("Listening");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial2.available() > 0){
    if(micros() - frame_start_time > 1000){
      currentIndex = 0;
    }
    if(currentIndex == 0){
      frame_start_time = micros();
    }
    const int fullFrameLength = currentIndex < 3 ? 5 : min(frame.frame.frameLength + CRSF_FRAME_LENGTH_ADDRESS + CRSF_FRAME_LENGTH_FRAMELENGTH, CRSF_FRAME_SIZE_MAX);
    if(currentIndex < fullFrameLength){
      int numBytesAvailable = Serial2.available();
      int numBytesRead = Serial2.readBytes(&frame.bytes[currentIndex], numBytesAvailable);
      currentIndex += numBytesRead;
      if(currentIndex >= fullFrameLength){
        if(frame.frame.type == 0x16 && frame.bytes[fullFrameLength-1] == crsfFrameCRC()){
          crsfPayloadRcChannelsPacked_t *channels = (crsfPayloadRcChannelsPacked_t*)&frame.frame.payload;
          Serial.printf("%4d %4d %4d\n", channels->chan0, channels->chan1, channels->chan2);
        }
      }
    }
  }
}
