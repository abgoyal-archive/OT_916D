

struct stmmac_timer {
	void (*timer_start) (unsigned int new_freq);
	void (*timer_stop) (void);
	unsigned int freq;
	unsigned int enable;
};

/* Open the HW timer device and return 0 in case of success */
int stmmac_open_ext_timer(struct net_device *dev, struct stmmac_timer *tm);
/* Stop the timer and release it */
int stmmac_close_ext_timer(void);
/* Function used for scheduling task within the stmmac */
void stmmac_schedule(struct net_device *dev);

#if defined(CONFIG_STMMAC_TMU_TIMER)
extern int tmu2_register_user(void *fnt, void *data);
extern void tmu2_unregister_user(void);
#endif
