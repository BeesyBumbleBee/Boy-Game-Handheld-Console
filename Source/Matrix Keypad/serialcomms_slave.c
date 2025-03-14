#include <REGX52.H>

// Led indicator - keypad specific
#define LED P0_0

// Address of this slave controller
#define SADDR 0x01

// --- Function Declarations ---
void SendSlave(unsigned char send_data);
void InitSerial(void);

void Serial_ISR(void);


// --- Global Variables ---
unsigned char RECV;
bit RECF;
bit SENF;


// --- Function Definitions ---
void SendSlave(unsigned char send_data) {
	P3_4 = 1;
	TI = 0;
	TB8 = 0;
	SBUF = send_data;
	LED = 1;
}

void InitSerial(void) {
	RECF = 0;
	RECV = 0;
	// SERIAL PORT (uses T2)
	P3_4 = 0;
	SCON = 0xF0; // SM2 = 1 for slave controller
	T2CON = 0x30;
	TH2 = RCAP2H = 0xFF;
	TL2 = RCAP2L = 0xDC;
	ES = 1;
	TR2 = 1;
	TI = 0;
	PS = 1;	
}


// --- Interrupt Handlers ---
void Serial_ISR(void) interrupt 4
{
	if (TI) {
		TI = 0;
		P3_4 = 0;
		SENF = 1;
	} else {
		RECV = SBUF;
		if (SM2 && RECV == SADDR) {
			RECV = 0;
			SM2 = 0;
			LED = 0;
		} else if (!SM2) {
			RECF = 1;
			SM2 = 1;
		}
		RI = 0;
	}
}