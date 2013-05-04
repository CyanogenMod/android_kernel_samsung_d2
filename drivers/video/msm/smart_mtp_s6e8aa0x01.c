/*
 * =================================================================
 *
 *       Filename:  smart_mtp_s6e8aa0x01.c
 *
 *    Description:  Smart dimming algorithm implementation
 *
 *        Author: jb09.kim
 *        Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
*/

#include "smart_mtp_s6e8aa0x01.h"
#include "smart_mtp_2p2_gamma.h"

/*
#define SMART_DIMMING_DEBUG
*/

static char max_lux_table[GAMMA_SET_MAX];
static char min_lux_table[GAMMA_SET_MAX];

/*
*	To support different center cell gamma setting
*/
static char V1_300CD_R;
static char V1_300CD_G;
static char V1_300CD_B;

static char V15_300CD_R;
static char V15_300CD_G;
static char V15_300CD_B;

static char V35_300CD_R;
static char V35_300CD_G;
static char V35_300CD_B;

static char V59_300CD_R;
static char V59_300CD_G;
static char V59_300CD_B;

static char V87_300CD_R;
static char V87_300CD_G;
static char V87_300CD_B;

static char V171_300CD_R;
static char V171_300CD_G;
static char V171_300CD_B;

static char V255_300CD_R_MSB;
static char V255_300CD_R_LSB;

static char V255_300CD_G_MSB;
static char V255_300CD_G_LSB;

static char V255_300CD_B_MSB;
static char V255_300CD_B_LSB;
int color_mods[5][21] = {
    {  0,  0,  5, -18, -16, -10, 0, 0,  3, 0, 0,  3, 0, 0,  3, 0, 0,  3, 0, 0,  3 },
    {  0,  0,  2,  -9,  -8,  -5, 0, 0,  1, 0, 0,  1, 0, 0,  1, 0, 0,  1, 0, 0,  1 },
    {  0,  0,  0,   0,   0,   0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0 },
    {  0,  0, -2,   9,   8,   5, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1 },
    {  0,  0, -5,  18,  16,  10, 0, 0, -3, 0, 0, -3, 0, 0, -3, 0, 0, -3, 0, 0, -3 }
};
unsigned int panelval = 2;
struct SMART_DIM *gpsmart;

static int char_to_int(char data1)
{
	int cal_data;

	if (data1 & 0x80)
		cal_data = 0xffffff00 | data1;
	else
		cal_data = data1;

	return cal_data;
}

void panel_load_colors(unsigned int val)
{
	panelval = val;
	smart_dimming_init(gpsmart);
}
#define v255_coefficient 100
#define v255_denominator 600
static int v255_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;
	int v255_value;

	v255_value = (V255_300CD_R_MSB << 8) | (V255_300CD_R_LSB);
	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + v255_value;
	result_1 = result_2 = (v255_coefficient+add_mtp + color_mods[panelval][0]) << BIT_SHIFT;
	do_div(result_2, v255_denominator);
	result_3 = (S6E8AA0X01_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E8AA0X01_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_0 = S6E8AA0X01_VREG0_REF;

	v255_value = (V255_300CD_G_MSB << 8) | (V255_300CD_G_LSB);
	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + v255_value;
	result_1 = result_2 = (v255_coefficient+add_mtp + color_mods[panelval][1]) << BIT_SHIFT;
	do_div(result_2, v255_denominator);
	result_3 = (S6E8AA0X01_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E8AA0X01_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_0 = S6E8AA0X01_VREG0_REF;

	v255_value = (V255_300CD_B_MSB << 8) | (V255_300CD_B_LSB);
	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + v255_value;
	result_1 = result_2 = (v255_coefficient+add_mtp + color_mods[panelval][2]) << BIT_SHIFT;
	do_div(result_2, v255_denominator);
	result_3 = (S6E8AA0X01_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E8AA0X01_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_0 = S6E8AA0X01_VREG0_REF;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V255 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_255,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_255,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_255);
#endif

	return 0;
}

static void v255_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = S6E8AA0X01_VREG0_REF -
		(pSmart->GRAY.TABLE[index[V255_INDEX]].R_Gray);

	result_2 = result_1 * v255_denominator;
	do_div(result_2, S6E8AA0X01_VREG0_REF);
	result_3 = result_2  - v255_coefficient;
	str[18] = (result_3 & 0xff00) >> 8;
	str[19] = result_3 & 0xff;

	result_1 = S6E8AA0X01_VREG0_REF -
		(pSmart->GRAY.TABLE[index[V255_INDEX]].G_Gray);
	result_2 = result_1 * v255_denominator;
	do_div(result_2, S6E8AA0X01_VREG0_REF);
	result_3 = result_2  - v255_coefficient;
	str[20] = (result_3 & 0xff00) >> 8;
	str[21] = result_3 & 0xff;

	result_1 = S6E8AA0X01_VREG0_REF -
			(pSmart->GRAY.TABLE[index[V255_INDEX]].B_Gray);
	result_2 = result_1 * v255_denominator;
	do_div(result_2, S6E8AA0X01_VREG0_REF);
	result_3 = result_2  - v255_coefficient;
	str[22] = (result_3 & 0xff00) >> 8;
	str[23] = result_3 & 0xff;

}

#define v1_coefficient 5
#define v1_denominator 600
static int v1_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_1);
	add_mtp = LSB + V1_300CD_R;
	result_1 = result_2 = (v1_coefficient + add_mtp + color_mods[panelval][3]) << BIT_SHIFT;
	do_div(result_2, v1_denominator);
	result_3 = (S6E8AA0X01_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E8AA0X01_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_1 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_1);
	add_mtp = LSB + V1_300CD_G;
	result_1 = result_2 = (v1_coefficient+add_mtp + color_mods[panelval][4]) << BIT_SHIFT;
	do_div(result_2, v1_denominator);
	result_3 = (S6E8AA0X01_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E8AA0X01_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_1 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_1);
	add_mtp = LSB + V1_300CD_B;
	result_1 = result_2 = (v1_coefficient+add_mtp + color_mods[panelval][5]) << BIT_SHIFT;
	do_div(result_2, v1_denominator);
	result_3 = (S6E8AA0X01_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E8AA0X01_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_1 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V1 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_1,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_1,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_1);
