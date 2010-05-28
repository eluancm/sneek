#include "syscalls.h"
#include <string.h>
#include "ehci_types.h"
#include "usb.h"
#include "ehci.h"
#include "alloc.h"
int usb_os_init(void)
{
	return 0;
}
//static u8* aligned_mem = 0;
//static u8* aligned_base = 0;
/* @todo hum.. not that nice.. */
void*ehci_maligned(int size,int alignement,int crossing)
{
	u32 addr = (u32)malloca( size, alignement );
    memset((void*)addr,0,size);
    return (void*)addr;
}
dma_addr_t ehci_virt_to_dma(void *a)
{

        return (dma_addr_t)virt_to_phys(a);
}
dma_addr_t ehci_dma_map_to(void *buf,size_t len)
{
        sync_after_write(buf, len);
        return (dma_addr_t)virt_to_phys(buf);

}
dma_addr_t ehci_dma_map_from(void *buf,size_t len)
{
        sync_after_write(buf, len);
        return (dma_addr_t)virt_to_phys(buf);
}
dma_addr_t ehci_dma_map_bidir(void *buf,size_t len)
{
        //dbgprintf("sync_after_write %p %x\n",buf,len);
 
        sync_after_write(buf, len);
        return (dma_addr_t)virt_to_phys(buf); 
}
void ehci_dma_unmap_to(dma_addr_t buf,size_t len)
{
        sync_before_read((void*)buf, len);
}
void ehci_dma_unmap_from(dma_addr_t buf,size_t len)
{
        sync_before_read((void*)buf, len);
}
void ehci_dma_unmap_bidir(dma_addr_t buf,size_t len)
{
        sync_before_read((void*)buf, len);
}


void *USB_Alloc(int size)
{
        return malloc(size);
}
void USB_Free(void *ptr)
{
        return free(ptr);
}

