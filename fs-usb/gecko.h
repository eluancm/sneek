/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	USBGecko support code

Copyright (c) 2008		Nuke - <wiinuke@gmail.com>
Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __GECKO_H__
#define __GECKO_H__

#include "global.h"
#include "hollywood.h"

void gecko_init(void);
u8 gecko_enable_console(const u8 enable);

#ifdef NDEBUG
#define gecko_printf(...) do { } while(0)
#else
int gecko_printf(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
#endif

void GeckoInit( void );
void GeckoSendBuffer( char *buffer );

void gecko_timer_initialize(void);
void gecko_timer(void);

#endif

