//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Timer Count Capture
// Application Overview - This application showcases Timer's count capture
//                        feature to measure frequency of an external signal.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Timer_Count_Capture_Application
// or
// docs\examples\CC32xx_Timer_Count_Capture_Application.pdf
//
//*****************************************************************************

// Driverlib includes
#include "hw_timer.h"
#include <stdint.h>
#include <stdbool.h>
#include "stdlib.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "timer.h"
#include "rom.h"
#include "rom_map.h"
#include "pin.h"

// Common interface includes
#include "uart_if.h"
#include "pinmux.h"


#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME        "Timer Count Capture"
#define TIMER_FREQ      80000000

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
static unsigned long g_ulSamples[2];
static unsigned long g_ulFreq;
void inputInt();
const double temp = 1.0/80.0;
//Stores the pulse length
volatile uint32_t pulse=0;
//Tells the main code if the a pulse is being read at the moment
int echowait=0;
#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

void
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t\t  CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
void
BoardInit(void)
{
#ifndef USE_TIRTOS
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();
}


void proxWhile() {
    while(1)
      {
        if(echowait != 1){
          MAP_GPIOPinWrite(GPIOA1_BASE, PIN_64,PIN_64);
          UtilsDelay(266);
          MAP_GPIOPinWrite(GPIOA1_BASE, PIN_64, ~PIN_64);
          while(echowait != 0);
          float percentage =0.0;
          int ad;
          float total =40.0;
          pulse =(uint32_t)(temp * pulse);
          pulse = pulse / 58;
          percentage =(float)(pulse/total);
          percentage= percentage*100;
          ad=(int)percentage;
          Report("distance = %2dcm \n\r" , pulse);
          Report("distance = %2dcm \n\r" , ad);
          Report("Percentage = %f -/- \n\r" , percentage);
        }
        UtilsDelay(8000000);
      }
}

//*****************************************************************************
//
//! Main  Function
//
//*****************************************************************************
int main()
{
    BoardInit();
    PinMuxConfig();
    InitTerm();
    ProxInit();
    proxWhile();
}

void ProxInit() {
    MAP_TimerConfigure(TIMERA2_BASE, TIMER_CFG_PERIODIC_UP);
    MAP_TimerEnable(TIMERA2_BASE,TIMER_A);
    DisplayBanner(APP_NAME);
    MAP_GPIOIntEnable(GPIOA2_BASE,PIN_08);
    MAP_GPIOIntTypeSet(GPIOA2_BASE, PIN_08,GPIO_BOTH_EDGES);
    MAP_TimerIntRegister(TIMERA2_BASE,TIMER_A,inputInt);
    MAP_GPIOIntRegister(GPIOA2_BASE,inputInt);
}

void inputInt(){
  MAP_GPIOIntClear(GPIOA2_BASE,PIN_08);
  /*
    If it's a rising edge then set he timer to 0
    It's in periodic mode so it was in some random value
  */
  if (GPIOPinRead(GPIOA2_BASE,PIN_08) == 2){
    HWREG(TIMERA2_BASE + TIMER_O_TAV ) = 0; //Loads value 0 into the timer.
     // TimerLoadSet(TIMERA2_BASE,TIMER_A,0xFFFFFFFF);
     long ad = MAP_TimerLoadGet(TIMERA2_BASE,TIMER_A);
    //Report("load = %dcm \n\r" , ad);
    TimerEnable(TIMERA2_BASE,TIMER_A);
    echowait=1;
  }
  /*
    If it's a falling edge that was detected, then get the value of the counter
  */
  else{
    pulse = TimerValueGet(TIMERA2_BASE,TIMER_A);
    long af = GPIOPinRead(GPIOA2_BASE,PIN_08);
    //Report("pin = %dcm \n\r" , af);
   // Report("distance = %dcm \n\r" , pulse);//record value
    TimerDisable(TIMERA2_BASE,TIMER_A);
    echowait=0;
  }


}
