#ifndef ZigBee_H

#define ZigBee_H

/* Zigbee Related Macro */
/** StartByte_Zigbee + UserIDByte + LeakageValueByteMSB + LeakageValueByteLSB
+ VoltageMSB + VoltagelSB + EndByte_Zigbee**/
#define recBufferSize_Zigbee 14// larger than PackLen
#define Zigbee_PackLen 7
#define Zigbee_AckLen 5

#define StartByte_Zigbee 0xAA
#define EndByte_Zigbee 0x75 //End byte should less than 128,since it's a character
#define ZigbeeQueryByte 0xCC //query command byte

#endif