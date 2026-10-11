# Hardware & Pinout Reference: Xiaomi IH Rice Cooker (IHFB01CM)

This document details the hardware architecture, integrated circuits, pin assignments, headers, and bench power connections for the **Xiaomi Mi Induction Heating Rice Cooker** (Model `IHFB01CM`, miIO identifier `chunmi.cooker.normal2`).

---

## 1. System Architecture

The cooker utilizes a distributed dual-controller architecture split across the lid logic board and the base induction inverter:

```
+---------------------------------------------------------------------------------+
|                               LID LOGIC BOARD (CMIHFKACO2)                      |
|                                                                                 |
|   +-----------------------+                    +----------------------------+   |
|   |  Xiaomi MHCW03P       |                    |  NXP LPC1100 Series MCU    |   |
|   |  (ESP8285N08 SoC)     |  UART 115200 8N1   |  (IC1, 48-pin LQFP)        |   |
|   |                       |<------------------>|                            |   |
|   |  - Wi-Fi Bridge       |  GPIO13 (RX)       |  - Master Appliance Brain  |   |
|   |  - 2MB Embedded Flash |  GPIO15 (TX)       |  - Safety State Machine    |   |
|   +-----------+-----------+                    +-------+------------+-------+   |
|               |                                        |            |           |
|          Flash Port                                    |            |           |
|          (GPIO1/3)                                     v            v           |
|                                                  Titan Micro    Capacitive      |
|                                                     TM1620      Touch Keys      |
|                                                  (IC4 Display)  & Pushbuttons   |
+--------------------------------------------------------|------------------------+
                                                         | 11-Wire Harness
                                                         v
+---------------------------------------------------------------------------------+
|                             BASE / CHASSIS (MAINS VOLTAGE)                      |
|                                                                                 |
|   - AC Mains Power Supply (produces 5V & logic rails)                           |
|   - Resonant Induction Inverter (IGBT stage & drive coils)                      |
|   - Thermal Sensors (IGBT NTC, Pan Bottom NTC, Lid NTC)                         |
|   - Piezo Buzzer & High-Voltage Safety Relays                                   |
+---------------------------------------------------------------------------------+
```

### Safety & Control Isolation
- **The IoT processor (ESP8285) does not control any relays, IGBTs, or heaters directly.**
- All high-voltage switching, thermal protection limits, pot detection, and hardware buttons are managed autonomously by the **NXP LPC11xx MCU (IC1)**.
- If the ESP8285 crashes, resets, or loses Wi-Fi connection, the cooker continues or safely terminates according to the LPC MCU's internal cooking profiles and watchdog routines.

---

## 2. Integrated Circuit Map

| Component | Package / Markings | Function | Role |
| :--- | :--- | :--- | :--- |
| **MHCW03P** | Custom Module (Shielded) | Espressif ESP8285N08 (Xtensa LX106 @ 160MHz) | Wi-Fi telemetry bridge, 2MB embedded QIO SPI flash |
| **IC1** | LQFP-48 (NXP Logo, 12.000MHz Crystal `Y1`) | NXP LPC1100 Series ARM Cortex-M0 MCU | Appliance main controller; drives logic, inverter signals, UI |
| **IC4** | SOP-20 (`TM1620`) | Titan Micro TM1620 | 7-segment / LED matrix driver (clock, data, strobe) |
| **U2** | SOP-16 (`ULN2003A`) | Darlington Transistor Array | High-current driver for buzzers and indicator lines |
| **FLASH** | SOIC-8 (Unpopulated footprint) | SPI Flash Header footprint | Optional external SPI flash pad layout on the lid board |

---

### 3. PCB Headers & Pin Assignments

```
               LID PCB (CMIHFKACO2) - TOP RIGHT VIEW

       [ UART1 ]          [ UART2 ]
     (MCU Comm Link)    (Debug / Aux)
     +-------------+    +-------------+
     | 1: VCC (3V3)|    | 1: VCC (3V3)|
     | 2: RX(GPIO13|<---| 2: RX (GPIO4)|
     | 3: TX(GPIO15|--->| 3: TX (GPIO2)| (Log spew @ 115200 / 74880)
     | 4: NC / +   |    | 4: NC / +   |
     | 5: GND      |    | 5: GND      |
     +-------------+    +-------------+
             |                 |
             +--------+--------+
                      |
                     [JW] (Jumper Pads: GND <-> GPIO0)
                      |
       +--------------v----------------+
       |         MHCW03P Module        |
       |  [U0TXD: GPIO1] [U0RXD: GPIO3]| (Through-holes for flashing)
       +-------------------------------+
```

### 3.1 Complete ESP8285 GPIO Map & Hardware Allocation

Inspection of IO Mux registers in the stock firmware image confirms that the ESP8285 operates purely as a serial communication bridge. All user-facing LEDs, buttons, and sensors are routed to the LPC11xx MCU.

