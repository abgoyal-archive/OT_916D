

#ifndef IVTV_GPIO_H
#define IVTV_GPIO_H

/* GPIO stuff */
int ivtv_gpio_init(struct ivtv *itv);
void ivtv_reset_ir_gpio(struct ivtv *itv);
int ivtv_reset_tuner_gpio(void *dev, int component, int cmd, int value);

#endif
