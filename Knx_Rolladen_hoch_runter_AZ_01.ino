// Pin16 Prog LED wenn WLAN an!
// Kondensator an VCC ?
// Taster links oben : Hoch
// Taster links unten: runter
// Taster rechts oben: Stop
// Taster rechts unten: Automatik sperren = 1
// WLAN Trigger via GA 2/1/121 !!

// http://library manager/all#Foo @ 1.2.3
#define KONNEKTING_SYSTEM_TYPE_SIMPLE
#include <KonnektingDevice.h>
#include "knx.h"
// #include <SerialFlash.h>
// ################################################
// ### KNX DEVICE CONFIGURATION
// ################################################
word individualAddress = P_ADDR(1, 1, 13);
// ################################################
// ### DEVICE CONFIGURATION SEND OBJECTS
// ################################################
// Die GA's der Sende und Empfangs GA's MÜSSEN unterschiedlich sein.....leider.........!!!!!!!!!!!!!!!!
word Rolladen_hoch_runter = G_ADDR(2, 2, 3);
word Rolladen_stop        = G_ADDR(2, 3, 14);
word Rolladen_Sperre      = G_ADDR(2, 2, 25);

// ################################################
// ### DEVICE CONFIGURATION RECEIVE OBJECTS
// ################################################
word Zustand_Rolladen      = G_ADDR(2, 2, 7);
word Device_Reset_GA = G_ADDR(2, 1, 121);

// ################################################
// ### BOARD CONFIGURATION
// ################################################

#define KNX_SERIAL Serial   // swaped Serial on D7(GPIO13)=RX/GPIO15(D8)=TX 19200 baud
#define TEST_LED 16         // External LED might also be used for deep sleep..
#define DEBUGSERIAL Serial1 // the 2nd serial port with TX only (GPIO2/D4) 115200 baud

// ################################################
// ### DEBUG Configuration
// ################################################
#define KDEBUG // comment this line to disable DEBUG mode

#ifdef KDEBUG
#include <DebugUtil.h>
#endif

// Define KONNEKTING Device related IDs
#define MANUFACTURER_ID 57005
#define DEVICE_ID 255
#define REVISION 0

#define Taster1 5
#define Taster2 4
#define Taster3 14
#define Taster4 12

// Wlan Config

ESP8266WebServer server(80);
WebConfig conf;

// Calculate Group Adress from String stored in SPIFFS
word Extract_G_Addr(String ga_name) {
  int first = ga_name.indexOf('/');
  int last  = ga_name.lastIndexOf('/');
  // Zwei / in GA vorhanden?
  if ((first != (-1)) && (last != (-1)))
  {
    area  = (ga_name.substring(0, first).toInt()) & 0x1f;
    linie = (ga_name.substring(first + 1, last).toInt()) & 7;
    grp   = ga_name.substring(last + 1).toInt();
    ga = (area * 2048) + (linie * 256) + grp;
  }
  else
  {
    // Default 1/1/1
    ga = 0x0901;
  }
  return (ga);
}

// Calculate physical Adress from String stored in SPIFFS
word Extract_P_Addr(String pa_name) {
  int first = pa_name.indexOf('.');
  int last  = pa_name.lastIndexOf('.');
  // Zwei . in PA vorhanden?
  if ((first != (-1)) && (last != (-1)))
  {
    area  = (pa_name.substring(0, first).toInt()) & 0xf;
    linie = (pa_name.substring(first + 1, last).toInt()) & 0xf;
    grp   = pa_name.substring(last + 1).toInt();
    pa = (area * 4096) + (linie * 256) + grp;
  }
  else
  {
    // Default 1.1.1
    pa = 0x1101;
  }
  return (pa);
}

void initWiFi() {
  // WiFi.forceSleepWake();
  WiFi.mode(WIFI_AP);
 delay(100);
 // WiFi.softAP("KnxRolladenAZ");
 // AP Name aus SPIffs Datei übernehmen
 WiFi.softAP(conf.getApName());
 delay(100);
}

void handleRoot() {
  //server.send(200, "text/html", "<h1>You are connected</h1>"); // For testing
  conf.handleFormRequest(&server);
}



// Definition of the Communication Objects attached to the device
KnxComObject KnxDevice::_comObjectsList[] = {
  /* Output */
  /* Suite-Index 0 : Roll_hoch_runter */ KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
  /* Suite-Index 1 : Rolladen_stop */    KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
  /* Suite-Index 2 : Rolladen_sperre */  KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
  /* Input um Zustände von anderen Schaltern mitzubekommen..
  /* Suite-Index 3 : Rolladen_Zustand */ KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
  /* Suite-Index 4: Roll_reset*/         KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
};
const byte KnxDevice::_numberOfComObjects = sizeof (_comObjectsList) / sizeof (KnxComObject); // do no change this code

