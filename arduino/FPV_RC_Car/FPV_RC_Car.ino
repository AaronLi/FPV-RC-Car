#include <Arduino.h>
#include "wiring_private.h"
#include <CRSFIn.h>

#include <FrSkyPixelOsd.h>

#include <Servo.h>

#define STEERING_TRIM -10

//#define OSD_ON

enum DriveMode {
    NO_CONNECTION,
    DIRECT,
    TURN_ASSIST,
    OFF
};

struct OSDDrawInfo {
  DriveMode driveMode;
  bool lightsOn = false;
  float throttle;
  float steering;
  float throttleInput;
  float steeringInput;
};

Uart Serial2(&sercom3, SCL, SDA, SERCOM_RX_PAD_0, UART_TX_PAD_2);
uint32_t last_update = millis();
uint32_t last_draw = millis();
Servo steering, throttle, lights;
FrSkyPixelOsd osd(&Serial1);
OSDDrawInfo drawInfo;
CRSFIn remote;

void SERCOM3_Handler(){
  Serial2.IrqHandler();
}

void setup() { 
  pinPeripheral(SDA, PIO_SERCOM);
  pinPeripheral(SCL, PIO_SERCOM);
  osd.begin();
  delay(500);
  osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
  osd.cmdSetStrokeWidth(3);
  delay(500);
  remote.begin(&Serial2);

  steering.attach(5);
  throttle.attach(13);
  lights.attach(10);
  steering.write(90);
  throttle.write(90);
  lights.write(0);
}
void loop() {

  
  if(remote.update()){
      int steeringAmount = 90;
      int throttleAmount = 90;
      float throttleInput = remote.getChannelFloat(0);
      float steeringInput = remote.getChannelFloat(1);
      drawInfo.throttleInput = throttleInput;
      drawInfo.steeringInput = steeringInput;
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
      lights.write(remote.getChannelFloat(3) * 180.f);
      drawInfo.lightsOn = (remote.getChannelFloat(3) * 180.f) > 90.f;
      drawInfo.throttle = (float)(throttleAmount - 90) / 90.f;
      drawInfo.steering = (float)(steeringAmount - 90) / 90.f;     
      last_update = millis();
  }

  if(millis() - last_update > 500){
    //Serial.println("No connection");
    steering.write(90);
    throttle.write(90);
    lights.write(0);
    drawInfo.driveMode = DriveMode::NO_CONNECTION;
  }
  #ifdef OSD_ON
  if(millis() - last_draw > 50 ) {
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
      const int startX = 40;
      const int startY = 70;
      const int circleRadius = 30;
      
      osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
      osd.cmdStrokeEllipseInRect(startX - circleRadius, startY - circleRadius, 2 * circleRadius, 2 * circleRadius);
      for(int i = 1; i< 2; i++){
        const int endX = (int)(circleRadius * cos(2. * PI * (2. * (drawInfo.steeringInput - 0.5)) / 2. + i * PI / 2.));
        const int endY = (int)(circleRadius * sin(2. * PI * (2. * (drawInfo.steeringInput - 0.5)) / 2. + i * PI / 2.));
        osd.cmdMoveToPoint(startX, startY);
        osd.cmdStrokeLineToPoint(startX+endX, startY+endY);
      }
      
//      osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
//      osd.cmdSetStrokeWidth(10);
//      osd.cmdMoveToPoint(startX, startY);
//      osd.cmdStrokeLineToPoint(newX, startY);
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
