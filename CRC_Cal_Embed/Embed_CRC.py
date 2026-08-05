import os
import struct
import sys

def embed_binary_crc(file_path: str):
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        return

    with open(file_path, "rb") as f:
        file_bytes = bytearray(f.read())

    print(f"Original File Size: {len(file_bytes)} bytes")

    # 1. Ask the user if this file already has a 4-byte CRC placeholder/old CRC
    # If it is a raw compilation, we keep all bytes. If it's a re-run, we strip the last 4.
    if len(file_bytes) >= 4:
        choice = input("Does this file already contain an old/placeholder 4-byte CRC at the end? (y/N): ").strip().lower()
        if choice == 'y':
            app_bytes = file_bytes[:-4]
            print("-> Stripped the existing 4-byte CRC trailing placeholder.")
        else:
            app_bytes = file_bytes
    else:
        app_bytes = file_bytes

    print(f"Application Payload Size: {len(app_bytes)} bytes")

    # 2. Handle Hardware Padding (Round application bytes up to 4-byte boundaries)
    remainder = len(app_bytes) % 4
    if remainder != 0:
        padding_size = 4 - remainder
        app_bytes.extend(b'\x00' * padding_size)
        print(f"-> Applied {padding_size} bytes of zero-padding for 32-bit hardware alignment.")

    # 3. Calculate the STM32F411 Hardware CRC32 (Matches your checker logic)
    POLYNOMIAL = 0x04C11DB7
    calculated_crc = 0xFFFFFFFF

    for i in range(0, len(app_bytes), 4):
        word = struct.unpack('<I', app_bytes[i:i+4])[0]
        calculated_crc ^= word
        for _ in range(32):
            if calculated_crc & 0x80000000:
                calculated_crc = ((calculated_crc << 1) ^ POLYNOMIAL) & 0xFFFFFFFF
            else:
                calculated_crc = (calculated_crc << 1) & 0xFFFFFFFF

    print(f"Calculated Hardware CRC to embed: 0x{calculated_crc:08X}")

    # 4. Pack the calculated CRC as 4 bytes (Little-Endian)
    crc_bytes = struct.pack('<I', calculated_crc)

    # 5. Append the CRC to the padded application payload
    final_output_bytes = app_bytes + crc_bytes

    # 6. Overwrite the file with the newly embedded data
    with open(file_path, "wb") as f:
        f.write(final_output_bytes)

    print(f"Success! New file size: {len(final_output_bytes)} bytes (Payload + 4-byte CRC).")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python embed_crc.py <path_to_binary.bin>")
        sys.exit(1)
        
    input_file_path = sys.argv[1]
    embed_binary_crc(input_file_path)
