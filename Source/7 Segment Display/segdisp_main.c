#include <REGX52.H>
#define ANIMATION_DELAY 25


// --- Function Declarations --- 
unsigned char getCharCode(unsigned char letter);
void setDisplay(unsigned char left, unsigned char right);	
void displayLetters(unsigned char left, unsigned char right);
void initScores(unsigned char* scores, unsigned char len);
void init(void);
void displayAnimation(void);
void sleep(void);

void Timer1_ISR(void);
void INT0_ISR(void);

// serialcomms_slave.c
extern void SendSlave(char data_send);
extern void InitSerial();

// storage_slave.c	
void AccessStorage(unsigned char addr);


// --- Global Variables ---
unsigned char xdata right_segment _at_ 0xFD00; // 1st digit in seg Disp
unsigned char xdata left_segment  _at_ 0xFE00; // 2nd digit in seg Disp
unsigned char data  on_display[2];
unsigned char data  animation_select;

// serialcomms_slave.c
extern unsigned char data RECV;
extern bit data RECF;

// --- Global Constants ---
const unsigned char code CHAR_LOOKUP[] = {119, 124, 57, 94, 121, 113, 111, 118, 48, 30, 118, 56, 21, 84, 63, 115, 103, 80, 109, 120, 62, 42, 28, 118, 110, 91, 63, 6, 91, 79, 102, 109, 125, 7, 127, 111};
const unsigned char code RADIX_LOOKUP[17] = "0123456789ABCDEF";
const unsigned char code ANIMATION_LOOKUP[] = 
	{ 
		0x63, 0x5C, 0x00, 0x00, 0x63, 0x5C, 0x00, 0x00, 0x63, 0x5C, 0x00, 0x00, 0x63, 0x5C, 0x00, 0x00,
		0x61, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x50, 0x58, 0x1c, 0x0e, 0x07, 0x23,
		0x30, 0x79, 0x4f, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 0x62, 0x54, 0x1c, 0x08,
		0x39, 0x31, 0x29, 0x19, 0x38, 0x39, 0x39, 0x39, 0x39, 0x31, 0x29, 0x19, 0x38, 0x39, 0x39, 0x39,
		0x00, 0x00, 0x5C, 0x63, 0x00, 0x00, 0x5C, 0x63, 0x00, 0x00, 0x5C, 0x63, 0x00, 0x00, 0x5C, 0x63,
		0x00, 0x40, 0x42, 0x43, 0x23, 0x31, 0x38, 0x1c, 0x4c, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x79, 0x4f, 0x06, 0x00, 0x01, 0x23, 0x62, 0x54, 0x1c, 0x08,
		0x07, 0x0F, 0x0F, 0x0F, 0x0F, 0x0E, 0x0D, 0x0B, 0x43, 0x53, 0x53, 0x53, 0x53, 0x52, 0x51, 0x13,
	};

// --- Main function ---
void main(void)
{
	unsigned char data score;
	unsigned char data scores[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	init();
	
	score = 0;
	displayLetters('0', '0');
	while(1) 
	{
		if (RECF) 
		{
			switch (RECV&0x0F) 
			{
				case 0x01: // -> player scored
					score++;
					displayLetters(RADIX_LOOKUP[score/16], RADIX_LOOKUP[score%16]);
				break;
				case 0x02: // -> game over
					if (scores[RECV>>4] < score) scores[RECV>>4] = score;
					displayLetters('0', '0');
				break;
				case 0x03: // -> get current score (and clear it)
					SendSlave(score);
					score = 0;
				break;
				case 0x04: // -> get highscore
					SendSlave(scores[RECV>>4]);
				break;
			}
			RECV = 0;
			RECF = 0;
		}
		if (score == 0) displayAnimation();
	}
}
	

// --- Function Definitions ---
void setDisplay(unsigned char left, unsigned char right)
{
	on_display[0] = left;
	on_display[1] = right;
}

void displayLetters(unsigned char left, unsigned char right)
{
	unsigned char c1 = getCharCode(left);
	unsigned char c2 = getCharCode(right);
	setDisplay(c1, c2);
}

unsigned char getCharCode(unsigned char letter)
{
	if (letter < '0' || letter > 'Z') return 0;
	if (letter >= 'A') return CHAR_LOOKUP[letter-'A'];
	return CHAR_LOOKUP[letter-'0' + 26];
}

void sleep(void) {
	unsigned char i;
	unsigned char j;
	for (i = 0xF0; i > 0; i--) {
		for(j = 0x4F; j > 0; j--) {
			if (RECF) return;
		}
	}
}

void displayAnimation(void)
{	
	unsigned char i = 0;
	while (!RECF) 
	{
		setDisplay(ANIMATION_LOOKUP[i + (16 * animation_select)], ANIMATION_LOOKUP[64 + i + (16 * animation_select)]);
		sleep();
		i = (i + 1) % 16;
	}
	setDisplay(0,0);
}


void init(void) 
{
	animation_select = 0;
	on_display[0] = 0;
	on_display[1] = 0;
	
	// INTERRUPTS
	EX0 = 1; // e/d INT0 interrupts
	ET1 = 1; // e/d T1 interrupts
	ES = 1;  // e/d Serial interrupts
	PS = 1;  // serial priority

	IT0 = 1; // 1 -> falling edge activation

	// SET T1 mode 2 -> display
	TMOD |= 0x20;
	TR1 = 1;
	
	InitSerial();
	
	EA = 1; // enable interrupts
}


// --- Interrupt Handlers ---
void Timer1_ISR(void) interrupt TF1_VECTOR
{
	left_segment  = on_display[0];
	right_segment = on_display[1];
}

void INT0_ISR(void) interrupt IE0_VECTOR
{
	animation_select = (animation_select + 1 ) % 4;
}