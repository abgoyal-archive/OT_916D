

#ifndef __IWCTL_H__
#define __IWCTL_H__

#include "device.h"

/*---------------------  Export Definitions -------------------------*/


/*---------------------  Export Classes  ----------------------------*/

/*---------------------  Export Variables  --------------------------*/

/*---------------------  Export Functions  --------------------------*/

struct iw_statistics *iwctl_get_wireless_stats (struct net_device *dev);


int iwctl_siwap(struct net_device *dev,
             struct iw_request_info *info,
			 struct sockaddr *wrq,
             char *extra);

int iwctl_giwrange(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_point *wrq,
             char *extra);


int iwctl_giwmode(struct net_device *dev,
             struct iw_request_info *info,
             __u32 *wmode,
             char *extra);

int iwctl_siwmode(struct net_device *dev,
             struct iw_request_info *info,
             __u32 *wmode,
             char *extra);

int iwctl_giwfreq(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_freq *wrq,
             char *extra);

int iwctl_siwfreq(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_freq *wrq,
             char *extra);

int iwctl_giwname(struct net_device *dev,
			 struct iw_request_info *info,
			 char *wrq,
			 char *extra);

int iwctl_giwnwid(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
                   char *extra) ;

int iwctl_giwsens(struct net_device *dev,
			 struct iw_request_info *info,
			 struct iw_param *wrq,
			 char *extra);

int iwctl_giwap(struct net_device *dev,
             struct iw_request_info *info,
			 struct sockaddr *wrq,
             char *extra);

int iwctl_giwaplist(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_point *wrq,
             char *extra);

int iwctl_siwessid(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_point *wrq,
             char *extra);

int iwctl_giwessid(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_point *wrq,
             char *extra);

int iwctl_siwrate(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);

int iwctl_giwrate(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_param *wrq,
             char *extra);

int iwctl_siwrts(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);


int iwctl_giwrts(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);

int iwctl_siwfrag(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);

int iwctl_giwfrag(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);

int iwctl_siwretry(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);

int iwctl_giwretry(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);

int iwctl_siwencode(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_point *wrq,
             char *extra);

int iwctl_giwencode(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_point *wrq,
             char *extra);

int iwctl_siwpower(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);

int iwctl_giwpower(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);

int iwctl_giwscan(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_point *wrq,
             char *extra);

int iwctl_siwscan(struct net_device *dev,
             struct iw_request_info *info,
			 struct iw_param *wrq,
             char *extra);

//2008-0409-07, <Add> by Einsn Liu
#ifdef WPA_SUPPLICANT_DRIVER_WEXT_SUPPORT
int iwctl_siwauth(struct net_device *dev,
			  struct iw_request_info *info,
			  struct iw_param *wrq,
			  char *extra);

int iwctl_giwauth(struct net_device *dev,
			  struct iw_request_info *info,
			  struct iw_param *wrq,
			  char *extra);

int iwctl_siwgenie(struct net_device *dev,
			  struct iw_request_info *info,
			  struct iw_point *wrq,
			  char *extra);

int iwctl_giwgenie(struct net_device *dev,
			  struct iw_request_info *info,
			  struct iw_point *wrq,
			  char *extra);

int iwctl_siwencodeext(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_point *wrq,
             char *extra);

int iwctl_giwencodeext(struct net_device *dev,
             struct iw_request_info *info,
             struct iw_point *wrq,
             char *extra);

int iwctl_siwmlme(struct net_device *dev,
			struct iw_request_info * info,
			struct iw_point *wrq,
			char *extra);
#endif // #ifdef WPA_SUPPLICANT_DRIVER_WEXT_SUPPORT
//End Add -- //2008-0409-07, <Add> by Einsn Liu


extern const struct iw_handler_def	iwctl_handler_def;
extern const struct iw_priv_args	iwctl_private_args;

#endif // __IWCTL_H__



