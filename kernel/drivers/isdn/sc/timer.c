

#include "includes.h"
#include "hardware.h"
#include "message.h"
#include "card.h"


static void setup_ports(int card)
{

	outb((sc_adapter[card]->rambase >> 12), sc_adapter[card]->ioport[EXP_BASE]);

	/* And the IRQ */
	outb((sc_adapter[card]->interrupt | 0x80),
		sc_adapter[card]->ioport[IRQ_SELECT]);
}

void sc_check_reset(unsigned long data)
{
	unsigned long flags;
	unsigned long sig;
	int card = (unsigned int) data;

	pr_debug("%s: check_timer timer called\n",
		sc_adapter[card]->devicename);

	/* Setup the io ports */
	setup_ports(card);

	spin_lock_irqsave(&sc_adapter[card]->lock, flags);
	outb(sc_adapter[card]->ioport[sc_adapter[card]->shmem_pgport],
		(sc_adapter[card]->shmem_magic>>14) | 0x80);
	sig = (unsigned long) *((unsigned long *)(sc_adapter[card]->rambase + SIG_OFFSET));

	/* check the signature */
	if(sig == SIGNATURE) {
		flushreadfifo(card);
		spin_unlock_irqrestore(&sc_adapter[card]->lock, flags);
		/* See if we need to do a startproc */
		if (sc_adapter[card]->StartOnReset)
			startproc(card);
	} else  {
		pr_debug("%s: No signature yet, waiting another %lu jiffies.\n",
			sc_adapter[card]->devicename, CHECKRESET_TIME);
		mod_timer(&sc_adapter[card]->reset_timer, jiffies+CHECKRESET_TIME);
		spin_unlock_irqrestore(&sc_adapter[card]->lock, flags);
	}
}

void check_phystat(unsigned long data)
{
	unsigned long flags;
	int card = (unsigned int) data;

	pr_debug("%s: Checking status...\n", sc_adapter[card]->devicename);
	/* 
	 * check the results of the last PhyStat and change only if
	 * has changed drastically
	 */
	if (sc_adapter[card]->nphystat && !sc_adapter[card]->phystat) {   /* All is well */
		pr_debug("PhyStat transition to RUN\n");
		pr_info("%s: Switch contacted, transmitter enabled\n", 
			sc_adapter[card]->devicename);
		indicate_status(card, ISDN_STAT_RUN, 0, NULL);
	}
	else if (!sc_adapter[card]->nphystat && sc_adapter[card]->phystat) {   /* All is not well */
		pr_debug("PhyStat transition to STOP\n");
		pr_info("%s: Switch connection lost, transmitter disabled\n", 
			sc_adapter[card]->devicename);

		indicate_status(card, ISDN_STAT_STOP, 0, NULL);
	}

	sc_adapter[card]->phystat = sc_adapter[card]->nphystat;

	/* Reinitialize the timer */
	spin_lock_irqsave(&sc_adapter[card]->lock, flags);
	mod_timer(&sc_adapter[card]->stat_timer, jiffies+CHECKSTAT_TIME);
	spin_unlock_irqrestore(&sc_adapter[card]->lock, flags);

	/* Send a new cePhyStatus message */
	sendmessage(card, CEPID,ceReqTypePhy,ceReqClass2,
		ceReqPhyStatus,0,0,NULL);
}

