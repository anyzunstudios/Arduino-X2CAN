# X2CAN – Modern MCP2515 CAN wrapper for Arduino

X2CAN is a modern C++ wrapper around the excellent  
[`autowp/arduino-mcp2515`](https://github.com/autowp/arduino-mcp2515) library,  
designed to provide:

- A **familiar API** similar to the classic SeeedStudio `MCP_CAN` library  
- **Improved timing and robustness** from `arduino-mcp2515`  
- **Multi-MCP2515 support** (up to 4 CAN controllers with independent INT pins)  
- Both **interrupt-based** and **pure software polling** receive methods  
- Extra helper functions for modern CAN development (loopback, listen-only, raw frames, etc.)

The goal is simple:

> **Keep the simplicity of the Seeed API, but with a more solid engine underneath.**

---

## Features

- Wrapper over [`autowp/arduino-mcp2515`](https://github.com/autowp/arduino-mcp2515)  
- API very close to Seeed’s `MCP_CAN`:
  - `begin`, `init_Mask`, `init_Filt`, `sendMsgBuf`, `readMsgBuf`, `checkReceive`, `checkError`, `getCanId`, etc.
- Supports **up to 4 MCP2515 modules** with:
  - Dedicated INT pins  
  - Per-instance interrupt slots
- Two receive strategies:
  - `checkReceive()` → interrupt-driven
  - `checkPoll()` → pure software polling (no INT pin required)
- Helper functions built on top of autowp:
  - `readMsgAll()` (ID + EXT + DLC + DATA in one call)
  - `rawSend()` / `rawRead()` (direct `struct can_frame` access)
  - `setModeNormal()`, `setModeLoopback()`, `setModeListenOnly()`

---

## Why this wrapper?

The SeeedStudio `MCP_CAN` library is simple and widely used, but:

- It is relatively old and not actively maintained.
- Its bit-timing configuration is less flexible.
- Multi-controller / multi-bus setups are not straightforward.
- Error handling and special modes are limited.

The [`arduino-mcp2515`](https://github.com/autowp/arduino-mcp2515) library by autowp is:

- Much more **robust** about timing and configuration.
- Explicit about controller **error flags**.
- Supports several **operating modes** (normal, loopback, listen-only).
- Structured around the standard `struct can_frame`.

However, the raw autowp API is less plug-and-play if you are used to Seeed’s `MCP_CAN` style.

**X2CAN exists to bridge that gap:**

- Keep the **old MCP_CAN style** API for quick migration / familiarity.
- Use the **autowp engine** under the hood for better accuracy and multi-bus setups.
- Add useful extras for modern CAN projects (gateways, ECUs, sniffers, etc.).

---

## Dependencies

You need:

1. **Arduino core** for your board (UNO, Mega, ESP32, RP2040 / Pico 2, etc.)  
2. **SPI library** (built-in in most Arduino cores):
   ```cpp
   #include <SPI.h>
   ```
3. **autowp/arduino-mcp2515**

   GitHub: https://github.com/autowp/arduino-mcp2515

   You can install it by:
   - Using the Arduino Library Manager (`arduino-mcp2515`), or  
   - Cloning the repo into your `libraries/` folder.

Make sure your MCP2515 modules are wired using proper SPI and that voltage levels are correct  
(3.3V MCUs like RP2040/ESP32 will typically need level shifting or 3.3V versions of the module).

---

## Library API

### Constructor

```cpp
X2CAN(uint8_t csPin, uint8_t intPin = 255);
```

- `csPin` – Chip Select pin for the MCP2515.
- `intPin` – Interrupt pin from MCP2515 (INT):
  - If `255`, no interrupt is configured → use `checkPoll()`.
  - Otherwise, it will be used by `checkReceive()` and an ISR will be attached.

You can create up to **4 instances**:

```cpp
X2CAN CAN_A(10, 2);  // Slot 0
X2CAN CAN_B(9,  3);  // Slot 1
X2CAN CAN_C(8,  18); // Slot 2
X2CAN CAN_D(7,  19); // Slot 3
```

---

### Initialization

```cpp
enum CAN_SPEED_CFG : uint8_t {
    CAN_SPEED_125KBPS_8MHZ,
    CAN_SPEED_250KBPS_8MHZ,
    CAN_SPEED_500KBPS_8MHZ,
    CAN_SPEED_125KBPS_16MHZ,
    CAN_SPEED_250KBPS_16MHZ,
    CAN_SPEED_500KBPS_16MHZ,
    CAN_SPEED_1000KBPS_16MHZ
};

INT8U begin(CAN_SPEED_CFG speedCfg);
```

- Internally maps to `MCP2515::CAN_SPEED` + `MCP2515::CAN_CLOCK`.
- Returns:
  - `CAN_OK` on success.
  - `CAN_FAILINIT` on failure (bitrate or mode).

---

### Masks and filters (Seeed style)

```cpp
INT8U init_Mask(INT8U num, INT8U ext, INT32U ulData);
INT8U init_Filt(INT8U num, INT8U ext, INT32U ulData);
```

- `num`:
  - `init_Mask`: `0` → MASK0, `1` → MASK1
  - `init_Filt`: `0..5` → RXF0..RXF5
- `ext`:
  - `0` → standard ID
  - non-zero → extended ID
- `ulData`:
  - Mask or filter value (like in the Seeed library).

Returns `CAN_OK` or `CAN_FAILINIT`.

---

### Sending frames

#### High-level (Seeed compatible)

```cpp
INT8U sendMsgBuf(INT32U id, INT8U ext, INT8U len, const INT8U *buf);
```

- `id` – CAN ID (11 or 29 bits).
- `ext` – 0 = standard, non-zero = extended.
- `len` – DLC (0..8), automatically clamped to 8.
- `buf` – pointer to data bytes.

Returns `CAN_OK` or `CAN_FAILTX`.

---

#### Raw frames (autowp style)

```cpp
MCP2515::ERROR rawSend(const struct can_frame &frame);
```

You can build a `struct can_frame` manually if you need full control.

---

### Receiving frames

#### Interrupt-based: `checkReceive() + readMsgBuf()`

```cpp
INT8U checkReceive(void);
INT8U readMsgBuf(INT8U *len, INT8U *buf);
INT32U getCanId(void) const;
```

Typical pattern:

```cpp
if (CAN1.checkReceive() == CAN_MSGAVAIL) {
    INT8U len;
    INT8U buf[8];
    if (CAN1.readMsgBuf(&len, buf) == CAN_OK) {
        INT32U id = CAN1.getCanId();
        // process ID + DATA
    }
}
```

- `checkReceive()`:
  - Uses the interrupt flag for this instance’s slot + MCP2515 RX flags.
  - Returns `CAN_MSGAVAIL` or `CAN_NOMSG`.
- `readMsgBuf()`:
  - Reads the next available message.
  - Fills `len` and `buf`.
  - Stores `id` internally, retrievable via `getCanId()`.

---

#### Software polling: `checkPoll() + readMsgBuf()`

```cpp
INT8U checkPoll(void);
```

- Does **not** use INT, only `mcp.getInterrupts()`.
- Same return codes as `checkReceive()`:
  - `CAN_MSGAVAIL` or `CAN_NOMSG`.

Use it when you don’t want to wire the INT pin.

---

#### Combined helper: `readMsgAll()`

```cpp
INT8U readMsgAll(INT32U *id, INT8U *ext, INT8U *len, INT8U *buf);
```

- Wrapper around `readMsgBuf()` + `getCanId()`.
- Quickly gets ID + EXT + DLC + DATA in one call.

---

#### Raw receive

```cpp
MCP2515::ERROR rawRead(struct can_frame &frame);
```

- When you want to work directly with `struct can_frame`.

---

### Error handling

```cpp
INT8U checkError(void);
```

- Wraps `mcp.getErrorFlags()`.
- Returns:
  - `CAN_OK` if controller error flags are 0.
  - `CAN_CTRLERROR` otherwise.

---

### Mode helpers

```cpp
INT8U setModeNormal();
INT8U setModeLoopback();
INT8U setModeListenOnly();
```

- Map directly to:
  - `mcp.setNormalMode()`
  - `mcp.setLoopbackMode()`
  - `mcp.setListenOnlyMode()`
- Return `CAN_OK` or `CAN_FAIL`.

---

## Examples

The library ships with multiple examples, organized in subfolders for clarity:

- `01_Basic/`
  - `Basic_TX/`
  - `Basic_RX/`
- `02_Filters/`
  - `Filters_TX/`
  - `Filters_RX/`
- `03_MultiBus/`
  - `MultiBus_NodeA/`
  - `MultiBus_NodeB/`
- `04_Polling/`
  - `Polling_TX/`
  - `Polling_RX/`
- `05_RawFrames/`
  - `Raw_TX/`
  - `Raw_RX/`
- `06_3v3_TXS0108E/`
  - `3V3_TXS0108E_Demo/`

Each folder contains a `.ino` sketch demonstrating the transmitter and receiver side  
of a specific concept (basic usage, filters, multi-bus setup, polling, raw frames, 3.3V level shifting, etc.).

---

## 06 – Using X2CAN on 3.3V MCUs with TXS0108E (level shifter)

Many boards (ESP32, RP2040 / Pico 2, etc.) are 3.3V only, while most MCP2515 modules are 5V.
A common solution is to use a **TXS0108E** level shifter between the MCU and the MCP2515.

However, the TXS0108E is not a perfect high-speed SPI driver, and very high SPI clock rates
can cause unstable CAN communication (random errors, lost frames, etc.).

This example shows:

- How to **configure SPI at a lower speed** when using TXS0108E.
- How to control this with a simple `bool useLowSpiSpeed`.
- How to use X2CAN normally on a 3.3V MCU with a 5V MCP2515 through a TXS0108E.

### Wiring overview (example)

- MCU (3.3V) → TXS0108E A-side  
- MCP2515 (5V) → TXS0108E B-side  

Typical connections:

- `MCU SCK`  ↔ `TXS0108E A?` → `TXS0108E B?` ↔ `MCP2515 SCK`
- `MCU MOSI` ↔ `TXS0108E A?` → `TXS0108E B?` ↔ `MCP2515 SI`
- `MCU MISO` ↔ `TXS0108E A?` → `TXS0108E B?` ↔ `MCP2515 SO`
- `MCU CS`   ↔ `TXS0108E A?` → `TXS0108E B?` ↔ `MCP2515 CS`
- `MCP2515 INT` → `TXS0108E B?` → `TXS0108E A?` → `MCU INT pin`

> Make sure:
> - `VCCA` of TXS0108E is connected to MCU 3.3V.
> - `VCCB` of TXS0108E is connected to 5V (MCP2515 side).
> - `OE` is pulled HIGH (enabled).

---

## License

Choose the license that best fits your project (MIT, Apache-2.0, GPL, etc.).  
For open source CAN tooling, **MIT** is usually a good fit.
