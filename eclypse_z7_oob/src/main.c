/*
 * main.c
 *
 *  Created on: Sep 26, 2019
 *      Author: arthur
 */

#include "xil_printf.h"
#include "PWM.h"
#include "xgpio.h"
#include "xparameters.h"
#include "sleep.h"

#define PWM_LED0_BLUE_INDEX  0
#define PWM_LED0_GREEN_INDEX 1
#define PWM_LED0_RED_INDEX   2
#define PWM_LED1_BLUE_INDEX  3
#define PWM_LED1_GREEN_INDEX 4
#define PWM_LED1_RED_INDEX   5

void PWM_SetDuties(u32 PwmBaseAddr, u32 intensity, u32 colors) {
	for (u32 index=0; index<6; index++) {
		if (((colors >> index) & 0x1) == 0x1) {
			if (intensity >= 128) {
				PWM_Set_Duty(PwmBaseAddr, 255 - intensity, index);
			} else {
				PWM_Set_Duty(PwmBaseAddr, intensity, index);
			}
		} else {
			PWM_Set_Duty(PwmBaseAddr, 0, index);
		}
	}
}

int main (void) {
	XGpio buttons;
	XGpio_Config *buttons_config_ptr;
	const u32 PwmBaseAddr = XPAR_PWM_RGBLED_0_PWM_AXI_BASEADDR;
	u32 button_data, last_button_data = 0;

	buttons_config_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_BTN_0_DEVICE_ID);
	XGpio_CfgInitialize(&buttons, buttons_config_ptr, buttons_config_ptr->BaseAddress);

	u32 intensity = 0, colors = 0b001001, us_counter = 0, mask;
	PWM_Set_Period(PwmBaseAddr, 256);
	PWM_SetDuties(PwmBaseAddr, intensity, colors);
	PWM_Enable(PwmBaseAddr);

	while (1) {
		button_data = XGpio_DiscreteRead(&buttons, 1) & 0x3;
		if ((button_data & 0x1) == 0x1 && (last_button_data & 0x1) == 0x0) {
			xil_printf("Button 1 pressed!\r\n");
		}
		if ((button_data & 0x2) == 0x2 && (last_button_data & 0x2) == 0x0) {
			xil_printf("Button 2 pressed!\r\n");
		}
		last_button_data = button_data;

		if (us_counter >= 3406) {
			us_counter = 0;
			if (intensity >= 255) {
				intensity = 0;
				colors = ((colors >> 5) | (colors << 1)) & 0b111111;
//				if (colors >= 0b111111) {
//					colors = 0;
//				} else {
//					colors++;
//				}
			} else {
				intensity++;
			}
			mask = 0b111111;
			if ((button_data & 0x1) == 0x1) {
				mask &= 0b111000;
			}
			if ((button_data & 0x2) == 0x2) {
				mask &= 0b000111;
			}
			PWM_SetDuties(PwmBaseAddr, intensity, colors & mask);
		} else {
			us_counter++;
		}


		usleep(1);
	}
}
