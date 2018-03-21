#ifndef __XC3_PRIVATE_H
#define __XC3_PRIVATE_H

#include <driver.h>
#include <common.h>
#include <dma.h>
#include <init.h>
#include <io.h>
#include <net.h>
#include <of.h>
#include <of_address.h>
#include <of_net.h>

#include <linux/mutex.h>

#define xc3_print(_xc3, _lvl, _args...) dev_ ## _lvl(&(_xc3)->pdev->dev, _args)
#define xc3_dbg(_xc3, _args...)  xc3_print(_xc3, dbg,  _args)
#define xc3_info(_xc3, _args...) xc3_print(_xc3, info, _args)
#define xc3_err(_xc3, _args...)  xc3_print(_xc3, err,  _args)

#define RESVD(_from, _to) u32 resvd ## _from [(((_to) - (_from)) >> 2)]

#define XC3_RANGES_WIDTH 4
#define XC3_PORTS 32
#define ADDR_COMPL(_i) ((_i) << 19)
#define ADDR_COMPL_SZ  0x80000

#define DYN_WIN ADDR_COMPL(7)

enum edsa_tag {
	EDSA_TO_CPU = 0,
	EDSA_FROM_CPU,
	EDSA_TO_ANALYZER,
	EDSA_FORWARD
};

#define ETH_PLEN       2
#define EDSA_HLEN      8

#define EDSA_TAG_MAKE(_tag) ((_tag) << 30)
#define EDSA_TAG_GET(_w0)   ((enum edsa_tag)((_w0) >> 30))

#define EDSA_PORT_MAKE(_port) (((_port) & 0x1f) << 19)
#define EDSA_PORT_GET(_w0)    (((_w0) >> 19) & 0x1f)

#define EDSA_W0_EXT_MAKE     BIT(12)
#define EDSA_W0_EXT_GET(_w0) ((_w) & BIT(12))

struct xc3 {
	//struct platform_device *pdev;
	struct device_d *pdev;
	struct xc3_mg __iomem *mg;

// 	struct net_device *cpu;
// 	struct net_device *port[XC3_PORTS];
	struct eth_device *cpu;
	struct eth_device *port[XC3_PORTS];

	struct mutex  dyn_win_lock;
	void __iomem *dyn_win;
/*
	struct {
		struct cdev       cdev;
		struct class     *class;
		struct device    *dev;
	} cdev;
*/
	struct cdev   cdev;
	struct dentry *debugfs;
};

u32 xc3_device_id(struct xc3 *xc3);

#define xc3_foreach_port(_xc3, _port) for \
		((_port) = (_xc3)->port; (_port) < &((_xc3)->port[XC3_PORTS]); (_port)++) \
		if (*(_port))

void __iomem *xc3_win_get(struct xc3 *xc3, u32 base);
void          xc3_win_put(struct xc3 *xc3);
void xc3_rmw(struct xc3 *xc3, u32 address, u32 mask, u32 val);


int xc3_cdev_init(struct xc3 *xc3);
#ifdef CONFIG_DEBUG_FS
int xc3_debugfs_init(struct xc3 *xc3);
#else
static inline int xc3_debugfs_init(struct xc3 *xc3) { return 0; }
#endif

#define XC3_ROW_WIDTH 8

struct xc3_row {
	u32 size;
	u32 data[XC3_ROW_WIDTH];
};

void xc3_tbl_read(struct xc3 *xc3, u32 base, u32 idx, struct xc3_row *row);
void xc3_tbl_write(struct xc3 *xc3, u32 base, u32 idx, const struct xc3_row *row);

/***************************************
 *            BRIDGE Support           *
 ***************************************/

#define XC3_TTI_BASE 0x16000000
#define XC3_BR_BASE  0x01000000
#define XC3_TXQ_BASE 0x02800000

#define BRIDGE_GLOBAL_CONF_REG_0 0x40000
#define BRIDGE_GLOBAL_CONF_REG_1 0x40004
#define DSA_TAG_SOURCE_DEVICE_LOCAL_FILTER  BIT(31)
#define ARP_BC_CMD_MIRROR_TO_CPU  BIT(7)

/****************************************
 *      Egress Filtering Registers      *
 ****************************************/
#define XC3_EGR_FILTER_REG0 0x02800010
#define XC3_EGR_FILTER_REG1 0x02800014
#define XC3_EGR_FILTER_REG2 0x028001f4

/***************************************
 *            VLAN Support             *
 ***************************************/
#define XC3_VLT_BASE  0x03800100
#define XC3_VLT_WIDTH 6

enum xc3_vlt_type {
	XC3_VLT_VLAN  = 0,
	XC3_VLT_MCGRP,
	XC3_VLT_STP,
	XC3_VLT_VRF,
	XC3_VLT_ECID,
};

struct xc3_vlt_entry {
	u32 data[XC3_VLT_WIDTH];
} __packed;

struct xc3_vlt_regs {
	struct xc3_vlt_entry entry;
	u32 cmd;
} __packed;

void xc3_vlt_write(struct xc3 *xc3, enum xc3_vlt_type table, u16 row,
		   const struct xc3_vlt_entry *entry);
