//-----------------------------------------------------------------------------------------------------------
// *** Obs�uga wy�wietlaczy alfanumerycznych zgodnych z HD44780 ***
//
// - Sterowanie: tryb 4-bitowy
// - Dowolne przypisanie ka�dego sygna�u steruj�cego do dowolnego pinu mikrokontrolera
// - Praca z pinem RW pod��czonym do GND lub do mikrokontrolera (sprawdzanie BusyFLAG - szybkie operacje LCD)
//
// Pliki 			: lcd44780.c , lcd44780.h
// Mikrokontrolery 	: Atmel AVR
// Kompilator 		: avr-gcc
// �r�d�o 			: http://www.atnel.pl
// Data 			: marzec 2010
// Autor 			: Miros�aw Karda�
//----------------------------------------------------------------------------------------------------------
#include "stm32f10x.h"
#include <stdlib.h>

#include "lcd44780.h"
#include "../OM_UART/uart.h"
#include "../delay/delay.h"

// makrodefinicje operacji na sygna�ach steruj�cych RS,RW oraz E





uint8_t check_BF(void);			// deklaracja funkcji wewn�trznej



//********************* FUNKCJE WEWN�TRZNE *********************

//----------------------------------------------------------------------------------------
//
//		 Ustawienie wszystkich 4 linii danych jako WYj�cia
//
//----------------------------------------------------------------------------------------
static inline void data_dir_out(void)
{
	GPIOB->CRL &= 0x00F00FFF;

	LCD_D4PORT->CRL |= (0b0011 << 12);
	LCD_D5PORT->CRL |= (0b0011 << 16);
	LCD_D6PORT->CRL |= (0b0011 << 24);
	LCD_D7PORT->CRL |= (0b0011 << 28);
}

//----------------------------------------------------------------------------------------
//
//		 Ustawienie wszystkich 4 linii danych jako WEj�cia
//
//----------------------------------------------------------------------------------------
static inline void data_dir_in(void)
{
	GPIOB->CRL &= 0x00F00FFF;

	LCD_D4PORT->CRL |= (0b0100 << 12);
	LCD_D5PORT->CRL |= (0b0100 << 16);
	LCD_D6PORT->CRL |= (0b0100 << 24);
	LCD_D7PORT->CRL |= (0b0100 << 28);
}

//----------------------------------------------------------------------------------------
//
//		 Wys�anie po��wki bajtu do LCD (D4..D7)
//
//----------------------------------------------------------------------------------------
static inline void lcd_sendHalf(uint8_t data)
{
	if (data&(1<<0)) LCD_D4PORT->BSRR |= LCD_D4; else LCD_D4PORT->BRR |= LCD_D4;
	if (data&(1<<1)) LCD_D5PORT->BSRR |= LCD_D5; else LCD_D5PORT->BRR |= LCD_D5;
	if (data&(1<<2)) LCD_D6PORT->BSRR |= LCD_D6; else LCD_D6PORT->BRR |= LCD_D6;
	if (data&(1<<3)) LCD_D7PORT->BSRR |= LCD_D7; else LCD_D7PORT->BRR |= LCD_D7;
}

#if USE_RW == 1
//----------------------------------------------------------------------------------------
//
//		 Odczyt po��wki bajtu z LCD (D4..D7)
//
//----------------------------------------------------------------------------------------
static inline uint8_t lcd_readHalf(void)
{
	uint8_t result=0;
//
	if(LCD_D4PORT->IDR & LCD_D4) result |= (1<<0);
	if(LCD_D5PORT->IDR & LCD_D5) result |= (1<<1);
	if(LCD_D6PORT->IDR & LCD_D6) result |= (1<<2);
	if(LCD_D7PORT->IDR & LCD_D7) result |= (1<<3);

	return result;
}


#endif

//----------------------------------------------------------------------------------------
//
//		 Zapis bajtu do wy�wietlacza LCD
//
//----------------------------------------------------------------------------------------
void _lcd_write_byte(unsigned char _data)
{
	// Ustawienie pin�w portu LCD D4..D7 jako wyj�cia
	data_dir_out();

#if USE_RW == 1
	CLR_RW;
#endif

	SET_E;
	lcd_sendHalf(_data >> 4);			// wys�anie starszej cz�ci bajtu danych D7..D4
	CLR_E;
	delay_us(10);
	SET_E;
	lcd_sendHalf(_data);				// wys�anie m�odszej cz�ci bajtu danych D3..D0
	CLR_E;

#if USE_RW == 1
	while( (check_BF() & (1<<7)) );
#else
	_delay_us(120);
#endif

}

