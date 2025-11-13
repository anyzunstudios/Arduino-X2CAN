#include <X2CAN.h>

// Node A: two separate CAN buses
X2CAN CAN_Motor(10, 2);   // CS=10, INT=2
X2CAN CAN_Comfort(9,  3); // CS=9,  INT=3

void setup() {
  Serial.begin(115200);

  CAN_Motor.begin(CAN_SPEED_500KBPS_16MHZ);
  CAN_Comfort.begin(CAN_SPEED_250KBPS_16MHZ);

  Serial.println("MultiBus NodeA ready (TX on Motor & Comfort)");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 500) {
    last = millis();

    uint8_t motor[8]   = {0x01,0x02,0x03,0x04,0,0,0,0};
    uint8_t comfort[8] = {0xA0,0xA1,0xA2,0xA3,0,0,0,0};

    CAN_Motor.sendMsgBuf(0x200, 0, 8, motor);
    CAN_Comfort.sendMsgBuf(0x300, 0, 8, comfort);

    Serial.println("TX on Motor(0x200) & Comfort(0x300)");
  }
}
