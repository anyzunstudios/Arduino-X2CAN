#include <X2CAN.h>

X2CAN CAN_Motor(10, 2);
X2CAN CAN_Comfort(9,  3);

void setup() {
  Serial.begin(115200);

  CAN_Motor.begin(CAN_SPEED_500KBPS_16MHZ);
  CAN_Comfort.begin(CAN_SPEED_250KBPS_16MHZ);

  Serial.println("MultiBus NodeB ready (RX Motor & Comfort)");
}

void loop() {
  // Motor bus
  if (CAN_Motor.checkReceive() == CAN_MSGAVAIL) {
    INT8U len, buf[8];
    if (CAN_Motor.readMsgBuf(&len, buf) == CAN_OK) {
      Serial.print("[MOTOR] ID=0x");
      Serial.println(CAN_Motor.getCanId(), HEX);
    }
  }

  // Comfort bus
  if (CAN_Comfort.checkReceive() == CAN_MSGAVAIL) {
    INT8U len, buf[8];
    if (CAN_Comfort.readMsgBuf(&len, buf) == CAN_OK) {
      Serial.print("[COMFORT] ID=0x");
      Serial.println(CAN_Comfort.getCanId(), HEX);
    }
  }
}
