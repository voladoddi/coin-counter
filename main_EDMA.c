#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "evmdm6437.h"
#include "evmdm6437_dip.h"
#include "evmdm6437_led.h"

#include "global.h"
#include "coin_label.h"
#include "icons.h"
#include "numbers.h"

/////////////////////////////////////////////////////////////////
//         DON'T change the following setting for EDMA         //
/////////////////////////////////////////////////////////////////
/* EDMA Registers for 6437*/
#define		PaRAM_OPT	0	// Channel Options Parameter
#define		PaRAM_SRC	1	// Channel Source Address
#define		PaRAM_BCNT	2	// Count for 2nd Dimension (BCNT) | Count for 1st Dimension (ACNT)
#define		PaRAM_DST	3	// Channel Destination Address
#define		PaRAM_BIDX	4	// Destination BCNT Index | Source BCNT Index
#define		PaRAM_RDL	5	// BCNT Reload (BCNTRLD) | Link Address (LINK)
#define		PaRAM_CIDX	6	// Destination CCNT Index | Source CCNT Index
#define		PaRAM_CCNT	7	// Count for 3rd Dimension (CCNT)

/* EDMA Registers for 6437*/
#define		EDMA_IPR	*(volatile int *)0x01C01068	// EDMA Channel interrupt pending low register
#define		EDMA_IPRH	*(volatile int *)0x01C0106C	// EDMA Channel interrupt pending high register
#define		EDMA_IER	*(volatile int *)0x01C01050	// EDMA Channel interrupt enable low register
#define		EDMA_IERH	*(volatile int *)0x01C01054	// EDMA Channel interrupt enable high register
#define		EDMA_ER 	*(volatile int *)0x01C01000	// EDMA Event low register
#define		EDMA_ERH	*(volatile int *)0x01C01004	// EDMA Event high register
#define		EDMA_EER	*(volatile int *)0x01C01020	// EDMA Event enable low register
#define		EDMA_EERH	*(volatile int *)0x01C01024	// EDMA Event enable high register
#define		EDMA_ECR	*(volatile int *)0x01C01008	// EDMA Event clear low register
#define		EDMA_ECRH	*(volatile int *)0x01C0100C	// EDMA Event clear high register
#define		EDMA_ESR	*(volatile int *)0x01C01010	// EDMA Event set low register
#define		EDMA_ESRH	*(volatile int *)0x01C01014	// EDMA Event set high register

/////////////////////////////////////////////////////////////////

extern Int16 video_loopback_test( Uint32, Uint32 );

// size for buffer_in: 720 * 480 / 2, the reason is explained below. 
#define Pixels 172800

// Resolution 720 * 480 (NTSC mode)
#define vWidth 720
#define vHeight 480

// CAN change the internal blocksize here, the example is 60 * 120
#define INTERNAL_BLK_WIDTH 60
#define INTERNAL_BLK_HEIGHT 120

//sine table
short sine_table[table_size];
//short cosine_table[table_size];


// Define a space on memory for save the information input and output (Interface data)
Uint32 buffer_out[Pixels]; //from 0x80000000
Uint32 buffer_in[Pixels]; //from 0x800A8C00, which is the same as 4 (bytes for integer) * Pixels

// Declare the internal buffer 
//Uint32 internal_buffer_2D[INTERNAL_BLK_HEIGHT][INTERNAL_BLK_WIDTH / 2];


/********************************************/
// Declaring two arrays:- to store copper and silver BINARY images

Uint8 copper[ARRAY_SIZE];
Uint8 silver[ARRAY_SIZE];
//Uint8 labels[vWidth*vHeight];

short forbidden[ARRAY_SIZE];
unsigned char hough_output[ARRAY_SIZE];

short cleared;

//////////////////////////
//#define K 2

//unsigned char mask[9];
//unsigned char array_mask[49];




// Define the position of the data (refer to linker.cmd)
// Internal memory L2RAM ".l2ram" 
// External memory DDR2 ".ddr2"

#pragma DATA_SECTION(buffer_out,".ddr2")
#pragma DATA_SECTION(buffer_in,".ddr2")

//#pragma DATA_SECTION(labels,".ddr2") 
#pragma DATA_SECTION(copper,".ddr2")
#pragma DATA_SECTION(silver,".ddr2")

