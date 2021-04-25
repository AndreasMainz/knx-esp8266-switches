// http://library manager/all#Foo @ 1.2.3
#define KONNEKTING_SYSTEM_TYPE_SIMPLE
#include <KonnektingDevice.h>
#include "knx.h"
//#include <SerialFlash.h>
//#include <SerialFlash.h>
// ################################################
// ### KNX DEVICE CONFIGURATION
// ################################################
word individualAddress = P_ADDR(1, 2, 100);
word groupAddressInput = G_ADDR(7, 7, 7);
// ################################################
// ### DEVICE CONFIGURATION SEND OBJECTS
// ################################################
// Die GA's der Sende und Empfangs GA's MÜSSEN unterschiedlich sein.....leider.........!!!!!!!!!!!!!!!!
word T1_Short_AddressOutput = G_ADDR(2, 0, 25);
word T2_Short_AddressOutput = G_ADDR(2, 1, 102);
word T3_Short_AddressOutput = G_ADDR(2, 1, 103);
word T4_Short_AddressOutput = G_ADDR(2, 1, 104);

word T1_Long_AddressOutput = G_ADDR(2, 1, 105);
word T2_Long_AddressOutput = G_ADDR(2, 1, 106);
word T3_Long_AddressOutput = G_ADDR(2, 1, 107);
word T4_Long_AddressOutput = G_ADDR(2, 1, 108);

// ################################################
// ### DEVICE CONFIGURATION RECEIVE OBJECTS
// ################################################
word T1_Short_AddressInput = G_ADDR(2, 0, 32);
word T2_Short_AddressInput = G_ADDR(2, 1, 110);
word T3_Short_AddressInput = G_ADDR(2, 1, 111);
word T4_Short_AddressInput = G_ADDR(2, 1, 112);

word T1_Long_AddressInput = G_ADDR(2, 1, 113);
word T2_Long_AddressInput = G_ADDR(2, 1, 114);
word T3_Long_AddressInput = G_ADDR(2, 1, 115);
word T4_Long_AddressInput = G_ADDR(2, 1, 116);

// ################################################
// ### BOARD CONFIGURATION
// ################################################

#define KNX_SERIAL Serial   // swaped Serial on D7(GPIO13)=RX/GPIO15(D8)=TX 19200 baud
#define TEST_LED 16         // External LED might also be used for deep sleep..
#define DEBUGSERIAL Serial1 // the 2nd serial port with TX only (GPIO2/D4) 115200 baud

// ################################################
// ### DEBUG Configuration
// ################################################
// #define KDEBUG // comment this line to disable DEBUG mode

#ifdef KDEBUG
#include <DebugUtil.h>
#endif

// Define KONNEKTING Device related IDs
#define MANUFACTURER_ID 57005
#define DEVICE_ID 255
#define REVISION 0

#define Taster1 5    // D1
#define Taster2 12   // D6
#define Taster3 14   // D5
#define Taster4 4    // D2


// Definition of the Communication Objects attached to the device
KnxComObject KnxDevice::_comObjectsList[] = {
 /* Output */                              
    /* Suite-Index 0 : T1_short */ KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
    /* Suite-Index 1 : T2_short */ KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
    /* Suite-Index 2 : T3_short */ KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
    /* Suite-Index 3 : T4_short */ KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
    /* Suite-Index 4 : T1_long */  KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
    /* Suite-Index 5 : T2_long*/   KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
    /* Suite-Index 6 : T3_long*/   KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
    /* Suite-Index 7 : T4_long*/   KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
 /* Input um Zustände von anderen Schaltern mitzubekommen..
    /* Suite-Index 8 : T1_short */ KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
    /* Suite-Index 9 : T2_short */ KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
    /* Suite-Index 10: T3_short */ KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
    /* Suite-Index 11: T4_short */ KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
    /* Suite-Index 12: T1_long */  KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
    /* Suite-Index 13: T2_long*/   KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
    /* Suite-Index 14: T3_long*/   KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
    /* Suite-Index 15: T4_long*/   KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
};
const byte KnxDevice::_numberOfComObjects = sizeof (_comObjectsList) / sizeof (KnxComObject); // do no change this code

// Definition of parameter size
byte KonnektingDevice::_paramSizeList[] = {
    /* Param Index 0 */ PARAM_UINT16
};
const int KonnektingDevice::_numberOfParams = sizeof (_paramSizeList); // do no change this code

