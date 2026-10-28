
import argparse
import struct
import time
import zlib
from pathlib import Path

import serial

SOF = 0xAA
CMD_METADATA = 0x01
CMD_DATA = 0x02

APP_MAX_SIZE = 917504
CHUNK_SIZE = 252
MAX_RETRIES = 3
ACK_TIMEOUT = 5.0


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


def build_packet(command: int, payload: bytes) -> bytes:
    if len(payload) > 256:
        raise ValueError("Payload exceeds 256 bytes")

    body = struct.pack("<BH", command, len(payload)) + payload
    crc = crc16_ccitt_false(body)

    return bytes([SOF]) + body + struct.pack("<H", crc)


def read_response(ser: serial.Serial) -> str:
    response = ser.readline().decode("ascii", errors="replace").strip()
    print(f"STM32: {response}")
    return response


def send_packet_with_retry(
    ser: serial.Serial,
    packet: bytes,
    expected_response: str,
    description: str,
) -> bool:
    for attempt in range(1, MAX_RETRIES + 1):
        print(f"{description} (attempt {attempt}/{MAX_RETRIES})")

        ser.write(packet)
        ser.flush()

        response = read_response(ser)

        if response == expected_response:
            return True

        if response.startswith("NAK"):
            print("Packet rejected by STM32.")
        elif not response:
            print("Response timeout; retrying packet.")
        else:
            print(f"Unexpected response: {response!r}")

        time.sleep(0.1)

    return False


def upload_firmware(
    firmware_path: Path,
    port: str,
    baudrate: int,
) -> None:
    if not firmware_path.is_file():
        raise FileNotFoundError(f"File not found: {firmware_path}")

    firmware = firmware_path.read_bytes()
    image_size = len(firmware)
    image_crc32 = zlib.crc32(firmware) & 0xFFFFFFFF

    if image_size == 0:
        raise ValueError("Firmware file is empty")

    if image_size > APP_MAX_SIZE:
        raise ValueError(
            f"Image is {image_size} bytes; maximum is {APP_MAX_SIZE}"
        )

    print(f"Firmware : {firmware_path}")
    print(f"Size     : {image_size} bytes")
    print(f"CRC-32   : 0x{image_crc32:08X}")
    print(f"Port     : {port} at {baudrate} baud")

    # Check the CRC implementation against the standard test vector.
    assert crc16_ccitt_false(b"123456789") == 0x29B1

    with serial.Serial(
        port=port,
        baudrate=baudrate,
        timeout=ACK_TIMEOUT,
        write_timeout=3,
    ) as ser:
        time.sleep(0.2)
        ser.reset_input_buffer()

        # 1. Request the switch from text mode to metadata mode.
        print("\nStarting update...")
        ser.write(b"BEGIN\n")
        ser.flush()

        if read_response(ser) != "OK":
            raise RuntimeError("STM32 did not accept BEGIN")

        # 2. Send metadata: image size + expected image CRC-32.
        metadata = struct.pack("<II", image_size, image_crc32)
        metadata_packet = build_packet(CMD_METADATA, metadata)
        print("MetaData Packet sent",metadata_packet);
        if not send_packet_with_retry(
            ser, metadata_packet, "READY", "Sending metadata"
        ):
            raise RuntimeError(
                "Metadata was not accepted. "
                "Check STM32 state and response handling."
            )

        # 3. Send DATA packets, waiting for ACK after each.
        print("\nSending firmware data...")

        offset = 0

        while offset < image_size:
            chunk = firmware[offset : offset + CHUNK_SIZE]

            # DATA payload: 4-byte little-endian offset + firmware bytes.
            payload = struct.pack("<I", offset) + chunk
            packet = build_packet(CMD_DATA, payload)
            print("Packet sent",packet);
            description = (
                f"DATA offset={offset}, length={len(chunk)}"
            )

            if not send_packet_with_retry(
                ser, packet, "ACK", description
            ):
                raise RuntimeError(
                    f"DATA transfer failed at offset {offset}. "
                    "Do not reboot into the application; "
                    "the image is not yet verified."
                )

            offset += len(chunk)

            percent = 100.0 * offset / image_size
            print(f"Progress: {offset}/{image_size} bytes ({percent:.1f}%)")

        print("\nAll DATA packets acknowledged.")
        print("WARNING: END/CRC-32 verification is not implemented yet.")
        print("The application must not be considered verified or bootable.")


def main():
    parser = argparse.ArgumentParser(
        description="Upload a binary image to the STM32F407 bootloader."
    )
    parser.add_argument("firmware", type=Path, help="Path to .bin file")
    parser.add_argument("--port", default="COM5")
    parser.add_argument("--baud", type=int, default=9600)
    args = parser.parse_args()

    try:
        upload_firmware(args.firmware, args.port, args.baud)
    except (OSError, ValueError, serial.SerialException, RuntimeError) as exc:
        print(f"\nUPLOAD FAILED: {exc}")


if __name__ == "__main__":
    main()
