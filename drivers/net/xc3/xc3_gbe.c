/*
 * Marvell xCAT 3 Packet Processor, GbE MAC
 *
 * Copyright (C) 2014 Westermo Teleindustri AB
 *
 * Author: Tobias Waldekranz <tobias@waldekranz.com>
 *         Joacim Zetterling <joacim.zetterling@westermo.se>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "xc3_private.h"

#define COUNTERS_BASE , 0x11000000
#define COUNTERS_STRIDE 0x400
#define COUNTERS(_i) (COUNTERS_BASE + (COUNTERS_STRIDE * (_i)))

#define BR_PORT_STRIDE 0x1000
#define BR_PORT(_i) (XC3_BR_BASE + (BR_PORT_STRIDE * (_i)))

#define CPU_PORT_CTRL 0x120dd090
#define CPU_PORT_CTRL_SDMA_EN BIT(1)

#define RX_RING_LEN 128
#define RX_RINGS    8
#define TX_RING_LEN 128
#define TX_RINGS    8
#define FRAG_SZ     2048

#define CPU_PORT_CTRL 0x120dd090
#define CPU_PORT_CTRL_SDMA_EN BIT(1)

struct xc3_sdma_regs {
	struct {
		u32  resvd0x0[3];
		u32 pointer;
	} rx_current_desc[8];
	u32 rx_queue_cmd;
	RESVD(0x84, 0xc0);
	u32 tx_current_desc[8];
	RESVD(0xe0, 0x100);
	struct {
		u32 cntr;
		u32 conf;
		u32 wrr_conf;
		u32  resvd0xc;
	} token_bucket[8];
	u32 token_bucket_cntr;
	RESVD(0x184, 0x200);
	u32 conf;
#define CONF_TX_WORD_SWAP BIT(24)
#define CONF_TX_BYTE_SWAP BIT(23)
#define CONF_RX_WORD_SWAP BIT(7)
#define CONF_RX_BYTE_SWAP BIT(6)
#define CONF_MASK (BIT(24) | BIT(23) | BIT(7) | BIT(6))
	RESVD(0x204, 0x20c);
	u32 rx_irq_cause;
#define RX_IRQ_CAUSE_Q(_i) BIT((_i) + 2)
	u32 tx_irq_cause;
#define TX_IRQ_CAUSE_Q(_i) BIT((_i) + 1)
	u32 rx_irq_mask;
	u32 tx_irq_mask;
	u32 rx_status;
	u32 rx_packet_cntr[8];
	u32 rx_byte_cntr[8];
	u32 rx_res_error_mode_cntr_0_1[2];
	u32 tx_queue_cmd;
	u32  resvd0x26c;
	u32 tx_fixed_prio_conf;
	u32 tx_wrr_token_params;
	u32 rx_res_error_mode_cntr_2_7[6];
	u32 rx_irq_cause1;
	u32 rx_irq_cause2;
	u32 tx_irq_cause1;
	u32 tx_irq_cause2;
	u32 rx_irq_mask1;
	u32 rx_irq_mask2;
	u32 tx_irq_mask1;
	u32 tx_irq_mask2;
	u32 tx_pkt_gen_conf[8];
	u32 tx_pkt_cntr_conf[8];
} __packed;

struct xc3_rx_desc {
	u32 cmd_status;
#define RX_DESC_SDMA_OWN BIT(31)
#define RX_DESC_ERR_BUS	 BIT(30)
#define RX_DESC_IRQ_ENA	 BIT(29)
#define RX_DESC_ERR_RES	 BIT(28)
#define RX_DESC_FIRST	 BIT(27)
#define RX_DESC_LAST	 BIT(26)
	u16 size;
	u16 len;
	u32 data;
	u32 next;
} __packed;

struct xc3_tx_desc {
	u32 cmd_status;
#define TX_DESC_SDMA_OWN BIT(31)
#define TX_DESC_IRQ_ENA	 BIT(23)
#define TX_DESC_FIRST	 BIT(21)
#define TX_DESC_LAST	 BIT(20)
#define TX_DESC_CRC_HW	 BIT(12)
	u16 info;
	u16 len;
	u32 data;
	u32 next;
} __packed;

struct xc3_rx_queue {
	int                         id;
	struct xc3_rx_desc __iomem *desc;
	dma_addr_t                  desc_phys;

	int    len;
	int    pos;
	size_t frag_sz;
};

struct xc3_tx_queue {
	int                         id;
	struct xc3_tx_desc __iomem *desc;
	dma_addr_t                  desc_phys;

	int len;
	int pos_push;
	int pos_pop;
};

struct xc3_sdma {
	struct xc3 *xc3;
	struct eth_device *port;
	struct xc3_sdma_regs __iomem *regs;

	struct xc3_rx_queue rxq[8];
	int rx_irq;
	u32 rx_irq_cause;

	struct xc3_tx_queue txq[8];
	int tx_irq;
	u32 tx_irq_cause;
};

struct xc3_br_port {
	u32 ingress_conf_0;
#define INGRESS_CONF_0_PVE_PORT(_p) ((_p) << 25)
#define INGRESS_CONF_0_PVE_EN  BIT(23)
#define INGRESS_CONF_0_ALL_PKT BIT(22)

	u32  resvd0x4[(0x10 - 0x4) >> 2];
	u32 ingress_conf_1;
} __packed;

struct xc3_gbe_counters {
	u32 rx_good_octets_lo;
	u32 rx_good_octets_hi;
	u32 rx_bad_octets;
	u32 tx_crc_fifo;
	u32 rx_unicast;
	u32 tx_deferred;
	u32 rx_broadcast;
	u32 rx_multicast;
	u32 rx_64b;
	u32 rx_65b_to_127b;
	u32 rx_128b_to_255b;
	u32 rx_256b_to_511b;
	u32 rx_512b_to_1023b;
	u32 rx_1kb_to_max;
	u32 tx_good_octets_lo;
	u32 tx_good_octets_hi;
	u32 tx_unicast;
	u32 tx_excessive_collision;
	u32 tx_multicast;
	u32 tx_broadcast;
	u32 tx_multiple;
	u32 tx_fc;
	u32 rx_fc;
	u32 rx_fifo;
	u32 rx_undersize;
	u32 rx_fragments;
	u32 rx_oversize;
	u32 rx_jabber;
	u32 rx_frame;
	u32 rx_crc;
	u32 tx_collisions;
	u32 tx_late_collisions;
} __packed;

struct xc3_gbe_regs {
	u32 mac_ctrl_0;
#define MAC_CTRL_0_PORTEN BIT(0)
	u32 mac_ctrl_1;
	u32 mac_ctrl_2;
#define MAC_CTRL_2_RESET BIT(6)
	u32 aneg_config;
#define ANEG_ANSPEEDEN BIT(7)
#define ANEG_FORCELINKUP BIT(1)
#define ANEG_FORCELINKDOWN BIT(0)
	u32 stat_0;
	u32 serial_param_conf_0;
	u32  resvd0x18[2];
	u32 irq_cause;
	u32 irq_mask;
	u32 serdes_conf_0;
#define SERDES_CONF_0_AN_MASTER_MODE BIT(11)
	u32  resvd0x2c;
	u32 serdes_conf_2;
	u32 ability_match_stat;
	u32 prbs_stat;
	u32 prbs_err_cntr;
	u32 stat_1;
	u32 mib_cntr_ctrl;
#define MIB_CNTR_CTRL_NO_CLEAR BIT(1)
	u32 mac_ctrl_3;
	u32  resvd0x4c[3];
	u32 ccfc_speed_timerp_idx0;
	u32 ccfc_speed_timerp_idx1;
	u32  resvd0x60[12];
	u32 mac_ctrl_4;
#define MAC_CTRL_4_QSGMII_BYPASS BIT(7)
	u32 serial_param_conf_1;
	u32  resvd0x98[2];
	u32 irq_sum_cause;
	u32 irq_sum_mask;
	u32  resvd0xa8[6];
	u32 lpi_ctrl_0;
	u32 lpi_ctrl_1;
	u32  resvd0xc8;
	u32 lpi_stat;
	u32 lpi_cntr;
} __packed; /* 0xd4 */

