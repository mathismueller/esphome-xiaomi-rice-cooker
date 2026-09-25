# Rice Cooker Firmware

Alternative strictly-local firmware for the Xiaomi IH Rice Cooker
(`IHFB01CM`, miIO `chunmi.cooker.normal2`).

**Approach:** replace firmware on the MHCW03P Wi-Fi module (ESP8266) with
ESPHome that speaks the stock UART dialogue to the cooker's own host MCU.
Heater control, thermal protection, display, buttons, and buzzer all stay
stock. Only low-voltage lid-board systems are ever accessed; mains/heater
hardware is out of scope.

**Status:** investigation + bench tooling. No custom firmware yet.

## Quickstart

```sh
uv sync                                  # project-pinned esphome, esptool, pyserial
uv run scripts/uart_probe.py ports       # find your USB-UART adapter
uv run scripts/uart_probe.py scan --port <dev> --label uart1        # rank bauds (passive)
uv run scripts/uart_probe.py capture --port <dev> --baud 115200 --label uart1  # log traffic
```

See `research/2026-09-25 feasibility.md` for the full bench procedure
(wiring, safety checklist, the three UART headers, expected signatures).

## Safety

- Heater base stays **unplugged from mains** during all bench work.
- Passive taps only (adapter RX + GND); adapter logic at **3.3V, never 5V**.
- Active UART transmission only with `--active`, on the identified header.
- Two verified `esptool read_flash` backups before any flash write.
- Never touch the lid host MCU firmware (possible lock bits, no recovery).

## Layout

- `research/` — chronological notes (teardown, feasibility, protocol).
- `images/` — teardown photos (descriptive names).
- `scripts/` — bench helpers (`uart_probe.py`).
- `src/` — Python package scaffold.
- `tmp/` — captures, dumps, scratch (git-ignored, never committed).
