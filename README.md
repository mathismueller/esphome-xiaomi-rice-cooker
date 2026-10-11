# Rice Cooker Firmware

Alternative strictly-local firmware for the Xiaomi IH Rice Cooker
(`IHFB01CM`, miIO `chunmi.cooker.normal2`).

**Approach:** replace firmware on the MHCW03P Wi-Fi module (ESP8265) with
ESPHome that speaks the stock UART dialogue to the cooker's own host MCU.
Heater control, thermal protection, display, buttons, and buzzer all stay
stock. Only low-voltage lid-board systems are ever accessed; mains/heater
hardware is out of scope.

**Status:** Working ESPHome telemetry firmware implemented, bench-tested, and verified on hardware. Inter-MCU link streaming live telemetry (temperature, mode, stage, countdown) to web interface and Home Assistant.

## Documentation

Detailed technical documentation is available under [`documentation/`](documentation/):

- [Hardware & Pinout Reference](documentation/hardware-and-pinout.md) — Silicon identification, pinouts for headers (UART1, UART2, Flash port, JW), and bench power setup.
- [Serial Protocol Specification](documentation/serial-protocol.md) — Line-based 115200 8N1 protocol, downlink commands, 12-element status array schema, and async hardware event codes.
- [Firmware Analysis & Flash Layout](documentation/firmware-analysis.md) — 2MB SPI flash geometry, bootloader, user1 miIO binary, embedded NXP LPC11xx MCU binary, and OTA mechanism.
- [ESPHome Implementation Architecture](documentation/esphome-architecture.md) — Custom `chunmi_cooker` external component design, C++ classes, and Home Assistant entity models.

## Quickstart

```sh
uv sync                                  # project-pinned esphome, esptool, pyserial
```

See `research/2026-09-25 feasibility.md` for the full bench procedure
(wiring, safety checklist, the three UART headers, expected signatures).

## Safety

- Heater base stays **unplugged from mains** during all bench work.

## Layout

- `documentation/` — structured technical guides, hardware pinouts, and protocol specs.
- `research/` — chronological notes (teardown, feasibility, protocol).
- `images/` — teardown photos (descriptive names).
- `scripts/` — bench helpers
- `src/` — Python package scaffold.
- `tmp/` — captures, dumps, scratch (git-ignored, never committed).
