


#ifndef RT2X00SOC_H
#define RT2X00SOC_H

int rt2x00soc_probe(struct platform_device *pdev, const struct rt2x00_ops *ops);
int rt2x00soc_remove(struct platform_device *pdev);
#ifdef CONFIG_PM
int rt2x00soc_suspend(struct platform_device *pdev, pm_message_t state);
int rt2x00soc_resume(struct platform_device *pdev);
#else
#define rt2x00soc_suspend	NULL
#define rt2x00soc_resume	NULL
#endif /* CONFIG_PM */

#endif /* RT2X00SOC_H */
