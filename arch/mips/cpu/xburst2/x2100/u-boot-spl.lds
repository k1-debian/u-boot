MEMORY { .sram : ORIGIN = CONFIG_SPL_TEXT_BASE,\
		LENGTH = CONFIG_SPL_MAX_SIZE }

OUTPUT_ARCH(mips)
ENTRY(_start)
SECTIONS
{
	.text      :
	{
		__start = .;
		*(.start_section*)
		*(.text*)
	} >.sram

	. = ALIGN(4);
	.rodata : { *(SORT_BY_ALIGNMENT(.rodata*)) } >.sram

	. = ALIGN(4);
	.data : { *(SORT_BY_ALIGNMENT(.data*)) } >.sram

	. = ALIGN(4);
	__image_copy_end = .;

	.bss : {
		. = ALIGN(4);
		__bss_start = .;
		*(.sbss.*)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	} >.sram

	/DISCARD/ : {
		*(.dynbss)
		*(.dynstr)
		*(.dynamic)
		*(.interp)
		*(.hash)
		*(.gnu.*)
		*(.plt)
		*(.got.plt)
		*(.rel.plt)
		*(.rel.dyn)
	}
}

#if !defined(CONFIG_NOR_BUILTIN_PARAMS) && !defined(CONFIG_NAND_BUILTIN_PARAMS) && defined(CONFIG_SPIFLASH_PART_OFFSET)
ASSERT(__image_copy_end - __start < (CONFIG_SPIFLASH_PART_OFFSET), \
        "SPL image too big");
#endif