//we do not need a ProgLED, but this function muss be defined
void progLed (bool state){ 
    //nothing to do here
}

unsigned long sendDelay = 4000;
unsigned long lastmillis = millis(); 
bool lastState = false;
bool lastLedState = false;
byte virtualEEPROM[64];

// ################################################
// ###  ISR Routinen
// ################################################
   void IRAM_ATTR ISR_T1()
   {
      T1_Event_Rising = millis();
      if ((T1_Event_Rising - T1_Event_Falling)> 10) // Bouncing passed
      {
         attachInterrupt(Taster1, ISR_T1_RELEASE, RISING);
      }  
   }
   void IRAM_ATTR ISR_T1_RELEASE()
   {
      T1_Event_Falling = millis();
      timer1 = (T1_Event_Falling - T1_Event_Rising);
      if (timer1 > 10) // Bouncing passed
      {
         attachInterrupt(Taster1, ISR_T1, FALLING);
         if (timer1 > 600) T1_GA_long  = 1; // Serial.println(" -> laaaaaaaang. ");
         else              T1_GA_short = 1; // Serial.println(" -> kurz. ");
      }
  }
   void IRAM_ATTR ISR_T2()
   {
      T2_Event_Rising = millis();
      if ((T2_Event_Rising - T2_Event_Falling)> 10) // Bouncing passed
      {
         attachInterrupt(Taster2, ISR_T2_RELEASE, RISING);
      }  
   }
   void IRAM_ATTR ISR_T2_RELEASE()
   {
      T2_Event_Falling = millis();
      timer1 = (T2_Event_Falling - T2_Event_Rising);
      if (timer1 > 10) // Bouncing passed
      {
         attachInterrupt(Taster2, ISR_T2, FALLING);
         if (timer1 > 600) T2_GA_long  = 1; // Serial.println(" -> laaaaaaaang. ");
         else              T2_GA_short = 1; // Serial.println(" -> kurz. ");
      }
  }
   void IRAM_ATTR ISR_T3()
   {
      T3_Event_Rising = millis();
      if ((T3_Event_Rising - T3_Event_Falling)> 10) // Bouncing passed
      {
         attachInterrupt(Taster3, ISR_T3_RELEASE, RISING);
      }  
   }
   void IRAM_ATTR ISR_T3_RELEASE()
   {
      T3_Event_Falling = millis();
      timer1 = (T3_Event_Falling - T3_Event_Rising);
      if (timer1 > 10) // Bouncing passed
      {
         attachInterrupt(Taster3, ISR_T3, FALLING);
         if (timer1 > 600) T3_GA_long  = 1; // Serial.println(" -> laaaaaaaang. ");
         else              T3_GA_short = 1; // Serial.println(" -> kurz. ");
      }
  }
   void IRAM_ATTR ISR_T4()
   {
      T4_Event_Rising = millis();
      if ((T4_Event_Rising - T4_Event_Falling)> 10) // Bouncing passed
      {
         attachInterrupt(Taster4, ISR_T4_RELEASE, RISING);
      }  
   }
   void IRAM_ATTR ISR_T4_RELEASE()
   {
      T4_Event_Falling = millis();
      timer1 = (T4_Event_Falling - T4_Event_Rising);
      if (timer1 > 10) // Bouncing passed
      {
         attachInterrupt(Taster4, ISR_T4, FALLING);
         if (timer1 > 600) T4_GA_long  = 1; // Serial.println(" -> laaaaaaaang. ");
         else              T4_GA_short = 1; // Serial.println(" -> kurz. ");
      }
  }


// ################################################
// ### KNX EVENT CALLBACK
// ################################################

void knxEvents(byte index) {
    // Empfangsobjekte Input
    DEBUGSERIAL.println("GA Botschaft empfangen");
    switch (index) {
        // Wenn ein externer Schalter auf der Input Adresse den Wert 0 geschrieben hat, 
        // muß beim nächsten Tasterevent das Gegenteil geschrieben werden, 
        // damit die Toggle Fkt erhalten bleibt!
        case 8: // object index has been updated
            T1_Short_State = !Knx.read(8);
            DEBUGSERIAL.println(T1_Short_State);
            break;
        case 9: // object index has been updated
            T2_Short_State = !Knx.read(9);
            break;
        case 10: // object index has been updated
            T3_Short_State = !Knx.read(10);
            break;
        case 11: // object index has been updated
            T4_Short_State = !Knx.read(11);
            break;
        case 12: // object index has been updated
            T1_Long_State = !Knx.read(12);
            break;
        case 13: // object index has been updated
            T2_Long_State = !Knx.read(13);
            break;
        case 14: // object index has been updated
            T3_Long_State = !Knx.read(14);
            break;
        case 15: // object index has been updated
            T4_Long_State = !Knx.read(15);
            break;

        default:
                DEBUGSERIAL.println("Uninteressantes Objekt empfangen");
            break;
    }
}

