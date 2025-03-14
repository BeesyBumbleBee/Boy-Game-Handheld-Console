#include "addresses.h"

sfr TL1 = 0x8B;

// --- Definitions ---
#define AB_EMPTY ' '
#define AB_PLAYER_HEAD 0
#define AB_PLAYER_TAIL 1
#define AB_ASTEROID_ONE 2
#define AB_ASTEROID_TWO 3
#define AB_EXPLOSION 4


// --- Function Declarations ---
void AB_MovePlayer(unsigned char key_pressed);
void AB_MovePlayerUp(void);
void AB_MovePlayerDown(void);
void AB_CheckScore(void);
void AB_MoveAsteroids(void);
void AB_SpawnAsteroids(void);

// lcddispl_main.c
extern void Sleep(unsigned int time);
extern unsigned char Rand(unsigned char seed);

// lcd.c
extern void LCD_NewCharacter(char pixels, unsigned char row, unsigned char addr);
extern void LCD_SetScreen(char *new_screen);
extern void LCD_DisplayScreen(void);

// serialcomms_master.c
extern void SendToSlave(char slave_addr, char send_data);
extern void WaitForResponse(void);
extern void WaitForSend(void);

// storage_master.c
void LoadCharacters(unsigned char slave_addr, unsigned char id);
void LoadScreen(unsigned char slave_addr, unsigned char id, bit alt_scr, bit show_loading);

// --- Global Variables ---
unsigned char data AB_difficulty;
bit AB_GameLoop;

// lcddisp_main.c
extern bit enable_haptics;

// lcd.c
extern char data Screen[];

// serialcomms_master.c
extern unsigned char data RECV;
extern bit RECF;


// ASTEROID BELT CHARACTERS
// 0. Character (AB_PLAYER_HEAD)
// ----| ----- |----
//  000|       |0x00
//  000| 1     |0x10 
//  000| 1111  |0x1E
//  000| 11111 |0x1F
//  000| 1111  |0x1E
//  000| 1     |0x10
//  000|       |0x00
// ==================
// 1. Character (AB_PLAYER_TAIL) -> animated //ToDo
// ----| ----- |----
//  000|       |0x00
//  000|       |0x00 
//  000|     1 |0x01
//  000|    11 |0x03
//  000|     1 |0x01
//  000|       |0x00
//  000|       |0x00
// ==================
// 2. Character (AB_ASTEROID_ONE)
// ----| ----- |----
//  000|       |0x00
//  000|  11   |0x0C
//  000| 1111  |0x1E 
//  000| 11111 |0x1F
//  000| 11111 |0x1F
//  000|  11   |0x0C
//  000|       |0x00
// ==================
// 3. Character (AB_ASTEROID_TWO)
// ----| ----- |----
//  000|  111  |0x0E 
//  000| 1111  |0x1E 
//  000|  11   |0x0C
//  000|     1 |0x01
//  000| 1111  |0x1E
//  000| 11111 |0x1F
//  000|  1111 |0x0F
// ==================
// 4. Character (AB_EXPLOSION)
// ----| ----- |----
//  000| 1 1 1 |0x15
//  000|  111  |0x0E 
//  000| 11 11 |0x1B
//  000| 1 1 1 |0x15
//  000| 11 11 |0x1B
//  000|  111  |0x0E
//  000| 1 1 1 |0x15
// ==================

// --- Main Game Loop ---
void StartAsteroidBelt(void) {
	unsigned char data pressed_key;
	unsigned char data tick;
	unsigned char data game_speed; // the lower the faster
	tick = 0;
	AB_difficulty = 16;
	game_speed = 6;
	LoadCharacters(AB_STORAGE_ADDRESS, 2);
	LoadScreen(AB_STORAGE_ADDRESS, 2, 0, 1);
	
	LCD_DisplayScreen();
	AB_GameLoop = 1;
	while (AB_GameLoop) {
		// Each game tick:
		// I - ask keyboard for pressed button
		// II - move player character on screen
		// III - move asteroids 
		// IV - check if scored a point
		// V - spawn new asteroids (clear the last column before)
		// Then update LCD display
		// steps III - V only happen when the move_tick variable reaches certain value -> difficulty
		
		// I
		SendToSlave(KEYPAD_ADDRESS, 0x01);
		WaitForResponse();
		pressed_key = RECV;
		
		// II
		Sleep(10); // configure for best performance
		AB_MovePlayer(pressed_key);
		
		if (tick == game_speed) {
			// III
			AB_MoveAsteroids();
			
			// IV	
			AB_CheckScore();
			
			// V
			AB_SpawnAsteroids();
			tick = 0;
		} else {
			// TODO: calculate actual delay
			Sleep(4); // counteract the time it would take to do III-V
		}
		
		if (!AB_difficulty && game_speed >= 1) {
			game_speed--;
			AB_difficulty = 16;
		}
		
		tick++;
		LCD_DisplayScreen();
	}
}


