// use i2c only
// Es weden alle 5 Schalter unterstützt, jedoch lange Betätigung ist noch frei
// Eine lange Betätigung bei Automatik Toggle entfällt
// Fensterkontakt einlesen fehlt noch, demnächst Versuch mit Pin10..
// zur Statemachine: Eigentlich gibt es 5 Hauptstates: Init, Ruhe, Hoch, Runter, RW_hr, RW_rh (RW = Richtungswechsel) und Sperre.
// Diese 5 Grundstates wurden weiter unterteilt in einen Active und einen Hold State, der Übergang von Active auf Hold Schreibt einmalig ein Befehl
// auf den I2C Bus und im Hold State werden die Wartezeiten abgearbeitet.


#include <Wire.h>
#include "knx.h"
#include <ESP8266WebServer.h>
#include <KonnektingDevice.h>
#define KNX_SERIAL  Serial  // swaped Serial on D7(GPIO13)=RX/GPIO15(D8)=TX 19200 baud


// ################################################
// ### DEBUG Configuration
// ################################################

// Define KONNEKTING Device related IDs
#define MANUFACTURER_ID 57005
#define DEVICE_ID 255
#define REVISION 0
#define KONNEKTING_SYSTEM_TYPE_SIMPLE

byte virtualEEPROM[128];

struct knx_data_in
{
  word R1_hoch;
  word R1_zu;
  word R1_stop;
  word R1_sperre;
};
knx_data_in Knx_In;

struct knx_data_out
{
  word R1_status;
  word F_kontakt;
};
knx_data_out Knx_Out;

unsigned char *p;
const byte Obj_in  = sizeof (Knx_In) / 2; // 2 bytes for 1 GA
const byte Obj_out = sizeof (Knx_Out) / 2; // 2 bytes for 1 GA

word individualAddress = P_ADDR(1, 2, 113);

// Definition of the Communication Objects attached to the device
KnxComObject KnxDevice::_comObjectsList[] = {
  /* Input */
  /* Suite-Index 0 : R1_hoch */       KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
  /* Suite-Index 1 : R1_zu    */      KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
  /* Suite-Index 2 : R1_stop */       KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
  /* Suite-Index 3 : R1_sperre*/      KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
  /* Suite-Index 4 : R1_status */       KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
  /* Suite-Index 5 : Fenster_kontakt*/ KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR)
};
const byte KnxDevice::_numberOfComObjects = sizeof (_comObjectsList) / sizeof (KnxComObject); // do no change this code
// Definition of parameter size
byte KonnektingDevice::_paramSizeList[] = {
  /* Param Index 0 */ PARAM_UINT16
};
const int KonnektingDevice::_numberOfParams = sizeof (_paramSizeList); // do no change this code

void Knx_Init(void) {
  Knx_In.R1_hoch       = G_ADDR(1, 2, 15);
  Knx_In.R1_zu         = G_ADDR(1, 2, 16);
  Knx_In.R1_stop       = G_ADDR(1, 2, 17);
  Knx_In.R1_sperre     = G_ADDR(1, 2, 18);

  Knx_Out.R1_status    = G_ADDR(1, 2, 19);
  Knx_Out.F_kontakt   = G_ADDR(1, 3, 5);
};


//Die IIC_Addr ist 0x47, oder höher, siehe Löt-Jumper PCB
uint8_t IIC_Addr_t = 0x47;

