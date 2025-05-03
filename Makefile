# Target microcontroller
MCU = atmega328p

# Target clock frequency
F_CPU = 16000000UL

# Paths
SRC_DIR = src
BUILD_DIR = build
FLASH_DIR = flash
INCLUDE = include

# Tools
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# Flags
CFLAGS = -mmcu=$(MCU) -Wall -DF_CPU=$(F_CPU) -Os -std=gnu11 -I$(INCLUDE)

# Port for avrdude (change if needed)
PORT = /dev/ttyACM0
BAUD = 115200

# File names
SRC = $(SRC_DIR)/main.c
OBJ = $(BUILD_DIR)/main.o
ELF = $(BUILD_DIR)/main.elf
HEX = $(FLASH_DIR)/main.hex

# Default target (build everything)
all: $(HEX)

# .c file into .o
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Link .o to .elf
$(ELF): $(OBJ)
	$(CC) -mmcu=$(MCU) $^ -o $@

# Convert .elf to .hex
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Flash the .hex to the Arduino
flash: $(HEX)
	$(AVRDUDE) -p $(MCU) -c arduino -P $(PORT) -b $(BAUD) -U flash:w:$(HEX)

# Clean up build files
clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.elf $(FLASH_DIR)/*.hex

dump:
	avr-objdump -S -m avr build/main.elf > build/main.lst


# Phony targets
.PHONY: all flash clean