#endif

	return 0;

}

static void v1_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	str[0] =  pSmart->MTP.R_OFFSET.OFFSET_1 + V1_300CD_R;
	str[1] =  pSmart->MTP.G_OFFSET.OFFSET_1 + V1_300CD_G;
	str[2] =  pSmart->MTP.B_OFFSET.OFFSET_1 + V1_300CD_B;
}


#define v171_coefficient 65
#define v171_denominator 320
static int v171_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_171);
	add_mtp = LSB + V171_300CD_R;
	result_1 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1)
				- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_255);
	result_2 = (v171_coefficient + add_mtp + color_mods[panelval][6]) << BIT_SHIFT;
	do_div(result_2, v171_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_171 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_171);
	add_mtp = LSB + V171_300CD_G;
	result_1 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1)
				- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_255);
	result_2 = (v171_coefficient + add_mtp + color_mods[panelval][7])  << BIT_SHIFT;
	do_div(result_2, v171_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_171 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_171);
	add_mtp = LSB + V171_300CD_B;
	result_1 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1)
				- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_255);
	result_2 = (v171_coefficient+add_mtp + color_mods[panelval][8]) << BIT_SHIFT;
	do_div(result_2, v171_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_171 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V171 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_171,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_171,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_171);
#endif

	return 0;

}

static void v171_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].R_Gray);
	result_2 = result_1 * v171_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[15] = (result_2  - v171_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].G_Gray);
	result_2 = result_1 * v171_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[16] = (result_2  - v171_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].B_Gray);
	result_2 = result_1 * v171_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[17] = (result_2  - v171_coefficient) & 0xff;

}


#define v87_coefficient 65
#define v87_denominator 320
static int v87_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_R;
	result_1 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_171);
	result_2 = (v87_coefficient + add_mtp + color_mods[panelval][9]) << BIT_SHIFT;
	do_div(result_2, v87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_87 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_G;
	result_1 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_171);
	result_2 = (v87_coefficient + add_mtp + color_mods[panelval][10]) << BIT_SHIFT;
	do_div(result_2, v87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_87 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_B;
	result_1 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_171);
	result_2 = (v87_coefficient + add_mtp + color_mods[panelval][11]) << BIT_SHIFT;
	do_div(result_2, v87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_87 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V87 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_87,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_87,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_87);
#endif

	return 0;

}

static void v87_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].R_Gray);
	result_2 = result_1 * v87_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[12] = (result_2  - v87_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].G_Gray);
	result_2 = result_1 * v87_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[13] = (result_2  - v87_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].B_Gray);
	result_2 = result_1 * v87_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[14] = (result_2  - v87_coefficient) & 0xff;
}


#define v59_coefficient 65
#define v59_denominator 320
static int v59_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_59);
	add_mtp = LSB + V59_300CD_R;
	result_1 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_87);
	result_2 = (v59_coefficient + add_mtp + color_mods[panelval][12]) << BIT_SHIFT;
	do_div(result_2, v59_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_59 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_59);
	add_mtp = LSB + V59_300CD_G;
	result_1 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_87);
	result_2 = (v59_coefficient + add_mtp + color_mods[panelval][13]) << BIT_SHIFT;
	do_div(result_2, v59_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_59 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_59);
	add_mtp = LSB + V59_300CD_B;
	result_1 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_87);
	result_2 = (v59_coefficient + add_mtp + color_mods[panelval][14]) << BIT_SHIFT;
	do_div(result_2, v59_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_59 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V59 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_59,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_59,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_59);
#endif

	return 0;

}


static void v59_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V59_INDEX]].R_Gray);
	result_2 = result_1 * v59_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[9] = (result_2  - v59_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V59_INDEX]].G_Gray);
	result_2 = result_1 * v59_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[10] = (result_2  - v59_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V59_INDEX]].B_Gray);
	result_2 = result_1 * v59_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[11] = (result_2  - v59_coefficient) & 0xff;

}


#define v35_coefficient 65
#define v35_denominator 320
static int v35_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35);
	add_mtp = LSB + V35_300CD_R;
	result_1 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_59);
	result_2 = (v35_coefficient + add_mtp + color_mods[panelval][15]) << BIT_SHIFT;
	do_div(result_2, v35_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_35 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35);
	add_mtp = LSB + V35_300CD_G;
	result_1 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_59);
	result_2 = (v35_coefficient + add_mtp + color_mods[panelval][16]) << BIT_SHIFT;
	do_div(result_2, v35_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_35 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35);
	add_mtp = LSB + V35_300CD_B;
	result_1 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_59);
	result_2 = (v35_coefficient + add_mtp + color_mods[panelval][17]) << BIT_SHIFT;
	do_div(result_2, v35_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_35 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V35 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_35,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_35,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_35);
#endif

	return 0;

}

static void v35_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].R_Gray);
	result_2 = result_1 * v35_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V59_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[6] = (result_2  - v35_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].G_Gray);
	result_2 = result_1 * v35_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V59_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[7] = (result_2  - v35_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].B_Gray);
	result_2 = result_1 * v35_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V59_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[8] = (result_2  - v35_coefficient) & 0xff;

}

