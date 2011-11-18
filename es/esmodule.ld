OUTPUT_FORMAT("elf32-bigarm")
OUTPUT_ARCH(arm)
EXTERN(_start)
ENTRY(_start)

__stack_size = 32;

MEMORY {

	code		: ORIGIN = 0x20100000,	LENGTH = 0xC200
	ram			: ORIGIN = 0x2010E000,	LENGTH = 0x46AC
	table		: ORIGIN = 0x100,		LENGTH = 16K
}

PHDRS
{
	phdrs PT_PHDR PHDRS AT ( 0x0 ) FLAGS ( 0 );

	table PT_NOTE FLAGS ( 0 );
	table PT_LOAD PHDRS AT ( 0x0 ) FLAGS ( 0 );

	code PT_LOAD AT ( 0x139F0000 ) ;
	ram PT_LOAD AT ( 0x139FE000 ) ;
}

SECTIONS
{
	.ios_info_table :
	{
		KEEP(*(.ios_info_table))
	} > table :table

	.init :
	{
		*(.init)
		. = ALIGN(4);
	} >code :code

	.text :
	{
		*(.text*)
		*(.text.*)
		*(.gnu.warning)
		*(.gnu.linkonce.t*)
		*(.glue_7)
		*(.glue_7t)
		. = ALIGN(4);
	} >code :code

	.rodata :
	{
		*(.rodata)
		*all.rodata*(*)
		*(.roda)
		*(.rodata.*)
		*(.gnu.linkonce.r*)
		. = ALIGN(4);
	} >code :code

	.data :
	{
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d*)
		. = ALIGN(4);
	} >code :code

	.bss :
	{
		__bss_start = . ;
		*(.dynbss)
		*(.gnu.linkonce.b*)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = . ;
	} >ram :ram

	.stack :
	{
		__stack_end = .;
		. = . +__stack_size;
		. = ALIGN(64);
		__stack_addr = .;
	} >code :code

}

