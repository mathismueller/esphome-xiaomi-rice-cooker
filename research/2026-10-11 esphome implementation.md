# ESPHome Implementation & Bench Verification

**Date**: 2026-10-11 / 2026-10-12  
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
     - **Mode**: `waiting`, `running`, `autokeepwarm`, `error`
     - **Menu**: `0001` (Standard Rice), `0002` (Quick Rice), `0003` (Porridge), `0004` (Keep Warm)
     - **Stage**: `Idle`, `Preheating`, `Water Absorption`, `Boiling`, `Gelatinization`, `Braising`, `Finished`
     - **Temperature**: tracked live on bench (`21°C`–`22°C`)
     - **Remaining Duration**: countdown minutes
     - **Total Duration**: program minutes
     - **Delay Time**: scheduled appointment minutes
     - **Settings**: auto-keep-warm, lid open alarms
     - **Versions**: Hardware `v3`, MCU `v26`

4. **Network & Web Server**:
   - Standalone hotspot fallback AP and Web Server on port 80 allow standalone monitoring and wireless OTA updates via browser.
   - Home Assistant native API active with encrypted credentials via `secrets.yaml`.

5. **Physical Wi-Fi Reset Button Handling**:
   - The host MCU sends `restore\r\n` or `factory\r\n` when the physical lid Wi-Fi reset button (`KEY` pin) is held.
   - The component updates `last_button` and `last_event` to `"wifi_reset"`, informs the MCU via `down MIIO_net_change unprov\r`, and triggers `App.safe_reboot()`.

6. **Full Protocol Command Suite & Command Queue**:
   - Implemented an asynchronous FIFO queue for MCU command dispatching to seamlessly match the MCU's 250ms `get_down` query rate without dropping or colliding requests.
   - Built-in cooking profiles (242 hex characters) for Quick Rice, Standard Rice, Porridge, Keep Warm, Cooking/Steam, and Cake.
   - Bidirectional `Cooker Menu Select` in Home Assistant: changes the active program on the cooker lid display and syncs in real-time when the physical menu button is pressed.
   - Dedicated buttons: Start presets, Start selected menu, Cancel / Stop, Mute warnings, Acknowledge warnings, Refresh status.
   - Custom ESPHome actions: `chunmi_cooker.start`, `chunmi_cooker.set_menu`, `chunmi_cooker.cancel`, `chunmi_cooker.send_raw`.

7. **Live Hardware Verification with AC Mains & Induction Power (2026-10-12)**:
   - **Inner Pot Detection**: Verified live. Removing the pot triggers `event nopan` and updates the `Inner Pot Present` binary sensor; replacing the pot restores `true`.
   - **Menu Picking**: Verified live. Selecting recipes in Home Assistant updates the TM1620 display, and hardware button cycling updates Home Assistant instantly via `menuchange` and `props button_pressed`.
   - **Starting Cooking Cycles**: Verified live. Triggering start dispatches `down set_start "<profile>"\r`, engaging the induction electronics and starting the countdown.
   - **Stopping Cooking Cycles**: Verified live. Triggering cancel dispatches `down set_func "end02"\r`, immediately halting heating and returning the cooker to `waiting` idle mode.

---

## 2. Terminal Monitoring Tip

When connecting to `UART2` TX (`GPIO2`) using `picocom`, use `--imap lfcrlf` to prevent stepped line wrapping:
```sh
picocom --baud 115200 --imap lfcrlf /dev/cu.usbserial-xxx
```