#define v15_Coefficient 20
#define v15_denominator 320
static int v15_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_15);
	add_mtp = LSB + V15_300CD_R;
	result_1 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_35);
	result_2 = (v15_Coefficient+add_mtp + color_mods[panelval][18]) << BIT_SHIFT;
	do_div(result_2, v15_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_15 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_15);
	add_mtp = LSB + V15_300CD_G;
	result_1 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_35);
	result_2 = (v15_Coefficient + add_mtp + color_mods[panelval][19]) << BIT_SHIFT;
	do_div(result_2, v15_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_15 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_15);
	add_mtp = LSB + V15_300CD_B;
	result_1 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_35);
	result_2 = (v15_Coefficient + add_mtp + color_mods[panelval][20]) << BIT_SHIFT;
	do_div(result_2, v15_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_15 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V15 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_15,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_15,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_15);
#endif

	return 0;

}

static void v15_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V15_INDEX]].R_Gray);
	result_2 = result_1 * v15_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[3] = (result_2  - v15_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V15_INDEX]].G_Gray);
	result_2 = result_1 * v15_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[4] = (result_2  - v15_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V15_INDEX]].B_Gray);
	result_2 = result_1 * v15_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[5] = (result_2  - v15_Coefficient) & 0xff;
}



/*V0, V1,V15,V35,V59,V87,V171,V255*/
int S6E8AA0X01_ARRAY[S6E8AA0X01_MAX] = {0, 1, 15, 35, 59, 87, 171, 255};

int non_linear_V1toV15[] = {
47, 42, 37, 32, 27,
23, 19, 15, 12, 9,
6, 4, 2
};
#define V1toV15_denominator 52

int non_linear_V15toV35[] = {
66, 62, 58, 54, 50,
46, 42, 38, 34, 30,
27, 24, 21, 18, 15,
12, 9, 6, 3,
};
#define V15toV35_denominator 70

#define V35toV59_Coefficient 23
#define V35toV59_Multiple 1
#define V35toV59_denominator 24

#define V59toV87_Coefficient 27
#define V59toV87_Multiple 1
#define V59toV87_denominator 28

#define V87toV171_Coefficient 83
#define V87toV171_Multiple 1
#define V87toV171_denominator 84

#define V171toV255_Coefficient 83
#define V171toV255_Multiple 1
#define V171toV255_denominator 84

static int cal_gray_scale_linear(int up, int low, int coeff,
int mul, int deno, int cnt)
{
	unsigned long long result_1, result_2, result_3, result_4;

	result_1 = up - low;
	result_2 = (result_1 * (coeff - (cnt * mul))) << BIT_SHIFT;
	do_div(result_2, deno);
	result_3 = result_2 >> BIT_SHIFT;
	result_4 = low + result_3;

	return (int)result_4;
}

static int cal_gray_scale_non_linear(int up, int low,
int *table, int deno, int cnt)
{
	unsigned long long result_1, result_2, result_3, result_4;

	result_1 = up - low;
	result_2 = (result_1 * (table[cnt])) << BIT_SHIFT;
	do_div(result_2, deno);
	result_3 = result_2 >> BIT_SHIFT;
	result_4 = low + result_3;

	return (int)result_4;
}


static int generate_gray_scale(struct SMART_DIM *pSmart)
{
	int cnt = 0, cal_cnt = 0;
	int array_index = 0;
	struct GRAY_VOLTAGE *ptable = (struct GRAY_VOLTAGE *)
						(&(pSmart->GRAY.TABLE));

	for (cnt = 0; cnt < S6E8AA0X01_MAX; cnt++) {
		pSmart->GRAY.TABLE[S6E8AA0X01_ARRAY[cnt]].R_Gray =
			((int *)&(pSmart->RGB_OUTPUT.R_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[S6E8AA0X01_ARRAY[cnt]].G_Gray =
			((int *)&(pSmart->RGB_OUTPUT.G_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[S6E8AA0X01_ARRAY[cnt]].B_Gray =
			((int *)&(pSmart->RGB_OUTPUT.B_VOLTAGE))[cnt];
	}

	/*
		below codes use hard coded value.
		So it is possible to modify on each model.
		V0, V1, V15, V35, V59, V87, V171, V255
	*/
	for (cnt = 0; cnt < S6E8AA0X01_GRAY_SCALE_MAX; cnt++) {

		if (cnt == S6E8AA0X01_ARRAY[0]) {
			/* 0 */
			array_index = 0;
		} else if (cnt == S6E8AA0X01_ARRAY[1]) {
			/* 1 */
			cal_cnt = 0;
		} else if ((cnt > S6E8AA0X01_ARRAY[1]) &&
			(cnt < S6E8AA0X01_ARRAY[2])) {
			/* 2 ~ 14 */
			array_index = 2;

			pSmart->GRAY.TABLE[cnt].R_Gray =
			cal_gray_scale_non_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].R_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].R_Gray,
			non_linear_V1toV15, V1toV15_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray =
			cal_gray_scale_non_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].G_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].G_Gray,
			non_linear_V1toV15, V1toV15_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray =
			cal_gray_scale_non_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].B_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].B_Gray,
			non_linear_V1toV15, V1toV15_denominator, cal_cnt);

			cal_cnt++;
		} else if (cnt == S6E8AA0X01_ARRAY[2]) {
			/* 15 */
			cal_cnt = 0;
		} else if ((cnt > S6E8AA0X01_ARRAY[2]) &&
			(cnt < S6E8AA0X01_ARRAY[3])) {
			/* 16 ~ 34 */
			array_index = 3;

			pSmart->GRAY.TABLE[cnt].R_Gray =
			cal_gray_scale_non_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].R_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].R_Gray,
			non_linear_V15toV35, V15toV35_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray =
			cal_gray_scale_non_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].G_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].G_Gray,
			non_linear_V15toV35, V15toV35_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray =
			cal_gray_scale_non_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].B_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].B_Gray,
			non_linear_V15toV35, V15toV35_denominator, cal_cnt);

			cal_cnt++;
		}  else if (cnt == S6E8AA0X01_ARRAY[3]) {
			/* 35 */
			cal_cnt = 0;
		} else if ((cnt > S6E8AA0X01_ARRAY[3]) &&
			(cnt < S6E8AA0X01_ARRAY[4])) {
			/* 35 ~ 58 */
			array_index = 4;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].R_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].R_Gray,
			V35toV59_Coefficient, V35toV59_Multiple,
			V35toV59_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].G_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].G_Gray,
			V35toV59_Coefficient, V35toV59_Multiple,
			V35toV59_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].B_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].B_Gray,
			V35toV59_Coefficient, V35toV59_Multiple,
			V35toV59_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == S6E8AA0X01_ARRAY[4]) {
			/* 59 */
			cal_cnt = 0;
		} else if ((cnt > S6E8AA0X01_ARRAY[4]) &&
			(cnt < S6E8AA0X01_ARRAY[5])) {
			/* 60 ~ 86 */
			array_index = 5;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].R_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].R_Gray,
			V59toV87_Coefficient, V59toV87_Multiple,
			V59toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].G_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].G_Gray,
			V59toV87_Coefficient, V59toV87_Multiple,
			V59toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].B_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].B_Gray,
			V59toV87_Coefficient, V59toV87_Multiple,
			V59toV87_denominator, cal_cnt);
			cal_cnt++;

		} else if (cnt == S6E8AA0X01_ARRAY[5]) {
			/* 87 */
			cal_cnt = 0;
		} else if ((cnt > S6E8AA0X01_ARRAY[5]) &&
			(cnt < S6E8AA0X01_ARRAY[6])) {
			/* 88 ~ 170 */
			array_index = 6;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].R_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].R_Gray,
			V87toV171_Coefficient, V87toV171_Multiple,
			V87toV171_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].G_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].G_Gray,
			V87toV171_Coefficient, V87toV171_Multiple,
			V87toV171_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].B_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].B_Gray,
			V87toV171_Coefficient, V87toV171_Multiple,
			V87toV171_denominator, cal_cnt);
			cal_cnt++;

		} else if (cnt == S6E8AA0X01_ARRAY[6]) {
			/* 171 */
			cal_cnt = 0;
		} else if ((cnt > S6E8AA0X01_ARRAY[6]) &&
			(cnt < S6E8AA0X01_ARRAY[7])) {
			/* 172 ~ 254 */
			array_index = 7;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].R_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].R_Gray,
			V171toV255_Coefficient, V171toV255_Multiple,
			V171toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].G_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].G_Gray,
			V171toV255_Coefficient, V171toV255_Multiple,
			V171toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[S6E8AA0X01_ARRAY[array_index-1]].B_Gray,
			ptable[S6E8AA0X01_ARRAY[array_index]].B_Gray,
			V171toV255_Coefficient, V171toV255_Multiple,
			V171toV255_denominator, cal_cnt);

			cal_cnt++;
		} else {
			if (cnt == S6E8AA0X01_ARRAY[7]) {
				pr_info("%s end\n", __func__);
			} else {
				pr_err("%s fail cnt:%d\n", __func__, cnt);
				return -1;
			}
		}

	}

