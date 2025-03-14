#include "addresses.h"

sfr TL1 = 0x8B;
sfr TH1 = 0x8D;

// --- Definitions ---
#define ED_EMPTY ' '
#define ED_ENEMY_ONE 0
#define ED_ENEMY_TWO 1
#define ED_PLAYER 2
#define ED_EARTH_UL 3
#define ED_EARTH_UR 4
#define ED_EARTH_DL 5
#define ED_EARTH_DR 6
#define ED_EXPLOSION 7
#define ED_MISSILE '-'


// --- Function Declarations ---
void ED_MovePlayer(unsigned char pressed_key);
void ED_SpawnMissile(unsigned char pressed_key);
void ED_MoveMissile(bit left);
void ED_MoveEnemies(bit left);
void ED_SpawnEnemies(void);

// lcddisp_main.c
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
bit ED_GameLoop;
unsigned char data ED_difficulty;

// lcddisp_main.c
extern bit enable_haptics;

// lcd.c
extern char data Screen[];

// serialcomms_master.c
extern unsigned char data RECV;
extern bit RECF;


// EARTH DEFENSE CHARACTERS
// 0. Character (ED_ENEMY_TWO)
// ----| ----- |----
//  000|       |0x00
//  000|       |0x00 
//  000| 11    |0x18
//  000| 11111 |0x1F
//  000| 11    |0x18
//  000|       |0x00
//  000|       |0x00
// ==================
// 1. Character (ED_ENEMY_ONE) -> animated //ToDo
// ----| ----- |----
//  000|       |0x00
//  000|  111  |0x0E 
//  000|  1 1  |0x0A
//  000| 11111 |0x1F
//  000| 11111 |0x1F
//  000|       |0x00
//  000|       |0x00
// ==================
// 2. Character (ED_PLAYER)
// ----| ----- |----
//  000|   1   |0x04
//  000|  111  |0x0E
//  000|  111  |0x1E 
//  000| 1 1 1 |0x15
//  000|  111  |0x0E
//  000|  111  |0x0E
//  000|   1   |0x04
// ==================
// 3. Character (ED_EARTH_UL)
// ----| ----- |----
//  000|       |0x00 
//  000|       |0x00 
//  000|   111 |0x07
//  000|  1111 |0x0F
//  000| 11111 |0x1F
//  000| 11111 |0x1F
//  000| 11111 |0x1F
// ==================
// 4. Character (ED_EARTH_UR)
// ----| ----- |----
//  000|       |0x00
//  000|       |0x00 
//  000| 111   |0x1C
//  000| 1111  |0x1E
//  000| 11111 |0x1F
//  000| 11111 |0x1F
//  000| 11111 |0x1F
// ==================
// 5. Character (ED_EARTH_DL)
// ----| ----- |----
//  000| 11111 |0x1F
//  000| 11111 |0x1F 
//  000| 11111 |0x1F
//  000|  1111 |0x0F
//  000|   111 |0x07
//  000|       |0x00
//  000|       |0x00
// ==================
// 6. Character (ED_EARTH_DR)
// ----| ----- |----
//  000| 11111 |0x1F
//  000| 11111 |0x1F 
//  000| 11111 |0x1F
//  000| 1111  |0x1E
//  000| 111   |0x1C
//  000|       |0x00
//  000|       |0x00
// ==================
// 7. Character (ED_EXPLOSION)
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
void StartEarthDefense(void) {
	unsigned char data pressed_key;
	unsigned char data tick;
	unsigned char data game_speed; // the lower the faster
	ED_difficulty = 8;
	tick = 0;
	game_speed = 8;
	LoadCharacters(ED_STORAGE_ADDRESS, 4);
	LoadScreen(ED_STORAGE_ADDRESS, 4, 0, 1);
	
	LCD_DisplayScreen();
	ED_GameLoop = 1;
	while(ED_GameLoop) {
		// Each game tick:
		// I    - ask keyboard for pressed button
		// II   - move player character
		// III  - spawn player missile
		// IV   - move player missile
		// V    - move enemies 
		// VI   - spawn enemies
		// III - V happends when tick % game_speed == 0
		
		// I
		SendToSlave(KEYPAD_ADDRESS, 0x01);
		WaitForResponse();
		pressed_key = RECV;
		Sleep(10);
		
		// II
		ED_MovePlayer(pressed_key);
		
		// III
		ED_SpawnMissile(pressed_key);

		if (tick % game_speed == 0) {
			
			// IV
			ED_MoveMissile(0);
			ED_MoveMissile(1);
			
			// V
			ED_MoveEnemies(0);
			ED_MoveEnemies(1);
			
			// VI
			ED_SpawnEnemies();
			
			tick = 0;
		} else {
			Sleep(1);
		}
		
		if (!ED_difficulty && game_speed > 3) {
			game_speed--;
			ED_difficulty = 8;
		}			
		tick++;
		LCD_DisplayScreen();
	}
}