| GPIO | Pin # | Hardware Net / Pad | Direction | Function in Cooker |
| :---: | :---: | :--- | :---: | :--- |
| **GPIO0** | 15 | Jumper Pad `JW` | Input (Pull-Up) | Short to GND at boot to enter ROM UART download mode. |
| **GPIO1** | 25 | Module Through-Hole `U0TXD` | Output | Primary UART0 TX used for factory flashing. |
| **GPIO2** | 14 | Header `UART2` TX | Output | Hardware UART1 TX console (boot ROM spew @ 74880, logger @ 115200). Must be HIGH at boot. |
| **GPIO3** | 26 | Module Through-Hole `U0RXD` | Input | Primary UART0 RX used for factory flashing. |
| **GPIO4** | 16 | Header `UART2` RX | Input | Auxiliary / unconfigured on stock firmware. |
| **GPIO5** | 24 | Unconnected / Internal | — | Unused on module. |
| **GPIO6–11** | — | Internal SPI Flash | I/O | Embedded 2MB QIO Flash (ESP8285 embedded memory). |
| **GPIO12** | 10 | Unconnected / Internal | — | Unused on module. |
| **GPIO13** | 12 | Header `UART1` RX | Input | Inter-MCU link RX (Swapped UART0 RX from LPC11xx TX). |
| **GPIO14** | 9 | Unconnected / Internal | — | Unused on module. |
| **GPIO15** | 13 | Header `UART1` TX | Output | Inter-MCU link TX (Swapped UART0 TX to LPC11xx RX). Pulled LOW at boot for SPI boot mode. |
| **GPIO16** | 8 | RTC / XPD_DCDC | — | Unused (deep sleep not supported). |

### 3.2 Wi-Fi Status LED Architecture (`B_LED` & `O_LED`)

The cooker possesses dual-color Wi-Fi status LEDs:
- **Blue LED (`B_LED`)**: Indicates Wi-Fi connected / local network operational.
- **Orange LED (`O_LED`)**: Indicates Wi-Fi unprovisioned / offline / connecting.

**Crucial Finding**: Neither LED is driven directly by ESP8285 GPIO pins. Instead, both LEDs are routed across the 11-pin chassis harness to the **NXP LPC11xx host MCU**.

The ESP8285 drives the LED state by issuing protocol notifications over the UART link:
- `down MIIO_net_change local\r`: Triggers solid **Blue LED** (Wi-Fi online).
- `down MIIO_net_change offline\r`: Triggers blinking or solid **Orange LED** (network disconnected).
- `down MIIO_net_change unprov\r`: Triggers slow-pulsing **Orange LED** (AP provisioning mode).
- `down MIIO_net_change updating\r`: Triggers rapid alternate blinking (firmware update in progress).

In ESPHome, these states are automatically coordinated using the `wifi.on_connect`, `wifi.on_disconnect`, and `ota.on_begin` automation triggers.

### 3.4 Bench Power Header (`POWER`)
Located at the bottom right corner of the lid board.
- `5V`: Injects 5.0V DC to power the onboard buck/LDO regulators.
- `GND`: Ground return.
- Bench power consumption is typically **80 mA to 250 mA at 5.0 V DC** with Wi-Fi active.

### 3.5 Host MCU Headers (LPC11xx Debug & Boot)
- **`JTAG` Header** (Bottom Right):
  - Pins: `GND`, `CLK` (SWCLK), `DIO` (SWDIO), `RST` (nRESET), `VCC`.
  - Used for ARM Serial Wire Debug (SWD) connections to IC1.
- **`BOOT` Header** (Bottom Center):
  - Pins: `BOOT`, `GND`, `TX`, `RX`, `5V`, `VCC`.
  - Pulling `BOOT` (`PIO0_1`) low during reset triggers the LPC11xx built-in ROM ISP bootloader over UART.

### 3.6 11-Wire Inter-Board Harness
Connects the lid board `CMIHFKACO2` to the lower chassis:
- **Top Row**: `GND`, `5V`, `SCL`, `SDA`, `BUZ1`, `BUZ2`, `VDD`
- **Bottom Row**: `B_LED`, `O_LED`, `KEY`, `GND`

---

## 4. Bench Work Safety Instructions

> [!WARNING]
> **Mains Voltage Hazard**:
> Never connect the AC mains plug to a wall outlet while the cooker casing is opened or while connecting bench instrumentation.

1. Keep the mains power cord completely disconnected from the cooker base.
2. Power the lid logic board exclusively through the `POWER` header pins (`5V` and `GND`) using a current-limited DC bench power supply set to **5.0 V DC** and **500 mA current limit**.
3. Use a 3.3V logic level USB-to-UART adapter. Never apply 5V logic signals directly to the ESP8285 or LPC11xx UART pins.
4. Establish common ground between the bench power supply, USB-UART adapter, and the board ground before connecting data lines.
