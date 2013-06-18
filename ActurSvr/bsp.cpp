//////////////////////////////////////////////////////////////////////////////
//This file was created by modifying the file, 
//qp/examples/qp/qp_dpp_qk/bsp.cpp received from Quantum Leaps, LLC.
//Sabao Akutsu. Mar. 27, 2013
// 
//Product : ActuaServer (On Arduino, Real-time control some cheap actuators)
//
//Copyright (C) 2002-2012 Quantum Leaps, LLC. All rights reserved.
//Copyright (C) 2013  Sabao Akutsu.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#include "qp_port.h"
#include "bsp.h"
#include "Arduino.h"                                   // Arduino include file
#include "qDevice.h"

using namespace QP;

Q_DEFINE_THIS_FILE

#define SAVE_POWER

#define USER_LED_ON()      (PORTB |= (1 << (5)))
#define USER_LED_OFF()     (PORTB &= ~(1 << (5)))
#define USER_LED_TOGGLE()  (PORTB ^= (1 << (5)))

#define TICK_DIVIDER       ((F_CPU / BSP_TICKS_PER_SEC / 1024) - 1)

#if TICK_DIVIDER > 255
#   error BSP_TICKS_PER_SEC too small
#elif TICK_DIVIDER < 2
#   error BSP_TICKS_PER_SEC too large
#endif

#ifdef Q_SPY
uint8_t l_TIMER2_COMPA;
#endif

// Serial Interface  -------------------------------------------------------------
extern SerialInterface* p_si;
// ISRs ----------------------------------------------------------------------
ISR(TIMER2_COMPA_vect) {
    // No need to clear the interrupt source since the Timer2 compare
    // interrupt is automatically cleard in hardware when the ISR runs.

    QK_ISR_ENTRY();                  // inform QK kernel about entering an ISR

    QF::TICK(&l_TIMER2_COMPA);                // process all armed time events
    
    p_si->On_ISR();
    
    QK_ISR_EXIT();                    // inform QK kernel about exiting an ISR
}

//............................................................................
void BSP_init(void) {
    DDRB  = 0xFF;                     // All PORTB pins are outputs (user LED)
    PORTB = 0x00;                                        // drive all pins low    
    
    //Serial.begin(115200, SERIAL_8E1);
    Serial.begin(115200);
    
    if (QS_INIT((void *)0) == 0) {       // initialize the QS software tracing
        Q_ERROR();
    }
    
    QS_OBJ_DICTIONARY(&l_SysTick_Handler);
}
//............................................................................
void QF::onStartup(void) {
          // set Timer2 in CTC mode, 1/1024 prescaler, start the timer ticking
    TCCR2A = ((1 << WGM21) | (0 << WGM20));
    TCCR2B = (( 1 << CS22 ) | ( 1 << CS21 ) | ( 1 << CS20 ));        // 1/2^10
    ASSR &= ~(1<<AS2);
    TIMSK2 = (1 << OCIE2A);                 // Enable TIMER2 compare Interrupt
    TCNT2 = 0;
    OCR2A = TICK_DIVIDER;     // must be loaded last for Atmega168 and friends
}
//............................................................................
void QF::onCleanup(void) {
}
//............................................................................
void QK::onIdle() {

    QF_INT_DISABLE();
    USER_LED_ON();     // toggle the User LED on Arduino on and off, see NOTE1
    USER_LED_OFF();
    QF_INT_ENABLE();

#ifdef SAVE_POWER

    SMCR = (0 << SM0) | (1 << SE);  // idle sleep mode, adjust to your project
    __asm__ __volatile__ ("sleep" "\n\t" :: );
    SMCR = 0;                                              // clear the SE bit

#endif
}

//............................................................................
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    QF_INT_DISABLE();                                // disable all interrupts
    USER_LED_ON();                                  // User LED permanently ON
    asm volatile ("jmp 0x0000");    // perform a software reset of the Arduino
}

//////////////////////////////////////////////////////////////////////////////
// NOTE1:
// The Arduino's User LED is used to visualize the idle loop activity.
// The brightness of the LED is proportional to the frequency of invcations
// of the idle loop. Please note that the LED is toggled with interrupts
// locked, so no interrupt execution time contributes to the brightness of
// the User LED.
//
// NOTE2:
// The QF_onIdle() callback is called with interrupts *locked* to prevent
// a race condtion of posting a new event from an interrupt while the
// system is already committed to go to sleep. The only *safe* way of
// going to sleep mode is to do it ATOMICALLY with enabling interrupts.
// As described in the "AVR Datasheet" in Section "Reset and Interrupt
// Handling", when using the SEI instruction to enable interrupts, the
// instruction following SEI will be executed before any pending interrupts.
// As the Datasheet shows in the assembly example, the pair of instructions
//     SEI       ; enable interrupts
//     SLEEP     ; go to the sleep mode
// executes ATOMICALLY, and so *no* interrupt can be serviced between these
// instructins. You should NEVER separate these two lines.
//