struct xc3_gbe_xregs {
	u32 mac_ctrl_0;
#define MAC_CTRL_0_PORTEN BIT(0)
	u32 mac_ctrl_1;
	u32 mac_ctrl_2;
	u32 stat;
	u32  resvd0x10;
	u32 irq_cause;
	u32 irq_mask;
	u32 mac_ctrl_3;
	u32  resvd0x20[3];
	u32 mib_cntr_ctrl;
	u32  resvd0x34;
	u32 ccfc_speed_timeri;
	u32  resvd0x3c[7];
	u32 ext_units_irq_cause;
	u32 ext_units_irq_mask;
	u32  resvd0x60[9];
	u32 mac_ctrl_4;
	u32 mac_ctrl_5;
} __packed; /* 0xxx */

struct xc3_gbe {
	struct device_d 					*pdev;
	struct xc3_gbe_regs __iomem	*regs;
	struct xc3_gbe_xregs __iomem	*xregs;
	struct xc3_gbe_counters 		counters;
	struct xc3 							*xc3;
 	struct xc3_sdma 					*sdma;

	struct eth_device *port;
	struct phy_device *phydev[24+4];
// 	struct phy_device *phydev;
	u32    phynum;
};


void xc3_sdma_dump(char *lbl, void *data, int len)
{
	if (len < 0)
		len = 0x80;

	pr_info("\n%s\n", lbl);
	print_hex_dump(KERN_INFO, "", 0, 16, 1, (char *)data, len, 0);
	pr_info("\n");
}