/*-----------------------------------------------------------------------------*/
//we do not need a ProgLED, but this function muss be defined
void progLed (bool state) {}; //nothing to do here

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
    if (timer1 > 600) T1_GA_long  = 1; // Serial.println(" -> laaaaaaaang. ");
    else              T1_GA_short = 1; // Serial.println(" -> kurz. ");
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
    if (timer1 > 600) T2_GA_long  = 1; // Serial.println(" -> laaaaaaaang. ");
    else              T2_GA_short = 1; // Serial.println(" -> kurz. ");
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
    if (timer1 > 600) T3_GA_long  = 1; // Serial.println(" -> laaaaaaaang. ");
    else              T3_GA_short = 1; // Serial.println(" -> kurz. ");
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
{ // keine Unterscheidung kurz /lang bei automatik toggle
  T4_Event_Falling = millis();
  timer1 = (T4_Event_Falling - T4_Event_Rising);
  if (timer1 > 10) // Bouncing passed
  {
    attachInterrupt(Taster4, ISR_T4, FALLING);
    T4_GA_short = 1; // Serial.println(" -> kurz. ");
    Sperre_local = 1;
  }
}
void IRAM_ATTR ISR_T5()
{
  T5_Event_Rising = millis();
  if ((T5_Event_Rising - T5_Event_Falling) > 10) // Bouncing passed
  {
    attachInterrupt(Kontakt, ISR_T5_RELEASE, RISING);
    T5_Event = 1;
    T5_State = 1;
  }
}
void IRAM_ATTR ISR_T5_RELEASE()
{
  T5_Event_Falling = millis();
  timer1 = (T5_Event_Falling - T5_Event_Rising);
  if (timer1 > 10) // Bouncing passed
  {
    attachInterrupt(Kontakt, ISR_T5, FALLING);
    T5_Event = 1;
    T5_State = 0;
  }
}


// ################################################
// ### KNX EVENT CALLBACK
// ################################################

