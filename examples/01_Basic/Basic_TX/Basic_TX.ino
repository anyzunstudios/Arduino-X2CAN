#include <X2CAN.h>

X2CAN CAN1(10, 2);   // CS=10, INT=2

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (CAN1.begin(CAN_SPEED_500KBPS_16MHZ) != CAN_OK) {
    Serial.println("X2CAN init failed");
    while (1);
  }
  Serial.println("X2CAN Basic TX ready");
}

void loop() {
  static uint32_t lastSend = 0;
  if (millis() - lastSend >= 500) {
    lastSend = millis();

    uint8_t data[8] = {0xAA, 0x01, 0x00, 0x10, 0x55, 0xAA, 0x00, 0x00};
    INT8U status = CAN1.sendMsgBuf(0x123, 0, 8, data);

    Serial.print("TX status = ");
    Serial.println(status == CAN_OK ? "OK" : "FAIL");
  }
}
