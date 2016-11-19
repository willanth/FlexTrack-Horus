/*------------------------------------------------------------------------------------------------------*\
|                                                                                                        |
| New tracker code that can be simply rebuilt for various hardware designs.  e.g. serial or i2c GPS,     |
| NTX2B or LoRa radio.  Using LoRa, it can configured to use TDMA in which case it will accept uplinked  |
| messages and/or repeat messages from other balloons.                                                   |
|                                                                                                        |
| Configuration is using #defines near the top of the file.  These control which other modules get       |
| used and linked in, and configure those modules (e.g. radio frequency).                                |
|                                                                                                        |
| V0.00   First stab                                                                                     |
|                                                                                                        |
\*------------------------------------------------------------------------------------------------------*/

#include <avr/pgmspace.h>

//------------------------------------------------------------------------------------------------------

// CONFIGURATION SECTION.

// Edit this section to choose the hardware design and set your payload ID etc

// PRIMARY PAYLOAD SETTINGS
#define PAYLOAD_ID          0
#define USE_TDMA            1             // Comment out to disable TDMA by default.
#define TOTAL_PAYLOADS      1             // Total number of payloads in the TDMA system.
#define LORA_FREQUENCY      431.650       // Primary operating frequency.
#define BACKUP_FREQUENCY    449.900       // Backup frequency in case GPS lock is lost.


// Authentication for Cutdown & Parameter Change Uplink commands.
#define AUTH_0  65
#define AUTH_1  65
#define AUTH_2  65

// CHOOSE BOARD (comment out one of these only)
#define UAVANUT_LORA

// Power settings
#define POWERSAVING	                      // Comment out to disable GPS power saving

// LORA settings
#define LORA_SLOT            PAYLOAD_ID
#define LORA_REPEAT_SLOT_1   0
#define LORA_REPEAT_SLOT_2   0
#define LORA_ID              PAYLOAD_ID
#define LORA_CYCLETIME       5*TOTAL_PAYLOADS // Set to zero to send continuously
#define LORA_MODE            0
#define LORA_BINARY          1              // Use binary packets exclusively.
#define LORA_LISTEN_TIME     10000L         // Time spent listening for uplink packets, when in non-TDMA mode.

//------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------

// HARDWARE DEFINITIONS

// For unsupported hardware, add your own section here

#ifdef UAVANUT_LORA
  #define GPS_I2C             1                // Comment out if using serial GPS
  #define LORA_NSS           10                // Comment out to disable LoRa code
  #define LORA_RESET          7                // Comment out if not connected
  #define LORA_DIO0           3                
  #define LORA_DIO5           2
  #define LED_WARN            5
  #define LED_OK              6
  #define A0_MULTIPLIER      4.9
  #define BATT_ADC            A0
  #define PYRO_ADC            A1
  #define PYRO_ENABLE         A2
#endif

//------------------------------------------------------------------------------------------------------

// Default serial port usage
#ifndef GPS_SERIAL
  #ifndef GPS_I2C
    #define GPS_SERIAL Serial
  #endif
#endif

#ifndef DEBUG_SERIAL
  #define DEBUG_SERIAL Serial
#endif

#define SENTENCE_LENGTH      100  // Length of internal packet buffer

//------------------------------------------------------------------------------------------------------
//
//  Globals

struct TBinaryPacket
{
  uint8_t   PacketType;
  uint8_t   PayloadFlags;
  uint8_t   PayloadIDs;
  uint16_t  Counter;
  uint16_t  BiSeconds;
  float   Latitude;
  float   Longitude;
  uint16_t    Altitude;
  uint8_t   Speed; // Speed in Knots (1-255 knots)
  uint8_t   Sats;
  uint8_t   Temp; // Twos Complement Temp value.
  uint8_t   BattVoltage; // 0 = 0.5v, 255 = 2.0V, linear steps in-between.
  uint8_t   PyroVoltage; // 0 = 0v, 255 = 5.0V, linear steps in-between.
  uint8_t   rxPktCount; // RX Packet Count.
  uint8_t   rxRSSI; // Ambient RSSI value, measured just before transmission.
  uint8_t   telemFlags; // Various payload flags.
};  //  __attribute__ ((packed));

struct TGPS
{
  int Hours, Minutes, Seconds;
  unsigned long SecondsInDay;					// Time in seconds since midnight
  float Longitude, Latitude;
  long Altitude;
  unsigned int Satellites;
  int Speed;
  int Direction;
  byte Lock;
  byte psm_status;
  float InternalTemperature;
  float BatteryVoltage;
  float ExternalTemperature;
  float Pressure;
  unsigned int BoardCurrent;
  unsigned int errorstatus;
  byte FlightMode;
  byte PowerMode;
} GPS;


int SentenceCounter=0;

//------------------------------------------------------------------------------------------------------

void setup()
{
  // Serial port(s)
  
  #ifdef GPS_SERIAL
    GPS_SERIAL.begin(9600);
  #endif
  
  #ifdef DEBUG_SERIAL
    DEBUG_SERIAL.begin(9600);
    Serial.println("");
    Serial.print("FlexTrack Flight Computer, payload ID(s)");

    #ifdef LORA_NSS
      Serial.print(' ');
      Serial.print(PAYLOAD_ID);
    #endif  
      
    Serial.println("");
    Serial.println("");
  #endif

#ifdef GPS_I2C
  Serial.println(F("I2C GPS"));
#else
  Serial.println(F("Serial GPS"));
#endif

  Serial.print(F("Free memory = "));
  Serial.println(freeRam());

  SetupLEDs();
  
  SetupGPS();
  
  SetupADC();
  
#ifdef LORA_NSS
  SetupLoRa();
#endif

}


void loop()
{  
  CheckGPS();
  
#ifdef LORA_NSS
  CheckLoRa();
#endif
  
  CheckADC();
  
  CheckLEDs();
}


int freeRam(void)
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}