#ifdef SMART_DIMMING_DEBUG
		for (cnt = 0; cnt < S6E8AA0X01_GRAY_SCALE_MAX; cnt++) {
			pr_info("%s %d R:%d G:%d B:%d\n", __func__, cnt,
				pSmart->GRAY.TABLE[cnt].R_Gray,
				pSmart->GRAY.TABLE[cnt].G_Gray,
				pSmart->GRAY.TABLE[cnt].B_Gray);
		}
#endif

	return 0;
}

/*
*	Because of AID operation & display quality.
*	Three type of gamma curve is used.
*	gamma_curve_change_point is changing point illumination.
*	300CD ~ 260CD use 2.2 gamma curve
*	250CD ~ 190CD use 2.15 gamma curve
*	below 190CD use 2.1 gamma curve
*
*	only smart dimmg range : 300CD ~ 190CD
*	AOR fix range : 180CD ~ 110CD
*	AOR adjust range : 100CD ~ 20CD
*/
#define GAMMA_CURVE_2P2 1
#define GAMMA_CURVE_2P15 2
#define GAMMA_CURVE_2P1 3
#define AOR_FIX_CD 180
#define AOR_ADJUST_CD 110
#define AOR_MUL_COFF 23720 /* 1.4477381 * 16384 */
#define AOR_ADD_COFF 190718 /* 11.6404762 * 16384 */
#define AOR_ADD_COMPENSATION 1

static int translate_table_188_to182[4] = {250, 234, 214, 201};
static int gamma_curve_change_point[3] = {300, 260, 190};
static int aid_compensation_cd[4] = {300, 190, 180, 110};

static int searching_function(long long candela, int *index, int gamma_curve)
{
	long long delta_1 = 0, delta_2 = 0;
	int cnt;

	/*
	*	This searching_functin should be changed with improved
		searcing algorithm to reduce searching time.
	*/
	*index = -1;

	for (cnt = 0; cnt < (S6E8AA0X01_GRAY_SCALE_MAX-1); cnt++) {
		if (gamma_curve == GAMMA_CURVE_2P2) {
			delta_1 = candela - curve_2p2[cnt];
			delta_2 = candela - curve_2p2[cnt+1];
		} else if (gamma_curve == GAMMA_CURVE_2P15) {
			delta_1 = candela - curve_2p15[cnt];
			delta_2 = candela - curve_2p15[cnt+1];
		} else {
			delta_1 = candela - curve_2p1[cnt];
			delta_2 = candela - curve_2p1[cnt+1];
		}

		if (delta_2 < 0) {
			*index = (delta_1 + delta_2) <= 0 ? cnt : cnt+1;
			break;
		}

		if (delta_1 == 0) {
			*index = cnt;
			break;
		}

		if (delta_2 == 0) {
			*index = cnt+1;
			break;
		}
	}

	if (*index == -1)
		return -1;
	else
		return 0;
}


