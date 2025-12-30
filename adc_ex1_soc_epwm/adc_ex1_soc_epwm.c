//#############################################################################
//
// FILE:   adc_ex1_soc_epwm.c
//
// TITLE:  ADC ePWM Triggering
//
//! \addtogroup bitfield_example_list
//! <h1>ADC ePWM Triggering</h1>
//!
//! This example sets up ePWM1 to periodically trigger a conversion on ADCA.
//!
//! \b External \b Connections \n
//!  - A1 should be connected to a signal to convert
//!
//! \b Watch \b Variables \n
//! - \b adcAResults - A sequence of analog-to-digital conversion samples from
//!   pin A1. The time between samples is determined based on the period
//!   of the ePWM timer.
//!
//
//#############################################################################
//
//
// 
// C2000Ware v6.00.00.00
//
// Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
#include "f28x_project.h"


//
// Defines
//
#define RESULTS_BUFFER_SIZE     256

//
// Globals
//
uint16_t adcAResults[RESULTS_BUFFER_SIZE];   // Buffer for results
uint16_t adcBResults[RESULTS_BUFFER_SIZE];   // Buffer for results
uint16_t adcCResults[RESULTS_BUFFER_SIZE];   // Buffer for results
uint16_t index;                              // Index into result buffer
volatile uint16_t bufferFull;                // Flag to indicate buffer is full
float R1 = 47000;
float R2 = 3300;
float R5 = 22000;
float R6 = 3300;
float na_d = 12;
float Vfs = 3.3;
float Vout; 
float Vin;
float iL;
float G = 0.1;
float Vq = 0.65;
float bit_range = 4095;
//
// Function Prototypes
//
void initADC(void);
void initEPWM(void);
void initADCSOC(void);
__interrupt void adcA1ISR(void);

//
// Main
//
void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    InitSysCtrl();

    //
    // Initialize GPIO
    //
    InitGpio();

    //
    // Disable CPU interrupts
    //
    DINT;

    //
    // Initialize the PIE control registers to their default state.
    // The default state is all PIE interrupts disabled and flags
    // are cleared.
    //
    InitPieCtrl();

    //
    // Disable CPU interrupts and clear all CPU interrupt flags:
    //
    IER = 0x0000;
    IFR = 0x0000;

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    InitPieVectTable();

    //
    // Map ISR functions
    //
    EALLOW;
    PieVectTable.ADCA1_INT = &adcA1ISR;     // Function for ADCA interrupt 1
    EDIS;

    //
    // Configure the ADC and power it up
    //
    initADC();

    //
    // Configure the ePWM
    //
    initEPWM();

    //
    // Setup the ADC for ePWM triggered conversions on channel 1
    //
    initADCSOC();

    //
    // Enable global Interrupts and higher priority real-time debug events:
    //
    IER |= M_INT1;  // Enable group 1 interrupts

    EINT;           // Enable Global interrupt INTM
    ERTM;           // Enable Global realtime interrupt DBGM

    //
    // Initialize results buffer
    //
    for(index = 0; index < RESULTS_BUFFER_SIZE; index++)
    {
        adcAResults[index] = 0;
        adcBResults[index] = 0;
        adcCResults[index] = 0;

    }

    index = 0;
    bufferFull = 0;

    //
    // Enable PIE interrupt
    //
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    //
    // Sync ePWM
    //
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;

    //
    // Take conversions indefinitely in loop
    //
    while(1)
    {
      
    }
}

//
// initADC - Function to configure and power up ADCA.
//
void initADC(void)
{
    //
    // Setup VREF as internal
    //
    SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3);

    EALLOW;

    AdcaRegs.ADCCTL2.bit.PRESCALE = 0; //ADC clocks the same as system clock
   
    //
    // Set pulse positions to late
    //
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    //
    // Power up the ADC and then delay for 1 ms
    //
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;
    EDIS;

    DELAY_US(1000);
}