// Definition of parameter size
byte KonnektingDevice::_paramSizeList[] = {
  /* Param Index 0 */ PARAM_UINT16
};
const int KonnektingDevice::_numberOfParams = sizeof (_paramSizeList); // do no change this code

//we do not need a ProgLED, but this function muss be defined
void progLed (bool state) {
  //nothing to do here
}

byte virtualEEPROM[64];

// ################################################
// ###  ISR Routinen
// ################################################

void IRAM_ATTR ISR_T1()
{
  T1_Event_Rising = millis();
  if ((T1_Event_Rising - T1_Event_Falling) > 10) // Bouncing passed
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
    T1_GA_short = 1;
  }
}
void IRAM_ATTR ISR_T2()
{
  T2_Event_Rising = millis();
  if ((T2_Event_Rising - T2_Event_Falling) > 10) // Bouncing passed
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
    T2_GA_short = 1; 
  }
}
void IRAM_ATTR ISR_T3()
{
  T3_Event_Rising = millis();
  if ((T3_Event_Rising - T3_Event_Falling) > 10) // Bouncing passed
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
    T3_GA_short = 1; // Serial.println(" -> kurz. ");
  }
}
void IRAM_ATTR ISR_T4()
{
  T4_Event_Rising = millis();
  if ((T4_Event_Rising - T4_Event_Falling) > 10) // Bouncing passed
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
    T4_GA_short = 1; // Serial.println(" -> kurz. ");
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
    case 3: // object index has been updated
      Rolladen_Status = !Knx.read(3);
      break;
    case 4: // object index has been updated
      {
        T_Reset_cnt ++;
        if (T_Reset_cnt == 2)
        { // memorize: boot with Wlan on
          #if 1
          EEPROM.write(1, 'b');
          EEPROM.commit();
          T_Reset_cnt ++; // Lock path
          Serial.swap();
          Serial.println("Starte WLan nach Boot! ");
          ESP.restart();
          #endif
        }
        break;
      }

    default:
      DEBUGSERIAL.println("Uninteressantes Objekt empfangen");
      break;
  }
}

