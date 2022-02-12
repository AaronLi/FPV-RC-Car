#include <Arduino.h>
#include "wiring_private.h"
#include <CRSFIn.h>

Uart Serial2(&sercom3, SCL, SDA, SERCOM_RX_PAD_0, UART_TX_PAD_2);

void SERCOM3_Handler(){
  Serial2.IrqHandler();
}

CRSFIn reader;

void setup() {
  pinPeripheral(SDA, PIO_SERCOM);
  pinPeripheral(SCL, PIO_SERCOM);
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Listening");
  reader.begin(&Serial2);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(reader.update()){
    Serial.printf("%4d %4d %4d\n", reader.getChannelRaw(0), reader.getChannelRaw(1), reader.getChannelRaw(2));
  }
}
