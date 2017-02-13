/*
 * FireBird Controls.cpp
 *
 * Created: 20-01-2017 13:14:10
 * Author : Shubham Shekhar & Vinay Satish
 * 
 */ 

#define F_CPU 14745600

#define  packet_size 10

#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"
#include "queue.h"

//ENCODER

class encoder
{
	private:
	static volatile unsigned int leftEnc;
	static volatile unsigned int rightEnc;
	public:

	encoder()
	{
		DDRE  = DDRE & 0xEF;  //Set the direction of the PORTE 4 pin as input
		PORTE = PORTE | 0x10; //Enable internal pull-up for PORTE 4 pin
		DDRE  = DDRE & 0xDF;  //Set the direction of the PORTE 4 pin as input
		PORTE = PORTE | 0x20; //Enable internal pull-up for PORTE 4 pin
		cli(); //Clears the global interrupt
		EICRB = EICRB | 0x02; // INT4 is set to trigger with falling edge
		EIMSK = EIMSK | 0x10; // Enable Interrupt INT4 for left position encoder
		EICRB = EICRB | 0x08; // INT5 is set to trigger with falling edge
		EIMSK = EIMSK | 0x20; // Enable Interrupt INT5 for right position encoder
		sei();   // Enables the global interrupt
	}

	static void counterReset()
	{
		leftEnc = 0;
		rightEnc = 0;
	}

	static void incrLeft()
	{
		leftEnc++;
	}
	static void incrRight()
	{
		rightEnc++;
	}

	int leftVal()
	{
		return leftEnc;
	}
	int rightVal()
	{
		return rightEnc;
	}
};

volatile unsigned int encoder::rightEnc = 0;
volatile unsigned int encoder::leftEnc = 0;

ISR(INT5_vect)
{
	encoder::incrRight();
}


//ISR for left position encoder
ISR(INT4_vect)
{
	encoder::incrLeft();
}

class buzzer
{
	public:

	buzzer()
	{
		cli();
		DDRC = DDRC | 0x08;
		PORTC = PORTC & 0xF7;
		sei();
	}

	void beep()
	{
		PORTC = PORTC | 0x08;
	}

	void mute()
	{
		PORTC = PORTC & 0xF7;
	}
};

class motion
{
	unsigned char leftMotor;
	unsigned char rightMotor;
	public:

	motion()
	{
		cli();
		DDRA = DDRA | 0x0F;
		PORTA = PORTA & 0xF0;
		DDRL = DDRL | 0x18;
		PORTL = PORTL | 0x18;
		sei();
	}

	void pwmEnable()
	{
		cli(); //disable all interrupts
		
		DDRA = DDRA | 0x0F;
		PORTA = PORTA & 0xF0;
		DDRL = DDRL | 0x18;   //Setting PL3 and PL4 pins as output for PWM generation
		PORTL = PORTL | 0x18; //PL3 and PL4 pins are for velocity control using PWM.
		
		// Timer 5 initialized in PWM mode for velocity control
		// Prescale:256
		// PWM 8bit fast, TOP=0x00FF
		// Timer Frequency:225.000Hz
		 
		TCCR5B = 0x00;	//Stop
		TCNT5H = 0xFF;	//Counter higher 8-bit value to which OCR5xH value is compared with
		TCNT5L = 0x01;	//Counter lower 8-bit value to which OCR5xH value is compared with
		OCR5AH = 0x00;	//Output compare register high value for Left Motor
		OCR5AL = 0xFF;	//Output compare register low value for Left Motor
		OCR5BH = 0x00;	//Output compare register high value for Right Motor
		OCR5BL = 0xFF;	//Output compare register low value for Right Motor
		OCR5CH = 0x00;	//Output compare register high value for Motor C1
		OCR5CL = 0xFF;	//Output compare register low value for Motor C1
		TCCR5A = 0xA9;	/*{COM5A1=1, COM5A0=0; COM5B1=1, COM5B0=0; COM5C1=1 COM5C0=0}
 						  For Overriding normal port functionality to OCRnA outputs.
				  		  {WGM51=0, WGM50=1} Along With WGM52 in TCCR5B for Selecting FAST PWM 8-bit Mode*/
	
		TCCR5B = 0x0B;	//WGM12=1; CS12=0, CS11=1, CS10=1 (Prescaler=64)
		sei(); //re-enable interrupts
	}

