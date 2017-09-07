// Successful Read/Write RAM .... Interfacing with GPS Controller

#opt 9
#include <18f6720.h>
#use delay(clock=10000000)
#fuses HS, PROTECT, NOOSCSEN, NOWDT, NOBROWNOUT, PUT, NOCPD, NOSTVREN, NODEBUG, NOLVP, NOWRT, NOCPB, NOEBTRB, NOEBTR, NOWRTD, NOWRTC, NOWRTB,
#use rs232(STREAM=COM1, baud=9600, xmit=PIN_C6, rcv=PIN_C7)
#use rs232(STREAM=COM2, baud=9600, xmit=PIN_G1, rcv=PIN_G2)
#priority RDA, RDA2, TIMER1

#use fast_io(A)
#use fast_io(B)
#use fast_io(C)
#use fast_io(D)
#use fast_io(E)
#use fast_io(F)
#use fast_io(G)

#byte PORTA = 0xF80
#byte PORTB = 0xF81
#byte PORTC = 0xF82
#byte PORTD = 0xF83
#byte PORTE = 0xF84
#byte PORTF = 0xF85
#byte PORTG = 0xF86

#define package_size 512				//Set Package size

#bit RAM_CE = PORTE.2				//Chip Select
#bit RAM_WR = PORTE.1				//Write Enable
#bit RAM_OE = PORTE.0				//Read Enable

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

void write_ext_ram( unsigned int16 , unsigned int  );
unsigned int read_ext_ram( unsigned int16 address );

static unsigned int serial_data;

///////////////////////////////////Flags////////////////////////////////////////////

static int1 reset_ack;
static int1 compare_data;
static int1 sync_ack;
static int1 initial_ack;
static int1 package_ack;
static int1 snapshot_ack;
static int1 getpicture_ack;
static int1 image_calc;
static int1 request_package;
static int1 send_new_id;
static int1 read_data_camera;
static int1 last_package;
static int1 write_RAM;

static int1 read_RAM;

////////////////////////////////////////////////////////////////////////////////////

static unsigned int16 i;
static unsigned int count_packet;
static unsigned int l,h;

static unsigned int32 counter;
static unsigned int buffer [1024];
static unsigned int32 expected_bytes;
static unsigned int check;

////////////////////////////image length calculators////////////////////////////////
unsigned int image_size [ 3 ];
static unsigned int32 image_length;
static unsigned int32 total_bytes;
static unsigned int16 last_transaction_size;
float packages;
static unsigned int number_packages;

//////////////////////////////////Package ID Senders////////////////////////////////
static unsigned int16 a,b,n;

static unsigned int32 increment_addr;


//Image buffer
unsigned int image_buffer [];
static unsigned int32 image_index;


//////////////////////////Frames to be send to Camera///////////////////////////////

unsigned int reset_frame [] = {0xAA,0x08,0x01,0x00,0x00,0xFF};
unsigned int reset_cmp[] = {0xAA,0x0E,0x08};

unsigned int sync_frame [] = {0xAA,0x0D,0x00,0x00,0x00,0x00};
unsigned int sync_cmp [] = {0xAA,0x0E,0x0D};

unsigned int initial_frame [] = {0xAA,0x01,0x07,0x07,0x03,07};
unsigned int initial_cmp [] = {0xAA,0x0E,0x01};

unsigned int package_frame [] = {0xAA,0x06,0x08,0x00,0x02,00};
unsigned int package_cmp [] = {0xAA,0x0E,0x06};

unsigned int snapshot_frame [] = {0xAA,0x05,0x00,0x00,0x00,00};
unsigned int snapshot_cmp [] = {0xAA,0x0E,0x05};

unsigned int getpicture_frame [] = {0xAA,0x04,0x01,0x00,0x00,00};
unsigned int getpicture_cmp [] = {0xAA,0x0E,0x04};

unsigned int ID_frame [] = {0xAA,0x0E,0x00,0x00};

////////////////////////////////////////////////////////////////////////////////////






//////////////////////Signals to and from GPS Controller////////////////////////////

static unsigned int read_from_RAM [512];
static unsigned int32 read_addr;
static unsigned int j;
static unsigned int16 store_at_addr;
unsigned int gps_buffer [];
unsigned int gps_image [] = "get image";

static unsigned int gps_counter;
static unsigned int gps_command;
static unsigned int package_ID;

static int1 take_snapshot;
static int1 requested_packet;

////////////////////////////////////////////////////////////////////////////////////


