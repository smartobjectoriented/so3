/*
 * Original driver for u-boot:
 *      Copyright (C)  2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * SO3 Port:
 *      Copyright (C) 2020 Julien Quartier <julien.quartier@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef _SMC911X_H_
#define _SMC911X_H_
#define DEBUG

#define DRIVERNAME "smc911x"

#define CONFIG_SMC911X_32_BIT

#if defined (CONFIG_SMC911X_32_BIT) && \
	defined (CONFIG_SMC911X_16_BIT)
#error "SMC911X: Only one of CONFIG_SMC911X_32_BIT and \
	CONFIG_SMC911X_16_BIT shall be set"
#endif

#if defined (CONFIG_SMC911X_32_BIT)
static inline u32 __smc911x_reg_read(eth_dev_t *dev, u32 offset)
{
	return *(volatile u32*)(dev->iobase + offset);
}
u32 smc911x_reg_read(eth_dev_t *dev, u32 offset)
	__attribute__((weak, alias("__smc911x_reg_read")));

static inline void __smc911x_reg_write(eth_dev_t *dev,
					u32 offset, u32 val)
{
	*(volatile u32*)(dev->iobase + offset) = val;
}
void smc911x_reg_write(eth_dev_t *dev, u32 offset, u32 val)
	__attribute__((weak, alias("__smc911x_reg_write")));
#elif defined (CONFIG_SMC911X_16_BIT)
static inline u32 smc911x_reg_read(eth_dev_t *dev, u32 offset)
{
	volatile u16 *addr_16 = (u16 *)(dev->iobase + offset);
	return ((*addr_16 & 0x0000ffff) | (*(addr_16 + 1) << 16));
}
static inline void smc911x_reg_write(eth_dev_t *dev,
					u32 offset, u32 val)
{
	*(volatile u16 *)(dev->iobase + offset) = (u16)val;
	*(volatile u16 *)(dev->iobase + offset + 2) = (u16)(val >> 16);
}
#else
#error "SMC911X: undefined bus width"
#endif /* CONFIG_SMC911X_16_BIT */

/* Below are the register offsets and bit definitions
 * of the Lan911x memory space
 */
#define RX_DATA_FIFO		 		0x00

#define TX_DATA_FIFO		 		0x20
#define	TX_CMD_A_INT_ON_COMP			0x80000000
#define	TX_CMD_A_INT_BUF_END_ALGN		0x03000000
#define	TX_CMD_A_INT_4_BYTE_ALGN		0x00000000
#define	TX_CMD_A_INT_16_BYTE_ALGN		0x01000000
#define	TX_CMD_A_INT_32_BYTE_ALGN		0x02000000
#define	TX_CMD_A_INT_DATA_OFFSET		0x001F0000
#define	TX_CMD_A_INT_FIRST_SEG			0x00002000
#define	TX_CMD_A_INT_LAST_SEG			0x00001000
#define	TX_CMD_A_BUF_SIZE			0x000007FF
#define	TX_CMD_B_PKT_TAG			0xFFFF0000
#define	TX_CMD_B_ADD_CRC_DISABLE		0x00002000
#define	TX_CMD_B_DISABLE_PADDING		0x00001000
#define	TX_CMD_B_PKT_BYTE_LENGTH		0x000007FF

#define RX_STATUS_FIFO				0x40
#define	RX_STS_PKT_LEN				0x3FFF0000
#define	RX_STS_ES				0x00008000
#define	RX_STS_BCST				0x00002000
#define	RX_STS_LEN_ERR				0x00001000
#define	RX_STS_RUNT_ERR				0x00000800
#define	RX_STS_MCAST				0x00000400
#define	RX_STS_TOO_LONG				0x00000080
#define	RX_STS_COLL				0x00000040
#define	RX_STS_ETH_TYPE				0x00000020
#define	RX_STS_WDOG_TMT				0x00000010
#define	RX_STS_MII_ERR				0x00000008
#define	RX_STS_DRIBBLING			0x00000004
#define	RX_STS_CRC_ERR				0x00000002
#define RX_STATUS_FIFO_PEEK			0x44
#define TX_STATUS_FIFO				0x48
#define	TX_STS_TAG				0xFFFF0000
#define	TX_STS_ES				0x00008000
#define	TX_STS_LOC				0x00000800
#define	TX_STS_NO_CARR				0x00000400
#define	TX_STS_LATE_COLL			0x00000200
#define	TX_STS_MANY_COLL			0x00000100
#define	TX_STS_COLL_CNT				0x00000078
#define	TX_STS_MANY_DEFER			0x00000004
#define	TX_STS_UNDERRUN				0x00000002
#define	TX_STS_DEFERRED				0x00000001
#define TX_STATUS_FIFO_PEEK			0x4C
#define ID_REV					0x50
#define	ID_REV_CHIP_ID				0xFFFF0000  /* RO */
#define	ID_REV_REV_ID				0x0000FFFF  /* RO */

