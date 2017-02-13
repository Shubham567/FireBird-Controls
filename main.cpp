#include "fbControls.h"

unsigned char data_in[5];
short int dptr = 0;
buzzer buz;
motion move(1);
servo srv(1);
ISR(USART0_RX_vect)	 	// ISR for receive complete interrupt
{
	unsigned char data = UDR0;
	UDR0 = data;
	data_in[dptr++] = (char)data;
	if(dptr == 4)
	{
		dptr = 0;
		unsigned char com = data_in[0];
		unsigned char val1 = data_in[1];
		unsigned char val2 = data_in[2];
		unsigned char valid = data_in[3];

		if(com == '8' && valid == 'z')
		{
			move.foreward();
			move.velocity(val1,val2);
		}
		else if(com == '2' && valid == 'z')
		{
			move.backward();
			move.velocity(val1,val2);
		}
		else if (com == 's' && valid == 'z')
		{
				srv.servo_1(val1);
		}
		else if (com == 'b' && valid == 'z')
		{
			if(val1 == '1')
				buz.beep();
			else
				buz.mute();
		}
	}
}

int main(void)
{
	XBEE_INIT();
	LCD disp;
	disp.writeString("BIRD ON FIRE");
	disp.moveCursor(2,1);
	while(1);
}
