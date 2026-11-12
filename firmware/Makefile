TOOLCHAIN ?= arm-none-eabi-
CC      = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
SIZE    = $(TOOLCHAIN)size

TARGET  = wnc_fw
SRCS    = main.c system_SWM190.c \
          sdk/driver/SWM190_port.c sdk/driver/SWM190_gpio.c sdk/driver/SWM190_pwm.c \
          sdk/driver/SWM190_uart.c sdk/driver/SWM190_timr.c
ASRCS   = startup_SWM190.s

CFLAGS  = -mcpu=cortex-m0 -mthumb -Os -g -Wall -Wno-unused-variable -std=gnu11 \
          -ffunction-sections -fdata-sections -fno-common -fno-delete-null-pointer-checks \
          -Isdk/core -Isdk/device -Isdk/driver -I.
LDFLAGS = -mcpu=cortex-m0 -mthumb -T swm190_codesram.ld -Wl,--gc-sections -Wl,-Map=$(TARGET).map \
          -nostartfiles --specs=nano.specs --specs=nosys.specs

OBJS = $(SRCS:.c=.o) $(ASRCS:.s=.o)

all: $(TARGET).bin

$(TARGET).elf: $(OBJS) swm190_codesram.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	@python3 -I check_image.py $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) -mcpu=cortex-m0 -mthumb -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).map

.PHONY: all clean
