#include "syscalls.h"
#include <string.h>
#include "ehci_types.h"
#include "usb.h"
#include "ehci.h"
#include "alloc.h"
//static  int heap;
int usb_os_init(void)
{
	//heap = heap_create((void*)0x13892000, 0x4000);
	//if(heap<0)
	//{
	//	debug_printf("\n\nunable to create heap :( %d\n\n",heap);
	//}
	return 0;
}
//static u8* aligned_mem = 0;
//static u8* aligned_base = 0;
/* @todo hum.. not that nice.. */
void*ehci_maligned(int size,int alignement,int crossing)
{
	u32 addr = malloca( size, alignement );
        //if (!aligned_mem )
        //{
        //        aligned_mem=aligned_base =  (void*)0x13890000;
        //}
        //u32 addr=(u32)aligned_mem;
        //alignement--;
        //addr += alignement;
        //addr &= ~alignement;
        //if (((addr +size-1)& ~(crossing-1)) != (addr&~(crossing-1)))
        //        addr = (addr +size-1)&~(crossing-1);
        //aligned_mem = (void*)(addr + size);
        //if (aligned_mem>aligned_base + 0x3000)
        //{
        //        debug_printf("not enough aligned memory!\n");
        //}
        //memset((void*)addr,0,size);
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
        //debug_printf("sync_after_write %p %x\n",buf,len);
 
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

