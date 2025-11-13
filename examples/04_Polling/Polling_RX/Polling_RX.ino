#include <X2CAN.h>

// No INT pin used
X2CAN CAN1(10, 255);

void setup() {
  Serial.begin(115200);
  CAN1.begin(CAN_SPEED_250KBPS_16MHZ);
  Serial.println("Polling RX ready (software-only polling)");
}

void loop() {
  if (CAN1.checkPoll() == CAN_MSGAVAIL) {
    INT8U len, buf[8];
    if (CAN1.readMsgBuf(&len, buf) == CAN_OK) {
      Serial.print("Polling RX ID=0x");
      Serial.println(CAN1.getCanId(), HEX);
    }
  }
}
