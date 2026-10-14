# Rice Cooker Firmware

Alternative strictly-local firmware for the Xiaomi IH Rice Cooker
(`IHFB01CM`, miIO `chunmi.cooker.normal2`).

**Approach:** replace firmware on the MHCW03P Wi-Fi module (ESP8285) with
ESPHome that speaks the stock UART dialogue to the cooker's own host MCU.
Heater control, thermal protection, display, buttons, and buzzer all stay
stock. Only low-voltage lid-board systems are ever accessed; mains/heater
hardware is out of scope.

**Status:** Fully working bidirectional ESPHome firmware implemented and verified on live hardware under AC mains power. Inner pot detection, bidirectional menu selection, cooking cycle start/stop, Wi-Fi reset handling, and extended telemetry (temperature, mode, stage, countdown) are all working locally via Home Assistant and the built-in web interface.

## Documentation

Detailed technical documentation is available under [`documentation/`](documentation/):

- [Hardware & Pinout Reference](documentation/hardware-and-pinout.md) — Silicon identification, pinouts for headers (UART1, UART2, Flash port, JW), and bench power setup.
- [Serial Protocol Specification](documentation/serial-protocol.md) — Line-based 115200 8N1 protocol, downlink commands, 12-element status array schema, async event codes, and command queue architecture.
- [Firmware Analysis & Flash Layout](documentation/firmware-analysis.md) — 2MB SPI flash geometry, bootloader, user1 miIO binary, embedded NXP LPC11xx MCU binary, and OTA mechanism.
- [ESPHome Implementation Architecture](documentation/esphome-architecture.md) — Custom `chunmi_cooker` external component design, C++ classes, Home Assistant entity models, and native API services.

## Home Assistant Integration

The firmware exposes full telemetry and control directly to Home Assistant without cloud dependencies:

* **Control & Presets**: Menu selector, cycle stop/cancel, and one-touch start buttons for Quick Rice, Standard Rice, Porridge, Keep Warm, Cooking/Steam, Cake, Stored Favorite, Sushi Rice (Experimental), and 30-minute Express Rice (Experimental).
* **Telemetry**: Real-time mode, cooking phase/stage, countdown timer, ambient/cooking temperature, pot interlock detection, auto keep-warm toggle, and stored favorite profile.
* **Safety & Diagnostics**: Dedicated error text sensor (`sensor.error_description`) with granular hardware fault descriptions, last lid button pressed, and hardware/firmware revision info.
* **Native API Actions / Services**: Call actions from Home Assistant automations or Developer Tools (`set_interaction`, `set_led_timeout`, `set_lid_warning_timeout`, `set_alarm_delay`, `send_raw_command`). See [Architecture Reference](documentation/esphome-architecture.md#41-home-assistant-native-api-services--actions) for parameters and syntax.

## Quickstart

```sh
uv sync                                  # project-pinned esphome, esptool, pyserial
uv run esphome compile cooker.yaml       # compile firmware
uv run esphome upload --device <IP> cooker.yaml # OTA flash
```

See `research/2026-09-25 feasibility.md` for the full bench procedure
(wiring, safety checklist, the three UART headers, expected signatures).

## Safety

- Appliance controller MCU (NXP LPC1100 ARM Cortex-M0) autonomously enforces all thermal safety margins, induction inverter drive, and pot detection loops.
- Heater base stays **unplugged from mains** during bench flashing.

## Layout

- `components/` — custom `chunmi_cooker` ESPHome external component.
- `documentation/` — structured technical guides, hardware pinouts, and protocol specs.
- `research/` — chronological notes (teardown, feasibility, protocol, implementation logs).
- `images/` — teardown photos (descriptive names).
- `scripts/` — bench helpers.
- `src/` — Python package scaffold.
- `tmp/` — captures, dumps, scratch (git-ignored, never committed).
