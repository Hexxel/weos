# Readme for dagger-spl

## Dagger platform boot sequence

This is an overview of how the dagger platform is booted.
Because of transaction collisions on the SPI bus the Dagger
platform is booted with a first-stage minimal bootloader that
sets up the dragonite core and uses it to load the second
stage bootloader from a secondary FLASH. This project contains
the first stage bootloader and code for the dragonite.

## How to use this project to load a barebox via JTAG

To be able to access the secondary FLASH the dragonite must be
initialised. The first stage bootloader performs this tasks.
After it has done the first setup we can halt the SOC and ramload
the desired bootloader to RAM and jump to it.

## Prerequisites

1. The make script must have console access to both the dagger board
and the JTAG through conserver.
2. The Makefile must be edited with the name of the consoles:
  * **BDI-CONSOLE** must point to the JTAG console.
  * **CON-CONSOLE** must point to the Dagger console.
3. In the **MVHeaderfile** file change arg3 to '0' so that the first stage
bootloader doesn't execute the bootloader from FLASH.

## Booting Dagger via JTAG
1. Do a 'make clean all'
2. Do a 'make test'
3. The make script should load this project through the UART. When it stops
after *'Loading bootloader'* go to the BDI-CONSOLE and do a 'halt' command
4. Still in the BDI-CONSOLE do a 'load 0 barebox-dagger.bin BIN' to load
the bootloader to RAM @ 0.
5. Execute is by 'go 0'. Is should now boot with the secondary FLASH
accessible.


## Booting Dagger via UART

The Dagger can also be bootstrapped via UART. Use the following
steps to bootstrap via UART0:

1. The consoles must be handled via conserver. This setup is not handled in this
document.
2. Change to **BDI-CONSOLE** and **CON-CONSOLE** in the westermo/Makefile
so that they match your setup.
3. Do a 'make'
4. Do a 'make test'

The Dagger should load the UART boot-image via the **CON-CONSOLE**.

## Files produced

1. dagger-pbl-spi.bin - The first stage bootloader
2. dagger-pbl-uart0.bin - Barebox image with headers to be used whe booting via UART.
3. images/start-dagger.pblx - Second stage bootloader with barebox image.
