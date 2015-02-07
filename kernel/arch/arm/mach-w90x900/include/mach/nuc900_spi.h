

#ifndef __ASM_ARCH_SPI_H
#define __ASM_ARCH_SPI_H

extern void mfp_set_groupg(struct device *dev);

struct nuc900_spi_info {
	unsigned int num_cs;
	unsigned int lsb;
	unsigned int txneg;
	unsigned int rxneg;
	unsigned int divider;
	unsigned int sleep;
	unsigned int txnum;
	unsigned int txbitlen;
	int bus_num;
};

struct nuc900_spi_chip {
	unsigned char bits_per_word;
};

#endif /* __ASM_ARCH_SPI_H */
