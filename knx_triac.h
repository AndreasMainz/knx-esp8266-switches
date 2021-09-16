#include <ESP8266WebServer.h>
// RAM
uint32_t LED_Color[8];

// RAM KNX
byte Status_R1_hoch, Status_R1_zu, Status_R1_stop ,Status_R1_sperre, Status_R1_status, Status_HT_Vorne_Zustand, Status_HT_Vorne_Riegel;
byte Status_HT_Hinten_Zustand, Status_HT_Hinten_Riegel;
byte Kontakt_Fenster_Kueche, Kontakt_Fenster_Kueche_old, Status_Fenster_WG3, Status_Fenster_WG4, Status_Fenster_WG5, Status_Fenster_Anbau1, Status_Fenster_Anbau2;
byte Status_Fenster_Anbau3, Status_Fenster_Kueche,Sperre_local;

// RAM
volatile int T1_Event_Falling, T1_Event_Rising, T1_GA_short, T1_GA_long, T1_Short_State, T1_Long_State;
volatile int T2_Event_Falling, T2_Event_Rising, T2_GA_short, T2_GA_long, T2_Short_State, T2_Long_State;
volatile int T3_Event_Falling, T3_Event_Rising, T3_GA_short, T3_GA_long, T3_Short_State, T3_Long_State;
volatile int T4_Event_Falling, T4_Event_Rising, T4_GA_short, T4_GA_long, T4_Short_State, T4_Long_State;
volatile int T5_Event, T5_State, T5_Event_Falling, T5_Event_Rising;

// Functions
void IRAM_ATTR ISR_T1(void);
void IRAM_ATTR ISR_T2(void);
void IRAM_ATTR ISR_T3(void);
void IRAM_ATTR ISR_T4(void);
void IRAM_ATTR ISR_T5(void);

void IRAM_ATTR ISR_T1_RELEASE(void);
void IRAM_ATTR ISR_T2_RELEASE(void);
void IRAM_ATTR ISR_T3_RELEASE(void);
void IRAM_ATTR ISR_T4_RELEASE(void);
void IRAM_ATTR ISR_T5_RELEASE(void);



// Hw resource
int timer1, Active_Led,Last_Led;
long Main_Clock;

typedef enum R_state
{
  Init = 0,
  Ruhe_active,
  Ruhe_hold,
  Hoch_active,
  Hoch_hold,
  Runter_active,
  Runter_hold,
  Rw_hr_active,
  Rw_hr_hold,
  Rw_rh_active,
  Rw_rh_hold,
  Sperre_active,
  Sperre_hold,
};

R_state R1_State, R1_Old;
uint16_t Pausentimer;
uint16_t Runtimer, Event_served;
uint32_t Ruhetimer;

#define Delay_Mask 0xFF
#define Laufzeit 8000
#define Wechselzeit 30
#define Taster1 5 
#define Taster2 4 
#define Taster3 14
#define Taster4 12
#define Kontakt 10
