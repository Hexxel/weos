/*
 * pcap.c - Ethernet packet capture
 *
 * Copyright (c) 2015 Tobias Waldekranz <tobias@waldekranz.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more detaiifup.
 *
 */
#define pr_fmt(fmt)  "pcap: " fmt

#include <command.h>
#include <common.h>
#include <fcntl.h>
#include <fs.h>
#include <getopt.h>
#include <libbb.h>
#include <net.h>
#include <libfile.h>

#include <asm-generic/div64.h>

typedef struct pcap_hdr_s {
        u32 magic_number;   /* magic number */
#define PCAP_MAGIC 0xa1b2c3d4
        u16 version_major;  /* major version number */
        u16 version_minor;  /* minor version number */
        s32 thiszone;       /* GMT to local correction */
        u32 sigfigs;        /* accuracy of timestamps */
        u32 snaplen;        /* max length of captured packets, in octets */
        u32 network;        /* data link type */
#define PCAP_NETWORK_ETHERNET 1
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
        u32 ts_sec;         /* timestamp seconds */
        u32 ts_usec;        /* timestamp microseconds */
        u32 incl_len;       /* number of octets of packet saved in file */
        u32 orig_len;       /* actual length of packet */
	u8 data[0];
} pcaprec_hdr_t;

static const pcap_hdr_t pcap_hdr = {
	.magic_number = PCAP_MAGIC,
	.version_major = 2,
	.version_minor = 4,
	.snaplen = PKTSIZE,
	.network = PCAP_NETWORK_ETHERNET,
};

enum pcap_opt {
	PCAP_OPT_BG = 0,
	PCAP_OPT_STOP,
	PCAP_OPT_REPLAY,
	PCAP_OPT_WRITE,

	PCAP_OPT_QUIET,
	PCAP_OPT_HEX,
	PCAP_OPT_L2,
};

static void (*rx_handler)(void *pkt, int len) = NULL;
static void (*tx_handler)(void *pkt, int len) = NULL;

void net_pcap_rx(void *pkt, int len)
{
	if (rx_handler)
		rx_handler(pkt, len);
}

void net_pcap_tx(void *pkt, int len)
{
	if (tx_handler)
		tx_handler(pkt, len);
}

static int pcap_append(int fd, void *pkt, int len)
{
	int err;
	pcaprec_hdr_t pcaphdr;
	u64 now = get_time_ns();
	u64 now_sec = now;
	u32 now_usec = do_div(now_sec, SECOND);

	pcaphdr.ts_sec = now_sec;
	pcaphdr.ts_usec = now_usec;
	pcaphdr.incl_len = len;
	pcaphdr.orig_len = len;

	err = write_full(fd, &pcaphdr, sizeof(pcaphdr));
	if (err < 0)
		return err;

	err = write_full(fd, pkt, len);
	if (err < 0)
		return err;

	return 0;
}

static const char *__ipproto_str(u8 proto)
{
	static char hexproto[] = "0x00";

	switch (proto) {
	case IPPROTO_ICMP:
		return "icmp";
	case IPPROTO_UDP:
		return "udp";
	case IPPROTO_TCP:
		return "tcp";
	default:
		sprintf(hexproto, "%#2.2x", proto);
		return hexproto;
	}
}

static const char *__ethertype_str(u16 ethertype)
{
	static char hextype[] = "0x0000";

	switch (ethertype) {
	case PROT_ARP:
		return "arp";
	case PROT_IP:
		return "ip4";
	case PROT_VLAN:
		return ".1q";
	default:
		sprintf(hextype, "%#4.4x", ethertype);
		return hextype;
	}
}

static void __push(char **ptr, size_t *len, size_t n)
{
	*ptr += n;
	*len -= n;
}

void pcap_snprint_bootp(char *buf, size_t size, char *pkt, size_t len)
{
	struct bootp *bp = (void *)pkt;

	if (len < sizeof(*bp)) {
		snprintf(buf, size, ", trunkated bootp");
		return;
	}

	switch (bp->bp_op) {
	case BOOTP_OP_REQUEST:
		snprintf(buf, size, ", bootp request %pI", &bp->bp_yiaddr);
		break;
	case BOOTP_OP_REPLY:
		snprintf(buf, size, ", bootp reply %pI", &bp->bp_yiaddr);
		break;
	default:
		snprintf(buf, size, ", bootp op:%#2.2x", bp->bp_op);
	}
}

