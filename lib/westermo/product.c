#define pr_fmt(fmt) "libwestermo: " fmt

#include <common.h>
#include <globalvar.h>
#include <fcntl.h>
#include <fs.h>
#include <init.h>
#include <net.h>
#include <of.h>

#include <generated/utsrelease.h>

/*
 * IMPORTANT
 * The values below are based on the information in the file 
 * "westermo/include/idmem/id_mem_data_defines.h" located in the 
 * weos git repository. Make sure to validate those values against
 * the ones in that file if any changes are to be made.
 */
#define CORONET_VIPER_212_MIN 0x4000
#define CORONET_VIPER_212_MAX 0x47FF
#define CORONET_VIPER_220_MIN 0x4800
#define CORONET_VIPER_220_MAX 0x4BFF
#define CORONET_VIPER_TBN_MIN 0x4C00
#define CORONET_VIPER_TBN_MAX 0x4FFF

static char idmem[128];

#if defined(CONFIG_CORAZON) || defined(CONFIG_CORONET)

#define PRODUCT_FMT "%.4x"

static u32 wmo_product(void)
{
   return (idmem[116] << 8) | idmem[117];
}

#elif defined(CONFIG_MACH_BASIS)

#define PRODUCT_FMT "%.6x"

static u32 wmo_product(void)
{
   return (idmem[2] << 16 | idmem[3] << 8 | idmem[5]);
}

#else

#error "Not a westermo product!"

#endif

static void set_product(int product)
{
   char str[10];
   snprintf (str, sizeof(str), PRODUCT_FMT, product);

   globalvar_add_simple("wmo.product", str);
}

static int read_raw_idmem(void)
{
   int fd = 0;
   int return_value = -1;
   static int alreadyRead = 0;

   if (alreadyRead == 0)
   {
      fd = open(CONFIG_WMO_IDMEM, O_RDONLY);
      if ((fd < 0) || (read(fd, idmem, sizeof(idmem)) != sizeof(idmem)))
      {
         pr_err("unable to read idmem\n");
         memset(idmem, 0, 128);
      }
      else
      {
         alreadyRead = 1;
         return_value = 0;
      }

      if (fd > 0)
      {
         close(fd);
      }
   }
   return return_value;
}

void set_product_id_from_idmem(void)
{
   u32 product;

   read_raw_idmem();
   product = wmo_product();
   set_product(product);
}

u32 get_product_id(void)
{
   u32 product;

   read_raw_idmem();
   product = wmo_product();

   return product;
}

u8 get_raw_value_from_idmem(int pos)
{
   u8 return_value = 0;

   read_raw_idmem();
   if ((pos >= 0) && (pos < 128))
   {
      return_value = idmem[pos];
   }

   return return_value;
}

int product_is_coronet_star(void)
{
   int return_value = false;
   u32 product = get_product_id();

   if ((product >= CORONET_VIPER_220_MIN) && (product <= CORONET_VIPER_220_MAX))
   {
      return_value = true;
   }

   return return_value;
}

int product_is_coronet_cascade(void)
{
   int return_value = false;
   u32 product = get_product_id();

   if ((product >= CORONET_VIPER_212_MIN) && (product <= CORONET_VIPER_212_MAX))
   {
      return_value = true;
   }

   return return_value;
}

int product_is_coronet_tbn(void)
{
   int return_value = false;
   u32 product = get_product_id();

   if ((product >= CORONET_VIPER_TBN_MIN) && (product <= CORONET_VIPER_TBN_MAX))
   {
      return_value = true;
   }

   return return_value;
}

