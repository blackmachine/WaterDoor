/*
 * File:   main.cpp
 * Author: Kreyl
 * Project: Salem Box
 *
 * Created on Mar 22, 2015, 01:23
 */

#include <buttons.h>
#include "main.h"
#include "Sequences.h"
#include "led.h"
#include "kl_lib_L15x.h"
#include "SimpleSensors.h"

App_t App;
LedBlinker_t Led({GPIOA, 0});   // Just LED to blink
// PWM output for IR LED
PinOutputPWM_t<1, invInverted, omOpenDrain> IRLed(GPIOB, 15, TIM11, 1);
// 12V output
PinOutputPushPull_t Output12V(GPIOA, 6);
// Time for rising and falling
struct SnsData_t {
    systime_t TimeMoveIn, TimeMoveOut;
    bool HasChanged;
};

SnsData_t SnsData[PIN_SNS_CNT];

// Universal VirtualTimer callback
void TmrGeneralCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI((eventmask_t)p);
    chSysUnlockFromIsr();
}

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V8);
    // 12 MHz*4 = 48; 48/3 = 16
    Clk.SetupPLLMulDiv(pllMul4, pllDiv3);
    Clk.SetupFlashLatency(16);  // Setup Flash Latency for clock in MHz
    Clk.SetupBusDividers(ahbDiv1, apbDiv1, apbDiv1);
    uint8_t ClkResult = Clk.SwitchToPLL();
    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();

    // ==== Init Hard & Soft ====
    Uart.Init(115200);
    Uart.Printf("\rWaterDoor AHB=%u", Clk.AHBFreqHz);
    if(ClkResult != OK) Uart.Printf("\rClk failure: %u", ClkResult);

    App.InitThread();
    // IR LED generator
    IRLed.Init();
    IRLed.SetFrequencyHz(56000);
    IRLed.Set(1);

    PinSensors.Init();

    Led.Init();
    Led.StartSequence(lsqBlink);
    Output12V.Init();
    // Main cycle
    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);

        if(EvtMsk == EVTMSK_SNS) {
            for(uint8_t i=0; i<PIN_SNS_CNT; i++) {
                if(SnsData[i].HasChanged) {
                    SnsData[i].HasChanged = false;
                    Uart.Printf("\rSns%02u: In=%u; Out=%u", i, SnsData[i].TimeMoveIn, SnsData[i].TimeMoveOut);
                }
            }
            Uart.Printf("\r");
        }

//        chThdSleepMilliseconds(207);
//        Output12V.SetHi();
//        chThdSleepMilliseconds(207);
//        Output12V.SetLo();
    } // while true
}

#if 1 // ==== Motion sensors ====
void ProcessSensors(PinSnsState_t *PState, uint32_t Len) {
    bool HasChanged = false;
    systime_t Now = chTimeNow();
    for(uint8_t i=0; i<PIN_SNS_CNT; i++) {
        if(PState[i] == pssFalling and SnsData[i].TimeMoveOut != Now) {
            HasChanged = true;
            SnsData[i].TimeMoveOut = Now;
            SnsData[i].HasChanged = true;
        }
        if(PState[i] == pssRising and SnsData[i].TimeMoveIn != Now) {
            HasChanged = true;
            SnsData[i].TimeMoveIn = Now;
            SnsData[i].HasChanged = true;
        }
    }
    if(HasChanged) App.SignalEvt(EVTMSK_SNS);
}


#endif