#pragma DATA_SECTION(hough_output,".ddr2")
#pragma DATA_SECTION(forbidden,".ddr2")

//#pragma DATA_SECTION(cent_icon,".ddr2")

// buffer_in represents one input frame which consists of two interleaved frames.
// Each 32 bit data has the information for two adjacent pixels in a row.
// Thus, the buffer holds 720/2 integer data points for each row of 2D image and there exist 480 rows.
//
// Format: yCbCr422 ( y1 | Cr | y0 | Cb )
// Each of y1, Cr, y0, Cb has 8 bits
// For each pixel in the frame, it has y, Cb, Cr components
//
// You can generate a lookup table for color conversion if you want to convert to different color space such as RGB.
// Could refer to http://www.fourcc.org/fccyvrgb.php for conversion between yCbCr and RGB
//
// y: 	[016-125-235] [0x10-0x7D-0xEB] 
// Cr: 	[000-128-255] [0x00-0x80-0xFF]
// Cb:	[000-128-255] [0x00-0x80-0xFF]
//



//-----------User-Defined-Function------------- 
//#pragma CODE_SECTION(clear_box, "IRAM");
void clear_box(int y, int x, int color)
{
   	int i, j;
	
	#pragma MUST_ITERATE(28, 28);
	for(i = y-12 - 2; i < y-12 + 26; ++i)
		#pragma MUST_ITERATE(17, 17);
		for (j = x-9 + 1; j < x-9 + 18; ++j) 
			buffer_out[i * vWidth/2 + j] = color;		
}


//-----------User-Defined-Function------------- 
//#pragma CODE_SECTION(clear_all, "IRAM");
inline void clear_all(void) 
{
	clear_box(MID, HOZ_MID_1, CYAN);
  	clear_box(MID, HOZ_MID_2, CYAN);
  	clear_box(MID, HOZ_MID_3, CYAN);
  	clear_box(MID, HOZ_MID_4, CYAN);
	clear_box(TOP+52, LEFT+245, DARK_SLATE);    //clear total amount
	clear_box(TOP+52, LEFT+263, DARK_SLATE);
	clear_box(TOP+54, LEFT+253, DARK_SLATE);
	cleared = 1;
}



//-----------User-Defined-Function------------- 

void separate_copper_silver()
{
  Uint32 yCbCr422;
  Uint16 y1, y0, Cr;
  int k = UPPER_LINE*vWidth, i;	
  Uint16 label1, label2;
  
//  memset(labels, 0, 2*Pixels);
  memset(copper, 0, 2*Pixels);
  memset(silver, 0, 2*Pixels);
	
	for (i = UPPER_LINE * vWidth/2; i < Pixels ; i++)    //get labels
	{
		yCbCr422 = buffer_in[i];
		
		y1 = (yCbCr422 >> 24) & 0xFF;
		y0 = (yCbCr422 >> 8) & 0xFF;
		Cr = (yCbCr422 >> 16) & 0xFF;
		
		if (y0 <= BG_TH || y1 <= BG_TH) {
			buffer_out[i] = 0x107F107F; // black
			k++;
			k++;
			continue;
		}

		label1 = classify(y0, Cr, copper, silver, k++);
		label2 = classify(y1, Cr, copper, silver, k++);
		   
		if (label1 == 1 && label2 == 1)
		    buffer_out[i] = WHEAT; // orange
		
		else if (label1 == 2 || label2 == 2)
	        buffer_out[i] = QUARTZ;
		    		
//		else
//		    //buffer_out[i] = 0x10FF10FF; // pink
//		    buffer_out[i] = yCbCr422; 

//		buffer_out[i] = (label1 && label2)? WHEAT:BLACK;

	}
    
    //hough();
}



//-----------User-Defined-Function------------- 
//#pragma CODE_SECTION(display_num, "IRAM");
void display_num(int y, int x, int num, int color) {
	int i, j;
	int left = x-9, top = y-12;
	unsigned char *label = num_array[num/10];
	
	#pragma MUST_ITERATE(24, 24);
	for(i = top; i < top+24; ++i)
		#pragma MUST_ITERATE(12, 12);
		for (j = left; j < left+12; ++j) 
			if (!label[(i-top) * 12 + j-left])
					buffer_out[i * vWidth/2 + j] = color;
	
	left = x-3;		
	label = num_array[num%10];
	
	#pragma MUST_ITERATE(24, 24);
	for(i = top; i < top+24; ++i)
		#pragma MUST_ITERATE(12, 12);
		for (j = left; j < left+12; ++j) 
			if (!label[(i-top) * 12 + j-left])
					buffer_out[i * vWidth/2 + j] = color;
}

