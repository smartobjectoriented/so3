

#include <asm/mmu.h>

#define MAX_IOMMU_UNITS	8

#define PADDR_OFF		(5)

#define TTBR_MASK		BIT_MASK(47, PADDR_OFF)
#define VTTBR_VMID_SHIFT	48

#define T0SZ_CELL		T0SZ(48)
#define SL0_CELL		(SL0_L0)

#define VTCR_CELL		(T0SZ_CELL | (SL0_CELL << TCR_SL0_SHIFT)\
				| (TCR_RGN_WB_WA << TCR_IRGN0_SHIFT)	\
				| (TCR_RGN_WB_WA << TCR_ORGN0_SHIFT)	\
				| (TCR_INNER_SHAREABLE << TCR_SH0_SHIFT)\
				| (5 << TCR_PS_SHIFT)	\
				| VTCR_RES1)

/** Count number of pages for given size (round up). */
#define PAGES(s)		(((s) + PAGE_SIZE-1) / PAGE_SIZE)
