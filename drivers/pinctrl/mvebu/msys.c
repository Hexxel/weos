/*
 * Marvell MSYS pinctrl driver based on mvebu pinctrl core
 *
 * Copyright (C) 2012 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <malloc.h>
#include <of.h>
#include <of_address.h>
#include <linux/sizes.h>

#include "common.h"

static void __iomem *mpp_base;

static int msys_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(mpp_base, pid, config);
}

static int msys_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	return default_mpp_ctrl_set(mpp_base, pid, config);
}

static struct mvebu_mpp_mode mv88f6710_mpp_modes[] = {
	MPP_MODE(0, "mpp0", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "uart0", "rxd")),
	MPP_MODE(1, "mpp1", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "uart0", "txd")),
	MPP_MODE(2, "mpp2", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "i2c0", "sck"),
	   MPP_FUNCTION(0x2, "uart0", "txd")),
	MPP_MODE(3, "mpp3", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "i2c0", "sda"),
	   MPP_FUNCTION(0x2, "uart0", "rxd")),
	MPP_MODE(4, "mpp4", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "cpu_pd", "vdd")),
	MPP_MODE(5, "mpp5", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txclko"),
	   MPP_FUNCTION(0x2, "uart1", "txd"),
	   MPP_FUNCTION(0x4, "spi1", "clk"),
	   MPP_FUNCTION(0x5, "audio", "mclk")),
	MPP_MODE(6, "mpp6", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txd0"),
	   MPP_FUNCTION(0x2, "sata0", "prsnt"),
	   MPP_FUNCTION(0x4, "tdm", "rst"),
	   MPP_FUNCTION(0x5, "audio", "sdo")),
	MPP_MODE(7, "mpp7", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txd1"),
	   MPP_FUNCTION(0x4, "tdm", "tdx"),
	   MPP_FUNCTION(0x5, "audio", "lrclk")),
	MPP_MODE(8, "mpp8", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txd2"),
	   MPP_FUNCTION(0x2, "uart0", "rts"),
	   MPP_FUNCTION(0x4, "tdm", "drx"),
	   MPP_FUNCTION(0x5, "audio", "bclk")),
	MPP_MODE(9, "mpp9", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txd3"),
	   MPP_FUNCTION(0x2, "uart1", "txd"),
	   MPP_FUNCTION(0x3, "sd0", "clk"),
	   MPP_FUNCTION(0x5, "audio", "spdifo")),
	MPP_MODE(10, "mpp10", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txctl"),
	   MPP_FUNCTION(0x2, "uart0", "cts"),
	   MPP_FUNCTION(0x4, "tdm", "fsync"),
	   MPP_FUNCTION(0x5, "audio", "sdi")),
	MPP_MODE(11, "mpp11", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxd0"),
	   MPP_FUNCTION(0x2, "uart1", "rxd"),
	   MPP_FUNCTION(0x3, "sd0", "cmd"),
	   MPP_FUNCTION(0x4, "spi0", "cs1"),
	   MPP_FUNCTION(0x5, "sata1", "prsnt"),
	   MPP_FUNCTION(0x6, "spi1", "cs1")),
	MPP_MODE(12, "mpp12", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxd1"),
	   MPP_FUNCTION(0x2, "i2c1", "sda"),
	   MPP_FUNCTION(0x3, "sd0", "d0"),
	   MPP_FUNCTION(0x4, "spi1", "cs0"),
	   MPP_FUNCTION(0x5, "audio", "spdifi")),
	MPP_MODE(13, "mpp13", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxd2"),
	   MPP_FUNCTION(0x2, "i2c1", "sck"),
	   MPP_FUNCTION(0x3, "sd0", "d1"),
	   MPP_FUNCTION(0x4, "tdm", "pclk"),
	   MPP_FUNCTION(0x5, "audio", "rmclk")),
	MPP_MODE(14, "mpp14", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxd3"),
	   MPP_FUNCTION(0x2, "pcie", "clkreq0"),
	   MPP_FUNCTION(0x3, "sd0", "d2"),
	   MPP_FUNCTION(0x4, "spi1", "mosi"),
	   MPP_FUNCTION(0x5, "spi0", "cs2")),
	MPP_MODE(15, "mpp15", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxctl"),
	   MPP_FUNCTION(0x2, "pcie", "clkreq1"),
	   MPP_FUNCTION(0x3, "sd0", "d3"),
	   MPP_FUNCTION(0x4, "spi1", "miso"),
	   MPP_FUNCTION(0x5, "spi0", "cs3")),
	MPP_MODE(16, "mpp16", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxclk"),
	   MPP_FUNCTION(0x2, "uart1", "rxd"),
	   MPP_FUNCTION(0x4, "tdm", "int"),
	   MPP_FUNCTION(0x5, "audio", "extclk")),
	MPP_MODE(17, "mpp17", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "ge", "mdc")),
	MPP_MODE(18, "mpp18", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge", "mdio")),
	MPP_MODE(19, "mpp19", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txclk"),
	   MPP_FUNCTION(0x2, "ge1", "txclkout"),
	   MPP_FUNCTION(0x4, "tdm", "pclk")),
	MPP_MODE(20, "mpp20", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txd4"),
	   MPP_FUNCTION(0x2, "ge1", "txd0")),
	MPP_MODE(21, "mpp21", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txd5"),
	   MPP_FUNCTION(0x2, "ge1", "txd1"),
	   MPP_FUNCTION(0x4, "uart1", "txd")),
	MPP_MODE(22, "mpp22", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txd6"),
	   MPP_FUNCTION(0x2, "ge1", "txd2"),
	   MPP_FUNCTION(0x4, "uart0", "rts")),
	MPP_MODE(23, "mpp23", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "ge0", "txd7"),
	   MPP_FUNCTION(0x2, "ge1", "txd3"),
	   MPP_FUNCTION(0x4, "spi1", "mosi")),
	MPP_MODE(24, "mpp24", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "col"),
	   MPP_FUNCTION(0x2, "ge1", "txctl"),
	   MPP_FUNCTION(0x4, "spi1", "cs0")),
	MPP_MODE(25, "mpp25", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxerr"),
	   MPP_FUNCTION(0x2, "ge1", "rxd0"),
	   MPP_FUNCTION(0x4, "uart1", "rxd")),
	MPP_MODE(26, "mpp26", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "crs"),
	   MPP_FUNCTION(0x2, "ge1", "rxd1"),
	   MPP_FUNCTION(0x4, "spi1", "miso")),
	MPP_MODE(27, "mpp27", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxd4"),
	   MPP_FUNCTION(0x2, "ge1", "rxd2"),
	   MPP_FUNCTION(0x4, "uart0", "cts")),
	MPP_MODE(28, "mpp28", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxd5"),
	   MPP_FUNCTION(0x2, "ge1", "rxd3")),
	MPP_MODE(29, "mpp29", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxd6"),
	   MPP_FUNCTION(0x2, "ge1", "rxctl"),
	   MPP_FUNCTION(0x4, "i2c1", "sda")),
	MPP_MODE(30, "mpp30", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "ge0", "rxd7"),
	   MPP_FUNCTION(0x2, "ge1", "rxclk"),
	   MPP_FUNCTION(0x4, "i2c1", "sck")),
	MPP_MODE(31, "mpp31", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x3, "tclk", NULL),
	   MPP_FUNCTION(0x4, "ge0", "txerr")),
	MPP_MODE(32, "mpp32", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "spi0", "cs0")),
	MPP_MODE(33, "mpp33", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "bootcs"),
	   MPP_FUNCTION(0x2, "spi0", "cs0")),
	MPP_MODE(34, "mpp34", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "wen0"),
	   MPP_FUNCTION(0x2, "spi0", "mosi")),
	MPP_MODE(35, "mpp35", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "oen"),
	   MPP_FUNCTION(0x2, "spi0", "sck")),
	MPP_MODE(36, "mpp36", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "a1"),
	   MPP_FUNCTION(0x2, "spi0", "miso")),
	MPP_MODE(37, "mpp37", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "a0"),
	   MPP_FUNCTION(0x2, "sata0", "prsnt")),
	MPP_MODE(38, "mpp38", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ready"),
	   MPP_FUNCTION(0x2, "uart1", "cts"),
	   MPP_FUNCTION(0x3, "uart0", "cts")),
	MPP_MODE(39, "mpp39", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad0"),
	   MPP_FUNCTION(0x2, "audio", "spdifo")),
	MPP_MODE(40, "mpp40", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad1"),
	   MPP_FUNCTION(0x2, "uart1", "rts"),
	   MPP_FUNCTION(0x3, "uart0", "rts")),
	MPP_MODE(41, "mpp41", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad2"),
	   MPP_FUNCTION(0x2, "uart1", "rxd")),
	MPP_MODE(42, "mpp42", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad3"),
	   MPP_FUNCTION(0x2, "uart1", "txd")),
	MPP_MODE(43, "mpp43", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad4"),
	   MPP_FUNCTION(0x2, "audio", "bclk")),
	MPP_MODE(44, "mpp44", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad5"),
	   MPP_FUNCTION(0x2, "audio", "mclk")),
	MPP_MODE(45, "mpp45", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad6"),
	   MPP_FUNCTION(0x2, "audio", "lrclk")),
	MPP_MODE(46, "mpp46", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad7"),
	   MPP_FUNCTION(0x2, "audio", "sdo")),
	MPP_MODE(47, "mpp47", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad8"),
	   MPP_FUNCTION(0x3, "sd0", "clk"),
	   MPP_FUNCTION(0x5, "audio", "spdifo")),
	MPP_MODE(48, "mpp48", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad9"),
	   MPP_FUNCTION(0x2, "uart0", "rts"),
	   MPP_FUNCTION(0x3, "sd0", "cmd"),
	   MPP_FUNCTION(0x4, "sata1", "prsnt"),
	   MPP_FUNCTION(0x5, "spi0", "cs1")),
	MPP_MODE(49, "mpp49", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad10"),
	   MPP_FUNCTION(0x2, "pcie", "clkreq1"),
	   MPP_FUNCTION(0x3, "sd0", "d0"),
	   MPP_FUNCTION(0x4, "spi1", "cs0"),
	   MPP_FUNCTION(0x5, "audio", "spdifi")),
	MPP_MODE(50, "mpp50", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad11"),
	   MPP_FUNCTION(0x2, "uart0", "cts"),
	   MPP_FUNCTION(0x3, "sd0", "d1"),
	   MPP_FUNCTION(0x4, "spi1", "miso"),
	   MPP_FUNCTION(0x5, "audio", "rmclk")),
	MPP_MODE(51, "mpp51", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad12"),
	   MPP_FUNCTION(0x2, "i2c1", "sda"),
	   MPP_FUNCTION(0x3, "sd0", "d2"),
	   MPP_FUNCTION(0x4, "spi1", "mosi")),
	MPP_MODE(52, "mpp52", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad13"),
	   MPP_FUNCTION(0x2, "i2c1", "sck"),
	   MPP_FUNCTION(0x3, "sd0", "d3"),
	   MPP_FUNCTION(0x4, "spi1", "sck")),
	MPP_MODE(53, "mpp53", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad14"),
	   MPP_FUNCTION(0x2, "sd0", "clk"),
	   MPP_FUNCTION(0x3, "tdm", "pclk"),
	   MPP_FUNCTION(0x4, "spi0", "cs2"),
	   MPP_FUNCTION(0x5, "pcie", "clkreq1")),
	MPP_MODE(54, "mpp54", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "ad15"),
	   MPP_FUNCTION(0x3, "tdm", "dtx")),
	MPP_MODE(55, "mpp55", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "cs1"),
	   MPP_FUNCTION(0x2, "uart1", "txd"),
	   MPP_FUNCTION(0x3, "tdm", "rst"),
	   MPP_FUNCTION(0x4, "sata1", "prsnt"),
	   MPP_FUNCTION(0x5, "sata0", "prsnt")),
	MPP_MODE(56, "mpp56", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "cs2"),
	   MPP_FUNCTION(0x2, "uart1", "cts"),
	   MPP_FUNCTION(0x3, "uart0", "cts"),
	   MPP_FUNCTION(0x4, "spi0", "cs3"),
	   MPP_FUNCTION(0x5, "pcie", "clkreq0"),
	   MPP_FUNCTION(0x6, "spi1", "cs1")),
	MPP_MODE(57, "mpp57", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "cs3"),
	   MPP_FUNCTION(0x2, "uart1", "rxd"),
	   MPP_FUNCTION(0x3, "tdm", "fsync"),
	   MPP_FUNCTION(0x4, "sata0", "prsnt"),
	   MPP_FUNCTION(0x5, "audio", "sdo")),
	MPP_MODE(58, "mpp58", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "cs0"),
	   MPP_FUNCTION(0x2, "uart1", "rts"),
	   MPP_FUNCTION(0x3, "tdm", "int"),
	   MPP_FUNCTION(0x5, "audio", "extclk"),
	   MPP_FUNCTION(0x6, "uart0", "rts")),
	MPP_MODE(59, "mpp59", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "ale0"),
	   MPP_FUNCTION(0x2, "uart1", "rts"),
	   MPP_FUNCTION(0x3, "uart0", "rts"),
	   MPP_FUNCTION(0x5, "audio", "bclk")),
	MPP_MODE(60, "mpp60", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "ale1"),
	   MPP_FUNCTION(0x2, "uart1", "rxd"),
	   MPP_FUNCTION(0x3, "sata0", "prsnt"),
	   MPP_FUNCTION(0x4, "pcie", "rst-out"),
	   MPP_FUNCTION(0x5, "audio", "sdi")),
	MPP_MODE(61, "mpp61", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "dev", "wen1"),
	   MPP_FUNCTION(0x2, "uart1", "txd"),
	   MPP_FUNCTION(0x5, "audio", "rclk")),
	MPP_MODE(62, "mpp62", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "dev", "a2"),
	   MPP_FUNCTION(0x2, "uart1", "cts"),
	   MPP_FUNCTION(0x3, "tdm", "drx"),
	   MPP_FUNCTION(0x4, "pcie", "clkreq0"),
	   MPP_FUNCTION(0x5, "audio", "mclk"),
	   MPP_FUNCTION(0x6, "uart0", "cts")),
	MPP_MODE(63, "mpp63", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpo", NULL),
	   MPP_FUNCTION(0x1, "spi0", "sck"),
	   MPP_FUNCTION(0x2, "tclk", NULL)),
	MPP_MODE(64, "mpp64", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "spi0", "miso"),
	   MPP_FUNCTION(0x2, "spi0-1", "cs1")),
	MPP_MODE(65, "mpp65", msys_mpp_ctrl,
	   MPP_FUNCTION(0x0, "gpio", NULL),
	   MPP_FUNCTION(0x1, "spi0", "mosi"),
	   MPP_FUNCTION(0x2, "spi0-1", "cs2")),
};

static struct mvebu_mpp_mode mv98dx3236_mpp_modes[] = {
	MPP_MODE(0, "mpp0", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "spi0", "mosi"),
		 MPP_FUNCTION(0x4, "dev", "ad8")),
	MPP_MODE(1, "mpp1", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "spi0", "miso"),
		 MPP_FUNCTION(0x4, "dev", "ad9")),
	MPP_MODE(2, "mpp2", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "spi0", "sck"),
		 MPP_FUNCTION(0x4, "dev", "ad10")),
	MPP_MODE(3, "mpp3", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "spi0", "cs0"),
		 MPP_FUNCTION(0x4, "dev", "ad11")),
	MPP_MODE(4, "mpp4", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "spi0", "cs1"),
		 MPP_FUNCTION(0x3, "smi", "mdc"),
		 MPP_FUNCTION(0x4, "dev", "cs0")),
	MPP_MODE(5, "mpp5", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "pex", "rsto"),
		 MPP_FUNCTION(0x2, "sd0", "cmd"),
		 MPP_FUNCTION(0x4, "dev", "bootcs")),
	MPP_MODE(6, "mpp6", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "sd0", "clk"),
		 MPP_FUNCTION(0x4, "dev", "a2")),
	MPP_MODE(7, "mpp7", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "sd0", "d0"),
		 MPP_FUNCTION(0x4, "dev", "ale0")),
	MPP_MODE(8, "mpp8", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "sd0", "d1"),
		 MPP_FUNCTION(0x4, "dev", "ale1")),
	MPP_MODE(9, "mpp9", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "sd0", "d2"),
		 MPP_FUNCTION(0x4, "dev", "ready0")),
	MPP_MODE(10, "mpp10", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "sd0", "d3"),
		 MPP_FUNCTION(0x4, "dev", "ad12")),
	MPP_MODE(11, "mpp11", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "uart1", "rxd"),
		 MPP_FUNCTION(0x3, "uart0", "cts"),
		 MPP_FUNCTION(0x4, "dev", "ad13")),
	MPP_MODE(12, "mpp12", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x2, "uart1", "txd"),
		 MPP_FUNCTION(0x3, "uart0", "rts"),
		 MPP_FUNCTION(0x4, "dev", "ad14")),
	MPP_MODE(13, "mpp13", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "intr", "out"),
		 MPP_FUNCTION(0x4, "dev", "ad15")),
	MPP_MODE(14, "mpp14", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "i2c0", "sck")),
	MPP_MODE(15, "mpp15", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x4, "i2c0", "sda")),
	MPP_MODE(16, "mpp16", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x4, "dev", "oe")),
	MPP_MODE(17, "mpp17", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x4, "dev", "clkout")),
	MPP_MODE(18, "mpp18", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x3, "uart1", "txd")),
	MPP_MODE(19, "mpp19", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x3, "uart1", "rxd"),
		 MPP_FUNCTION(0x4, "dev", "rb")),
	MPP_MODE(20, "mpp20", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x4, "dev", "we0")),
	MPP_MODE(21, "mpp21", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "ad0")),
	MPP_MODE(22, "mpp22", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "ad1")),
	MPP_MODE(23, "mpp23", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "ad2")),
	MPP_MODE(24, "mpp24", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "ad3")),
	MPP_MODE(25, "mpp25", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "ad4")),
	MPP_MODE(26, "mpp26", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "ad5")),
	MPP_MODE(27, "mpp27", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "ad6")),
	MPP_MODE(28, "mpp28", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "ad7")),
	MPP_MODE(29, "mpp29", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "a0")),
	MPP_MODE(30, "mpp30", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "dev", "a1")),
	MPP_MODE(31, "mpp31", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "slv_smi", "mdc"),
		 MPP_FUNCTION(0x3, "smi", "mdc"),
		 MPP_FUNCTION(0x4, "dev", "we1")),
	MPP_MODE(32, "mpp32", msys_mpp_ctrl,
		 MPP_FUNCTION(0x0, "gpio", NULL),
		 MPP_FUNCTION(0x1, "slv_smi", "mdio"),
		 MPP_FUNCTION(0x3, "smi", "mdio"),
		 MPP_FUNCTION(0x4, "dev", "cs1")),
};

static struct mvebu_pinctrl_soc_info mv88f6710_pinctrl_info = {
	.modes = mv88f6710_mpp_modes,
	.nmodes = ARRAY_SIZE(mv88f6710_mpp_modes),
	.variant = 0,
};

static struct mvebu_pinctrl_soc_info mv98dx3236_pinctrl_info = {
	.modes = mv98dx3236_mpp_modes,
	.nmodes = ARRAY_SIZE(mv98dx3236_mpp_modes),
	.variant = 0,
};

static struct of_device_id msys_pinctrl_of_match[] = {
	{
		.compatible = "marvell,mv88f6710-pinctrl",
		.data = &mv88f6710_pinctrl_info,
	},
	{
		.compatible = "marvell,98dx3236-pinctrl",
		.data = &mv98dx3236_pinctrl_info,
	},
	{ },
};

static int msys_pinctrl_probe(struct device_d *dev)
{
	struct resource *iores;
	const struct of_device_id *match =
		of_match_node(msys_pinctrl_of_match, dev->device_node);
	struct mvebu_pinctrl_soc_info *soc =
		(struct mvebu_pinctrl_soc_info *)match->data;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	mpp_base = IOMEM(iores->start);

	return mvebu_pinctrl_probe(dev, soc);
}

static struct driver_d msys_pinctrl_driver = {
	.name		= "pinctrl-msys",
	.probe		= msys_pinctrl_probe,
	.of_compatible	= msys_pinctrl_of_match,
};

static int msys_pinctrl_init(void)
{
	return platform_driver_register(&msys_pinctrl_driver);
}
postcore_initcall(msys_pinctrl_init);