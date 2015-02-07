
#undef DEBUG

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/io.h>

#include <plat/omap_device.h>
#include <plat/omap_hwmod.h>

/* These parameters are passed to _omap_device_{de,}activate() */
#define USE_WAKEUP_LAT			0
#define IGNORE_WAKEUP_LAT		1


#define OMAP_DEVICE_MAGIC 0xf00dcafe

/* Private functions */

static int _omap_device_activate(struct omap_device *od, u8 ignore_lat)
{
	struct timespec a, b, c;

	pr_debug("omap_device: %s: activating\n", od->pdev.name);

	while (od->pm_lat_level > 0) {
		struct omap_device_pm_latency *odpl;
		unsigned long long act_lat = 0;

		od->pm_lat_level--;

		odpl = od->pm_lats + od->pm_lat_level;

		if (!ignore_lat &&
		    (od->dev_wakeup_lat <= od->_dev_wakeup_lat_limit))
			break;

		read_persistent_clock(&a);

		/* XXX check return code */
		odpl->activate_func(od);

		read_persistent_clock(&b);

		c = timespec_sub(b, a);
		act_lat = timespec_to_ns(&c);

		pr_debug("omap_device: %s: pm_lat %d: activate: elapsed time "
			 "%llu nsec\n", od->pdev.name, od->pm_lat_level,
			 act_lat);

		if (act_lat > odpl->activate_lat) {
			odpl->activate_lat_worst = act_lat;
			if (odpl->flags & OMAP_DEVICE_LATENCY_AUTO_ADJUST) {
				odpl->activate_lat = act_lat;
				pr_warning("omap_device: %s.%d: new worst case "
					   "activate latency %d: %llu\n",
					   od->pdev.name, od->pdev.id,
					   od->pm_lat_level, act_lat);
			} else
				pr_warning("omap_device: %s.%d: activate "
					   "latency %d higher than exptected. "
					   "(%llu > %d)\n",
					   od->pdev.name, od->pdev.id,
					   od->pm_lat_level, act_lat,
					   odpl->activate_lat);
		}

		od->dev_wakeup_lat -= odpl->activate_lat;
	}

	return 0;
}

static int _omap_device_deactivate(struct omap_device *od, u8 ignore_lat)
{
	struct timespec a, b, c;

	pr_debug("omap_device: %s: deactivating\n", od->pdev.name);

	while (od->pm_lat_level < od->pm_lats_cnt) {
		struct omap_device_pm_latency *odpl;
		unsigned long long deact_lat = 0;

		odpl = od->pm_lats + od->pm_lat_level;

		if (!ignore_lat &&
		    ((od->dev_wakeup_lat + odpl->activate_lat) >
		     od->_dev_wakeup_lat_limit))
			break;

		read_persistent_clock(&a);

		/* XXX check return code */
		odpl->deactivate_func(od);

		read_persistent_clock(&b);

		c = timespec_sub(b, a);
		deact_lat = timespec_to_ns(&c);

		pr_debug("omap_device: %s: pm_lat %d: deactivate: elapsed time "
			 "%llu nsec\n", od->pdev.name, od->pm_lat_level,
			 deact_lat);

		if (deact_lat > odpl->deactivate_lat) {
			odpl->deactivate_lat_worst = deact_lat;
			if (odpl->flags & OMAP_DEVICE_LATENCY_AUTO_ADJUST) {
				odpl->deactivate_lat = deact_lat;
				pr_warning("omap_device: %s.%d: new worst case "
					   "deactivate latency %d: %llu\n",
					   od->pdev.name, od->pdev.id,
					   od->pm_lat_level, deact_lat);
			} else
				pr_warning("omap_device: %s.%d: deactivate "
					   "latency %d higher than exptected. "
					   "(%llu > %d)\n",
					   od->pdev.name, od->pdev.id,
					   od->pm_lat_level, deact_lat,
					   odpl->deactivate_lat);
		}


		od->dev_wakeup_lat += odpl->activate_lat;

		od->pm_lat_level++;
	}

	return 0;
}

static inline struct omap_device *_find_by_pdev(struct platform_device *pdev)
{
	return container_of(pdev, struct omap_device, pdev);
}


/* Public functions for use by core code */

int omap_device_count_resources(struct omap_device *od)
{
	struct omap_hwmod *oh;
	int c = 0;
	int i;

	for (i = 0, oh = *od->hwmods; i < od->hwmods_cnt; i++, oh++)
		c += omap_hwmod_count_resources(oh);

	pr_debug("omap_device: %s: counted %d total resources across %d "
		 "hwmods\n", od->pdev.name, c, od->hwmods_cnt);

	return c;
}

