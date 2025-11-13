#include <X2CAN.h>

X2CAN CAN1(10, 2);

void setup() {
  Serial.begin(115200);
  CAN1.begin(CAN_SPEED_500KBPS_16MHZ);
  Serial.println("Filters TX ready");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();

    // Two IDs: one inside the filter range, one outside
    uint8_t dataIn[8]  = {0x11,0x22,0x33,0x44,0,0,0,0};
    uint8_t dataOut[8] = {0x99,0x88,0x77,0x66,0,0,0,0};

    CAN1.sendMsgBuf(0x105, 0, 8, dataIn);
    CAN1.sendMsgBuf(0x250, 0, 8, dataOut);

    Serial.println("Sent 0x105 (should pass) and 0x250 (should be filtered)");
  }
}