#define INT_CFG					0x54
#define	INT_CFG_INT_DEAS			0xFF000000  /* R/W */
#define	INT_CFG_INT_DEAS_CLR			0x00004000
#define	INT_CFG_INT_DEAS_STS			0x00002000
#define	INT_CFG_IRQ_INT				0x00001000  /* RO */
#define	INT_CFG_IRQ_EN				0x00000100  /* R/W */
					/* R/W Not Affected by SW Reset */
#define	INT_CFG_IRQ_POL				0x00000010
					/* R/W Not Affected by SW Reset */
#define	INT_CFG_IRQ_TYPE			0x00000001

#define INT_STS					0x58
#define	INT_STS_SW_INT				0x80000000  /* R/WC */
#define	INT_STS_TXSTOP_INT			0x02000000  /* R/WC */
#define	INT_STS_RXSTOP_INT			0x01000000  /* R/WC */
#define	INT_STS_RXDFH_INT			0x00800000  /* R/WC */
#define	INT_STS_RXDF_INT			0x00400000  /* R/WC */
#define	INT_STS_TX_IOC				0x00200000  /* R/WC */
#define	INT_STS_RXD_INT				0x00100000  /* R/WC */
#define	INT_STS_GPT_INT				0x00080000  /* R/WC */
#define	INT_STS_PHY_INT				0x00040000  /* RO */
#define	INT_STS_PME_INT				0x00020000  /* R/WC */
#define	INT_STS_TXSO				0x00010000  /* R/WC */
#define	INT_STS_RWT				0x00008000  /* R/WC */
#define	INT_STS_RXE				0x00004000  /* R/WC */
#define	INT_STS_TXE				0x00002000  /* R/WC */
/*#define	INT_STS_ERX		0x00001000*/  /* R/WC */
#define	INT_STS_TDFU				0x00000800  /* R/WC */
#define	INT_STS_TDFO				0x00000400  /* R/WC */
#define	INT_STS_TDFA				0x00000200  /* R/WC */
#define	INT_STS_TSFF				0x00000100  /* R/WC */
#define	INT_STS_TSFL				0x00000080  /* R/WC */
/*#define	INT_STS_RXDF		0x00000040*/  /* R/WC */
#define	INT_STS_RDFO				0x00000040  /* R/WC */
#define	INT_STS_RDFL				0x00000020  /* R/WC */
#define	INT_STS_RSFF				0x00000010  /* R/WC */
#define	INT_STS_RSFL				0x00000008  /* R/WC */
#define	INT_STS_GPIO2_INT			0x00000004  /* R/WC */
#define	INT_STS_GPIO1_INT			0x00000002  /* R/WC */
#define	INT_STS_GPIO0_INT			0x00000001  /* R/WC */
#define INT_EN					0x5C
#define	INT_EN_SW_INT_EN			0x80000000  /* R/W */
#define	INT_EN_TXSTOP_INT_EN			0x02000000  /* R/W */
#define	INT_EN_RXSTOP_INT_EN			0x01000000  /* R/W */
#define	INT_EN_RXDFH_INT_EN			0x00800000  /* R/W */
/*#define	INT_EN_RXDF_INT_EN		0x00400000*/  /* R/W */
#define	INT_EN_TIOC_INT_EN			0x00200000  /* R/W */
#define	INT_EN_RXD_INT_EN			0x00100000  /* R/W */
#define	INT_EN_GPT_INT_EN			0x00080000  /* R/W */
#define	INT_EN_PHY_INT_EN			0x00040000  /* R/W */
#define	INT_EN_PME_INT_EN			0x00020000  /* R/W */
#define	INT_EN_TXSO_EN				0x00010000  /* R/W */
#define	INT_EN_RWT_EN				0x00008000  /* R/W */
#define	INT_EN_RXE_EN				0x00004000  /* R/W */
#define	INT_EN_TXE_EN				0x00002000  /* R/W */
/*#define	INT_EN_ERX_EN			0x00001000*/  /* R/W */
#define	INT_EN_TDFU_EN				0x00000800  /* R/W */
#define	INT_EN_TDFO_EN				0x00000400  /* R/W */
#define	INT_EN_TDFA_EN				0x00000200  /* R/W */
#define	INT_EN_TSFF_EN				0x00000100  /* R/W */
#define	INT_EN_TSFL_EN				0x00000080  /* R/W */
/*#define	INT_EN_RXDF_EN			0x00000040*/  /* R/W */
#define	INT_EN_RDFO_EN				0x00000040  /* R/W */
#define	INT_EN_RDFL_EN				0x00000020  /* R/W */
#define	INT_EN_RSFF_EN				0x00000010  /* R/W */
#define	INT_EN_RSFL_EN				0x00000008  /* R/W */
#define	INT_EN_GPIO2_INT			0x00000004  /* R/W */
#define	INT_EN_GPIO1_INT			0x00000002  /* R/W */
#define	INT_EN_GPIO0_INT			0x00000001  /* R/W */

#define BYTE_TEST				0x64
#define FIFO_INT				0x68
#define	FIFO_INT_TX_AVAIL_LEVEL			0xFF000000  /* R/W */
#define	FIFO_INT_TX_STS_LEVEL			0x00FF0000  /* R/W */
#define	FIFO_INT_RX_AVAIL_LEVEL			0x0000FF00  /* R/W */
#define	FIFO_INT_RX_STS_LEVEL			0x000000FF  /* R/W */