int omap_device_fill_resources(struct omap_device *od, struct resource *res)
{
	struct omap_hwmod *oh;
	int c = 0;
	int i, r;

	for (i = 0, oh = *od->hwmods; i < od->hwmods_cnt; i++, oh++) {
		r = omap_hwmod_fill_resources(oh, res);
		res += r;
		c += r;
	}

	return 0;
}

struct omap_device *omap_device_build(const char *pdev_name, int pdev_id,
				      struct omap_hwmod *oh, void *pdata,
				      int pdata_len,
				      struct omap_device_pm_latency *pm_lats,
				      int pm_lats_cnt, int is_early_device)
{
	struct omap_hwmod *ohs[] = { oh };

	if (!oh)
		return ERR_PTR(-EINVAL);

	return omap_device_build_ss(pdev_name, pdev_id, ohs, 1, pdata,
				    pdata_len, pm_lats, pm_lats_cnt,
				    is_early_device);
}

struct omap_device *omap_device_build_ss(const char *pdev_name, int pdev_id,
					 struct omap_hwmod **ohs, int oh_cnt,
					 void *pdata, int pdata_len,
					 struct omap_device_pm_latency *pm_lats,
					 int pm_lats_cnt, int is_early_device)
{
	int ret = -ENOMEM;
	struct omap_device *od;
	char *pdev_name2;
	struct resource *res = NULL;
	int res_count;
	struct omap_hwmod **hwmods;

	if (!ohs || oh_cnt == 0 || !pdev_name)
		return ERR_PTR(-EINVAL);

	if (!pdata && pdata_len > 0)
		return ERR_PTR(-EINVAL);

	pr_debug("omap_device: %s: building with %d hwmods\n", pdev_name,
		 oh_cnt);

	od = kzalloc(sizeof(struct omap_device), GFP_KERNEL);
	if (!od)
		return ERR_PTR(-ENOMEM);

	od->hwmods_cnt = oh_cnt;

	hwmods = kzalloc(sizeof(struct omap_hwmod *) * oh_cnt,
			 GFP_KERNEL);
	if (!hwmods)
		goto odbs_exit1;

	memcpy(hwmods, ohs, sizeof(struct omap_hwmod *) * oh_cnt);
	od->hwmods = hwmods;

	pdev_name2 = kzalloc(strlen(pdev_name) + 1, GFP_KERNEL);
	if (!pdev_name2)
		goto odbs_exit2;
	strcpy(pdev_name2, pdev_name);

	od->pdev.name = pdev_name2;
	od->pdev.id = pdev_id;

	res_count = omap_device_count_resources(od);
	if (res_count > 0) {
		res = kzalloc(sizeof(struct resource) * res_count, GFP_KERNEL);
		if (!res)
			goto odbs_exit3;
	}
	omap_device_fill_resources(od, res);

	od->pdev.num_resources = res_count;
	od->pdev.resource = res;

	platform_device_add_data(&od->pdev, pdata, pdata_len);

	od->pm_lats = pm_lats;
	od->pm_lats_cnt = pm_lats_cnt;

	od->magic = OMAP_DEVICE_MAGIC;

	if (is_early_device)
		ret = omap_early_device_register(od);
	else
		ret = omap_device_register(od);

	if (ret)
		goto odbs_exit4;

	return od;

odbs_exit4:
	kfree(res);
odbs_exit3:
	kfree(pdev_name2);
odbs_exit2:
	kfree(hwmods);
odbs_exit1:
	kfree(od);

	pr_err("omap_device: %s: build failed (%d)\n", pdev_name, ret);

	return ERR_PTR(ret);
}

int omap_early_device_register(struct omap_device *od)
{
	struct platform_device *devices[1];

	devices[0] = &(od->pdev);
	early_platform_add_devices(devices, 1);
	return 0;
}

int omap_device_register(struct omap_device *od)
{
	pr_debug("omap_device: %s: registering\n", od->pdev.name);

	return platform_device_register(&od->pdev);
}


/* Public functions for use by device drivers through struct platform_data */