void pcap_snprint_udp(char *buf, size_t size, char *pkt, size_t len)
{
	struct udphdr *udp = (void *)pkt;
	u16 sp = ntohs(udp->uh_sport), dp = ntohs(udp->uh_dport);


	if (len < sizeof(*udp)) {
		snprintf(buf, size, ", trunkated udp");
		return;
	}

	if ((sp == 67 && dp == 68) || (sp == 68 && dp == 67)) {
		__push(&pkt, &len, sizeof(*udp));
		pcap_snprint_bootp(buf, size, pkt, len);
		return;
	}


	snprintf(buf, size, ", len:%u(%#x)",
		 ntohs(udp->uh_ulen), ntohs(udp->uh_ulen));
}

void pcap_snprint_icmp(char *buf, size_t size, char *pkt, size_t len)
{
	struct icmphdr *icmp = (void *)pkt;

	if (len < sizeof(*icmp)) {
		snprintf(buf, size, ", trunkated icmp");
		return;
	}

	switch (icmp->type) {
	case ICMP_ECHO_REQUEST:
	case ICMP_ECHO_REPLY:
		snprintf(buf, size, ", %s id:%u seq:%u",
			 (icmp->type == ICMP_ECHO_REQUEST)? "request" : "reply",
			 ntohs(icmp->un.echo.id), ntohs(icmp->un.echo.sequence));
		break;

	default:
		snprintf(buf, size, ", type:%#2.2x code:%#2.2x",
			 icmp->type, icmp->code);
	}
}

void pcap_snprint_ip4(char *buf, size_t size, char *pkt, size_t len)
{
	struct iphdr *ip = (void *)pkt;
	size_t n;

	if (len < sizeof(*ip)) {
		snprintf(buf, size, ", trunkated ip4");
		return;
	}

	if (ip->protocol == IPPROTO_UDP || ip->protocol == IPPROTO_TCP) {
		u16 *ports = (void *)(&ip[1]);
		u16 sport = ntohs(ports[0]), dport = ntohs(ports[1]);

		n = snprintf(buf, size, ", %pI:%d > %pI:%d %s",
			     &ip->saddr, sport, &ip->daddr, dport,
			     __ipproto_str(ip->protocol));
	} else
		n = snprintf(buf, size, ", %pI > %pI %s", &ip->saddr, &ip->daddr,
			     __ipproto_str(ip->protocol));

	__push(&buf, &size, n);
	__push(&pkt, &len, sizeof(*ip));

	switch (ip->protocol) {
	case IPPROTO_ICMP:
		pcap_snprint_icmp(buf, size, pkt, len);
		break;
	case IPPROTO_UDP:
		pcap_snprint_udp(buf, size, pkt, len);
		break;
	}
}

void pcap_snprint_arp(char *buf, size_t size, char *pkt, size_t len)
{
	struct arprequest *arp = (void *)pkt;
	struct arprequest arph;
	char *sa, *da, *src, *dst;

	if (len < sizeof(*arp)) {
		snprintf(buf, size, ", trunkated arp");
		return;
	}

	arph.ar_hrd = ntohs(arp->ar_hrd);
	arph.ar_pro = ntohs(arp->ar_pro);
	arph.ar_op = ntohs(arp->ar_op);

	if (arph.ar_hrd != ARP_ETHER || arph.ar_pro != PROT_IP) {
		snprintf(buf, size, ", hw:%#4.4x prot:%#4.4x op:%#4.4x",
			 arph.ar_hrd, arph.ar_pro, arph.ar_op);
		return;
	}

	sa  = &arp->ar_data[ 0];
	src = &arp->ar_data[ 6];
	da  = &arp->ar_data[10];
	dst = &arp->ar_data[16];

	switch (arph.ar_op) {
	case ARPOP_REQUEST:
		snprintf(buf, size, ", request from:%pI for:%pI", src, dst);
		break;
	case ARPOP_REPLY:
		snprintf(buf, size, ", response from:%pI at:%pM", src, sa);
		break;

	default:
		snprintf(buf, size, ", op:%#4.4x src:%pI(%pM) dst:%pI(%pM)",
			 arph.ar_op, src, sa, dst, da);
	}
}

