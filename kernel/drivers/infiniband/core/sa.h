

#ifndef SA_H
#define SA_H

#include <rdma/ib_sa.h>

static inline void ib_sa_client_get(struct ib_sa_client *client)
{
	atomic_inc(&client->users);
}

static inline void ib_sa_client_put(struct ib_sa_client *client)
{
	if (atomic_dec_and_test(&client->users))
		complete(&client->comp);
}

int ib_sa_mcmember_rec_query(struct ib_sa_client *client,
			     struct ib_device *device, u8 port_num,
			     u8 method,
			     struct ib_sa_mcmember_rec *rec,
			     ib_sa_comp_mask comp_mask,
			     int timeout_ms, gfp_t gfp_mask,
			     void (*callback)(int status,
					      struct ib_sa_mcmember_rec *resp,
					      void *context),
			     void *context,
			     struct ib_sa_query **sa_query);

int mcast_init(void);
void mcast_cleanup(void);

#endif /* SA_H */
