#ifdef CONFIG_MACH_DAGGER
#define PRE_IMAGE \
        .pre_image : {                                  \
                KEEP(*(.flash_jmp_barebox*))             \
                KEEP(*(.flash_header*))             \
                . = ALIGN(256);                         \
                flash_header_end = .;               \
        }
#endif
