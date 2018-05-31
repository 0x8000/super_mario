#include "battle_city.h"
#include "map.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xio.h"
#include <math.h>

/*
 * GENERATED BY BC_MEM_PACKER
 * DATE: Wed Jul 08 21:00:48 2015
 */

// ***** 16x16 IMAGES *****
#define IMG_16x16_background			    0x0100  //0
#define IMG_16x16_bang			            0x0140  //1
#define IMG_16x16_car_blue			         0x0180  //2
#define IMG_16x16_car_red			        0x01C0  //3
#define IMG_16x16_flag			            0x0200  //4
#define IMG_16x16_map_element_00			0x0240  //5
#define IMG_16x16_map_element_01			0x0280  //6
#define IMG_16x16_map_element_02			0x02C0  //7
#define IMG_16x16_map_element_03			0x0300  //8
#define IMG_16x16_map_element_04			0x0340  //9
#define IMG_16x16_map_element_05			0x0380  //a
#define IMG_16x16_map_element_06			0x03C0  //b
#define IMG_16x16_map_element_07			0x0400  //c
#define IMG_16x16_map_element_08			0x0440  //d
#define IMG_16x16_map_element_09			0x0480  //e
#define IMG_16x16_map_element_10			0x04C0  //f
#define IMG_16x16_map_element_11			0x0500  //g
#define IMG_16x16_map_element_12			0x0540  //h
#define IMG_16x16_map_element_13			0x0580  //i
#define IMG_16x16_map_element_14			0x05C0  //j
#define IMG_16x16_map_element_15			0x0600  //k
#define IMG_16x16_map_element_16			0x0640  //l
#define IMG_16x16_map_element_17			0x0680  //m
#define IMG_16x16_map_element_18			0x06C0  //n
#define IMG_16x16_map_element_19			0x0700  //o
#define IMG_16x16_map_element_20			0x0740  //p
#define IMG_16x16_map_element_21			0x0780  //q
#define IMG_16x16_map_element_22			0x07C0  //r
#define IMG_16x16_map_element_23			0x0800  //s
#define IMG_16x16_map_element_24			0x0840  //t
#define IMG_16x16_map_element_25			0x0880  //u
#define IMG_16x16_rock			            0x08C0  //v
#define IMG_16x16_smoke			            0x0900  //w
// ***** MAP *****

#define MAP_BASE_ADDRESS			    2368  // MAP_OFFSET in battle_city.vhd
#define MAP_X							0
#define MAP_X2							640
#define MAP_Y							4
#define MAP_W							64
#define MAP_H							56

#define SPRITES_REG_OFFSET               4096
#define OFFSET_ROW_REG_OFFSET (4096+2048+0)
#define OFFSET_COL_REG_OFFSET (4096+2048+1)
#define STAT_IMG_SIZE_IS_16_REG_OFFSET (4096+2048+2)

#define BTN_DOWN( b )                   ( !( b & 0x01 ) )
#define BTN_UP( b )                     ( !( b & 0x10 ) )
#define BTN_LEFT( b )                   ( !( b & 0x02 ) )
#define BTN_RIGHT( b )                  ( !( b & 0x08 ) )
#define BTN_SHOOT( b )                  ( !( b & 0x04 ) )

// Prva 2 registra su formulko
#define TANK1_REG_L                     8
#define TANK1_REG_H                     9

// Ostali idu redom, oni koji se pomeraju, definisani u battle_city.vhd
#define TANK_AI_REG_L                   4
#define TANK_AI_REG_H                   5
#define TANK_AI_REG_L2                  6
#define TANK_AI_REG_H2                  7
#define TANK_AI_REG_L3                  2
#define TANK_AI_REG_H3                  3
#define TANK_AI_REG_L4                  10
#define TANK_AI_REG_H4                  11

// Lives
#define TANK_AI_REG_L5                  12
#define TANK_AI_REG_H5                  13
#define TANK_AI_REG_L6                  14
#define TANK_AI_REG_H6                  15
#define TANK_AI_REG_L7                  16
#define TANK_AI_REG_H7                  17

// Base registri
#define BASE_REG_L						0
#define BASE_REG_H	                    1

#define CAR_CENTAR_X                    320
#define CAR_CENTAR_Y					240