	motion(short int x)
	{
		motion();
		pwmEnable();
	}

	

	void velocity (unsigned char left_motor, unsigned char right_motor)
	{
		leftMotor = left_motor;
		rightMotor = right_motor;
		OCR5AL = left_motor;
		OCR5BL = right_motor;
	}
	void foreward()
	{
		PORTA = PORTA & 0xF0;
		PORTA = PORTA | 0x06;
	}
	// One pulse = 5.44 mm
	// Hard turn = 4.090 degrees
	void forewardDist(unsigned int distance_cm)
	{
		encoder pulses;
		encoder::counterReset();
		int reqdPulses = distance_cm / 0.544;
		reqdPulses += ((pulses.leftVal() + pulses.rightVal()) / 2) ;

		foreward();
		while(reqdPulses > ((pulses.leftVal() + pulses.rightVal()) / 2) );
		stop();
	}

	void rotateLeft(unsigned int degree)
	{
		encoder pulses;
		encoder::counterReset();
		int reqdPulses = ((float)degree) / 2.045;
		reqdPulses += pulses.rightVal();
		softLeft();
		while(reqdPulses > pulses.rightVal());
		stop();

	}

	void rotateRight(unsigned int degree)
	{
		encoder pulses;
		encoder::counterReset();
		int reqdPulses = ((float)degree) / 2.045;
		reqdPulses += pulses.leftVal();
		softRight();
		while(reqdPulses > pulses.leftVal());
		stop();

	}


	void backward()
	{
		PORTA = PORTA & 0xF0;
		PORTA = PORTA | 0x09;
	}

	void hardLeft()
	{
		PORTA = PORTA & 0xF0;
		PORTA = PORTA | 0x05;
	}

	void hardRight()
	{
		PORTA = PORTA & 0xF0;
		PORTA = PORTA | 0x0A;
	}

	void softLeft()
	{
		PORTA = PORTA & 0xF0;
		PORTA = PORTA | 0x04;
	}

	void softRight()
	{
		PORTA = PORTA & 0xF0;
		PORTA = PORTA | 0x02;
	}

	void revsRight()
	{
		PORTA = PORTA & 0xF0;
		PORTA = PORTA | 0x08;
	}

	void revsLeft()
	{
		PORTA = PORTA & 0xF0;
		PORTA = PORTA | 0x01;
	}
	void stop()
	{
		PORTA = PORTA & 0xF0;
	}
};

class LCD
{
	public:

	LCD()
	{
		cli();
		lcd_port_config();
		sei();
		lcd_init();
	}

	void moveCursor(short int row,short int col)
	{
		lcd_cursor(row,col);
	}

	void writeString(char *str)
	{
		lcd_string(str);
	}

	void cursorHome()
	{
		lcd_home();
	}

	void printNum(short int row,short int col,unsigned int num,short int digits)
	{
		lcd_print(row,col,num,digits);
	}

};



//SERVO FROM HERE
class servo
{
	void servo1_pin_config (void)
	{
		DDRB  = DDRB | 0x20;  //making PORTB 5 pin output
		PORTB = PORTB | 0x20; //setting PORTB 5 pin to logic 1
	}
	void servo2_pin_config (void)
	{
		DDRB  = DDRB | 0x40;  //making PORTB 6 pin output
		PORTB = PORTB | 0x40; //setting PORTB 6 pin to logic 1
	}
	void servo3_pin_config (void)
	{
		DDRB  = DDRB | 0x80;  //making PORTB 7 pin output
		PORTB = PORTB | 0x80; //setting PORTB 7 pin to logic 1
	}

	public:

