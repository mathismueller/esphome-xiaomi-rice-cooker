# Firmware Analysis & Flash Dump Reverse Engineering

This document documents the disassembly and analysis of the 2 MByte flash memory dump (`dumps/stock_backup_2MB.bin`) extracted from the **Xiaomi MHCW03P** module on the Xiaomi IH Rice Cooker (`IHFB01CM`).

---

## 1. Flash Memory Geometry & Partition Layout

- **Total Capacity**: 2,097,152 Bytes (2.0 MByte / 16 Mbit).
- **SoC**: Espressif **ESP8285N08** with internal embedded SPI flash.
- **Flash Hardware ID**: Manufacturer `0xC8` (GigaDevice), Device `0x4015` (GD25Q16 compatible).
- **Flash Mode**: QIO @ 40 MHz.
- **Partition Mapping**: 16 Mbit dual-OTA slot map (`1024KB + 1024KB`).

```
+-------------------------------------------------------------------------+
|                  2MB SPI FLASH MEMORY MAP (ESP8285N08)                  |
+-------------------------------------------------------------------------+
| Offset Range          | Size     | Content                              |
+-----------------------+----------+--------------------------------------+
| 0x000000 - 0x000FFF   | 4 KB     | Espressif 2nd-stage Bootloader v1.4  |
| 0x001000 - 0x0FEFFF   | ~1016 KB | OTA Slot 1: Active `user1.bin`       |
| 0x0FF000 - 0x0FFFFF   | 4 KB     | System Parameters / Boot Sector      |
| 0x100000 - 0x100FFF   | 4 KB     | Reserved / OTA Sector                |
| 0x101000 - 0x181FFF   | ~516 KB  | OTA Slot 2: `user2.bin` / Recovery   |
| 0x182000 - 0x18FF5F   | ~56 KB   | NXP LPC11xx Host MCU Firmware Binary |
| 0x190000 - 0x1FBFFF   | ~432 KB  | Unallocated / 0xFF Padding           |
| 0x1FC000 - 0x1FCFFF   | 4 KB     | RF Calibration (`esp_init_data_default.bin`, 128 bytes) |
| 0x1FD000 - 0x1FDFFF   | 4 KB     | System Parameters Slot 1 (Wi-Fi config, PSM) |
| 0x1FE000 - 0x1FEFFF   | 4 KB     | System Parameters Slot 2 (Backup config, PSM) |
| 0x1FF000 - 0x1FFFFF   | 4 KB     | System RF Calibration & MAC Flags    |
+-------------------------------------------------------------------------+
```

---

## 2. Bootloader & `user1.bin` (ESP8285)

### 2.1 Bootloader (Offset `0x000000`)
- Validated with `esptool image-info`:
  - Entry point: `0x401002d8`
  - Mode: QIO @ 40 MHz
  - Segments: 3 (IRAM: `0x40100000`, DRAM: `0x3ffe8000`, `0x3ffe8308`)

### 2.2 Active User Firmware (`user1.bin` @ `0x001000`)
- Image Format: Version 2 (ESP8266 OTA image header `0xEA`).
- Total payload: 327,336 bytes.
- Operating System: Espressif ESP FreeRTOS SDK v1.4.0.
- Application: Xiaomi miIO Device Agent v1.2.8.
- Serial Log Banner extracted from strings:
  ```text
  BUILD TIME: Sep 13 2016, 20:47:05
  firmware: 1.2.8
  ESP SDK VER: 1.4.0(30e0bd0)
  model: chunmi.cooker.normal2
  ```

---

## 3. Discovery: Host MCU Firmware in Flash (`0x182000`)

A significant discovery from reverse-engineering the flash dump was identifying a complete **ARM Cortex-M0 firmware binary** stored inside the upper flash region (`0x182000` to `0x18FF5F`, length 57,184 bytes).

### 3.1 Architecture Identification
Analysis of machine instructions and peripheral addresses confirms the binary targets the **NXP LPC1100 series MCU (IC1)**:
- **Instruction Set**: ARMv6-M (Thumb / Thumb-2 subset).
- **RAM Base**: `0x10000000` (NXP LPC on-chip SRAM base address).
- **Peripheral Base**: `0x40000000` (NXP LPC APB peripheral space):
  - `0x4000C000`: LPC UART0 (configured for 115200 baud).
  - `0x40004000`: LPC I2C controller.
  - `0x40014000`: 16-bit Timer 0.
  - `0x4001C000`: 32-bit Timer 0.
  - `0x40044000`: LPC SYSCON (System Control Block).
  - `0x40048000`: LPC IOCON (Pin Function Selection).

### 3.2 Contents of the MCU Image
The MCU binary contains:
1. The full state machine for cooking curves, PWM frequency tables for induction heating coils, and ADC lookup tables for the NTC thermistors.
2. The ASCII command parser for the `down ` protocol.
3. The JSON string generator for `{"result":[...]}`.
4. The error detection routines that broadcast `event *`.

### 3.3 Inter-MCU OTA Update Subsystem
The ESP8285 firmware includes routines to flash the host MCU over UART:
- Strings located in `user1.bin` at `0x055A64`:
  - `[I] OTA app fw and mcu fw both.`
  - `[I] OTA mcu fw begin.`
  - `[I] No valid mcu fw found. Skip...`
  - `down update_fw`
- When a cloud firmware update includes new appliance code, the ESP8285 stores the MCU binary at `0x182000`, sends `down update_fw\r`, and streams the image to the LPC11xx MCU's internal flash.

---

## 5. RF Calibration & System Parameter Preservation

### 5.1 Factory RF Initialization Data (`0x1FC000`)
At offset `0x1FC000` (the 4th sector from the end of the 2MB flash), Espressif stores `esp_init_data_default.bin` (version `0x05`, 128 bytes). This data tunes:
- Crystal oscillator frequency load capacitance
- 2.4 GHz PA (Power Amplifier) transmit power tables
- LNA (Low Noise Amplifier) gain and receiver sensitivity
- Target regulatory domain power limits

### 5.2 Preservation During Flashing
- Standard ESPHome flashing writes `.esphome/.../firmware.bin` from `0x000000` to `~0x062000` (~400 KB).
- **Because the binary is under 1MB, it never writes to or erases sector `0x1FC000`**. The factory RF calibration remains untouched.
- If the flash is ever completely erased (`esptool erase-flash`), the extracted RF calibration file stored at [`dumps/esp_init_data_2MB.bin`](../dumps/esp_init_data_2MB.bin) must be restored:
  ```sh
  esptool write-flash 0x1fc000 dumps/esp_init_data_2MB.bin
  ```

---

## 4. Hardware UART Swapping (`system_uart_swap()`)

On the ESP8266/ESP8285:
- Standard UART0 uses `GPIO1` (TX) and `GPIO3` (RX).
- When `system_uart_swap()` is invoked in firmware:
  - UART0 TX switches to `GPIO15`.
  - UART0 RX switches to `GPIO13`.
- UART1 is a transmit-only peripheral on `GPIO2` (used for FreeRTOS logging).
- The PCB routes `GPIO13` and `GPIO15` directly to the `UART1` test header and the LPC11xx MCU, ensuring the primary inter-MCU serial bus operates without interference from boot ROM spew.