int lives = 0;
int score = 0;
int mapPart = 1;
int udario_glavom_skok = 0;
int map_move = 0;
int brojac = 0;
int udario_u_blok = 0;

int number_of_flags = 0;

int car_se_pomerio = 0;

int car_map_x = 16;
int car_map_y = 9;

typedef enum {
	b_false, b_true
} bool_t;

typedef enum {
	DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_STILL
} direction_t;

typedef struct {
	unsigned int x;
	unsigned int y;
	direction_t dir;
	unsigned int type;

	int object_in_the_path;

	bool_t destroyed;

	// Sta je reg_l, reg_h
	unsigned int reg_l;
	unsigned int reg_h;

	unsigned int collected_flags;
	unsigned int lives;
} characters;

characters car = { 783,	                          // x
		656, 		                     // y
		DIR_STILL,               			// dir
		IMG_16x16_car_blue,  			// type

		1, 								//object_in_the_path
		b_false,                		// destroyed

		TANK1_REG_L,            		// reg_l
		TANK1_REG_H,             		// reg_h

		0, 3

};

static void chhar_spawn(characters * chhar) {
	Xil_Out32(
			XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + chhar->reg_l ),
			(unsigned int )0x8F010000 | (unsigned int )chhar->type);
	Xil_Out32(
			XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + chhar->reg_h ),
			(240 << 16) | 320);
}

static void map_update(characters * car) {
	int x, y;

	long int addr;

	int current_car_map_x = car->x / 16;
	int current_car_map_y = car->y / 16;

	int i = current_car_map_x - car_map_x;
	int j = current_car_map_y - car_map_y;

	car_map_x = current_car_map_x;
	car_map_y = current_car_map_y;

	float Xx;
	float Yy;
	int roundX = 0;
	int roundY = 0;

	Xx = car->x;
	Yy = car->y;

	roundX = floor(Xx / 16);
	roundY = floor(Yy / 16);

	int z, w;

	for (y = 0; y < MAP_HEIGHT; y++) {
		for (x = 0; x < MAP_WIDTH; x++) {
			addr = XPAR_BATTLE_CITY_PERIPH_0_BASEADDR
					+ 4 * (MAP_BASE_ADDRESS + y * MAP_WIDTH + x);
			switch (map1[(roundY - 15) + y][(roundX - 20) + x]) {

			case '0':
				Xil_Out32(addr, IMG_16x16_background);
				break;
			case '1':
				Xil_Out32(addr, IMG_16x16_bang);
				break;
			case '2':
				Xil_Out32(addr, IMG_16x16_car_blue);
				break;
			case '3':
				Xil_Out32(addr, IMG_16x16_car_red);
				break;
			case '4':
				Xil_Out32(addr, IMG_16x16_flag);
				break;
			case '5':
				Xil_Out32(addr, IMG_16x16_map_element_00);
				break;
			case '6':
				Xil_Out32(addr, IMG_16x16_map_element_01);
				break;
			case '7':
				Xil_Out32(addr, IMG_16x16_map_element_02);
				break;
			case '8':
				Xil_Out32(addr, IMG_16x16_map_element_03);
				break;
			case '9':
				Xil_Out32(addr, IMG_16x16_map_element_04);
				break;
			case 'a':
				Xil_Out32(addr, IMG_16x16_map_element_05);
				break;
			case 'b':
				Xil_Out32(addr, IMG_16x16_map_element_06);
				break;
			case 'c':
				Xil_Out32(addr, IMG_16x16_map_element_07);
				break;
			case 'd':
				Xil_Out32(addr, IMG_16x16_map_element_08);
				break;
			case 'e':
				Xil_Out32(addr, IMG_16x16_map_element_09);
				break;
			case 'f':
				Xil_Out32(addr, IMG_16x16_map_element_10);
				break;
			case 'g':
				Xil_Out32(addr, IMG_16x16_map_element_11);
				break;
			case 'h':
				Xil_Out32(addr, IMG_16x16_map_element_12);
				break;
			case 'i':
				Xil_Out32(addr, IMG_16x16_map_element_13);
				break;
			case 'j':
				Xil_Out32(addr, IMG_16x16_map_element_14);
				break;
			case 'k':
				Xil_Out32(addr, IMG_16x16_map_element_15);
				break;
			case 'l':
				Xil_Out32(addr, IMG_16x16_map_element_16);
				break;
			case 'm':
				Xil_Out32(addr, IMG_16x16_map_element_17);
				break;
			case 'n':
				Xil_Out32(addr, IMG_16x16_map_element_18);
				break;
			case 'o':
				Xil_Out32(addr, IMG_16x16_map_element_19);
				break;
			case 'p':
				Xil_Out32(addr, IMG_16x16_map_element_20);
				break;
			case 'q':
				Xil_Out32(addr, IMG_16x16_map_element_21);
				break;
			case 'r':
				Xil_Out32(addr, IMG_16x16_map_element_22);
				break;
			case 's':
				Xil_Out32(addr, IMG_16x16_map_element_23);
				break;
			case 't':
				Xil_Out32(addr, IMG_16x16_map_element_24);
				break;
			case 'u':
				Xil_Out32(addr, IMG_16x16_map_element_25);
				break;
			case 'v':
				Xil_Out32(addr, IMG_16x16_rock);
				break;
			case 'w':
				Xil_Out32(addr, IMG_16x16_smoke);
				break;

			default:
				Xil_Out32(addr, IMG_16x16_background);
				break;
			}
		}

	}

}

