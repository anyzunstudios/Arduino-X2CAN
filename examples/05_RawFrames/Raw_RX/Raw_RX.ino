#include <X2CAN.h>

X2CAN CAN1(10, 2);

void setup() {
  Serial.begin(115200);
  CAN1.begin(CAN_SPEED_500KBPS_16MHZ);
  Serial.println("Raw RX ready");
}

void loop() {
  if (CAN1.checkPoll() == CAN_MSGAVAIL) {
    struct can_frame rx;
    if (CAN1.rawRead(rx) == MCP2515::ERROR_OK) {
      Serial.print("RAW RX ID=0x");
      Serial.print(rx.can_id & CAN_SFF_MASK, HEX); // basic print for standard IDs
      Serial.print(" DLC=");
      Serial.print(rx.can_dlc);
      Serial.print(" DATA=");
      for (uint8_t i = 0; i < rx.can_dlc; i++) {
        Serial.print(" 0x");
        Serial.print(rx.data[i], HEX);
      }
      Serial.println();
    }
  }
}
