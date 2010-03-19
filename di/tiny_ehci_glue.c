/*   
	EHCI glue. A bit hacky for the moment. needs cleaning..

    Copyright (C) 2008 kwiirk.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "tiny_ehci_glue.h"

#define static
#define inline extern


void BUG(void)
{
        debug_printf("bug\n");
//        stack_trace();
        while(1);
}
#define BUG_ON(a) if(a)BUG()
/*void udelay(int usec)
{
        u32 tmr;
        tmr = get_timer();
        tmr+=2*usec;
        while(get_timer() <= tmr);
}*/
void msleep(int msec)//@todo not really sleeping..
{
        u32 tmr;
        tmr = get_timer();
        tmr+=2048*msec;
        while(get_timer() <= tmr);

}
extern u32 __exe_start_virt__;
extern u32 __ram_start_virt__;

extern u32 ios_thread_stack;

void print_hex_dump_bytes(char *header,int prefix,u8 *buf,int len)
{
        int i;
        if (len>0x100)len=0x100;
        debug_printf("%s  %08X\n",header,(u32)buf);
        for (i=0;i<len;i++){
                debug_printf("%02x ",buf[i]);
                if((i&0xf) == 0xf) 
                        debug_printf("\n");
        }
        debug_printf("\n");
                
}
#define DUMP_PREFIX_OFFSET 1
#include "ehci.h"
#define ehci_readl(a) ((*((volatile u32*)(a))))
//#define ehci_writel(e,v,a) do{msleep(40);debug_printf("writel %08X %08X\n",a,v);*((volatile u32*)(a))=(v);}while(0)
#define ehci_writel(v,a) do{*((volatile u32*)(a))=(v);}while(0)

struct ehci_hcd _ehci;
struct ehci_hcd *ehci = &_ehci;

#include "ehci.c"

int usb_os_init(void);
int tiny_ehci_init(void)
{
	int retval;
	ehci = &_ehci;

	//memset(ehci,0,sizeof(*ehci));
	if(usb_os_init()<0)
			return 0;

	ehci->caps = (void*)0x0D040000;
	ehci->regs = (void*)(0x0D040000 + HC_LENGTH(ehci_readl(&ehci->caps->hc_capbase)));

	ehci->num_port = 4;
	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = ehci_readl( &ehci->caps->hcs_params );

	/* data structure init */
	retval = ehci_init();
	if (retval)
		return retval; 

	ehci_release_ports(); //quickly release none usb2 port

	return 0;
}
