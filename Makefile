# Automatická detekcia názvu aplikácie z priečinku
CURRENT_DIR_NAME := $(notdir $(CURDIR))
VALID_APPS := test_button test_RS485 test_RS485_modbus
ifneq ($(filter $(CURRENT_DIR_NAME), $(VALID_APPS)),)
    APP ?= $(CURRENT_DIR_NAME)
else
    APP ?= test_RS485
endif

.SILENT:

TARGET_APP := $(APP)
# TARGET_BUILD = RAM or FLASH
TARGET_BUILD ?= RAM
# TARGET_BUILD ?= FLASH

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size

ifeq ($(TARGET_BUILD), RAM)
	LDSCRIPT := bsp/STM32F103C8/stm32f103c8_ram.ld
	BUILD_DIR := build/$(APP)_ram
	DEFS := -DSTM32F10X_MD -DVECT_TAB_SRAM
else ifeq ($(TARGET_BUILD), FLASH)
	LDSCRIPT := bsp/STM32F103C8/stm32f103c8_flash.ld
	BUILD_DIR := build/$(APP)_flash
	DEFS := -DSTM32F10X_MD
else
	$(error Invalid TARGET_BUILD='$(TARGET_BUILD)'. Use RAM or FLASH)
endif

APP_DIR := apps/$(APP)

CMSIS_DEVICE := cmsis/device/STM32F103C8
CMSIS_CORE := cmsis/core
RTT_RTT := cmsis/RTT/RTT
RTT_CFG := cmsis/RTT/Config

SRCS := \
	$(APP_DIR)/main.c \
	bsp/STM32F103C8/startup_gcc.c \
	bsp/STM32F103C8/bsp_irq.c \
	$(CMSIS_DEVICE)/system_stm32f10x.c \
	$(RTT_RTT)/SEGGER_RTT.c \
	$(RTT_RTT)/SEGGER_RTT_printf.c \
	$(wildcard driver/*.c)

INCLUDES := \
	-I$(APP_DIR) \
	-Ibsp/STM32F103C8 \
	-Idriver \
	-I$(CMSIS_DEVICE) \
	-I$(CMSIS_CORE) \
	-I$(RTT_RTT) \
	-I$(RTT_CFG)

CPUFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft

CFLAGS := $(CPUFLAGS) \
	-std=c11 \
	-Os \
	-ffunction-sections -fdata-sections \
	-fno-common \
	-Wall -Wextra \
	-g3 \
	$(DEFS) $(INCLUDES)

LDFLAGS := $(CPUFLAGS) \
	-T$(LDSCRIPT) \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET_APP).map \
	-Wl,--gc-sections \
	-Wl,--print-memory-usage \
	-Wl,--undefined=Reset_Handler \
	-Wl,--undefined=main \
	-specs=nano.specs \
	-specs=nosys.specs \
	-nostartfiles

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

ELF := $(BUILD_DIR)/$(TARGET_APP).elf
HEX := $(BUILD_DIR)/$(TARGET_APP).hex
BIN := $(BUILD_DIR)/$(TARGET_APP).bin

.PHONY: all clean clean_all clean_r clean_f list-apps r f

all: $(ELF) $(HEX) $(BIN)

r:
	$(MAKE) TARGET_BUILD=RAM all

f:
	$(MAKE) TARGET_BUILD=FLASH all

clean_r:
	$(MAKE) TARGET_BUILD=RAM clean

clean_f:
	$(MAKE) TARGET_BUILD=FLASH clean

list:
	@echo test_button
	@echo test_RS485
	@echo test_RS485_modbus

$(ELF): $(OBJS)
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: %.c
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"

clean_all:
	@if exist build rmdir /S /Q build