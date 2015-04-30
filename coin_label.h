#ifndef COIN_LABEL_H_
#define COIN_LABEL_H_

#include "global.h"

/////////////////////////////////////////////////////////////////

//double mean[4]= {65.1783,99.3107,142.7550,131.0024};
//double sd1[3]= {399.2461,88.5944,44.5489};
//double sd2[3]= {500.2593,5.0016,11.2989};

#define MEAN1_Y    82//65.1783// 69.5159 //
#define MEAN1_Cr   143//99.3107//100.9549 //
#define VAR1_Y     485 //399.2461//524.5965 //
#define VAR1_Cr    53 //44.5489//68.3434 //
#define CORR1      ( 15876.0 / (VAR1_Y * VAR1_Cr))//( 88.5944 * 88.5944 / (VAR1_Y * VAR1_Cr))//( 120.9906 * 120.9906 / (VAR1_Y * VAR1_Cr))//
#define PROP1      ( 126.0 / (VAR1_Y * VAR1_Cr))//( 88.5944 / (VAR1_Y * VAR1_Cr))//( 120.9906 / (VAR1_Y * VAR1_Cr))//( 126.0 / (VAR1_Y * VAR1_Cr))
//#define VAR1_Y      40
//#define VAR1_Cr     4
//#define CORR1       8
//#define PROP1       8


#define MEAN2_Y    110//142.7550//102.0549 //
#define MEAN2_Cr   131//131.0024//110.7191 //
#define VAR2_Y      414//500.2593//381.7973 //
#define VAR2_Cr     7//11.2989//21.9995 //
#define CORR2       ( 16.0 / (VAR2_Y * VAR2_Cr) )//( 5.0016*5.0016 / (VAR2_Y * VAR2_Cr) )//( 20.3553*20.3553 / (VAR2_Y * VAR2_Cr) ) //
#define PROP2      ( 4.0 / (VAR2_Y * VAR2_Cr) )//( 5.0016/ (VAR2_Y * VAR2_Cr) )//( 20.3553 / (VAR2_Y * VAR2_Cr) ) //
//#define VAR2_Y      50
//#define VAR2_Cr     1
//#define CORR2       0.5
//#define PROP2       0.5

/////////////////////////////////////////////////////////////////

Uint8 classify(int y,int Cr, Uint8* copper, Uint8* silver, int k);

Uint8 binary_filter(Uint8* label, int y, int x, int width);

/////////////////////////////////////////////////////////////////

inline Uint32 binarize(Uint32 yCbCr422) {
	if (((yCbCr422 >> 24) & 0xff) <= BG_TH && ((yCbCr422 >> 8) & 0xff) <= BG_TH)
		return 0x107F107F;  // black
	else 
		return 0x10001000;  // green
}

inline Uint32 remove_color(Uint32 yCbCr422) {
	return (yCbCr422 & 0xFF00FF00) | 0x00800080;
}

inline Uint8 isEdge(Uint8 *label, int y, int x, int width) {
	int pos = y*width + x;

	return (
		   	(label[pos-width-1] + (label[pos-width]<<1) + label[pos-width+1]
	         - label[pos+width-1] - (label[pos+width]<<1) - label[pos+width+1])
	//top-bottom difference    
	
	    ||  (label[pos-width-1] + (label[pos-1]<<1) + label[pos+width-1]
	         - label[pos-width+1] - (label[pos+1]<<1) - label[pos+width+1])
	//left-right difference
	); 
}

/////////////////////////////////////////////////////////////////




#endif /*COIN_LABEL_H_*/
