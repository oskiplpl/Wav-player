#include "stm32f10x.h"
#include "FatFs/ff.h"
#include "FatFs/diskio.h"
#include "OM_UART/uart.h"
#include "OM_SPI/SPI.h"
#include "LCD/lcd44780.h"
#include "wav_format/wav.h"
#include "menu/menu.h"
#include "player/player.h"
#include "delay/delay.h"
#include "button_handler/buttons.h"


void timer2_init(void);
void forward(void);

WAV_FORMAT wav;

int16_t timer;
uint16_t timer2;

int main(void)
{
	SysTick_Config(SystemCoreClock / 100);

	GPIO_InitTypeDef gpio;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	GPIO_StructInit(&gpio);

	gpio.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_8;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_10;
	gpio.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &gpio);

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
		while((uart_getChar() != 'm'));
	}

	fresult = f_mount(&g_sFatFs, "", 0);

	spi_1_init();

	readPlayerSettings(&player);

	//DisplayMenu(START, player, wav);

	PlayFirstSong(&wav);
	DisplayMenu(DEFAULT, player, wav);

	char uartRx = 0;
	BUTTON_STATE btnState;

	while (1) {

		btnState = plusButton();

		if(btnState == HOLDING && timer2 > 3)
		{
			player.audio_pause = 1;
			forward();
			timer2 = 0;
		}
		else if(btnState == RELEASED)
		{
			PlayNextSong(&wav);
			DisplayMenu(DEFAULT, player, wav);
		}
		else if(btnState == NONE)
			player.audio_pause = 0;

		if((uartRx = uart_getChar()))
		{
			uart_putInt((int)uartRx);

			switch(uartRx)
			{
				case 'a':
				{

					PlaySong("boh.wav", &wav);

					break;
				}
				case 'n':
				{

					PlayNextSong(&wav);
					DisplayMenu(DEFAULT, player, wav);
					break;
				}
				case 'q':
				{

					PlaySong("korn.wav", &wav);

					break;
				}
				case 'z':
				{

					PlaySong("don44.wav", &wav);
					break;
				}
				case 'x':
				{

					PlaySong("don48.wav", &wav);
					break;
				}
				case 'p':
				{
					player.audio_pause ^= 1;
					break;
				}
				case '+':
				{
					timer = 0;

					switch (MenuState) {
						case BASS:
						{
							player.lowPassVolume += 1;
							if(player.lowPassVolume > 10) player.lowPassVolume = 10;
							uart_putInt(player.lowPassVolume);
							DisplayMenu(BASS, player, wav);
							break;
						}
						case TREBLE:
						{
							player.highPassVolume += 1;
							if(player.highPassVolume > 10) player.highPassVolume = 10;
							DisplayMenu(TREBLE, player, wav);
							break;
						}
						default:
						{
							player.volume += 1;
							if(player.volume > 9)
								player.volume = 9;
							DisplayMenu(VOLUME, player, wav);
							break;
						}
					}
					break;
				}
				case '-':
				{
					timer = 0;
					switch (MenuState) {
						case BASS:
						{
							player.lowPassVolume -= 1;
							if(player.lowPassVolume < -10) player.lowPassVolume = -10;
							uart_putInt(player.lowPassVolume);
							DisplayMenu(BASS, player, wav);
							break;
						}
						case TREBLE:
						{
							player.highPassVolume -= 1;
							if(player.highPassVolume < -10) player.highPassVolume = -10;
							DisplayMenu(TREBLE, player, wav);
							break;
						}
						default:
						{
							player.volume -= 1;
							if(player.volume < 0)
								player.volume = 0;
							DisplayMenu(VOLUME, player, wav);
							break;
						}
					}
					break;
				}
//				case 'f':
//				{
//					if(player.isPlaying)
//					{
//						player.audio_pause = 1;
//						f_lseek(&plik, f_tell(&plik) + currentSong.bitRate * 4);
//						currentSong.currentSec++;
//						if(currentSong.currentSec >= 60)
//						{
//							currentSong.currentMin++;
//							currentSong.currentSec = 0;
//						}
//						player.audio_pause = 0;
//					}
//					break;
//				}
//				case 'g':
//				{
//					if(player.isPlaying)
//					{
//						player.audio_pause = 1;
//						f_lseek(&plik, f_tell(&plik) - currentSong.bitRate * 4);
//						if(currentSong.currentSec == 0)
//						{
//							currentSong.currentMin--;
//							currentSong.currentSec = 60;
//						} else
//							currentSong.currentSec--;
//						player.audio_pause = 0;
//					}
//					break;
//				}
				case 'm':
				{
					timer = -20;
					if(MenuState == DEFAULT || MenuState == VOLUME)
						MenuState = BASS;
					else if(MenuState == BASS)
						MenuState = TREBLE;
					else if(MenuState == TREBLE)
						MenuState = DEFAULT;

					DisplayMenu(MenuState, player, wav);
					break;
				}
				case 27:
				{
					DisplayMenu(DEFAULT, player, wav);
					break;
				}
			}
		}

		if(ReadWavData() == STARTED_NEW_SONG)
			DisplayMenu(DEFAULT, player, wav);

		DisplayTime(wav);

		if(timer > 300 && MenuState != DEFAULT)
		{
			DisplayMenu(DEFAULT, player, wav);
		}
	}
}

void forward(void)
{
	if(player.isPlaying)
	{
		f_lseek(&plik, f_tell(&plik) + wav.bitRate * 4);
		wav.currentSec++;
		if(wav.currentSec >= 60)
		{
			wav.currentMin++;
			wav.currentSec = 0;
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
	timer++;
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


