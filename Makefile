# Compiler
CC=avr-gcc

# Objcopy
CP=avr-objcopy

# Compile Flags
CFLAGS=-Wall -Os

# Objcopy Command Flags
TFLAGS=-j .text -j .data -O ihex

# Download Command Flags
DFLAG=-p m644p -c usbasp -e -U 

# Operating System Type
UNAME=$(shell uname)
OS_TYPE=$(shell (printf $(UNAME) | tr A-Z a-z))

# Source File Actually
SourceDir=./include
HEADERS=$(SourceDir)/ds1307.c \
$(SourceDir)/usart.c \
$(SourceDir)/at24c128.c \
$(SourceDir)/init.c \
$(SourceDir)/GPRS.c

# Macros
MACROS=#-DDEBUG

## file name lists
## GPRS_Config_NakedSend.c
## Zigbee_Coordinator_July_24.c

# Main Source File
#SOURCE=Zigbee_Coordinator_July_24.c
SOURCE=ZigbeeCoordinator_gprs.c

# Object File (Cooresponding to Main Source File)
OBJECT=$(SOURCE:.c=.o)

# Output File
OUTPUT=output.hex


# Rules Start Here
# ---------------------------------------------------------------
all:$(OUTPUT)

$(OUTPUT):$(OBJECT)
	$(CP) $(TFLAGS) $< $@

$(OBJECT):$(SOURCE)
	$(CC) $(MACROS) -mmcu=atmega644p $(CFLAGS) -o $@ $< $(HEADERS)

#$(SOURCE):$(HEADERS)

## dl for Download
dl:
	@echo "Start Downloading!"
ifneq (, $(findstring linux,$(OS_TYPE)))
	sudo avrdude $(DFLAG) flash:w:$(OUTPUT)
else ifneq (, $(findstring mingw, $(OS_TYPE)))
	avrdude $(DFLAG) flash:w:$(OUTPUT)
else ifneq (, $(findstring cygwin, $(OS_TYPE)))
	avrdude $(DFLAG) flash:w:$(OUTPUT)
else
	@echo "un-defined OS , execuate default operation!"
	avrdude $(DFLAG) flash:w:$(OUTPUT)

	@echo "Downloading Finished!"
endif

clean:
	rm -f $(OBJECT) $(OUTPUT)
