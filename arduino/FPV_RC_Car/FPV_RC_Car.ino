#include <Arduino.h>
#include "wiring_private.h"
#include <CRSFIn.h>

#include <FrSkyPixelOsd.h>

#include <Servo.h>

#define STEERING_TRIM -5

//#define OSD_ON

enum DriveMode {
    NO_CONNECTION,
    DIRECT,
    TURN_ASSIST,
    OFF
};

struct OSDDrawInfo {
  DriveMode driveMode;
  uint8_t led[3];
  float throttle;
  float steering;
};

Uart Serial2(&sercom3, SCL, SDA, SERCOM_RX_PAD_0, UART_TX_PAD_2);
uint32_t last_update = millis();
uint32_t last_draw = millis();
Servo steering, throttle;
#ifdef OSD_ON
FrSkyPixelOsd osd(&Serial1);
#endif
OSDDrawInfo drawInfo;
CRSFIn remote;

void SERCOM3_Handler(){
  Serial2.IrqHandler();
}

void setup() { 
  pinPeripheral(SDA, PIO_SERCOM);
  pinPeripheral(SCL, PIO_SERCOM);
  steering.attach(5);
  throttle.attach(13);
  steering.write(90);
  throttle.write(90);
  #ifdef OSD_ON
  osd.begin();
  delay(500);
  osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
  osd.cmdSetStrokeWidth(3);
  delay(500);
  #endif
  remote.begin(&Serial2);
}
void loop() {

  
  if(remote.update()){
      int steeringAmount = 90;
      int throttleAmount = 90;
      float throttleInput = remote.getChannelFloat(0);
      float steeringInput = remote.getChannelFloat(1);
      float modeSelect = remote.getChannelFloat(2);
      if(modeSelect < 0.33) {
        steeringAmount = (int)(steeringInput * 180.f);
        throttleAmount = (int)(throttleInput * 180.f);
        drawInfo.driveMode = DriveMode::DIRECT; 
      }else if(modeSelect < 0.66) {
        steeringAmount = (int)(steeringInput * 180.f);
        float turn_reduction_strength = 0.7f*(2.f * abs(steeringInput-0.5f));
        float turn_reduction_factor = 1.f - turn_reduction_strength;
        float throttleCentered = 2.f*(throttleInput-0.5f);
        throttleAmount = (int)(throttleCentered * turn_reduction_factor * 90.f + 90.f);
        drawInfo.driveMode = DriveMode::TURN_ASSIST;
      }
      else {
        drawInfo.driveMode = DriveMode::OFF;
      }
      steeringAmount += STEERING_TRIM;
      steeringAmount = constrain(steeringAmount, 0, 180);
      throttleAmount = constrain(throttleAmount, 0, 180);
      steering.write(steeringAmount);
      throttle.write(throttleAmount);
      drawInfo.throttle = (float)(throttleAmount - 90) / 90.f;
      drawInfo.steering = (float)(steeringAmount - 90) / 90.f;     
      last_update = millis();
  }

  if(millis() - last_update > 500){
    //Serial.println("No connection");
    steering.write(90);
    throttle.write(90);
    drawInfo.driveMode = DriveMode::NO_CONNECTION;
  }
  #ifdef OSD_ON
  if(millis() - last_draw > 100) {
    osd.cmdTransactionBegin();
    osd.cmdClearRect(0, 0, 380, 252);
    if(drawInfo.driveMode == DriveMode::DIRECT){
      char string[] = "DIRECT";
      osd.cmdDrawGridString(1, 1, string, sizeof(string));
    }else if(drawInfo.driveMode == DriveMode::TURN_ASSIST){
      char string[] = "TURN ASSIST";
      osd.cmdDrawGridString(1, 1, string, sizeof(string));
    }else if(drawInfo.driveMode == DriveMode::NO_CONNECTION){
      char string[] = "NO CONNECTION";
      osd.cmdDrawGridString(1, 1, string, sizeof(string));
    }else if(drawInfo.driveMode == DriveMode::OFF){
      char string[] = "OFF";
      osd.cmdDrawGridString(1, 1, string, sizeof(string));
    }

    if(drawInfo.driveMode != DriveMode::NO_CONNECTION){
      const int startX = 41;
      const int startY = 40;
      int newX = startX + (int)round(drawInfo.steering * 30.f);
      osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_BLACK);
      osd.cmdSetStrokeWidth(10);
      osd.cmdMoveToPoint(startX, startY);
      osd.cmdStrokeLineToPoint(newX, startY);
      osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
      osd.cmdSetStrokeWidth(6);
      osd.cmdMoveToPoint(startX, startY);
      osd.cmdStrokeLineToPoint(newX, startY);
    }else{
      osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
      osd.cmdSetStrokeWidth(6);
      osd.cmdMoveToPoint(91, 40);
      osd.cmdStrokeLineToPoint(91 + (int)(80.f * sin(PI * 2 * (((float)(millis() - last_update)) / 1000.f)/3.f)), 40);
    }
    
    osd.cmdTransactionCommit();
    last_draw = millis();
  }
  #endif
}
