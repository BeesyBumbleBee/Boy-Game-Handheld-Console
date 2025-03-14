#include <REGX52.H>
#include "addresses.h"


// --- Function Declarations --- 
unsigned char Rand(unsigned char seed);
void Sleep(unsigned int time);
void init(void);
void scoreScreen(unsigned char game_id);
void welcomeScreen(void);

// lcd.c
extern void LCD_Init(void);
extern void LCD_Cursor (char row, char column);
extern void LCD_DisplayCharacterAt(char a_char, char row, char column);
extern void LCD_DisplayScreen (void);
extern void LCD_WriteControl (unsigned char cmd);
extern void LCD_WriteData (unsigned char cmd);
extern void LCD_NewCharacter (char *character, unsigned char addr);

// serialcomms_master.c
extern void WaitForResponse(void);
extern void Send(char send_data, bit masked);
extern void SendToSlave(char slave_addr, char send_data);
extern void InitSerial(void);

// storage_master.c
void LoadScreen(unsigned char slave_addr, unsigned char id, bit alt_scr, bit show_loading);

// asteroid_belt.c
extern void StartAsteroidBelt(void);

// earth_defense.c
extern void StartEarthDefense(void);


// --- Global Variables ---
bit enable_haptics;

// lcd.c
extern char data Screen[];

// serialcomms_master.c
extern unsigned char data RECV;
extern data bit RECF;


// --- Main Function ---
void main(void)
{
	unsigned char SELECT;	
	init();
	LCD_Init();
	SELECT = 0; // setting 0x80 bit means a select menu has to be loaded

	welcomeScreen();
	Sleep(60);
	SELECT = 0x80;
	while (1) 
	{
		if (SELECT & 0x80) {
			LoadScreen(MENUS_STORAGE_ADDRESS, 0, 1, 0);
			LCD_DisplayScreen();
			SELECT &= 0x7F;
		}
		
		LCD_DisplayCharacterAt(' ', 3, 1);
		LCD_DisplayCharacterAt(' ', 2, 1);
		if (SELECT & 0x0F) {
			LCD_DisplayCharacterAt('>', 3, 1);	
		} else {
			LCD_DisplayCharacterAt('>', 2, 1);	
		}
			
		SendToSlave(KEYPAD_ADDRESS, 0x01); // get user input 
		WaitForResponse();
		switch (RECV) 
		{
			case '2':
				SELECT &= 0x80;
				SELECT |= 0x00;
			break;
			case '8':
				SELECT &= 0x80;
				SELECT |= 0x01;
			break;
			case '0':
				switch (SELECT & 0x0F) 
				{
					case 0:
						StartAsteroidBelt();
						SendToSlave(SCORES_ADDRESS, 0x02);
					break;
					case 1:
						StartEarthDefense();
						SendToSlave(SCORES_ADDRESS, 0x12);
					break;
				}
				scoreScreen(SELECT & 0x0F);
				SELECT = 0x80;
			break;
		}
		RECV = 0;
	}
}


// --- Function Definitions ---
void Sleep(unsigned int time) // T0
{	
	TR0 = 0;
	if (time < 60) {
		TL0 = 0xFF - (time<<2);
		TH0 = 0xFF - (time<<2);
		TR0 = 1;
		while (!TF0) {;}
		TF0 = 0;
		TR0 = 0;
	} else {
		time /= 60;
		while (time > 0)
		{
			TL0 &= 0x0;
			TH0 &= 0x0;
			TR0 = 1;
			while (!TF0) {;}
			TF0 = 0;
			TR0 = 0;
			time--;
		}
	}
}

unsigned char Rand(unsigned char seed) 
{
	seed = (seed << 5) ^ seed;
	return ((seed * (seed * seed * 41 + 61) + 113) ^ seed);
}

void init(void) 
{
	enable_haptics = 1;
	
	EX1 = 1; // e/d INT0 interrupts
	IT1 = 1; // falling edge activation
	
	// Set T0 mode 1 -> sleep
	TMOD |= 0x01;
	
	// SET T1 mode 2 -> random number generator*
	TMOD |= 0x20;
	TH1 = 0x00;
	TR1 = 1;
	ET1 = 0;
	
	InitSerial();
	
	EA = 1; // enable interrupts
}

void scoreScreen(unsigned char game_id) 
{
	LoadScreen(MENUS_STORAGE_ADDRESS, 1, 0, 0);
	LCD_DisplayScreen();
	SendToSlave(SCORES_ADDRESS, 0x03 | (game_id << 4));
	WaitForResponse();
	LCD_DisplayCharacterAt(RECV / 100 + '0', 2, 13);
	LCD_DisplayCharacterAt(RECV / 10  + '0', 2, 14);
	LCD_DisplayCharacterAt(RECV % 10  + '0', 2, 15);
	SendToSlave(SCORES_ADDRESS, 0x04 | (game_id << 4));
	WaitForResponse();
	LCD_DisplayCharacterAt(RECV / 100 + '0', 3, 13);
	LCD_DisplayCharacterAt(RECV / 10  + '0', 3, 14);
	LCD_DisplayCharacterAt(RECV % 10  + '0', 3, 15);
	RECV = 0;
		
	while (RECV != '5') 
	{
		SendToSlave(KEYPAD_ADDRESS, 0x01);
		WaitForResponse();
	}
}

void welcomeScreen(void) {
	unsigned char i;
	const unsigned char code welcome_screen[] = {0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
												 0x20, 0x20, 0x20, 0x20, 0x42, 0x4F, 0x59, 0x20, 0x47, 0x41, 0x4D, 0x45, 0x20, 0x20, 0x20, 0x20,
												 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
												 0x20, 0x53, 0x2E, 0x4F, 0x4C, 0x45, 0x20, 0x57, 0x41, 0x54, 0x20, 0x32, 0x30, 0x32, 0x35, 0x20};
	for (i = 0; i < 64; i++) {
		LCD_Cursor((i >> 4) + 1, (i % 16) + 1);
		LCD_WriteData(*(welcome_screen + i));
	}		
}


// --- Interrupt Handlers ---
void INT1_ISR () interrupt IE1_VECTOR
{
	enable_haptics = 1;
}