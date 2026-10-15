# 2026-10-15: Reverse Engineering Temperature Telemetry & Lid Warning

## Context
During cooking sessions in Home Assistant (e.g. Porridge / `ZHUZHOU`), remaining countdown time and stage transitions updated accurately, but the temperature sensor remained completely flat at ambient (~16°C). The moment the cooking cycle was cancelled or ended, the temperature immediately jumped to 80°C and followed an expected cooling curve. In addition, opening the lid during cooking to stir did not trip the `lid_open_warning` entity in Home Assistant.

To uncover why live temperature was missing during active heating and understand the lid warning behavior, the NXP LPC11xx ARM Cortex-M0 host MCU binary was extracted and disassembled directly from the stock full flash dump (`dumps/stock_esp8285_full_flash_2mb.bin` at offset `0x182000`).

---

## 1. Status Polling Behavior (`down get_prop "all"`)

In the MCU disassembly at `0x4ac8` (the uplink JSON array generator):
- **When idle/waiting (`is_running == 0`)**: Field `result[3]` is formatted with `"%d"` reading directly from RAM address `0x100003c8` (the bottom center pan NTC thermistor in °C).
- **When cooking (`is_running == 1`)**: Field `result[3]` changes completely: it is formatted with `"%02x%02x%02x%02x%02x%02x"` as a 12-character hex encoded string containing execution timestamps (`0x1000048b`–`0x1000048e` hours/mins/secs/ticks).

Because [`chunmi_cooker.cpp`](../components/chunmi_cooker/chunmi_cooker.cpp) checks `t_str.length() <= 3 && isdigit(t_str[0])`, `result[3]` is intentionally not parsed as a temperature during cooking. Live cooking temperature is instead intended to be read via `down get_temp_history`.

---

## 2. Reverse Engineering `down get_temp_history`

### 2.1 MCU Command Handling & Assembly
Disassembling the command dispatcher at `0x5096` and handler at `0x4610`–`0x46a0` revealed the exact firmware logic:
```text
0x4610: LDR R0, =0x10000190
0x4612: LDRB R0, [R0, #11]
0x4614: CMP R0, #1
0x4616: BEQ is_running
...
0x46a6: ADR R2, =b'result "0"'      ; If not cooking, returns: result "0"\r\n
...
0x4628: LDR R0, =0x100003fc        ; History length counter
0x462a: LDRH R7, [R0]
0x462c: CMP R7, #0
0x462e: BEQ return_empty_quotes    ; Returns: result ""\r\n
0x4632: ADR R2, =b'result "'       ; Starts with: result "
; Loop through history samples:
0x4658: ADDS R0, #0x42             ; History buffer at 0x100001d2
0x465a: LDRB R2, [R0, R6]          ; Load sample byte
0x4660: BL sprintf                 ; Format as %02x
...
0x4686: ADR R2, =b'\r\n'           ; Appends \r\n DIRECTLY without a closing quote!
0x468c: BL sprintf
0x4696: BL uart_write
```

### 2.2 The Root Cause: Missing Closing Quote
The host MCU emits:
```text
result "<hex_bytes>\r\n
```
Notice that **no closing quote is ever appended**!

In earlier code:
```cpp
size_t first = line.find('"');
size_t last = line.rfind('"');
if (first != std::string::npos && last != std::string::npos && last > first) { ... }
```
Because the line contained only one quote (at index 7), `first == last == 7`, meaning `last > first` was **always false**. The entire temperature history response was silently discarded on every single poll cycle!

### 2.3 Delimiter `0xAA`
The MCU inserts `0xAA` (170 in decimal) into the hex stream whenever the cooking profile transitions between stages (e.g. `Preheat` -> `Water Absorbing` -> `Boiling`).
A robust parser must walk backward through the hex sequence in 2-character steps and skip `0xAA` to find the most recent physical temperature reading.

### 2.4 Buffer Capacity Requirements
At roughly one temperature sample every 10–15 seconds, a 60–90 minute cooking program accumulates 250–350 bytes (500–700 hex characters).
- ESP8266 default `uart:` `rx_buffer_size` is 256 bytes, which dropped bytes mid-frame during transmission bursts.
- Increasing `rx_buffer_size: 1024` on `cooker_uart` and setting internal `rx_buffer_` capacity to 1024 bytes completely eliminates buffer drops.

---

## 3. Disassembly Analysis of the Lid Warning System & `props runstatus`

Disassembly of all MCU event tables (`0x38c8`–`0x4480`) and status formatters confirmed:
- Bit `0x08` of `result[7]` (`setting`) is strictly a **user configuration toggle** (`lid_warning_enabled`), not a live microswitch telemetry sensor.
- However, during active cooking cycles, the MCU pushes asynchronous execution events via `props runstatus`:
  - `notice_<recipe_8h><remaining_4h><stage_4h><notice_code_2h><param_2h><param_2h>`:
    Broadcast whenever a cooking alert occurs.
    - Code `0x01`: Add Ingredient Notice (recipe step alert)
    - Code `0x02`: Stir Notice
    - Code `0x03`: Lid Opened Alert (triggered when the lid is unlatched/opened during active heating)
    - Code `0x04`: Close Lid Reminder (warning buzzer if lid remains open)
    - Code `0x06`: Steam Valve Alert
    - Code `0x07`: Pressure Alert
  - `fire_<hex>`: Heating Active / Resumed (broadcast when heating kicks in or resumes after lid closure)
  - `start_<hex>` / `startp_<hex>`: Cooking Program Started
  - `cancel_<hex>`: Cooking Cancelled
  - `pause_<hex>`: Cooking Paused
  - `resume_<hex>` / `resumep_<hex>`: Cooking Resumed
  - `finish_<hex>` / `finisha_<hex>`: Cooking Finished
- These events are parsed in [`chunmi_cooker.cpp`](../components/chunmi_cooker/chunmi_cooker.cpp) into clean, human-readable strings for the `Last Event` text sensor.

