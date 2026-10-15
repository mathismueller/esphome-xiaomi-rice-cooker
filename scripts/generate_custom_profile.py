#!/usr/bin/env python3
"""
Chunmi IH Rice Cooker (IHFB01CM) Profile Generator & Validator.

Generates and validates 242-hex character (121-byte) profile strings for the
NXP LPC11xx MCU on the Xiaomi IH Rice Cooker.
"""

import argparse
import binascii
import sys
from typing import Any, Dict, Optional

# Stock profiles for reference and template use
STOCK_PROFILES = {
    "fine_rice": "0001E101000000000000800200A00069030103730000085A020000EB006B040103740000095A0400012D006E0501037400000A5A0401FFFF00700601047600000C5A0401052D0A0F3C0A1E91FF820E01FF05FF78826EFF10FF786E02690F0DFF826EFF691000FF826EFF69100069FF5AFF00000000000081E9",
    "quick_rice": "0002E100280000000000800200D20069030103730000075A0200012D006B040103740000075A02000182006E050003740000095A0401FFFF0070060004760000095A040100280A063C0D1E91FF820E01FF05FF78826EFF10FF786E02690F0FFF826EFF69100082826EFF69100069FF5AFF00000000000015B6",
    "porridge": "0003E2011E04000028008000019055140601001600000000000001B855140601001900000000000001E0561406000020000000000000FFFF57140600002200000000000000280A0082001E914E730E01001E82FF736E0610FF756E02690A1E75826E0269100F75826E0269100069005A000000000000001A2B",
    "keep_warm": "00040C180018000001000045000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000280A0082001E914E730801001E82FF736E0610FF756E02690A0F75826E0169101E75826E0169100069005A000000000000001BA2",
    "cake": "010088010001000028000012000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000280A003C00089178320600001E8278666E041000826E00690FFF96666E0469100082826E0069100069005A00000000000000B267",
    "cooking": "010366001E0100000A010540000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000280A003C001E9155730E0000008278736E001000756E00690A0075826E006910FF756E6E0869100069005A00000000000000F462",
    "sushi_rice": "0105E100320000000002800200780069030103730000085A020000B4006B040103740000095A04000140006E0501037400000A5A0401FFFF00700601047600000C5A040103840A0F3C0A1E91FF820E01FF05FF78826EFF10FF786E02690F0DFF826EFF691000FF826EFF69100069FF5AFF000000000000FB8B",
    "express_rice": "0106E1001E00000000018002005A0069030103730000075A02000078006B040103740000075A0200012C006E050103740000095A0401FFFF0070060104760000095A040101680A0F3C0A1E91FF820E01FF05FF78826EFF10FF786E02690F0DFF826EFF691000FF826EFF69100069FF5AFF00000000000021A8",
}


def parse_int_auto(val: str) -> int:
    """Parse integer from hex (0x...) or decimal string."""
    return int(val, 0)


def generate_custom_profile(
    menu_id: int,
    duration_hours: int = 0,
    duration_minutes: int = 30,
    max_hours: int = 0,
    max_minutes: int = 0,
    min_hours: int = 0,
    min_minutes: int = 0,
    allow_keepwarm: bool = True,
    stages_raw: Optional[bytes] = None,
    pwm_hold_raw: Optional[bytes] = None,
    template: Optional[str] = None,
) -> str:
    """
    Generate a 242-char hex profile string accepted by the NXP LPC11xx MCU.
    
    Structure:
      0x00..0x0C (12 B): Header & Duration Limits
      0x0C..0x44 (56 B): Stages 1-4 (4 records * 14 B)
      0x44..0x73 (47 B): Stage 5 / PWM Hold Table
      0x73..0x77 ( 4 B): Null Padding (0x00000000)
      0x77..0x79 ( 2 B): Big-endian CRC-16/CCITT-FALSE
      Total: 121 Bytes = 242 hex characters.
    """
    if menu_id < 0x0100 or menu_id > 0xFFFF:
        raise ValueError(
            f"Custom profiles must have a Menu ID >= 0x0100 (0x{menu_id:04X} given; 0x0001-0x00FF are reserved by MCU)"
        )

    # Initialize empty 119-byte payload (up to CRC offset)
    payload = bytearray(119)

    # If template is selected, borrow its stage curves and hold table
    if template and template in STOCK_PROFILES:
        tmpl_bytes = bytes.fromhex(STOCK_PROFILES[template])
        payload[12:68] = tmpl_bytes[12:68]   # 56 bytes stage curves
        payload[68:115] = tmpl_bytes[68:115] # 47 bytes PWM hold table

    # 1. Menu ID (Big-Endian)
    payload[0:2] = menu_id.to_bytes(2, "big")

    # 2. Features Bitmask
    flags = 0x80  # Induction active
    if allow_keepwarm:
        flags |= 0x20
    if max_hours > 0 or max_minutes > 0:
        flags |= 0x02  # Adjustable duration flag
    payload[2] = flags

    # 3. Durations
    payload[3] = duration_hours
    payload[4] = duration_minutes
    payload[5] = max_hours
    payload[6] = max_minutes
    payload[7] = min_hours
    payload[8] = min_minutes
    payload[9] = 0x01 if (max_hours > 0 or max_minutes > 0) else 0x00
    payload[10:12] = (0x8002).to_bytes(2, "big")

    # 4. Stages Curve (56 bytes: 4 stages * 14 bytes)
    if stages_raw is not None:
        if len(stages_raw) != 56:
            raise ValueError(f"stages_raw must be exactly 56 bytes (got {len(stages_raw)})")
        payload[12:68] = stages_raw

    # 5. Induction Modulation & Hold (47 bytes)
    if pwm_hold_raw is not None:
        if len(pwm_hold_raw) != 47:
            raise ValueError(f"pwm_hold_raw must be exactly 47 bytes (got {len(pwm_hold_raw)})")
        payload[68:115] = pwm_hold_raw
    elif not template:
        # Default hold table (Hold 73°C with end delimiters, exactly 47 bytes)
        payload[68:115] = bytes.fromhex(
            "00280a0082001e914e730801001e82ff736e0610ff756e02690a0f75826e0169101e75826e0169100069005a000000"
        )

    # 6. Trailer (4 bytes null padding at 115..118 already initialized to 0)

    assert len(payload) == 119, f"Internal error: payload length is {len(payload)}, expected 119"

    # 7. Checksum (CRC-16/CCITT-FALSE across 119 bytes)
    crc = binascii.crc_hqx(bytes(payload), 0x0000)
    full_profile = bytes(payload) + crc.to_bytes(2, "big")

    assert len(full_profile) == 121, f"Internal error: full_profile length is {len(full_profile)}, expected 121"

    return full_profile.hex().upper()