/* -1 means V1 */
#define S6E8AA0X01_TABLE_MAX  (S6E8AA0X01_MAX-1)
void(*Make_hexa[S6E8AA0X01_TABLE_MAX])(int*, struct SMART_DIM*, char*) = {
	v255_hexa,
	v171_hexa,
	v87_hexa,
	v59_hexa,
	v35_hexa,
	v15_hexa,
	v1_hexa
};


#ifdef AID_OPERATION_4_8_INCH
#if defined(AID_CASE_1)
static int below_110cd_v15_adding[9][4] = {
/*	CD	R	G	B	*/
	{20,	-33,	-53,	62},
	{30,	-44,	-53,	32},
	{40,	-44,	-53,	14},
	{50,	-44,	-53,	0},
	{60,	-27,	-32,	0},
	{70,	-17,	-20,	0},
	{80,	-10,	-12,	0},
	{90,	-6,	-7,	0},
	{100,	-2,	-3,	0},
};

static int below_110cd_v35_adding[9][4] = {
/*	CD	R	G	B	*/
	{20,	-15,	-12,	0},
	{30,	0,	0,	0},
	{40,	0,	0,	0},
	{50,	0,	0,	0},
	{60,	0,	0,	0},
	{70,	0,	0,	0},
	{80,	0,	0,	0},
	{90,	0,	0,	0},
	{100,	0,	0,	0},
};

static void gamma_init(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[S6E8AA0X01_TABLE_MAX] = {-1, };
	int bl_index[S6E8AA0X01_TABLE_MAX] = {-1, };

	long long temp_cal_data = 0;
	int bl_level, cnt;
	int level_255_temp = 0;
	int level_255_temp_MSB = 0;
	int point_index;
	unsigned int aid_bl_level;
	int index;

	bl_level = pSmart->brightness_level;

	/*calculate candela level */
	if (bl_level >= gamma_curve_change_point[1]) {
		/* upper 260CD */
		for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++) {
			point_index = S6E8AA0X01_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p2[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
#ifdef SMART_DIMMING_DEBUG
		pr_info("\nupper 260CD LUX:%d\n", bl_level);
#endif
	} else if ((bl_level < gamma_curve_change_point[1]) &&
		(bl_level >= gamma_curve_change_point[2])) {
		/* 260CD ~ 190CD */
		for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++) {
			point_index = S6E8AA0X01_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p15[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
#ifdef SMART_DIMMING_DEBUG
		pr_info("\n260CD ~ 190CD LUX:%d\n", bl_level);
#endif
	} else {
		/*below 190CD */
		if ((bl_level < (AOR_FIX_CD + 10)) &&
				(bl_level >= AOR_ADJUST_CD)) {
			/* 180CD ~ 110CD*/
			if (bl_level <= 180) {
				aid_bl_level = ((AOR_MUL_COFF * bl_level) +
						AOR_ADD_COFF) >> BIT_SHIFT;
				aid_bl_level += AOR_ADD_COMPENSATION;
			} else if (bl_level <= 182)
				aid_bl_level = translate_table_188_to182[0];
			else if (bl_level <= 184)
				aid_bl_level = translate_table_188_to182[1];
			else if (bl_level <= 186)
				aid_bl_level = translate_table_188_to182[2];
			else
				aid_bl_level = translate_table_188_to182[3];
		} else {
			/* 100CD ~ 20CD */
			aid_bl_level = AOR_ADJUST_CD;
		}

		for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++) {
			point_index = S6E8AA0X01_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p1[point_index])) *
			((long long)(aid_bl_level));
			candela_level[cnt] = temp_cal_data;
		}

#ifdef SMART_DIMMING_DEBUG
		pr_info("\nbelow 190CD LUX:%d AID_LUX:%d\n",
			bl_level, (int)aid_bl_level);
#endif
	}

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s candela_1:%llu candela_15:%llu  candela_35:%llu  "
		"candela_59:%llu  candela_171:%llu  candela_171:%llu "
		"candela_255:%llu\n", __func__, candela_level[0],
		candela_level[1], candela_level[2], candela_level[3],
		candela_level[4], candela_level[5], candela_level[6]);
#endif

	/*calculate brightness level*/
	if (bl_level >= gamma_curve_change_point[1]) {
		/* upper 260CD */
		for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++) {
			if (searching_function(candela_level[cnt],
				&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
				pr_info("%s searching functioin error cnt:%d\n",
					__func__, cnt);
			}
		}
	} else if ((bl_level < gamma_curve_change_point[1]) &&
		(bl_level >= gamma_curve_change_point[2])) {
		/* 260CD ~ 190CD */
		for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++) {
			if (searching_function(candela_level[cnt],
				&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
				pr_info("%s searching functioin error cnt:%d\n",
					__func__, cnt);
			}
		}

		/*
		*	190CD compensation
		*	V15, V35, V59, V87 level + 1
		*/
		if (bl_level == 190) {
			bl_index[1] += 1;
			bl_index[2] += 1;
			bl_index[3] += 1;
			bl_index[4] += 1;
		}
	} else {
		/*below 190CD */
		for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++) {
			if (searching_function(candela_level[cnt],
				&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
				pr_info("%s searching functioin error cnt:%d\n",
					__func__, cnt);
			}
		}
	}

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s bl_index_1:%d bl_index_15:%d bl_index_35:%d bl_index_59:%d "
		"bl_index_87:%d bl_index_171:%d bl_index_255:%d\n", __func__,
		bl_index[0], bl_index[1], bl_index[2], bl_index[3],
		bl_index[4], bl_index[5], bl_index[6]);
