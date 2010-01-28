OUTPUT_FORMAT("elf32-bigarm")
OUTPUT_ARCH(arm)
EXTERN(_start)
ENTRY(_start)

__stack_size = 2048;

MEMORY {

	code		: ORIGIN = 0x20000000,	LENGTH = 0x7350
	ram			: ORIGIN = 0x20009000,	LENGTH = 0x469EC
	stk			: ORIGIN = 0x2004f1c4,	LENGTH = 2048
	table		: ORIGIN = 0x100,		LENGTH = 16K
}

PHDRS
{
	phdrs PT_PHDR PHDRS AT ( 0x0 ) FLAGS ( 0 );

	table PT_NOTE FLAGS ( 0 );
	table PT_LOAD PHDRS AT ( 0x0 ) FLAGS ( 0 );

	code PT_LOAD AT ( 0x13A10000 ) ;
	ram PT_LOAD AT ( 0x13A19000 ) ;
	stk PT_LOAD AT ( 0x13A5F1C4 ) ;
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
		. = ALIGN(4);
		__stack_addr = .;
	} >stk :stk

}

