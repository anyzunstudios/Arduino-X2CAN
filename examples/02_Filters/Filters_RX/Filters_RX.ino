#include <X2CAN.h>

X2CAN CAN1(10, 2);

void setup() {
  Serial.begin(115200);

  CAN1.begin(CAN_SPEED_500KBPS_16MHZ);

  // Accept only IDs from 0x100 to 0x1FF (simple example)
  CAN1.init_Mask(0, 0, 0x700);  // mask for first RX bank
  CAN1.init_Filt(0, 0, 0x100);  // base ID in that range

  Serial.println("Filters RX ready (only 0x100-0x1FF range)");
}

void loop() {
  if (CAN1.checkReceive() == CAN_MSGAVAIL) {
    INT8U len, buf[8];
    if (CAN1.readMsgBuf(&len, buf) == CAN_OK) {
      INT32U id = CAN1.getCanId();
      Serial.print("RX passing filter: ID=0x");
      Serial.println(id, HEX);
    }
  }
}