#if USE_RW == 1
//----------------------------------------------------------------------------------------
//
//		 Odczyt bajtu z wy�wietlacza LCD
//
//----------------------------------------------------------------------------------------
uint8_t _lcd_read_byte(void)
{
	uint8_t result=0;
	data_dir_in();

	SET_RW;

	SET_E;
	result = (lcd_readHalf() << 4);	// odczyt starszej cz�ci bajtu z LCD D7..D4
	CLR_E;

	SET_E;
	result |= lcd_readHalf();			// odczyt m�odszej cz�ci bajtu z LCD D3..D0
	CLR_E;

	return result;
}
#endif


#if USE_RW == 1
//----------------------------------------------------------------------------------------
//
//		 Sprawdzenie stanu Busy Flag (Zaj�to�ci wy�wietlacza)
//
//----------------------------------------------------------------------------------------
uint8_t check_BF(void)
{
	CLR_RS;
	return _lcd_read_byte();
}
#endif


//----------------------------------------------------------------------------------------
//
//		 Zapis komendy do wy�wietlacza LCD
//
//----------------------------------------------------------------------------------------
void lcd_write_cmd(uint8_t cmd)
{
	CLR_RS;
	_lcd_write_byte(cmd);
}

//----------------------------------------------------------------------------------------
//
//		 Zapis danych do wy�wietlacza LCD
//
//----------------------------------------------------------------------------------------
void lcd_write_data(uint8_t data)
{
	SET_RS;
	_lcd_write_byte(data);
}




//**************************  FUNKCJE PRZEZNACZONE TAK�E DLA INNYCH MODU��W  ******************

#if USE_LCD_CHAR == 1
//----------------------------------------------------------------------------------------
//
//		 Wys�anie pojedynczego znaku do wy�wietlacza LCD w postaci argumentu
//
//		 8 w�asnych znak�w zdefiniowanych w CGRAM
//		 wysy�amy za pomoc� kod�w 0x80 do 0x87 zamiast 0x00 do 0x07
//
//----------------------------------------------------------------------------------------
void lcd_char(char c)
{
	lcd_write_data( ( c>=0x80 && c<=0x87 ) ? (c & 0x07) : c);
}
#endif

//----------------------------------------------------------------------------------------
//
//		 Wys�anie stringa do wy�wietlacza LCD z pami�ci RAM
//
//		 8 w�asnych znak�w zdefiniowanych w CGRAM
//		 wysy�amy za pomoc� kod�w 0x80 do 0x87 zamiast 0x00 do 0x07
//
//----------------------------------------------------------------------------------------
void lcd_str(char * str)
{
	register char znak;
	while ( (znak=*(str++)) )
		lcd_write_data( ( znak>=0x80 && znak<=0x87 ) ? (znak & 0x07) : znak);
}


#if USE_LCD_INT == 1
//----------------------------------------------------------------------------------------
//
//		 Wy�wietla liczb� dziesi�tn� na wy�wietlaczu LCD
//
//----------------------------------------------------------------------------------------
void lcd_int(int val)
{
	char bufor[17];
	lcd_str( itoa(val, bufor, 10) );
}
#endif

#if USE_LCD_HEX == 1
//----------------------------------------------------------------------------------------
//
//		 Wy�wietla liczb� szestnastkow� HEX na wy�wietlaczu LCD
//
//----------------------------------------------------------------------------------------
void lcd_hex(int val)
{
	char bufor[17];
	lcd_str( itoa(val, bufor, 16) );
}
#endif

#if USE_LCD_DEFCHAR == 1
//----------------------------------------------------------------------------------------
//
//		Definicja w�asnego znaku na LCD z pami�ci RAM
//
//		argumenty:
//		nr: 		- kod znaku w pami�ci CGRAM od 0x80 do 0x87
//		*def_znak:	- wska�nik do tablicy 7 bajt�w definiuj�cych znak
//
//----------------------------------------------------------------------------------------
void lcd_defchar(uint8_t nr, uint8_t *def_znak)
{
	register uint8_t i,c;
	lcd_write_cmd( 64+((nr&0x07)*8) );
	for(i=0;i<8;i++)
	{
		c = *(def_znak++);
		lcd_write_data(c);
	}
}
#endif



#if USE_LCD_LOCATE == 1
//----------------------------------------------------------------------------------------
//
//		Ustawienie kursora w pozycji Y-wiersz, X-kolumna
//
// 		Y = od 0 do 3
// 		X = od 0 do n
//
//		funkcja dostosowuje automatycznie adresy DDRAM
//		w zale�no�ci od rodzaju wy�wietlacza (ile posiada wierszy)
//
//----------------------------------------------------------------------------------------
void lcd_locate(uint8_t y, uint8_t x)
{
	switch(y)
	{
		case 0: y = LCD_LINE1; break;

#if (LCD_Y>1)
	    case 1: y = LCD_LINE2; break; // adres 1 znaku 2 wiersza
#endif
#if (LCD_Y>2)
    	case 2: y = LCD_LINE3; break; // adres 1 znaku 3 wiersza
#endif
#if (LCD_Y>3)
    	case 3: y = LCD_LINE4; break; // adres 1 znaku 4 wiersza
#endif
	}

	lcd_write_cmd( (0x80 + y + x) );
}
#endif


