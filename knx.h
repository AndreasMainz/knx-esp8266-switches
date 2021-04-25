#include <ESP8266WiFi.h>

// RAM
volatile int T1_Event_Falling, T1_Event_Rising, T1_GA_short, T1_GA_long, T1_Short_State, T1_Long_State;
volatile int T2_Event_Falling, T2_Event_Rising, T2_GA_short, T2_GA_long, T2_Short_State, T2_Long_State;
volatile int T3_Event_Falling, T3_Event_Rising, T3_GA_short, T3_GA_long, T3_Short_State, T3_Long_State;
volatile int T4_Event_Falling, T4_Event_Rising, T4_GA_short, T4_GA_long, T4_Short_State, T4_Long_State;

// Functions
void IRAM_ATTR ISR_T1(void);
void IRAM_ATTR ISR_T2(void);
void IRAM_ATTR ISR_T3(void);
void IRAM_ATTR ISR_T4(void);

void IRAM_ATTR ISR_T1_RELEASE(void);
void IRAM_ATTR ISR_T2_RELEASE(void);
void IRAM_ATTR ISR_T3_RELEASE(void);
void IRAM_ATTR ISR_T4_RELEASE(void);

// Hw resource
int timer1;