byte readMemory(int index) {
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
  // system_soft_wdt_stop(); // disable the software watchdog
  EEPROM.begin(512);
  T_Boot_Mode = EEPROM.read(1);
  Serial.begin(115200); //Serial zunächst in der Standard_Config starten, wird durch Konnekting.init dann umgelegt auf Pin13/15
  Serial.println(params);
  conf.setDescription(params);
  conf.readConfig();
  // uint8_t cnt = conf.getCount();
  //Serial.println(cnt);
  //Serial.println(conf.values[2]);
  //Serial.println(conf.values[3]);
  /////////////////////////////////////////////////////////////////////
  // Default KNX parameter mit den Werten aus dem SPIffs überschreiben
  /////////////////////////////////////////////////////////////////////
  individualAddress = Extract_P_Addr(conf.values[0]);
  //Serial.println(individualAddress);
  Rolladen_hoch_runter = Extract_G_Addr(conf.values[1]);
  //Serial.println(Rolladen_hoch_runter);
  Rolladen_stop        = Extract_G_Addr(conf.values[2]);
  //Serial.println(Rolladen_stop);
  Rolladen_Sperre      = Extract_G_Addr(conf.values[3]);
  //Serial.println(Rolladen_Sperre);
  Zustand_Rolladen     = Extract_G_Addr(conf.values[4]);
  //Serial.println(Zustand_Rolladen);
  Device_Reset_GA      = Extract_G_Addr(conf.values[5]);
  //Serial.println(Device_Reset_GA);
  
  // T_Boot_Mode = 'o'; // Testing
  if (T_Boot_Mode != 'b')
  { // Wlan aus, Stromsparmodus an, Serve Knx Events
    T_Reset_cnt = 0;
    WiFi.forceSleepBegin();
    memset(virtualEEPROM, 0xFF, 64);
    // Init Inputs
    pinMode(Taster1, INPUT_PULLUP);
    pinMode(Taster2, INPUT_PULLUP);
    pinMode(Taster3, INPUT_PULLUP);
    pinMode(Taster4, INPUT_PULLUP);
    // Attach interrupts
    attachInterrupt(Taster1, ISR_T1, FALLING);
    attachInterrupt(Taster2, ISR_T2, FALLING);
    attachInterrupt(Taster3, ISR_T3, FALLING);
    attachInterrupt(Taster4, ISR_T4, FALLING);

    Konnekting.setMemoryReadFunc(&readMemory);
    Konnekting.setMemoryWriteFunc(&writeMemory);
    Konnekting.setMemoryUpdateFunc(&updateMemory);
    Konnekting.setMemoryCommitFunc(&commitMemory);

    // simulating already programmed Konnekting device:
    // in this case we don't need Konnekting Suite
    // write hardcoded PA and GAs
    writeMemory(0,  0x7F);  //Set "not factory" flag
    writeMemory(1,  (byte)(individualAddress >> 8));
    writeMemory(2,  (byte)(individualAddress));
    /******************************************************* OUT ************************************************/
    /* ID0 */
    writeMemory(10, (byte)(Rolladen_hoch_runter  >> 8));
    writeMemory(11, (byte)(Rolladen_hoch_runter ));
    writeMemory(12, 0x80);  //activate OutputGA
    /* ID1 */
    writeMemory(13, (byte)(Rolladen_stop >> 8));
    writeMemory(14, (byte)(Rolladen_stop ));
    writeMemory(15, 0x80);  //activate OutputGA
    /* ID2 */
    writeMemory(16, (byte)(Rolladen_Sperre >> 8));
    writeMemory(17, (byte)(Rolladen_Sperre ));
    writeMemory(18, 0x80);  //activate OutputGA
    /* ID3 */
    writeMemory(19, (byte)(Zustand_Rolladen >> 8));
    writeMemory(20, (byte)(Zustand_Rolladen));
    writeMemory(21, 0x80);  //activate OutputGA
    /* ID4 */
    writeMemory(22, (byte)(Device_Reset_GA >> 8));
    writeMemory(23, (byte)(Device_Reset_GA));
    writeMemory(24, 0x80);  //activate OutputGA
    // debug related stuff
#ifdef KDEBUG

    // Start debug serial with 115200 bauds
    DEBUGSERIAL.begin(115200);

    //waiting 3 seconds, so you have enough time to start serial monitor
    // delay(3000);
    DEBUGSERIAL.println("Setup done");
    // make debug serial port known to debug class
    // Means: KONNEKTING will use the same serial port for console debugging
    Debug.setPrintStream(&DEBUGSERIAL);
#endif
    // Switch serial !!
    Serial.println("End startup communication..switch to Knx Rx/Tx");
    // Initialize KNX enabled Arduino Board
    Konnekting.init(KNX_SERIAL,
                    &progLed,
                    MANUFACTURER_ID,
                    DEVICE_ID,
                    REVISION);

    // Init Outputs
    // pinMode(TEST_LED,OUTPUT); Pin 16 might be conneted to RST Pin for Deep sleep
  }
  else
  { // Wlan an
    // Serial.begin(115200);
    // Wenn das Eeprom im Boot Mode war, wird dieser wieder gelöscht.
    // Der Bootmode kann nur durch das zweifache Schreiben der Boot GA aktiviert werden
    EEPROM.write(1, '0');
    EEPROM.commit();

    // Serial.println(params);
    //conf.setDescription(params);
    // conf.readConfig();
    initWiFi();
    delay(200);
    server.on("/", handleRoot);
    server.begin();
    delay(2000);
  }
}


void loop() {
  /* kein Programmiermodus ?
  */
  Main_Clock++;
   if (T_Boot_Mode != 'b') {
    Knx.task();
    // Write GA's according Switch period
    if (T1_GA_short)
    {
      T1_GA_short = 0;
      Knx.write(0, 0);
    }
    if (T2_GA_short)
    {
      T2_GA_short = 0;
      Knx.write(1, 1);
    }
    if (T3_GA_short)
    {
      T3_GA_short = 0;
      Knx.write(0, 1);
    }
    if (T4_GA_short)
    {
      T4_GA_short = 0;
      Knx.write(2, Rolladen_State_Sperre);
      Rolladen_State_Sperre = !Rolladen_State_Sperre; // toggle state -> UM Taster Mode
    } 
  }
  else
  { // in Boot mode Serve Wlan requests
    if ((Main_Clock & 0x3f)== 0){
      server.handleClient(); // Zieht viel Strom!!! Oft Absturz!! Nicht zu häufig aufrufen..
      Serial.print(".");
    }
    if (Main_Clock < 2)Serial.println(" Wlan modus! ");
  }
  delay(30);
} // End loop
