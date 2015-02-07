

#ifndef SPI_LIS3L02DQ_H_
#define SPI_LIS3L02DQ_H_
#define LIS3L02DQ_READ_REG(a) ((a) | 0x80)
#define LIS3L02DQ_WRITE_REG(a) a

/* Calibration parameters */
#define LIS3L02DQ_REG_OFFSET_X_ADDR		0x16
#define LIS3L02DQ_REG_OFFSET_Y_ADDR		0x17
#define LIS3L02DQ_REG_OFFSET_Z_ADDR		0x18

#define LIS3L02DQ_REG_GAIN_X_ADDR		0x19
#define LIS3L02DQ_REG_GAIN_Y_ADDR		0x1A
#define LIS3L02DQ_REG_GAIN_Z_ADDR		0x1B

/* Control Register (1 of 2) */
#define LIS3L02DQ_REG_CTRL_1_ADDR		0x20
/* Power ctrl - either bit set corresponds to on*/
#define LIS3L02DQ_REG_CTRL_1_PD_ON	0xC0

/* Decimation Factor  */
#define LIS3L02DQ_DEC_MASK			0x30
#define LIS3L02DQ_REG_CTRL_1_DF_128		0x00
#define LIS3L02DQ_REG_CTRL_1_DF_64		0x10
#define LIS3L02DQ_REG_CTRL_1_DF_32		0x20
#define LIS3L02DQ_REG_CTRL_1_DF_8		(0x10 | 0x20)

/* Self Test Enable */
#define LIS3L02DQ_REG_CTRL_1_SELF_TEST_ON	0x08

/* Axes enable ctrls */
#define LIS3L02DQ_REG_CTRL_1_AXES_Z_ENABLE	0x04
#define LIS3L02DQ_REG_CTRL_1_AXES_Y_ENABLE	0x02
#define LIS3L02DQ_REG_CTRL_1_AXES_X_ENABLE	0x01

/* Control Register (2 of 2) */
#define LIS3L02DQ_REG_CTRL_2_ADDR		0x21

/* Block Data Update only after MSB and LSB read */
#define LIS3L02DQ_REG_CTRL_2_BLOCK_UPDATE	0x40

/* Set to big endian output */
#define LIS3L02DQ_REG_CTRL_2_BIG_ENDIAN		0x20

/* Reboot memory content */
#define LIS3L02DQ_REG_CTRL_2_REBOOT_MEMORY	0x10

/* Interupt Enable - applies data ready to the RDY pad */
#define LIS3L02DQ_REG_CTRL_2_ENABLE_INTERRUPT	0x08

/* Enable Data Ready Generation - relationship with previous unclear in docs */
#define LIS3L02DQ_REG_CTRL_2_ENABLE_DATA_READY_GENERATION 0x04

/* SPI 3 wire mode */
#define LIS3L02DQ_REG_CTRL_2_THREE_WIRE_SPI_MODE	0x02

#define LIS3L02DQ_REG_CTRL_2_DATA_ALIGNMENT_16_BIT_LEFT_JUSTIFIED	0x01

/* Interupt related stuff */
#define LIS3L02DQ_REG_WAKE_UP_CFG_ADDR			0x23

/* Switch from or combination fo conditions to and */
#define LIS3L02DQ_REG_WAKE_UP_CFG_BOOLEAN_AND		0x80

#define LIS3L02DQ_REG_WAKE_UP_CFG_LATCH_SRC		0x40

/* Z Interupt on High (above threshold)*/
#define LIS3L02DQ_REG_WAKE_UP_CFG_INTERRUPT_Z_HIGH	0x20
/* Z Interupt on Low */
#define LIS3L02DQ_REG_WAKE_UP_CFG_INTERRUPT_Z_LOW	0x10
/* Y Interupt on High */
#define LIS3L02DQ_REG_WAKE_UP_CFG_INTERRUPT_Y_HIGH	0x08
/* Y Interupt on Low */
#define LIS3L02DQ_REG_WAKE_UP_CFG_INTERRUPT_Y_LOW	0x04
/* X Interupt on High */
#define LIS3L02DQ_REG_WAKE_UP_CFG_INTERRUPT_X_HIGH	0x02
/* X Interupt on Low */
#define LIS3L02DQ_REG_WAKE_UP_CFG_INTERRUPT_X_LOW 0x01

#define LIS3L02DQ_REG_WAKE_UP_SRC_ADDR			0x24
/* top bit ignored */
/* Interupt Active */
#define LIS3L02DQ_REG_WAKE_UP_SRC_INTERRUPT_ACTIVATED	0x40
/* Interupts that have been triggered */
#define LIS3L02DQ_REG_WAKE_UP_SRC_INTERRUPT_Z_HIGH	0x20
#define LIS3L02DQ_REG_WAKE_UP_SRC_INTERRUPT_Z_LOW	0x10
#define LIS3L02DQ_REG_WAKE_UP_SRC_INTERRUPT_Y_HIGH	0x08
#define LIS3L02DQ_REG_WAKE_UP_SRC_INTERRUPT_Y_LOW	0x04
#define LIS3L02DQ_REG_WAKE_UP_SRC_INTERRUPT_X_HIGH	0x02
#define LIS3L02DQ_REG_WAKE_UP_SRC_INTERRUPT_X_LOW	0x01

