# Makefile for TinyMatrix

NAME	   = TinyMatrix
OBJECTS    = $(NAME).o
DEVICE     = attiny88
CLOCK      = 4000000
FUSES      = -U hfuse:w:0xDF:m -U lfuse:w:0xE2:m
PROGRAMMER = usbtiny
## PROGRAMMER = dragon_isp

AVRDUDE = avrdude -c $(PROGRAMMER) -P usb -p t88
## AVRDUDE = avrdude -c $(PROGRAMMER) -p t88
COMPILE = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

# symbolic targets:
all:	$(NAME).hex

.c.o:
	$(COMPILE) -c $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:$(NAME).hex:i

fuse:
	$(AVRDUDE) $(FUSES)

clean:
	rm -f $(NAME).hex $(NAME).elf $(OBJECTS)

# file targets:
$(NAME).elf: $(OBJECTS)
	$(COMPILE) -o $(NAME).elf $(OBJECTS)

$(NAME).hex: $(NAME).elf
	rm -f $(NAME).hex
	avr-objcopy -j .text -j .data -O ihex $(NAME).elf $(NAME).hex