//
// initEPWM - Function to configure ePWM1 to generate the SOC.
//
void initEPWM(void)
{
    EALLOW;

    EPwm7Regs.ETSEL.bit.SOCAEN = 1;
    EPwm7Regs.ETSEL.bit.SOCASEL = 3;
    EPwm7Regs.ETPS.bit.SOCPSSEL = 1;
    EPwm7Regs.ETSOCPS.bit.SOCAPRD2 = 10;
    EPwm7Regs.ETPS.bit.SOCAPRD = 1;
    EPwm7Regs.TBCTL.bit.CLKDIV = 0b000;
    EPwm7Regs.TBCTL.bit.HSPCLKDIV = 0b000;
    EPwm7Regs.TBCTL.bit.CTRMODE = 0b10; // Updown Count mode
    EPwm7Regs.TBPRD = 1250;             // (Update for lab4) Period of time-based conter or Max value the counter should count to for PWM: Nr = 1/(2*fsw*Tclk) Tclk = 1/150MHz
    EPwm7Regs.CMPA.bit.CMPA = 625;      // (Update for lab4) Compare value Vc to the waveform: Vc = Nr*D
    EPwm7Regs.AQCTLA.bit.CAD = 0b10;    // In count updown mode, during counting down, when Nr = CPMA, set output to high
    EPwm7Regs.AQCTLA.bit.CAU = 0b01;    // In count updown mode, during counting up, when Nr = CPMA, set output to low
    EPwm7Regs.AQCTLB.bit.CAD = 0b01;    // Complementary waveform of A
    EPwm7Regs.AQCTLB.bit.CAU = 0b10;    // Complementary waveform of A
    EPwm7Regs.AQCTLA.bit.PRD = 0b00;
    EPwm7Regs.AQCTLA.bit.ZRO = 0b00;
    EPwm7Regs.AQCTLB.bit.PRD = 0b00;
    EPwm7Regs.AQCTLB.bit.ZRO = 0b00;

    GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0b01;  //Enable EPWMA on pin GPIO-12
    GpioCtrlRegs.GPAGMUX1.bit.GPIO12 = 0b00; //Select peripheral 1 on pin GPIO-12
    GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 0b01;  //Enable EPWMB on pin GPIO-13
    GpioCtrlRegs.GPAGMUX1.bit.GPIO13 = 0b00; //Select peripheral 1 on pin GPIO-13
    EPwm7Regs.DBCTL.bit.IN_MODE = 0b00;     //Use EPWMA as source for both rising and falling edge
    EPwm7Regs.DBCTL.bit.POLSEL = 0b10;      //Invert EPWMB
    EPwm7Regs.DBCTL.bit.OUT_MODE = 0b11;    //Fully enable DBM (for both RED and FED)
    EPwm7Regs.DBFED.bit.DBFED = 15;         //#of cycles during dead time = fclk*Tdead
    EPwm7Regs.DBRED.bit.DBRED = 15;



    EDIS;
}

//
// initADCSOC - Function to configure ADCA's SOC0 to be triggered by ePWM1.
//
void initADCSOC(void)
{
    //
    // Select the channels to convert and the end of conversion flag
    //
    EALLOW;

    // Vout
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;     // SOC0 will convert pin A0
                                           // 0:A0  1:A1  2:A2  3:A3
                                           // 4:A4   5:A5   6:A6   7:A7
                                           // 8:A8   9:A9   A:A10  B:A11
                                           // C:A12  D:A13  E:A14  F:A15
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 20;     // Sample window is 15 SYSCLK cycles sample window size 100ns
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 0x11;   // Trigger on ePWM7 SOCA

    // Vin
    AdcaRegs.ADCSOC1CTL.bit.CHSEL = 4;
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = 20;     // Sample window is 15 SYSCLK cycles sample window size 100ns
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL = 0x11;

    
 
    // Current
    AnalogSubsysRegs.AGPIOCTRLH.bit.GPIO227 = 1;
    GpioCtrlRegs.GPHAMSEL.bit.GPIO227 = 1;
    AdcaRegs.ADCSOC2CTL.bit.CHSEL = 9;
    AdcaRegs.ADCSOC2CTL.bit.ACQPS = 20;     // Sample window is 15 SYSCLK cycles sample window size 100ns
    AdcaRegs.ADCSOC2CTL.bit.TRIGSEL = 0x11;


    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0; // End of SOC0 will set INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;   // Enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; // Make sure INT1 flag is cleared

    EDIS;
}

//
// adcA1ISR - ADC A Interrupt 1 ISR
//
__interrupt void adcA1ISR(void)
{
    //
    // Add the latest result to the buffer
    // ADCRESULT0 is the result register of SOC0
    adcAResults[index++] = AdcaResultRegs.ADCRESULT0;
    adcBResults[index++] = AdcaResultRegs.ADCRESULT1;
    adcCResults[index++] = AdcaResultRegs.ADCRESULT2;

    Vout = (R1+R2)/R2 * AdcaResultRegs.ADCRESULT0 * Vfs /bit_range;
    Vin = (R5 + R6)/R6 * AdcaResultRegs.ADCRESULT1 * Vfs /bit_range;
    iL = 1/G * ((AdcaResultRegs.ADCRESULT2) / bit_range * Vfs - Vq);

    //
    // Set the bufferFull flag if the buffer is full
    //
    if(RESULTS_BUFFER_SIZE <= index)
    {
        index = 0;
        bufferFull = 1;
    }

    //
    // Clear the interrupt flag
    //
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;

    //
    // Check if overflow has occurred
    //
    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1; //clear INT1 overflow flag
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear INT1 flag
    }

    //
    // Acknowledge the interrupt
    //
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//
// End of File
//