#define RX_CFG					0x6C
#define	RX_CFG_RX_END_ALGN			0xC0000000  /* R/W */
#define		RX_CFG_RX_END_ALGN4		0x00000000  /* R/W */
#define		RX_CFG_RX_END_ALGN16		0x40000000  /* R/W */
#define		RX_CFG_RX_END_ALGN32		0x80000000  /* R/W */
#define	RX_CFG_RX_DMA_CNT			0x0FFF0000  /* R/W */
#define	RX_CFG_RX_DUMP				0x00008000  /* R/W */
#define	RX_CFG_RXDOFF				0x00001F00  /* R/W */
/*#define	RX_CFG_RXBAD			0x00000001*/  /* R/W */

#define TX_CFG					0x70
/*#define	TX_CFG_TX_DMA_LVL		0xE0000000*/	 /* R/W */
						 /* R/W Self Clearing */
/*#define	TX_CFG_TX_DMA_CNT		0x0FFF0000*/
#define	TX_CFG_TXS_DUMP				0x00008000  /* Self Clearing */
#define	TX_CFG_TXD_DUMP				0x00004000  /* Self Clearing */
#define	TX_CFG_TXSAO				0x00000004  /* R/W */
#define	TX_CFG_TX_ON				0x00000002  /* R/W */
#define	TX_CFG_STOP_TX				0x00000001  /* Self Clearing */

#define HW_CFG					0x74
#define	HW_CFG_TTM				0x00200000  /* R/W */
#define	HW_CFG_SF				0x00100000  /* R/W */
#define	HW_CFG_TX_FIF_SZ			0x000F0000  /* R/W */
#define	HW_CFG_TR				0x00003000  /* R/W */
#define	HW_CFG_PHY_CLK_SEL			0x00000060  /* R/W */
#define	HW_CFG_PHY_CLK_SEL_INT_PHY		0x00000000 /* R/W */
#define	HW_CFG_PHY_CLK_SEL_EXT_PHY		0x00000020 /* R/W */
#define	HW_CFG_PHY_CLK_SEL_CLK_DIS		0x00000040 /* R/W */
#define	HW_CFG_SMI_SEL				0x00000010  /* R/W */
#define	HW_CFG_EXT_PHY_DET			0x00000008  /* RO */
#define	HW_CFG_EXT_PHY_EN			0x00000004  /* R/W */
#define	HW_CFG_32_16_BIT_MODE			0x00000004  /* RO */
#define	HW_CFG_SRST_TO				0x00000002  /* RO */
#define	HW_CFG_SRST				0x00000001  /* Self Clearing */

#define RX_DP_CTRL				0x78
#define	RX_DP_CTRL_RX_FFWD			0x80000000  /* R/W */
#define	RX_DP_CTRL_FFWD_BUSY			0x80000000  /* RO */

#define RX_FIFO_INF				0x7C
#define	 RX_FIFO_INF_RXSUSED			0x00FF0000  /* RO */
#define	 RX_FIFO_INF_RXDUSED			0x0000FFFF  /* RO */

#define TX_FIFO_INF				0x80
#define	TX_FIFO_INF_TSUSED			0x00FF0000  /* RO */
#define	TX_FIFO_INF_TDFREE			0x0000FFFF  /* RO */

#define PMT_CTRL				0x84
#define	PMT_CTRL_PM_MODE			0x00003000  /* Self Clearing */
#define	PMT_CTRL_PHY_RST			0x00000400  /* Self Clearing */
#define	PMT_CTRL_WOL_EN				0x00000200  /* R/W */
#define	PMT_CTRL_ED_EN				0x00000100  /* R/W */
					/* R/W Not Affected by SW Reset */
#define	PMT_CTRL_PME_TYPE			0x00000040
#define	PMT_CTRL_WUPS				0x00000030  /* R/WC */
#define	PMT_CTRL_WUPS_NOWAKE			0x00000000  /* R/WC */
#define	PMT_CTRL_WUPS_ED			0x00000010  /* R/WC */
#define	PMT_CTRL_WUPS_WOL			0x00000020  /* R/WC */
#define	PMT_CTRL_WUPS_MULTI			0x00000030  /* R/WC */
#define	PMT_CTRL_PME_IND			0x00000008  /* R/W */
#define	PMT_CTRL_PME_POL			0x00000004  /* R/W */
					/* R/W Not Affected by SW Reset */
#define	PMT_CTRL_PME_EN				0x00000002
#define	PMT_CTRL_READY				0x00000001  /* RO */