static int xc3_sdma_rx_desc_fill(struct xc3_rx_desc __iomem *rxd, size_t frag_sz)
{
	void *frag = xzalloc(frag_sz + 0x80);

	if ((int)frag % 0x80)
		frag = (void *)(((int)frag & 0xffffff00) + 0x80);

	if (!frag)
		return -ENOMEM;

	writel(RX_DESC_SDMA_OWN | RX_DESC_IRQ_ENA, &rxd->cmd_status);
	writel(virt_to_phys(frag), &rxd->data);
	writew(frag_sz, &rxd->size);
	return 0;
}

static int xc3_sdma_rx_setup_queue(struct xc3_sdma *sdma, int id,
				   int len, size_t frag_sz)
{
	struct xc3_rx_queue *rxq = &sdma->rxq[id];
	int err, i;

	rxq->pos     = 0;
	rxq->len     = len;
	rxq->frag_sz = frag_sz;

	for (i = 0; i < len; i++) {
		writel(rxq->desc_phys + (i + 1) * sizeof(*rxq->desc),
		       &rxq->desc[i].next);

		err = xc3_sdma_rx_desc_fill(&rxq->desc[i], frag_sz);
		if (err)
			return err;
	}

	/* link the last descriptor to the first */
	writel(rxq->desc_phys, &rxq->desc[len - 1].next);

	writel(rxq->desc_phys, &sdma->regs->rx_current_desc[id].pointer);
	return 0;
}

static int xc3_sdma_rx_setup_queues(struct xc3_sdma *sdma)
{
	struct xc3_rx_desc __iomem *rxd;
	dma_addr_t rxd_phys;
	size_t rxd_sz;
	int err, i;

	rxd_sz = RX_RINGS * RX_RING_LEN * sizeof(struct xc3_rx_desc);
	rxd = dma_alloc_coherent(rxd_sz, &rxd_phys);
	if (!rxd)
		return -ENOMEM;

	for (i = 0; i < RX_RINGS; i++, rxd += RX_RING_LEN,
		     rxd_phys += RX_RING_LEN * sizeof(*rxd)) {
		sdma->rxq[i].id = i;
		sdma->rxq[i].desc = rxd;
		sdma->rxq[i].desc_phys = rxd_phys;

		err = xc3_sdma_rx_setup_queue(sdma, i, RX_RING_LEN, FRAG_SZ);
		if (err)
			break;
	}

	return err;
}

