#ifndef _TEGLUE_
#define _TEGLUE_

#include "string.h"
#include "syscalls.h"

#include "ehci_types.h"
#include "utils.h"

int tiny_ehci_init(void);

#define readl(a) (*((volatile u32*)(a)))
#define writel(v,a) do{*((volatile u32*)(a))=(v);}while(0)
#define ehci_dbg(a...) debug_printf(a)
#define printk(a...) debug_printf(a)
#define get_timer()  (*(((volatile u32*)0x0D800010)))

#define cpu_to_le32(a) swab32(a)
#define le32_to_cpu(a) swab32(a)
#define cpu_to_le16(a) swab16(a)
#define le16_to_cpu(a) swab16(a)
#define cpu_to_be32(a) (a)
#define be32_to_cpu(a) (a)

#endif