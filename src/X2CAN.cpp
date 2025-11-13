#include "X2CAN.h"

// -----------------------------------------------------------------------------
// Global management for up to 4 instances with independent INT flags
// -----------------------------------------------------------------------------

static volatile bool s_canIntFlag[4] = { false, false, false, false };
static uint8_t       s_canIntPin[4]  = { 255,   255,   255,   255   };
static uint8_t       s_x2can_instanceCount = 0;

static void _canIntISR0() { s_canIntFlag[0] = true; }
static void _canIntISR1() { s_canIntFlag[1] = true; }
static void _canIntISR2() { s_canIntFlag[2] = true; }
static void _canIntISR3() { s_canIntFlag[3] = true; }

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
X2CAN::X2CAN(uint8_t csPin, uint8_t intPin)
    : mcp(csPin), _intPin(intPin)
{
    _slotIndex = (s_x2can_instanceCount < 4) ? s_x2can_instanceCount : 3;
    if (s_x2can_instanceCount < 4) {
        s_x2can_instanceCount++;
    }

    s_canIntPin[_slotIndex] = intPin;

    m_nExtFlg = 0;
    m_nID     = 0;
    m_nDlc    = 0;
    m_nRtr    = 0;
    m_nfilhit = 0;
    for (uint8_t i = 0; i < 8; i++) {
        m_nDta[i] = 0;
    }
}

// -----------------------------------------------------------------------------
// begin() – configuration wrapper around autowp timing + mode
// -----------------------------------------------------------------------------
INT8U X2CAN::begin(CAN_SPEED_CFG speedCfg)
{
    MCP2515::CAN_SPEED spd;
    MCP2515::CAN_CLOCK clk;

    switch (speedCfg) {
        case CAN_SPEED_125KBPS_8MHZ:
            spd = MCP2515::CAN_125KBPS; clk = MCP2515::MCP_8MHZ;  break;
        case CAN_SPEED_250KBPS_8MHZ:
            spd = MCP2515::CAN_250KBPS; clk = MCP2515::MCP_8MHZ;  break;
        case CAN_SPEED_500KBPS_8MHZ:
            spd = MCP2515::CAN_500KBPS; clk = MCP2515::MCP_8MHZ;  break;

        case CAN_SPEED_125KBPS_16MHZ:
            spd = MCP2515::CAN_125KBPS; clk = MCP2515::MCP_16MHZ; break;
        case CAN_SPEED_250KBPS_16MHZ:
            spd = MCP2515::CAN_250KBPS; clk = MCP2515::MCP_16MHZ; break;
        case CAN_SPEED_500KBPS_16MHZ:
            spd = MCP2515::CAN_500KBPS; clk = MCP2515::MCP_16MHZ; break;
        case CAN_SPEED_1000KBPS_16MHZ:
            spd = MCP2515::CAN_1000KBPS; clk = MCP2515::MCP_16MHZ; break;

        default:
            return CAN_FAILINIT;
    }

    mcp.reset();

    if (mcp.setBitrate(spd, clk) != MCP2515::ERROR_OK) {
        return CAN_FAILINIT;
    }

    if (mcp.setNormalMode() != MCP2515::ERROR_OK) {
        return CAN_FAILINIT;
    }

    // Configure INT if provided
    if (_intPin != 255) {
        pinMode(_intPin, INPUT);
        switch (_slotIndex) {
            case 0:
                attachInterrupt(digitalPinToInterrupt(_intPin), _canIntISR0, FALLING);
                break;
            case 1:
                attachInterrupt(digitalPinToInterrupt(_intPin), _canIntISR1, FALLING);
                break;
            case 2:
                attachInterrupt(digitalPinToInterrupt(_intPin), _canIntISR2, FALLING);
                break;
            case 3:
                attachInterrupt(digitalPinToInterrupt(_intPin), _canIntISR3, FALLING);
                break;
        }
    }

    return CAN_OK;
}

// -----------------------------------------------------------------------------
// init_Mask() – wrapper around setFilterMask()
// -----------------------------------------------------------------------------
INT8U X2CAN::init_Mask(INT8U num, INT8U ext, INT32U ulData)
{
    MCP2515::MASK maskEnum;

    if (num == 0)      maskEnum = MCP2515::MASK0;
    else if (num == 1) maskEnum = MCP2515::MASK1;
    else               return CAN_FAILINIT;

    bool extended = (ext != 0);

    MCP2515::ERROR err = mcp.setFilterMask(maskEnum, extended, ulData);
    return (err == MCP2515::ERROR_OK) ? CAN_OK : CAN_FAILINIT;
}

// -----------------------------------------------------------------------------
// init_Filt() – wrapper around setFilter()
// -----------------------------------------------------------------------------
INT8U X2CAN::init_Filt(INT8U num, INT8U ext, INT32U ulData)
{
    MCP2515::RXF filtEnum;

    switch (num) {
        case 0: filtEnum = MCP2515::RXF0; break;
        case 1: filtEnum = MCP2515::RXF1; break;
        case 2: filtEnum = MCP2515::RXF2; break;
        case 3: filtEnum = MCP2515::RXF3; break;
        case 4: filtEnum = MCP2515::RXF4; break;
        case 5: filtEnum = MCP2515::RXF5; break;
        default:
            return CAN_FAILINIT;
    }

    bool extended = (ext != 0);

    MCP2515::ERROR err = mcp.setFilter(filtEnum, extended, ulData);
    return (err == MCP2515::ERROR_OK) ? CAN_OK : CAN_FAILINIT;
}

