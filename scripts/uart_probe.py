#!/usr/bin/env python3
"""Passive/active UART probing helper for the rice-cooker lid board.

The lid logic board (CMIHFKAC02) exposes three UART interfaces plus
FLASH/JTAG/POWER headers. This script helps identify what each UART does
*before* any firmware is touched:

  capture   passive tap: log timestamped hex+ASCII from one UART (default)
  scan      try common baud rates, rank them by how "text-like" the traffic is
  send      ACTIVE probe: transmit a command and capture the reply
            (refused unless --active is passed; see safety notes)

Passive wiring (capture/scan): USB-UART adapter RX -> board TX, GND -> GND.
Do NOT connect the adapter TX for passive taps. See research/ for the full
bench procedure and per-header pin tables.

Run with the project environment, e.g.:
  uv run scripts/uart_probe.py capture --port /dev/tty.usbserial-XXX --baud 115200 --label uart1
  uv run scripts/uart_probe.py scan --port /dev/tty.usbserial-XXX --label uart1
  uv run scripts/uart_probe.py send --port ... --label uart1 --cmd 'help' --active

Captures are written to tmp/ (git-ignored) as
  tmp/uart-capture-<label>-<baud>-<YYYYMMDD-HHMMSS>.log
"""

from __future__ import annotations

import argparse
import datetime
import sys
import time
from pathlib import Path

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    sys.exit("pyserial is required: run via 'uv run scripts/uart_probe.py ...'")

PROJECT_ROOT = Path(__file__).resolve().parent.parent
CAPTURE_DIR = PROJECT_ROOT / "tmp"

# Baud rates worth trying, most-likely first for this hardware:
# 115200 = MHCW03P miIO comm UART, 74880 = ESP8266 boot log,
# 9600 = common MCU debug / newer-cooker precedent.
BAUD_CANDIDATES = [115200, 74880, 9600, 57600, 38400, 19200, 230400, 460800]

PRINTABLE = set(b"\r\n\t" + bytes(range(0x20, 0x7F)))


def score_bytes(data: bytes) -> float:
    """Fraction of bytes that look like text (0.0-1.0). Empty -> 0.0."""
    if not data:
        return 0.0
    return sum(1 for b in data if b in PRINTABLE) / len(data)


def format_line(data: bytes, t0: float) -> str:
    """One capture-log line: elapsed time, hex dump, ASCII gutter."""
    elapsed = time.monotonic() - t0
    hexpart = " ".join(f"{b:02x}" for b in data)
    asciipart = "".join(chr(b) if b in PRINTABLE - {ord("\r"), ord("\n"), ord("\t")} else "." for b in data)
    return f"[+{elapsed:9.3f}s] {hexpart:<48} |{asciipart}|"


def open_port(port: str, baud: int) -> serial.Serial:
    return serial.Serial(
        port=port,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.1,
        write_timeout=1.0,
    )


def capture_stream(ser: serial.Serial, outfile, t0: float, duration: float, quiet: bool) -> int:
    """Read until duration elapses (0 = forever). Returns bytes captured."""
    total = 0
    end = None if duration <= 0 else time.monotonic() + duration
    try:
        while end is None or time.monotonic() < end:
            chunk = ser.read(256)
            if chunk:
                total += len(chunk)
                line = format_line(chunk, t0)
                outfile.write(line + "\n")
                outfile.flush()
                if not quiet:
                    print(line, flush=True)
    except KeyboardInterrupt:
        print("\nstopped by user", flush=True)
    return total


