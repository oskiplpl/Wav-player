#include "spi.h"

void SPIx_Init(void)
{	 
	 SPI_InitTypeDef spi;
	 GPIO_InitTypeDef gpio;

	 RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	 GPIO_StructInit(&gpio);
	 gpio.GPIO_Pin = GPIO_Pin_15|GPIO_Pin_13; // SCK, MOSI
	 gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	 gpio.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOB, &gpio);

	 gpio.GPIO_Pin = GPIO_Pin_14; // MISO
	 gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	 GPIO_Init(GPIOB, &gpio);

	 gpio.GPIO_Pin = GPIO_Pin_12; // CS
	 gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	 GPIO_Init(GPIOB, &gpio);

	 SPI_StructInit(&spi);
	 spi.SPI_Mode = SPI_Mode_Master;
	 spi.SPI_NSS = SPI_NSS_Soft;
	 spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	 SPI_Init(SPI2, &spi);

	 SPI_Cmd(SPI2, ENABLE);
}   


u8 SPIx_ReadWriteByte(u8 TxData)
{		
	 // poczekaj az bufor nadawczy bedzie wolny
	 while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	 SPI_I2S_SendData(SPI2, TxData);

	 // poczekaj na dane w buforze odbiorczym
	 while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
	 return SPI_I2S_ReceiveData(SPI2);
}































