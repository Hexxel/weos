#ifndef __MV88E6XXX_H
#define __MV88E6XXX_H

struct mv88e6xxx_pdata {
	int smi_addr;

	int cpu_speed;

	int cpu_port;
	int cascade_ports[2];
	int cascade_rgmii[2];

	struct phy_device *slaves[4];
};

#endif	/* __MV88E6XXX_H */