static int xc3_sdma_tx_desc_reset(struct xc3_tx_desc __iomem *txd)
{
	u32 cmd_status;

	cmd_status = TX_DESC_IRQ_ENA | TX_DESC_FIRST | TX_DESC_LAST;

	writel(cmd_status, &txd->cmd_status);
	return 0;
}

static int xc3_sdma_tx_setup_queue(struct xc3_sdma *sdma, int id, int len)
{
	struct xc3_tx_queue *txq = &sdma->txq[id];
	int err, i;

	txq->pos_push = 0;
	txq->pos_pop  = 0;
	txq->len      = len;

	for (i = 0; i < len; i++) {
		writel(txq->desc_phys + (i + 1) * sizeof(*txq->desc),
		       &txq->desc[i].next);

		err = xc3_sdma_tx_desc_reset(&txq->desc[i]);
		if (err)
			return err;
	}

	/* link the last descriptor to the first */
	writel(txq->desc_phys, &txq->desc[len - 1].next);

	writel(txq->desc_phys, &sdma->regs->tx_current_desc[id]);
	return 0;
}

static int xc3_sdma_tx_setup_queues(struct xc3_sdma *sdma)
{
	struct xc3_tx_desc __iomem *txd;
	dma_addr_t txd_phys;
	size_t txd_sz;
	int err, i;

	txd_sz = TX_RINGS * TX_RING_LEN * sizeof(struct xc3_tx_desc);
	txd    = dma_alloc_coherent(txd_sz, &txd_phys);
	if (!txd)
		return -ENOMEM;

	for (i = 0; i < 1/*TX_RINGS*/; i++, txd += TX_RING_LEN,
		     txd_phys += TX_RING_LEN * sizeof(*txd)) {
		sdma->txq[i].id = i;
		sdma->txq[i].desc = txd;
		sdma->txq[i].desc_phys = txd_phys;

		err = xc3_sdma_tx_setup_queue(sdma, i, TX_RING_LEN);
		if (err)
			break;
	}

	return err;
}

static void xc3_sdma_rx_init(struct xc3_sdma *sdma)
{
	writel((0xff << 11) | (0xff << 2), &sdma->regs->rx_irq_mask);
	writel(0xff, &sdma->regs->rx_queue_cmd);
}

static void xc3_sdma_tx_init(struct xc3_sdma *sdma)
{
	int i;

	/* the token bucket config is just copied from datasheet, all
	 * bits are marked reserved but require these initial
	 * values. no idea what this does, thanks again marvell. */
	for (i = 0; i < 8; i++) {
		writel(0x00000000, &sdma->regs->token_bucket[i].cntr);
		writel(0xffffffcf, &sdma->regs->token_bucket[i].conf);
	}

	writel(0x00000000, &sdma->regs->token_bucket_cntr);
	writel(0xffffffc1, &sdma->regs->tx_wrr_token_params);

	writel(0xff, &sdma->regs->tx_fixed_prio_conf);

	writel((0xff << 9) | (0xff << 1), &sdma->regs->tx_irq_mask);
}

static inline void __xc3_put_tag(uint8_t *buf, u32 tag[2])
{
	buf[0] = (tag[0] >> 24);
	buf[1] = (tag[0] >> 16) & 0xff;
	buf[2] = (tag[0] >> 8) & 0xff;
	buf[3] = (tag[0] >> 0) & 0xff;

	buf[4] = (tag[1] >> 24);
	buf[5] = (tag[1] >> 16) & 0xff;
	buf[6] = (tag[1] >> 8) & 0xff;
	buf[7] = (tag[1] >> 0) & 0xff;
}

static void xc3_gbe_edev_adjust_link(struct eth_device *edev)
{
	struct xc3_gbe *gbe = edev->priv;
	int i;

	for (i = 0; i < gbe->phynum; i++) {
		/* update phy (link) status */
		phy_update_status(gbe->phydev[i]);

		/* Check for an open link */
		if (gbe->phydev[i]->link) {
			edev->phydev = gbe->phydev[i];
			break;
		}
	}
}

