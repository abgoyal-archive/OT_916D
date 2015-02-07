

#include <crypto/hash.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static struct crypto_shash *tfm;

u32 crc32c(u32 crc, const void *address, unsigned int length)
{
	struct {
		struct shash_desc shash;
		char ctx[crypto_shash_descsize(tfm)];
	} desc;
	int err;

	desc.shash.tfm = tfm;
	desc.shash.flags = 0;
	*(u32 *)desc.ctx = crc;

	err = crypto_shash_update(&desc.shash, address, length);
	BUG_ON(err);

	return *(u32 *)desc.ctx;
}

EXPORT_SYMBOL(crc32c);

static int __init libcrc32c_mod_init(void)
{
	tfm = crypto_alloc_shash("crc32c", 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);

	return 0;
}

static void __exit libcrc32c_mod_fini(void)
{
	crypto_free_shash(tfm);
}

module_init(libcrc32c_mod_init);
module_exit(libcrc32c_mod_fini);

MODULE_AUTHOR("Clay Haapala <chaapala@cisco.com>");
MODULE_DESCRIPTION("CRC32c (Castagnoli) calculations");
MODULE_LICENSE("GPL");
