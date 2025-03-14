#include <REGX52.H>


// --- Definitions ---
#define KEYPAD P2


// --- Function Declarations ---
void init(void);
unsigned char get_key (unsigned char port);

// serialcomms_slave.c
extern void SendSlave (unsigned char data_send);
extern void InitSerial (void);

// storage_slave.c
extern void AccessStorage (unsigned char addr);

// --- Global Variables ---
// serialcomms_slave.c
extern unsigned char data RECV;
extern bit data RECF;


// --- Main Function ---
void main(void) 
{	
	unsigned char i;
	unsigned char key_pressed;
	init();
	while (1)
	{

		
		if (RECF) {
			if (RECV & 0x80) 
			{
				AccessStorage(RECV);
				RECV = 0;
				RECF = 0;
				continue;
			}
			
			switch (RECV) {
				case 0x01:
					for (i = 0; i < 4; i++)
					{
						KEYPAD = 0xFF;
						KEYPAD ^= 0x10 << i;
						
						if ((KEYPAD & 0xF) != 0xF) {
							key_pressed = get_key(KEYPAD);
						}
					}
					SendSlave(key_pressed);
					key_pressed = 0;
			}
			RECF = 0;
		}
	}
}


// --- Function Definitions ---
unsigned char get_key(unsigned char port)
{
	switch (port) {
	case 0xE7: return '1';
	case 0xEB: return '2';
	case 0xED: return '3';
	case 0xD7: return '4';
	case 0xDB: return '5';
	case 0xDD: return '6';
	case 0xB7: return '7';
	case 0xBB: return '8';
	case 0xBD: return '9';
	case 0x77: return '*';
	case 0x7B: return '0';
	case 0x7D: return '#';
	default: return 0;
	}
}

void init(void) {
	InitSerial();
	
	EA = 1;
}