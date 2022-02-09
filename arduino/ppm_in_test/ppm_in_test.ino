#define PULSE_BUFFER_SIZE 9

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  attachInterrupt(SDA, onPulse, RISING);
}

void loop() {
  unsigned int *headVal = getVal(0);
  if(headVal != NULL && *headVal < 3000){
    popPulseLeft();
  }
  
  if(numPulses == PULSE_BUFFER_SIZE){
    noInterrupts();
    for(int i = 0; i<PULSE_BUFFER_SIZE; i++){
      Serial.printf("%4ld ", *getVal(i));
    }
    Serial.println();
    pulseHead = 0;
    pulseTail = 1;
    numPulses = 0;
    interrupts();
  }
}