#endif

	/*Generate Gamma table*/
	for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++)
		(void)Make_hexa[cnt](bl_index , pSmart, str);

	/*subtration MTP_OFFSET value from generated gamma table*/
	if (bl_level < aid_compensation_cd[3]) {
		/* below 110CD */
		index = (bl_level / 10) - 2;
		str[3] = str[3]  + below_110cd_v15_adding[index][1];
		str[4] = str[4]  + below_110cd_v15_adding[index][2];
		str[5] = str[5]  + below_110cd_v15_adding[index][3];

		str[6] = str[6]  + below_110cd_v35_adding[index][1];
		str[7] = str[7]  + below_110cd_v35_adding[index][2];
		str[8] = str[8]  + below_110cd_v35_adding[index][3];
	} else if ((bl_level <= aid_compensation_cd[2]) &&
		(bl_level >= aid_compensation_cd[3])) {
		/* 180CD ~ 110CD */
		str[3] = str[3]  + 1;
		str[4] = str[4]  - 1;
		str[5] = str[5]  + 5;
	}

	str[3] -= pSmart->MTP.R_OFFSET.OFFSET_15;
	str[4] -= pSmart->MTP.G_OFFSET.OFFSET_15;
	str[5] -= pSmart->MTP.B_OFFSET.OFFSET_15;

	str[6] -= pSmart->MTP.R_OFFSET.OFFSET_35;
	str[7] -= pSmart->MTP.G_OFFSET.OFFSET_35;
	str[8] -= pSmart->MTP.B_OFFSET.OFFSET_35;

	str[9] -= pSmart->MTP.R_OFFSET.OFFSET_59;
	str[10] -= pSmart->MTP.G_OFFSET.OFFSET_59;
	str[11] -= pSmart->MTP.B_OFFSET.OFFSET_59;

	str[12] -= pSmart->MTP.R_OFFSET.OFFSET_87;
	str[13] -= pSmart->MTP.G_OFFSET.OFFSET_87;
	str[14] -= pSmart->MTP.B_OFFSET.OFFSET_87;

	str[15] -= pSmart->MTP.R_OFFSET.OFFSET_171;
	str[16] -= pSmart->MTP.G_OFFSET.OFFSET_171;
	str[17] -= pSmart->MTP.B_OFFSET.OFFSET_171;

	level_255_temp = (str[18] << 8) | str[19] ;
	level_255_temp -=  pSmart->MTP.R_OFFSET.OFFSET_255_LSB;
	level_255_temp_MSB = level_255_temp / 256;
	str[18] = level_255_temp_MSB & 0xff;
	str[19] = level_255_temp & 0xff;

	level_255_temp = (str[20] << 8) | str[21] ;
	level_255_temp -=  pSmart->MTP.G_OFFSET.OFFSET_255_LSB;
	level_255_temp_MSB = level_255_temp / 256;
	str[20] = level_255_temp_MSB & 0xff;
	str[21] = level_255_temp & 0xff;

	level_255_temp = (str[22] << 8) | str[23] ;
	level_255_temp -=  pSmart->MTP.B_OFFSET.OFFSET_255_LSB;
	level_255_temp_MSB = level_255_temp / 256;
	str[22] = level_255_temp_MSB & 0xff;
	str[23] = level_255_temp & 0xff;

}
#endif
#endif

static void pure_gamma_init(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[S6E8AA0X01_TABLE_MAX] = {-1, };
	int bl_index[S6E8AA0X01_TABLE_MAX] = {-1, };

	long long temp_cal_data = 0;
	int bl_level, cnt;
	int level_255_temp = 0;
	int level_255_temp_MSB = 0;
	int point_index;

	bl_level = pSmart->brightness_level;

	for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++) {
			point_index = S6E8AA0X01_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p2[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
	}

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s candela_1:%llu candela_15:%llu  candela_35:%llu  "
		"candela_59:%llu  candela_171:%llu  candela_171:%llu "
		"candela_255:%llu\n", __func__, candela_level[0],
		candela_level[1], candela_level[2], candela_level[3],
		candela_level[4], candela_level[5], candela_level[6]);
#endif

	/*calculate brightness level*/
	for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++) {
			if (searching_function(candela_level[cnt],
				&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
				pr_info("%s searching functioin error cnt:%d\n",
					__func__, cnt);
			}
	}

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s bl_index_1:%d bl_index_15:%d bl_index_35:%d bl_index_59:%d "
		"bl_index_87:%d bl_index_171:%d bl_index_255:%d\n", __func__,
		bl_index[0], bl_index[1], bl_index[2], bl_index[3],
		bl_index[4], bl_index[5], bl_index[6]);
#endif

	/*Generate Gamma table*/
	for (cnt = 0; cnt < S6E8AA0X01_TABLE_MAX; cnt++)
		(void)Make_hexa[cnt](bl_index , pSmart, str);

	/*subtration MTP_OFFSET value from generated gamma table*/
	str[3] -= pSmart->MTP.R_OFFSET.OFFSET_15;
	str[4] -= pSmart->MTP.G_OFFSET.OFFSET_15;
	str[5] -= pSmart->MTP.B_OFFSET.OFFSET_15;

	str[6] -= pSmart->MTP.R_OFFSET.OFFSET_35;
	str[7] -= pSmart->MTP.G_OFFSET.OFFSET_35;
	str[8] -= pSmart->MTP.B_OFFSET.OFFSET_35;

	str[9] -= pSmart->MTP.R_OFFSET.OFFSET_59;
	str[10] -= pSmart->MTP.G_OFFSET.OFFSET_59;
	str[11] -= pSmart->MTP.B_OFFSET.OFFSET_59;

	str[12] -= pSmart->MTP.R_OFFSET.OFFSET_87;
	str[13] -= pSmart->MTP.G_OFFSET.OFFSET_87;
	str[14] -= pSmart->MTP.B_OFFSET.OFFSET_87;

	str[15] -= pSmart->MTP.R_OFFSET.OFFSET_171;
	str[16] -= pSmart->MTP.G_OFFSET.OFFSET_171;
	str[17] -= pSmart->MTP.B_OFFSET.OFFSET_171;

	level_255_temp = (str[18] << 8) | str[19] ;
	level_255_temp -=  pSmart->MTP.R_OFFSET.OFFSET_255_LSB;
	level_255_temp_MSB = level_255_temp / 256;
	str[18] = level_255_temp_MSB & 0xff;
	str[19] = level_255_temp & 0xff;

	level_255_temp = (str[20] << 8) | str[21] ;
	level_255_temp -=  pSmart->MTP.G_OFFSET.OFFSET_255_LSB;
	level_255_temp_MSB = level_255_temp / 256;
	str[20] = level_255_temp_MSB & 0xff;
	str[21] = level_255_temp & 0xff;

	level_255_temp = (str[22] << 8) | str[23] ;
	level_255_temp -=  pSmart->MTP.B_OFFSET.OFFSET_255_LSB;
	level_255_temp_MSB = level_255_temp / 256;
	str[22] = level_255_temp_MSB & 0xff;
	str[23] = level_255_temp & 0xff;

}