#define GPIO_CFG				0x88
#define	GPIO_CFG_LED3_EN			0x40000000  /* R/W */
#define	GPIO_CFG_LED2_EN			0x20000000  /* R/W */
#define	GPIO_CFG_LED1_EN			0x10000000  /* R/W */
#define	GPIO_CFG_GPIO2_INT_POL			0x04000000  /* R/W */
#define	GPIO_CFG_GPIO1_INT_POL			0x02000000  /* R/W */
#define	GPIO_CFG_GPIO0_INT_POL			0x01000000  /* R/W */
#define	GPIO_CFG_EEPR_EN			0x00700000  /* R/W */
#define	GPIO_CFG_GPIOBUF2			0x00040000  /* R/W */
#define	GPIO_CFG_GPIOBUF1			0x00020000  /* R/W */
#define	GPIO_CFG_GPIOBUF0			0x00010000  /* R/W */
#define	GPIO_CFG_GPIODIR2			0x00000400  /* R/W */
#define	GPIO_CFG_GPIODIR1			0x00000200  /* R/W */
#define	GPIO_CFG_GPIODIR0			0x00000100  /* R/W */
#define	GPIO_CFG_GPIOD4				0x00000010  /* R/W */
#define	GPIO_CFG_GPIOD3				0x00000008  /* R/W */
#define	GPIO_CFG_GPIOD2				0x00000004  /* R/W */
#define	GPIO_CFG_GPIOD1				0x00000002  /* R/W */
#define	GPIO_CFG_GPIOD0				0x00000001  /* R/W */

#define GPT_CFG					0x8C
#define	GPT_CFG_TIMER_EN			0x20000000  /* R/W */
#define	GPT_CFG_GPT_LOAD			0x0000FFFF  /* R/W */

#define GPT_CNT					0x90
#define	GPT_CNT_GPT_CNT				0x0000FFFF  /* RO */

#define ENDIAN					0x98
#define FREE_RUN				0x9C
#define RX_DROP					0xA0
#define MAC_CSR_CMD				0xA4
#define	 MAC_CSR_CMD_CSR_BUSY			0x80000000  /* Self Clearing */
#define	 MAC_CSR_CMD_R_NOT_W			0x40000000  /* R/W */
#define	 MAC_CSR_CMD_CSR_ADDR			0x000000FF  /* R/W */

#define MAC_CSR_DATA				0xA8
#define AFC_CFG					0xAC
#define		AFC_CFG_AFC_HI			0x00FF0000  /* R/W */
#define		AFC_CFG_AFC_LO			0x0000FF00  /* R/W */
#define		AFC_CFG_BACK_DUR		0x000000F0  /* R/W */
#define		AFC_CFG_FCMULT			0x00000008  /* R/W */
#define		AFC_CFG_FCBRD			0x00000004  /* R/W */
#define		AFC_CFG_FCADD			0x00000002  /* R/W */
#define		AFC_CFG_FCANY			0x00000001  /* R/W */

#define E2P_CMD					0xB0
#define		E2P_CMD_EPC_BUSY		0x80000000  /* Self Clearing */
#define		E2P_CMD_EPC_CMD			0x70000000  /* R/W */
#define		E2P_CMD_EPC_CMD_READ		0x00000000  /* R/W */
#define		E2P_CMD_EPC_CMD_EWDS		0x10000000  /* R/W */
#define		E2P_CMD_EPC_CMD_EWEN		0x20000000  /* R/W */
#define		E2P_CMD_EPC_CMD_WRITE		0x30000000  /* R/W */
#define		E2P_CMD_EPC_CMD_WRAL		0x40000000  /* R/W */
#define		E2P_CMD_EPC_CMD_ERASE		0x50000000  /* R/W */
#define		E2P_CMD_EPC_CMD_ERAL		0x60000000  /* R/W */
#define		E2P_CMD_EPC_CMD_RELOAD		0x70000000  /* R/W */
#define		E2P_CMD_EPC_TIMEOUT		0x00000200  /* RO */
#define		E2P_CMD_MAC_ADDR_LOADED		0x00000100  /* RO */
#define		E2P_CMD_EPC_ADDR		0x000000FF  /* R/W */

#define E2P_DATA				0xB4
#define	E2P_DATA_EEPROM_DATA			0x000000FF  /* R/W */
/* end of LAN register offsets and bit definitions */

/* MAC Control and Status registers */
#define MAC_CR			0x01  /* R/W */

/* MAC_CR - MAC Control Register */
#define MAC_CR_RXALL			0x80000000
#define MAC_CR_HBDIS			0x10000000
#define MAC_CR_RCVOWN			0x00800000
#define MAC_CR_LOOPBK			0x00200000
#define MAC_CR_FDPX			0x00100000
#define MAC_CR_MCPAS			0x00080000
#define MAC_CR_PRMS			0x00040000
#define MAC_CR_INVFILT			0x00020000
#define MAC_CR_PASSBAD			0x00010000
#define MAC_CR_HFILT			0x00008000
#define MAC_CR_HPFILT			0x00002000
#define MAC_CR_LCOLL			0x00001000
#define MAC_CR_BCAST			0x00000800
#define MAC_CR_DISRTY			0x00000400
#define MAC_CR_PADSTR			0x00000100
#define MAC_CR_BOLMT_MASK		0x000000C0
#define MAC_CR_DFCHK			0x00000020
#define MAC_CR_TXEN			0x00000008
#define MAC_CR_RXEN			0x00000004

#define ADDRH			0x02	  /* R/W mask 0x0000FFFFUL */
#define ADDRL			0x03	  /* R/W mask 0xFFFFFFFFUL */
#define HASHH			0x04	  /* R/W */
#define HASHL			0x05	  /* R/W */

