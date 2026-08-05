# stm32f4_bootloader

This is a lightweight, custom bootloader for STM32F4 microcontrollers that allows firmware updates directly from a FAT32-formatted USB flash drive.

## Features

- **USB Host FAT32 Support:** Reads and loads `image.bin` application file straight from a standard USB drive.
- **Hardware Trigger:** Activated safely on startup by holding a user push button.
- **CRC Verification:** Performs a strict CRC-32 integrity check on the flashed binary to ensure safe execution.
- **Automatic Jump:** Boots smoothly into the main user application if no update request is found or after a successful flash.

---

## How It Works

1. **Trigger:** The bootloader will be triggered and detect a FAT32 USB drive when the user button is pressed and hold while resetting or powering on the board.
2. **Flash:** The bootloader mounts the USB drive, locates the binary file called `image.bin`, erases the target flash sectors, and writes the new code.
3. **Verify:** A CRC-32 check runs automatically across the newly written flash region.
4. **Run:** If verification passes, the system jumps to the application vector table.

#### Note:

The CRC-32 checksum of the target BIN file must be calculated and embedded into the last 4 bytes of the file in little-endian format. To do that, follow the instructions below:

1. Adjust the memory map / linker script in target application project to account for the bootloader size offset (or the `APPLICATION_ADDRESS` defined in the `flash_program.h`. This varies from one MCU to another).
2. Copy the newly compiled target BIN file into the CRC_Cal_Embed folder and rename it to `image.bin`.
3. Navigate into CRC_Cal_Embed folder in command prompt and type "python Embed_CRC.py image.bin".
4. Type 'y' if this is the first time the CRC-32 of the BIN file is being inserted into the file.
5. Type "python Cal_CRC.py image.bin" to verify that the CRC-32 has been correctly calculated and added to the target BIN file.

---

## Directory Layout

    stm32f4_bootloader
    ├── Core
    |   ├── Src                          # Main source files
    │   └── Inc                          # Main header files
    ├── CRC_Cal_Embed                    # Python scripts to embed/verify CRC-32 into/on target bin file
    ├── Drivers
    │   ├── CMSIS                        # CMSIS drivers
    │   └── STM32F4xx_HAL_DRIVER         # STM32F4 HAL drivers
    ├── FATFS                            # FAT filesystem application files
    ├── Middlewares                      # USB Host library
    └── USB_HOST                        # USB Host application files

---

## Required Tools

For Windows users, MSYS2 is required: https://www.msys2.org

Once MSYS2 is installed, install the follwoing packages in command prompt:

1. GCC Arm Compiler: pacman -S mingw-w64-x86_64-arm-none-eabi-gcc
2. OpenOCD: pacman -S mingw-w64-x86_64-openocd
3. GNU Debugger(GDB): pacman -S mingw-w64-x86_64-gdb-multiarch

Once the packages above are installed, add this to your System PATH: C:\msys64\mingw64\bin

#### Note:

The example project in this repository was developed solely using OpenOCD, but it can be migrated to any IDE, such as Keil MDK or STM32CubeIDE, if necessary. It was also tested using an STM32F411E-DISCO discovery kit (https://www.st.com/en/evaluation-tools/32f411ediscovery.html) but can be ported to other STM32F4 development boards. ST-LINK was used as the programmer and debugger so it's necessary to install its USB driver (STSW-LINK009): https://www.st.com/en/development-tools/stsw-link009.html.

### Building and Debugging

To build and program the code into your target:

1. Navigate into the stm32f4_bootloader folder in command prompt and type "make" to build the code. An Executable and Linkable Format file (.elf) file will be generated inside a folder named "build".
2. Type "make program" to program your target.

For debugging:

1. Type "make debug".
2. Open a new command prompt and navigate it into the stm32f4_bootloader folder and type "make start_debug".
3. Once the GNU Debugger is invoked, type this to connect it to the OpenOCD: target extended-remote:3333

For more information on OpenOCD and GDB, please refer to: https://openocd.org/pages/documentation.html and https://www.sourceware.org/gdb/.

To clean up all the generated files during compilation, type "make clean".