// -----------------------------------------------------------------------------
// sendMsgBuf() – Seeed-style TX, autowp engine
// -----------------------------------------------------------------------------
INT8U X2CAN::sendMsgBuf(INT32U id, INT8U ext, INT8U len, const INT8U *buf)
{
    struct can_frame frame;

    frame.can_id = id;
    if (ext) {
        frame.can_id |= CAN_EFF_FLAG;
    }

    if (len > 8) len = 8;
    frame.can_dlc = len;

    for (INT8U i = 0; i < len; i++) {
        frame.data[i] = buf[i];
    }

    MCP2515::ERROR err = mcp.sendMessage(&frame);
    return (err == MCP2515::ERROR_OK) ? CAN_OK : CAN_FAILTX;
}

// -----------------------------------------------------------------------------
// readMsgBuf() – Seeed-style RX, internally updates m_nID, etc.
// -----------------------------------------------------------------------------
INT8U X2CAN::readMsgBuf(INT8U *len, INT8U *buf)
{
    struct can_frame frame;
    MCP2515::ERROR err = mcp.readMessage(&frame);

    if (err == MCP2515::ERROR_NOMSG) {
        return CAN_NOMSG;
    }
    if (err != MCP2515::ERROR_OK) {
        return CAN_FAIL;
    }

    bool extended = (frame.can_id & CAN_EFF_FLAG);
    m_nExtFlg = extended ? 1 : 0;
    if (extended) {
        m_nID = frame.can_id & CAN_EFF_MASK;
    } else {
        m_nID = frame.can_id & CAN_SFF_MASK;
    }

    m_nDlc = frame.can_dlc;
    if (m_nDlc > 8) m_nDlc = 8;

    for (INT8U i = 0; i < m_nDlc; i++) {
        m_nDta[i] = frame.data[i];
        if (buf) {
            buf[i] = frame.data[i];
        }
    }

    if (len) {
        *len = m_nDlc;
    }

    s_canIntFlag[_slotIndex] = false;

    return CAN_OK;
}

// -----------------------------------------------------------------------------
// checkReceive() – interrupt-based RX availability
// -----------------------------------------------------------------------------
INT8U X2CAN::checkReceive(void)
{
    if (_intPin == 255) {
        return CAN_NOMSG;
    }

    if (!s_canIntFlag[_slotIndex]) {
        return CAN_NOMSG;
    }

    uint8_t irq = mcp.getInterrupts();

    if (irq & (MCP2515::CANINTF_RX0IF | MCP2515::CANINTF_RX1IF)) {
        return CAN_MSGAVAIL;
    }

    s_canIntFlag[_slotIndex] = false;
    return CAN_NOMSG;
}

// -----------------------------------------------------------------------------
// checkPoll() – software polling, no INT required
// -----------------------------------------------------------------------------
INT8U X2CAN::checkPoll(void)
{
    uint8_t irq = mcp.getInterrupts();

    if (irq & (MCP2515::CANINTF_RX0IF | MCP2515::CANINTF_RX1IF)) {
        return CAN_MSGAVAIL;
    }

    return CAN_NOMSG;
}

// -----------------------------------------------------------------------------
// checkError() – wraps getErrorFlags()
// -----------------------------------------------------------------------------
INT8U X2CAN::checkError(void)
{
    uint8_t flags = mcp.getErrorFlags();
    return (flags == 0) ? CAN_OK : CAN_CTRLERROR;
}

// -----------------------------------------------------------------------------
// readMsgAll() – convenience helper
// -----------------------------------------------------------------------------
INT8U X2CAN::readMsgAll(INT32U *id, INT8U *ext, INT8U *len, INT8U *buf)
{
    INT8U ret = readMsgBuf(len, buf);
    if (ret != CAN_OK) {
        return ret;
    }

    if (id)  *id  = m_nID;
    if (ext) *ext = m_nExtFlg;

    return CAN_OK;
}

// -----------------------------------------------------------------------------
// rawSend / rawRead – direct autowp access
// -----------------------------------------------------------------------------
MCP2515::ERROR X2CAN::rawSend(const struct can_frame &frame)
{
    return mcp.sendMessage(&frame);
}

MCP2515::ERROR X2CAN::rawRead(struct can_frame &frame)
{
    return mcp.readMessage(&frame);
}

// -----------------------------------------------------------------------------
// Mode helpers
// -----------------------------------------------------------------------------
INT8U X2CAN::setModeNormal()
{
    return (mcp.setNormalMode() == MCP2515::ERROR_OK) ? CAN_OK : CAN_FAIL;
}

INT8U X2CAN::setModeLoopback()
{
    return (mcp.setLoopbackMode() == MCP2515::ERROR_OK) ? CAN_OK : CAN_FAIL;
}

INT8U X2CAN::setModeListenOnly()
{
    return (mcp.setListenOnlyMode() == MCP2515::ERROR_OK) ? CAN_OK : CAN_FAIL;
}