#define MII_ACC			0x06	  /* R/W */
#define MII_ACC_PHY_ADDR		0x0000F800
#define MII_ACC_MIIRINDA		0x000007C0
#define MII_ACC_MII_WRITE		0x00000002
#define MII_ACC_MII_BUSY		0x00000001

#define MII_DATA		0x07	  /* R/W mask 0x0000FFFFUL */

#define FLOW			0x08	  /* R/W */
#define FLOW_FCPT			0xFFFF0000
#define FLOW_FCPASS			0x00000004
#define FLOW_FCEN			0x00000002
#define FLOW_FCBSY			0x00000001

#define VLAN1			0x09	  /* R/W mask 0x0000FFFFUL */
#define VLAN1_VTI1			0x0000ffff

#define VLAN2			0x0A	  /* R/W mask 0x0000FFFFUL */
#define VLAN2_VTI2			0x0000ffff

#define WUFF			0x0B	  /* WO */

#define WUCSR			0x0C	  /* R/W */
#define WUCSR_GUE			0x00000200
#define WUCSR_WUFR			0x00000040
#define WUCSR_MPR			0x00000020
#define WUCSR_WAKE_EN			0x00000004
#define WUCSR_MPEN			0x00000002

/* Chip ID values */
#define CHIP_89218	0x218a
#define CHIP_9115	0x115
#define CHIP_9116	0x116
#define CHIP_9117	0x117
#define CHIP_9118	0x118
#define CHIP_9211	0x9211
#define CHIP_9215	0x115a
#define CHIP_9216	0x116a
#define CHIP_9217	0x117a
#define CHIP_9218	0x118a
#define CHIP_9220	0x9220
#define CHIP_9221	0x9221


/* PHY registers */
//#define MII_BMCR            0  /* Basic mode control register */
//#define MII_BMSR            1  /* Basic mode status register */
#define MII_PHYID1          2  /* ID register 1 */
#define MII_PHYID2          3  /* ID register 2 */
#define MII_ANAR            4  /* Autonegotiation advertisement */
#define MII_ANLPAR          5  /* Autonegotiation lnk partner abilities */
#define MII_ANER            6  /* Autonegotiation expansion */
#define MII_ANNP            7  /* Autonegotiation next page */
#define MII_ANLPRNP         8  /* Autonegotiation link partner rx next page */
//#define MII_CTRL1000        9  /* 1000BASE-T control */
//#define MII_STAT1000        10 /* 1000BASE-T status */
#define MII_MDDACR          13 /* MMD access control */
#define MII_MDDAADR         14 /* MMD access address data */
#define MII_EXTSTAT         15 /* Extended Status */
#define MII_NSR             16
#define MII_LBREMR          17
#define MII_REC             18
#define MII_SNRDR           19
#define MII_TEST            25

/* PHY registers fields */
#define MII_BMCR_RESET      (1 << 15)
#define MII_BMCR_LOOPBACK   (1 << 14)
#define MII_BMCR_SPEED100   (1 << 13)  /* LSB of Speed (100) */
#define MII_BMCR_SPEED      MII_BMCR_SPEED100
#define MII_BMCR_AUTOEN     (1 << 12) /* Autonegotiation enable */
#define MII_BMCR_PDOWN      (1 << 11) /* Enable low power state */
#define MII_BMCR_ISOLATE    (1 << 10) /* Isolate data paths from MII */
#define MII_BMCR_ANRESTART  (1 << 9)  /* Auto negotiation restart */
#define MII_BMCR_FD         (1 << 8)  /* Set duplex mode */
#define MII_BMCR_CTST       (1 << 7)  /* Collision test */
#define MII_BMCR_SPEED1000  (1 << 6)  /* MSB of Speed (1000) */

#define MII_BMSR_100TX_FD   (1 << 14) /* Can do 100mbps, full-duplex */
#define MII_BMSR_100TX_HD   (1 << 13) /* Can do 100mbps, half-duplex */
#define MII_BMSR_10T_FD     (1 << 12) /* Can do 10mbps, full-duplex */
#define MII_BMSR_10T_HD     (1 << 11) /* Can do 10mbps, half-duplex */
#define MII_BMSR_100T2_FD   (1 << 10) /* Can do 100mbps T2, full-duplex */
#define MII_BMSR_100T2_HD   (1 << 9)  /* Can do 100mbps T2, half-duplex */
#define MII_BMSR_EXTSTAT    (1 << 8)  /* Extended status in register 15 */
#define MII_BMSR_MFPS       (1 << 6)  /* MII Frame Preamble Suppression */
#define MII_BMSR_AN_COMP    (1 << 5)  /* Auto-negotiation complete */
#define MII_BMSR_RFAULT     (1 << 4)  /* Remote fault */
#define MII_BMSR_AUTONEG    (1 << 3)  /* Able to do auto-negotiation */
#define MII_BMSR_LINK_ST    (1 << 2)  /* Link status */
#define MII_BMSR_JABBER     (1 << 1)  /* Jabber detected */
#define MII_BMSR_EXTCAP     (1 << 0)  /* Ext-reg capability */

