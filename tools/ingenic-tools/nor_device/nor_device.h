#ifndef  __NOR_DEVICE_H
#define  __NOR_DEVICE_H


#define CMD_INFO(COMMAND, DUMMY_BIT, ADDR_LEN, TRANSFER_MODE) {	\
	.cmd = COMMAND,			\
	.dummy_byte = DUMMY_BIT,	\
	.addr_nbyte = ADDR_LEN,		\
	.transfer_mode = TRANSFER_MODE,	\
}

#define ST_INFO(COMMAND, BIT_SHIFT, MASK, VAL, LEN, DUMMY_BIT) { \
	.cmd = COMMAND,		\
	.bit_shift = BIT_SHIFT,	\
	.mask = MASK,		\
	.val = VAL,		\
	.len = LEN,		\
	.dummy = DUMMY_BIT,	\
}

#endif
