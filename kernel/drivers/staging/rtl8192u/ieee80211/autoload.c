
#include "kmap_types.h"

#include <linux/kernel.h>
//#include <linux/crypto.h>
#include "rtl_crypto.h"
#include <linux/string.h>
#include <linux/kmod.h>
#include "internal.h"

void crypto_alg_autoload(const char *name)
{
	request_module(name);
}

struct crypto_alg *crypto_alg_mod_lookup(const char *name)
{
	struct crypto_alg *alg = crypto_alg_lookup(name);
	if (alg == NULL) {
		crypto_alg_autoload(name);
		alg = crypto_alg_lookup(name);
	}
	return alg;
}
