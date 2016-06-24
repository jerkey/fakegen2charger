//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "compiler_defs.h"
#include "C8051F500_defs.h"            // SFR declarations
#include <stdio.h>
#include <intrins.h>


#define SYSCLK       24000000          // System clock speed in Hz

#define RxOk  0x10                     // Receive Message Successfully
#define TxOk  0x08                     // Transmitted Message Successfully

SBIT (LED, SFR_P1, 2);					// LED = 1 turns on the LED
SBIT (CANmode,  SFR_P1, 5);				// Silent = 1

unsigned char pot;					//


//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------

void main (void)
{
	unsigned int a = 0;					// GP Int
	unsigned char b = 0;				// GP byte
	unsigned int c = 0;					// GP Int
	unsigned char count = 0;			// Regulates timing from Master/Slave tx/rx mode
//	unsigned char IntID;				// CAN message number
	unsigned char crc;					//
//	unsigned char pot;					//
	unsigned char pss = 0x10;			// Status Byte
	unsigned char can[8];				// CAN Frame data byte array

	SFRPAGE = ACTIVE_PAGE;              // Set for PCA0MD

	PCA0MD &= ~0x40;                    // Disable Watchdog Timer

	SFRPAGE = CONFIG_PAGE;

	OSCICN = 0x87;                      // Set internal oscillator divider to 1 (24mhz)

//	P0SKIP |= 0x01;                     // Skip P0.0 (VREF)
	P1SKIP |= 0x08;                     // Skip P1.3 (ADC input)

	P0MDIN = 0xff;						//Port set to Digital mode
	P1MDIN = 0xff;						//Port set to Digital mode
	P2MDIN = 0xff;						//Port set to Digital mode
	P3MDIN = 0xff;						//Port set to Digital mode
//	P0MDIN  &= ~0x01;                   // Set VREF to analog

//	P0MDOUT  |= 0x54;                   // P0.2 (PTX), P0.4 (TXD) & P0.6 (CAN0 TX) is push-pull
//	P1MDOUT  |= 0x24;                   // P1.5 (Can Mode) & P1.2 (LED) is push-pull
	P0MDOUT = 0x54;                   // P0.2 (PTX), P0.4 (TXD) & P0.6 (CAN0 TX) is push-pull
	P1MDOUT = 0xA4;                   // P1.7 (EVout), P1.5 (Can Mode) & P1.2 (LED) is push-pull

	P2MDOUT = 0x00;

	XBR0     = 0x03;                    // Enable UART and CAN0 on Crossbar
	XBR2     = 0x40;                    // Enable Crossbar and weak pull-ups

	CANmode = 0;

	SFRPAGE = ACTIVE_PAGE;

	//Timer 2
   TMR2CN  = 0x00;                     // Stop Timer2; Clear TF2;
                                       // use SYSCLK as timebase, 16-bit
                                       // auto-reload
   CKCON  |= 0x10;                     // Select SYSCLK for Timer 2 source
   TMR2RL  = 65535 - (SYSCLK / 10000); // Init reload value for 10uS
   TMR2    = 0xFFFF;                   // Set to reload immediately
   TR2     = 1;                        // Start Timer2

	//ADC
   ADC0CF |= 0x01;                     // Set GAINEN = 1
   ADC0H   = 0x04;                     // Load the ADC0GNH address
   ADC0L   = 0x6C;                     // Load the upper byte of 0x6CA to 
                                       // ADC0GNH
   ADC0H   = 0x07;                     // Load the ADC0GNL address
   ADC0L   = 0xA0;                     // Load the lower nibble of 0x6CA to 
                                       // ADC0GNL
   ADC0H   = 0x08;                     // Load the ADC0GNA address
   ADC0L   = 0x01;                     // Set the GAINADD bit
   ADC0CF &= ~0x01;                    // Set GAINEN = 0

   ADC0CN = 0x03;                      // ADC0 disabled, normal tracking,
                                       // conversion triggered on TMR2 overflow
                                       // Output is right-justified

   REF0CN = 0x33;                      // Enable on-chip VREF and buffer
                                       // Set voltage reference to 2.25V

   ADC0MX = 0x0B;                      // Set ADC input to P1.3 (pot)
 //  ADC0MX = 0x01;                      // Set ADC input to P0.1

   ADC0CF = ((SYSCLK / 3000000) - 1) << 3;   // Set SAR clock to 3MHz

   EIE1 |= 0x04;                       // Enable ADC0 conversion complete int.

   AD0EN = 1;                          // Enable ADC0
	RSTSRC = 0x04;                      // Enable missing clock detector

//	EA = 0;                             // Disable global interrupts
LED=!LED;
	SFRPAGE  = CAN0_PAGE;               // All CAN register are on page 0x0C

//	CAN0CN |= 0x01;                     // Start Intialization mode
	CAN0CN = 0x01;
//	CAN0CN |= 0x4E;                     // Enable Error and Module, access to bit timing register
	CAN0CN = 0x43;
	CAN0BT = 0x3AC2;                   // Based on 24 Mhz CAN clock, set the CAN bit rate to 500 Kbps
//	CAN0BT = 0x0380;                    // Based on 24 Mhz CAN clock, set the CAN bit rate to 500 Kbps
	CAN0CN = 0x03;

// RX Generic frames

	CAN0IF1CM = 0x00F0;					// cmd mask
//	CAN0IF1CM = 0x00B0;					// cmd mask
	CAN0IF1M1 = 0x0000;					// mask regs
	CAN0IF1M2 = 0x0000;					// mask regs
	CAN0IF1A1 = 0x0000;					// arbitration regs
	CAN0IF1MC = 0x1488;					// msg ctl reg - No mask, Enable RX Int, FIFO member, Size=8
	CAN0IF1A2 = 0x8000;					// MsgVal=1
	CAN0IF1CR = 1;						// msg ID - Start command request
	while (CAN0IF1CRH & 0x80) {}		// Poll on Busy bit
// TX9 Generic frame

	CAN0IF2CM = 0x00F0;
	CAN0IF2M1 = 0x0000;
	CAN0IF2M2 = 0x0000;					// mask regs
	CAN0IF2MC = 0x0088;					// length=8
	CAN0IF2A1 = 0x0000;
	CAN0IF2A2 = 0xA000; 					// message ID
	CAN0IF2CR = 31;						// message opbject #
	while (CAN0IF2CRH & 0x80) {}           // Poll on Busy bit

//	CAN0CN &= ~0x41;                    // Return to Normal Mode and disable access to bit timing register
	CAN0CN = 0x02;						// Return to Normal Mode and disable access to bit timing register
//	EIE2 |= 0x02;                       // Enable CAN interupts
	CAN0STAT |= TxOk;

//	SendStr("\fReady!\n");

	SFRPAGE = ACTIVE_PAGE;              // Set for PCA0MD
	PCA0MD  &= ~0x40;					// Disable watchdog timer
/*	PCA0L    = 0x00;					// Set lower byte of PCA counter to 0
	PCA0H    = 0x00;					// Set higher byte of PCA counter to 0
	PCA0CPL5 = 0xFF;					// Write offset for the WDT
	PCA0MD  |= 0x40;					// Enable the WDT
*/
	a=0x500;

//	SendStr("\fReady!\n");

	SFRPAGE = ACTIVE_PAGE;              // Set for PCA0MD
	EA = 1;

	while (1) {

	SFRPAGE = ACTIVE_PAGE;              // Set for PCA0MD
	PCA0CPH5 = 0x00;					// Reset WDT
//	pot=ADC0;


	SFRPAGE  = CAN0_PAGE;               // All CAN register are on page 0x0C
	if ((CAN0STAT & RxOk) && 0)
		{
//		IntID = CAN0IID;			// Read which message object caused the interrupt
		CAN0IF1CM = 0x007F;			// Read all of message object to IF1, Clear IntPnd and newData
		CAN0IF1CR = 1;				// Start command request to actually clear the interrupt
		while (CAN0IF1CRH & 0x80) {}	// Poll on Busy bit
		CAN0STAT &= ~RxOk;
			CAN0IF1MCH &= ~0x40;
		b = 0x0F & CAN0IF1MC;
		crc = (0xF0 & CAN0IF1MC << 4) | (0x07 & CAN0IF1A2H >> 2);
		a = 0x7FF & CAN0IF1A2 >> 2;
		can[0] = CAN0IF1DA1L;
		can[1] = CAN0IF1DA1H;
		can[2] = CAN0IF1DA2L;
		can[3] = CAN0IF1DA2H;
		can[4] = CAN0IF1DB1L;
		can[5] = CAN0IF1DB1H;
		can[6] = CAN0IF1DB2L;
		can[7] = CAN0IF1DB2H;
		}
	else if (CAN0STAT & TxOk)
		{
//LED=!LED;
		a++;
		if (a>0x509) a=0x501;

		for (c=0;c<20000;c++) {}
		CAN0STAT &= ~TxOk;

		SFRPAGE  = CAN0_PAGE;               // All CAN register are on page 0x0C
		CAN0IF2A2 = 0x0000;					// Set MsgVal = 0
		CAN0IF2MC = 0x0080 | 8;				// length
		CAN0IF2A2 = 0xA000 | (a << 2);		// message ID
//		CAN0IF2CR = 31;						// message opbject #
//		while (CAN0IF2CRH & 0x80) {}           // Poll on Busy bit

//		CAN0IF2DA1L = rand()>>7;
		CAN0IF2DA1L = pot;
		CAN0IF2DA1H = rand()>>7;
		CAN0IF2DA2L = rand()>>7;
		CAN0IF2DA2H = rand()>>7;
		CAN0IF2DB1L = rand()>>7;
		CAN0IF2DB1H = rand()>>7;
		CAN0IF2DB2L = rand()>>7;
		CAN0IF2DB2H = rand()>>7;
		CAN0IF2CM = 0x00F7;					// command mask
		CAN0IF2CR = 9;						// message opbject # (command request)
		while (CAN0IF2CRH & 0x80) {}		// Poll on Busy bit
//			while (!(CAN0STAT & TxOk)) {}
			}
	}
}



INTERRUPT (ADC_ISR, INTERRUPT_ADC0_EOC)
{
	static S32 accumulator = 0;         // Accumulator for averaging
	static U8 measurements = 0;     // Measurement counter
	U8 SFRPAGE_save = SFRPAGE;
	SFRPAGE = ACTIVE_PAGE;

	accumulator += ADC0;                // Add most recent sample
	measurements--;                     // Subtract counter

//	AD0INT = 0;                         // Clear ADC0 conv. complete flag

	if (measurements == 0)
		{
		pot = accumulator / 4110;
		accumulator=0;
LED=!LED;
		}
	AD0INT = 0;                         // Clear ADC0 conv. complete flag

	SFRPAGE = SFRPAGE_save;             // Restore SFRPAGE
}
