#include <X2CAN.h>

// No INT pin used
X2CAN CAN1(10, 255);

void setup() {
  Serial.begin(115200);
  CAN1.begin(CAN_SPEED_250KBPS_16MHZ);
  Serial.println("Polling TX ready");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    uint8_t data[8] = {0x10,0x20,0x30,0x40,0,0,0,0};
    CAN1.sendMsgBuf(0x123, 0, 8, data);
    Serial.println("Polling TX sent frame");
  }
}