void knxEvents(byte index) {
  // Empfangsobjekte Input
  R1_Old = Init;
  switch (index) {
    case 0: //R1_hoch
      Status_R1_hoch = Knx.read(index);
      if (R1_State != Sperre_hold)
      {
        if (R1_State == Ruhe_hold)     R1_State = Hoch_active;
        else {
          if (R1_State == Runter_hold) R1_State = Rw_rh_active;
        }
      }
      break;
    case 1: // R1_zu
      Status_R1_zu = Knx.read(index);
      if (R1_State != Sperre_hold)
      {
        if (R1_State == Ruhe_hold)   R1_State = Runter_active;
        else {
          if (R1_State == Hoch_hold) R1_State = Rw_hr_active;
        }
      }
      break;
    case 2: // R1_stop
      Status_R1_stop = Knx.read(index);
      if (R1_State != Sperre_hold)
      {
        R1_State = Ruhe_active;
      }
      break;
    case 3: // R1_sperre
      if (!Sperre_local)
        Status_R1_sperre = Knx.read(index); // Sperre via KNX
      else {
        Status_R1_sperre = T4_Short_State;     // Sperre via lokaler Taster
        Sperre_local    = 0;
      }
      if (Status_R1_sperre) R1_State = Sperre_active;
      else                 R1_State = Ruhe_active;
      break;
    default:
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

/////////////////////////////////////////////////////////
static void TriacSendCommand(uint8_t *Data)
{
  Wire.beginTransmission(IIC_Addr_t);
  Wire.write(Data[0]);
  Wire.write(Data[1]);
  Wire.write(Data[2]);
  Wire.endTransmission();
}
static void TriacSendSingle(uint8_t data0, uint8_t data1, uint8_t data2)
{
  Wire.beginTransmission(IIC_Addr_t);
  Wire.write(data0);
  Wire.write(data1);
  Wire.write(data2);
  Wire.endTransmission();
  //  Serial1.print("Wire send :");
  //  Serial1.print(data0);
  //  Serial1.print(data1);
  //  Serial1.println(data2);
}


/******************************************************************************
  function:   Set working mode
  parameter:  Mode:
                  0: Switch mode
                  1: Voltage regulation mode
  Info:
******************************************************************************/
void TriacSetMode(uint8_t Mode)
{ // Schreibe ins Register 0x01, 0: an/aus, 1: Dimmen
  uint8_t ch[4] = {0x01, 0x00, 0x00, 0x00};
  ch[2] = Mode & 0x01;
  TriacSendCommand(ch);
  // Serial1.print("Triac SetMode 100");
  // delay(100);
}

/******************************************************************************
  function:   Set channel enable
  parameter:  none: Switch always both channels
  Info:
******************************************************************************/
void TriacDisable(void)
{ // Schreibt ebenfalls in Register0x02, schaltet beide Kanäle aus
  uint8_t T_disable[4] = {0x02, 0x00, 0x00, 0x00};
  TriacSendCommand(T_disable);
  // delay(100);
}
void TriacEnable(void)
{ // Schreibt ebenfalls in Register0x02, schaltet beide Kanäle an
  uint8_t T_enable[4] = {0x02, 0x00, 0x03, 0x00};
  TriacSendCommand(T_enable);
  // Serial1.print("Triac enable 203");
  // delay(100);
}
uint8_t CH_EN[4] = {0x02, 0x00, 0x00, 0x00};
void TriacChannelEnable(uint8_t Channel)
{ // Register 0x02, siehe Channel enable oben
  if (Channel == 1) {
    CH_EN[2] |= 0x01;
    TriacSendCommand(CH_EN);
  } else if (Channel == 2) {
    CH_EN[2] |= 0x02;
    TriacSendCommand(CH_EN);
  }
  // delay(100);
}


/******************************************************************************
  function:   Set Voltage Regulation
  parameter:  Channel:
                  1: Channel 1
                  2: Channel 2
            Angle:
                   0~179 Conduction angle
  Info:
******************************************************************************/
uint8_t Angle1[4] = {0x03, 0x00, 0x00, 0x00};
uint8_t Angle2[4] = {0x04, 0x00, 0x00, 0x00};
//Parameter 4 Set conduction angle 0-179
//If in mode 1, then the conduction angle greater than 90 degrees will be on
//If set to 180 the actual effect is 0
void TriacDimmer(uint8_t Channel,  uint8_t Angle)
{
  if (Channel == 1) {
    Angle1[2] = Angle % 180;
    TriacSendCommand(Angle1);
  } else if (Channel == 2) {
    Angle2[2] = Angle % 180;
    TriacSendCommand(Angle2);
  }
  // delay(100);
}

// Switch on binary
void Triac1(uint8_t On_off)
{
  if (On_off)TriacSendSingle(3, 0, 0x91);
  else       TriacSendSingle(3, 0, 1);

}

void Triac2(uint8_t On_off)
{
  if (On_off)TriacSendSingle(4, 0, 0x91);
  else       TriacSendSingle(4, 0, 1);
}

/******************************************************************************
  function:   SCR Reset
  parameter:  Delay:
                    0~255 Delay in milliseconds before reset
  Info:
    Reset all settings except baud rate and grid frequency,
******************************************************************************/
void TriacReset(uint8_t Delay)
{ // Schreibe direkten Wert: 100ms
  uint8_t ch[4] = {0x06, 0x00, 0x00, 0x64};
  TriacSendCommand(ch);
  // delay(100);
}
void Triac_init(void) {
  TriacSetMode(0); // Switch Mode
  // delay(10);
  TriacChannelEnable(1);       // beide Kanäle an..
  TriacChannelEnable(2);

  // TriacEnable;     // beide Kanäle an..
  //TriacSetMode(0); // Switch Mode
  //TriacDimmer(1, 0); // Ch1 ausgedimmt
  //TriacDimmer(2, 0); // Ch2 ausgedimmt
  // Serial1.println("Triac loop init");
}

////////////////////////////////////////////////////////

void setup() {
  WiFi.forceSleepBegin();  // WLan aus
  delay(5000); //Warten auf Supercap
  //////////////////////////////////// Switch ///////////////////////////////////////////////////
  // Init Inputs
  pinMode(Taster1, INPUT_PULLUP);
  pinMode(Taster2, INPUT_PULLUP);
  pinMode(Taster3, INPUT_PULLUP);
  pinMode(Taster4, INPUT_PULLUP);
  pinMode(Kontakt, INPUT_PULLUP);
  // Attach interrupts
  attachInterrupt(Taster1, ISR_T1, FALLING);
  attachInterrupt(Taster2, ISR_T2, FALLING);
  attachInterrupt(Taster3, ISR_T3, FALLING);
  attachInterrupt(Taster4, ISR_T4, FALLING);
  attachInterrupt(Kontakt, ISR_T5, FALLING);
  

  //////////////////////////////////////////// KNX Part /////////////////////////////////////////
  memset(virtualEEPROM, 0xFF, 128); // check data size of Knx Struct!!

  Konnekting.setMemoryReadFunc(&readMemory);
  Konnekting.setMemoryWriteFunc(&writeMemory);
  Konnekting.setMemoryUpdateFunc(&updateMemory);
  Konnekting.setMemoryCommitFunc(&commitMemory);

  Knx_Init(); // Write GA's to RAM
  /******************************************************* PA ************************************************/
  // write hardcoded PA and GAs
  writeMemory(0,  0x7F);  //Set "not factory" flag
  writeMemory(1,  (byte)(individualAddress >> 8));
  writeMemory(2,  (byte)(individualAddress));
  /******************************************************* OUT ************************************************/
  // Write all receive GA to Eeprom
  p = (unsigned char*)&Knx_In;
  for (int index = 0; index < Obj_in; index++) {
    virtualEEPROM[(3 * index) + 11] = *p++; // Order is fliped in data structure..
    virtualEEPROM[(3 * index) + 10] = *p++;
    virtualEEPROM[(3 * index) + 12] = 0x80;
  }
  // Write all send GA to Eeprom
  p = (unsigned char*)&Knx_Out;
  for (int index = Obj_in; index < (Obj_in + Obj_out); index++) {
    virtualEEPROM[(3 * index) + 11] = *p++; // Order is fliped in data structure..
    virtualEEPROM[(3 * index) + 10] = *p++;
    virtualEEPROM[(3 * index) + 12] = 0x80;
  }
  // Initialize KNX
  Konnekting.init(KNX_SERIAL, &progLed, MANUFACTURER_ID, DEVICE_ID, REVISION);
  // Now serial is swiched to RX2/TX2!
  delay(500);
  Wire.begin(3, 1); //3 = Rx0 = SDA, 1 = Tx0 = SCL
  delay(500);
  Serial1.begin(115200);
  Serial1.println("I2c started");
  //////////////////////////////////////// Init Triac ///////////////////////////////////////////
  TriacSetMode(0); // Switch Mode
  TriacChannelEnable(1);       // beide Kanäle an..
  delay(50);
  TriacChannelEnable(2);
  TriacDimmer(1, 0); // Ch1 ausgedimmt
  TriacDimmer(2, 0); // Ch2 ausgedimmt
  // Serial1.println("Triac setup init");
  R1_State = Ruhe_active;
  R1_Old = Init;
  Sperre_local = 0;
  Serial1.println("Setup finished");
}

void loop() {
  Main_Clock++;
  /////////////////////////////////////////////////////////////////////////////////////
  Knx.task();
  /////////////////////////////////////////////////////////////////////////////////////
  if (R1_Old != R1_State) {
    Serial1.print("R1_State: ");
    Serial1.print(R1_State);
    Serial1.print("R1_Old: ");
    Serial1.print(R1_Old);
    Serial1.println(".");
  }

  // Auswirkungen State Machine
  switch (R1_State)
  {
    case Hoch_active:
      Triac1(1); //on
      Triac2(0); //off
      Runtimer = 0;
      R1_State = Hoch_hold;
      break;
    case Hoch_hold:
      if ((Main_Clock & Delay_Mask) == 0)Runtimer ++;
      if (Runtimer > Laufzeit) {
        R1_State = Ruhe_active;
        Knx.write(4, 1); // Schreibe Status 1 = oben
        // Serial1.println("R oben");
      }
      break;
    case Runter_active:
      Triac2(1); //on
      Triac1(0); //off
      Runtimer = 0;
      R1_State = Runter_hold;
      break;
    case Runter_hold:
      if ((Main_Clock & Delay_Mask) == 0)Runtimer ++;
      if (Runtimer > Laufzeit) {
        R1_State = Ruhe_active;
        Knx.write(4, 0); // Schreibe Status 0 = unten
        // Serial1.println("R unten");
      }
      break;
    case Ruhe_active:
      Ruhetimer = 0;
      Triac2(0); //off
      Triac1(0); //off
      R1_State = Ruhe_hold;
      break;
    case Ruhe_hold:
      if ((Main_Clock & Delay_Mask) == 0)Ruhetimer ++;
      break;
    case Rw_hr_active:
      Triac2(0); //off
      Triac1(0); //off
      R1_State = Rw_hr_hold;
      Pausentimer = 0;
      break;
    case Rw_hr_hold:
      if ((Main_Clock & Delay_Mask) == 0)Pausentimer ++;
      if (Pausentimer > Wechselzeit) {
        R1_State = Runter_active;
      }
      break;
    case Rw_rh_active:
      Triac2(0); //off
      Triac1(0); //off
      Pausentimer = 0;
      R1_State = Rw_rh_hold;
      break;
    case Rw_rh_hold:
      if ((Main_Clock & Delay_Mask) == 0)Pausentimer ++;
      if (Pausentimer > Wechselzeit) {
        R1_State = Hoch_active;
      }
      break;
    case Sperre_active:
      Triac2(0); //off
      Triac1(0); //off
      R1_State = Sperre_hold;
      break;
    case Sperre_hold:
      R1_State = Sperre_hold;
      break;
    default:
      Triac1(0); //off
      Triac2(0); //off
      break;
  }
////////////////////////////////////// Abfrage eigene Schalter ///////////////////////////////
  R1_Old = R1_State;
  if (T1_GA_short)
  {
    T1_GA_short = 0;
    knxEvents(0); // hoch
    // Serial1.println("T1");
    T1_Short_State = !T1_Short_State; // toggle state -> UM Taster Mode
  } else {
    if (T1_GA_long)
    {
      T1_GA_long = 0;
      // Aktion einfügen
      T1_Long_State = !T1_Long_State; // toggle state -> UM Taster Mode
    }
  }
  if (T2_GA_short)
  {
    T2_GA_short = 0;
    knxEvents(2); //Stop
    // Serial1.println("T2");
    T2_Short_State = !T2_Short_State; // toggle state -> UM Taster Mode
  } else {
    if (T2_GA_long)
    {
      T2_GA_long = 0;
      // Knx.write(5, T2_Long_State);
      T2_Long_State = !T2_Long_State; // toggle state -> UM Taster Mode
    }
  }
  if (T3_GA_short)
  {
    T3_GA_short = 0;
    knxEvents(1); // Runter
    // Serial1.println("T3");
    T3_Short_State = !T3_Short_State; // toggle state -> UM Taster Mode
  } else {
    if (T3_GA_long)
    {
      T3_GA_long = 0;
      // Knx.write(6, T3_Long_State);
      T3_Long_State = !T3_Long_State; // toggle state -> UM Taster Mode
    }
  }
  if (T4_GA_short)
  {
    T4_GA_short = 0;
    knxEvents(3);  // Toggle state Sperre
    // Serial1.println("T4 Toggle");
    T4_Short_State = !T4_Short_State; // toggle state -> UM Taster Mode
  }
  ///////////////////////////////////// Fensterkontakt auf Pin10 ////////////////////////////
  if (T5_Event)
  {
    T5_Event = 0;
    Knx.write(5, T5_State);  
  }
}
