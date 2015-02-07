

#include <asm/iseries/vio.h>
#include <asm/iseries/hv_lp_event.h>
#include <asm/iseries/hv_types.h>
#include <asm/iseries/hv_lp_config.h>
#include <asm/vio.h>
#include <linux/device.h>
#include "ibmvscsi.h"

/* global variables */
static struct ibmvscsi_host_data *single_host_data;

struct srp_lp_event {
	struct HvLpEvent lpevt;	/* 0x00-0x17          */
	u32 reserved1;		/* 0x18-0x1B; unused  */
	u16 version;		/* 0x1C-0x1D; unused  */
	u16 subtype_rc;		/* 0x1E-0x1F; unused  */
	struct viosrp_crq crq;	/* 0x20-0x3F          */
};

static void iseriesvscsi_handle_event(struct HvLpEvent *lpevt)
{
	struct srp_lp_event *evt = (struct srp_lp_event *)lpevt;

	if (!evt) {
		printk(KERN_ERR "ibmvscsi: received null event\n");
		return;
	}

	if (single_host_data == NULL) {
		printk(KERN_ERR
		       "ibmvscsi: received event, no adapter present\n");
		return;
	}

	ibmvscsi_handle_crq(&evt->crq, single_host_data);
}

static int iseriesvscsi_init_crq_queue(struct crq_queue *queue,
				       struct ibmvscsi_host_data *hostdata,
				       int max_requests)
{
	int rc;

	single_host_data = hostdata;
	rc = viopath_open(viopath_hostLp, viomajorsubtype_scsi, max_requests);
	if (rc < 0) {
		printk("viopath_open failed with rc %d in open_event_path\n",
		       rc);
		goto viopath_open_failed;
	}

	rc = vio_setHandler(viomajorsubtype_scsi, iseriesvscsi_handle_event);
	if (rc < 0) {
		printk("vio_setHandler failed with rc %d in open_event_path\n",
		       rc);
		goto vio_setHandler_failed;
	}
	return 0;

      vio_setHandler_failed:
	viopath_close(viopath_hostLp, viomajorsubtype_scsi, max_requests);
      viopath_open_failed:
	return -1;
}

static void iseriesvscsi_release_crq_queue(struct crq_queue *queue,
					   struct ibmvscsi_host_data *hostdata,
					   int max_requests)
{
	vio_clearHandler(viomajorsubtype_scsi);
	viopath_close(viopath_hostLp, viomajorsubtype_scsi, max_requests);
}

static int iseriesvscsi_reset_crq_queue(struct crq_queue *queue,
					struct ibmvscsi_host_data *hostdata)
{
	return 0;
}

static int iseriesvscsi_reenable_crq_queue(struct crq_queue *queue,
					   struct ibmvscsi_host_data *hostdata)
{
	return 0;
}

static int iseriesvscsi_send_crq(struct ibmvscsi_host_data *hostdata,
				 u64 word1, u64 word2)
{
	single_host_data = hostdata;
	return HvCallEvent_signalLpEventFast(viopath_hostLp,
					     HvLpEvent_Type_VirtualIo,
					     viomajorsubtype_scsi,
					     HvLpEvent_AckInd_NoAck,
					     HvLpEvent_AckType_ImmediateAck,
					     viopath_sourceinst(viopath_hostLp),
					     viopath_targetinst(viopath_hostLp),
					     0,
					     VIOVERSION << 16, word1, word2, 0,
					     0);
}

static int iseriesvscsi_resume(struct ibmvscsi_host_data *hostdata)
{
	return 0;
}

struct ibmvscsi_ops iseriesvscsi_ops = {
	.init_crq_queue = iseriesvscsi_init_crq_queue,
	.release_crq_queue = iseriesvscsi_release_crq_queue,
	.reset_crq_queue = iseriesvscsi_reset_crq_queue,
	.reenable_crq_queue = iseriesvscsi_reenable_crq_queue,
	.send_crq = iseriesvscsi_send_crq,
	.resume = iseriesvscsi_resume,
};