// --- Function Definitions ---
void AB_MovePlayer(unsigned char key_pressed) {
	if (key_pressed == '2') {
		AB_MovePlayerUp();
	} else if (key_pressed == '8') {
		AB_MovePlayerDown();
	}
}

void AB_MovePlayerUp(void) {
	char data row;
	for (row = 0; row < 3; row++) {
		if (Screen[( (row + 1) * 16 ) + 1] == AB_PLAYER_HEAD) {
			Screen[( (row + 1) * 16 ) + 1] = AB_EMPTY;
			Screen[( (row + 1) * 16 ) + 0] = AB_EMPTY;
			Screen[( (row + 0) * 16 ) + 1] = AB_PLAYER_HEAD;
			Screen[( (row + 0) * 16 ) + 0] = AB_PLAYER_TAIL;
		}
	}
}

void AB_MovePlayerDown(void) {
	unsigned char data row;
	for (row = 3; row > 0; row--) {
		if (Screen[( (row - 1) * 16 ) + 1] == AB_PLAYER_HEAD) {
			Screen[( (row - 1) * 16 ) + 1] = AB_EMPTY;
			Screen[( (row - 1) * 16 ) + 0] = AB_EMPTY;
			Screen[( (row + 0) * 16 ) + 1] = AB_PLAYER_HEAD;
			Screen[( (row + 0) * 16 ) + 0] = AB_PLAYER_TAIL;
		}
	}
}

void AB_MoveAsteroids(void) {
	unsigned char data col;
	unsigned char data row;
	for (row = 0; row < 4; row++) {
		if (Screen[(row * 16) + 1] == AB_PLAYER_HEAD) {
			if (Screen[(row * 16) + 2] != AB_EMPTY) {
				Screen[(row * 16) + 1] = AB_EXPLOSION; 				
				AB_GameLoop = 0;
			}
		} else {
			Screen[(row * 16) + 0] = Screen[(row * 16) + 1];
			Screen[(row * 16) + 1] = Screen[(row * 16) + 2];
		}
		
		for (col = 2; col < 15; col++) {
			Screen[(row * 16) + col] = Screen[(row * 16) + col + 1];
		}
	}
}

void AB_CheckScore(void) {
	if ( 
		(Screen[1]  == AB_PLAYER_HEAD &&  Screen[17] != AB_EMPTY) ||
		(Screen[17] == AB_PLAYER_HEAD && (Screen[1]  != AB_EMPTY  || Screen[33] != AB_EMPTY)) ||
		(Screen[33] == AB_PLAYER_HEAD && (Screen[17] != AB_EMPTY  || Screen[49] != AB_EMPTY)) ||
	    (Screen[49] == AB_PLAYER_HEAD &&  Screen[33] != AB_EMPTY)
	   ) 
	{	
		SendToSlave(SCORES_ADDRESS, 0x01);
		if (enable_haptics) SendToSlave(MOTORS_ADDRESS, 0x72);
		if (enable_haptics) SendToSlave(MOTORS_ADDRESS, 0x71);
		AB_difficulty--;
	}
}

void AB_SpawnAsteroids(void) {
	unsigned char data i;
	unsigned char data val;
	for (i = 0; i < 4; i++) {
		val = Rand(TL1);
		Screen[15 + 16*i] = AB_EMPTY;
		Screen[15 + 16*i] = val % 5 == 0 ? (val % 2 == 0 ? AB_ASTEROID_ONE : AB_ASTEROID_TWO) : AB_EMPTY;
	}
}

