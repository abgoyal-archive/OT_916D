
#ifndef _MIPS_MALTAINT_H
#define _MIPS_MALTAINT_H

#include <irq.h>

#define MALTA_INT_BASE		0

/* CPU interrupt offsets */
#define MIPSCPU_INT_SW0		0
#define MIPSCPU_INT_SW1		1
#define MIPSCPU_INT_MB0		2
#define MIPSCPU_INT_I8259A	MIPSCPU_INT_MB0
#define MIPSCPU_INT_MB1		3
#define MIPSCPU_INT_SMI		MIPSCPU_INT_MB1
#define MIPSCPU_INT_IPI0	MIPSCPU_INT_MB1	/* GIC IPI */
#define MIPSCPU_INT_MB2		4
#define MIPSCPU_INT_IPI1	MIPSCPU_INT_MB2	/* GIC IPI */
#define MIPSCPU_INT_MB3		5
#define MIPSCPU_INT_COREHI	MIPSCPU_INT_MB3
#define MIPSCPU_INT_MB4		6
#define MIPSCPU_INT_CORELO	MIPSCPU_INT_MB4

#define MSC01C_INT_BASE		64

/* SOC-it Classic interrupt offsets */
#define MSC01C_INT_TMR		0
#define MSC01C_INT_PCI		1

#define MSC01E_INT_BASE		64

/* SOC-it EIC interrupt offsets */
#define MSC01E_INT_SW0		1
#define MSC01E_INT_SW1		2
#define MSC01E_INT_MB0		3
#define MSC01E_INT_I8259A	MSC01E_INT_MB0
#define MSC01E_INT_MB1		4
#define MSC01E_INT_SMI		MSC01E_INT_MB1
#define MSC01E_INT_MB2		5
#define MSC01E_INT_MB3		6
#define MSC01E_INT_COREHI	MSC01E_INT_MB3
#define MSC01E_INT_MB4		7
#define MSC01E_INT_CORELO	MSC01E_INT_MB4
#define MSC01E_INT_TMR		8
#define MSC01E_INT_PCI		9
#define MSC01E_INT_PERFCTR	10
#define MSC01E_INT_CPUCTR	11

/* GIC's Nomenclature for Core Interrupt Pins on the Malta */
#define GIC_CPU_INT0		0 /* Core Interrupt 2 	*/
#define GIC_CPU_INT1		1 /* .			*/
#define GIC_CPU_INT2		2 /* .			*/
#define GIC_CPU_INT3		3 /* .			*/
#define GIC_CPU_INT4		4 /* .			*/
#define GIC_CPU_INT5		5 /* Core Interrupt 5   */

#define GIC_EXT_INTR(x)		x

/* Dummy data */
#define X			0xdead

/* External Interrupts used for IPI */
#define GIC_IPI_EXT_INTR_RESCHED_VPE0	16
#define GIC_IPI_EXT_INTR_CALLFNC_VPE0	17
#define GIC_IPI_EXT_INTR_RESCHED_VPE1	18
#define GIC_IPI_EXT_INTR_CALLFNC_VPE1	19
#define GIC_IPI_EXT_INTR_RESCHED_VPE2	20
#define GIC_IPI_EXT_INTR_CALLFNC_VPE2	21
#define GIC_IPI_EXT_INTR_RESCHED_VPE3	22
#define GIC_IPI_EXT_INTR_CALLFNC_VPE3	23

#define MIPS_GIC_IRQ_BASE	(MIPS_CPU_IRQ_BASE + 8)

#ifndef __ASSEMBLY__
extern void maltaint_init(void);
#endif

#endif /* !(_MIPS_MALTAINT_H) */
