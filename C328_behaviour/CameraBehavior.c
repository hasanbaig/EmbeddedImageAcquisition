#include <18f6720.h>
#include <string.h>
#fuses HS, PROTECT, NOOSCSEN, NOWDT, WDT128, NOBROWNOUT, PUT, NOCPD, NOSTVREN, NODEBUG, NOLVP, NOWRT, NOCPB, NOEBTRB, NOEBTR, NOWRTD, NOWRTC, NOWRTB
#use delay(clock=10000000)
#use rs232(STREAM=COM1, baud=9600, xmit=PIN_C6, rcv=PIN_C7)
#use rs232(STREAM=COM2, baud=9600, xmit=PIN_G1, rcv=PIN_G2)

static int1 reset_ack;
static int1 compare_data;
static int1 check;
static int1 sync_ack;
static int1 initial_ack;
static int1 package_ack;
static int1 snapshot_ack;
static int1 getpicture_ack;
static int1 send_imagedata;
static int1 image_calc;
static int1 request_package;
static int1 read_data;
static int1 last_package;
static int1 printup;

unsigned int reset_frame[] = {0xAA,0x0E,0x08,0x00,0x00,0x00};
unsigned int sync_frame[] = {0xAA,0x0E,0x0D,0x02,0x00,0x00,0xAA,0x0D,0x00,0x00,0x00,0x00};
unsigned int initial_frame [] = {0xAA,0x0E,0x01,0x01,0x00,0x00};
unsigned int package_frame [] = {0xAA,0x0E,0x06,0x01,0x00,0x00};
unsigned int snapshot_frame [] = {0xAA,0x0E,0x05,0x01,0x00,0X00};
unsigned int getpicture_frame [] = {0xAA,0x0E,0x04,0x01,0x00,0X00};
unsigned int imagesize_frame [] = {0xAA,0x0A,0x01,0x5C,0x47,0X00};
unsigned int image_data [512];

static unsigned int16 i;
static unsigned int j;

static unsigned int buffer [512];
static unsigned int16 counter;
static unsigned int expected_bytes;


#int_RDA
void RDA_isr(void)
{
	buffer [counter] = getc(COM1);

	counter = counter + 1;

}








void main (void)
{
	set_tris_c( 0x80 );
	set_tris_g( 0x02 );
	enable_interrupts( INT_RDA );
	enable_interrupts( GLOBAL );

	expected_bytes = 6; 			//resets required = expected_bytes / 6
	reset_ack = 1;


	while(1)
	{
		if(reset_ack)
		{
			if(counter == expected_bytes)
			{

				for(i=0;i<=5;i++)
				{
				fprintf(COM1,"%C",reset_frame[i]);
				//fprintf(COM2,"\r\n%lu",counter);
				}
				delay_ms (10);
				reset_ack = 0;
				sync_ack = 1;
				counter = 0;
				expected_bytes = 6;   //synchronizations required = expected bytes / 6

			}
		}

		if(sync_ack)
		{
			if(counter == expected_bytes)
			{
				for(i=0;i<=11;i++)
				fprintf(COM1,"%C",sync_frame[i]);

				delay_ms (10);
				sync_ack = 0;
				initial_ack = 1;
				counter = 0;
				expected_bytes = 6;
			}

		}

		if(initial_ack)
		{

			if(counter == expected_bytes)
			{
				for(i=0;i<=5;i++)
				fprintf(COM1,"%C",initial_frame[i]);

				delay_ms (10);
				initial_ack = 0;
				package_ack = 1;
				counter = 0;
				expected_bytes = 6;
			}
		}

		if(package_ack)
		{
			if(counter == expected_bytes)
			{
				for(i=0;i<=5;i++)
				fprintf(COM1,"%C",package_frame[i]);

				delay_ms (10);
				package_ack = 0;
				snapshot_ack = 1;
				counter = 0;
				expected_bytes = 6;
			}

		}

		if(snapshot_ack)
		{
			if(counter == expected_bytes)
			{
				for(i=0;i<=5;i++)
				fprintf(COM1,"%C",snapshot_frame[i]);

				delay_ms (10);
				snapshot_ack = 0;
				getpicture_ack = 1;
				counter = 0;
				expected_bytes = 6;
			}
		}

		if(getpicture_ack)
		{
			if(counter == expected_bytes)
			{
				for(i=0;i<=5;i++)
				fprintf(COM1,"%C",getpicture_frame[i]);

				for(i=0;i<=5;i++)
				fprintf(COM1,"%C",imagesize_frame[i]);

				delay_ms (10);
				getpicture_ack = 0;
				send_imagedata = 1;
				counter = 0;
				expected_bytes = 6;
				j=0;
			}
		}

		if(send_imagedata)
		{
			if(j != 3)
			{
				if(counter == expected_bytes)
				{
					for(i=0;i<512;i++)
					fprintf(COM1,"%C",i);


					counter = 0;
					if(j==2)
						send_imagedata = 0;

					j = j + 1;


				}

			}
		}

	}
}
