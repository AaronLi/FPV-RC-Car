#include <AutoPID.h>

#include <SparkFunLSM6DS3_SPI.h>

#include <Arduino.h>
#include "wiring_private.h"
#include <CRSFIn.h>
#include <Servo.h>

#define STEERING_TRIM -10
#define GYRO_YAW_CAL 0.98795
#define DEBUG
//#define OSD_ON

#ifdef OSD_ON
#include <FrSkyPixelOsd.h>
FrSkyPixelOsd osd(&Serial1);

struct OSDDrawInfo {
  DriveMode driveMode;
  bool lightsOn = false;
  float throttle;
  float steering;
  float throttleInput;
  float steeringInput;
};
OSDDrawInfo drawInfo;
#endif

enum DriveMode {
  NO_CONNECTION,
  DIRECT,
  TURN_ASSIST,
  OFF
};
Uart Serial2(&sercom3, SCL, SDA, SERCOM_RX_PAD_0, UART_TX_PAD_2);
uint32_t last_update = millis();
uint32_t last_draw = millis();
Servo steering, throttle, lights;
CRSFIn remote;
LSM6DS3 imu(SPI_MODE, 2);
uint32_t last_sensor = micros();
double yaw = 0;
double yaw_v = 0;
double target_yaw_v = 0;
double turn_rate_out = 0;
float current_speed = 0;
DriveMode drive_mode = DriveMode::NO_CONNECTION;
DriveMode previous_drive_mode = DriveMode::NO_CONNECTION;
AutoPID turn_rate_pid(&yaw_v, &target_yaw_v, &turn_rate_out, -1, 1, 1.0, 0.0, 0.0);

void sensor_event() {
  double elapsed = (double)(micros() - last_sensor) / 1000000.0;
  double yaw_in = -(imu.readFloatGyroZ() + GYRO_YAW_CAL);
  yaw += yaw_in * elapsed;
  yaw_v = yaw_v * 0.8 + yaw_in * 0.2;
  last_sensor = micros();
}

void SERCOM3_Handler() {
  Serial2.IrqHandler();
}