//void display(int y, int x, unsigned char *num, int color) {
//	int i, j;
//	int left = x-6, top = y-12;
//	
//	for(i = top; i < y+12; ++i)
//		for (j = left; j < x+6; ++j) 
//			if (!num[(i-top) * 12 + j-left])
//					buffer_out[i * vWidth/2 + j] = color;
//}



//-----------User-Defined-Function------------- 
//#pragma CODE_SECTION(draw_circle, "IRAM");
void draw_circle(Uint8 * output, Uint8 *input, int R) {
	int i,j, a,b;
	int th;
	
	for(i = R; i < vHeight - R; i++)
	{
		for(j = R; j < vWidth - R; j++)
		{
			if(input[i*vWidth+j])
			{
				#pragma MUST_ITERATE(table_size, table_size);
				for(th = 0; th < table_size; th++)
				{
					a = j - floor((R * (sine_table[(18+th)%(table_size-1)])) >> 10);
			    	//a = j - floor(R * (sine_table[th]) );
					b = i - floor((R * (sine_table[th])) >> 10);

					output[b*vWidth + a] += (output[b*vWidth + a] < 255);
				}
				
			}
		}
	}
}



//-----------User-Defined-Function------------- 
//#pragma CODE_SECTION(median_filter, "IRAM");
void median_filter(Uint8* output, Uint8* input) {
	int y, x, i, j;
	int sum = 0;
	
	#pragma MUST_ITERATE(vHeight-FILTER_SIZE, vHeight-FILTER_SIZE+1);
	for(y = FILTER_SIZE/2; y < vHeight - FILTER_SIZE/2; y++)
	{
		#pragma MUST_ITERATE(vWidth-FILTER_SIZE, vWidth-FILTER_SIZE+1);
		for(x = FILTER_SIZE/2; x < vWidth - FILTER_SIZE/2; x++)
		{ 	
			sum = 0;
			
			#pragma MUST_ITERATE(FILTER_SIZE, FILTER_SIZE);
			for (i = y - FILTER_SIZE/2; i <= y + FILTER_SIZE/2; i++) 
				#pragma MUST_ITERATE(FILTER_SIZE, FILTER_SIZE);
				for (j = x - FILTER_SIZE/2; j <= x + FILTER_SIZE/2; j++) {
					sum += input[i*vWidth + j];
				}
			
			output[y*vWidth + x] = (sum > FILTER_SIZE * FILTER_SIZE/2.0);
		}
	}
}



//-----------User-Defined-Function------------- 
//#pragma CODE_SECTION(detect_edge, "IRAM");
void detect_edge(Uint8 *output, Uint8 *input){
	int i, j;
	#pragma MUST_ITERATE(vHeight-2, vHeight-2);
	for(i=1;i<vHeight-1;i++)
	{
		#pragma MUST_ITERATE(vWidth-2, vWidth-2);
		for(j=1;j<vWidth-1;j++)
		{
			output[i*vWidth+j] = isEdge(input, i, j, vWidth);
		}
	}
}



//-----------User-Defined-Function------------- 
//#pragma CODE_SECTION(display_point, "IRAM");
void display_point(int y, int x, int color) {
//	int i, j;
	int pos = y*vWidth/2+x;
	
	buffer_out[pos] = color;
	buffer_out[pos+1] = color;
	
	buffer_out[pos+vWidth/2] = color;
	buffer_out[pos+vWidth/2+1] = color;
	
	pos += vWidth;
	
	buffer_out[pos] = color;
	buffer_out[pos+1] = color;
	
	buffer_out[pos+vWidth/2] = color;
	buffer_out[pos+vWidth/2+1] = color;
	
	pos += vWidth;
	
	buffer_out[pos] = color;
	buffer_out[pos+1] = color;
	
	buffer_out[pos+vWidth/2] = color;
	buffer_out[pos+vWidth/2+1] = color;
}



