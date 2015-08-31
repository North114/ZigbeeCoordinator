CC=avr-gcc
CP=avr-objcopy
CFLAGS=-Wall -Os
TFLAGS=-j .text -j .data -O ihex
DFLAG=-p m644p -c usbasp -e -U 
## file name lists
## GPRS_Config_NakedSend
SOURCE=Zigbee_Coordinator_July_24.c
OBJECT=$(SOURCE:.c=.o)

all:output.hex

$(OBJECT):$(SOURCE)
	$(CC) -mmcu=atmega644p $(CFLAGS) -o $@ $<

output.hex:$(OBJECT)
	$(CP) $(TFLAGS) $< $@

## dl for down load
dl:
	sudo avrdude $(DFLAG) flash:w:output.hex

clean:
	rm $(OBJECT) output.hex