#define MII_ANAR_PAUSE_ASYM (1 << 11) /* Try for asymetric pause */
#define MII_ANAR_PAUSE      (1 << 10) /* Try for pause */
#define MII_ANAR_TXFD       (1 << 8)
#define MII_ANAR_TX         (1 << 7)
#define MII_ANAR_10FD       (1 << 6)
#define MII_ANAR_10         (1 << 5)
#define MII_ANAR_CSMACD     (1 << 0)

#define MII_ANLPAR_ACK      (1 << 14)
#define MII_ANLPAR_PAUSEASY (1 << 11) /* can pause asymmetrically */
#define MII_ANLPAR_PAUSE    (1 << 10) /* can pause */
#define MII_ANLPAR_TXFD     (1 << 8)
#define MII_ANLPAR_TX       (1 << 7)
#define MII_ANLPAR_10FD     (1 << 6)
#define MII_ANLPAR_10       (1 << 5)
#define MII_ANLPAR_CSMACD   (1 << 0)

#define MII_ANER_NWAY       (1 << 0) /* Can do N-way auto-nego */

#define MII_CTRL1000_FULL   (1 << 9)  /* 1000BASE-T full duplex */
#define MII_CTRL1000_HALF   (1 << 8)  /* 1000BASE-T half duplex */

#define MII_STAT1000_FULL   (1 << 11) /* 1000BASE-T full duplex */
#define MII_STAT1000_HALF   (1 << 10) /* 1000BASE-T half duplex */

/* Generic MII registers. */
#define MII_BMCR		0x00	/* Basic mode control register */
#define MII_BMSR		0x01	/* Basic mode status register  */
#define MII_PHYSID1		0x02	/* PHYS ID 1                   */
#define MII_PHYSID2		0x03	/* PHYS ID 2                   */
#define MII_ADVERTISE		0x04	/* Advertisement control reg   */
#define MII_LPA			0x05	/* Link partner ability reg    */
#define MII_EXPANSION		0x06	/* Expansion register          */
#define MII_CTRL1000		0x09	/* 1000BASE-T control          */
#define MII_STAT1000		0x0a	/* 1000BASE-T status           */
#define MII_MMD_CTRL		0x0d	/* MMD Access Control Register */
#define MII_MMD_DATA		0x0e	/* MMD Access Data Register */
#define MII_ESTATUS		0x0f	/* Extended Status             */
#define MII_DCOUNTER		0x12	/* Disconnect counter          */
#define MII_FCSCOUNTER		0x13	/* False carrier counter       */
#define MII_NWAYTEST		0x14	/* N-way auto-neg test reg     */
#define MII_RERRCOUNTER		0x15	/* Receive error counter       */
#define MII_SREVISION		0x16	/* Silicon revision            */
#define MII_RESV1		0x17	/* Reserved...                 */
#define MII_LBRERROR		0x18	/* Lpback, rx, bypass error    */
#define MII_PHYADDR		0x19	/* PHY address                 */
#define MII_RESV2		0x1a	/* Reserved...                 */
#define MII_TPISTATUS		0x1b	/* TPI status for 10mbps       */
#define MII_NCONFIG		0x1c	/* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV		0x003f	/* Unused...                   */
#define BMCR_SPEED1000		0x0040	/* MSB of Speed (1000)         */
#define BMCR_CTST		0x0080	/* Collision test              */
#define BMCR_FULLDPLX		0x0100	/* Full duplex                 */
#define BMCR_ANRESTART		0x0200	/* Auto negotiation restart    */
#define BMCR_ISOLATE		0x0400	/* Isolate data paths from MII */
#define BMCR_PDOWN		0x0800	/* Enable low power state      */
#define BMCR_ANENABLE		0x1000	/* Enable auto negotiation     */
#define BMCR_SPEED100		0x2000	/* Select 100Mbps              */
#define BMCR_LOOPBACK		0x4000	/* TXD loopback bits           */
#define BMCR_RESET		0x8000	/* Reset to default state      */
#define BMCR_SPEED10		0x0000	/* Select 10Mbps               */

/* Basic mode status register. */
#define BMSR_ERCAP		0x0001	/* Ext-reg capability          */
#define BMSR_JCD		0x0002	/* Jabber detected             */
#define BMSR_LSTATUS		0x0004	/* Link status                 */
#define BMSR_ANEGCAPABLE	0x0008	/* Able to do auto-negotiation */
#define BMSR_RFAULT		0x0010	/* Remote fault detected       */
#define BMSR_ANEGCOMPLETE	0x0020	/* Auto-negotiation complete   */
#define BMSR_RESV		0x00c0	/* Unused...                   */
#define BMSR_ESTATEN		0x0100	/* Extended Status in R15      */
#define BMSR_100HALF2		0x0200	/* Can do 100BASE-T2 HDX       */
#define BMSR_100FULL2		0x0400	/* Can do 100BASE-T2 FDX       */
#define BMSR_10HALF		0x0800	/* Can do 10mbps, half-duplex  */
#define BMSR_10FULL		0x1000	/* Can do 10mbps, full-duplex  */
#define BMSR_100HALF		0x2000	/* Can do 100mbps, half-duplex */
#define BMSR_100FULL		0x4000	/* Can do 100mbps, full-duplex */
#define BMSR_100BASE4		0x8000	/* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT		0x001f	/* Selector bits               */
#define ADVERTISE_CSMA		0x0001	/* Only selector supported     */
#define ADVERTISE_10HALF	0x0020	/* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL	0x0020	/* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL	0x0040	/* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF	0x0040	/* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF	0x0080	/* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE	0x0080	/* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL	0x0100	/* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM	0x0100	/* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4	0x0200	/* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP	0x0400	/* Try for pause               */
#define ADVERTISE_PAUSE_ASYM	0x0800	/* Try for asymetric pause     */
#define ADVERTISE_RESV		0x1000	/* Unused...                   */
#define ADVERTISE_RFAULT	0x2000	/* Say we can detect faults    */
#define ADVERTISE_LPACK		0x4000	/* Ack link partners response  */
#define ADVERTISE_NPAGE		0x8000	/* Next page bit               */