//-----------User-Defined-Function------------- 
//#pragma CODE_SECTION(draw_block, "IRAM");
void draw_block(int y, int x, int halfH, int halfW, int color)
{
	int i, j;
	int left = x - halfW;
	int right = x + halfW;
	int top = y - halfH;
	int bottom = y + halfH;
   
	for(j = left; j <= right; j++) {
		buffer_out[top * vWidth/2 + j] = color;
		buffer_out[bottom * vWidth/2 + j] = color;
	}
   	
   	for(i = top; i <= bottom; i++) {
   		buffer_out[i * vWidth/2 + left] = color;
		buffer_out[i * vWidth/2 + right] = color;
   	}
}




//-----------User-Defined-Function------------- 
//#pragma CODE_SECTION(find_center, "IRAM");
int find_center(Uint8* label, int threshold, short* occupied, int color) {
	int i, j, a, b;
	int count = 0;
	int pos;
	
	#pragma MUST_ITERATE(ARRAY_SIZE, ARRAY_SIZE);
	for(i=0;i<ARRAY_SIZE;i++)
		label[i] = (label[i] > threshold);
	
	#pragma MUST_ITERATE(vHeight-WIN_SZ-WIN_SZ, vHeight-WIN_SZ-WIN_SZ);
	for(i=WIN_SZ;i<vHeight-WIN_SZ;i++)

		#pragma MUST_ITERATE(vWidth-WIN_SZ-WIN_SZ, vWidth-WIN_SZ-WIN_SZ);
		for(j=WIN_SZ;j<vWidth-WIN_SZ;j++)
		{
			pos = i * vWidth + j;
			if(label[pos] && !occupied[pos])
			{
				#pragma MUST_ITERATE(WIN_SZ+WIN_SZ+1, WIN_SZ+WIN_SZ+1);
				for(a=-WIN_SZ;a<=WIN_SZ;a++)
				{	
					#pragma MUST_ITERATE(WIN_SZ+WIN_SZ+1, WIN_SZ+WIN_SZ+1);
					for(b=-WIN_SZ;b<=WIN_SZ;b++)
					{
						label[pos + (a*vWidth+b)] = 0;
						occupied[pos + (a*vWidth+b)] = 1;
					}
				}
				label[pos]=255;
				display_num(i, j/2, ++count, color);
//				draw_block(i, j/2, 12, 12, BLUE);
			}
		}

	// count centers
//	for(i=0;i<ARRAY_SIZE;i++)
//	{
//		if(label[i]==255)
//		{
//			display_num(i/vWidth, (i%vWidth)/2, ++count, WHITE);
//		}
//	}
	return count;
}






//*********************************************//
//-----------Hough Transform Function------------- 
//*********************************************//