// --- Function Definitions ---
void ED_MovePlayer(unsigned char pressed_key) {
	unsigned char data i;
	unsigned char data row;
	unsigned char data col;
	unsigned char data incr;
	
	switch (pressed_key) {
		case '1':
			row = 0;
			incr = 1;
			col = 6;
		break;
		case '3':
			row = 0;
			incr = 1;
			col = 9;
		break;
		case '7':
			row = 3;
			incr = -1;
			col = 6;
		break;
		case '9':
			row = 3;
			incr = -1;
			col = 9;
		break;
		default:
			return;
	}
	
	for (i = 0; i < 3; i++) {
		if (Screen[( (row + incr) * 16 ) + col] == ED_PLAYER) {
			Screen[( (row + incr) * 16 ) + col] = ED_EMPTY;
			Screen[( (row + 0) * 16 ) + col] = ED_PLAYER;
		}
		row += incr;
	}
}

void ED_SpawnMissile(unsigned char pressed_key) {
	unsigned char data col;
	unsigned char data row;
	unsigned char data incr;
	if (pressed_key == '4') {
		col = 5;
		incr = 1;
	} else if (pressed_key == '6') {
		col = 10;
		incr = -1;
	} else {
		return;
	}
	
	for (row = 0; row < 4; row++) {
		if (Screen[(row*16) + col + incr] == ED_PLAYER) 
			if (Screen[(row*16) + col] == ED_ENEMY_ONE || Screen[(row*16) + col] == ED_ENEMY_TWO) {
				Screen[(row*16) + col] = ED_EXPLOSION;

				SendToSlave(SCORES_ADDRESS, 0x01);
				if (enable_haptics) SendToSlave(MOTORS_ADDRESS, 0x72);
				if (enable_haptics) SendToSlave(MOTORS_ADDRESS, 0x71);
				ED_difficulty--;
			} else {
				Screen[(row*16) + col] = ED_MISSILE;
			}
	}
}

void ED_MoveMissile(bit left) {
	unsigned char data col;
	unsigned char data row;
	unsigned char data cell;
	unsigned char data i;
	unsigned char data incr;
	
	// if going right
	incr = -1;
	col = 15;
	if (left) {
		incr = 1;
		col = 0;
	}
	for (row = 0; row < 4; row++) {
		cell = (row << 4) + col;
		// move enemies 
		for (i = 0; i < 8; i++) {
			if (Screen[cell] == ED_EXPLOSION) Screen[cell] = ED_EMPTY;
			
			if (Screen[cell + incr] == ED_MISSILE) {
				if (i == 0) Screen[cell] = ED_EMPTY;
				if (Screen[cell] == ED_ENEMY_ONE || Screen[cell] == ED_ENEMY_TWO) {
					Screen[cell] = ED_EXPLOSION;
					
					SendToSlave(SCORES_ADDRESS, 0x01);
					if (enable_haptics) SendToSlave(MOTORS_ADDRESS, 0x02 | ((0x07 - i) << 4));
					if (enable_haptics) SendToSlave(MOTORS_ADDRESS, 0x01 | ((0x07 - i) << 4));
					ED_difficulty--;
					
				} else {
					Screen[cell] = Screen[cell + incr];
				}
				Screen[cell + incr] = ED_EMPTY;
			}
			cell += incr;
		}
	}
}

void ED_MoveEnemies(bit left) {
	unsigned char data col;
	unsigned char data row;
	unsigned char data cell;
	unsigned char data i;
	unsigned char data incr;
	
	// if going right
	incr = 1;
	col = 8;
	if (left) {
		incr = -1;
		col = 7;
	}
	for (row = 0; row < 4; row++) {
		cell = (row << 4) + col;
		// move enemies 
		for (i = 0; i < 7; i++) {
			if (Screen[cell + incr] == ED_ENEMY_TWO || Screen[cell + incr] == ED_ENEMY_ONE) {
				if (Screen[cell] == ED_PLAYER || i == 0) {
					if (i == 0) Screen[(((row >> 1) + 1) << 4) + col] = ED_EXPLOSION;
					else Screen[cell] = ED_EXPLOSION;
					ED_GameLoop = 0;
				} else if (Screen[cell] == ED_MISSILE) {
					Screen[cell] = ED_EXPLOSION;
					
					SendToSlave(SCORES_ADDRESS, 0x01);
					if (enable_haptics) SendToSlave(MOTORS_ADDRESS, 0x02 | ((0x0F - i + 2) << 4));
					if (enable_haptics) SendToSlave(MOTORS_ADDRESS, 0x01 | ((0x0F - i + 2) << 4));
					ED_difficulty--;
					
				} else {
					Screen[cell] = Screen[cell + incr];
				}
				Screen[cell + incr] = ED_EMPTY;
			}
			cell += incr;
		}
	}
}

void ED_SpawnEnemies(void) {
	unsigned char data row;
	unsigned char data val;
	
	for (row = 0; row < 4; row++) {
		val = Rand(TL1);
		Screen[(row * 16) + 15] = ED_EMPTY;
		Screen[(row * 16) + 15] = val % 11 == 0 ? ED_ENEMY_TWO : ED_EMPTY;
	}
	for (row = 0; row < 4; row++) {
		val = Rand(TL1);
		Screen[(row * 16)] = ED_EMPTY;
		Screen[(row * 16)] = val % 13 == 0 ? ED_ENEMY_ONE : ED_EMPTY;
	}
}
