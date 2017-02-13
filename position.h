
struct motor_position
{
	unsigned int left;
	unsigned int right;

	motor_position()
	{
		cli();
		left = 0;
		right = 0;
		DDRE = DDRE | 0xEF;
		PORTE = PORTE | 0x10;
		DDRE = DDRE | 0xDF;
		PORTE = PORTE | 0x20;
		EIMSK = EIMSK | 0x30;
		EICRB = EICRB | 0x0A;
		sei();
	}
};

motor_position pos;

ISR(INT4_vect)
{
	pos.left++;
}
ISR(INT5_vect)
{
	pos.right++;
}