void xc3_vlt_read(struct xc3 *xc3, enum xc3_vlt_type table, u16 row,
		  struct xc3_vlt_entry *entry);


enum xc3_vlan_tag {
	XC3_VLAN_UNTAGGED = 0,
	XC3_VLAN_TAG0,
	XC3_VLAN_TAG1,
	XC3_VLAN_TAG01,
	XC3_VLAN_TAG10,
	XC3_VLAN_TAG0_PUSH,
	XC3_VLAN_POP
};

enum xc3_vlan_isolation {
	XC3_VLAN_NO_ISO = 0,
	XC3_VLAN_L2_ISO,
	XC3_VLAN_L3_ISO,
	XC3_VLAN_L2_L3_ISO
};

extern const struct xc3_vlt_entry XC3_VLAN_DEFAULT;

int  xc3_vlan_get_valid(struct xc3_vlt_entry *e);
void xc3_vlan_set_valid(struct xc3_vlt_entry *e, int valid);

enum xc3_vlan_isolation xc3_vlan_get_isolation(struct xc3_vlt_entry *e);
void xc3_vlan_set_isolation(struct xc3_vlt_entry *e, enum xc3_vlan_isolation iso);

int  xc3_vlan_get_member(struct xc3_vlt_entry *e, u32 port);
void xc3_vlan_set_member(struct xc3_vlt_entry *e, u32 port, int member);
int  xc3_vlan_get_cpu_member(struct xc3_vlt_entry *e);
void xc3_vlan_set_cpu_member(struct xc3_vlt_entry *e, int member);
void xc3_vlan_set_cpu_trapping(struct xc3_vlt_entry *e, u32 trap);

enum xc3_vlan_tag xc3_vlan_get_tag(struct xc3_vlt_entry *e, u32 port);
void xc3_vlan_set_tag(struct xc3_vlt_entry *e, u32 port, enum xc3_vlan_tag tag);
void xc3_vlan_set_stg(struct xc3_vlt_entry *e, int stg_idx);

void xc3_port_pvid_set(struct xc3 *xc3, int port, u16 vid);

int xc3_br_init(struct xc3 *xc3);

/***************************************
 *             FDB Support             *
 ***************************************/
#define XC3_FDB_BASE  0x0b000000
#define XC3_FDB_WIDTH 4

enum xc3_fdb_type {
	XC3_FDB_MAC = 0,
	XC3_FDB_IPV4,
	XC3_FDB_IPV6
};

struct xc3_fdb_entry {
	u32 data[XC3_FDB_WIDTH];
} __packed;

void xc3_fdb_write(struct xc3 *xc3, u16 row, struct xc3_fdb_entry *entry);
void xc3_fdb_read (struct xc3 *xc3, u16 row, struct xc3_fdb_entry *entry);

int xc3_fdb_get_port(struct xc3_fdb_entry *e);
void xc3_fdb_get_mac(struct xc3_fdb_entry *e, u8 *mac);
u16 xc3_fdb_get_vid(struct xc3_fdb_entry *e);
enum xc3_fdb_type xc3_fdb_get_type(struct xc3_fdb_entry *e);
int xc3_fdb_get_age(struct xc3_fdb_entry *e);
int xc3_fdb_get_skip(struct xc3_fdb_entry *e);
int xc3_fdb_get_valid(struct xc3_fdb_entry *e);
int xc3_fdb_get_static(struct xc3_fdb_entry *e);

void xc3_fdb_set_static(struct xc3_fdb_entry *e, int static_entry);
void xc3_fdb_write_mac(struct xc3 *xc3, const unsigned char *mac, int row, int port, int valid);
int xc3_fdb_find_addr_row(struct xc3 *xc3, const unsigned char *mac, int port);
int xc3_fdb_find_empty_row(struct xc3 *xc3);
int xc3_fdb_get_number_of_macs(struct xc3 *xc3);

void xc3_fdb_write_mac_fifo(struct xc3 *xc3, const unsigned char *mac, int port);

/****************************************
 *        Spanning Tree Support         *
 ****************************************/
#define XC3_PORT_STATE_BASE 0x03880000
#define XC3_PORT_STATE_WIDTH 0x10

enum xc3_port_state {
        XC3_PORT_DISABLED = 0,
        XC3_PORT_BLOCKING,
        XC3_PORT_LISTENING,
        XC3_PORT_FORWARDING
};

void xc3_stp_gen_stg_idx(int port, int vid);
int xc3_stp_get_stg_idx(int port);
void xc3_stp_del_stg_idx(int port, int vid);

/**
 * @descr: Enable/Disable Spanning Tree State Egress filtering for routed packets
 * @param: val -> 0 - Disabled
 *                1 - Enabled
 */
void xc3_stp_egr_filter(struct xc3 *xc3, u32 state);


/* Register manipulation  */
void xc3_reg_read(struct xc3 *xc3, u32 reg, u32 *val);
void xc3_reg_write(struct xc3 *xc3, u32 reg, u32 val);

#endif	/* __XC3_PRIVATE_H */
