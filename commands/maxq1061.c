/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 *
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <spi/spi.h>


#define MAX_BUFFER_SIZE 2300U
#define CMD_HEADER_SIZE 5U // 0xaa / cmd1 cmd2 / ll ll / .... / crc1 crc2
#define MAX_DATA_SIZE (MAX_BUFFER_SIZE-CMD_HEADER_SIZE)

// unsigned char cmdbuffer[MAX_BUFFER_SIZE];
// unsigned char *cmd_data = (cmdbuffer + CMD_HEADER_SIZE);


#define CRC16 0x8005

u16  outputval;


static u16 crc16(unsigned char *data, int size)
{
	int i, j;
	unsigned short output_bak;
	unsigned short crc;

	int bits_read = 0, bit_flag = 0;
	
	outputval = 0x0000;

	while (size > 0) {
		bit_flag = outputval >> 15;
		outputval <<= 1;
		outputval |= (*data >> bits_read) & 1;
		bits_read++;

		if (bits_read > 7) {
			bits_read = 0;
			data++;
			size--;
		}

		if (bit_flag)
			outputval ^= CRC16;
	}

	output_bak = outputval;

	for (i = 0; i < 16; ++i) {
		bit_flag = output_bak >> 15;
		output_bak <<= 1;

		if (bit_flag)
			output_bak ^= CRC16;
	}

	i = 0x8000;
	j = 0x0001;
	crc = 0;
	
	for (; i != 0; i >>=1, j <<= 1)
		if (i & output_bak) 
			crc |= j;

	return crc;
}


static int do_maxq1061(int argc, char *argv[])
{
	struct spi_device spi;
	int bus = 0;
	int read = 0;
	int verbose = 0;
	int opt, count, i, ret;
	int byte_per_word;
	u16 crc;
	u32 timeout = 0x10000;

	u8 *tx_buf, *rx_buf;

	memset(&spi, 0, sizeof(struct spi_device));

	spi.max_speed_hz = 1 * 1000 * 1000;
	spi.bits_per_word = 8;
	spi.mode = 0;
	spi.chip_select = 2;
	byte_per_word = max(spi.bits_per_word / 8, 1);
	
	while ((opt = getopt(argc, argv, "b:c:f:v")) > 0) {
		switch (opt) {
		case 'b':
			bus = simple_strtol(optarg, NULL, 0);
			break;
		case 'c':
			spi.chip_select = simple_strtoul(optarg, NULL, 0);
			break;
		case 'f':
			spi.max_speed_hz = simple_strtoul(optarg, NULL, 0);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	count = argc - optind;

 	if (!count || !spi.max_speed_hz || !spi.bits_per_word)
 		return COMMAND_ERROR_USAGE;

	spi.master = spi_get_master(bus);
	if (!spi.master) {
		printf("spi bus %d not found\n", bus);
		return -ENODEV;
	}

	if (spi.chip_select > spi.master->num_chipselect) {
		printf("spi chip select (%d)> master num chipselect (%d)\n",
			spi.chip_select, spi.master->num_chipselect);
		return -EINVAL;
	}

	ret = spi.master->setup(&spi);
	if (ret) {
		printf("can not setup the master (%d)\n", ret);
		return ret;
	}

	tx_buf = xmalloc(count + 3);
	rx_buf = xmalloc(MAX_BUFFER_SIZE);

	/* Fill in the message header */
	tx_buf[0] = 0xaa;
	for (i = 0; i < count; i++)
		tx_buf[i + 1] = (u8) simple_strtol(argv[optind + i], NULL, 16);
	
	crc = crc16(tx_buf, count + 1);
	tx_buf[i + 1] = crc;
	tx_buf[i + 2] = crc >> 8;
	count += 3;
	
	/* Send command message and receive respond header */
	ret = spi_write_then_read(&spi, tx_buf, count, rx_buf, 0/*count + 5*/);
    if (ret)
		goto out;

	/* Respond message */
	while (rx_buf[0] != 0x55) {
		ret = spi_write_then_read(&spi, tx_buf, 0, rx_buf, 1);
		if (ret)
			goto out;
		if (--timeout == 0)
			return -1;
	}	
	ret = spi_write_then_read(&spi, tx_buf, 0, rx_buf + 1, 4);
	if (ret)
		goto out;
	read = (((u16)(rx_buf[3]) << 8) | ((u16)(rx_buf[4]))) + 2;  // incl crc
	ret = spi_write_then_read(&spi, tx_buf, 0, rx_buf + 5, read);
	if (ret)
		goto out;
	read += 5;
	
	if (verbose) {
		printf("device config\n");
		printf("    bus_num       = %d\n", spi.master->bus_num);
		printf("    max_speed_hz  = %d\n", spi.max_speed_hz);
		printf("    chip_select   = %d\n", spi.chip_select);
		printf("    mode          = 0x%x\n", spi.mode);
		printf("    bits_per_word = %d\n", spi.bits_per_word);
		printf("\n");

		printf("wrote %i bytes\n", count);
		memory_display(tx_buf, 0, count, byte_per_word, 0);
		printf("\n");

		printf("read %i bytes\n", read);
		memory_display(rx_buf, 0, read, byte_per_word, 0);
		printf("\n");
		
		crc = crc16(rx_buf, read - 2);
		printf("received crc 0x%.4x (0x%.4x), ", crc, ((u16)(rx_buf[read - 2]) << 8) | ((u16)(rx_buf[read - 1])));
 		if (crc != (((u16)(rx_buf[read - 1]) << 8) | ((u16)(rx_buf[read - 2]))))
			printf("not ok!\n");
		else
			printf("ok!\n");
		printf("\n");
	}
	else {
		if (read) {
			memory_display(rx_buf, 0, read, byte_per_word, 0);
			printf("\n");
		}
	}

	printf("0x%.4x\n", ((u16)(rx_buf[1]) << 8) | ((u16)(rx_buf[2])));

out:
	free(rx_buf);
	free(tx_buf);
	return ret;
}


BAREBOX_CMD_HELP_START(maxq1061)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b BUS\t",  "SPI bus number (default 0)")
BAREBOX_CMD_HELP_OPT ("-c\t",      "chip select (default 0)")
BAREBOX_CMD_HELP_OPT ("-f HZ\t",   "max speed frequency, in Hz (default 1 MHz)")
BAREBOX_CMD_HELP_OPT ("-v\t",      "verbose")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(maxq1061)
	.cmd		= do_maxq1061,
	BAREBOX_CMD_DESC("write/read from MAXQ SPI device")
	BAREBOX_CMD_OPTS("[-bcfv] cmd_byte_hi cmd_byte_lo len_data_hi len_data_lo len_lo DATA...")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_maxq1061_help)
BAREBOX_CMD_END
