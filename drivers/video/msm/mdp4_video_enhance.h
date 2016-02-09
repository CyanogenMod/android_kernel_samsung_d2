/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
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
#ifndef _MDP4_VIDEO_ENHANCE_H_
#define _MDP4_VIDEO_ENHANCE_H_

//#define BROWSER_COLOR_TONE_SET

#define SIG_MDNIE_UI_MODE				0
#define SIG_MDNIE_VIDEO_MODE			1
/*#define SIG_MDNIE_VIDEO_WARM_MODE	2*/
/*#define SIG_MDNIE_VIDEO_COLD_MODE	3*/
#define SIG_MDNIE_CAMERA_MODE		4
#define SIG_MDNIE_NAVI					5
#define SIG_MDNIE_GALLERY				6
#define SIG_MDNIE_VT					7
#define SIG_MDNIE_BYPASS				8

#define SIG_MDNIE_DYNAMIC				0
#define SIG_MDNIE_STANDARD				1
#define SIG_MDNIE_MOVIE				2

/*#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)*/
#define SIG_MDNIE_DMB_MODE			20
/*#define SIG_MDNIE_DMB_WARM_MODE	21
#define SIG_MDNIE_DMB_COLD_MODE	22
#endif*/

#ifdef CONFIG_TARGET_LOCALE_NTT
#define SIG_MDNIE_ISDBT_MODE		30
#define SIG_MDNIE_ISDBT_WARM_MODE	31
#define SIG_MDNIE_ISDBT_COLD_MODE	32
#endif

#ifdef BROWSER_COLOR_TONE_SET
#define SIG_MDNIE_BROWSER_TONE1	40
#define SIG_MDNIE_BROWSER_TONE2	41
#define SIG_MDNIE_BROWSER_TONE3	42
#endif
void init_mdnie_class(void);
void lut_tune(int num, u8 *pLutTable);
void sharpness_tune(int num);
int is_negativeMode_on(void);
int s3c_mdnie_start(void);
int s3c_mdnie_off(void);
void reenable_mdnie(void);

#endif /*_MDP4_VIDEO_ENHANCE_H_*/