void pcap_snprint(char *buf, size_t size, void *vpkt, size_t len, int flags)
{
	char *pkt = vpkt;
	size_t n;
	struct ethernet *et = (struct ethernet *)pkt;
	u16 ethertype = ntohs(et->et_protlen);
	u8 *sa = et->et_src, *da = et->et_dest;

	if (len < sizeof(*et)) {
		snprintf(buf, size, "truncated ethernet");
		return;
	}

	if (flags & (1 << PCAP_OPT_L2)) {
		n = snprintf(buf, size, "%pM > %pM ", sa, da);
		__push(&buf, &size, n);
	}

	n = snprintf(buf, size, __ethertype_str(ethertype));
	__push(&buf, &size, n);
	__push(&pkt, &len, sizeof(*et));

	if (ethertype == PROT_VLAN) {
		u16 tci = ntohs(*((u16 *)pkt));

		ethertype = ntohs(*((u16 *)(pkt + 2)));

		if (len < 2) {
			snprintf(buf, size, ", truncated vlan");
			return;
		}

		n = snprintf(buf, size, ", vlan%d", tci & 0xfff);
		__push(&buf, &size, n);
		__push(&pkt, &len, 4);
	}

	switch (ethertype) {
	case PROT_ARP:
		pcap_snprint_arp(buf, size, pkt, len);
		break;
	case PROT_IP:
		pcap_snprint_ip4(buf, size, pkt, len);
		break;
	}
}

void pcap_print(const char *prefix, void *pkt, int len, int flags)
{
	char *pktstr = malloc(0x400);

	if (!pktstr) {
		pr_err("out of memory\n");
		return;
	}

	pcap_snprint(pktstr, 0x400, pkt, len, flags);
	pr_info("%s%s\n", prefix, pktstr);

	if (flags & (1 << PCAP_OPT_HEX))
		memory_display(pkt, 0, len, 1, 0);

	free(pktstr);
}

static void pcap_write(int *fd, void *pkt, int len)
{
	int err;

	if (*fd < 0)
		return;

	err = pcap_append(*fd, pkt, len);
	if (err) {
		close(*fd);
		*fd = -1;
	}
}

/* #if IS_ENABLED(CONFIG_NET_CMD_PCAP) */

static int pcap_flags = 0;
static int wfd = -1;

static void pcap_rx(void *pkt, int len)
{
	if (!(pcap_flags & (1 << PCAP_OPT_QUIET)))
		pcap_print("rx ", pkt, len, pcap_flags);

	if (pcap_flags & (1 << PCAP_OPT_WRITE))
		pcap_write(&wfd, pkt, len);
}

static void pcap_tx(void *pkt, int len)
{
	if (!(pcap_flags & (1 << PCAP_OPT_QUIET)))
		pcap_print("tx ", pkt, len, pcap_flags);

	if (pcap_flags & (1 << PCAP_OPT_WRITE))
		pcap_write(&wfd, pkt, len);
}

static int pcap_start_bg(int flags, const char *file)
{
	if (rx_handler || tx_handler)
		return -EBUSY;

	if (file && wfd < 0) {
		int err;

		wfd = open(file, O_CREAT | O_WRONLY);
		if (wfd < 0)
			return wfd;

		err = write_full(wfd, (void *)&pcap_hdr, sizeof(pcap_hdr));
		if (err < 0) {
			close(wfd);
			wfd = -1;
			return err;
		}
	}

	pcap_flags = flags;
	rx_handler = pcap_rx;
	tx_handler = pcap_tx;
	return 0;
}

static int pcap_stop(void)
{
	tx_handler = NULL;
	rx_handler = NULL;
	pcap_flags = 0;

	if (wfd >= 0) {
		close(wfd);
		wfd = -1;
	}
	return 0;
}

static int pcap_start(int flags, const char *file)
{
	int err;

	err = pcap_start_bg(flags, file);
	if (err)
		return err;

	while (1) {
		net_poll();

		if (ctrlc())
			break;

		mdelay(10);
	}

	return pcap_stop();
}