static int xc3_gbe_edev_init(struct eth_device *edev)
{
	struct xc3_gbe *gbe = edev->priv;
	struct xc3_br_port __iomem *brp;
	int i;

	struct device_d *pdev = edev->parent; /* 8019f000.ethernet device */
	struct device_node *parent = pdev->parent->device_node; /* f60f800c.packet-processor device_node */
	struct device_node *child, *phy_node;
	struct phy_device *phy;
	struct device_d *dev;
	phy_interface_t intf;
	int addr, ret;

// pr_info ("[%s] ***** \n", __FUNCTION__);

	for (i = 0; i < 24+4; i++) {
		brp = xc3_win_get(gbe->xc3, BR_PORT(i));

		/* default: send all packets to CPU(63) until the port is
		* placed in a bridge */
		writel(readl(&brp->ingress_conf_0) |
	       INGRESS_CONF_0_PVE_PORT(63) |
	       INGRESS_CONF_0_PVE_EN | INGRESS_CONF_0_ALL_PKT,
	       &brp->ingress_conf_0);
		/* writel(readl(&brp->ingress_conf_0) | BIT(14) | BIT(13) | BIT(12), &brp->ingress_conf_0); */

		xc3_win_put(gbe->xc3);
	}

	gbe->phynum = 0;
 	for_each_child_of_node(parent, child) {
		if ((of_device_is_compatible(child, "marvell,xcat-3-mdio")) ||
			 (of_device_is_compatible(child, "marvell,xcat-3-xsmi-mdio"))) {
			for_each_child_of_node(child, phy_node) {
				/* warn on missing port reg property */
				if (of_property_read_u32(phy_node, "reg", &addr))
					pr_err("error: port node is missing reg property\n");

				ret = of_get_phy_mode(phy_node);
				if (ret > 0)
					intf = ret;
				else
					intf = PHY_INTERFACE_MODE_RGMII;

				bus_for_each_device(&mdio_bus_type, dev) {
					if (dev->device_node == phy_node) {
						phy = container_of(dev, struct phy_device, dev);
						//edev->phydev = phy;
						edev->phydev = NULL;
						ret = phy_device_connect(edev, phy->bus, addr, xc3_gbe_edev_adjust_link, 0, intf);
						if (ret)
							continue;

						if (gbe->phynum < 24+4)
							gbe->phydev[gbe->phynum++] = edev->phydev;
						break;
					}
				}
			}
		}
	}

	return 0;
}

static void xc3_gbe_edev_setup(struct eth_device *edev)
{
	struct xc3_gbe *gbe = edev->priv;
	struct xc3_gbe_regs __iomem *regs = gbe->regs;
	struct xc3_gbe_xregs __iomem *xregs = gbe->xregs;
	int i;
	u32 data;

	for (i = 0; i < 24; i++) {
		data = 0x00008BE4;
		writel(data, &regs->mac_ctrl_0);

		data = 0x0000B268;
		writel(data, &regs->aneg_config);

		regs = (struct xc3_gbe_regs __iomem *)((char*)regs + 0x1000);
	}

	for (i = 0; i < 4; i++) {
		data = 0x00008BE4;
		writel(data, &regs->mac_ctrl_0);

		data = 0x00000680;
		writel(data, &xregs->mac_ctrl_0);

		regs = (struct xc3_gbe_regs __iomem *)((char*)regs + 0x1000);
		xregs = (struct xc3_gbe_xregs __iomem *)((char*)xregs + 0x1000);
	}
}

static void xc3_gbe_edev_halt(struct eth_device *edev)
{
	struct xc3_sdma *priv = edev->priv;
	xc3_rmw(priv->xc3, CPU_PORT_CTRL, ~CPU_PORT_CTRL_SDMA_EN, 0);
}

