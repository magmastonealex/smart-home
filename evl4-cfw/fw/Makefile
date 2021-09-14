BPATH = out
DEPDIR = $(BPATH)/deps
MCU=atxmega64d4
OBJS = $(addprefix $(BPATH)/,$(patsubst src/%.c,%.o,$(wildcard src/*.c)))

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

CFLAGS+=-Wall -Werror -g -Os -mmcu=$(MCU) -std=c99 -DDEBUG=1
AVRDUDEFLAGS+=-v -c atmelice_pdi -p x64d4

all: $(BPATH)/prog.hex

$(DEPDIR): 
	mkdir -p $@
$(BPATH):
	mkdir -p $@

$(BPATH)/prog.hex: $(BPATH)/prog.elf
	avr-objcopy -O ihex $(BPATH)/prog.elf $(BPATH)/prog.hex

$(BPATH)/prog.elf: $(OBJS) | $(BPATH)
	avr-gcc $(CFLAGS) -o $(BPATH)/prog.elf $(OBJS)
	avr-objdump -Pmem-usage $(BPATH)/prog.elf

$(BPATH)/%.o: src/%.c | $(DEPDIR) $(BPATH)
	avr-gcc $(DEPFLAGS) $(CFLAGS) -o $@ -c $<

flash:
	avrdude $(AVRDUDEFLAGS) -U application:w:$(BPATH)/prog.hex
flashfuses:
	avrdude $(AVRDUDEFLAGS) -U lfuse:w:0xE2:m -U hfuse:w:0xD9:m  -U efuse:w:0xFF:m

clean:
	rm -fr $(BPATH)


DEPFILES := $(OBJS:$(BPATH)/%.o=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))

.PHONY: clean flash flashfuses