#define ADVERTISE_FULL		(ADVERTISE_100FULL | ADVERTISE_10FULL | \
				 ADVERTISE_CSMA)
#define ADVERTISE_ALL		(ADVERTISE_10HALF | ADVERTISE_10FULL | \
				 ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_SLCT		0x001f	/* Same as advertise selector  */
#define LPA_10HALF		0x0020	/* Can do 10mbps half-duplex   */
#define LPA_1000XFULL		0x0020	/* Can do 1000BASE-X full-duplex */
#define LPA_10FULL		0x0040	/* Can do 10mbps full-duplex   */
#define LPA_1000XHALF		0x0040	/* Can do 1000BASE-X half-duplex */
#define LPA_100HALF		0x0080	/* Can do 100mbps half-duplex  */
#define LPA_1000XPAUSE		0x0080	/* Can do 1000BASE-X pause     */
#define LPA_100FULL		0x0100	/* Can do 100mbps full-duplex  */
#define LPA_1000XPAUSE_ASYM	0x0100	/* Can do 1000BASE-X pause asym*/
#define LPA_100BASE4		0x0200	/* Can do 100mbps 4k packets   */
#define LPA_PAUSE_CAP		0x0400	/* Can pause                   */
#define LPA_PAUSE_ASYM		0x0800	/* Can pause asymetrically     */
#define LPA_RESV		0x1000	/* Unused...                   */
#define LPA_RFAULT		0x2000	/* Link partner faulted        */
#define LPA_LPACK		0x4000	/* Link partner acked us       */
#define LPA_NPAGE		0x8000	/* Next page bit               */

#define LPA_DUPLEX		(LPA_10FULL | LPA_100FULL)
#define LPA_100			(LPA_100FULL | LPA_100HALF | LPA_100BASE4)

/* Expansion register for auto-negotiation. */
#define EXPANSION_NWAY		0x0001	/* Can do N-way auto-nego      */
#define EXPANSION_LCWP		0x0002	/* Got new RX page code word   */
#define EXPANSION_ENABLENPAGE	0x0004	/* This enables npage words    */
#define EXPANSION_NPCAPABLE	0x0008	/* Link partner supports npage */
#define EXPANSION_MFAULTS	0x0010	/* Multiple faults detected    */
#define EXPANSION_RESV		0xffe0	/* Unused...                   */

#define ESTATUS_1000_XFULL	0x8000	/* Can do 1000BX Full */
#define ESTATUS_1000_XHALF	0x4000	/* Can do 1000BX Half */
#define ESTATUS_1000_TFULL	0x2000	/* Can do 1000BT Full          */
#define ESTATUS_1000_THALF	0x1000	/* Can do 1000BT Half          */

/* N-way test register. */
#define NWAYTEST_RESV1		0x00ff	/* Unused...                   */
#define NWAYTEST_LOOPBACK	0x0100	/* Enable loopback for N-way   */
#define NWAYTEST_RESV2		0xfe00	/* Unused...                   */

/* 1000BASE-T Control register */
#define ADVERTISE_1000FULL	0x0200  /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF	0x0100  /* Advertise 1000BASE-T half duplex */
#define CTL1000_AS_MASTER	0x0800
#define CTL1000_ENABLE_MASTER	0x1000

/* 1000BASE-T Status register */
#define LPA_1000LOCALRXOK	0x2000	/* Link partner local receiver status */
#define LPA_1000REMRXOK		0x1000	/* Link partner remote receiver status */
#define LPA_1000FULL		0x0800	/* Link partner 1000BASE-T full duplex */
#define LPA_1000HALF		0x0400	/* Link partner 1000BASE-T half duplex */

/* Flow control flags */
#define FLOW_CTRL_TX		0x01
#define FLOW_CTRL_RX		0x02

/* MMD Access Control register fields */
#define MII_MMD_CTRL_DEVAD_MASK	0x1f	/* Mask MMD DEVAD*/
#define MII_MMD_CTRL_ADDR	0x0000	/* Address */
#define MII_MMD_CTRL_NOINCR	0x4000	/* no post increment */
#define MII_MMD_CTRL_INCR_RDWT	0x8000	/* post increment on reads & writes */
#define MII_MMD_CTRL_INCR_ON_WT	0xC000	/* post increment on writes only */

