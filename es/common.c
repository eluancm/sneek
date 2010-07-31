#include "string.h"
#include "syscalls.h"
#include "global.h"

#define PANIC_ON	200000
#define PANIC_OFF	300000
#define PANIC_INTER	1000000

void PanicBlink(int Pattern, ...)
{
	int arg;
	va_list ap;
	
	*(vu32*)0xd8000E0 &= ~0x000020;
	*(vu32*)0xd8000E4 &= ~0x000020;
	*(vu32*)0xd8000FC &= ~0x000020;
	
	while(1)
	{
		va_start(ap, Pattern);
		
		while(1)
		{
			arg = va_arg(ap, int);

			if(arg < 0)
				break;
			
			*(vu32*)0xd8000E0 |= 0x000020;

			udelay(arg * PANIC_ON);

			*(vu32*)0xd8000E0 &= ~0x000020;

			udelay(PANIC_OFF);
		}
		
		va_end(ap);
		
		udelay(PANIC_INTER);
	}
}


void udelay(int us)
{
	u8 heap[0x10];
	u32 msg;
	s32 mqueue = -1;
	s32 timer = -1;

	mqueue = mqueue_create(heap, 1);
	if(mqueue < 0)
		goto out;
	timer = TimerCreate(us, 0, mqueue, 0xbabababa);
	if(timer < 0)
		goto out;
	mqueue_recv(mqueue, &msg, 0);
	
out:
	if(timer > 0)
		TimerDestroy(timer);
	if(mqueue > 0)
		mqueue_destroy(mqueue);
}
