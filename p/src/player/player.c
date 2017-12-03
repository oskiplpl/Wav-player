/*
 * player.c
 *
 *  Created on: 18.11.2017
 *      Author: OSKI
 */
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "../OM_UART/uart.h"
#include "../delay/delay.h"

void timer3_init(void);
void timer3_init_48(void);

uint8_t spiBuf[BUFFER_SIZE];
uint8_t spiBuf2[BUFFER_SIZE];
uint8_t * spiReadBufWsk;
WAV_FORMAT *thisWav;


void PlayerInit(void)
{

}

void PlayFirstSong(WAV_FORMAT *wav)
{
	thisWav = wav;
    const char *patern = "*.wav";
    const char *x = "";

    fresult = f_findfirst(&dir, &fileinfo, x, patern);

    if(!fresult)
    {
    	PlaySong(fileinfo.fname, wav);
    }
}

void PlayNextSong(WAV_FORMAT *wav)
{
	thisWav = wav;
    fresult = f_findnext(&dir, &fileinfo);

    if(!fresult)
    {
    	PlaySong(fileinfo.fname, wav);
    }

    if(fileinfo.fname[0] == 0)
    {
    	PlayFirstSong(wav);
    }
}

void PlayPreviousSong(WAV_FORMAT *wav)
{
	thisWav = wav;
    const char *patern = "*.wav";
    const char *x = "";

    char tempTitle[30];

    uint16_t counter = 0;

    while(1)
    {
    	counter++;

        if( f_findnext(&dir, &fileinfo) ) return;

        if(fileinfo.fname[0] == 0)
        {
            if( f_findfirst(&dir, &fileinfo, x, patern)) return;
        }

        if( strcmp(wav->title, fileinfo.fname) == 0)
        	break;

    	strcpy(tempTitle, fileinfo.fname);
    }

    for(uint16_t i = 0; i < counter - 1; i++)
    {
        f_findnext(&dir, &fileinfo);

        if(fileinfo.fname[0] == 0)
        	f_findfirst(&dir, &fileinfo, x, patern);
    }

    PlaySong(tempTitle, wav);
}

void StopPlaying(void)
{
	player.isPlaying = 0;
	fresult = f_close(&plik);
}

void PlaySong(char* fileName, WAV_FORMAT *wav)
{
	thisWav = wav;

	player.isPlaying = 0;

	fresult = f_close(&plik);
	fresult = f_open(&plik, fileName, FA_READ);
	if(fresult) return;

	*wav = GetWavFormat(&plik);

	f_stat(fileName, &fileinfo);

	strcpy(wav->title, fileinfo.fname);

	if(wav->error) return;

	switch (wav->bitRate) {
		case 44100:
			timer3_init();
			break;
		case 48000:
			timer3_init_48();
			break;
		default:
			break;
	}

	player.isPlaying = 1;
}

PLAYER_STATE ReadWavData(void)
{
	PLAYER_STATE result = PLAYING;
	if(player.isPlaying)
	{
		if(player.canRead == 1)
		{
			f_read(&plik, spiReadBufWsk, BUFFER_SIZE, &read);
			if(read < BUFFER_SIZE)
			{
				StopPlaying();
				delay_ms(300);
				PlayNextSong(thisWav);
				result = STARTED_NEW_SONG;
			}
			player.canRead = 0;
		}
	}
	return result;
}

