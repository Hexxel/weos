#ifndef __LIBWESTERMO_H
#define __LIBWESTERMO_H

#include <mv88e6xxx.h>

#define SC0_SMI_ADDR 4
#define SC1_SMI_ADDR 1
#define SC2_SMI_ADDR 2
#define SC3_SMI_ADDR 3

int wmo_backplane_setup(struct mv88e6xxx_pdata *backplane);

void set_product_id_from_idmem(void);
u32 get_product_id(void);
u8 get_raw_value_from_idmem(int pos);

int product_is_coronet_star(void);
int product_is_coronet_cascade(void);
int product_is_coronet_tbn(void);

#endif	/* __LIBWESTERMO_H */