def validate_profile(hex_str: str) -> Dict[str, Any]:
    """
    Validate a 242-char hex profile string according to MCU acceptance rules.
    """
    hex_clean = hex_str.strip().replace(" ", "").replace('"', "").replace("'", "")
    
    result: Dict[str, Any] = {
        "hex": hex_clean.upper(),
        "char_length": len(hex_clean),
        "valid": False,
        "errors": [],
        "warnings": [],
    }

    if len(hex_clean) != 242:
        result["errors"].append(
            f"Invalid length: Expected exactly 242 hex characters (121 bytes), got {len(hex_clean)}."
        )
        return result

    try:
        raw = bytes.fromhex(hex_clean)
    except ValueError as e:
        result["errors"].append(f"Invalid hex string: {e}")
        return result

    # Checksum validation
    calc_crc = binascii.crc_hqx(raw[:119], 0x0000)
    stored_crc = int.from_bytes(raw[119:121], "big")
    result["calculated_crc"] = f"0x{calc_crc:04X}"
    result["stored_crc"] = f"0x{stored_crc:04X}"

    if calc_crc != stored_crc:
        result["errors"].append(
            f"CRC mismatch! Computed 0x{calc_crc:04X}, profile specifies 0x{stored_crc:04X}."
        )

    # Parse Fields
    menu_id = int.from_bytes(raw[0:2], "big")
    flags = raw[2]
    dur_h, dur_m = raw[3], raw[4]
    max_h, max_m = raw[5], raw[6]
    min_h, min_m = raw[7], raw[8]

    result["menu_id"] = f"0x{menu_id:04X}"
    result["nominal_duration"] = f"{dur_h:02d}:{dur_m:02d} ({dur_h * 60 + dur_m} min)"
    result["adjustable_duration"] = bool(flags & 0x02)
    result["max_duration"] = f"{max_h:02d}:{max_m:02d}" if (max_h or max_m) else "None"
    result["min_duration"] = f"{min_h:02d}:{min_m:02d}" if (min_h or min_m) else "None"
    result["auto_keepwarm"] = bool(flags & 0x20)
    result["induction_active"] = bool(flags & 0x80)

    # Menu ID Validation
    if 0x0001 <= menu_id <= 0x0004:
        result["profile_type"] = "Stock Built-in Routine"
    elif 0x0005 <= menu_id <= 0x00FF:
        result["profile_type"] = "Reserved System ID"
        result["errors"].append(
            f"Menu ID 0x{menu_id:04X} is in the reserved range (0x0005-0x00FF) and will be rejected by the MCU."
        )
    else:
        result["profile_type"] = "Custom / Extended Profile"

    result["valid"] = len(result["errors"]) == 0
    return result