int omap_device_enable(struct platform_device *pdev)
{
	int ret;
	struct omap_device *od;

	od = _find_by_pdev(pdev);

	if (od->_state == OMAP_DEVICE_STATE_ENABLED) {
		WARN(1, "omap_device: %s.%d: %s() called from invalid state %d\n",
		     od->pdev.name, od->pdev.id, __func__, od->_state);
		return -EINVAL;
	}

	/* Enable everything if we're enabling this device from scratch */
	if (od->_state == OMAP_DEVICE_STATE_UNKNOWN)
		od->pm_lat_level = od->pm_lats_cnt;

	ret = _omap_device_activate(od, IGNORE_WAKEUP_LAT);

	od->dev_wakeup_lat = 0;
	od->_dev_wakeup_lat_limit = UINT_MAX;
	od->_state = OMAP_DEVICE_STATE_ENABLED;

	return ret;
}

int omap_device_idle(struct platform_device *pdev)
{
	int ret;
	struct omap_device *od;

	od = _find_by_pdev(pdev);

	if (od->_state != OMAP_DEVICE_STATE_ENABLED) {
		WARN(1, "omap_device: %s.%d: %s() called from invalid state %d\n",
		     od->pdev.name, od->pdev.id, __func__, od->_state);
		return -EINVAL;
	}

	ret = _omap_device_deactivate(od, USE_WAKEUP_LAT);

	od->_state = OMAP_DEVICE_STATE_IDLE;

	return ret;
}

int omap_device_shutdown(struct platform_device *pdev)
{
	int ret, i;
	struct omap_device *od;
	struct omap_hwmod *oh;

	od = _find_by_pdev(pdev);

	if (od->_state != OMAP_DEVICE_STATE_ENABLED &&
	    od->_state != OMAP_DEVICE_STATE_IDLE) {
		WARN(1, "omap_device: %s.%d: %s() called from invalid state %d\n",
		     od->pdev.name, od->pdev.id, __func__, od->_state);
		return -EINVAL;
	}

	ret = _omap_device_deactivate(od, IGNORE_WAKEUP_LAT);

	for (i = 0, oh = *od->hwmods; i < od->hwmods_cnt; i++, oh++)
		omap_hwmod_shutdown(oh);

	od->_state = OMAP_DEVICE_STATE_SHUTDOWN;

	return ret;
}

int omap_device_align_pm_lat(struct platform_device *pdev,
			     u32 new_wakeup_lat_limit)
{
	int ret = -EINVAL;
	struct omap_device *od;

	od = _find_by_pdev(pdev);

	if (new_wakeup_lat_limit == od->dev_wakeup_lat)
		return 0;

	od->_dev_wakeup_lat_limit = new_wakeup_lat_limit;

	if (od->_state != OMAP_DEVICE_STATE_IDLE)
		return 0;
	else if (new_wakeup_lat_limit > od->dev_wakeup_lat)
		ret = _omap_device_deactivate(od, USE_WAKEUP_LAT);
	else if (new_wakeup_lat_limit < od->dev_wakeup_lat)
		ret = _omap_device_activate(od, USE_WAKEUP_LAT);

	return ret;
}

bool omap_device_is_valid(struct omap_device *od)
{
	return (od && od->magic == OMAP_DEVICE_MAGIC);
}

struct powerdomain *omap_device_get_pwrdm(struct omap_device *od)
{
	/*
	 * XXX Assumes that all omap_hwmod powerdomains are identical.
	 * This may not necessarily be true.  There should be a sanity
	 * check in here to WARN() if any difference appears.
	 */
	if (!od->hwmods_cnt)
		return NULL;

	return omap_hwmod_get_pwrdm(od->hwmods[0]);
}


int omap_device_enable_hwmods(struct omap_device *od)
{
	struct omap_hwmod *oh;
	int i;

	for (i = 0, oh = *od->hwmods; i < od->hwmods_cnt; i++, oh++)
		omap_hwmod_enable(oh);

	/* XXX pass along return value here? */
	return 0;
}

int omap_device_idle_hwmods(struct omap_device *od)
{
	struct omap_hwmod *oh;
	int i;

	for (i = 0, oh = *od->hwmods; i < od->hwmods_cnt; i++, oh++)
		omap_hwmod_idle(oh);

	/* XXX pass along return value here? */
	return 0;
}

int omap_device_disable_clocks(struct omap_device *od)
{
	struct omap_hwmod *oh;
	int i;

	for (i = 0, oh = *od->hwmods; i < od->hwmods_cnt; i++, oh++)
		omap_hwmod_disable_clocks(oh);

	/* XXX pass along return value here? */
	return 0;
}

int omap_device_enable_clocks(struct omap_device *od)
{
	struct omap_hwmod *oh;
	int i;

	for (i = 0, oh = *od->hwmods; i < od->hwmods_cnt; i++, oh++)
		omap_hwmod_enable_clocks(oh);

	/* XXX pass along return value here? */
	return 0;
}