//----------------------------------------------------------------------------------------
//
//		Kasowanie ekranu wy�wietlacza
//
//----------------------------------------------------------------------------------------
void lcd_cls(void)
{
	lcd_write_cmd( LCDC_CLS );

	#if USE_RW == 0
		_delay_ms(4.9);
	#endif
}


#if USE_LCD_CURSOR_HOME == 1
//----------------------------------------------------------------------------------------
//
//		Powr�t kursora na pocz�tek
//
//----------------------------------------------------------------------------------------
void lcd_home(void)
{
	lcd_write_cmd( LCDC_CLS|LCDC_HOME );

	#if USE_RW == 0
		_delay_ms(4.9);
	#endif
}
#endif

#if USE_LCD_CURSOR_ON == 1
//----------------------------------------------------------------------------------------
//
//		W��czenie kursora na LCD
//
//----------------------------------------------------------------------------------------
void lcd_cursor_on(void)
{
	lcd_write_cmd( LCDC_ONOFF|LCDC_DISPLAYON|LCDC_CURSORON);
}

//----------------------------------------------------------------------------------------
//
//		Wy��czenie kursora na LCD
//
//----------------------------------------------------------------------------------------
void lcd_cursor_off(void)
{
	lcd_write_cmd( LCDC_ONOFF|LCDC_DISPLAYON);
}
#endif


#if USE_LCD_CURSOR_BLINK == 1
//----------------------------------------------------------------------------------------
//
//		W��cza miganie kursora na LCD
//
//----------------------------------------------------------------------------------------
void lcd_blink_on(void)
{
	lcd_write_cmd( LCDC_ONOFF|LCDC_DISPLAYON|LCDC_CURSORON|LCDC_BLINKON);
}

//----------------------------------------------------------------------------------------
//
//		WY��cza miganie kursora na LCD
//
//----------------------------------------------------------------------------------------
void lcd_blink_off(void)
{
	lcd_write_cmd( LCDC_ONOFF|LCDC_DISPLAYON);
}
#endif


//----------------------------------------------------------------------------------------
//
//		 ******* INICJALIZACJA WY�WIETLACZA LCD ********
//
//----------------------------------------------------------------------------------------
void lcd_init(void)
{


	// inicjowanie pin�w port�w ustalonych do pod��czenia z wy�wietlaczem LCD
	// ustawienie wszystkich jako wyj�cia
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

	GPIO_InitTypeDef gpio;

	data_dir_out();
	lcd_sendHalf(0);

	GPIO_StructInit(&gpio);

	gpio.GPIO_Pin = LCD_RS;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LCD_RSPORT, &gpio);

	gpio.GPIO_Pin = LCD_E;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LCD_EPORT, &gpio);

	#if USE_RW == 1
		gpio.GPIO_Pin = LCD_RW;
		gpio.GPIO_Mode = GPIO_Mode_Out_PP;
		gpio.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(LCD_RWPORT, &gpio);
	#endif

	CLR_E;
	CLR_RS;
	CLR_RW;

	delay_ms(41);

	// jeszcze nie mo�na u�ywa� Busy Flag
	SET_E;
	lcd_sendHalf(0x03);	// tryb 8-bitowy
	CLR_E;
	delay_ms(5);

	SET_E;
	lcd_sendHalf(0x03);	// tryb 8-bitowy
	CLR_E;
	delay_ms(5);

	SET_E;
	lcd_sendHalf(0x03);	// tryb 8-bitowy
	CLR_E;
	delay_ms(5);

	SET_E;
	lcd_sendHalf(0x02); // tryb 4-bitowy
	CLR_E;
	delay_ms(5);

	// ju� mo�na u�ywa� Busy Flag
	// tryb 4-bitowy, 2 wiersze, znak 5x7
	lcd_write_cmd( LCDC_FUNC|LCDC_FUNC4B|LCDC_FUNC2L|LCDC_FUNC5x7 );
	// wy��czenie kursora
	lcd_write_cmd( LCDC_ONOFF|LCDC_CURSOROFF );
	// w��czenie wy�wietlacza
	lcd_write_cmd( LCDC_ONOFF|LCDC_DISPLAYON );
	// przesuwanie kursora w prawo bez przesuwania zawarto�ci ekranu
	lcd_write_cmd( LCDC_ENTRY|LCDC_ENTRYR );

	// kasowanie ekranu
	lcd_cls();
}


