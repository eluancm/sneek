OUTPUT_FORMAT("elf32-bigarm")
OUTPUT_ARCH(arm)
EXTERN(_start)
ENTRY(_start)

__stack_size = 0x8000;

MEMORY {

	code		: ORIGIN = 0x20200000,	LENGTH = 0x7F00
	ram			: ORIGIN = 0x20208000,	LENGTH = 0x140
	stk			: ORIGIN = 0x20209000,	LENGTH = 0x2BDC4
}

PHDRS
{
	code PT_LOAD AT ( 0x139B0000 ) ;
	ram  PT_LOAD AT ( 0x139B8000 ) ;
	stk  PT_LOAD AT ( 0x139B9000 ) ;
}

SECTIONS
{

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
	} >stk :stk

	.data :
	{
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d*)
		. = ALIGN(4);
	} >stk :stk

	.bss :
	{
		__bss_start = . ;
		*(.dynbss)
		*(.gnu.linkonce.b*)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = . ;
	} >stk :stk

	.stack :
	{
		__stack_end = .;
		. = . +__stack_size;
		. = ALIGN(4);
		__stack_addr = .;
	} >stk :stk

	/DISCARD/ :
	{
		*(.ARM.exidx*)
		*(.ARM.extab*)
	}
}

