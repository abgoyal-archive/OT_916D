

#define IRQ_CSINT			4
#define IRQ_EINT1			5
#define IRQ_EINT2			6
#define IRQ_EINT3			7
#define IRQ_TC1OI			8
#define IRQ_TC2OI			9
#define IRQ_RTCMI			10
#define IRQ_TINT			11
#define IRQ_UTXINT1			12
#define IRQ_URXINT1			13
#define IRQ_UMSINT			14
#define IRQ_SSEOTI			15

#define INT1_IRQS			(0x0000fff0)
#define INT1_ACK_IRQS			(0x00004f10)

#define IRQ_KBDINT			(16+0)	/* bit 0 */
#define IRQ_SS2RX			(16+1)	/* bit 1 */
#define IRQ_SS2TX			(16+2)	/* bit 2 */
#define IRQ_UTXINT2			(16+12)	/* bit 12 */
#define IRQ_URXINT2			(16+13)	/* bit 13 */

#define INT2_IRQS			(0x30070000)
#define INT2_ACK_IRQS			(0x00010000)

#define NR_IRQS                         30

