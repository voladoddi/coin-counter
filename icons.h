#ifndef GLOBAL_H_
#define GLOBAL_H_

/////////////////////////////////////////////////////////////////

#define 	COLOR		0
#define		GRAY_SCALE	1
#define     REMAIN      2

#define     STEP        5
#define     table_size  (360 / STEP + 1)
/////////////////////////////////////////////////////////////////

#define 	FROM_BUFFER_IN  	0
#define		FROM_BUFFER_MID		1


/////////////////////////////////////////////////////////////////
#define PI 3.14159265359

#define LEFT        23
#define TOP         23
#define BOTTOM     (480-20)
#define RIGHT      (360-24)

#define MID         (TOP+35)
#define HOZ_MID_1   (LEFT+34)
#define HOZ_MID_2   (LEFT+84)
#define HOZ_MID_3   (LEFT+137)
#define HOZ_MID_4   (LEFT+192)

#define UPPER_LINE (TOP+85)

#define BG_TH       40


/////////////////////////////////////////////////////////////////
#define RC  25   //copper
#define RD  23   //dime
#define RN  27   //nickel
#define RQ  32   //Quarter

/////////////////////////////////////////////////////////////////

#define ORANGE 0x10FF1000
#define BLUE   0x296E29F0
#define GREEN  0x10001000

#define PINK   0x10FF10FF
#define LEMON  0xD491D436

#define WHITE  0xEB80EB80
#define BLACK  0x107F107F

#define DARK_SLATE  0x4F7F4FA1
#define BEIGE       0xE082E075 
#define WHEAT	    0xD08DD06A
#define QUARTZ	    0xCD7ECD8B
#define LIGHT_GRAY	0xC080C080
#define CYAN		0xCC56CC92
/////////////////////////////////////////////////////////////////

#define FILTER_SIZE 7
#define WIN_SZ 7

#define ARRAY_SIZE 345600

/////////////////////////////////////////////////////////////////

#ifndef Uint32
#define Uint32 unsigned int
#endif

#ifndef Uint16
#define Uint16 unsigned short
#endif

#ifndef Uint8
#define Uint8 unsigned char
#endif

/////////////////////////////////////////////////////////////////

#endif /*GLOBAL_H_*/
