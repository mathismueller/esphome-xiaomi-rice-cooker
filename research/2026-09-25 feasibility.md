# Feasibility: ESPHome on the MHCW03P, stock heater + UI untouched

Date: 2026-09-25. Builds on `2026-09-25 manual investigation.md` (teardown,
miIO local control) and settles the approach + first bench steps.

## 1. Scope decisions (fixed)

- **Firmware target: MHCW03P Wi-Fi module only.** The ESP8266 currently runs
  Xiaomi's miIO firmware; it gets replaced with ESPHome acting as a
  strict-local UART peer to the stock host MCU(s).
- **Heater/power hardware is never touched.** Only low-voltage lid-board
  systems are accessed, and they keep their stock firmware. All thermal
  protection stays exactly as Xiaomi built it.
- **Lid display, buttons, buzzer keep working** via the stock host MCU +
  TM1624. ESPHome mirrors state; it does not drive the UI.
- **Non-destructive bench work only**: soldering pin headers onto unpopulated
  pads and unplugging the logic board are OK; no desoldering, no mains work.

## 2. Hardware picture (from teardown photos)

- Device `IHFB01CM`, miIO model `chunmi.cooker.normal2`
  (`MODEL_NORMAL_GROUP1`; `miiocli` reports `Hardware version: ESP8266`,
  `Firmware version: 1.2.8`; `set_menu`/`start`/`stop` verified working).
- Lid logic board `CMIHFKACO2` carries:
  - MHCW03P module (ESP8266, label `MODEL MHCW03P`, `CMIIT ID 2016DP1293`) —
    see `wifi-module-mhcw03p-closeup.jpeg`.
  - IC1: center QFP host MCU (**markings still unread — do not touch it**).
  - IC4: TM1624 LED driver. U2: unidentified SOIC. U4: SPI flash on the
    `FLASH` header (ownership ESP-vs-MCU **unconfirmed** — never write it
    until mapped).
  - 11-wire harness to the heater base (`GND,5V,SCL,SDA,BUZ1,BUZ2,VDD` +
    `B_LED,O_LED,KEY,GND`) — see `main-board-connector-closeup.jpeg`.

## 3. The three exposed UART interfaces

Read off the silkscreen in `main-board-top-side-overview.jpeg`. All probing
is **passive first** (adapter RX + GND only).

| # | Header (silkscreen) | Location | Pins (as printed) | Working hypothesis |
|---|---|---|---|---|
| 1 | `UART1` | top right, col 1 | `VCC / RX / TX / + / GND` | ESP8266 UART0 (boot log + miIO link side) |
| 2 | `UART2` | top right, col 2 | `VCC / RX / TX / + / GND` | Second ESP8266/host-MCU channel, or host MCU debug |
| 3 | unlabeled + `BOOT` | bottom middle, near U2 | `BOOT / GND / TX / RX / 5V / VCC` | Host MCU (IC1/U2) debug/console UART |

Notes:

- The 4th row of UART1/2 (`+`) is unreadable in photos — **verify with a
  DMM (continuity to VCC/GND) before connecting anything**.
- `VCC` rail voltage is unconfirmed (3.3V vs 5V). **Measure VCC–GND with the
  DMM before powering any adapter from the board**; prefer USB-powered
  adapter + common GND, or the bench supply with a current limit.
- Neither the UART1-vs-UART2 assignment nor the bottom UART's owner is
  known — determining that is exactly what the first probe session is for.

## 4. Protocol expectation