int hough(){
	int i,j; 
	//int filter, a,b, sum0=0, sum1=1, Gx, Gy, temp, x[] = {1,K,1,0,0,0,-1,-K,-1}, y[] = {-1,0,1,-K,0,K,-1,0,1},R;
	//double Gxy;
	
    int cent = 0;
    int dime = 0;
    int nickel = 0;
    int quarter = 0;
    int sum;
    
    if (!cleared) clear_all();
    
    memset(forbidden, 0, sizeof(short)*ARRAY_SIZE);
		
//	printf("start hough\n");

	/********* process on copper coins ***********/
	
	//median filter
	memset(hough_output, 0, ARRAY_SIZE);   //clear hough_output[]
	
    median_filter(hough_output, copper);
    

	//void edge_detection(unsigned char []){
	//filter=3;	
	#pragma MUST_ITERATE(vHeight-2, vHeight-2);
	for(i=1;i<vHeight-1;i++)
	{
		#pragma MUST_ITERATE(vWidth-2, vWidth-2);
		for(j=1;j<vWidth-1;j++)
		{
			copper[i*vWidth+j] = isEdge(hough_output, i, j, vWidth);
		}
	}
//	detect_edge(copper, hough_output);

	//COUNT CENTS, R=24, RC=25;
		memset(hough_output, 0, ARRAY_SIZE);    //clear hough_output[]
		draw_circle(hough_output, copper, RC);
		cent = find_center(hough_output, 27, forbidden, ORANGE);
		
		display_num(MID,HOZ_MID_1,cent,ORANGE);
	
	/********* process on silver coins ***********/
	
	//median filter
	memset(hough_output, 0, ARRAY_SIZE);    //clear hough_output[]
	median_filter(hough_output, silver);

	//edge-detection for silver image
	//filter=3;
	#pragma MUST_ITERATE(vHeight-2, vHeight-2);
	for(i=1;i<vHeight-1;i++)
	{
		#pragma MUST_ITERATE(vWidth-2, vWidth-2);
		for(j=1;j<vWidth-1;j++)
		{
			silver[i*vWidth+j] = isEdge(hough_output, i, j, vWidth);
		}
	}
//	detect_edge(silver, hough_output);
	

	// COUNT DIMES, R=22, RD=23;
		memset(hough_output, 0, ARRAY_SIZE);   //clear hough_output[]
		draw_circle(hough_output, silver, RD);
		dime = find_center(hough_output, 38, forbidden, BLUE);
		
		display_num(MID,HOZ_MID_2,dime,BLUE);
		
	
	//COUNT QUARTERS, R=31, RQ=32;
		memset(hough_output, 0, ARRAY_SIZE);
		draw_circle(hough_output, silver, RQ);
		quarter = find_center(hough_output, 36, forbidden, GREEN);
		
		display_num(MID,HOZ_MID_4,quarter,GREEN);
		
	
	// COUNT NICKELS, R=26, RN=27;
		memset(hough_output, 0, ARRAY_SIZE);
		draw_circle(hough_output, silver, RN);
		nickel = find_center(hough_output, 35, forbidden, BLACK);
		
		display_num(MID,HOZ_MID_3,nickel,BLACK);
		
		
	//RETURN CURRENCY
		sum = (dime*10)+(nickel*5)+(quarter*25)+(cent);	
			
		display_num(TOP+52, LEFT+245, sum/100, LEMON);
		
		display_num(TOP+52, LEFT+263, sum%100, LEMON);
		
		display_point(TOP+54, LEFT+253, LEMON);
		
		cleared = 0;
				
		return sum;	
}




// Using EDMA, copy data from input buffer to output buffer; both on the external memory
void DirectTransferEDMA(void){
	int i;
	int *Event;

	// DON'T change the following setting except the source and destination address
	// Event[PaRAM_SRC] is the source data
	// Event[PaRAM_DST] is the destination data
	// Direct copy from External to External
	Event			 = (int *)(0x01C04000 + 32 * 9);
	Event[PaRAM_OPT] = 0x0010000C;
	Event[PaRAM_SRC] = (int)buffer_in;			// Source address
	Event[PaRAM_BCNT]= ((vHeight) << 16) | (vWidth/2 * 4);
	Event[PaRAM_DST] = (int)buffer_out;  	// Destination address
	Event[PaRAM_BIDX]= ((vWidth/2 * 4) << 16) | (vWidth/2 * 4);
	Event[PaRAM_RDL] = 0x0000FFFF;  //
	Event[PaRAM_CIDX]= 0x00000000;
	Event[PaRAM_CCNT]= 0x00000001;
	
	// DON'T change the following setting
	for(i = 0; i < 500; i++) 
		if(EDMA_IPR & 0x200) break; // Waiting for EDMA channel 9 transfer complete	
	EDMA_IPR = 0x200;             // Clear CIP9
	EDMA_ESR = EDMA_ESR | 0x200;    // Start channel 9 EDMA transfer
}




void Copy(int label){
	int i, j;
	
	// Copy data from input buffer to output buffer and 
	// draw green square box at the center of view
	for(i=UPPER_LINE; i<vHeight; ++i)
		for (j=0; j<vWidth/2; ++j) {
			if (label == REMAIN)
				buffer_out[i * vWidth/2 + j] = buffer_out[i * vWidth/2 + j];
				
			else if (label == GRAY_SCALE)
				buffer_out[i * vWidth/2 + j] = remove_color(buffer_in[i * vWidth/2 + j]);
			
			else if (i >= TOP && i < TOP+85)
				buffer_out[i * vWidth/2 + j] = DARK_SLATE;
				
//			else if (label == BINARY)
//				buffer_out[i * vWidth/2 + j] = binarize(buffer_in[i * vWidth/2 + j]);
				
			else 
				buffer_out[i * vWidth/2 + j] = buffer_in[i * vWidth/2 + j];
		}
	

}



