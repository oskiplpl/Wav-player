#include "stm32f10x.h"
#include "FatFs/ff.h"
#include "FatFs/diskio.h"
#include "OM_UART/uart.h"
#include "OM_SPI/SPI.h"
#include "LCD/lcd44780.h"
#include "wav_format/wav.h"
#include "menu/menu.h"
#include "player/player.h"
#include "player/functionHandler.h"
#include "delay/delay.h"
#include "button_handler/buttons.h"

void timer2_init(void);

uint16_t timer2;

int main(void)
{
	SysTick_Config(SystemCoreClock / 100);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	buttons_init();
	lcd_init();

	spi_init();
	uart_init();

	timer2_init();

	while(1)
	{
		disk_initialize(0);
		disk_status(0);

		if(!disk_status(0)) break;

		DisplayMenu(NO_SD_ERR, player, wav);

		while(1)
		{
			btnState = menuButton();

			if(btnState == PRESSED)
				break;
		}
	}

	fresult = f_mount(&g_sFatFs, "", 0);

	spi_1_init();

	readPlayerSettings(&player);

	DisplayMenu(START, player, wav);

	PlayFirstSong(&wav);
	DisplayMenu(DEFAULT, player, wav);

	while (1) {

		//NEXT BTN
		btnState = nextButton();
		if(btnState == HOLDING && timer2 > 3)
		{
			audioPlayPause(1);
			forward();
			timer2 = 0;
		}
		else if(btnState == RELEASED)
		{
			nextSong();
		}
		else if(btnState == HOLDING_RELEASED)
			audioPlayPause(0);

		//PREVIOUS BTN
		btnState = previousButton();
		if(btnState == HOLDING && timer2 > 3)
		{
			audioPlayPause(1);
			backward();
			timer2 = 0;
		}
		else if(btnState == RELEASED)
		{
			previousSong();
		}
		else if(btnState == HOLDING_RELEASED)
			audioPlayPause(0);


		//PLUS BTN
		btnState = plusButton();
		if(btnState == PRESSED)
			volumeUp();
		else if(btnState == HOLDING && timer2 > 15)
		{
			volumeUp();
			timer2 = 0;
		}
		else if(btnState == HOLDING_RELEASED || btnState == RELEASED)
			savePlayerSettings();

		//MINUS BTN
		btnState = minusButton();
		if(btnState == PRESSED)
			volumeDown();
		else if(btnState == HOLDING && timer2 > 15)
		{
			volumeDown();
			timer2 = 0;
		}
		else if(btnState == HOLDING_RELEASED || btnState == RELEASED)
			savePlayerSettings();

		//MENU BTN
		btnState = menuButton();
		if(btnState == PRESSED)
			menuSelect();

		//PAUSE BTN
		btnState = pauseButton();
		if(btnState == PRESSED)
			toggleBtnPause();


		if(ReadWavData() == STARTED_NEW_SONG)
			DisplayMenu(DEFAULT, player, wav);

		DisplayTime(wav);

		if(timer > 300 && MenuState != DEFAULT)
		{
			DisplayMenu(DEFAULT, player, wav);
		}
	}
}


void TIM2_IRQHandler()
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

		disk_timerproc();
	}
}

void SysTick_Handler()
{
	functionTimer();
	timer2++;
}

void timer2_init(void)
{
	TIM_TimeBaseInitTypeDef tim;
	NVIC_InitTypeDef nvic;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseStructInit(&tim);
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Prescaler = 720;
	tim.TIM_Period = 1000 - 1;
	TIM_TimeBaseInit(TIM2, &tim);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, ENABLE);

	nvic.NVIC_IRQChannel = TIM2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
}