Per Xiaomi's `miio_open` MHCW03P docs
([wifi/](https://github.com/MiEcosystem/miio_open/tree/master/wifi/MHCW03P)):

- LOG/boot output: **74880 8N1**. Comm UART: **115200 8N1**.
- Text commands, `\r`-terminated, ask/answer style. `help` lists commands,
  `echo on` enables echo. Host MCU reports `model` + `mcu_version` at boot,
  pushes state via `props ...`, and polls `get_down` (~200ms) for cloud
  commands (`down none` vs. e.g. `set_power on`, answered with `result`).
- So on the correct UART at 115200 we expect readable ASCII (`model`,
  `mcu_version`, `props`, `get_down`/`down ...`); at 74880 during reset we
  expect the ESP8266 boot ROM + `firmware: 1.2.x` banner.

Contrast: the newer-cooker project
([daniel-dona/MFB120A-1](https://github.com/daniel-dona/MFB120A-1), ESP32 as
*main* SoC, binary `0x55/0xAA` + CRC16 frames at 9600, ESPHome owns the
heater relay) is **not** our architecture — it violates our low-voltage-only
rule. Reuse from it: the ESPHome external-component *skeleton*
(UARTDevice + poll loop + HA entities + program state machine), never the
heater ownership or frame codec.

## 5. Feasibility verdict

**Feasible, medium effort.** The whole project reduces to: (a) identify which
header carries the miIO text dialogue, (b) record its semantics against the
already-documented `python-miio` cooker model, (c) re-flash the MHCW03P with
an ESPHome firmware speaking the same dialogue. No heater reverse
engineering, no display driver work, no lid-MCU firmware.

Brick risk: the ESP8266 has **no secure-boot / flash-encryption eFuses**
(those are ESP32-only hazards), so there is no lockout to burn. Any bad
flash is recoverable via UART download mode (`GPIO0` low at reset) or, worst
case, a SOIC-8 clip + CH341A rewrite of a verified backup. Rule stands: **two
verified `esptool read_flash` dumps before any write.** The lid MCU is the
only component that *could* have lock bits — hence it stays read-only.

## 6. Tooling (pinned in this repo, status 2026-09-25)

- `uv sync` provides: esphome 2026.9.0, esptool v5.3.1, pyserial 3.5.
  Invoke per AGENT.md: `uv run --no-sync esphome ...`,
  `uv run --no-sync esptool.py ...`.
- `scripts/uart_probe.py` (verified with a pty loopback selftest):
  - `capture --port … --baud … --label uart1` — passive timestamped
    hex+ASCII log to `tmp/` (default, safe).
  - `scan --port … --label uart1` — rank baud candidates by text-likeness.
  - `send --cmd help --active` — transmit `\r`-terminated command; refused
    without `--active`.
  - `ports` — list adapters.
- Still to acquire: 8ch logic analyzer (Saleae-class clone is enough for
  ≤115200 baud), SOIC-8 clip + CH341A (unbrick fallback). Owned already:
  3.3V USB-UART, bench supply, multimeter, soldering station.

## 7. First probe session — bench procedure

Preconditions:

1. Heater base **unplugged from mains**; lid board powered on-bench only
   (bench supply, current-limited) or via its 5V rail with cooker reassembled
   but **mains disconnected**.
2. USB-UART adapter strapped to **3.3V logic** (never 5V). Stock firmware is
   running untouched.
3. DMM checks done: GND continuity adapter↔board, VCC rail voltage noted,
   UART1/2 `+` row identified.

Per UART header (UART1 → UART2 → bottom BOOT UART):

1. `uv run scripts/uart_probe.py scan --port <adapter> --label <uartN>`
   (adapter RX ← board TX, GND ↔ GND, adapter TX disconnected).
2. If traffic found: `capture --baud <winner> --duration 60` across a reset
   (watch for 74880 boot banner) and across `miiocli` commands
   (`status`, `set_menu`, `start`, `stop`) to correlate wire traffic with
   known miIO actions.
3. Record per header in the table below; move on. Stop at the first header
   showing the miIO text dialogue — remaining headers are documented
   opportunistically, not exhaustively.

Expected signatures:

- ESP8266 boot log @74880 (ROM spew, then `firmware: 1.2.x`,
  `mac address is …`).
- miIO dialogue @115200 (`model`, `mcu_version`, `props …`, `get_down` /
  `down …`, `result`).
- Anything else (binary frames, silence) → note baud + sample, do not force.

Results (fill in at the bench):

| Header | Traffic? | Baud | Signature sample | Verdict |
|---|---|---|---|---|
| UART1 | | | | |
| UART2 | | | | |
| bottom BOOT UART | | | | |

Active probing (`send help` / `echo on`) only after: the miIO UART is
identified, captures are saved, heater base is unplugged, and adapter TX →
board RX is wired to the *correct* header. One command at a time, capture
every reply.

## 8. Next steps after identification

1. Full `esptool read_flash` backup(s) → `tmp/` (git-ignored), checksummed.
2. Minimal ESPHome YAML for the MHCW03P (ESP8266, `logger.baud_rate: 0`,
   HW UART at sniffed baud), bench-boot with MCU link open, then reconnect
   and log inbound frames.
3. Custom `cooker` UART component mirroring `MODEL_NORMAL_GROUP1`
   semantics (mode/temp/remaining sensors, menu select, start/stop), with
   safety guards (command allowlist, max-duration watchdog, valid-temp
   requirement). MFB120A-1's component layout is the template.
4. Supervised cooking validation; update README + a dated research note.
