#include <common.h>
#include <generated/utsrelease.h>

#define __flash_header_section       __section(.flash_header)
#define __flash_jmp_section          __section(.flash_jmp_barebox)

struct bb_flash_header {
	unsigned char cookie[4];
	unsigned char header_version[4];
	unsigned char platform[16];
	unsigned char bb_version[40];
};

struct bb_flash_header __flash_header_section flash_header = {
	.cookie              = {"bbox"},
	.header_version      = {'1'},
	.platform            = {CONFIG_BBOX_PLATFORM},
	.bb_version          = {UTS_RELEASE}
};

void __naked start_dagger(void);

void __flash_jmp_section flash_jmp_barebox(void) {
	start_dagger();
}