static int xc3_gbe_edev_send(struct eth_device *edev, void *data, int len)
{
/* TODO: does pos_push need a lock? */

	int id = 0;
	struct xc3_gbe *priv = edev->priv;
	struct xc3_sdma *sdma = priv->sdma;
	struct xc3_tx_queue *txq = &sdma->txq[id];
	struct xc3_tx_desc __iomem *txd = &txq->desc[txq->pos_push];

	u32 tag[2];
	u32 taglen = 8;
	u8  *txptr;

	int loop = 1;
	u32 cmd_status;

	if (unlikely(readl(&txd->cmd_status) & TX_DESC_SDMA_OWN)) {
		return 0;
	}

 	tag[0] = EDSA_TAG_MAKE(EDSA_FORWARD) | EDSA_PORT_MAKE(63) | EDSA_W0_EXT_MAKE | 1;
	tag[1] = 0;

	dma_sync_single_for_device((unsigned long)virt_to_phys(data), max(64/*60/64 ETH_ZLEN*/, (int)len) + 4/*ETH_FCS_LEN*/, DMA_TO_DEVICE);
		
	/* Injects a Marvel tag */
// 	memmove((char *)data + 12 + taglen, (char *)data + 12, len - 12);
// 	__xc3_put_tag((char *)data + 12, tag);
// 	len += taglen;
	txptr = xmemalign(32, len + taglen);
	memcpy(txptr, data, 12);
	__xc3_put_tag(txptr + 12, tag);
	memcpy(txptr + 12 + taglen, data + 12, len - 12);	
	len += taglen;

// 	writel(virt_to_phys(data), &txd->data);
	writel(virt_to_phys(txptr), &txd->data);
 	writew(max(64/*60/64 ETH_ZLEN*/, (int)len) + 4/*ETH_FCS_LEN*/, &txd->len);
	writel(readl(&txd->cmd_status) | TX_DESC_SDMA_OWN, &txd->cmd_status);

	writel(BIT(id), &sdma->regs->tx_queue_cmd);
	txq->pos_push = (txq->pos_push + 1) & (txq->len - 1);
	free(txptr);

	dma_sync_single_for_cpu((unsigned long)virt_to_phys(data), max(64/*60/64 ETH_ZLEN*/, (int)len) + 4/*ETH_FCS_LEN*/, DMA_TO_DEVICE);

	while (--loop) {
		sdma->tx_irq_cause = readl(&sdma->regs->tx_irq_cause);
		if (sdma->tx_irq_cause & TX_IRQ_CAUSE_Q(0)) {
			while (txq->pos_pop != txq->pos_push) {
				cmd_status = readl(&txd->cmd_status);
				if (cmd_status & TX_DESC_SDMA_OWN)
					break;

				txq->pos_pop = (txq->pos_pop + 1) & (txq->len - 1);
				txd = &txq->desc[txq->pos_pop];
			}
		}
	}
	
	return 0;
}

static int xc3_gbe_edev_recv(struct eth_device *edev)
{
	struct xc3_gbe *priv = edev->priv;
	struct xc3_sdma *sdma = priv->sdma;
 	struct xc3_rx_queue *rxq;
 	struct xc3_rx_desc __iomem *rxd;

	u32 cmd_status;
	const u32 taglen = 8;
	u16 len;
 	int i;

	if ((sdma->rx_irq_cause = readl(&sdma->regs->rx_irq_cause)) == 0)
		return 0;

	for (i = 7; i >= 0; i --) {
		if (sdma->rx_irq_cause & RX_IRQ_CAUSE_Q(i)) {
			rxq = &sdma->rxq[i];
			rxd = &rxq->desc[rxq->pos];

			while (1) {
				cmd_status = readl(&rxd->cmd_status);
				if (cmd_status & RX_DESC_SDMA_OWN)
					break;

				len = readw(&rxd->len) & 0x3fff;
				dma_sync_single_for_cpu((unsigned long)phys_to_virt(readl(&rxd->data)), len - 4/*ETH_FCS_LEN*/, DMA_FROM_DEVICE);
				memmove((void *)(rxd->data + 12), (void *)(rxd->data + 12 + taglen), len - 12 - taglen);
				len -= taglen;

				/* received packet is padded with two null bytes (Marvell header) */
				net_receive(edev, (void *)(phys_to_virt(readl(&rxd->data))), len - 4/*ETH_FCS_LEN*/);
				dma_sync_single_for_device((unsigned long)phys_to_virt(readl(&rxd->data)), len - 4/*ETH_FCS_LEN*/, DMA_FROM_DEVICE);

				writel(RX_DESC_SDMA_OWN | RX_DESC_IRQ_ENA, &rxd->cmd_status);

 				rxq->pos = (rxq->pos + 1) & (rxq->len - 1);
				rxd = &rxq->desc[rxq->pos];
			}
		}
	}

	return 0;
}

