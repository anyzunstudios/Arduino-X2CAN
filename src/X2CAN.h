#ifndef _X2CAN_H_
#define _X2CAN_H_

#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>   // autowp/arduino-mcp2515

// Legacy-style aliases
typedef uint8_t  INT8U;
typedef uint32_t INT32U;

// Status codes (roughly aligned with Seeed MCP_CAN)
#define CAN_OK                 0
#define CAN_FAILINIT           1
#define CAN_FAILTX             2
#define CAN_MSGAVAIL           3
#define CAN_NOMSG              4
#define CAN_CTRLERROR          5
#define CAN_FAIL               0xFF

// Speed + clock configuration wrapper
enum CAN_SPEED_CFG : uint8_t {
    CAN_SPEED_125KBPS_8MHZ,
    CAN_SPEED_250KBPS_8MHZ,
    CAN_SPEED_500KBPS_8MHZ,
    CAN_SPEED_125KBPS_16MHZ,
    CAN_SPEED_250KBPS_16MHZ,
    CAN_SPEED_500KBPS_16MHZ,
    CAN_SPEED_1000KBPS_16MHZ
};

class X2CAN
{
  private:
    MCP2515 mcp;
    uint8_t _slotIndex;   // 0..3 for up to 4 instances
    uint8_t _intPin;      // INT pin (255 = none)

    // Legacy-style state for last received frame
    INT8U   m_nExtFlg;     // 0 = standard, 1 = extended
    INT32U  m_nID;         // last received ID
    INT8U   m_nDlc;        // DLC
    INT8U   m_nDta[8];     // data
    INT8U   m_nRtr;        // reserved
    INT8U   m_nfilhit;     // reserved

  public:
    X2CAN(uint8_t csPin, uint8_t intPin = 255);

    // Initialization
    INT8U begin(CAN_SPEED_CFG speedCfg);

    // Masks & filters (Seeed-style)
    INT8U init_Mask(INT8U num, INT8U ext, INT32U ulData);
    INT8U init_Filt(INT8U num, INT8U ext, INT32U ulData);

    // TX (Seeed-style)
    INT8U sendMsgBuf(INT32U id, INT8U ext, INT8U len, const INT8U *buf);

    // RX (Seeed-style)
    INT8U readMsgBuf(INT8U *len, INT8U *buf);
    INT32U getCanId(void) const { return m_nID; }

    // RX availability checks
    INT8U checkReceive(void);  // uses INT
    INT8U checkPoll(void);     // pure software polling

    // Error & mode helpers
    INT8U checkError(void);
    INT8U setModeNormal();
    INT8U setModeLoopback();
    INT8U setModeListenOnly();

    // Convenience helper: ID + EXT + DLC + DATA at once
    INT8U readMsgAll(INT32U *id, INT8U *ext, INT8U *len, INT8U *buf);

    // Raw access to autowp API
    MCP2515::ERROR rawSend(const struct can_frame &frame);
    MCP2515::ERROR rawRead(struct can_frame &frame);

// --- NEW HELPERS FOR DIAGNOSTICS & AUTO-RECOVERY ---

// Reads a raw register directly from the MCP2515 via SPI.
// Useful to detect SPI lockups or MCP2515 "zombie state"
// (returns 0x00 or 0xFF when the chip is not responding properly).
uint8_t readRegister(uint8_t addr) { 
    return mcp.readRegister(addr); 
}

// Returns the raw EFLG (Error Flag) byte from the MCP2515.
// This allows checking exact error conditions such as:
// - TXBO (Bus-Off)
// - TXEP / RXEP (Error Passive)
// - RX0OVR / RX1OVR (Buffer Overflows)
// Perfect for implementing custom CAN error-handling logic.
uint8_t getErrorFlagsRaw() { 
    return mcp.getErrorFlags(); 
}
};

#endif // _X2CAN_H_
