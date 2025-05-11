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
TASK_SRC = $(UWIRE_DIR)/tasks.c
SERIAL_SRC = $(SERIAL_DIR)/serial.c
OBJ = $(BUILD_DIR)/main.o
TASK_OBJ = $(BUILD_DIR)/tasks.o
SERIAL_OBJ = $(BUILD_DIR)/serial.o

ELF = $(BUILD_DIR)/prj.elf
HEX = $(FLASH_DIR)/prj.hex

# Default target (build everything)
all: $(HEX)

# .c file into .o
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(TASK_OBJ): $(TASK_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERIAL_OBJ): $(SERIAL_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Link .o to .elf
$(ELF): $(OBJ) $(TASK_OBJ) $(SERIAL_OBJ)
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