void setup() {
  pinPeripheral(SDA, PIO_SERCOM);
  pinPeripheral(SCL, PIO_SERCOM);
  turn_rate_pid.setTimeStep(5);
#ifdef OSD_ON
  osd.begin();
  delay(500);
  osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
  osd.cmdSetStrokeWidth(3);
#endif
  remote.begin(&Serial2);

  steering.attach(5);
  throttle.attach(13);
  lights.attach(10);
  steering.write(90);
  throttle.write(90);
  lights.write(0);

  attachInterrupt(A5, sensor_event, RISING);

  imu.begin();
  imu.writeRegister(0x0D, 0b00000010);
  imu.readFloatGyroZ();
#ifdef DEBUG
  Serial.begin(115200);
#endif
}
void loop() {


  if (remote.update()) {
    int steeringAmount = 90;
    int throttleAmount = 90;
    float throttleInput = remote.getChannelFloat(0);
    float steeringInput = remote.getChannelFloat(1);
#ifdef OSD_ON
    drawInfo.throttleInput = throttleInput;
    drawInfo.steeringInput = steeringInput;
#endif
    float modeSelect = remote.getChannelFloat(2);
    if (modeSelect < 0.33) {
      drive_mode = DriveMode::OFF;
    } else if (modeSelect < 0.66) {
      drive_mode = DriveMode::DIRECT;
    } else {
      drive_mode = DriveMode::TURN_ASSIST;
    }
    float turn_assist_mode = remote.getChannelFloat(3);
    switch (drive_mode) {
      case DriveMode::DIRECT:
        steeringAmount = (int)(steeringInput * 180.f);
        throttleAmount = (int)(throttleInput * 180.f);
        break;
      case DriveMode::TURN_ASSIST:
        if (turn_assist_mode < 0.33) {
          steeringAmount = (int)(steeringInput * 180.f);
          float turn_reduction_strength = 0.7f * (2.f * abs(steeringInput - 0.5f));
          float turn_reduction_factor = 1.f - turn_reduction_strength;
          float throttleCentered = 2.f * (throttleInput - 0.5f);
          throttleAmount = (int)(throttleCentered * turn_reduction_factor * 90.f + 90.f);
//        } else if (turn_assist_mode < 0.66) {
        }else{
          if (previous_drive_mode != drive_mode) {
            turn_rate_pid.reset();
            Serial.println("pid reset");
          }
          throttleAmount = (int)(throttleInput * 180.f);
          target_yaw_v = (steeringInput - 0.5f) * 30.f;
          steeringAmount = (turn_rate_out + 1)*90.f;
          Serial.printf("target: %.2f current: %.2f output: %d ", target_yaw_v, yaw_v, steeringAmount);
        }
        break;
      case DriveMode::OFF:
        steeringAmount = 90;
        throttleAmount =  90;
        break;
    }
    steeringAmount += STEERING_TRIM;
    steeringAmount = constrain(steeringAmount, 0, 180);
    throttleAmount = constrain(throttleAmount, 0, 180);
    steering.write(steeringAmount);
    throttle.write(throttleAmount);
    lights.write(remote.getChannelFloat(3) * 180.f);
#ifdef OSD_ON
    drawInfo.lightsOn = (remote.getChannelFloat(3) * 180.f) > 90.f;
    drawInfo.throttle = (float)(throttleAmount - 90) / 90.f;
    drawInfo.steering = (float)(steeringAmount - 90) / 90.f;
#endif
    #ifdef DEBUG
      Serial.printf("steering: %d throttle: %d\n", steeringAmount, throttleAmount);
    #endif
    previous_drive_mode = drive_mode;
    last_update = millis();
  }

  if (millis() - last_update > 500) {
    //Serial.println("No connection");
    steering.write(90);
    throttle.write(90);
    lights.write(0);
#ifdef OSD_ON
    drawInfo.driveMode = DriveMode::NO_CONNECTION;
#endif
  }
#ifdef OSD_ON
  if (millis() - last_draw > 50 ) {
    osd.cmdTransactionBegin();
    osd.cmdClearRect(0, 0, 380, 252);
    if (drawInfo.driveMode == DriveMode::DIRECT) {
      char string[] = "DIRECT";
      osd.cmdDrawGridString(1, 1, string, sizeof(string));
    } else if (drawInfo.driveMode == DriveMode::TURN_ASSIST) {
      char string[] = "TURN ASSIST";
      osd.cmdDrawGridString(1, 1, string, sizeof(string));
    } else if (drawInfo.driveMode == DriveMode::NO_CONNECTION) {
      char string[] = "NO CONNECTION";
      osd.cmdDrawGridString(1, 1, string, sizeof(string));
    } else if (drawInfo.driveMode == DriveMode::OFF) {
      char string[] = "OFF";
      osd.cmdDrawGridString(1, 1, string, sizeof(string));
    }
    if (drawInfo.driveMode != DriveMode::NO_CONNECTION) {
      const int startX = 40;
      const int startY = 70;
      const int circleRadius = 30;

      osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
      osd.cmdStrokeEllipseInRect(startX - circleRadius, startY - circleRadius, 2 * circleRadius, 2 * circleRadius);
      for (int i = 1; i < 2; i++) {
        const int endX = (int)(circleRadius * cos(2. * PI * (2. * (drawInfo.steeringInput - 0.5)) / 2. + i * PI / 2.));
        const int endY = (int)(circleRadius * sin(2. * PI * (2. * (drawInfo.steeringInput - 0.5)) / 2. + i * PI / 2.));
        osd.cmdMoveToPoint(startX, startY);
        osd.cmdStrokeLineToPoint(startX + endX, startY + endY);
      }

      //      osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
      //      osd.cmdSetStrokeWidth(10);
      //      osd.cmdMoveToPoint(startX, startY);
      //      osd.cmdStrokeLineToPoint(newX, startY);
    } else {
      osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
      osd.cmdSetStrokeWidth(6);
      osd.cmdMoveToPoint(91, 40);
      osd.cmdStrokeLineToPoint(91 + (int)(80.f * sin(PI * 2 * (((float)(millis() - last_update)) / 1000.f) / 3.f)), 40);
    }

    osd.cmdTransactionCommit();
    last_draw = millis();
  }
  
#endif
turn_rate_pid.run();
}