def new_capture_file(label: str, baud: int):
    CAPTURE_DIR.mkdir(parents=True, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    path = CAPTURE_DIR / f"uart-capture-{label}-{baud}-{stamp}.log"
    fh = path.open("w", encoding="utf-8")
    fh.write(f"# port label={label} baud={baud} started={stamp}\n")
    return fh, path


def cmd_capture(args) -> int:
    ser = open_port(args.port, args.baud)
    fh, path = new_capture_file(args.label, args.baud)
    print(f"capturing {args.port} @ {args.baud} 8N1 -> {path}")
    print("wiring reminder: adapter RX <- board TX, GND <-> GND, adapter TX DISCONNECTED")
    if args.duration > 0:
        print(f"capturing for {args.duration:.0f}s (Ctrl-C to stop early)")
    else:
        print("capturing until Ctrl-C")
    with ser, fh:
        total = capture_stream(ser, fh, time.monotonic(), args.duration, args.quiet)
    print(f"done: {total} bytes -> {path}")
    return 0


def cmd_scan(args) -> int:
    print(f"scanning {args.port} ({args.per_baud:.0f}s per baud, label={args.label})")
    print("wiring reminder: adapter RX <- board TX, GND <-> GND, adapter TX DISCONNECTED")
    results = []
    for baud in BAUD_CANDIDATES:
        fh, path = new_capture_file(f"{args.label}-scan", baud)
        with open_port(args.port, baud) as ser, fh:
            # Fresh RX buffer per baud so leftovers don't pollute scoring.
            ser.reset_input_buffer()
            t0 = time.monotonic()
            total = capture_stream(ser, fh, t0, args.per_baud, quiet=True)
        # Re-read raw bytes for scoring.
        raw = bytearray()
        for line in path.read_text(encoding="utf-8").splitlines()[1:]:
            hexpart = line.split("] ", 1)[1].rsplit(" |", 1)[0]
            raw.extend(int(tok, 16) for tok in hexpart.split())
        s = score_bytes(bytes(raw))
        results.append((s, total, baud, path))
        print(f"  {baud:>7}: {total:>6} bytes, text-score {s:.2f}  ({path.name})")
    results.sort(reverse=True)
    print("\nranking (best first):")
    for s, total, baud, path in results:
        print(f"  {baud:>7}: score {s:.2f}, {total} bytes")
    best = results[0]
    if best[1] == 0:
        print("\nno traffic on any baud: check wiring (RX<-TX, GND), board power, or try the next UART header.")
        return 1
    print(f"\nlikely baud: {best[2]} (verify by eye in {best[3].name}: boot text / 'help' prompts?)")
    return 0


def cmd_send(args) -> int:
    if not args.active:
        print(
            "refusing active probe without --active: transmitting on a live MCU link\n"
            "can confuse the stock firmware. Re-run with --active if you have the\n"
            "adapter TX wired to the board RX of the correct header and the heater\n"
            "base is UNPLUGGED. See research/ for the bench procedure.",
            file=sys.stderr,
        )
        return 2
    ser = open_port(args.port, args.baud)
    fh, path = new_capture_file(f"{args.label}-active", args.baud)
    payload = (args.cmd + "\r").encode("utf-8", errors="replace")
    print(f"sending {payload!r} on {args.port} @ {args.baud}, capturing reply -> {path}")
    with ser, fh:
        t0 = time.monotonic()
        ser.reset_input_buffer()
        ser.write(payload)
        total = capture_stream(ser, fh, t0, args.wait, args.quiet)
    print(f"done: {total} reply bytes -> {path}")
    return 0


def cmd_ports(_args) -> int:
    found = False
    for p in serial.tools.list_ports.comports():
        print(f"{p.device:30} {p.description} ({p.manufacturer or '?'})")
        found = True
    if not found:
        print("no serial ports found (adapter plugged in?)")
        return 1
    return 0


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)

    p = sub.add_parser("capture", help="passive timestamped capture (default, safe)")
    p.add_argument("--port", required=True, help="e.g. /dev/tty.usbserial-0001")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--label", default="uart", help="e.g. uart1, uart2, mcu-uart")
    p.add_argument("--duration", type=float, default=0, help="seconds, 0 = until Ctrl-C")
    p.add_argument("--quiet", action="store_true", help="log to file only")
    p.set_defaults(func=cmd_capture)

    p = sub.add_parser("scan", help="rank baud rates by text-likeness (passive, safe)")
    p.add_argument("--port", required=True)
    p.add_argument("--label", default="uart")
    p.add_argument("--per-baud", type=float, default=5, help="seconds per baud candidate")
    p.set_defaults(func=cmd_scan)

    p = sub.add_parser("send", help="transmit a \\r-terminated command (ACTIVE, needs --active)")
    p.add_argument("--port", required=True)
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--label", default="uart")
    p.add_argument("--cmd", default="help", help="command text, \\r is appended automatically")
    p.add_argument("--wait", type=float, default=3, help="seconds to capture the reply")
    p.add_argument("--quiet", action="store_true")
    p.add_argument("--active", action="store_true", help="acknowledge active probing (required)")
    p.set_defaults(func=cmd_send)

    p = sub.add_parser("ports", help="list serial ports")
    p.set_defaults(func=cmd_ports)

    return ap


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
