


#define WDT_COUNT0		(io+0)
#define WDT_COUNT1		(io+1)
#define WDT_COUNT2		(io+2)
#define WDT_CR			(io+3)
#define WDT_SR			(io+4)	/* Start buzzer on PCI write */
#define WDT_RT			(io+5)	/* Stop buzzer on PCI write */
#define WDT_BUZZER		(io+6)	/* PCI only: rd=disable, wr=enable */
#define WDT_DC			(io+7)

#define WDT_CLOCK		(io+12)	/* COUNT2: rd=16.67MHz, wr=2.0833MHz */
/* inverted opto isolated reset output: */
#define WDT_OPTONOTRST		(io+13)	/* wr=enable, rd=disable */
/* opto isolated reset output: */
#define WDT_OPTORST		(io+14)	/* wr=enable, rd=disable */
/* programmable outputs: */
#define WDT_PROGOUT		(io+15)	/* wr=enable, rd=disable */

							 /* FAN 501 500 */
#define WDC_SR_WCCR		1	/* Active low */ /*  X   X   X  */
#define WDC_SR_TGOOD		2			 /*  X   X   -  */
#define WDC_SR_ISOI0		4			 /*  X   X   X  */
#define WDC_SR_ISII1		8			 /*  X   X   X  */
#define WDC_SR_FANGOOD		16			 /*  X   -   -  */
#define WDC_SR_PSUOVER		32	/* Active low */ /*  X   X   -  */
#define WDC_SR_PSUUNDR		64	/* Active low */ /*  X   X   -  */
#define WDC_SR_IRQ		128	/* Active low */ /*  X   X   X  */