#int_RDA
void RDA_isr(void)
{

	buffer [counter] = getc(COM1);
	counter = counter + 1;

	if(counter == expected_bytes)
	{
		//counter = 0;
		compare_data = 0;
		send_new_id = 1;
		count_packet = count_packet +1;
		write_RAM = 1;
	//	request_package = 1;
	}
}


#int_RDA2
void RDA2_isr(void)
{
	gps_command	= getc(COM2);

	if(requested_packet)
	{
		package_ID = gps_command;
		read_RAM = 1;
	}
	else
	{
		gps_buffer [gps_counter]  = gps_command ;
		gps_counter = gps_counter + 1;

		if(gps_counter == 9)
		{
			take_snapshot = 1;
			gps_counter = 0;
		}
	}
}







void main( void )
{
	set_tris_c( 0x80 );
	set_tris_g( 0x02 );
	set_tris_b( 0x00 );			// Low Address Byte
	set_tris_f( 0x00 );			// High Address Byte

	//set_tris_d( 0x00 );			//Data to RAM
	set_tris_e( 0x00 );			// CE = E0  , WE = E1

	enable_interrupts( INT_RDA );
	enable_interrupts( GLOBAL );

	OUTPUT_E(0x0F);

	expected_bytes = 6;
	reset_ack = 0;
	compare_data = 1;

	read_data_camera = 0;

	while(1)
	{

		if(take_snapshot)
		{

			check = strcmp(gps_buffer,gps_image);

			if(check)
				{
					reset_ack = 0;
					take_snapshot = 0;
				}
			else
			{
				fprintf(COM2,"OK");				// acknowledge to gps controller
				reset_ack = 1;
			}


			// Reseting Camera
			if(reset_ack)
			{
				if(compare_data)
				{
					for(i=0;i<=5;i++)
					 	fprintf(COM1,"%C",reset_frame[i]);

					delay_ms (100);
				}
				else
				{
					check = strncmp(buffer,reset_cmp,3);
			//		fprintf(COM2,"%d",check);				//can be omitted
					if(check)
					{
						compare_data = 1;
						counter = 0;
					}
					else
					{
						counter = 0;
						reset_ack = 0;
						sync_ack = 1;
						compare_data = 1;
						expected_bytes = 12;
					}
				}
			}


			// Camera Synchronization
			if(sync_ack)
			{
				if(compare_data)
				{
					for(i=0;i<=5;i++)
						fprintf(COM1,"%C",sync_frame[i]);

					delay_ms (10);
				}
				else
				{
					check = strncmp(buffer,sync_cmp,3);
			//		fprintf(COM2,"%d",check);				//can be omitted
					if(check)
					{
						compare_data = 1;
						counter = 0;
					}
					else
					{
						for(i=0;i<=5;i++)
							fprintf(COM1,"%C",buffer[i]);

						delay_ms(1000);
						counter = 0;
						sync_ack = 0;
						initial_ack = 1;
						compare_data = 1;
						expected_bytes = 6;
					}
				}
			}


			// Camera initialization
			if(initial_ack)
			{
				if(compare_data)
				{
					for(i=0;i<=5;i++)
					 	fprintf(COM1,"%C",initial_frame[i]);

					delay_ms (10);
				}
				else
				{
					check = strncmp(buffer,initial_cmp,3);
			//		fprintf(COM2,"%d",check);				//can be omitted
					if(check)
					{
						compare_data = 1;
						counter = 0;
					}
					else
					{
						counter = 0;
						initial_ack = 0;
						package_ack = 1;
						compare_data = 1;
						expected_bytes = 6;
					}
				}
			}


			// Setting Package size
			if(package_ack)
			{
				if(compare_data)
				{
					for(i=0;i<=5;i++)
					 	fprintf(COM1,"%C",package_frame[i]);

					delay_ms (10);
				}
				else
				{
					check = strncmp(buffer,package_cmp,3);
			//		fprintf(COM2,"%d",check);				//can be omitted
					if(check)
					{
						compare_data = 1;
						counter = 0;
					}
					else
					{
						counter = 0;
						package_ack = 0;
						snapshot_ack = 1;
						compare_data = 1;
						expected_bytes = 6;
					}
				}
			}


			// Snapshot Configuration
			if(snapshot_ack)
			{
				if(compare_data)
				{
					for(i=0;i<=5;i++)
					 	fprintf(COM1,"%C",snapshot_frame[i]);

					delay_ms (10);
				}
				else
				{
					check = strncmp(buffer,snapshot_cmp,3);
			//		fprintf(COM2,"%d",check);				//can be omitted
					if(check)
					{
						compare_data = 1;
						counter = 0;
					}
					else
					{
						counter = 0;
						snapshot_ack = 0;
						getpicture_ack = 1;
						compare_data = 1;
						expected_bytes = 12;
						delay_ms(1000);
					}
				}
			}


			//Get image data frame
			if(getpicture_ack)
			{
				if(compare_data)
				{
					for(i=0;i<=5;i++)
					 	fprintf(COM1,"%C",getpicture_frame[i]);

					delay_ms (10);

				}
				else
				{
					check = strncmp(buffer,getpicture_cmp,3);
			//		fprintf(COM2,"%d",check);				//can be omitted
					if(check)
					{
						compare_data = 1;
						counter = 0;
					}
					else
					{
						for(i=11;i>=9;i--)
							image_size [11-i] = buffer[i];

						counter = 0;
						getpicture_ack = 0;
						image_calc = 1;
						compare_data = 1;

					}
				}
			}


			// Image Size Calculation
			if(image_calc)
			{

				for(i=0;i<=2;i++)
				{

					if(i==0)
					{
						image_length = image_size[i];
						image_length = image_length << 8;
					}

					if(i==1)
					{
						image_length = image_length | image_size[i];
						image_length = image_length << 8;
					}

					if(i==2)
						image_length = image_length | image_size[i];
				}

			//	fprintf(COM2,"\n\rimage length = %lu",image_length);

				packages = (image_length/(512.0-6));

			//	fprintf(COM2,"\n\rPackages = %f",packages);

				number_packages = ceil(packages);

			//	fprintf(COM2,"\n\rNo. of Packages = %lu",number_packages);

				total_bytes = image_length + (number_packages * 6);

				last_transaction_size = total_bytes - (number_packages - 1)*package_size ;

			//	fprintf(COM2,"\n\rtotal bytes = %lu \n\rlast_transaction_size = %lu",total_bytes,last_transaction_size);

				image_calc = 0;

				request_package = 1;

				send_new_id = 1;

				expected_bytes = package_size;

				count_packet = 0;
				write_RAM = 0;

			}


			//Sending Image ID
			if(request_package)
			{
				if((send_new_id == 1) && (count_packet < number_packages ))
				{

					read_data_camera = 1;
					if(count_packet == (number_packages - 1))
						expected_bytes = last_transaction_size;


					for(i=0;i<=3;i++)
						fprintf(COM1,"%C",ID_frame[i]);

					fprintf(COM1,"%c%c",count_packet,count_packet>>8);
					send_new_id = 0;
				}
				delay_ms(10);
				request_package = 0;
			}

			// Acquiring data from camera and writing into the RAM
			if(read_data_camera)
			{
				if(write_RAM)
				{

					if(count_packet == (number_packages))
					{
						for(i=(counter - last_transaction_size);i<(counter);i++)
							write_ext_ram(i + increment_addr, buffer [i]);

						fprintf(COM2,"%c",total_bytes);
						delay_ms(1);
						fprintf(COM2,"%c",number_packages);
						requested_packet = 1;
					}
					else
					{
						for(i=(counter - package_size);i<(counter);i++)
							write_ext_ram(i + increment_addr, buffer [i]);

						write_RAM = 0;
						request_package = 1;
						counter = 0;
						increment_addr = increment_addr + package_size;
					}
				}
			}


			//Reading data from RAM
			if(read_RAM)
			{
				read_addr =	package_ID * package_size;
				for(i=read_addr;i<(read_addr + package_size);i++)
					read_ext_ram(i);

					//read_addr = read_addr + package_size;

				read_RAM = 0;
			}

		}

	}
}



void write_ext_ram( unsigned int16 address, unsigned int data )
{

	set_tris_d( 0x00 );

	PORTB = address;			// Address Low Byte
	PORTF = address >> 8;		// Address high Byte

	PORTD = data;

	delay_us(1);
	RAM_CE = 0;

	delay_us(1);
	RAM_WR = 0;

	delay_us(1);
	RAM_WR = 1;

	delay_us(1);
	RAM_CE = 1;

	OUTPUT_E(0x0F);			// Chip disable
	//fprintf(COM2,"%C",data);
}


unsigned int read_ext_ram( unsigned int16 address )
{
	set_tris_d( 0xFF );

	store_at_addr = address - read_addr;

	PORTB = address;			// Address Low Byte
	PORTF = address >> 8;		// Address high Byte

	delay_us(1);
	RAM_CE = 0;

	delay_us(1);
	RAM_OE = 0;

	delay_us(1);
	read_from_RAM [ store_at_addr ] = INPUT_D();

	delay_us(1);
	RAM_OE = 1;

	delay_us(1);
	RAM_CE = 1;

	fprintf(COM2,"%C",read_from_RAM [ store_at_addr ]);

}
