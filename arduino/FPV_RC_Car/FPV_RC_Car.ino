#include <FrSkyPixelOsd.h>

#include <Servo.h>

#define NUM_CHANNELS 8
#define STEERING_TRIM -5

#define MIN_uS_IN 950
#define MAX_uS_IN 2050

enum DriveMode {
    NO_CONNECTION,
    DIRECT,
    TURN_ASSIST,
    OFF
};

struct OSDDrawInfo {
  DriveMode driveMode;
};

//############## PPM READING ##############
#define PULSE_BUFFER_SIZE (NUM_CHANNELS+1)

unsigned int pulseBuffer[PULSE_BUFFER_SIZE];
int pulseHead = 0;
int pulseTail = 1;
int numPulses = 0;
uint32_t last_pulse = micros();
void pushPulseRight(unsigned int pulse){
  if(numPulses < PULSE_BUFFER_SIZE){
    pulseBuffer[pulseTail] = pulse;
    numPulses += 1;
    pulseTail = (pulseTail + 1) % PULSE_BUFFER_SIZE;
  }
}

unsigned int popPulseLeft(){
  if(numPulses > 0){
    unsigned int pulseOut = pulseBuffer[pulseHead];
    numPulses -= 1;
    pulseHead = (pulseHead + 1) % PULSE_BUFFER_SIZE;
    return pulseOut;
  }
  return 0;
}

void onPulse(){
  uint32_t elapsed = micros() - last_pulse;
  last_pulse = micros();
  pushPulseRight(elapsed);
}

unsigned int *getVal(int index){
  if(0 < numPulses && index < numPulses){
    return &pulseBuffer[(pulseHead+index)%PULSE_BUFFER_SIZE];
  }
  return NULL;
}
//###########################################

uint32_t last_update = millis();
uint32_t last_draw = millis();
Servo steering, throttle;
float channels[NUM_CHANNELS];
FrSkyPixelOsd osd(&Serial1);
OSDDrawInfo drawInfo;

void setup() {
  Serial.begin(115200);
  steering.attach(5);
  throttle.attach(13);
  steering.write(90);
  throttle.write(90);
  osd.begin();
  osd.cmdSetStrokeColor(FrSkyPixelOsd::COLOR_WHITE);
  osd.cmdSetStrokeWidth(3);
  attachInterrupt(SDA, onPulse, RISING);
}

boolean updateChannels(){
  boolean valid = true;
  float range = (float)MAX_uS_IN - (float)MIN_uS_IN;
  for (int i = 0; i<NUM_CHANNELS; i++){
    unsigned int *vIn = getVal(i+1);
    if(vIn == NULL){
      valid = false;
    }
    Serial.printf("%4d ", *vIn);
    channels[i] = ((float)(*vIn) - (float)MIN_uS_IN) / range;
  }
  Serial.println();
  return valid;
}

void loop() {
  if(numPulses > 0){
    if(*getVal(0) < 4000){
      popPulseLeft();
    }
  }
  
  if(numPulses == PULSE_BUFFER_SIZE){
    noInterrupts();
    boolean valid = updateChannels();
    if(valid){
      int steeringAmount = 90;
      int throttleAmount = 90;
      if(channels[2] < 0.33) {
        steeringAmount = (int)(channels[1] * 180.f);
        throttleAmount = (int)(channels[0] * 180.f);
        drawInfo.driveMode = DriveMode::DIRECT; 
      }else if(channels[2] < 0.66) {
        steeringAmount = (int)(channels[1] * 180.f);
        float turn_reduction_strength = 0.7f*(2.f * abs(channels[1]-0.5f));
        float turn_reduction_factor = 1.f - turn_reduction_strength;
        
        throttleAmount = (int)(channels[0] * turn_reduction_factor * 180.f);
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
      //Serial.printf("Steering: %3d Throttle: %3d\n", steeringAmount, throttleAmount);
      last_update = millis();
    }
    else{
      Serial.println("No connection");
      steering.write(90);
      throttle.write(90);
      drawInfo.driveMode = DriveMode::NO_CONNECTION;
    }
    pulseHead = 0;
    pulseTail = 1;
    numPulses = 0;
    interrupts();
  }

  if(millis() - last_draw > 40) {
    osd.cmdTransactionBegin();
    char *string;
    switch(drawInfo.driveMode){
      case DriveMode::DIRECT:
        string = "DIRECT";
        osd.cmdDrawString(1, 1, string, sizeof(string));
      break;
      case DriveMode::NO_CONNECTION:
        string = "NO CONNECTION";
        osd.cmdDrawString(1, 1, string, sizeof(string));
      break;
      case DriveMode::TURN_ASSIST:
        string = "TURN ASSIST";
        osd.cmdDrawString(1, 1, string, sizeof(string));
      break;
      case DriveMode::OFF:
        string = "OFF";
        osd.cmdDrawString(1, 1, string, sizeof(string));
      break;
      
    }
    
    osd.cmdTransactionCommit();
    last_draw = millis();
  }
}
