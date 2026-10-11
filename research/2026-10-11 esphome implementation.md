# ESPHome Implementation & Bench Verification

**Date**: 2026-10-11  
**Target**: Xiaomi IH Rice Cooker (`IHFB01CM` / `chunmi.cooker.normal2`)  
**Firmware**: ESPHome 2026.9.1 with custom `chunmi_cooker` component  

---

## 1. Summary of Milestones Achieved

1. **Hardware & Pinout Verified**:
   - The ESP8285 communicates with the NXP LPC11xx MCU via swapped UART0 (`GPIO13` RX, `GPIO15` TX) at **115200 8N1**.
   - Debug logging was redirected to hardware `UART1` (`GPIO2`), which physically routes to the `TX` pin of the `UART2` header on the lid PCB.
   - Bootloader flashing verified via the module through-holes (`GPIO1`/`GPIO3`) at **230400 baud** with jumper `JW` bridged.

2. **Protocol Discovery: `get_down` Polling Loop**:
   - Live hardware captures revealed that the LPC11xx MCU initiates queries every ~250 ms with the string `get_down\r`.
   - ESPHome responds with `down none\r` when idle, or dispatches queued commands (such as `down get_prop "all"\r` or `down MIIO_net_change local\r`).
   - The MCU confirms the handshake with:
     ```text
     model chunmi.cooker.normal2
     mcu_version 0026
     ```

3. **Status Telemetry Parsing Verified**:
   - The MCU returns telemetry in response to `down get_prop "all"` formatted as:
     ```text
     result "waiting","0001","null","22","60","-1","60","0607","050415","0003001a","0103","ffff1100ffff011effff001e00001616"
     ```
   - Parsed by ArduinoJson in C++ to extract:
     - **Mode**: `waiting`
     - **Menu**: `0001` (Standard Rice)
     - **Stage**: `Idle`
     - **Temperature**: tracked live on bench from `22°C` to `21°C`
     - **Remaining Duration**: `60` minutes
     - **Inner Pot**: `true` (present)

4. **Network & Web Server**:
   - Standalone hotspot fallback AP (`Xiaomi-Cooker-Bench`) and Web Server on port 80 allow standalone monitoring and wireless OTA updates via browser.
   - Home Assistant native API added, with credentials managed via `secrets.yaml`.

---

## 2. Terminal Monitoring Tip

When connecting to `UART2` TX (`GPIO2`) using `picocom`, use `--imap lfcrlf` to prevent stepped line wrapping:
```sh
picocom --baud 115200 --imap lfcrlf /dev/cu.usbserial-xxx
```