byte readMemory(int index){
    return virtualEEPROM[index];
}

void writeMemory(int index, byte val) {
    virtualEEPROM[index] = val;
}

void updateMemory(int index, byte val) {
    if (readMemory(index) != val) {
        writeMemory(index, val);
    }
}

void commitMemory() {

}

void setup() {
    system_update_cpu_freq(60);
    WiFi.forceSleepBegin();
    // wifi_set_sleep_type(MODEM_SLEEP_T);
    memset(virtualEEPROM,0xFF,64);
    Konnekting.setMemoryReadFunc(&readMemory);
    Konnekting.setMemoryWriteFunc(&writeMemory);
    Konnekting.setMemoryUpdateFunc(&updateMemory);
    Konnekting.setMemoryCommitFunc(&commitMemory);

    // simulating allready programmed Konnekting device:
    // in this case we don't need Konnekting Suite
    // write hardcoded PA and GAs
    writeMemory(0,  0x7F);  //Set "not factory" flag
    writeMemory(1,  (byte)(individualAddress >> 8));
    writeMemory(2,  (byte)(individualAddress));
    /******************************************************* OUT ************************************************/
    /* ID0 */
    writeMemory(10, (byte)(T1_Short_AddressOutput >> 8));
    writeMemory(11, (byte)(T1_Short_AddressOutput));
    writeMemory(12, 0x80);  //activate OutputGA
    /* ID1 */    
    writeMemory(13, (byte)(T2_Short_AddressOutput >> 8));
    writeMemory(14, (byte)(T2_Short_AddressOutput));
    writeMemory(15, 0x80);  //activate OutputGA
    /* ID2 */
    writeMemory(16, (byte)(T3_Short_AddressOutput >> 8));
    writeMemory(17, (byte)(T3_Short_AddressOutput));
    writeMemory(18, 0x80);  //activate OutputGA
    /* ID3 */    
    writeMemory(19, (byte)(T4_Short_AddressOutput >> 8));
    writeMemory(20, (byte)(T4_Short_AddressOutput));
    writeMemory(21, 0x80);  //activate OutputGA
    /* ID4 */    
    writeMemory(22, (byte)(T1_Long_AddressOutput >> 8));
    writeMemory(23, (byte)(T1_Long_AddressOutput));
    writeMemory(24, 0x80);  //activate OutputGA
    /* ID5 */        
    writeMemory(25, (byte)(T2_Long_AddressOutput >> 8));
    writeMemory(26, (byte)(T2_Long_AddressOutput));
    writeMemory(27, 0x80);  //activate OutputGA
    /* ID6 */        
    writeMemory(28, (byte)(T3_Long_AddressOutput >> 8));
    writeMemory(29, (byte)(T3_Long_AddressOutput));
    writeMemory(30, 0x80);  //activate OutputGA
    /* ID7 */        
    writeMemory(31, (byte)(T4_Long_AddressOutput >> 8));
    writeMemory(32, (byte)(T4_Long_AddressOutput));
    writeMemory(33, 0x80);  //activate OutputGA
    /******************************************************* IN **************************************************/
    /* ID8 */
    writeMemory(34, (byte)(T1_Short_AddressInput >> 8));
    writeMemory(35, (byte)(T1_Short_AddressInput));
    writeMemory(36, 0x80);  //activate InputGA
    /* ID9 */
    writeMemory(37, (byte)(T2_Short_AddressInput >> 8));
    writeMemory(38, (byte)(T2_Short_AddressInput));
    writeMemory(39, 0x80);  //activate InputGA
    /* ID10 */
    writeMemory(40, (byte)(T3_Short_AddressInput >> 8));
    writeMemory(41, (byte)(T3_Short_AddressInput));
    writeMemory(42, 0x80);  //activate InputGA
    /* ID11 */
    writeMemory(43, (byte)(T4_Short_AddressInput >> 8));
    writeMemory(44, (byte)(T4_Short_AddressInput));
    writeMemory(45, 0x80);  //activate InputGA
    /* ID12 */
    writeMemory(46, (byte)(T1_Long_AddressInput >> 8));
    writeMemory(47, (byte)(T1_Long_AddressInput));
    writeMemory(48, 0x80);  //activate InputGA
    /* ID13 */
    writeMemory(49, (byte)(T2_Long_AddressInput >> 8));
    writeMemory(50, (byte)(T2_Long_AddressInput));
    writeMemory(51, 0x80);  //activate InputGA
    /* ID14 */
    writeMemory(52, (byte)(T3_Long_AddressInput >> 8));
    writeMemory(53, (byte)(T3_Long_AddressInput));
    writeMemory(54, 0x80);  //activate InputGA
    /* ID15 */
    writeMemory(55, (byte)(T4_Long_AddressInput >> 8));
    writeMemory(56, (byte)(T4_Long_AddressInput));
    writeMemory(57, 0x80);  //activate InputGA

    // debug related stuff
#ifdef KDEBUG

    // Start debug serial with 115200 bauds
    DEBUGSERIAL.begin(115200);

    //waiting 3 seconds, so you have enough time to start serial monitor
    // delay(3000);
    DEBUGSERIAL.println("Setup done");
    Serial.println("Setup done");
    // make debug serial port known to debug class
    // Means: KONNEKTING will use the same serial port for console debugging
    Debug.setPrintStream(&DEBUGSERIAL);
#endif

    // Initialize KNX enabled Arduino Board
    Konnekting.init(KNX_SERIAL,
            &progLed,
            MANUFACTURER_ID,
            DEVICE_ID,
            REVISION);

    // Init Outputs
    // pinMode(TEST_LED,OUTPUT); Pin 16 might be conneted to RST Pin for Deep sleep

    // Init Inputs
    pinMode(Taster1,INPUT_PULLUP);
    pinMode(Taster2,INPUT_PULLUP);
    pinMode(Taster3,INPUT_PULLUP);
    pinMode(Taster4,INPUT_PULLUP);
    // Attach interrupts
    attachInterrupt(Taster1, ISR_T1, FALLING);
    attachInterrupt(Taster2, ISR_T2, FALLING);
    attachInterrupt(Taster3, ISR_T3, FALLING);
    attachInterrupt(Taster4, ISR_T4, FALLING);
}

 
void loop() {
    Knx.task();
    /* kein Programmiermodus ?
     */
    if (Konnekting.isReadyForApplication()) {
        // Write GA's according Switch period
        if (T1_GA_short)
        {
          T1_GA_short = 0;
          Knx.write(0, T1_Short_State);
          DEBUGSERIAL.println("T1 kurz");
          T1_Short_State = !T1_Short_State; // toggle state -> UM Taster Mode
        } else {
          if (T1_GA_long)
          {
            T1_GA_long = 0;
            Knx.write(4, T1_Long_State);
            DEBUGSERIAL.println("T1 lang");
            T1_Long_State = !T1_Long_State; // toggle state -> UM Taster Mode
          }
        }
        if (T2_GA_short)
        {
          T2_GA_short = 0;
          Knx.write(1, T2_Short_State);
          T2_Short_State = !T2_Short_State; // toggle state -> UM Taster Mode
        } else {
          if (T2_GA_long)
          {
            T2_GA_long = 0;
            Knx.write(5, T2_Long_State);
            T2_Long_State = !T2_Long_State; // toggle state -> UM Taster Mode
          }
        }
        if (T3_GA_short)
        {
          T3_GA_short = 0;
          Knx.write(2, T3_Short_State);
          T3_Short_State = !T3_Short_State; // toggle state -> UM Taster Mode
        } else {
          if (T3_GA_long)
          {
            T3_GA_long = 0;
            Knx.write(6, T3_Long_State);
            T3_Long_State = !T3_Long_State; // toggle state -> UM Taster Mode
          }
        }
        if (T4_GA_short)
        {
          T4_GA_short = 0;
          Knx.write(3, T4_Short_State);
          T4_Short_State = !T4_Short_State; // toggle state -> UM Taster Mode
        } else {
          if (T4_GA_long)
          {
            T4_GA_long = 0;
            Knx.write(7, T4_Long_State);
            T4_Long_State = !T4_Long_State; // toggle state -> UM Taster Mode
          }
        }
     }
} // End loop