static int xc3_gbe_edev_set_ethaddr(struct eth_device *edev, const unsigned char *mac)
{
	return 0;
}

static int xc3_gbe_edev_get_ethaddr(struct eth_device *edev, unsigned char *mac)
{
	return 0;
}

static int xc3_gbe_edev_open(struct eth_device *edev)
{
	struct xc3_gbe *gbe = edev->priv;
	struct xc3_gbe_regs __iomem *regs = gbe->regs;
	struct xc3_gbe_xregs __iomem *xregs = gbe->xregs;
	static int init_done = 0;

	int i;
	u32 data;

	if (!gbe) {
		pr_err("error: %d\n", -1);
		return -1;
	}

	if (!init_done) {
		xc3_gbe_edev_init(edev);
		init_done = 1;
	}

	for (i = 0; i < 24; i++) {
		data = readl(&regs->mac_ctrl_2) & ~MAC_CTRL_2_RESET;
		writel(data, &regs->mac_ctrl_2);

		data = readl(&regs->mac_ctrl_4) & ~MAC_CTRL_4_QSGMII_BYPASS;
		writel(data, &regs->mac_ctrl_4);

		data = readl(&regs->mib_cntr_ctrl) | MIB_CNTR_CTRL_NO_CLEAR;
		writel(data, &regs->mib_cntr_ctrl);

		data = readl(&regs->mac_ctrl_0) | MAC_CTRL_0_PORTEN;
		writel(data, &regs->mac_ctrl_0);

		regs = (struct xc3_gbe_regs __iomem *)((char*)regs + 0x1000);
	}

	for (i = 0; i < 4; i++) {
// pr_info ("[%s] regs = 0x%.8x, xregs = 0x%.8x\n", __func__, (unsigned int)regs, (unsigned int)xregs);
		
		data = 0x0000FCDA;//readl(&regs->mac_ctrl_4) & ~MAC_CTRL_0_PORTEN;
		writel(data, &regs->mac_ctrl_4);
		
		data = 0x00000000;
		writel(data, &xregs->mac_ctrl_3);

		data = readl(&regs->mac_ctrl_0) & ~MAC_CTRL_0_PORTEN;
		writel(data, &regs->mac_ctrl_0);

		data = 0x0000B0EC;//readl(&regs->aneg_config) | ANEG_FORCELINKDOWN;
		writel(data, &regs->aneg_config);

		data = 0x00000C01;//readl(&regs->mac_ctrl_1) & ~xxxxxxx;
		writel(data, &regs->mac_ctrl_1);

		data = 0x0000C048;//readl(&regs->mac_ctrl_2) & ~xxxxxxx;
		writel(data, &regs->mac_ctrl_2);
		data = 0x0000C008;//readl(&regs->mac_ctrl_2) | xxxxxxx;
		writel(data, &regs->mac_ctrl_2);

// 		data = readl(&regs->mac_ctrl_0) | MAC_CTRL_0_PORTEN;
// 		writel(data, &regs->mac_ctrl_0);
		writel(0x8be5, &regs->mac_ctrl_0);

// 		data = readl(&xregs->mac_ctrl_0) | MAC_CTRL_0_PORTEN;
//  		writel(data, &xregs->mac_ctrl_0);
		writel(0x681, &xregs->mac_ctrl_0);

		regs = (struct xc3_gbe_regs __iomem *)((char*)regs + 0x1000);
		xregs = (struct xc3_gbe_xregs __iomem *)((char*)xregs + 0x1000);
	}

	/* Start sdma DMA */
	xc3_rmw(gbe->xc3, CPU_PORT_CTRL, ~CPU_PORT_CTRL_SDMA_EN,
		CPU_PORT_CTRL_SDMA_EN);

	/* Check if any link up, set linked phy to eth */
	xc3_gbe_edev_adjust_link(edev);

	return 0;
}

