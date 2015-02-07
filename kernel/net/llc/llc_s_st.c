
#include <linux/types.h>
#include <net/llc_if.h>
#include <net/llc_s_ev.h>
#include <net/llc_s_ac.h>
#include <net/llc_s_st.h>

static struct llc_sap_state_trans llc_sap_state_trans_end;

static llc_sap_action_t llc_sap_inactive_state_actions_1[] = {
	[0] = llc_sap_action_report_status,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_inactive_state_trans_1 = {
	.ev =		llc_sap_ev_activation_req,
	.next_state =	LLC_SAP_STATE_ACTIVE,
	.ev_actions =	llc_sap_inactive_state_actions_1,
};

/* array of pointers; one to each transition */
static struct llc_sap_state_trans *llc_sap_inactive_state_transitions[] = {
	[0] = &llc_sap_inactive_state_trans_1,
	[1] = &llc_sap_state_trans_end,
};

/* state LLC_SAP_STATE_ACTIVE transition for LLC_SAP_EV_RX_UI event */
static llc_sap_action_t llc_sap_active_state_actions_1[] = {
	[0] = llc_sap_action_unitdata_ind,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_active_state_trans_1 = {
	.ev =		llc_sap_ev_rx_ui,
	.next_state =	LLC_SAP_STATE_ACTIVE,
	.ev_actions =	llc_sap_active_state_actions_1,
};

/* state LLC_SAP_STATE_ACTIVE transition for LLC_SAP_EV_UNITDATA_REQ event */
static llc_sap_action_t llc_sap_active_state_actions_2[] = {
	[0] = llc_sap_action_send_ui,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_active_state_trans_2 = {
	.ev =		llc_sap_ev_unitdata_req,
	.next_state =	LLC_SAP_STATE_ACTIVE,
	.ev_actions =	llc_sap_active_state_actions_2,
};

/* state LLC_SAP_STATE_ACTIVE transition for LLC_SAP_EV_XID_REQ event */
static llc_sap_action_t llc_sap_active_state_actions_3[] = {
	[0] = llc_sap_action_send_xid_c,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_active_state_trans_3 = {
	.ev =		llc_sap_ev_xid_req,
	.next_state =	LLC_SAP_STATE_ACTIVE,
	.ev_actions =	llc_sap_active_state_actions_3,
};

/* state LLC_SAP_STATE_ACTIVE transition for LLC_SAP_EV_RX_XID_C event */
static llc_sap_action_t llc_sap_active_state_actions_4[] = {
	[0] = llc_sap_action_send_xid_r,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_active_state_trans_4 = {
	.ev =		llc_sap_ev_rx_xid_c,
	.next_state =	LLC_SAP_STATE_ACTIVE,
	.ev_actions =	llc_sap_active_state_actions_4,
};

/* state LLC_SAP_STATE_ACTIVE transition for LLC_SAP_EV_RX_XID_R event */
static llc_sap_action_t llc_sap_active_state_actions_5[] = {
	[0] = llc_sap_action_xid_ind,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_active_state_trans_5 = {
	.ev =		llc_sap_ev_rx_xid_r,
	.next_state =	LLC_SAP_STATE_ACTIVE,
	.ev_actions =	llc_sap_active_state_actions_5,
};

/* state LLC_SAP_STATE_ACTIVE transition for LLC_SAP_EV_TEST_REQ event */
static llc_sap_action_t llc_sap_active_state_actions_6[] = {
	[0] = llc_sap_action_send_test_c,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_active_state_trans_6 = {
	.ev =		llc_sap_ev_test_req,
	.next_state =	LLC_SAP_STATE_ACTIVE,
	.ev_actions =	llc_sap_active_state_actions_6,
};

/* state LLC_SAP_STATE_ACTIVE transition for LLC_SAP_EV_RX_TEST_C event */
static llc_sap_action_t llc_sap_active_state_actions_7[] = {
	[0] = llc_sap_action_send_test_r,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_active_state_trans_7 = {
	.ev =		llc_sap_ev_rx_test_c,
	.next_state =	LLC_SAP_STATE_ACTIVE,
	.ev_actions =	llc_sap_active_state_actions_7
};

/* state LLC_SAP_STATE_ACTIVE transition for LLC_SAP_EV_RX_TEST_R event */
static llc_sap_action_t llc_sap_active_state_actions_8[] = {
	[0] = llc_sap_action_test_ind,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_active_state_trans_8 = {
	.ev =		llc_sap_ev_rx_test_r,
	.next_state =	LLC_SAP_STATE_ACTIVE,
	.ev_actions =	llc_sap_active_state_actions_8,
};

static llc_sap_action_t llc_sap_active_state_actions_9[] = {
	[0] = llc_sap_action_report_status,
	[1] = NULL,
};

static struct llc_sap_state_trans llc_sap_active_state_trans_9 = {
	.ev =		llc_sap_ev_deactivation_req,
	.next_state =	LLC_SAP_STATE_INACTIVE,
	.ev_actions =	llc_sap_active_state_actions_9
};

/* array of pointers; one to each transition */
static struct llc_sap_state_trans *llc_sap_active_state_transitions[] = {
	[0] = &llc_sap_active_state_trans_2,
	[1] = &llc_sap_active_state_trans_1,
	[2] = &llc_sap_active_state_trans_3,
	[3] = &llc_sap_active_state_trans_4,
	[4] = &llc_sap_active_state_trans_5,
	[5] = &llc_sap_active_state_trans_6,
	[6] = &llc_sap_active_state_trans_7,
	[7] = &llc_sap_active_state_trans_8,
	[8] = &llc_sap_active_state_trans_9,
	[9] = &llc_sap_state_trans_end,
};

/* SAP state transition table */
struct llc_sap_state llc_sap_state_table[LLC_NR_SAP_STATES] = {
	[LLC_SAP_STATE_INACTIVE - 1] = {
		.curr_state	= LLC_SAP_STATE_INACTIVE,
		.transitions	= llc_sap_inactive_state_transitions,
	},
	[LLC_SAP_STATE_ACTIVE - 1] = {
		.curr_state	= LLC_SAP_STATE_ACTIVE,
		.transitions	= llc_sap_active_state_transitions,
	},
};
