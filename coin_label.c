#include <math.h>
#include "coin_label.h"

Uint8 classify(int y,int Cr, Uint8* copper, Uint8* silver, int k){
	       
	double index1=0.0000,index2=0.0000;
	double del_1Y, del_1Cr, del_2Y, del_2Cr;  //delX and delY refer matlab code
    double prob1, prob2;
    
//    if (y < BG_TH)  return 0;
    
//#define MEAN1_Y    75
//#define MEAN1_Cr   143
//#define VAR1_Y      485
//#define VAR1_Cr     53
//#define CORR1      ( 15876.0 / (VAR1_Y * VAR1_Cr))
//#define PROP1      ( 126.0 / (VAR1_Y * VAR1_Cr))
//
//
//#define MEAN2_Y    110
//#define MEAN2_Cr   131
//#define VAR2_Y      414
//#define VAR2_Cr     7
//#define CORR2      ( 16.0 / (VAR2_Y * VAR2_Cr) )
//#define PROP2      ( 4.0 / (VAR2_Y * VAR2_Cr) )
	
	del_1Y = y - MEAN1_Y; 
	del_1Cr = Cr - MEAN1_Cr;

	del_2Y = y - MEAN2_Y; 
	del_2Cr = Cr - MEAN2_Cr;

	index1 = (1.0/(1.0-CORR1)) * ( del_1Y*del_1Y / VAR1_Y + del_1Cr*del_1Cr / VAR1_Cr - 2*PROP1 * del_1Y*del_1Cr);
	index2 = (1.0/(1.0-CORR2)) * ( del_2Y*del_2Y / VAR2_Y + del_2Cr*del_2Cr / VAR2_Cr - 2*PROP2 * del_2Y*del_2Cr);


	prob1 = (1.0/2.0*PI) / sqrt(VAR1_Y * VAR1_Cr * (1.0-CORR1) * exp(index1));
	prob2 = (1.0/2.0*PI) / sqrt(VAR2_Y * VAR2_Cr * (1.0-CORR2) * exp(index2));
			
	if (prob1 < prob2) {
		silver[k] = 1;
		return 2;
	}
	else {
		copper[k] = 1;
		return 1;
	}
}


Uint8 binary_filter(Uint8* label, int y, int x, int width) {
	int i, j;
	int sum = 0;
	
	for (i = y - FILTER_SIZE/2; i <= y + FILTER_SIZE/2; i++)
		for (j = x - FILTER_SIZE/2; j <= x + FILTER_SIZE/2; j++) {
			sum += label[i*width + j];
		}
		
	if (sum > FILTER_SIZE * FILTER_SIZE/2.0) 
		return 1;
	else 
		return 0;
}


//Uint8 isEdge(Uint8 *label, int y, int x, int width) {
//	int pos = y*width + x;
//	int diff;
//	char pattern = 0;
//	
//	pattern |= label[pos++] << 7;      //upper-left
//	pattern |= label[pos++] << 6;      //upper
//	pattern |= label[pos++] << 5;      //upper-right
//	
//	pos += width;
//	
//	pattern |= label[pos] << 4;        //right
//	
//	pos += width;
//	
//	pattern |= label[pos--] << 3;      //lower-right
//	pattern |= label[pos--] << 2;      //lower
//	pattern |= label[pos--] << 1;      //lower-left
//	
//	pos -= width;
//	
//	pattern |= label[pos];        //left
//
//
//	if (label[pos-width] - label[pos+width])     
//		return 1; //up to down
//		
//	if (label[pos-width-1] - label[pos+width+1]) 
//		return 1; //upper-left to lower-right
//		
//	if (label[pos-width+1] - label[pos+width-1]) 
//		return 1; //upper-right to lower-left
//		
//	if (label[pos+1] - label[pos-1]) 
//	    return 1; //right-to-left
//
//	
//	diff = label[pos-width-1] + (label[pos-width]<<1) + label[pos-width+1]
//	       - label[pos+width-1] - (label[pos+width]<<1) - label[pos+width+1];
//	//top-bottom difference
//	       
//	if (diff != 0)  return 255;     
//	
//	diff = label[pos-width-1] + (label[pos-1]<<1) + label[pos+width-1]
//	       - label[pos-width+1] - (label[pos+1]<<1) - label[pos+width+1];
//	//left-right difference
//
//	if (diff != 0)  return 255; 
//	else            return 0;
//}






