#include <VescUart.h>
#include <buffer.h>
#include <crc.h>
#include <datatypes.h>

#include <AutoPID.h>

#include <SparkFunLSM6DS3_SPI.h>

#include <Arduino.h>
#include "wiring_private.h"
#include <CRSFIn.h>
#include <Servo.h>

#define STEERING_TRIM 0
#define GYRO_YAW_CAL 1.2
#define STEERING_PIN 5
#define THROTTLE_PIN 13
#define PID_MAX_GAIN 1.0 / 512.0
#define PID_MIN_GAIN 0.025 / 512.0
#define PID_SCALE_POINT 20 // km/h
#define PID_I_TERM 1.0 / 32.0
#define PID_D_TERM 1.0 / 64.0
#define GYRO_LOWPASS_ALPHA 0.5  // lower is stronger filtering

#define MOTOR_POLES 2.0
#define DRIVE_RATIO 10.83
#define WHEEL_CIRCUMFERENCE 0.3676 // meters
#define KMH_TO_METERS_PER_MIN 16.66667
#define KMH_TO_MOTOR_ERPM ((DRIVE_RATIO * MOTOR_POLES * KMH_TO_METERS_PER_MIN) / WHEEL_CIRCUMFERENCE)

#define MAX_SPEED_KMH 10
#define MAX_STEERING_DEG_S 180.0
// #define DEBUG
//#define OSD_ON

enum DriveMode {
  NO_CONNECTION,
  DIRECT,
  TURN_ASSIST,
  OFF
};
Uart Serial2(&sercom3, 26, 27, SERCOM_RX_PAD_1, UART_TX_PAD_0);
uint32_t last_update = millis();
Servo steering, lights;
VescUart esc;
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
AutoPID turn_rate_pid(&yaw_v, &target_yaw_v, &turn_rate_out, -1, 1, PID_MAX_GAIN, PID_I_TERM, PID_D_TERM);

float PID_CONFIG_SPEED[] = {0.0, 5.0, 15.0};
float PID_CONFIG_VALUE[] = {1.0 / 512.0, 1.0 / 512.0, 1.0 / 20480.0};

float steeringCommand = 0;
float throttleCommand = 0;

void SERCOM3_Handler() {
  remote.IrqHandler();
}

float get_pid_p_for_speed_kmh(float speed){
  int choose_value = 0;
  for(; choose_value<sizeof(PID_CONFIG_SPEED) / sizeof(float); choose_value++){
    if(PID_CONFIG_SPEED[choose_value+1] > speed){
      break;
    }
  }
  #ifdef DEBUG
  Serial.printf("pid value: %d\n", choose_value);
  #endif
  return PID_CONFIG_VALUE[choose_value];
}

void handleRemote(){
  if (millis() - remote.last_frame_timestamp < 200) {
    #ifdef DEBUG
    Serial.println("Command");
    #endif
    float throttleInput = remote.getChannelFloat(0);
    float steeringInput = remote.getChannelFloat(1);
    float modeSelect = remote.getChannelFloat(2);
    float turn_assist_mode = remote.getChannelFloat(3);
    #ifdef DEBUG
    Serial.println(steeringInput);
    #endif
    if (modeSelect < 0.33) {
      drive_mode = DriveMode::OFF;
    } else if (modeSelect < 0.66) {
      drive_mode = DriveMode::DIRECT;
    } else {
      drive_mode = DriveMode::TURN_ASSIST;
    }
    
    switch (drive_mode) {
      case DriveMode::DIRECT:
        if (!steering.attached()) {
          steering.attach(STEERING_PIN, 1000, 2000);
        }

        steeringCommand = (steeringInput - 0.5) * 2.0;
        throttleCommand = (throttleInput - 0.5) * 2.0;
        break;
      case DriveMode::TURN_ASSIST:
        if (!steering.attached()) {
          steering.attach(STEERING_PIN, 1000, 2000);
        }
        
        if (previous_drive_mode != drive_mode) {
          turn_rate_pid.reset();
          #ifdef DEBUG
          Serial.println("pid reset");
          #endif
        }

        throttleCommand = (throttleInput - 0.5) * 2.0;
        target_yaw_v = ((double)steeringInput - 0.5) * 2.0 * MAX_STEERING_DEG_S;
      
        break;
      case DriveMode::OFF:
        steering.detach();
        throttleCommand = 0;
        break;
    }
    
    previous_drive_mode = drive_mode;
    last_update = millis();
  }
}