void TIM3_IRQHandler()
{
	static int16_t lowPass_outL_1, lowPass_outR_1;
	static int16_t highPass_outL_1, highPass_outR_1, inL_1, inR_1;
	static uint16_t index;
	static uint8_t * buf = spiBuf;
	static uint8_t previousBit;
	static uint8_t left_right;
	static uint32_t timeTicks;
	int16_t toSend = 0;
	int32_t tempSend = 0;
	int32_t lowPass = 0, highPass = 0;

	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

		if(!index)
		{
			if(buf == spiBuf)
			{
				buf = spiBuf2;
				spiReadBufWsk = spiBuf;
			}
			else if(buf == spiBuf2)
			{
				buf = spiBuf;
				spiReadBufWsk = spiBuf2;
			}
			player.canRead = 1;
		}

		if(!player.audio_pause && player.isPlaying)
		{
			left_right ^= 1;

			toSend |= buf[index+1];
			toSend <<= 8;
			toSend |= buf[index];


			tempSend = toSend * player.volume;
			toSend = (int16_t)(tempSend >> 5);

			index += 2;

			if(left_right)
			{
				lowPass = (2268*toSend + 30500*lowPass_outL_1) >> 15;
				lowPass_outL_1 = lowPass;
				lowPass >>= 3;

				highPass = (toSend - inL_1 + highPass_outL_1) >> 1;
				highPass_outL_1 = highPass;

				inL_1 = toSend;

				highPass >>= 3;

				tempSend = toSend + highPass*player.highPassVolume + lowPass*player.lowPassVolume;
				if(tempSend > 32766) tempSend = 32766;
				if(tempSend < -32766)  tempSend = -32766;
				toSend = (int16_t)tempSend;

				spi1_setCS();

			}
			else
			{
				lowPass = (2268*toSend + 30500*lowPass_outR_1) >> 15;
				lowPass_outR_1 = lowPass;
				lowPass >>= 3;

				highPass = (toSend - inR_1 + highPass_outR_1) >> 1;
				highPass_outR_1 = highPass;

				inR_1 = toSend;

				highPass >>= 3;


				tempSend = toSend + highPass*player.highPassVolume + lowPass*player.lowPassVolume;
				if(tempSend > 32766) tempSend = 32766;
				if(tempSend < -32766)  tempSend = -32766;
				toSend = (int16_t)tempSend;

				spi1_resetCS();
			}

			toSend >>= 1;
			if(previousBit)
				toSend |= 0x8000;
			previousBit = buf[index+1] & 0x01;

			timeTicks++;

			if(timeTicks >= thisWav->doubleBitRate)
			{
				timeTicks = 0;
				thisWav->currentSec++;
				if(thisWav->currentSec >= 60)
				{
					thisWav->currentSec = 0;
					thisWav->currentMin++;
				}
			}
		}

		if(index > BUFFER_SIZE-1)
			index = 0;

		spi_1_sendrecv(toSend);

	}
}

void timer3_init(void)
{
	TIM_TimeBaseInitTypeDef tim;
	NVIC_InitTypeDef nvic;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	TIM_TimeBaseStructInit(&tim);
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Prescaler = 68;
	tim.TIM_Period = 12 - 1;
	TIM_TimeBaseInit(TIM3, &tim);

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);

	nvic.NVIC_IRQChannel = TIM3_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
}

void timer3_init_48(void)
{
	TIM_TimeBaseInitTypeDef tim;
	NVIC_InitTypeDef nvic;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	TIM_TimeBaseStructInit(&tim);
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Prescaler = 75;
	tim.TIM_Period = 10 - 1;
	TIM_TimeBaseInit(TIM3, &tim);

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);

	nvic.NVIC_IRQChannel = TIM3_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
}

uint32_t startAddress = 0x0800fC00;

void savePlayerSettings(void)
{
	static uint32_t cursor = 0;

	if(cursor > 1000)
	{
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
		FLASH_ErasePage(startAddress);
		FLASH_Lock();
		cursor = 0;
	}

	FLASH_Unlock();
	FLASH_ProgramHalfWord((startAddress + cursor), player.volume);
	FLASH_ProgramHalfWord((startAddress + 2 + cursor), player.displayVolume);
	FLASH_ProgramHalfWord((startAddress + 4 + cursor), player.highPassVolume);
	FLASH_ProgramHalfWord((startAddress + 6 + cursor), player.lowPassVolume);
	FLASH_Lock();

	cursor += 8;
}

void readPlayerSettings(PLAYER *player)
{
	uint32_t cursor = 0;

	while(1)
	{
		player->volume = *(int8_t *)(startAddress + cursor);
		player->displayVolume = *(int8_t *)(startAddress + 2 + cursor);
		player->highPassVolume = *(int8_t *)(startAddress + 4 + cursor);
		player->lowPassVolume = *(int8_t *)(startAddress + 6 + cursor);

		if((player->volume == -1) && (player->highPassVolume == -1) && (player->lowPassVolume == -1))
			break;

		cursor += 8;
	}

	if(cursor) cursor -= 8;

	player->volume = *(int8_t *)(startAddress + cursor);
	player->displayVolume = *(int8_t *)(startAddress + 2 + cursor);
	player->highPassVolume = *(int8_t *)(startAddress + 4 + cursor);
	player->lowPassVolume = *(int8_t *)(startAddress + 6 + cursor);

	if(player->volume < 0 || player->volume > 32)
	{
		player->volume = 8;
		player->displayVolume = 8;
	}
	if(player->highPassVolume < -10 || player->highPassVolume > 10) player->highPassVolume = 0;
	if(player->lowPassVolume < -10 || player->lowPassVolume > 10) player->lowPassVolume = 0;


	//clear memory
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
	FLASH_ErasePage(startAddress);
	FLASH_Lock();

	savePlayerSettings();
}