static int pcap_replay(int flags, const char *file)
{
	size_t size;
	void *pktdata = read_file(file, &size);
	pcap_hdr_t *hdr = pktdata;
	pcaprec_hdr_t *rec;
	int err = 0;

	if (!hdr)
		return -EIO;

	if (size < sizeof(*hdr) || hdr->magic_number != PCAP_MAGIC) {
		err = -EINVAL;
		goto out;
	}

	__push((char **)&hdr, &size, sizeof(*hdr));

	for (rec = (void *)hdr; size >= sizeof(*rec);) {
		pcap_print("rp ", rec->data, rec->incl_len, flags);
		__push((char **)&rec, &size, sizeof(*rec) + rec->incl_len);
	}

out:
	free(pktdata);
	return err;
}

static const int valid_mask =
	((1 << PCAP_OPT_BG) | (1 << PCAP_OPT_STOP) |
	 (1 << PCAP_OPT_REPLAY) | (1 << PCAP_OPT_WRITE));

static const int valid[] = {
	((1 << PCAP_OPT_STOP)),
	((1 << PCAP_OPT_REPLAY)),
	((1 << PCAP_OPT_BG)),
	((1 << PCAP_OPT_BG) | (1 << PCAP_OPT_WRITE)),
	((1 << PCAP_OPT_WRITE)),
	((0)),

	-1
};

static int do_pcap(int argc, char *argv[])
{
	char *rfile = NULL, *wfile = NULL;
	int flags = 0;
	int i, opt;

	while ((opt = getopt(argc, argv, "2bqr:sw:x")) > 0) {
		switch (opt) {
		case '2':
			flags |= (1 << PCAP_OPT_L2);
			break;
		case 'b':
			flags |= (1 << PCAP_OPT_BG);
			break;
		case 'q':
			flags |= (1 << PCAP_OPT_QUIET);
			break;
		case 'r':
			flags |= (1 << PCAP_OPT_REPLAY);
			rfile = optarg;
			break;
		case 's':
			flags |= (1 << PCAP_OPT_STOP);
			break;
		case 'w':
			flags |= (1 << PCAP_OPT_WRITE);
			wfile = optarg;
			break;
		case 'x':
			flags |= (1 << PCAP_OPT_HEX);
			break;
		}
	}

	for (i = 0; valid[i] >= 0; i++)
		if ((flags & valid_mask) == (valid[i] & valid_mask))
			break;

	if (valid[i] < 0)
		return COMMAND_ERROR_USAGE;


	if (flags & (1 << PCAP_OPT_STOP))
		return pcap_stop();

	if (rfile)
		return pcap_replay(flags, rfile);

	if (flags & (1 << PCAP_OPT_BG))
		return pcap_start_bg(flags, wfile);

	return pcap_start(flags, wfile);
}

BAREBOX_CMD_HELP_START(pcap)
BAREBOX_CMD_HELP_TEXT("\n"
		       "pcap [-2qx] [-w FILE]     Live capture\n"
		       "pcap [-2qx] [-w FILE] -b  Start background capture\n"
		       "pcap [-2x]  -r FILE       Replay capture\n"
		       "pcap -s                   Stop background capture\n\n")
BAREBOX_CMD_HELP_OPT  ("-2",  "Include L2 in output\n")
BAREBOX_CMD_HELP_OPT  ("-b",  "Run in the background\n")
BAREBOX_CMD_HELP_OPT  ("-q",  "Quiet, disable console output\n")
BAREBOX_CMD_HELP_OPT  ("-r FILE",  "Replay capture from file\n")
BAREBOX_CMD_HELP_OPT  ("-w FILE",  "Write capture to file\n")
BAREBOX_CMD_HELP_OPT  ("-x",  "Include hexdump of packet\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(pcap)
	.cmd		= do_pcap,
	BAREBOX_CMD_DESC("Ethernet packet capture")
	BAREBOX_CMD_HELP(cmd_pcap_help)
BAREBOX_CMD_END

/* #endif	/\* CONFIG_NET_CMD_PCAP *\/ */
