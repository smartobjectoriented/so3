/*
 * linux/include/asm-arm/vfp.h
 *
 * VFP register definitions.
 * First, the standard VFP set.
 */

#ifndef VFP_H
#define VFP_H

#define FPEXC_EX                (1u << 31)
#define FPEXC_EN                (1u << 30)
#define FPEXC_FP2V              (1u << 28)

#define MVFR0_A_SIMD_MASK       (0xf << 0)

#define FPSID_IMPLEMENTER_BIT   (24)
#define FPSID_IMPLEMENTER_MASK  (0xff << FPSID_IMPLEMENTER_BIT)
#define FPSID_ARCH_BIT          (16)
#define FPSID_ARCH_MASK         (0xf << FPSID_ARCH_BIT)
#define FPSID_PART_BIT          (8)
#define FPSID_PART_MASK         (0xff << FPSID_PART_BIT)
#define FPSID_VARIANT_BIT       (4)
#define FPSID_VARIANT_MASK      (0xf << FPSID_VARIANT_BIT)
#define FPSID_REV_BIT           (0)
#define FPSID_REV_MASK          (0xf << FPSID_REV_BIT)

#ifndef __ASSEMBLY__
struct vfp_state {
  uint64_t fpregs1[16]; /* {d0-d15} */
	uint64_t fpregs2[16]; /* {d16-d31} */
	uint32_t fpexc;
	uint32_t fpscr;

	/* VFP implementation specific state */
	uint32_t fpinst;
	uint32_t fpinst2;
};

struct vcpu;
void vfp_save_state(struct domain *v);
void vfp_restore_state(struct domain *v);
void vfp_enable(void);


#endif /* __ASSEMBLY__ */



#endif /* VFP_H */
