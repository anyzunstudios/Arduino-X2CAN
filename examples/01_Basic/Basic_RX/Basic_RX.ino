#include <X2CAN.h>

X2CAN CAN1(10, 2);   // CS=10, INT=2

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (CAN1.begin(CAN_SPEED_500KBPS_16MHZ) != CAN_OK) {
    Serial.println("X2CAN init failed");
    while (1);
  }

  Serial.println("X2CAN Basic RX ready");
}

void loop() {
  if (CAN1.checkReceive() == CAN_MSGAVAIL) {
    INT8U len;
    INT8U buf[8];

    if (CAN1.readMsgBuf(&len, buf) == CAN_OK) {
      INT32U id = CAN1.getCanId();

      Serial.print("RX ID=0x");
      Serial.print(id, HEX);
      Serial.print(" DLC=");
      Serial.print(len);
      Serial.print(" DATA=");

      for (INT8U i = 0; i < len; i++) {
        Serial.print(" 0x");
        Serial.print(buf[i], HEX);
      }
      Serial.println();
    }
  }
}
