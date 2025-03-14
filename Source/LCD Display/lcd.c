// --- Definitions ---

sfr  P2    = 0xA0;
sbit P2_6  = P2^6;
sbit P2_5  = P2^5;
sbit P2_4  = P2^4;
sbit P2_3  = P2^3;
sbit P2_2  = P2^2;
sbit P2_1  = P2^1;
sbit P2_0  = P2^0;

#define False    			0  
#define True     			1

// LCD connections
#define LCD_RS    P2_4         /* p1.4 LCD Register Select line           */
#define LCD_RW    P2_5         /* p1.5 LCD Read / Write line              */
#define LCD_E     P2_6         /* p1.6 LCD Enable line                    */
#define LCD_DB4   P2_0         /* high nibble of port 1 is used for data  */
#define LCD_DB5   P2_1         /* high nibble of port 1 is used for data  */
#define LCD_DB6   P2_2         /* high nibble of port 1 is used for data  */
#define LCD_DB7   P2_3         /* high nibble of port 1 is used for data  */

// LCd commands (as specified by LCD driver documentation)
#define LCD_CONFIG		      0x28
#define LCD_CLEAR		      0x01
#define LCD_HOME		      0x02
#define LCD_ENTRY_MODE		  0x06
#define LCD_DISPLAY_OFF		  0x08
#define LCD_CURSOR_ON 		  0x0A
#define LCD_DISPLAY_ON		  0x0C
#define LCD_CURSOR_BLINK	  0x0D
#define LCD_CURSOR_LINE		  0x0E
#define LCD_CURSOR_COM		  0x0F
#define LCD_CURSOR_LEFT		  0x10
#define LCD_CURSOR_RIGHT	  0x14
#define LCD_SHIFT_LEFT		  0x18
#define LCD_SHIFT_RIGHT		  0x1C
#define LCD_SET_CGRAM_ADDR	  0x40
#define LCD_SET_DDRAM_ADDR	  0x80


// --- Function Declarations ---
void LCD_WriteControl (unsigned char cmd);
void LCD_WriteData (unsigned char cmd);
void LCD_Cursor (char row, char column);
void LCD_DisplayScreen (void);
void LCD_DisplayCharacterAt(char a_char, char row, char column);
void LCD_NewCharacter(char pixels, unsigned char row, unsigned char addr);
void LCD_Init(void);


// lcddisp_main.c
extern void Sleep(unsigned int time);


// --- Global Variables ---
unsigned char data Screen[] =    "                "
								 "                "
								 "                "
								 "                ";


// --- Function Definitions ---
// Sending control commands to LCD driver (4-bit communication)
void LCD_WriteControl (unsigned char cmd)
{
	unsigned char LCD_comm = 0;
	static bit LCD_ready;

	LCD_RS = False;
	LCD_RW = False;

	LCD_comm = cmd >> 4;
	P2 &= 0xF0;
	P2 |= LCD_comm;

	LCD_E = True;
	LCD_E = False;

	LCD_comm = cmd & 0x0F;  
	P2 &=0xF0;      // P2 = P2 & 0xF0
	P2 |= LCD_comm;

	LCD_E = True;
	LCD_E = False;

	P2 |= 0x0F;     // P2 = P2 | 0x0F

	LCD_RW = True;
	LCD_RS = False;


	LCD_ready = 1;
	while (LCD_ready == 1)
	{
		LCD_E = True;
		LCD_ready = LCD_DB7;
		LCD_E = False;
		LCD_E = True;
		LCD_E = False;
	}
}

// Sending data commands to LCD driver (4-bit communication)
void LCD_WriteData (unsigned char cmd)
{
	unsigned char LCD_data = 0;
	static bit LCD_ready;

	LCD_RS = True;
	LCD_RW = False;

	LCD_data = cmd >> 4;
	P2 &= 0xF0;
	P2 |= LCD_data;

	LCD_E = True;
	LCD_E = False;

	LCD_data = cmd & 0x0F;  
	P2 &=0xF0;   
	P2 |= LCD_data;

	LCD_E = True;
	LCD_E = False;

	P2 |= 0x0F;

	LCD_RW = True;
	LCD_RS = False;

	LCD_ready = 1;
	while (LCD_ready == 1)
	{
		LCD_E = True;
		LCD_ready = LCD_DB7;
		LCD_E	= False;
		LCD_E	= True;
		LCD_E	= False;
	}
 }


/* How LCD driver interprets position of cursor 
*     1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16
*    ---------------------------------------------------------------
* 1 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
* 2 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
* 3 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
* 4 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
*    ---------------------------------------------------------------
*/
void LCD_Cursor (char row, char column)
 {
   if (row == 1) LCD_WriteControl (0x80 + column - 1);
   if (row == 2) LCD_WriteControl (0xc0 + column - 1);
   if (row == 3) LCD_WriteControl (0x90 + column - 1);
   if (row == 4) LCD_WriteControl (0xd0 + column - 1);
 }

// Display the global Screen[] variable on the LCD
void LCD_DisplayScreen (void)
{
	unsigned char i;
	for (i = 0; i < 64; i++) {
		LCD_Cursor((i >> 4) + 1, (i % 16) + 1);
		LCD_WriteData(*(Screen + i));
	}
}

// Display given character at a specified cell
void LCD_DisplayCharacterAt(char a_char, char row, char column) {
	LCD_Cursor(row, column);
	LCD_WriteData(a_char);
}


// Character is 7 rows of 5 bit pixels (7 chars)
// addr + 8 -> CGRAMAddr 
void LCD_NewCharacter(char pixels, unsigned char row, unsigned char addr) {
	LCD_WriteControl(LCD_SET_CGRAM_ADDR | (row + (8*addr)));
	LCD_WriteData(pixels);
}

// Initialize LCD with 4 rows and 4 bit communication
void LCD_Init(void)
 {
   Sleep(5);
   P2 = 0x83;
   LCD_E   = True;
   Sleep(1);
   LCD_E   = False;
   Sleep(2);
   LCD_E   = True;
   Sleep(1);
   LCD_E   = False;
   Sleep(1);
   LCD_E   = True;
   Sleep(1);
   LCD_E   = False;
   Sleep(1);
   LCD_DB4 = False;
   LCD_E   = True;
   Sleep(1);
   LCD_E   = False;
   Sleep(1);
   LCD_WriteControl(LCD_CONFIG);     
   LCD_WriteControl(LCD_CLEAR);      
   LCD_WriteControl(LCD_DISPLAY_OFF);
   LCD_WriteControl(LCD_DISPLAY_ON); 
   LCD_WriteControl(LCD_ENTRY_MODE); 
   LCD_WriteControl(LCD_CLEAR);      
 }