

#ifndef __WL_UTIL_H__
#define __WL_UTIL_H__

int dbm( int value );

int is_valid_key_string( char *s );

void key_string2key( char *ks, KEY_STRCT *key );

int hexdigit2int( char c );

void wl_hcf_error( struct net_device *dev, int hcfStatus );

void wl_endian_translate_event( ltv_t *pLtv );

int wl_has_wep( IFBP ifbp );


#if DBG
const char *DbgHwAddr( unsigned char *hwAddr );
#endif // DBG

hcf_8   wl_parse_ds_ie( PROBE_RESP *probe_rsp );
hcf_8 * wl_parse_wpa_ie( PROBE_RESP *probe_rsp, hcf_16 *length );
hcf_8 * wl_print_wpa_ie( hcf_8 *buffer, int length );

int wl_get_tallies(struct wl_private *, CFG_HERMES_TALLIES_STRCT *);
int wl_is_a_valid_chan( int channel );
int wl_is_a_valid_freq( long frequency );
long wl_get_freq_from_chan( int channel );
int wl_get_chan_from_freq( long frequency );

void wl_process_link_status( struct wl_private *lp );
void wl_process_probe_response( struct wl_private *lp );
void wl_process_updated_record( struct wl_private *lp );
void wl_process_assoc_status( struct wl_private *lp );
void wl_process_security_status( struct wl_private *lp );

#endif  // __WL_UTIL_H__
