# Target microcontroller
MCU = atmega328p

# Target clock frequency
F_CPU = 16000000UL

# Paths
SRC_DIR = src
BUILD_DIR = build
FLASH_DIR = flash
INCLUDE = include
UWIRE_DIR = uWire
SERIAL_DIR = serial

# Tools
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# Flags
CFLAGS = -mmcu=$(MCU) -Wall -DF_CPU=$(F_CPU) -Os -std=gnu11 -I$(INCLUDE)\
 -I$(UWIRE_DIR) -I$(SERIAL_DIR)

# Port for avrdude (change if needed)
PORT = /dev/ttyACM0
BAUD = 115200

# File names
SRC = $(SRC_DIR)/main.c 
UWIRE_SRC = $(UWIRE_DIR)/uWire.c
SERIAL_SRC = $(SERIAL_DIR)/serial.c
OBJ = $(BUILD_DIR)/main.o
UWIRE_OBJ = $(BUILD_DIR)/uWire.o
SERIAL_OBJ = $(BUILD_DIR)/serial.o
PRJ_DUMP = $(BUILD_DIR)/prj.lst

ELF = $(BUILD_DIR)/prj.elf
HEX = $(FLASH_DIR)/prj.hex

# Default target (build everything)
all: $(HEX)

# .c file into .o
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(UWIRE_OBJ): $(UWIRE_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERIAL_OBJ): $(SERIAL_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Link .o to .elf
$(ELF): $(OBJ) $(UWIRE_OBJ) $(SERIAL_OBJ)
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
	avr-objdump -S -m avr $(ELF) > $(PRJ_DUMP)


# Phony targets
.PHONY: all flash clean
