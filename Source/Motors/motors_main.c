#include <REGX52.H>


// --- Definitions ---
#define MAX 255
#define MIN 0

sbit Bulb1_IN = 0xA1;
sbit Bulb2_IN = 0xA2;
sbit Motor_IN1 = 0xA5;
sbit Motor_IN2 = 0xA6;



// --- Function Declarations ---
void init(void);

// serialcomms_slave.c
extern void SendSlave(char data_send);
extern void InitSerial(void);


// --- Global Variables ---
unsigned char data PWM_bulbs; // 0 -> 100% speed --> 255 0% speed
unsigned char data PWM_motor;
unsigned char data Suwak_bulbs;
unsigned char data Suwak_motor;
unsigned char data motor_rotations;
bit bulb_one;
bit bulb_two;
bit motor_clockwise;

// serialcomms_slave.c
extern unsigned char data RECV;
extern bit data RECF;

// storage_slave.c
extern void AccessStorage(unsigned char addr);


// --- Main Function ---
void main(void) {
	init();
	while (1) {
		if (!(bulb_one | bulb_two)) PWM_bulbs = MIN;
		if (!motor_rotations) PWM_motor = MIN;
		if (RECF) {
			if (RECV & 0x80) 
			{
				AccessStorage(RECV);
				RECV = 0;
				RECF = 0;
				continue;
			}

			switch (RECV & 0x0F) {
				case 0x01:
					PWM_bulbs = (RECV >> 4) * 0x24;
					bulb_one = 1;
					bulb_two = 1;
				break;
				case 0x02:
					PWM_motor = (RECV >> 4) * 0x24;
					motor_rotations = 2;
				break;
			}
			RECV = 0;
			RECF = 0;
		}
	}
}


// --- Function Definitions ---
void init(void) {
	// INTERRUPTS
	EX0 = 1; // e/d INT0 interrupts
	EX1 = 0; // e/d INT1 interrupts
	ET0 = 1; // e/d T0 interrupts
	ET1 = 1; // e/d T1 interrupts
	ET2 = 1; // e/d T2 interrupts
	
	IE0 = 1; // falling edge activation
	IE1 = 1; // falling edge activation
	IT0 = 1; // falling edge activation
	IT1 = 1; //	falling edge activation
	
	TMOD &= 0x00;
	TMOD |= 0x11;
	TR1 = 0; 
	TR0 = 0;
	TH0 = 0xFF;
	TL0 = 0xF0;
	TH1 = 0xFF;
	TL1 = 0xF0;
	
	InitSerial();
	
	Motor_IN1 = Motor_IN2 = Bulb1_IN = Motor_IN1 = 0;
	Suwak_bulbs = 0;
	Suwak_motor = 0;
	
	PWM_bulbs = MIN;
	PWM_motor = MIN; 
	
	motor_clockwise = 0;
	TR0 = 1;
	TR1 = 1;
	
	EA = 1; // enable interrupts
}


// --- Interrupt Handlers ---
// Bulbs PWM
void Timer0_ISR() interrupt TF0_VECTOR
{
	TR0 = 0;
	Suwak_bulbs++;
	if (PWM_bulbs == MIN) {
		Bulb1_IN = Bulb2_IN = 0;
	} else {
		if (Suwak_bulbs <= PWM_bulbs) {
			if (bulb_one) Bulb1_IN = 1;
			if (bulb_two) Bulb2_IN = 1;
		} else {
			Bulb1_IN = 0;
			Bulb2_IN = 0;
		}
	}
	if (Suwak_bulbs == MAX) Suwak_bulbs = MIN;
	TH0 = 0xFF;
	TL0 = 0xF0;
	TR0 = 1;
}

// Motor PWM
void Timer1_ISR() interrupt TF1_VECTOR
{
	TR1 = 0;
	Suwak_motor++;
	if (PWM_motor == MIN) {
		Motor_IN1 = Motor_IN2 = 0;
	} else {
		if (Suwak_motor <= PWM_motor) {
			if (motor_clockwise) Motor_IN1 = 1;
			else Motor_IN2 = 1;
		} else {
			if (motor_clockwise) Motor_IN1 = 0;
			else Motor_IN2 = 0;
		}
	}
	if (Suwak_motor == MAX) Suwak_motor = MIN;
	TH1 = 0xFF;
	TL1 = 0xF0;
	TR1 = 1;
}

// Motor rotation interrupt
void INT0_ISR () interrupt IE0_VECTOR
{
	bulb_one = 0;
	bulb_two = 0;
	if (motor_rotations) motor_rotations--;
}