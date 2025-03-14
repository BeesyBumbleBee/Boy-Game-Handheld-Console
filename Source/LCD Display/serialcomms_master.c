#include <REGX52.H>

/*
sfr SCON    = 0x98;
sfr SBUF    = 0x99;
sfr T2CON   = 0xC8;
sfr TL2     = 0xCC;
sfr TH2     = 0xCD;
sfr RCAP2L  = 0xCA;
sfr RCAP2H  = 0xCB;
sbit ES   = 0xAC; 
sbit PS   = 0xBC;
sbit TR2  = 0xCA;
sbit RI   = 0x98;
sbit TI   = 0x99;
sbit RB8  = 0x9A;
sbit TB8  = 0x9B;
sbit P3_4 = 0xB4;
*/

// --- Function Declarations ---
void InitSerial(void);
void WaitForResponse(void);
void WaitForSend(void);
void SendToSlave(char slave_addr, char send_data);
void Serial_ISR();


// --- Global Variables ---
unsigned char data RECV;
bit RECF;
bit SENF;


// --- Function Definitions ---
void WaitForResponse(void) {
	RECF = 0;
	while (!RECF) {;}
	RECF = 0;
}

void WaitForSend(void) {
	SENF = 0;
	while (!SENF) {;}
	SENF = 0;
}

void SendToSlave(char slave_addr, char send_data) {
	P3_4 = 1;
	TI = 0;
	TB8 = 1;
	SBUF = slave_addr;
	
	WaitForSend();
	
	P3_4 = 1;
	TI = 0;
	TB8 = 0;
	SBUF = send_data;
		
	WaitForSend();
}

void InitSerial(void) {
	RECF = 0;
	SENF = 0;
	RECV = 0;
	// SERIAL PORT (uses T2)
	P3_4 = 0;
	SCON = 0xD0; // SM2 = 0 for master controller
	T2CON = 0x30;
	TH2 = RCAP2H = 0xFF;
	TL2 = RCAP2L = 0xDC;
	ES = 1;
	TR2 = 1;
	TI = 0;
	PS = 1;	
}



// --- Interrupt Handlers ---
void Serial_ISR() interrupt 4
{
	if (TI) {
		SENF = 1;
		TI = 0;
		P3_4 = 0;
	} else {
		RECF = 1;
		RECV = SBUF;
		RI = 0;
	}
}