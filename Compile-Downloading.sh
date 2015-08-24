#Compile and Downloading program into ATmega644pa(Zigbee Coordinator)
#

#we need only to chang this file name when we need to compile another file
#  InitTime
#  LCD_Timer0
#  WatchTimeDog
#  AT24C128_INIT
#  Zigbee_Coordinator_July_06
#  time_tester
#  Blink
#  Eeprom_Test
#  USART
#  DS1307
#  AT24C128_test
#  Button
#  GPRS_Config
#  GPRS_Config_NakedSend
#  Selector
#  GPRS_Config_Button
#  Zigbee_Coordinator_July_06
#  Zigbee_Coordinator_July_15
#  Zigbee_Coordinator_July_24
FileName=Zigbee_Coordinator_July_24

echo "Compilling ... ... "

avr-gcc -mmcu=atmega644p -Wall -Os -o $FileName.o $FileName.c
#if [ -f "$FileName.o" ];then

# $? return execuation status of last command
#if [ $? -eq 0 ];then

#    echo "Compiling Failed"

#    else

    echo "Transformating ... ... "

    avr-objcopy -j .text -j .data -O ihex  $FileName.o $FileName.hex

#    if [$? -eq 0 ]
#    	then
#	echo "Transforming Failed"
#	else
	
	echo "Downloading ... ... "

	sudo avrdude -p m644p -c usbasp -e -U flash:w:$FileName.hex
	rm $FileName.o $FileName.hex
#    fi

#fi