	servo(int numServo = 1)
	{
		cli();
		servo1_pin_config();
		if(numServo == 2)
			servo2_pin_config();
		else if ( numServo == 3)
			servo3_pin_config();

		TCCR1B = 0x00; //stop
		TCNT1H = 0xFC; //Counter high value to which OCR1xH value is to be compared with
		TCNT1L = 0x01;	//Counter low value to which OCR1xH value is to be compared with
		OCR1AH = 0x03;	//Output compare Register high value for servo 1
		OCR1AL = 0xFF;	//Output Compare Register low Value For servo 1
		OCR1BH = 0x03;	//Output compare Register high value for servo 2
		OCR1BL = 0xFF;	//Output Compare Register low Value For servo 2
		OCR1CH = 0x03;	//Output compare Register high value for servo 3
		OCR1CL = 0xFF;	//Output Compare Register low Value For servo 3
		ICR1H  = 0x03;	
		ICR1L  = 0xFF;
		TCCR1A = 0xAB; /*{COM1A1=1, COM1A0=0; COM1B1=1, COM1B0=0; COM1C1=1 COM1C0=0}
 					For Overriding normal port functionality to OCRnA outputs.
				  {WGM11=1, WGM10=1} Along With WGM12 in TCCR1B for Selecting FAST PWM Mode*/
		TCCR1C = 0x00;
		TCCR1B = 0x0C; //WGM12=1; CS12=1, CS11=0, CS10=0 (Prescaler=256)
		sei();
	}
	void servo_1(unsigned char degrees)
	{
		float PositionPanServo = 0;
		PositionPanServo = ((float)degrees / 1.86) + 35.0;
		OCR1AH = 0x00;
		OCR1AL = (unsigned char) PositionPanServo;
	}
	void servo_2(unsigned char degrees)
	{
		float PositionTiltServo = 0;
		PositionTiltServo = ((float)degrees / 1.86) + 35.0;
		OCR1BH = 0x00;
		OCR1BL = (unsigned char) PositionTiltServo;
	}
	void servo_3(unsigned char degrees)
	{
		float PositionServo = 0;
		PositionServo = ((float)degrees / 1.86) + 35.0;
		OCR1CH = 0x00;
		OCR1CL = (unsigned char) PositionServo;
	}
	void servo_1_free (void) //makes servo 1 free rotating
	{
		OCR1AH = 0x03;
		OCR1AL = 0xFF; //Servo 1 off
	}
	void servo_2_free (void) //makes servo 2 free rotating
	{
		OCR1BH = 0x03;
		OCR1BL = 0xFF; //Servo 2 off
	}
	void servo_3_free (void) //makes servo 3 free rotating
	{
		OCR1CH = 0x03;
		OCR1CL = 0xFF; //Servo 3 off
	}
};

/*class XBEE
{
	static queue que;
	public:
	XBEE()
	{
		cli();
		UCSR0B = 0x00; //disable while setting baud rate
		UCSR0A = 0x00;
		UCSR0C = 0x06;
		UBRR0L = 0x5F; //set baud rate lo
		UBRR0H = 0x00; //set baud rate hi
		UCSR0B = 0x98;
		sei();
	}
	static void rX(unsigned char data)
	{
		que.push(data);
	}
	static void tx(unsigned char data)
	{
		UDR0 = data;
	}
	static unsigned char readChar()
	{
		return que.pop();
	}
	static short int dataSize()
	{
		return que.packetPresent();
	}
	static unsigned char* readAll()
	{
		unsigned char arr[q_size];
		short int i = 0;
		while(!que.isEmpty())
		{
			arr[i++] = que.pop(); 
		}
		arr[i] = 0;
		return arr;
	}

};
queue XBEE::que;

ISR(USART0_RX_vect)	 	// ISR for receive complete interrupt
{
	unsigned char data = UDR0;
	XBEE::rX(data);
	XBEE::tx(data);
}*/

void XBEE_INIT()
{
	cli();
	UCSR0B = 0x00; //disable while setting baud rate
	UCSR0A = 0x00;
	UCSR0C = 0x06;
	UBRR0L = 0x5F; //set baud rate lo
	UBRR0H = 0x00; //set baud rate hi
	UCSR0B = 0x98;
	sei();
}