void update_car_position(characters * car) {
	u8 offset_x;
	u8 offset_y;
	if (car->object_in_the_path == 1) {
		switch (car->dir) {
		case DIR_RIGHT:
			car->x += 2;
			offset_x = car->x & 0xf; // % 16

			Xil_Out32(
					XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4*OFFSET_COL_REG_OFFSET,
					offset_x);

			Xil_Out32(
					XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + car->reg_l ),
					(unsigned int )0x8F000000 | (unsigned int )car->type);

			break;

		case DIR_LEFT:
			car->x -= 2;
			offset_x = car->x & 0xf; // % 16

			Xil_Out32(
					XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4*OFFSET_COL_REG_OFFSET,
					offset_x);

			Xil_Out32(
					XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + car->reg_l ),
					(unsigned int )0x8F020000 | (unsigned int )car->type);

			break;
		case DIR_UP:
			car->y -= 2;
			offset_y = car->y & 0xf; // % 16

			Xil_Out32(
					XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4*OFFSET_ROW_REG_OFFSET,
					offset_y);

			Xil_Out32(
					XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + car->reg_l ),
					(unsigned int )0x8F010000 | (unsigned int )car->type);

			break;
		case DIR_DOWN:
			car->y += 2;
			offset_y = car->y & 0xf; // % 16

			Xil_Out32(
					XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4*OFFSET_ROW_REG_OFFSET,
					offset_y);

			Xil_Out32(
					XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + car->reg_l ),
					(unsigned int )0x8F100000 | (unsigned int )car->type);

			break;

		}
	}

}

/**
 * Provera kolizije sa bitnim objektima
 */
int provera(int x, int y) {
	unsigned int i;

	float fx = x;
	float fy = y;

	u8 roundX = x >> 4;
	u8 roundY = y >> 4;

	if (map1[roundY][roundX] == '0') {
		return 1;
	} else if (map1[roundY][roundX] == '4') {
		// Flag
		map1[roundY][roundX] = '0';

		(&car)->collected_flags++;
		print_flags(&car);
		map_update(&car);

		if ((&car)->collected_flags == 3) {
			for (i = 0; i < 10000000; i++);  // wait for a bit
			start_new_game(&car);
		}

		return 1;

	} else if (map1[roundY][roundX] == 'v') {
		// Rock
		(&car)->lives--;

		map1[roundY][roundX] = '1';  // bang bang!
		print_lives(&car);
		map_update(&car);

		for (i = 0; i < 10000000; i++);

		map1[roundY][roundX] = '0';

		if ((&car)->lives == 0) {
			start_new_game(&car);
		}

		return 0;
	} else
		return 0;
}

