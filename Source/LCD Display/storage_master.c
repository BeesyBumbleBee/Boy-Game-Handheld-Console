
// --- Function Declarations ---
// lcd.c
extern void LCD_DisplayCharacterAt(char a_char, char row, char column);
extern void LCD_NewCharacter(char pixels, unsigned char row, unsigned char addr);

// serialcomms_master.c
extern void SendToSlave(char slave_addr, char send_data);
extern void WaitForResponse(void);
extern void WaitForSend(void);


// --- Global Variables ---
// lcd.c
extern char data Screen[];

// serialcomms_master.c
extern unsigned char data RECV;


// --- Function Definitions ---
void LoadCharacters(unsigned char slave_addr, unsigned char id) {
	unsigned char data __storage_iterator;
	unsigned char instr = 0xC0 | (id << 3);
	for (__storage_iterator = 0; __storage_iterator < 64; __storage_iterator++) {
		SendToSlave(slave_addr, instr + (__storage_iterator / 8));
		WaitForResponse();
		LCD_NewCharacter(RECV, __storage_iterator % 8, __storage_iterator / 8);
	}
}

void LoadScreen(unsigned char slave_addr, unsigned char id, bit alt_scr, bit show_loading) {
	unsigned char data __storage_iterator;
	unsigned char instr = 0x80 | (id << 3) | ((unsigned char)alt_scr << 2);
	for (__storage_iterator = 0; __storage_iterator < 64; __storage_iterator++) {
		if (show_loading) LCD_DisplayCharacterAt('#', 4 ,(__storage_iterator / 4) + 1);
		SendToSlave(slave_addr, instr + (__storage_iterator / 16));
		WaitForResponse();
		Screen[__storage_iterator] = RECV;
	}
}