void initial_config(void) {
	int i, j;
	
	for(i=0; i<UPPER_LINE; ++i)
		for (j=0; j<vWidth/2; ++j) {
				buffer_out[i * vWidth/2 + j] = 0x4F7F4FA1; //dark slate blue
		}
	
	for(i=MID-24; i<MID+24; ++i)
		for (j=LEFT; j<LEFT+24; ++j) {     //12
				buffer_out[i * vWidth/2 + j] = cent_icon[(i-MID+24) * 24 + j-LEFT];
		}
		
	draw_block(MID, HOZ_MID_1, 18, 9, BLACK);
	
	for(i=MID-22; i<MID+22; ++i)
		for (j=LEFT+50; j<LEFT+72; ++j) {   //61
				buffer_out[i * vWidth/2 + j] = dime_icon[(i-MID+22) * 22 + j-LEFT-50];
		}
		
	draw_block(MID, HOZ_MID_2, 18, 9, BLACK);
		
	for(i=MID-26; i<MID+26; ++i)
		for (j=LEFT+100; j<LEFT+126; ++j) {  //113
				buffer_out[i * vWidth/2 + j] = nickel_icon[(i-MID+26) * 26 + j-LEFT-100];
		}
		
	draw_block(MID, HOZ_MID_3, 18, 9, BLACK);
	
	for(i=MID-30; i<MID+30; ++i)
		for (j=LEFT+152; j<LEFT+182; ++j) {  //167
				buffer_out[i * vWidth/2 + j] = quarter_icon[(i-MID+30) * 30 + j-LEFT-152];
		}
		
	draw_block(MID, HOZ_MID_4, 18, 9, BLACK);
	
    for(i=TOP+5; i<TOP+5 + 25; ++i)
    	for (j=LEFT+215; j<LEFT+215 + 90; ++j) {
        	if (!total_sign[(i-TOP-5) * 90 + j-LEFT-215])
				buffer_out[i * vWidth/2 + j] = LEMON;
          }
          
	for(i=TOP+40; i<TOP+40 + 24; ++i)
		for (j=LEFT+220; j<LEFT+220 + 12; ++j) {
			if (!dollar_sign[(i-TOP-40) * 12 + j-LEFT-220])
				buffer_out[i * vWidth/2 + j] = LEMON; //0xD491D436; //lemon yellow
		}

	clear_all();
}



void initial_sine_table(void) {
	int i;
	for ( i = 0 ; i < table_size ; i++){
		sine_table[i] =   (short)((sin(PI*STEP*i/180)) *1024);
//		cosine_table[i] = (short)((cos(PI*STEP*i/180)) *1024);
	}
}



void main( void )
{
	Int16 dip0, dip1, dip2, dip3;

	/* Initialize BSL */
	EVMDM6437_init();
	
    /* Initialize the DIP Switches & LEDs if needed */
    EVMDM6437_DIP_init( );
    EVMDM6437_LED_init( );
    
	video_loopback_test( (Uint32)buffer_in, (Uint32)buffer_out );
	
	initial_sine_table();
	initial_config();
	
	while (1){	

        /* Will return DIP_DOWN or DIP_UP */
        dip0 = EVMDM6437_DIP_get( DIP_0 );
        dip1 = EVMDM6437_DIP_get( DIP_1 );
        dip2 = EVMDM6437_DIP_get( DIP_2 );
        dip3 = EVMDM6437_DIP_get( DIP_3 );

        // Run different procedures depending on the DIP switches pressed.
        if ( dip0 == DIP_DOWN ) {
//        	BlockProcessingEDMA();
			Copy(GRAY_SCALE);
			clear_all();
        }
        else if ( dip1 == DIP_DOWN ) {
//        	DirectTransferEDMA();
			separate_copper_silver();
        	hough();
        }
        else if ( dip2 == DIP_DOWN ) {
        	separate_copper_silver();
        }
        else if ( dip3 == DIP_DOWN ) {
//        	Inv();
			Copy(COLOR);
        }
        else {
        	Copy(REMAIN);
        }
	}
}


