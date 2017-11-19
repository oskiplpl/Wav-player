#include "stm32f10x.h"
#include "../OM_UART/uart.h"



void spi_1_init(void);
uint8_t spi_1_sendrecv(uint16_t byte);
void spi1_resetCS(void);
void spi1_setCS(void);
void spi_send (uint8_t byte);
uint8_t spi_recv(void);
void RCC_Configuration(void);
