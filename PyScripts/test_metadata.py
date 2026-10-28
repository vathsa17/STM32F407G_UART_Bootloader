
import argparse
import struct
import time
import zlib
from pathlib import Path

import serial


# ============================================================
# Configuration — must match the STM32 bootloader
# ============================================================

DEFAULT_PORT = "COM5"
DEFAULT_BAUDRATE = 9600

SOF = 0xAA
CMD_METADATA = 0x01  # Change if your STM32 defines another value

APP_MAX_SIZE = 917504  # 896 KiB application region


# ============================================================
# CRC-16/CCITT-FALSE
# Polynomial: 0x1021
# Initial value: 0xFFFF
# RefIn/RefOut: False
# XOR-out: 0x0000
# ============================================================

def crc16_ccitt_false(data: bytes) -> int:
    crc = 0xFFFF

    for byte in data:
        crc ^= byte << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF

    return crc


# ============================================================
# Build protocol frame
#
# SOF | CMD | LEN (LE) | PAYLOAD | CRC16 (LE)
#
# CRC covers CMD + LEN + PAYLOAD.
# ============================================================

def build_packet(command: int, payload: bytes) -> bytes:
    if len(payload) > 256:
        raise ValueError("Payload exceeds 256 bytes")

    header_and_payload = (
        struct.pack("<BH", command, len(payload))
        + payload
    )

    crc = crc16_ccitt_false(header_and_payload)

    return (
        bytes([SOF])
        + header_and_payload
        + struct.pack("<H", crc)
    )


# ============================================================
# Read firmware file and calculate metadata
# ============================================================

def read_firmware(path: Path):
    if not path.is_file():
        raise FileNotFoundError(f"Firmware file not found: {path}")

    firmware = path.read_bytes()

    if len(firmware) == 0:
        raise ValueError("Firmware file is empty")

    if len(firmware) > APP_MAX_SIZE:
        raise ValueError(
            f"Firmware is too large: {len(firmware)} bytes. "
            f"Maximum allowed: {APP_MAX_SIZE} bytes."
        )

    image_size = len(firmware)
    image_crc32 = zlib.crc32(firmware) & 0xFFFFFFFF

    return firmware, image_size, image_crc32


# ============================================================
# Metadata packet test
# ============================================================

def test_metadata(firmware_path: Path, port: str, baudrate: int):
    firmware, image_size, image_crc32 = read_firmware(firmware_path)

    print("Firmware file :", firmware_path)
    print("Image size    :", image_size, "bytes")
    print(f"Image CRC-32  : 0x{image_crc32:08X}")

    # Metadata payload:
    # Bytes 0..3: image size, little-endian
    # Bytes 4..7: image CRC-32, little-endian
    metadata_payload = struct.pack("<II", image_size, image_crc32)

    packet = build_packet(CMD_METADATA, metadata_payload)

    print("\nMetadata payload:", metadata_payload.hex(" "))
    print("Complete packet :", packet.hex(" "))
    print(f"Packet CRC-16   : 0x{int.from_bytes(packet[-2:], 'little'):04X}")

    # Sanity-check the Python CRC implementation.
    test_crc = crc16_ccitt_false(b"123456789")
    assert test_crc == 0x29B1, (
        f"CRC implementation error: got 0x{test_crc:04X}"
    )

    print("\nOpening serial port:", port)

    with serial.Serial(
        port=port,
        baudrate=baudrate,
        timeout=3,
        write_timeout=3
    ) as ser:

        # Allow the serial port to settle.
        time.sleep(0.2)
        ser.reset_input_buffer()

        # ----------------------------------------------------
        # Step 1: Request binary mode using the text command.
        # ----------------------------------------------------
        print("\nSending text command: BEGIN")

        ser.write(b"BEGIN\n")
        ser.flush()

        response = ser.readline()

        print("STM32 response:", repr(response))

        if response.strip() != b"OK":
            print(
                "ERROR: Expected OK from STM32. "
                "Check BEGIN command handling and text mode."
            )
            return

        # ----------------------------------------------------
        # Step 2: Send the binary metadata frame.
        # ----------------------------------------------------
        print("\nSending binary metadata packet...")

        bytes_written = ser.write(packet)
        ser.flush()

        if bytes_written != len(packet):
            print(
                f"ERROR: Sent {bytes_written} of "
                f"{len(packet)} bytes."
            )
            return

        print(f"Sent {bytes_written} bytes.")

        # ----------------------------------------------------
        # Step 3: Read temporary STM32 test response.
        #
        # Expected during the current development test:
        #   CRC OK\r\n
        # or
        #   CRC ERROR\r\n
        #
        # Your STM32 must currently print these responses.
        # ----------------------------------------------------
        response = ser.readline()

        print("STM32 validation response:", repr(response))

        if response.strip() == b"MetaData OK":
            print("\nSUCCESS: Metadata frame CRC-16 validated.")
            print("The metadata came from the actual .bin file.")
        elif response.strip() == b"CRC ERROR":
            print("\nFAIL: STM32 rejected the packet CRC-16.")
        elif response == b"":
            print("\nTIMEOUT: No response received from STM32.")
            print("Check binary mode, parser integration and UART.")
        else:
            print("\nUnexpected response.")
            print("Expected the temporary response: CRC OK or CRC ERROR")


# ============================================================
# Entry point
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        description="Test STM32 bootloader metadata packet parsing."
    )

    parser.add_argument(
        "firmware",
        type=Path,
        help="Path to the firmware .bin file"
    )

    parser.add_argument(
        "--port",
        default=DEFAULT_PORT,
        help=f"Serial port (default: {DEFAULT_PORT})"
    )

    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUDRATE,
        help=f"UART baud rate (default: {DEFAULT_BAUDRATE})"
    )

    args = parser.parse_args()

    try:
        test_metadata(args.firmware, args.port, args.baud)
    except (OSError, ValueError, serial.SerialException) as exc:
        print(f"\nERROR: {exc}")


if __name__ == "__main__":
    main()
