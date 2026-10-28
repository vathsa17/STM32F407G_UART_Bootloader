# STM32F407 UART Bootloader

A register-level C bootloader for the STM32F407G development board. The bootloader receives firmware over USART2 from a Python host, programs the application into internal Flash, and jumps to the application.

> **Current milestone:** The bootloader has been tested end-to-end: a `.bin` application can be transferred to the STM32F407, programmed into the application Flash region, and executed through the bootloader's application-jump path. Final whole-image CRC-32 verification and an explicit `END` command are not yet implemented, so the current transfer should not yet be treated as fully production-safe.

## Contents

- [Features and current status](#features-and-current-status)
- [Hardware and software](#hardware-and-software)
- [Project layout](#project-layout)
- [Flash memory layout](#flash-memory-layout)
- [Application build configuration](#application-build-configuration)
- [Build and upload](#build-and-upload)
- [Boot flow](#boot-flow)
- [UART configuration and ring buffer](#uart-configuration-and-ring-buffer)
- [Text commands](#text-commands)
- [Binary packet protocol](#binary-packet-protocol)
- [Metadata packet](#metadata-packet)
- [Firmware DATA packet](#firmware-data-packet)
- [CRC algorithms](#crc-algorithms)
- [Python host uploader](#python-host-uploader)
- [Application jump and vector table](#application-jump-and-vector-table)
- [Debugging notes](#debugging-notes)
- [Known limitations and next steps](#known-limitations-and-next-steps)
- [Safety notes](#safety-notes)

## Features and current status

### Implemented and tested

- Register-level C implementation in STM32CubeIDE.
- USART2 UART communication with a Python host using PySerial.
- Interrupt-driven UART receive and ring-buffered byte processing.
- Text command parser.
- `BEGIN` transition from text mode to binary metadata mode.
- Binary packet parser with SOF, command, length, payload and CRC fields.
- CRC-16/CCITT-FALSE validation for binary packets.
- Metadata containing firmware size and expected firmware CRC-32.
- Application Flash erase covering sectors 5–11, leaving bootloader sectors 0–4 untouched.
- DATA packet transfer using 252-byte firmware chunks.
- Application programming to the Flash region beginning at `0x08020000`.
- Application vector-table validation and jump, including `SCB->VTOR` relocation.
- Tested generation and upload of a raw `.bin` application image.
- Confirmed that the programmed application can be launched from the bootloader.

### Not yet complete / needs hardening

- An explicit `END` command to finish a transfer.
- CRC-32 calculation over the programmed Flash image and comparison with metadata.
- A persistent validity marker so only a fully verified image is booted.
- Fully robust retry and duplicate-DATA handling; confirm this in the implementation before relying on retransmissions.
- Recovery policy for power loss or reset during an update.
- A finalized binary ACK/NAK format. The current host script uses text responses such as `READY\r\n` and `ACK\r\n`.

## Hardware and software

- **MCU:** STM32F407G development board.
- **Language:** C, using register-level peripheral drivers.
- **IDE/toolchain:** STM32CubeIDE and Arm GNU Toolchain.
- **Transport:** UART using USART2.
- **Host:** Python 3 and PySerial.
- **Current Python defaults:** `COM5`, `9600` baud.

The UART pin configuration in the firmware and the physical wiring must match the board. The host and target must use the same baud rate and serial settings. The firmware currently configures 8 data bits, no parity, and one stop bit (8-N-1).

Install the Python dependency:

```bash
python -m pip install pyserial
```

## Project layout

The precise filenames can differ depending on how the STM32CubeIDE project is organized. The main logical components are:

- `main.c` — initialization, main processing loop, bootloader mode switching and USART2 interrupt handler.
- UART driver files — USART2 initialization and transmission.
- Ring-buffer files — interrupt-to-main-loop byte queue.
- `boot_protocol.c/.h` — binary frame parser, packet structure and CRC-16 validation.
- `bootloader.c/.h` — command handling, metadata processing, DATA handling and application jump.
- `flash_driver.c/.h` — application Flash erase and programming.
- `upload_firmware.py` — Python host-side firmware uploader.
- Application project — for example, an LED blinker linked to run at the application address.

Use the actual filenames from your repository if they differ from these logical module names.

## Flash memory layout

The first 128 KiB of Flash is reserved for the bootloader. The application starts at `0x08020000`.

| Region | Start address | End address (exclusive) | Size |
|---|---:|---:|---:|
| Bootloader | `0x08000000` | `0x08020000` | 128 KiB |
| Application | `0x08020000` | `0x08100000` | 896 KiB |
| Maximum application image | — | — | 917,504 bytes |

Relevant definitions:

```c
#define APP_FLASH_START 0x08020000UL
#define APP_FLASH_END   0x08100000UL
#define APP_MAX_SIZE    (APP_FLASH_END - APP_FLASH_START)
```

On the STM32F407 Flash layout used by this project, sectors 0–4 are reserved for the bootloader and sectors 5–11 are used for the application. The application erase routine must not erase sectors 0–4.

`APP_FLASH_END` is an exclusive bound. A valid image must fit entirely within:

```text
[0x08020000, 0x08100000)
```

## Application build configuration

The application must be **linked for `0x08020000`**, not for the bootloader address `0x08000000`. In STM32CubeIDE, configure the application project's linker/memory settings so its Flash origin and vector table match `0x08020000`.

### Generate a `.bin` file in STM32CubeIDE

1. Right-click the **application project** (not the bootloader project).
2. Select **Properties**.
3. Open **C/C++ Build → Settings**.
4. Find **MCU/MPU Post build outputs**.
5. Enable **Convert to binary file (`-O binary`)**.
6. Apply the settings and rebuild the application.

The `.bin` is normally generated beside the `.elf`, in the build configuration directory such as `Debug/` or `Release/`.

### Convert an ELF manually

If automatic binary generation is not enabled, use:

```bash
arm-none-eabi-objcopy -O binary Debug/YourApplication.elf Debug/YourApplication.bin
```

Replace `YourApplication` and the paths with the actual project and build output names.

**Upload the `.bin`, not the `.elf`.** An ELF file starts with an ELF header (`7F 45 4C 46`); those bytes are not the application's initial stack pointer. Uploading an ELF as if it were a raw image causes the bootloader to read invalid vector-table values.

You can inspect the first bytes with Python:

```python
from pathlib import Path

image = Path("Debug/YourApplication.bin").read_bytes()
print("Image size:", len(image))
print("First 8 bytes:", image[:8].hex(" ").upper())
```

The first four bytes of a correctly linked STM32 application image should encode an initial stack pointer in SRAM, and the next four bytes should encode the reset-handler address.

## Build and upload

Build the bootloader and flash it to the board using STM32CubeIDE. Build the application separately, with its linker configured for `0x08020000`, and generate its `.bin` file.

Example uploader command:

```bash
python upload_firmware.py Debug/YourApplication.bin --port COM5 --baud 9600
```

The script defaults to `COM5` and `9600` baud, so the shorter command also works if those defaults match your setup:

```bash
python upload_firmware.py Debug/YourApplication.bin
```

Close any other application that is using the serial port before starting the uploader.

## Boot flow

The intended flow is:

1. The MCU resets and begins executing the bootloader from `0x08000000`.
2. The bootloader initializes USART2, the receive ring buffer and other required peripherals.
3. The bootloader accepts text commands.
4. The host sends `BEGIN\n`.
5. The bootloader responds `OK\r\n` and switches to binary metadata mode.
6. The host sends a metadata packet with image size and expected image CRC-32.
7. The bootloader parses the packet and validates its CRC-16 and metadata fields.
8. The bootloader prepares the application region for the new image, including erasing sectors 5–11.
9. The bootloader responds `READY\r\n`.
10. The host sends DATA packets in order, waiting for `ACK\r\n` after each accepted packet.
11. The bootloader writes the received image bytes into application Flash.
12. The bootloader can jump to the programmed application when instructed and when its vector table passes the implemented checks.

**Planned completion flow:** Add an `END` command, verify that the exact declared number of bytes was programmed, calculate CRC-32 over the programmed image, and only then mark the application valid and permit normal boot. That final verification/validity flow is not yet implemented.

## UART configuration and ring buffer

The current configuration is:

- Serial port on host: `COM5` by default.
- Baud rate: `9600`.
- UART framing: 8 data bits, no parity, 1 stop bit.
- Text command terminator: LF (`\n`). The text parser ignores CR (`\r`) and processes the command at LF.
- Binary data: raw bytes; do not encode it as text or apply newline conversion.

USART2's RX interrupt reads the received byte from the USART data register and pushes it into a ring buffer. The main loop removes bytes from the ring buffer and routes them to the text parser or binary protocol parser according to the current bootloader mode.

The ring buffer uses one empty slot to distinguish full from empty, so its usable capacity is `BUFFER_SIZE - 1`. A ring-buffer overflow means at least one received byte was dropped and the current frame/transfer should be considered unreliable.

During the metadata-to-firmware mode transition, the implementation resets the ring buffer and protocol parser before sending `READY`. This is safe with the current host sequence because the Python script waits for `READY` before sending DATA. Do not reset the ring buffer during an active frame or while host data that must be preserved may already be queued.

Do not transmit debug text on the same UART while the Python uploader is expecting `READY`, `ACK`, or `NAK`; debug output is indistinguishable from protocol responses to the host. Use debugger breakpoints or a RAM trace instead. Be aware that breakpoints can cause UART overrun if the host continues sending while the CPU is halted.

## Text commands

The text command processor supports these commands; the exact informational text depends on the firmware implementation.

| Command | Purpose |
|---|---|
| `PING` | Check that the bootloader responds. |
| `INFO` | Display bootloader/device information. |
| `HELP` | List supported commands. |
| `BOOT` | Validate the application vector table and jump to the application. |
| `ERASE` | Erase the application Flash region. This destroys the application. |
| `FLASHTEST` | Exercise the Flash test path. |
| `BEGIN` | Start an update and switch from text mode to binary metadata mode. |

Send commands as ASCII text followed by LF. Example:

```text
BEGIN\n
```

The expected response for the mode transition is:

```text
OK\r\n
```

Once the bootloader switches to binary mode, do not send text commands until the firmware explicitly returns to text mode.

## Binary packet protocol

### Frame format

Every binary frame has the following layout:

| Field | Size | Description |
|---|---:|---|
| SOF | 1 byte | Start-of-frame marker, `0xAA` |
| CMD | 1 byte | Command identifier |
| LEN | 2 bytes | Payload length, unsigned 16-bit little-endian |
| PAYLOAD | 0–256 bytes | Command-specific payload |
| CRC-16 | 2 bytes | CRC-16/CCITT-FALSE, little-endian |

Total frame size:

```text
1 + 1 + 2 + payload_length + 2 bytes
```

The CRC-16 covers `CMD + LEN_LSB + LEN_MSB + PAYLOAD`. It excludes SOF and the two CRC bytes.

The current packet structure is:

```c
typedef struct
{
    uint8_t  command;
    uint16_t length;
    uint8_t  payload[PROTOCOL_MAX_PAYLOAD];
    uint16_t crc;
} BootPacket_t;

#define PROTOCOL_SOF         0xAAU
#define PROTOCOL_MAX_PAYLOAD 256U
```

`Protocol_ProcessByte()` consumes one byte at a time. It returns a status indicating an incomplete frame, a completed packet, or an invalid frame. After `PROTOCOL_PACKET_READY`, `Protocol_GetPacket()` returns the assembled packet and the caller should validate the CRC before processing the packet.

The returned pointer refers to a static packet buffer. Process the completed packet before the parser assembles another packet.

### Command IDs

| Command | ID | Payload |
|---|---:|---|
| `CMD_METADATA` | `0x01` | 8-byte metadata payload |
| `CMD_DATA` | `0x02` | 4-byte image offset followed by firmware data |
| `CMD_END` | Not finalized | Planned end-of-transfer verification |
| ACK/NAK | Text response currently used | `READY\r\n`, `ACK\r\n`, and error text; binary response IDs are not finalized |

The C and Python implementations must use the same command IDs, lengths, byte order and CRC rules.

### Parser states

The parser uses these states:

- `RX_WAIT_SOF`
- `RX_COMMAND`
- `RX_LENGTH_LOW`
- `RX_LENGTH_HIGH`
- `RX_PAYLOAD`
- `RX_CRC_LOW`
- `RX_CRC_HIGH`

The parser must treat a byte value of `0xAA` according to the current state. For example, `0xAA` can legitimately occur inside payload data or as a CRC byte. It is a new SOF only when the parser is in `RX_WAIT_SOF`.

## Metadata packet

The metadata payload is exactly 8 bytes:

| Payload offset | Size | Field | Encoding |
|---:|---:|---|---|
| 0 | 4 bytes | Firmware image size | Unsigned 32-bit little-endian |
| 4 | 4 bytes | Expected firmware CRC-32 | Unsigned 32-bit little-endian |

Python calculates the values from the `.bin` file:

```python
image_size = len(firmware)
image_crc32 = zlib.crc32(firmware) & 0xFFFFFFFF
metadata = struct.pack("<II", image_size, image_crc32)
metadata_packet = build_packet(CMD_METADATA, metadata)
```

The image size must be nonzero and no larger than `APP_MAX_SIZE` (917,504 bytes).

Example metadata packet from the current test:

```text
AA 01 08 00 18 31 00 00 53 9C 14 85 AA 69
```

Decoded:

| Bytes | Meaning |
|---|---|
| `AA` | SOF |
| `01` | Metadata command |
| `08 00` | Payload length: 8 |
| `18 31 00 00` | Image size: 12,568 bytes (`0x00003118`) |
| `53 9C 14 85` | Image CRC-32: `0x85149C53` |
| `AA 69` | Packet CRC-16: `0x69AA`, low byte first |

The `0xAA` in the CRC (`AA 69`) is **not** another SOF. The parser is in `RX_CRC_LOW` when it receives that byte and must consume it as CRC data.

The metadata packet CRC-16 protects the metadata frame. The metadata's CRC-32 is intended to validate the complete firmware image after programming; final target-side CRC-32 verification remains to be implemented.

## Firmware DATA packet

The maximum protocol payload is 256 bytes. A DATA payload uses four bytes for the image offset, leaving up to **252 bytes of firmware data**.

| Payload offset | Size | Field |
|---:|---:|---|
| 0 | 4 bytes | Image offset, unsigned 32-bit little-endian |
| 4 | 1–252 bytes | Firmware data |

Python constructs each DATA packet as follows:

```python
chunk = firmware[offset : offset + CHUNK_SIZE]
payload = struct.pack("<I", offset) + chunk
packet = build_packet(CMD_DATA, payload)
```

Current Python constants:

```python
CMD_DATA = 0x02
CHUNK_SIZE = 252
```

The destination Flash address for a packet is:

```c
destinationAddress = APP_FLASH_START + offset;
```

For full chunks, the offsets are `0`, `252`, `504`, `756`, and so on. The final packet may contain fewer than 252 bytes.

Before programming, the target should verify:

- Packet CRC-16 is valid.
- The transfer is active and the command is `CMD_DATA`.
- Payload length is greater than four bytes and no greater than 256.
- Offset and data length remain within the declared image size and application Flash region.
- Offset is the expected next offset, or the packet is an explicitly recognized duplicate retry.
- Flash programming succeeds before advancing the expected offset or acknowledging the packet.

The full DATA packet with 252 image bytes is 262 bytes total: 1 SOF + 1 command + 2 length + 256 payload + 2 CRC.

For the first packet, assuming offset zero and a `.bin` that starts with the ELF-independent application vector table, the first eight frame bytes are:

```text
AA 02 00 01 00 00 00 00
```

`LEN = 256` is transmitted little-endian as `00 01`.

## CRC algorithms

### Packet CRC-16/CCITT-FALSE

| Parameter | Value |
|---|---|
| Polynomial | `0x1021` |
| Initial value | `0xFFFF` |
| Input reflection | No |
| Output reflection | No |
| Final XOR | `0x0000` |
| Check value for ASCII `123456789` | `0x29B1` |

Python implementation:

```python
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
```

The 16-bit CRC is transmitted low byte first:

```python
struct.pack("<H", crc)
```

### Firmware CRC-32

Python uses the standard CRC-32 algorithm provided by `zlib`:

```python
image_crc32 = zlib.crc32(firmware) & 0xFFFFFFFF
```

The 32-bit value is included in metadata as little-endian:

```python
struct.pack("<II", image_size, image_crc32)
```

The STM32 must eventually calculate the matching CRC-32 over exactly `image_size` bytes starting at `APP_FLASH_START`. It must not include unused Flash bytes after the image.

## Python host uploader

The current uploader uses `pyserial`, reads the `.bin` file, computes image size and CRC-32, and sends metadata followed by DATA chunks.

Its expected sequence is:

1. Open the serial port.
2. Send `BEGIN\n`.
3. Expect `OK`.
4. Send the metadata frame; expect `READY`.
5. Send each DATA frame; expect `ACK`.
6. Stop with an error if the expected response is not received after the configured retries.

The current Python script uses:

```python
SOF = 0xAA
CMD_METADATA = 0x01
CMD_DATA = 0x02

APP_MAX_SIZE = 917504
CHUNK_SIZE = 252
MAX_RETRIES = 3
ACK_TIMEOUT = 5.0
```

It validates its CRC-16 implementation with:

```python
assert crc16_ccitt_false(b"123456789") == 0x29B1
```

### Response handling

The host reads line-oriented responses with `ser.readline()`. The target must send the expected response lines consistently:

- `OK\r\n` after `BEGIN`.
- `READY\r\n` after valid metadata and successful application-region preparation.
- `ACK\r\n` after a DATA packet has been successfully handled.
- A documented `NAK...` response on failure.

Do not transmit diagnostic messages on USART2 during upload. A debug line can be consumed by `read_response()` instead of the expected response and can trigger an unnecessary retry. Use the debugger or RAM traces for diagnostics.

**Important:** The current script does not send an `END` packet and explicitly warns that final CRC-32 verification is not implemented. It reports that all DATA packets were acknowledged, not that the firmware image has been cryptographically or end-to-end verified.

## Application jump and vector table

The application's vector table must begin at `APP_FLASH_START` (`0x08020000`). The first two 32-bit words are:

| Address | Contents |
|---|---|
| `APP_FLASH_START` | Initial Main Stack Pointer (MSP) |
| `APP_FLASH_START + 4` | Reset-handler address |

The bootloader validates that the initial stack pointer is within the configured SRAM range and suitably aligned. For this project, the SRAM1 bounds are:

```c
#define SRAM1_START 0x20000000UL
#define SRAM1_END   0x20020000UL
```

`SRAM1_END` is the upper boundary; `0x20020000` can be a valid initial stack pointer because the stack begins at the top of RAM and grows downward.

Before jumping, the bootloader relocates the vector table:

```c
SCB->VTOR = APP_FLASH_START;
__DSB();
__ISB();
```

The jump routine must also set the MSP to the application's initial stack pointer and branch to the reset handler using the correct Thumb-state address. Validate the reset-handler address as well as the stack pointer before jumping.

## Debugging notes

### If the stack pointer looks invalid

If the inspected `appStackPointer` is `0x464C457F`, the uploaded file likely starts with the ELF magic bytes `7F 45 4C 46`. Generate and upload a raw `.bin` file, not the `.elf`.

### If the parser sees an unexpected `0xAA`

`0xAA` may appear as SOF, payload data, or a CRC byte. In the example metadata packet, the CRC-16 low byte is `0xAA`, so two occurrences of `0xAA` in the complete metadata frame are valid.

Use a RAM trace to record `(state_before_processing, byte)` without transmitting debug text over the protocol UART. Confirm that each ring-buffer byte is passed to `Protocol_ProcessByte()` exactly once and that only the protocol module changes its parser state.

### If the host receives an unexpected response

Remove UART debug prints while uploading. The Python host expects one line response (`OK`, `READY`, or `ACK`) at each stage. Any extra line can desynchronize host response handling.

### If a transfer fails

Inspect `RingBufferOverflow` and any USART overrun/framing/noise/parity error counters. A ring-buffer overflow means bytes were dropped. Do not consider the current image valid after a failed or interrupted transfer.

## Known limitations and next steps

Recommended next steps, in order:

1. **Implement `END`:** host sends it after all DATA packets have been acknowledged.
2. **Verify final image CRC-32:** compute CRC-32 over the programmed Flash bytes and compare it with metadata.
3. **Track image validity:** only allow automatic boot or `BOOT` after successful final verification; define how validity is stored across reset.
4. **Harden retransmission:** recognize an exact duplicate of the last successfully programmed DATA packet and ACK it without programming twice. Reject mismatched duplicates and unexpected offsets.
5. **Improve error responses:** define stable error codes and consistent ACK/NAK handling.
6. **Handle interrupted updates:** remain in the bootloader if the application is absent or unverified; consider a validity marker or dual-slot design if recovery requirements justify it.
7. **Add timeout and abort behavior:** recover cleanly when the host disconnects or stops sending.
8. **Test boundary cases:** minimum-size image, maximum-size image, final partial chunk, bad CRC, wrong offset, duplicate packet, Flash failure, UART overflow and reset during transfer.

## Safety notes

- Never erase or program the bootloader region (`0x08000000`–`0x0801FFFF`) during an application update.
- Validate packet CRC-16 before acting on packet payloads.
- Validate image size, offset, payload length and destination bounds before Flash access.
- Do not treat ACKs alone as proof that the entire image is valid.
- Do not mark an image bootable until final CRC-32 verification succeeds.
- An interrupted update after erasing the application can leave no working application. Keep the bootloader independently recoverable over UART.
- Ensure the application is linked for `0x08020000`; generating a `.bin` does not relocate a project linked for `0x08000000`.
- Keep command IDs, payload sizes, byte order, CRC algorithms and response strings synchronized between C and Python.
