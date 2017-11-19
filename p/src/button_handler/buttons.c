/*
 * buttons.c
 *
 *  Created on: 18.11.2017
 *      Author: OSKI
 */
#include "buttons.h"
#include "stm32f10x.h"
#include "../OM_UART/uart.h"

BUTTON_STATE plusButton(void)
{
	BUTTON_STATE result = NONE;
	static uint16_t key_lock;
	static uint32_t holdOn;

	if( !key_lock && !GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) )
	{
		key_lock=50000;

		result = PRESSED;
		holdOn = 1;
	}
	else if( key_lock && GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) )
	{
		key_lock++;
	}
	else if(holdOn && !GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10))
	{
		if(holdOn > 100000)
		{
			result = HOLDING;
		}
		else
			holdOn++;
	}
	else if( holdOn && holdOn <= 100000 && GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) )
	{
		result = RELEASED;
		holdOn = 0;
	}
	return result;
}


