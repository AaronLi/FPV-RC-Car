#define PULSE_BUFFER_SIZE 9

// initialize circular buffer variables
unsigned int pulseBuffer[PULSE_BUFFER_SIZE];
// head points at the first value in the buffer (if present)
int pulseHead = 0;
// head points at the first empty space in the buffer (if present)
int pulseTail = 1;
// number of pulse durations recorded
int numPulses = 0;
uint32_t last_pulse = micros();

// pushes a pulse length into the buffer if there's space
void pushPulseRight(unsigned int pulse){
  if(numPulses < PULSE_BUFFER_SIZE){
    pulseBuffer[pulseTail] = pulse;
    numPulses += 1;
    pulseTail = (pulseTail + 1) % PULSE_BUFFER_SIZE;
  }
}

// pops a pulse from the front of the buffer if there are elements in the buffer
unsigned int popPulseLeft(){
  if(numPulses > 0){
    unsigned int pulseOut = pulseBuffer[pulseHead];
    numPulses -= 1;
    pulseHead = (pulseHead + 1) % PULSE_BUFFER_SIZE;
    return pulseOut;
  }
  return 0;
}

// interrupt function, called on rising edge of ppm signal
void onPulse(){
  uint32_t elapsed = micros() - last_pulse; // time between last pulse and now
  last_pulse = micros();
  pushPulseRight(elapsed);
}

// get the index from the pulseBuffer (not always pulseBuffer[0] since head can move)
unsigned int *getVal(int index){
  if(0 < numPulses && index < numPulses){
    return &pulseBuffer[(pulseHead+index)%PULSE_BUFFER_SIZE];
  }
  return NULL;
}

void setup() {
  Serial.begin(115200);
  attachInterrupt(SDA, onPulse, RISING);
}

void loop() {
  unsigned int *headVal = getVal(0);
  // check if first value of buffer is a break (pause between ppm frames). If it isn't, remove the value (eventually it will be a long pause ~9000us)
  if (headVal != NULL && *headVal < 3000){
    popPulseLeft();
    headVal = getVal(0);
  }

  // if all 9 expected pulses are received and the head is a long pause
  if(numPulses == PULSE_BUFFER_SIZE && *headVal > 3000){
    noInterrupts();
    // print the values
    for(int i = 1; i<PULSE_BUFFER_SIZE; i++){
      Serial.printf("%4ld ", *getVal(i));
    }
    Serial.println();
    // clear the circular buffer
    pulseHead = 0;
    pulseTail = 1;
    numPulses = 0;
    interrupts();
  }
}
