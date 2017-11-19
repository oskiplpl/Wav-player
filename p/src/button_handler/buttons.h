/*
 * keys.h
 *
 *  Created on: 18.11.2017
 *      Author: OSKI
 */

#ifndef BUTTON_HANDLER_BUTTONS_H_
#define BUTTON_HANDLER_BUTTONS_H_

typedef enum
{
	PRESSED,
	RELEASED,
	HOLDING,
	NONE
}BUTTON_STATE;

BUTTON_STATE plusButton(void);

#endif /* BUTTON_HANDLER_BUTTONS_H_ */