int detekcija_okoline(characters *car) {
	if (car->dir == DIR_RIGHT) {
		if (provera(car->x + 16, car->y + 2) == 1
				&& (provera(car->x + 16, car->y + 14) == 1)) {
			car->object_in_the_path = 1;
		}
	}
	if (car->dir == DIR_LEFT) {
		if (provera(car->x, car->y + 2) == 1
				&& (provera(car->x, car->y + 14) == 1)) {
			car->object_in_the_path = 1;
		}
	}
	if (car->dir == DIR_UP) {
		if (provera(car->x + 2, car->y) == 1
				&& (provera(car->x + 14, car->y) == 1)) {
			car->object_in_the_path = 1;
		}
	}
	if (car->dir == DIR_DOWN) {
		if (provera(car->x + 2, car->y + 16) == 1
				&& (provera(car->x + 12, car->y + 16) == 1)) {
			car->object_in_the_path = 1;
		}
	}
}

void print_lives(characters *car) {

	if (car->lives == 3) {
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L5 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_car_blue);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H5 ),
				(24 << 16) | 568);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L6 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_car_blue);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H6 ),
				(24 << 16) | 592);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L7 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_car_blue);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H7 ),
				(24 << 16) | 616);
	}

	if (car->lives == 2) {
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L5 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_bang);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H5 ),
				(24 << 16) | 568);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L6 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_car_blue);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H6 ),
				(24 << 16) | 592);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L7 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_car_blue);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H7 ),
				(24 << 16) | 616);
	}

	if (car->lives == 1) {
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L5 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_bang);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H5 ),
				(24 << 16) | 568);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L6 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_bang);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H6 ),
				(24 << 16) | 592);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L7 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_car_blue);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H7 ),
				(24 << 16) | 616);
	}

	if (car->lives == 0) {
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L5 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_bang);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H5 ),
				(24 << 16) | 568);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L6 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_bang);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H6 ),
				(24 << 16) | 592);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L7 ),
				(unsigned int )0x8F000000 | (unsigned int )IMG_16x16_bang);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H7 ),
				(24 << 16) | 616);
	}
}

void start_new_game(characters *car) {
	unsigned int i, j;

	chhar_spawn(car);

	car->x = 783;
	car->y = 656;

	car->lives = 3;
	car->collected_flags = 0;

	// Spawn flags and rocks again
	for (i = 0; i < 80; i++){
		for(j = 0; j < 100; j++){
			map1[i][j] = map1_original[i][j];
		}
	}

	print_lives(car);
	print_flags(car);

	map_update(car);
}

void print_flags(characters *car) {

	if (car->collected_flags == 0) {
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L ),
				(unsigned int )0x8F000000 | (unsigned int ) 1);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H ),
				(24 << 16) | 1000);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L2 ),
				(unsigned int )0x8F000000 | (unsigned int ) 1);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H2 ),
				(24 << 16) | 1000);

		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L3 ),
				(unsigned int )0x8F000000 | (unsigned int ) 1);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H3 ),
				(24 << 16) | 1000);
	} else if (car->collected_flags == 1) {
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L ),
				(unsigned int )0x8F000000 | (unsigned int ) IMG_16x16_flag);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H ),
				(24 << 16) | 24);
	} else if (car->collected_flags == 2) {
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L2 ),
				(unsigned int )0x8F000000 | (unsigned int ) IMG_16x16_flag);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H2 ),
				(24 << 16) | 40);
	} else if (car->collected_flags == 3) {
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_L3 ),
				(unsigned int )0x8F000000 | (unsigned int ) IMG_16x16_flag);
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( SPRITES_REG_OFFSET + TANK_AI_REG_H3 ),
				(24 << 16) | 56);
	}

}

void battle_city() {

	unsigned int i;
	unsigned int buttons;

	start_new_game(&car);

	map_update(&car);
	print_lives(&car);

	while (1) {

		buttons = XIo_In32( XPAR_IO_PERIPH_BASEADDR );
		car.object_in_the_path = 0;
		car.dir = DIR_STILL;
		if (BTN_LEFT(buttons)) {
			car.dir = DIR_LEFT;
		} else if (BTN_RIGHT(buttons)) {
			car.dir = DIR_RIGHT;
		}

		if (BTN_UP (buttons) && !BTN_LEFT(buttons) && !BTN_RIGHT(buttons)) {
			car.dir = DIR_UP;
		}
		if (BTN_DOWN (buttons) && !BTN_LEFT(buttons) && !BTN_RIGHT(buttons)) {
			car.dir = DIR_DOWN;
		}

		detekcija_okoline(&car);
		update_car_position(&car);
		map_update(&car);

		for (i = 0; i < 100000; i++) {
		}

	}
}
