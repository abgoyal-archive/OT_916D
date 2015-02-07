

#include <linux/types.h>
#include <linux/interrupt.h>
#include <asm/delay.h>
#include <asm/sn/sn_sal.h>
#include "ioerror.h"
#include <asm/sn/addrs.h>
#include <asm/sn/shubio.h>
#include <asm/sn/geo.h>
#include "xtalk/xwidgetdev.h"
#include "xtalk/hubdev.h"
#include <asm/sn/bte.h>

void hubiio_crb_error_handler(struct hubdev_info *hubdev_info);
extern void bte_crb_error_handler(cnodeid_t, int, int, ioerror_t *,
				  int);
static irqreturn_t hub_eint_handler(int irq, void *arg)
{
	struct hubdev_info *hubdev_info;
	struct ia64_sal_retval ret_stuff;
	nasid_t nasid;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	hubdev_info = (struct hubdev_info *)arg;
	nasid = hubdev_info->hdi_nasid;

	if (is_shub1()) {
		SAL_CALL_NOLOCK(ret_stuff, SN_SAL_HUB_ERROR_INTERRUPT,
			(u64) nasid, 0, 0, 0, 0, 0, 0);

		if ((int)ret_stuff.v0)
			panic("%s: Fatal %s Error", __func__,
				((nasid & 1) ? "TIO" : "HUBII"));

		if (!(nasid & 1)) /* Not a TIO, handle CRB errors */
			(void)hubiio_crb_error_handler(hubdev_info);
	} else
		if (nasid & 1) {	/* TIO errors */
			SAL_CALL_NOLOCK(ret_stuff, SN_SAL_HUB_ERROR_INTERRUPT,
				(u64) nasid, 0, 0, 0, 0, 0, 0);

			if ((int)ret_stuff.v0)
				panic("%s: Fatal TIO Error", __func__);
		} else
			bte_error_handler((unsigned long)NODEPDA(nasid_to_cnodeid(nasid)));

	return IRQ_HANDLED;
}

void hubiio_crb_free(struct hubdev_info *hubdev_info, int crbnum)
{
	ii_icrb0_b_u_t icrbb;

	/*
	 * The hardware does NOT clear the mark bit, so it must get cleared
	 * here to be sure the error is not processed twice.
	 */
	icrbb.ii_icrb0_b_regval = REMOTE_HUB_L(hubdev_info->hdi_nasid,
					       IIO_ICRB_B(crbnum));
	icrbb.b_mark = 0;
	REMOTE_HUB_S(hubdev_info->hdi_nasid, IIO_ICRB_B(crbnum),
		     icrbb.ii_icrb0_b_regval);
	/*
	 * Deallocate the register wait till hub indicates it's done.
	 */
	REMOTE_HUB_S(hubdev_info->hdi_nasid, IIO_ICDR, (IIO_ICDR_PND | crbnum));
	while (REMOTE_HUB_L(hubdev_info->hdi_nasid, IIO_ICDR) & IIO_ICDR_PND)
		cpu_relax();

}


void hubiio_crb_error_handler(struct hubdev_info *hubdev_info)
{
	nasid_t nasid;
	ii_icrb0_a_u_t icrba;	/* II CRB Register A */
	ii_icrb0_b_u_t icrbb;	/* II CRB Register B */
	ii_icrb0_c_u_t icrbc;	/* II CRB Register C */
	ii_icrb0_d_u_t icrbd;	/* II CRB Register D */
	ii_icrb0_e_u_t icrbe;	/* II CRB Register D */
	int i;
	int num_errors = 0;	/* Num of errors handled */
	ioerror_t ioerror;

	nasid = hubdev_info->hdi_nasid;

	/*
	 * XXX - Add locking for any recovery actions
	 */
	/*
	 * Scan through all CRBs in the Hub, and handle the errors
	 * in any of the CRBs marked.
	 */
	for (i = 0; i < IIO_NUM_CRBS; i++) {
		/* Check this crb entry to see if it is in error. */
		icrbb.ii_icrb0_b_regval = REMOTE_HUB_L(nasid, IIO_ICRB_B(i));

		if (icrbb.b_mark == 0) {
			continue;
		}

		icrba.ii_icrb0_a_regval = REMOTE_HUB_L(nasid, IIO_ICRB_A(i));

		IOERROR_INIT(&ioerror);

		/* read other CRB error registers. */
		icrbc.ii_icrb0_c_regval = REMOTE_HUB_L(nasid, IIO_ICRB_C(i));
		icrbd.ii_icrb0_d_regval = REMOTE_HUB_L(nasid, IIO_ICRB_D(i));
		icrbe.ii_icrb0_e_regval = REMOTE_HUB_L(nasid, IIO_ICRB_E(i));

		IOERROR_SETVALUE(&ioerror, errortype, icrbb.b_ecode);

		/* Check if this error is due to BTE operation,
		 * and handle it separately.
		 */
		if (icrbd.d_bteop ||
		    ((icrbb.b_initiator == IIO_ICRB_INIT_BTE0 ||
		      icrbb.b_initiator == IIO_ICRB_INIT_BTE1) &&
		     (icrbb.b_imsgtype == IIO_ICRB_IMSGT_BTE ||
		      icrbb.b_imsgtype == IIO_ICRB_IMSGT_SN1NET))) {

			int bte_num;

			if (icrbd.d_bteop)
				bte_num = icrbc.c_btenum;
			else	/* b_initiator bit 2 gives BTE number */
				bte_num = (icrbb.b_initiator & 0x4) >> 2;

			hubiio_crb_free(hubdev_info, i);

			bte_crb_error_handler(nasid_to_cnodeid(nasid), bte_num,
					      i, &ioerror, icrbd.d_bteop);
			num_errors++;
			continue;
		}
	}
}

void hub_error_init(struct hubdev_info *hubdev_info)
{

	if (request_irq(SGI_II_ERROR, hub_eint_handler, IRQF_SHARED,
			"SN_hub_error", hubdev_info)) {
		printk(KERN_ERR "hub_error_init: Failed to request_irq for 0x%p\n",
		    hubdev_info);
		return;
	}
	sn_set_err_irq_affinity(SGI_II_ERROR);
}


void ice_error_init(struct hubdev_info *hubdev_info)
{

        if (request_irq
            (SGI_TIO_ERROR, (void *)hub_eint_handler, IRQF_SHARED, "SN_TIO_error",
             (void *)hubdev_info)) {
                printk("ice_error_init: request_irq() error hubdev_info 0x%p\n",
                       hubdev_info);
		return;
	}
	sn_set_err_irq_affinity(SGI_TIO_ERROR);
}