def print_validation_report(res: Dict[str, Any]) -> None:
    print("=" * 60)
    print("CHUNMI IH COOKER PROFILE VALIDATION REPORT")
    print("=" * 60)
    print(f"Status:             {'VALID' if res['valid'] else 'INVALID'}")
    print(f"Hex Length:         {res['char_length']} chars (121 bytes)")
    if "menu_id" in res:
        print(f"Menu ID:            {res['menu_id']} ({res.get('profile_type', 'Unknown')})")
        print(f"Nominal Duration:   {res['nominal_duration']}")
        print(f"Adjustable Range:   {res['min_duration']} - {res['max_duration']}")
        print(f"Auto Keep-Warm:     {'Enabled' if res['auto_keepwarm'] else 'Disabled'}")
        print(f"Induction Drive:    {'Active' if res['induction_active'] else 'Inactive'}")
        print(f"CRC-16 Checksum:    {res['stored_crc']} (Computed: {res['calculated_crc']})")

    if res["warnings"]:
        print("\nWarnings:")
        for w in res["warnings"]:
            print(f"  [!] {w}")

    if res["errors"]:
        print("\nErrors:")
        for err in res["errors"]:
            print(f"  [X] {err}")
    print("=" * 60)


def main():
    parser = argparse.ArgumentParser(
        description="Chunmi IH Rice Cooker (IHFB01CM) Profile Generator & Validator"
    )
    subparsers = parser.add_subparsers(dest="command", help="Command to execute")

    # Command: generate
    gen_parser = subparsers.add_parser("generate", help="Generate a custom profile string")
    gen_parser.add_argument(
        "--menu-id",
        type=parse_int_auto,
        default=0x0105,
        help="Menu ID (hex e.g. 0x0105 or int >= 256). Default: 0x0105",
    )
    gen_parser.add_argument(
        "--hours",
        type=int,
        default=0,
        help="Nominal duration hours (0-23). Default: 0",
    )
    gen_parser.add_argument(
        "--minutes",
        type=int,
        default=45,
        help="Nominal duration minutes (0-59). Default: 45",
    )
    gen_parser.add_argument(
        "--max-hours",
        type=int,
        default=0,
        help="Max adjustable hours. Default: 0",
    )
    gen_parser.add_argument(
        "--max-minutes",
        type=int,
        default=0,
        help="Max adjustable minutes. Default: 0",
    )
    gen_parser.add_argument(
        "--min-hours",
        type=int,
        default=0,
        help="Min adjustable hours. Default: 0",
    )
    gen_parser.add_argument(
        "--min-minutes",
        type=int,
        default=0,
        help="Min adjustable minutes. Default: 0",
    )
    gen_parser.add_argument(
        "--keepwarm",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Enable auto keep-warm upon completion (--keepwarm or --no-keepwarm). Default: true",
    )
    gen_parser.add_argument(
        "--template",
        choices=list(STOCK_PROFILES.keys()),
        default=None,
        help="Base stage curves and modulation on a stock profile template (e.g. fine_rice, sushi_rice, cooking, cake)",
    )

    # Command: validate
    val_parser = subparsers.add_parser("validate", help="Validate a 242-hex character profile")
    val_parser.add_argument("profile_hex", help="242-character hex profile string")

    # Command: stock
    subparsers.add_parser("stock", help="List and validate all stock cooker profiles")

    args = parser.parse_args()

    if args.command == "generate":
        try:
            profile = generate_custom_profile(
                menu_id=args.menu_id,
                duration_hours=args.hours,
                duration_minutes=args.minutes,
                max_hours=args.max_hours,
                max_minutes=args.max_minutes,
                min_hours=args.min_hours,
                min_minutes=args.min_minutes,
                allow_keepwarm=args.keepwarm,
                template=args.template,
            )
            print(f"\nGenerated Custom Profile for Menu ID 0x{args.menu_id:04X} ({args.hours}h {args.minutes}m):")
            print(profile)
            print("\nValidation check:")
            res = validate_profile(profile)
            print_validation_report(res)
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            sys.exit(1)

    elif args.command == "validate":
        res = validate_profile(args.profile_hex)
        print_validation_report(res)
        if not res["valid"]:
            sys.exit(1)

    elif args.command == "stock":
        print(f"Validating {len(STOCK_PROFILES)} Stock Profiles:\n")
        all_ok = True
        for name, hex_str in STOCK_PROFILES.items():
            res = validate_profile(hex_str)
            status = "OK" if res["valid"] else "FAIL"
            if not res["valid"]:
                all_ok = False
            print(f"- {name:<14} [0x{int.from_bytes(bytes.fromhex(hex_str)[:2], 'big'):04X}]: {res['nominal_duration']:<18} CRC {res['stored_crc']} -> {status}")
        if not all_ok:
            sys.exit(1)

    else:
        # No arguments given: show help and run quick demo
        parser.print_help()
        print("\n" + "-" * 60)
        print("Quick Demo (Generating sample 45-min Steam Profile):")
        demo = generate_custom_profile(menu_id=0x0105, duration_hours=0, duration_minutes=45, allow_keepwarm=False)
        print(demo)
        print("-" * 60)


if __name__ == "__main__":
    main()
