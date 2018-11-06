#include <common.h>
#include <command.h>
#include <getopt.h>

#include <linux/phy.h>

#define M88E6352 0x3520
#define M88E6097 0x0990

#define ADDRESS_CHIP_VERSION 0x10
#define REGISTER_CHIP_VERSION 0x3

#define ADDRESS_COUNTER_OPS 0x1b
#define REGISTER_OP_CONTROL 0x1d
#define REGISTER_FOR_BYTE_3_2 0x1e
#define REGISTER_FOR_BYTE_1_0 0x1f

#define OP_CAPTURE_COUNTERS 0xd400
#define OP_READ_COUNTERS 0xc400
#define OP_6352_PORT_SHIFT 0x5
#define RGMII_PORT_ADDRESS 0x15
#define PORT_STATUS_REGISTER 0x00
#define HISTOGRAM_MODE 0xc00

#define OP_COMMAND_DONE 0x8000
#define TRY_COUNT 0x5

enum counters
{
   OP_READ_IN_UNICAST = 0x4,
   OP_READ_IN_BROADCAST = 0x6,
   OP_READ_IN_MULTICAST = 0x7,
   OP_READ_OUT_UNICAST = 0x10,
   OP_READ_OUT_BROADCAST = 0x13,
   OP_READ_OUT_MULTICAST = 0x12,
   MAX = 0x4242
};

static int wait_for_op(struct mii_bus *bus)
{
   int value = -1;
   int return_value = COMMAND_ERROR;
   int i;

   for (i = 0; i < TRY_COUNT; i++)
   {
      value = bus->read(bus, ADDRESS_COUNTER_OPS, REGISTER_OP_CONTROL);
      if ((value & OP_COMMAND_DONE) == 0x0)
      {
         return_value = COMMAND_SUCCESS;
         break;
      }
   }

   return return_value;
}

static int do_read_op(int product_num, struct mii_bus *bus, unsigned short port)
{
   int return_value = COMMAND_SUCCESS;
   int high_value = -1;
   int low_value = -1;
   int op_cmd = OP_CAPTURE_COUNTERS | port;
   enum counters i;
   char* label;

   /* dbg commented out : printf("%s : entering with op_cmd = %x and port = %x\n", __func__, op_cmd, port); */

   if (product_num == M88E6390)
      op_cmd &= ~HISTOGRAM_MODE;

   bus->write(bus, ADDRESS_COUNTER_OPS, REGISTER_OP_CONTROL, op_cmd);
   if (wait_for_op(bus) == COMMAND_SUCCESS)
   {
      for (i = OP_READ_IN_UNICAST; i < MAX; i++)
      {
         switch (i)
         {
            case OP_READ_IN_UNICAST:
               label = "in unicast";
               break;
            case OP_READ_IN_BROADCAST:
               label = "in broadcast";
               break;
            case OP_READ_IN_MULTICAST:
               label = "in multicast";
               break;
            case OP_READ_OUT_UNICAST:
               label = "out unicast";
               break;
            case OP_READ_OUT_BROADCAST:
               label = "out broadcast";
               break;
            case OP_READ_OUT_MULTICAST:
               label = "out multicast";
               break;
            default:
               continue;
         }
         op_cmd = OP_READ_COUNTERS | i;
	 if (product_num == M88E6390)
            op_cmd &= ~HISTOGRAM_MODE;

         /* dbg commented out : printf("%s : reading with op_cmd = %x\n", __func__, op_cmd); */
         bus->write(bus, ADDRESS_COUNTER_OPS, REGISTER_OP_CONTROL, op_cmd);
         if (wait_for_op(bus) == COMMAND_SUCCESS)
         {
            high_value = bus->read(bus, ADDRESS_COUNTER_OPS, REGISTER_FOR_BYTE_3_2);
            low_value = bus->read(bus, ADDRESS_COUNTER_OPS, REGISTER_FOR_BYTE_1_0);
            printf("%s package count : %d", label, ((high_value << 16) | low_value));
         }
         else
         {
            return_value = COMMAND_ERROR;
            printf("%s : failed to read %x", __func__, i);
            i = MAX;
            break;
         }
         printf("\n");
      }

      /* Needed to still have link after read status */
      (void)bus->read(bus, RGMII_PORT_ADDRESS, PORT_STATUS_REGISTER);
   }
   return return_value;
}

static int do_rmon_op(struct mii_bus *bus, int port)
{
   int return_value = COMMAND_ERROR;
   int reg_value = -1;
   int product_num = -1;
   unsigned short port_num = -1;

   /* Find out what chip we ask questions to */
   /* Supported chip right now is 6097 */
   reg_value = bus->read(bus, ADDRESS_CHIP_VERSION, REGISTER_CHIP_VERSION);

   /* Only care about product number right now, not revision */
   product_num = reg_value & 0xfff0;

   switch (product_num)
   {
      case M88E6097:
         printf("We have found a 6097, which is supported.\n");
         port_num = (unsigned short)port;
         return_value = do_read_op(product_num, bus, port_num);
         break;
      case M88E6352:
         printf("We have found a 6352, which is supported.\n");
         port_num = (unsigned short)((port + 1) << OP_6352_PORT_SHIFT);
         return_value = do_read_op(product_num, bus, port_num);
         break;
      case M88E6390:
         printf("We have found a 6390, which is supported.\n");
         port_num = (unsigned short)((port + 1) << OP_6352_PORT_SHIFT);
         return_value = do_read_op(product_num, bus, port_num);
         break;
      case 0xffff:
         printf("Error when ask for chip version :(\n");
         break;
      default:
         printf("Chip type not supported ...\n");
         break;
   }

   return return_value;
}

static int do_rmon(int argc, char *argv[])
{
   struct mii_bus *bus;
   char *bus_name = NULL;
   int port = -1;
   int return_value = COMMAND_ERROR_USAGE;

   if (argc > 2)
   {
      bus_name = argv[1];
      port = simple_strtol(argv[2], NULL, 0);

      for_each_mii_bus(bus) {
         if (!bus_name || !strcmp(bus_name, dev_name(&bus->dev)) ||
             !strcmp(bus_name, dev_name(bus->parent)))
         {
            return_value = do_rmon_op(bus, port);
            break;
         }
      }
   }

   return return_value;
}

BAREBOX_CMD_START(rmon)
	.cmd      = do_rmon,
	BAREBOX_CMD_DESC("read traffic stats for port on a bus (device).")
	BAREBOX_CMD_OPTS("<bus> <port>")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
BAREBOX_CMD_END