void handleEscTelemetry(){
  if(esc.getVescValues()){

    float motor_erpm = esc.data.rpm;
    current_speed = motor_erpm / KMH_TO_MOTOR_ERPM;
    uint16_t speed_kmh_mul_10 = (uint16_t)abs(current_speed*10.0);
    // Serial.printf("Read rpm %.2f battery v: %.2f\n", motor_erpm, esc.data.inpVoltage);
    crsfGpsFrame_t info = {
      .groundSpeed = speed_kmh_mul_10
    };
    remote.transmitGpsFrame(info);
    crsfVoltageFrame_t voltageInfo = {
      .voltage = (uint16_t)(esc.data.inpVoltage * 10.0),
      .current = (uint16_t)(esc.data.avgInputCurrent * 100.0)
    };
    remote.transmitVoltageFrame(voltageInfo);
  }
}

void executeCommands(){
  if (millis() - last_update > 500) {
    steering.write(90);
    steering.detach();
    lights.write(0);
  }else{
    steeringCommand = constrain(steeringCommand, -1., 1.);
    throttleCommand = constrain(throttleCommand, -1., 1.);
    float desired_kmh = throttleCommand * MAX_SPEED_KMH;
    float desired_erpm = desired_kmh * KMH_TO_MOTOR_ERPM;
    steering.write((int)(90. * (steeringCommand + 1.)));
    
    if(abs(desired_erpm) > 300) {
      esc.setRPM(desired_erpm);
    }else{
      esc.setRPM(0);
    }
  }
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  delay(1000);
  Serial.println("Begin!");
  #endif
  pinPeripheral(26, PIO_SERCOM);
  pinPeripheral(27, PIO_SERCOM);
  pinMode(A5, INPUT);
  turn_rate_pid.setTimeStep(1000 / 50);
  remote.begin(&Serial2);
  Serial1.begin(115200);
  esc.setSerialPort(&Serial1);
  // lights.attach(10);
  // lights.write(0);

  // attachInterrupt(A5, sensor_event, RISING);
  #ifdef DEBUG
  if(!imu.begin()){
    Serial.println("Failed imu init");
  }else{
    Serial.println("IMU init");
  };
  #else
  imu.begin();
  #endif
  imu.writeRegister(0x0D, 0b00000010);
  imu.readFloatGyroZ();
}

void loop() {
  if(digitalRead(A5)){
    double elapsed = (double)(micros() - last_sensor) / 1000000.0;
    double yaw_in = -(imu.readFloatGyroZ() + GYRO_YAW_CAL);
    yaw += yaw_in * elapsed;
    yaw_v = yaw_v * (1.0 - GYRO_LOWPASS_ALPHA) + yaw_in * GYRO_LOWPASS_ALPHA;
    last_sensor = micros();
  }
  handleEscTelemetry();
  
  handleRemote();
  
  double i_term = turn_rate_pid.getIntegral();
  if (abs(i_term) > 2) {
    turn_rate_pid.setIntegral(2.0 * i_term / abs(i_term));
  }
  turn_rate_pid.run();
  if(drive_mode == DriveMode::TURN_ASSIST) {
    steeringCommand = (float)turn_rate_out;
    float newPValue = get_pid_p_for_speed_kmh(current_speed);
    turn_rate_pid.setGains((double)newPValue, PID_I_TERM, PID_D_TERM);
    #ifdef DEBUG
    Serial.printf("target: %.2f current: %.2f output: %.2f\n", target_yaw_v, yaw_v, steeringCommand);
    #endif
  }

  executeCommands();
}