static void set_max_lux_table(void)
{
	max_lux_table[0] = V1_300CD_R;
	max_lux_table[1] = V1_300CD_G;
	max_lux_table[2] = V1_300CD_B;

	max_lux_table[3] = V15_300CD_R;
	max_lux_table[4] = V15_300CD_G;
	max_lux_table[5] = V15_300CD_B;

	max_lux_table[6] = V35_300CD_R;
	max_lux_table[7] = V35_300CD_G;
	max_lux_table[8] = V35_300CD_B;

	max_lux_table[9] = V59_300CD_R;
	max_lux_table[10] = V59_300CD_G;
	max_lux_table[11] = V59_300CD_B;

	max_lux_table[12] = V87_300CD_R;
	max_lux_table[13] = V87_300CD_G;
	max_lux_table[14] = V87_300CD_B;

	max_lux_table[15] = V171_300CD_R;
	max_lux_table[16] = V171_300CD_G;
	max_lux_table[17] = V171_300CD_B;

	max_lux_table[18] = V255_300CD_R_MSB;
	max_lux_table[19] = V255_300CD_R_LSB;

	max_lux_table[20] = V255_300CD_G_MSB;
	max_lux_table[21] = V255_300CD_G_LSB;

	max_lux_table[22] = V255_300CD_B_MSB;
	max_lux_table[23] = V255_300CD_B_LSB;

}

static void set_min_lux_table(struct SMART_DIM *psmart)
{
	psmart->brightness_level = MIN_CANDELA;
	pure_gamma_init(psmart, min_lux_table, GAMMA_SET_MAX);
}

void get_min_lux_table(char *str, int size)
{
	memcpy(str, min_lux_table, size);
}

void generate_gamma(struct SMART_DIM *psmart, char *str, int size)
{
	int lux_loop;
	struct illuminance_table *ptable = (struct illuminance_table *)
						(&(psmart->gen_table));

	/* searching already generated gamma table */
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		if (ptable[lux_loop].lux == psmart->brightness_level) {
			memcpy(str, &(ptable[lux_loop].gamma_setting), size);
			break;
		}
	}

	/* searching fail... Setting 300CD value on gamma table */
	if (lux_loop == psmart->lux_table_max) {
		pr_info("%s searching fail lux : %d\n", __func__,
				psmart->brightness_level);
		memcpy(str, max_lux_table, size);
	}

#ifdef SMART_DIMMING_DEBUG
	if (lux_loop != psmart->lux_table_max)
		pr_info("%s searching ok index : %d lux : %d", __func__,
			lux_loop, ptable[lux_loop].lux);
#endif
}