struct chip_id {
	u16 id;
	char *name;
};

static const struct chip_id chip_ids[] =  {
	{ CHIP_89218, "LAN89218" },
	{ CHIP_9115, "LAN9115" },
	{ CHIP_9116, "LAN9116" },
	{ CHIP_9117, "LAN9117" },
	{ CHIP_9118, "LAN9118" },
	{ CHIP_9211, "LAN9211" },
	{ CHIP_9215, "LAN9215" },
	{ CHIP_9216, "LAN9216" },
	{ CHIP_9217, "LAN9217" },
	{ CHIP_9218, "LAN9218" },
	{ CHIP_9220, "LAN9220" },
	{ CHIP_9221, "LAN9221" },
	{ 0, NULL },
};

static u32 smc911x_get_mac_csr(eth_dev_t *dev, u8 reg)
{
	while (smc911x_reg_read(dev, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY)
		;
	smc911x_reg_write(dev, MAC_CSR_CMD,
			MAC_CSR_CMD_CSR_BUSY | MAC_CSR_CMD_R_NOT_W | reg);
	while (smc911x_reg_read(dev, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY)
		;

	return smc911x_reg_read(dev, MAC_CSR_DATA);
}

static void smc911x_set_mac_csr(eth_dev_t *dev, u8 reg, u32 data)
{
	while (smc911x_reg_read(dev, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY)
		;
	smc911x_reg_write(dev, MAC_CSR_DATA, data);
	smc911x_reg_write(dev, MAC_CSR_CMD, MAC_CSR_CMD_CSR_BUSY | reg);
	while (smc911x_reg_read(dev, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY)
		;
}

static int smc911x_detect_chip(eth_dev_t *dev)
{
	unsigned long val, i;

	val = smc911x_reg_read(dev, BYTE_TEST);
	if (val == 0xffffffff) {
		/* Special case -- no chip present */
		return -1;
	} else if (val != 0x87654321) {
		printk(DRIVERNAME ": Invalid chip endian 0x%08lx\n", val);
		return -1;
	}

	val = smc911x_reg_read(dev, ID_REV) >> 16;
	for (i = 0; chip_ids[i].id != 0; i++) {
		if (chip_ids[i].id == val) break;
	}
	if (!chip_ids[i].id) {
		printk(DRIVERNAME ": Unknown chip ID %04lx\n", val);
		return -1;
	}

	printk("Network Interface Controller (NIC) found %s", chip_ids[i].name);

	dev->priv = (void *)&chip_ids[i];

	return 0;
}

static void smc911x_reset(eth_dev_t *dev)
{
	int timeout, resets = 1;
	u32 reg;

	/*
	 *  Take out of PM setting first
	 *  Device is already wake up if PMT_CTRL_READY bit is set
	 */
	if ((smc911x_reg_read(dev, PMT_CTRL) & PMT_CTRL_READY) == 0) {
		/* Write to the bytetest will take out of powerdown */
		smc911x_reg_write(dev, BYTE_TEST, 0x0);

		timeout = 10;

		while (timeout-- &&
			!(smc911x_reg_read(dev, PMT_CTRL) & PMT_CTRL_READY))
			udelay(10);
		if (timeout < 0) {
			printk(DRIVERNAME
				": timeout waiting for PM restore\n");
			return;
		}
	}

	/* Disable interrupts */
	smc911x_reg_write(dev, INT_EN, 0);

	while(resets--){
        smc911x_reg_write(dev, HW_CFG, HW_CFG_SRST);
        timeout=10;
        do {
            udelay(10);
            reg = smc911x_reg_read(dev, HW_CFG);
            /* If chip indicates reset timeout then try again */
            if (reg & HW_CFG_SRST_TO) {
                printk("chip reset timeout, retrying...\n");
                resets++;
                break;
            }
        } while (--timeout && (reg & HW_CFG_SRST));
    }

    if (timeout == 0) {
        printk("smc911x_reset timeout waiting for reset\n");
        return;
    }

	timeout = 1000;
	while (timeout-- && smc911x_reg_read(dev, E2P_CMD) & E2P_CMD_EPC_BUSY)
		udelay(10);

	if (timeout < 0) {
		printk(DRIVERNAME ": reset timeout\n");
		return;
	}

    /* Initialize interrupts */
    smc911x_reg_write(dev, INT_EN, 0);
    smc911x_reg_write(dev, INT_STS, -1); // Clear all interrupts

	/* Reset the FIFO level and flow control settings */
	smc911x_set_mac_csr(dev, FLOW, FLOW_FCPT | FLOW_FCEN);
	smc911x_reg_write(dev, AFC_CFG, 0x0050287F);

	/* Set to LED outputs */
	smc911x_reg_write(dev, GPIO_CFG, 0x70070000);


    smc911x_reg_write(dev, INT_CFG, (0x1 << 24) | INT_CFG_IRQ_EN | INT_CFG_IRQ_POL | INT_CFG_IRQ_TYPE);

}

#endif
