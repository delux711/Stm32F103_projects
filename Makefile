APP ?= test_RS485

TARGET := $(APP)
BUILD_DIR := build/$(APP)

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size

LDSCRIPT := bsp/STM32F103C8/stm32f103c8_flash.ld

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

DEFS := \
	-DSTM32F10X_MD

CPUFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft

CFLAGS := $(CPUFLAGS) \
	-std=c11 \
	-Os \
	-ffunction-sections -fdata-sections \
	-fno-common \
	-Wall -Wextra \
	$(DEFS) $(INCLUDES)

LDFLAGS := $(CPUFLAGS) \
	-T$(LDSCRIPT) \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
	-Wl,--gc-sections \
	-Wl,--print-memory-usage \
	-specs=nano.specs \
	-specs=nosys.specs \
	-nostartfiles

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

ELF := $(BUILD_DIR)/$(TARGET).elf
HEX := $(BUILD_DIR)/$(TARGET).hex
BIN := $(BUILD_DIR)/$(TARGET).bin

.PHONY: all clean list-apps

all: $(ELF) $(HEX) $(BIN)

list-apps:
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
	@if exist build rmdir /S /Q build