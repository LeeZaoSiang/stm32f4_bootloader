import os
import struct
import sys

def verify_binary_crc(file_path: str):
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        return

    with open(file_path, "rb") as f:
        file_bytes = bytearray(f.read())

    # 1. Ensure file is large enough to contain a CRC
    if len(file_bytes) < 4:
        print("Error: File size is less than 4 bytes. Cannot verify CRC.")
        return

    # 2. Extract the embedded CRC from the last 4 bytes (Little-Endian)
    embedded_crc_bytes = file_bytes[-4:]
    embedded_crc = struct.unpack('<I', embedded_crc_bytes)[0]
    
    # 3. Strip the 4-byte checksum to isolate the actual application data
    app_bytes = file_bytes[:-4]
    
    print(f"Total File Size: {len(file_bytes)} bytes")
    print(f"Application Size: {len(app_bytes)} bytes")
    print(f"Embedded CRC in file: 0x{embedded_crc:08X}")

    # 4. Handle Hardware Padding (Round application bytes up to 4-byte boundaries)
    remainder = len(app_bytes) % 4
    if remainder != 0:
        padding_size = 4 - remainder
        app_bytes.extend(b'\x00' * padding_size)
        print(f"-> Applied {padding_size} bytes of zero-padding for 32-bit hardware alignment.")

    # 5. Calculate the expected STM32F411 Hardware CRC32
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

    print(f"Calculated Hardware CRC: 0x{calculated_crc:08X}")

    # 6. Final verification output
    if calculated_crc == embedded_crc:
        print("\n✅ VERIFICATION SUCCESS: The embedded CRC matches the data payload!")
    else:
        print("\n❌ VERIFICATION FAILURE: The embedded CRC does NOT match.")


if __name__ == "__main__":
    # Check if the user forgot to provide a filename argument
    if len(sys.argv) < 2:
        print("Usage: python check_crc.py <path_to_binary.bin>")
        sys.exit(1)
        
    # Read the explicit binary file path provided via your command line input
    input_file_path = sys.argv[1]
    
    # Run the verification function using the correct variable
    verify_binary_crc(input_file_path)