static void gamma_cell_determine(int ldi_revision)
{
	pr_info("%s ldi_revision:%d", __func__, ldi_revision);

#if defined(CONFIG_MACH_STRETTO) || defined(CONFIG_MACH_SUPERIORLTE_SKT)
	if (ldi_revision == 0xAE) {
		V1_300CD_R = V1_300CD_R_AE;
		V1_300CD_G = V1_300CD_G_AE;
		V1_300CD_B = V1_300CD_B_AE;

		V15_300CD_R = V15_300CD_R_AE;
		V15_300CD_G = V15_300CD_G_AE;
		V15_300CD_B = V15_300CD_B_AE;

		V35_300CD_R = V35_300CD_R_AE;
		V35_300CD_G = V35_300CD_G_AE;
		V35_300CD_B = V35_300CD_B_AE;

		V59_300CD_R = V59_300CD_R_AE;
		V59_300CD_G = V59_300CD_G_AE;
		V59_300CD_B = V59_300CD_B_AE;

		V87_300CD_R = V87_300CD_R_AE;
		V87_300CD_G = V87_300CD_G_AE;
		V87_300CD_B = V87_300CD_B_AE;

		V171_300CD_R = V171_300CD_R_AE;
		V171_300CD_G = V171_300CD_G_AE;
		V171_300CD_B = V171_300CD_B_AE;

		V255_300CD_R_MSB = V255_300CD_R_MSB_AE;
		V255_300CD_R_LSB = V255_300CD_R_LSB_AE;

		V255_300CD_G_MSB = V255_300CD_G_MSB_AE;
		V255_300CD_G_LSB = V255_300CD_G_LSB_AE;

		V255_300CD_B_MSB = V255_300CD_B_MSB_AE;
		V255_300CD_B_LSB = V255_300CD_B_LSB_AE;

		return;
	}
#endif
	if (ldi_revision == 0x60) {
		V1_300CD_R = V1_300CD_R_60;
		V1_300CD_G = V1_300CD_G_60;
		V1_300CD_B = V1_300CD_B_60;

		V15_300CD_R = V15_300CD_R_60;
		V15_300CD_G = V15_300CD_G_60;
		V15_300CD_B = V15_300CD_B_60;

		V35_300CD_R = V35_300CD_R_60;
		V35_300CD_G = V35_300CD_G_60;
		V35_300CD_B = V35_300CD_B_60;

		V59_300CD_R = V59_300CD_R_60;
		V59_300CD_G = V59_300CD_G_60;
		V59_300CD_B = V59_300CD_B_60;

		V87_300CD_R = V87_300CD_R_60;
		V87_300CD_G = V87_300CD_G_60;
		V87_300CD_B = V87_300CD_B_60;

		V171_300CD_R = V171_300CD_R_60;
		V171_300CD_G = V171_300CD_G_60;
		V171_300CD_B = V171_300CD_B_60;

		V255_300CD_R_MSB = V255_300CD_R_MSB_60;
		V255_300CD_R_LSB = V255_300CD_R_LSB_60;

		V255_300CD_G_MSB = V255_300CD_G_MSB_60;
		V255_300CD_G_LSB = V255_300CD_G_LSB_60;

		V255_300CD_B_MSB = V255_300CD_B_MSB_60;
		V255_300CD_B_LSB = V255_300CD_B_LSB_60;

	} else if (ldi_revision == 0x40) {
		V1_300CD_R = V1_300CD_R_40;
		V1_300CD_G = V1_300CD_G_40;
		V1_300CD_B = V1_300CD_B_40;

		V15_300CD_R = V15_300CD_R_40;
		V15_300CD_G = V15_300CD_G_40;
		V15_300CD_B = V15_300CD_B_40;

		V35_300CD_R = V35_300CD_R_40;
		V35_300CD_G = V35_300CD_G_40;
		V35_300CD_B = V35_300CD_B_40;

		V59_300CD_R = V59_300CD_R_40;
		V59_300CD_G = V59_300CD_G_40;
		V59_300CD_B = V59_300CD_B_40;

		V87_300CD_R = V87_300CD_R_40;
		V87_300CD_G = V87_300CD_G_40;
		V87_300CD_B = V87_300CD_B_40;

		V171_300CD_R = V171_300CD_R_40;
		V171_300CD_G = V171_300CD_G_40;
		V171_300CD_B = V171_300CD_B_40;

		V255_300CD_R_MSB = V255_300CD_R_MSB_40;
		V255_300CD_R_LSB = V255_300CD_R_LSB_40;

		V255_300CD_G_MSB = V255_300CD_G_MSB_40;
		V255_300CD_G_LSB = V255_300CD_G_LSB_40;

		V255_300CD_B_MSB = V255_300CD_B_MSB_40;
		V255_300CD_B_LSB = V255_300CD_B_LSB_40;

	} else {
		V1_300CD_R = V1_300CD_R_20;
		V1_300CD_G = V1_300CD_G_20;
		V1_300CD_B = V1_300CD_B_20;

		V15_300CD_R = V15_300CD_R_20;
		V15_300CD_G = V15_300CD_G_20;
		V15_300CD_B = V15_300CD_B_20;

		V35_300CD_R = V35_300CD_R_20;
		V35_300CD_G = V35_300CD_G_20;
		V35_300CD_B = V35_300CD_B_20;

		V59_300CD_R = V59_300CD_R_20;
		V59_300CD_G = V59_300CD_G_20;
		V59_300CD_B = V59_300CD_B_20;

		V87_300CD_R = V87_300CD_R_20;
		V87_300CD_G = V87_300CD_G_20;
		V87_300CD_B = V87_300CD_B_20;

		V171_300CD_R = V171_300CD_R_20;
		V171_300CD_G = V171_300CD_G_20;
		V171_300CD_B = V171_300CD_B_20;

		V255_300CD_R_MSB = V255_300CD_R_MSB_20;
		V255_300CD_R_LSB = V255_300CD_R_LSB_20;

		V255_300CD_G_MSB = V255_300CD_G_MSB_20;
		V255_300CD_G_LSB = V255_300CD_G_LSB_20;

		V255_300CD_B_MSB = V255_300CD_B_MSB_20;
		V255_300CD_B_LSB = V255_300CD_B_LSB_20;
	}
}

int smart_dimming_init(struct SMART_DIM *psmart)
{
	int lux_loop;
#ifdef SMART_DIMMING_DEBUG
	int cnt;
	char pBuffer[256];
	memset(pBuffer, 0x00, 256);
#endif
	gpsmart = psmart;

	gamma_cell_determine(psmart->ldi_revision);
	set_max_lux_table();

	v255_adjustment(psmart);
	v1_adjustment(psmart);
	v171_adjustment(psmart);
	v87_adjustment(psmart);
	v59_adjustment(psmart);
	v35_adjustment(psmart);
	v15_adjustment(psmart);

	if (generate_gray_scale(psmart)) {
		pr_info(KERN_ERR "lcd smart dimming fail generate_gray_scale\n");
		return -1;
	}

	/*Generating lux_table*/
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		/* To set brightness value */
		psmart->brightness_level = psmart->plux_table[lux_loop];
		/* To make lux table index*/
		psmart->gen_table[lux_loop].lux = psmart->plux_table[lux_loop];

#ifdef AID_OPERATION_4_8_INCH
		gamma_init(psmart,
			(char *)(&(psmart->gen_table[lux_loop].gamma_setting)),
			GAMMA_SET_MAX);
#else
		pure_gamma_init(psmart,
			(char *)(&(psmart->gen_table[lux_loop].gamma_setting)),
			GAMMA_SET_MAX);
#endif
	}

	/* set 300CD max gamma table */
	memcpy(&(psmart->gen_table[lux_loop-1].gamma_setting),
			max_lux_table, GAMMA_SET_MAX);

	set_min_lux_table(psmart);

#ifdef SMART_DIMMING_DEBUG
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
			snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %d",
				psmart->gen_table[lux_loop].gamma_setting[cnt]);

		pr_info("lux : %d  %s", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}

	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
			snprintf(pBuffer + strnlen(pBuffer, 256), 256,
				" 0x%02x",
				psmart->gen_table[lux_loop].gamma_setting[cnt]);

		pr_info("lux : %d  %s", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
#endif

	return 0;
}