#define LIS3L02DQ_REG_WAKE_UP_ACK_ADDR			0x25

/* Status register */
#define LIS3L02DQ_REG_STATUS_ADDR			0x27
/* XYZ axis data overrun - first is all overrun? */
#define LIS3L02DQ_REG_STATUS_XYZ_OVERRUN		0x80
#define LIS3L02DQ_REG_STATUS_Z_OVERRUN			0x40
#define LIS3L02DQ_REG_STATUS_Y_OVERRUN			0x20
#define LIS3L02DQ_REG_STATUS_X_OVERRUN			0x10
/* XYZ new data available - first is all 3 available? */
#define LIS3L02DQ_REG_STATUS_XYZ_NEW_DATA 0x08
#define LIS3L02DQ_REG_STATUS_Z_NEW_DATA			0x04
#define LIS3L02DQ_REG_STATUS_Y_NEW_DATA			0x02
#define LIS3L02DQ_REG_STATUS_X_NEW_DATA			0x01

#define LIS3L02DQ_REG_OUT_X_L_ADDR			0x28
#define LIS3L02DQ_REG_OUT_X_H_ADDR			0x29
#define LIS3L02DQ_REG_OUT_Y_L_ADDR			0x2A
#define LIS3L02DQ_REG_OUT_Y_H_ADDR			0x2B
#define LIS3L02DQ_REG_OUT_Z_L_ADDR			0x2C
#define LIS3L02DQ_REG_OUT_Z_H_ADDR			0x2D

#define LIS3L02DQ_REG_THS_L_ADDR			0x2E
#define LIS3L02DQ_REG_THS_H_ADDR			0x2F

#define LIS3L02DQ_DEFAULT_CTRL1 (LIS3L02DQ_REG_CTRL_1_PD_ON	      \
				 | LIS3L02DQ_REG_CTRL_1_AXES_Z_ENABLE \
				 | LIS3L02DQ_REG_CTRL_1_AXES_Y_ENABLE \
				 | LIS3L02DQ_REG_CTRL_1_AXES_X_ENABLE \
				 | LIS3L02DQ_REG_CTRL_1_DF_128)

#define LIS3L02DQ_DEFAULT_CTRL2	0

#define LIS3L02DQ_MAX_TX 12
#define LIS3L02DQ_MAX_RX 12
struct lis3l02dq_state {
	struct spi_device		*us;
	struct work_struct		work_trigger_to_ring;
	struct iio_work_cont		work_cont_thresh;
	bool				inter;
	s64				last_timestamp;
	struct iio_dev			*indio_dev;
	struct iio_trigger		*trig;
	u8				*tx;
	u8				*rx;
	struct mutex			buf_lock;
};

int lis3l02dq_spi_read_reg_8(struct device *dev,
			     u8 reg_address,
			     u8 *val);

int lis3l02dq_spi_write_reg_8(struct device *dev,
			      u8 reg_address,
			      u8 *val);

#ifdef CONFIG_IIO_RING_BUFFER
void lis3l02dq_remove_trigger(struct iio_dev *indio_dev);
int lis3l02dq_probe_trigger(struct iio_dev *indio_dev);

ssize_t lis3l02dq_read_accel_from_ring(struct device *dev,
				       struct device_attribute *attr,
				       char *buf);


int lis3l02dq_configure_ring(struct iio_dev *indio_dev);
void lis3l02dq_unconfigure_ring(struct iio_dev *indio_dev);

int lis3l02dq_initialize_ring(struct iio_ring_buffer *ring);
void lis3l02dq_uninitialize_ring(struct iio_ring_buffer *ring);
#else /* CONFIG_IIO_RING_BUFFER */

static inline void lis3l02dq_remove_trigger(struct iio_dev *indio_dev) {};
static inline int lis3l02dq_probe_trigger(struct iio_dev *indio_dev)
{
	return 0;
};

static inline ssize_t
lis3l02dq_read_accel_from_ring(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	return 0;
};

static int lis3l02dq_configure_ring(struct iio_dev *indio_dev)
{
	return 0;
};
static inline void lis3l02dq_unconfigure_ring(struct iio_dev *indio_dev)
{};
static inline int lis3l02dq_initialize_ring(struct iio_ring_buffer *ring)
{
	return 0;
};
static inline void lis3l02dq_uninitialize_ring(struct iio_ring_buffer *ring) {};
#endif /* CONFIG_IIO_RING_BUFFER */
#endif /* SPI_LIS3L02DQ_H_ */
