


#include <scsi/libfc.h>


struct fc_lport *libfc_vport_create(struct fc_vport *vport, int privsize)
{
	struct Scsi_Host *shost = vport_to_shost(vport);
	struct fc_lport *n_port = shost_priv(shost);
	struct fc_lport *vn_port;

	vn_port = libfc_host_alloc(shost->hostt, privsize);
	if (!vn_port)
		goto err_out;
	if (fc_exch_mgr_list_clone(n_port, vn_port))
		goto err_put;

	vn_port->vport = vport;
	vport->dd_data = vn_port;

	mutex_lock(&n_port->lp_mutex);
	list_add_tail(&vn_port->list, &n_port->vports);
	mutex_unlock(&n_port->lp_mutex);

	return vn_port;

err_put:
	scsi_host_put(vn_port->host);
err_out:
	return NULL;
}
EXPORT_SYMBOL(libfc_vport_create);

struct fc_lport *fc_vport_id_lookup(struct fc_lport *n_port, u32 port_id)
{
	struct fc_lport *lport = NULL;
	struct fc_lport *vn_port;

	if (n_port->port_id == port_id)
		return n_port;

	if (port_id == FC_FID_FLOGI)
		return n_port;		/* for point-to-point */

	mutex_lock(&n_port->lp_mutex);
	list_for_each_entry(vn_port, &n_port->vports, list) {
		if (vn_port->port_id == port_id) {
			lport = vn_port;
			break;
		}
	}
	mutex_unlock(&n_port->lp_mutex);

	return lport;
}

enum libfc_lport_mutex_class {
	LPORT_MUTEX_NORMAL = 0,
	LPORT_MUTEX_VN_PORT = 1,
};

static void __fc_vport_setlink(struct fc_lport *n_port,
			       struct fc_lport *vn_port)
{
	struct fc_vport *vport = vn_port->vport;

	if (vn_port->state == LPORT_ST_DISABLED)
		return;

	if (n_port->state == LPORT_ST_READY) {
		if (n_port->npiv_enabled) {
			fc_vport_set_state(vport, FC_VPORT_INITIALIZING);
			__fc_linkup(vn_port);
		} else {
			fc_vport_set_state(vport, FC_VPORT_NO_FABRIC_SUPP);
			__fc_linkdown(vn_port);
		}
	} else {
		fc_vport_set_state(vport, FC_VPORT_LINKDOWN);
		__fc_linkdown(vn_port);
	}
}

void fc_vport_setlink(struct fc_lport *vn_port)
{
	struct fc_vport *vport = vn_port->vport;
	struct Scsi_Host *shost = vport_to_shost(vport);
	struct fc_lport *n_port = shost_priv(shost);

	mutex_lock(&n_port->lp_mutex);
	mutex_lock_nested(&vn_port->lp_mutex, LPORT_MUTEX_VN_PORT);
	__fc_vport_setlink(n_port, vn_port);
	mutex_unlock(&vn_port->lp_mutex);
	mutex_unlock(&n_port->lp_mutex);
}
EXPORT_SYMBOL(fc_vport_setlink);

void fc_vports_linkchange(struct fc_lport *n_port)
{
	struct fc_lport *vn_port;

	list_for_each_entry(vn_port, &n_port->vports, list) {
		mutex_lock_nested(&vn_port->lp_mutex, LPORT_MUTEX_VN_PORT);
		__fc_vport_setlink(n_port, vn_port);
		mutex_unlock(&vn_port->lp_mutex);
	}
}

