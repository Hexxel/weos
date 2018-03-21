#include "drg-spi.h"
	
.global _start
_start:
	b	reset
1:	b	1b
1:	b	1b
1:	b	1b
1:	b	1b
1:	b	1b
1:	b	1b
1:	b	1b

reset:	
	ldr	sp, =(DTCM_BASE + DTCM_SIZE)

	bl	main