static int xc3_gbe_probe(struct device_d *pdev)
{
	struct device_node *np = pdev->device_node;
 	struct eth_device *edev;
	struct xc3_gbe *priv;
	struct xc3_sdma *sdma;
	int err = -ENOMEM;
	int ret;
	u32 conf;

	priv = xzalloc(sizeof(*priv));
	if (!priv) {
		pr_err("error: xzalloc: %d\n", err);
		return err;
	}

	priv->pdev = pdev;
	priv->xc3 = pdev->parent->type_data;

	priv->regs = of_iomap(np, 0);
	if (!priv->regs) {
		pr_err("error: of_iomap: %d\n", -ENOMEM);
		return -ENOMEM;
	}

//	priv->xregs = of_iomap(np, 2);
	priv->xregs = (struct xc3_gbe_xregs *)((char*)priv->regs + 0xd8000);
	if (!priv->xregs) {
		pr_err("error: of_iomap: %d\n", -ENOMEM);
		return -ENOMEM;
	}

	sdma = xzalloc(sizeof(*sdma));
	if (!sdma) {
		pr_err("error: xzalloc: %d\n", -ENOMEM);
		return -ENOMEM;
	}

	sdma->xc3  = pdev->parent->type_data;
	sdma->regs = of_iomap(np, 1);
	writel(0xff00, &sdma->regs->rx_queue_cmd);	/* disable sdma rx engine */

	err = xc3_sdma_rx_setup_queues(sdma);
	if (err) {
		pr_err("error: sdma_rx_setup_queues: %d\n", err);
		return err;
	}

	err = xc3_sdma_tx_setup_queues(sdma);
	if (err) {
		pr_err("error: sdma_tx_setup_queues: %d\n", err);
		return err;
	}

	conf  = readl(&sdma->regs->conf);
	conf &= ~(CONF_TX_WORD_SWAP | CONF_TX_BYTE_SWAP);
	conf |=   CONF_RX_WORD_SWAP | CONF_RX_BYTE_SWAP;
	writel(conf, &sdma->regs->conf);

	xc3_sdma_rx_init(sdma);
	xc3_sdma_tx_init(sdma);
	priv->sdma = sdma;

#if 0
	priv->phy_node = of_parse_phandle(np, "phy-handle", 0);
	if (!priv->phy_node) {
		pr_err("error: of_parse_phandle: %d\n", -ENODEV);
		return -ENODEV;
	}
#endif

	edev = xzalloc(sizeof(*edev));
	if (!edev) {
		pr_err("error: edev xzalloc: %d\n", -ENOMEM);
		return -ENOMEM;
	}

	priv->port = edev;
	sdma->port = edev;
	xc3_gbe_edev_setup(edev);

	/* register eth device */
	edev->priv = priv;
	edev->open = xc3_gbe_edev_open;
	edev->send = xc3_gbe_edev_send;
	edev->recv = xc3_gbe_edev_recv;
	edev->halt = xc3_gbe_edev_halt;
 	edev->set_ethaddr = xc3_gbe_edev_set_ethaddr;
 	edev->get_ethaddr = xc3_gbe_edev_get_ethaddr;
	//edev->parent = pdev;
	edev->parent = priv->pdev;

	ret = eth_register(edev);
	if (ret) {
		pr_err("error: eth_register: %d\n", -ENOMEM);
		return -ENOMEM;
	}

	pr_info("%s: probed\n", dev_name(pdev));

	return 0;
}


static struct of_device_id xc3_gbe_dt_ids[] = {
	{ .compatible = "marvell,xcat-3-gbe-mac" },
	{ }
};

static struct driver_d xc3_gbe_driver = {
	.name   = "xc3_gbe",
	.probe  = xc3_gbe_probe,
	.of_compatible = DRV_OF_COMPAT(xc3_gbe_dt_ids),
};

device_platform_driver(xc3_gbe_driver);
