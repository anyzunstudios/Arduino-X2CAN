#include <X2CAN.h>

X2CAN CAN1(10, 2);

void setup() {
  Serial.begin(115200);
  CAN1.begin(CAN_SPEED_500KBPS_16MHZ);
  Serial.println("Raw TX ready");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();

    struct can_frame f;
    f.can_id  = 0x321;
    f.can_dlc = 3;
    f.data[0] = 0xDE;
    f.data[1] = 0xAD;
    f.data[2] = 0xBE;

    auto err = CAN1.rawSend(f);
    Serial.print("rawSend: ");
    Serial.println(err == MCP2515::ERROR_OK ? "OK" : "ERR");
  }
}
