

#ifndef __KVM_IODEV_H__
#define __KVM_IODEV_H__

#include <linux/kvm_types.h>
#include <asm/errno.h>

struct kvm_io_device;

struct kvm_io_device_ops {
	int (*read)(struct kvm_io_device *this,
		    gpa_t addr,
		    int len,
		    void *val);
	int (*write)(struct kvm_io_device *this,
		     gpa_t addr,
		     int len,
		     const void *val);
	void (*destructor)(struct kvm_io_device *this);
};


struct kvm_io_device {
	const struct kvm_io_device_ops *ops;
};

static inline void kvm_iodevice_init(struct kvm_io_device *dev,
				     const struct kvm_io_device_ops *ops)
{
	dev->ops = ops;
}

static inline int kvm_iodevice_read(struct kvm_io_device *dev,
				    gpa_t addr, int l, void *v)
{
	return dev->ops->read ? dev->ops->read(dev, addr, l, v) : -EOPNOTSUPP;
}

static inline int kvm_iodevice_write(struct kvm_io_device *dev,
				     gpa_t addr, int l, const void *v)
{
	return dev->ops->write ? dev->ops->write(dev, addr, l, v) : -EOPNOTSUPP;
}

static inline void kvm_iodevice_destructor(struct kvm_io_device *dev)
{
	if (dev->ops->destructor)
		dev->ops->destructor(dev);
}

#endif /* __KVM_IODEV_H__ */
