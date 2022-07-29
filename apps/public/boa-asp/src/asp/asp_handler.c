#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#ifdef TCSUPPORT_SYSLOG_ENHANCE
#include <syslog.h>
#endif
#include <sys/ipc.h>
#include <sys/shm.h>

#include "mini-asp.h"
#include "grammar.h"
#include "http-get-utils.h"
#include "../boa.h"
#include "libtcapi.h"
#include "tcapi.h"
#include "apps.h"
#include <json.h>

#if defined(TCSUPPORT_GENERAL_MULTILANGUAGE)
#include "parsers.h"
#endif

//Sam
#ifdef RTCONFIG_USB
#include "disk_io_tools.h"
#include "disk_initial.h"
#include "disk_share.h"
#endif
#include "shutils.h"
#include "rtstate.h"
#include "tcutils.h"
#include "shared.h"
#include "bcmnvram.h"
#define wan_prefix(unit, prefix) snprintf(prefix, sizeof(prefix), "wan%d_", unit)
#define WANDUCK	"Wanduck_Common"
#define APPS_DATA	"Apps_Entry"
#define DUALWAN_DATA "Dualwan_Entry"

#include "asp_handler.h"	//javi
#include "../../../networkmap/networkmap.h"
#include <libutils.h>
#ifdef RTCONFIG_OPENVPN
#include "openvpn_options.h"
#endif
#include <math.h> /*for ceil(), ASUS Router (AiHome) APP */
#include <misc.h>

char SystemCmd[128];

/*krammer add for bug 1321*/
#define WAN_PVC "Wan_PVC"
#define MAX_PVC_NUMBER 11
#define DEFAULT_RT_ATTR_LENGTH 4
#define DEFAULT_RT "DEFAULTROUTE"
#define TO_ASCII_OFFSET 48

#define MAX_ATTRIBUTE_LEN 1024 //Ren
#define SAFE_FREE(x)		if(x){free(x); x = NULL;}	//Andy Chiu, 2015/01/27

#ifdef RTCONFIG_NOTIFICATION_CENTER
#include <libnt.h>
#endif

//Andy Chiu, 2015/01/23.
//use it as printf, such as dbgprintf("test %s %d\n", "string", 123);
//it shows debug messages on console.
void con_dbgprintf (const char * format, ...)
{
	FILE *f;
	int nfd;
	va_list args;
	/* Don't let /dev/console block */
	if((nfd = open("/dev/console", O_WRONLY | O_NONBLOCK)) > 0){
		if((f = fdopen(nfd, "w")) != NULL){
			va_start(args, format);
			vfprintf(f, format, args);
			va_end(args);
			fclose(f);
		}
		close(nfd);
	}
}

//Ren.B
#define websWrite( wp, fmt, args... ) do{sprintf( wp, fmt, ## args );asp_send_response( NULL, wp, strlen(wp) );}while(0)
#define PTM_SIZE	2
#define WIRELESS2G_SIZE	4 //Wifi 2.4G has 4 SSIDs
#define WIRELESS5G_SIZE	4 //Wifi 5G has 4 SSIDs
//Ren.E

#define MAX_LINE_SIZE 512

#define IF_LAN "br0"	//andy chiu, 2014/10/13

/* image path for ASUS Router (AiHome) APP */
#define IMAGE_MODEL_PRODUCT	"/images/Model_product.png"
#define IMAGE_WANUNPLUG		"/images/WANunplug.png" //not support now
#define IMAGE_ROUTER_MODE	"/images/New_ui/rt.jpg"
#define IMAGE_REPEATER_MODE	"/images/New_ui/re.jpg"
#define IMAGE_AP_MODE		"/images/New_ui/ap.jpg"
#define IMAGE_MEDIA_BRIDGE_MODE	"/images/New_ui/mb.jpg" //not support now

#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */

static unsigned char _ctype[] = {
	_C,_C,_C,_C,_C,_C,_C,_C,			/* 0-7 */
	_C,_C|_S,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,		/* 8-15 */
	_C,_C,_C,_C,_C,_C,_C,_C,			/* 16-23 */
	_C,_C,_C,_C,_C,_C,_C,_C,			/* 24-31 */
	_S|_SP,_P,_P,_P,_P,_P,_P,_P,			/* 32-39 */
	_P,_P,_P,_P,_P,_P,_P,_P,			/* 40-47 */
	_D,_D,_D,_D,_D,_D,_D,_D,			/* 48-55 */
	_D,_D,_P,_P,_P,_P,_P,_P,			/* 56-63 */
	_P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,	/* 64-71 */
	_U,_U,_U,_U,_U,_U,_U,_U,			/* 72-79 */
	_U,_U,_U,_U,_U,_U,_U,_U,			/* 80-87 */
	_U,_U,_U,_P,_P,_P,_P,_P,			/* 88-95 */
	_P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,	/* 96-103 */
	_L,_L,_L,_L,_L,_L,_L,_L,			/* 104-111 */
	_L,_L,_L,_L,_L,_L,_L,_L,			/* 112-119 */
	_L,_L,_L,_P,_P,_P,_P,_C,			/* 120-127 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 128-143 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 144-159 */
	_S|_SP,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,   /* 160-175 */
	_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,       /* 176-191 */
	_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,       /* 192-207 */
	_U,_U,_U,_U,_U,_U,_U,_P,_U,_U,_U,_U,_U,_U,_U,_L,       /* 208-223 */
	_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,       /* 224-239 */
	_L,_L,_L,_L,_L,_L,_L,_P,_L,_L,_L,_L,_L,_L,_L,_L	/* 240-255 */
};
#define __ismask(x) (_ctype[(int)(unsigned char)(x)])
#define isalnum(c)	((__ismask(c)&(_U|_L|_D)) != 0)
#define SHUT_RDWR 2
void websRedirect_iframe(char *wp, char *url);
void websApply(char *wp, char *url);
extern	char *get_cgi_json(s_var **vars, char *name, json_object *root);
void json_unescape(char *s);
void decode_json_buffer(char *query);
static int appSet_attribute(char *wp, json_object *root);
static int appDo_rc_service(char *rc_service, char *wp, json_object *root);
char *unqstrstr_t(char *haystack, char *needle);
char *get_arg_t(char *args, char **next);
char *filter(char *string, char c);
//int ejArgs(int argc, char **argv, char *fmt, ...);
char* reverse_str(char *str);
static int check_macrepeat(char *maclist,char *mac);
static int appGet_attribute(char *stream, char *name);
static void appGet_encode(char *str);
static void app_call(char *func, char *stream);
static void do_appGet_cgi (asp_reent* reent, const asp_text* params, asp_text* ret);
static void do_appGet_image_path_cgi (asp_reent* reent, const asp_text* params, asp_text* ret);
static void do_upload_cgi (asp_reent* reent, const asp_text* params, asp_text* ret);


static void get_post_multipart(request *req);
static void get_post(request *req);
static void get_query(request *req);
static int http_header();

void init_asp_funcs(void);
#ifndef TRENDCHIP
void init_asp_constants (void);
#endif
/*transfrom the number to ASCII*/
int decode_uri(char *uri);
static char hex_to_decimal(char char1,char char2);

static void asp_Write (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void request_Form (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void tcWebApi_Set (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void tcWebApi_Get (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void tcWebApi_Unset (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void tcWebApi_Commit (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void tcWebApi_Save (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void tcWebApi_CurrentDefaultRoute(asp_reent * reent, const asp_text * params, asp_text * ret);
static void tcWebApi_MatchThenWrite (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void tcWebApi_clean_get (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void get_post_parameter (asp_reent* reent, const asp_text* params, asp_text* ret);

static void _webs_clean_write(char* buf);
static void get_all_accounts (asp_reent* reent, const asp_text* params, asp_text* ret);

//Sam
#ifdef RTCONFIG_USB
inline int ishex(int x);
int decode(const char *s, char *dec);
static void get_folder_tree (asp_reent* reent, const asp_text* params, asp_text* ret);
static void get_share_tree (asp_reent* reent, const asp_text* params, asp_text* ret);
static void disk_pool_mapping_info (asp_reent* reent, const asp_text* params, asp_text* ret);
static void available_disk_names_and_sizes (asp_reent* reent, const asp_text* params, asp_text* ret);
static void get_AiDisk_status (asp_reent* reent, const asp_text* params, asp_text* ret);
static void set_AiDisk_status (asp_reent* reent, const asp_text* params, asp_text* ret);
static void set_share_mode (asp_reent* reent, const asp_text* params, asp_text* ret);
static void create_sharedfolder (asp_reent* reent, const asp_text* params, asp_text* ret);
static void delete_sharedfolder (asp_reent* reent, const asp_text* params, asp_text* ret);
static void modify_sharedfolder (asp_reent* reent, const asp_text* params, asp_text* ret);
static void get_permissions_of_account (asp_reent* reent, const asp_text* params, asp_text* ret);
static void get_usb_accounts (asp_reent* reent, const asp_text* params, asp_text* ret);
static void create_account (asp_reent* reent, const asp_text* params, asp_text* ret);
static void delete_account (asp_reent* reent, const asp_text* params, asp_text* ret);
static void modify_account (asp_reent* reent, const asp_text* params, asp_text* ret);
static void initial_account (asp_reent* reent, const asp_text* params, asp_text* ret);
static void set_account_permission (asp_reent* reent, const asp_text* params, asp_text* ret);
static void get_printer_info (asp_reent* reent, const asp_text* params, asp_text* ret);
static void safely_remove_disk (asp_reent* reent, const asp_text* params, asp_text* ret);
static void modify_admin_account (asp_reent* reent, const asp_text* params, asp_text* ret);
static void modify_aidisk_account (asp_reent* reent, const asp_text* params, asp_text* ret);
#ifdef RTCONFIG_CLOUDSYNC
static void cloud_status (asp_reent* reent, const asp_text* params, asp_text* ret);
static void UI_cloud_status (asp_reent* reent, const asp_text* params, asp_text* ret);
static void UI_cloud_dropbox_status (asp_reent* reent, const asp_text* params, asp_text* ret);
static void UI_cloud_ftpclient_status (asp_reent* reent, const asp_text* params, asp_text* ret);
static void UI_cloud_sambaclient_status (asp_reent* reent, const asp_text* params, asp_text* ret);
static void UI_cloud_usbclient_status (asp_reent* reent, const asp_text* params, asp_text* ret);
static void UI_rs_status (asp_reent* reent, const asp_text* params, asp_text* ret);
#endif
static void ej_webdavInfo(asp_reent* reent, const asp_text* params, asp_text* ret);
static void apps_info(asp_reent* reent, const asp_text* params, asp_text* ret);
static void apps_action(asp_reent* reent, const asp_text* params, asp_text* ret);
#ifdef RTCONFIG_MEDIA_SERVER
static void dms_info(asp_reent* reent, const asp_text* params, asp_text* ret);
#endif
#ifdef TCSUPPORT_SWAP_FILE
static void do_swap(asp_reent* reent, const asp_text* params, asp_text* ret);
#endif
#endif
#ifdef RTCONFIG_OPENVPN
static void vpn_server_get_parameter(asp_reent* reent, const asp_text* params, asp_text* ret);
static void ej_vpn_crt_server(asp_reent* reent, const asp_text* params, asp_text* ret);
static void ej_vpn_crt_client(asp_reent* reent, const asp_text* params, asp_text* ret);
static void do_vpnupload_cgi(asp_reent* reent, const asp_text* params, asp_text* ret);
static void _save_openvpn_client_cert();
static void _save_openvpn_server_tls();
static void _save_openvpn_server_static();
#endif

static void qis_do_dsl_iptv (asp_reent* reent, const asp_text* params, asp_text* ret);
static void do_apply_cgi (asp_reent* reent, const asp_text* params, asp_text* ret); /* modify for ASUS Router (AiHome) APP */
static void wanstate (asp_reent* reent, const asp_text* params, asp_text* ret);
static void set_current_active_wan(asp_reent* reent, const asp_text* params, asp_text* ret);
static void wl_nband_info(asp_reent* reent, const asp_text* params, asp_text* ret);
static void disable_other_wan (asp_reent* reent, const asp_text* params, asp_text* ret);
static void wanlink (asp_reent* reent, const asp_text* params, asp_text* ret);
static void first_wanlink (asp_reent* reent, const asp_text* params, asp_text* ret);
static void secondary_wanlink (asp_reent* reent, const asp_text* params, asp_text* ret);
static void get_wan_unit_hook (asp_reent* reent, const asp_text* params, asp_text* ret);
static void get_wan_primary_pvcunit_hook (asp_reent* reent, const asp_text* params, asp_text* ret);
static void get_wan_secondary_pvcunit_hook (asp_reent* reent, const asp_text* params, asp_text* ret);
static void setting_lan (asp_reent* reent, const asp_text* params, asp_text* ret);
static void login_state_hook (asp_reent* reent, const asp_text* params, asp_text* ret);
static void ajax_wanstate (asp_reent* reent, const asp_text* params, asp_text* ret);
static void secondary_ajax_wanstate (asp_reent* reent, const asp_text* params, asp_text* ret);
static void stop_service (asp_reent* reent, const asp_text* params, asp_text* ret);
static void sysinfo(asp_reent* reent, const asp_text* params, asp_text* ret);
static void ej_dump(asp_reent* reent, const asp_text* params, asp_text* ret);
static void ej_nvram_get_ddns(asp_reent* reent, const asp_text* params, asp_text* ret);
static void NetworkToolsNetstat(asp_reent* reent, const asp_text* params, asp_text* ret);
static void NetworkToolsAnalysis(asp_reent* reent, const asp_text* params, asp_text* ret);
static void WOL_Invoke(asp_reent* reent, const asp_text* params, asp_text* ret);	//andy chiu, 2014/10/13
static void ClientList_Refresh(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu
static void ClientList_Update(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu
static void set_primary_pvc(asp_reent* reent, const asp_text* params, asp_text* ret);
static void wl_sta_list_2g(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2014/12/02
static void wl_sta_list_5g(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2014/12/02
static void get_client_list(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2014/12/03
static void get_client_list_cache(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2015/03/26
static void get_asus_dev_list(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2014/12/03
static void get_cl_userdef_list(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2014/12/04
static void get_static_dhcp_list(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2014/12/30
static void dhcpLeaseMacList(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2015/08/11
#ifdef ASUS_LOGIN_SESSION
static void IP_dhcpLeaseInfo(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2015/12/03
#endif
#ifdef ASUS_DISK_UTILITY
static void disk_scan(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2015/07/23
static void disk_scan_result(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2015/07/23
#endif
static int get_nat_vserver_table(char *wp);	//Andy Chiu, 2016/03/08
#ifdef TCSUPPORT_WEBMON
static void get_web_history_table(char *wp);
#endif

static void load_parameters_to_generic(asp_reent* reent, const asp_text* params, asp_text* ret);
static void load_parameters_from_generic(asp_reent* reent, const asp_text* params, asp_text* ret);
static void load_MBSSID_parameters_to_generic(asp_reent* reent, const asp_text* params, asp_text* ret);
static void load_MBSSID_parameters_from_generic(asp_reent* reent, const asp_text* params, asp_text* ret);
static void ej_uptime(asp_reent* reent, const asp_text* params, asp_text* ret);
static void wl_get_2G_guestnetwork(asp_reent* reent, const asp_text* params, asp_text* ret);
static void wl_get_5G_guestnetwork(asp_reent* reent, const asp_text* params, asp_text* ret);
static void auto_detection(asp_reent* reent, const asp_text* params, asp_text* ret);
static void wps_info(asp_reent* reent, const asp_text* params, asp_text* ret);
static void wl_auth_list(asp_reent* reent, const asp_text* params, asp_text* ret);
static void tcWebApi_char_to_ascii(asp_reent* reent, const asp_text* params, asp_text* ret);
static void ej_wl_scan_2g(asp_reent* reent, const asp_text* params, asp_text* ret);
static void ej_wl_scan_5g(asp_reent* reent, const asp_text* params, asp_text* ret);
static void tcWebApi_get_arp_list(asp_reent* reent, const asp_text* params, asp_text* ret); /* Paul add 2013/6/17 */
static void get_DSL_WAN_list(asp_reent* reent, const asp_text* params, asp_text* ret);

//Ren.B
static void tcWebApi_UpdateNetdev(asp_reent* reent, const asp_text* params, asp_text* ret);
static void tcWebApi_UpdateBandwidth(asp_reent* reent, const asp_text* params, asp_text* ret);
static void update_variables(asp_reent* reent, const asp_text* params, asp_text* ret);
#ifndef TCSUPPORT_WLAN_RT6856
static void init_wifiIfname();
#endif
static void init_netdevData();
//Ren.E

int char_all_to_ascii(const char *output, const char *input, int outsize);

static void show_message(asp_reent* reent, const asp_text* params, asp_text* ret);//debug purpose
static void show_message2(asp_reent* reent, const asp_text* params, asp_text* ret);//debug purpose
static void show_file_content(asp_reent* reent, const asp_text* params, asp_text* ret);//Ren
static void show_error_msg(const char *const msg_num);
static void ej_memory_usage(asp_reent* reent, const asp_text* params, asp_text* ret);
static void ej_cpu_usage(asp_reent* reent, const asp_text* params, asp_text* ret);
static void factory_telnet(asp_reent* reent, const asp_text* params, asp_text* ret);
static void set_factory_mode(asp_reent* reent, const asp_text* params, asp_text* ret);	//Andy Chiu, 2015/11/02
#ifdef RTCONFIG_SPECTRUM
static void ej_spectrum(asp_reent* reent, const asp_text* params, asp_text* ret);//Ren
#endif
static void ej_check_acpw(asp_reent* reent, const asp_text* params, asp_text* ret);//Ren
static void clean_error_msg();
static char *get_msg_from_dict(char *lang, const char *const msg_name);
void not_ej_initial_folder_var_file();
int dump_file(char *wp, char *filename);
int clean_dump_file(char *wp, char *filename);
void sys_script(char *name);
int gen_modemlog(void);

#ifdef TCSUPPORT_SYSLOG_ENHANCE
void logmessage(char *logheader, char *fmt, ...);
#endif

static char *error_msg_console = NULL, *error_msg = NULL;

#if defined(TCSUPPORT_IMPROVE_GUI_PERFM)
static void tcWebApi_constSet (asp_reent* reent, const asp_text* params,  asp_text* ret);
#endif
static void tcWebApi_staticGet (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void tcWebApi_CommitWithoutSave (asp_reent* reent, const asp_text* params,  asp_text* ret);
static void tcWebApi_delay_CommitWithoutSave (asp_reent* reent, const asp_text* params,  asp_text* ret);

#ifdef RA_PARENTALCONTROL
static void getClientMacAddr(char * ip_addr);
#endif/*RA_PARENTALCONTROL*/
#if defined(TCSUPPORT_WEBSERVER_SSL)
static request* temp_req = NULL;
#endif

int fd_out;
extern char cur_username[129];
#if defined(TCSUPPORT_WEB_SAVE)
extern int flag_save;
#endif

#if defined(TCSUPPORT_GENERAL_MULTILANGUAGE)
extern	int	getString(const char *pkey, char *value);
extern	int	initandparserfile(void);
extern	void	closefp(void);
extern	int	islangChanged(void);
#endif
#ifdef TCSUPPORT_DSL_LINE_DIAGNOSTIC
static void stop_dsl_diag(asp_reent* reent, const asp_text* params, asp_text* ret);
#endif
#ifdef ASUS_BUSYBOX_NEW
static void get_isp_dhcp_opt_list(asp_reent* reent, const asp_text* params, asp_text* ret);
#endif
#ifdef ASUS_LOGIN_SESSION
static void do_login(asp_reent* reent, const asp_text* params, asp_text* ret);
static int login_try;
static int MAX_login;
#endif

static void get_iptv_wan_list_ad(asp_reent* reent, const asp_text* params, asp_text* ret);
#if defined(TCSUPPORT_WAN_PTM) && defined(TCSUPPORT_MULTISERVICE_ON_WAN)
static void get_iptv_wan_list_vd(asp_reent* reent, const asp_text* params, asp_text* ret);
#endif
#if defined(TCSUPPORT_WAN_ETHER) && defined(TCSUPPORT_MULTISERVICE_ON_WAN)
static void get_iptv_wan_list_eth(asp_reent* reent, const asp_text* params, asp_text* ret);
#endif
#ifdef TCSUPPORT_WAN_ETHER_LAN
static void available_eth_lan_wan_port(asp_reent* reent, const asp_text* params, asp_text* ret);
#endif

#define MAX_IF_NUM 4

//Ren.B
typedef struct netDev_s
{
	unsigned long long tx;
	unsigned long long rx;
}netDev_t;

netDev_t WiredData; //eth0.1
netDev_t InternetData;
netDev_t BridgeData;
netDev_t Wireless2GData[MAX_IF_NUM]; //ra00_0, ra00_1, ra00_2, ra00_3
netDev_t Sum2GData;
netDev_t Sum5GData;
netDev_t Wireless5GData[MAX_IF_NUM]; //ra01_0, ra01_1, ra01_2, ra01_3
netDev_t ATMData; //nas0
netDev_t PTMData[2]; //nas8, nas9
netDev_t EthernetWANData; //nas10
//Ren.E

int isPPPoA_boa = 0;
const char cl_cache_path[] = "/var/tmp/cl_cache"; /* for ASUS Router (AiHome) APP */

//Ren.B
typedef struct wifiIfName_s
{
	unsigned int count;
	char ifnames[MAX_IF_NUM][16];
}wifiIfName_t;

wifiIfName_t Wifi2Gifname;
wifiIfName_t Wifi5Gifname;
//Ren.E

/* for web history */
typedef struct clientInfo_s
{
	char ipaddr[64];
	char mac[32];
}clientInfo_t;
clientInfo_t landev[512];

int asp_handler(request * req)
{
	int ret, i;
	char nodename[15], username[129];

	memset(username, 0, sizeof(username));
	//fprintf(stderr,"%s mothed:%d %s \n",__FUNCTION__,req->method,req->pathname);
	//dbgprintf("**req->pathname: %s\n", req->pathname);
	for(i = 0 ; i < 1; i++){
		sprintf(nodename, "Account_Entry%d", i);
		ret = tcapi_get(nodename, "username", username);
		if(ret < 0){ //Fail to acquire username from romfile
			fprintf(stderr, "Fail to acquire username from romfile: ret = %d\r\n", ret);
			return -1;
		}
		if(!strcmp(cur_username, username)){
			sprintf(nodename, "%d", i);
			ret = tcapi_set("WebCurSet_Entry", "CurrentAccess", nodename);
			if(ret < 0){ //Fail to set current username
				fprintf(stderr, "Fail to set current username: ret = %d\r\n", ret);
				return -1;
			}
			tcapi_get("WebCurSet_Entry", "CurrentAccess", username);
			break;
		}
	}

	//special case: logout. Sam, 2013/7/3
	if(strstr(req->pathname, "Logout.asp"))
	{
#ifdef ASUS_LOGIN_SESSION		
		//Andy Chiu, 2015/12/14. remove token
		dbgprintf("[%s, %d]reset data for logout.\n", __FUNCTION__, __LINE__);
		RM_TOKEN_ITEM_BY_IP(req->remote_ip_addr);
#endif
	}

	g_var_post = NULL;
#if defined(TCSUPPORT_WEBSERVER_SSL)
	temp_req = req;
	dontDoAsp = 0;
#endif
	fd_out = req->fd;
#ifdef ASUS_LOGIN_SESSION		
	if(!strstr(req->pathname, "login.asp"))
	{
#endif
	if(http_header() == -1)
	{
		close(fd_out);
		req->status = DEAD;
		req->keepalive = KA_INACTIVE;//Ian_20070326
		return 0;//Ian_20070326
	}
#ifdef ASUS_LOGIN_SESSION		
	}
#endif

	if((req->method == M_POST) && req->content_length){
		if(req->content_type == NULL)
		{
			if(req->query_string)	//Sam, 2013/7/3
				get_query(req);
			else
				get_post(req);
		}
		/*add support to parse multipart. shnwind 2009.4.8*/
		else if(strstr(req->content_type,"multipart") == NULL){
			get_post(req);
		}else{
			get_post_multipart(req);
		}
	}else if((req->method == M_GET) && req->query_string)
		get_query(req);

//	init_asp_funcs ();
//    init_asp_constants();
#ifdef RA_PARENTALCONTROL
	/* Modify for taking CGI environment to ASP handler by richard.lai */
	do_asp (req, req->pathname);
#else
	do_asp (NULL, req->pathname);
#endif/*RA_PARENTALCONTROL*/
#if defined(TCSUPPORT_WEB_SAVE)
		if(flag_save == 1){
			flag_save = 0;

			tcapi_save();
		}
#endif
	#ifdef TRENDCHIP
	if(dontDoAsp !=0){
		dontDoAsp=0;
	}
	#endif
	free_param_line(g_var_post);
	req->cgi_status = CGI_DONE;
	req->status = DONE;
	close(fd_out);
	//fprintf(stderr,"%s END \n",__FUNCTION__);
	req->keepalive = KA_INACTIVE;//Ian_20070326
	return 0;//Ian_20070326
}

static void get_post_multipart(request *req){

	//char boundary[64];
	char boundary[256] = {0};
	char *c_ret, *post;
	int fd, content_len;
	struct stat buf_st;
	int i, len_bound;

	/*get boundary*/
	c_ret = strstr(req->content_type,"boundary=");
	if(c_ret != NULL){
		len_bound = strlen(c_ret);
		if(len_bound <= sizeof(boundary)){
			for(i = strlen("boundary=") ; i < len_bound ;i++){
				/*do not need "-------"*/
				if(c_ret[i] != '-'){
					break;
				}
			}
			strcpy(boundary,c_ret+i);
			//dbgprintf("bound = %s\n",boundary);
		}else{
			dbgprintf("boundary too long!!!upgrade fail\n");
			return;
		}
	}
	/*paser all item information*/
	multipart_parser(req, boundary);
	/*get all item information*/
	if(stat(MULTI_TO_NORMAL_TEMP, &buf_st) != 0){
		return ;
	}
	content_len = buf_st.st_size;
	post = (char*)malloc(content_len+1);
	fd = open(MULTI_TO_NORMAL_TEMP,O_RDONLY);
	if(fd < 0){
		free(post);
		return;
	}
	if(read(fd,post,content_len) <= 0){
		free(post);
		close(fd);
		return;
	}
	close(fd);
	g_var_post = parse_param_line (post);
	free(post);
	//move to cfg_manager
	//unlink(MULTI_TO_NORMAL_TEMP);
}


static void get_post(request *req)
{
	char *post;
	int ret,len;

	len = atoi(req->content_length);
	post = (char*)malloc(len+1);
	memset(post,0,len+1);

	lseek(req->post_data_fd, 0,SEEK_SET);
	ret = read(req->post_data_fd,post,len);
	if(ret<=0)
		goto exit;

#ifdef ASUS_LOGIN_SESSION
	//Andy Chiu, 2015/12/30. For app login
	if(strstr(req->request_uri, "login.asp")
		/* for ASUS Router (AiHome) APP */
		|| strstr(req->request_uri, "appGet.asp")
		|| strstr(req->request_uri, "Applyapp.asp"))
	{
		decode_uri(post);
	}
#endif

	g_var_post = parse_param_line (post);

exit:
	#if 0  //lee 2006-11-27
	close(req->post_data_fd);
	req->post_data_fd = 0;
	#endif
	free(post);
}

static void get_query(request *req)
{
#if 0
	char *query;
	int len;

	len = strlen(req->query_string);
	query =(char*) malloc(len+1);
	memcpy(query,req->query_string,len);
	//fprintf(stderr,"query:%s %d\n",query,len);
	g_var_post = parse_param_line (query);
	free(query);
#endif
	g_var_post = parse_param_line (req->query_string);
}

static int http_header()
{
	char buf[512] = {0};
	char *p = buf;
	char HTTP_OK[] = "HTTP/1.0 200 OK\r\n";
	strcat(p, HTTP_OK);
	p += strlen(HTTP_OK);

	char DATA[] = "Date: "
		"                             "
		"\r\n";
	rfc822_time_buf(DATA + 6, 0);
	strcat(p, DATA);
	p += strlen(DATA);

/*pork 20090309 added*/
#if 0
	char CONTENT_TYPE[] = "Content-type: text/html;charset=GB2312\r\n\r\n";
#else
	char CONTENT_TYPE[64];
	if(charset){
		sprintf(CONTENT_TYPE,"Content-type: text/html;charset=%s\r\n\r\n",charset);
	}
	else{
		/*default char set: ISO-8859-1*/
		sprintf(CONTENT_TYPE,"Content-type: text/html;charset=ISO-8859-1\r\n\r\n");
	}
#endif
/*pork 20090309 added over*/
	strcat(p, CONTENT_TYPE);
	p += strlen(CONTENT_TYPE);

	//if(asp_send_response (NULL, HTTP_OK, strlen(HTTP_OK)) == -1)
		//return -1;

	//if(asp_send_response (NULL, CONTENT_TYPE, strlen(CONTENT_TYPE)) ==-1)
		//return -1;

	if(asp_send_response (NULL, buf, strlen(buf)) ==-1)
		return -1;

	return 0;
}
#ifndef TRENDCHIP
static void Request_Form(asp_reent* reent, int id, const asp_text* params,  asp_text* ret)
{
	char *val,*tmp;

	//fprintf(stderr, "(%s ID: %d ", __FUNCTION__, id);
    ret->str ="";
    ret->len = 0;
    if (params->str && params->len)
    {
    	//fwrite(params->str, params->len, 1, stderr);

    	tmp = (char*)asp_alloc(reent,params->len+1);
    	memset(tmp,0,params->len+1);
    	memcpy(tmp,params->str,params->len);
    	val = get_param(g_var_post,tmp);
    	if(val ==NULL)
    		return;

		ret->str = val;
		ret->len = strlen(val);
    }
    //else
    //	fprintf (stderr, "No Params )");
    //fprintf (stderr, "\n");

}

extern asp_reent *my_reent;
static void TcWebApi_set (asp_reent* reent, int id, const asp_text* params,  asp_text* ret)
{
	char *tmp;

    //fprintf(stderr, "(%s ID: %d ", __FUNCTION__, id);
    if (params->str && params->len)
    {
      //fwrite(params->str, params->len, 1, stderr);

      tmp = (char*)asp_alloc(reent,params->len+1);
      memset(tmp,0,params->len+1);
      memcpy(tmp,params->str,params->len);
      my_reent = reent;
      asp_TCWebApi_set(id,tmp);
    }
    //else
    //	fprintf (stderr, "No Params )");
    //fprintf (stderr, "\n");

}


static void TcWebApi_get (asp_reent* reent, int id, const asp_text* params,  asp_text* ret)
{
	char *val;

	#if 0
    fprintf(stderr, "(%s ID: %d ", __FUNCTION__, id);

    if (params->str && params->len)
      fwrite(params->str, params->len, 1, stderr);
    else
      fprintf (stderr, "No Params )");
    fprintf (stderr, "\n");
    #endif

    my_reent = reent;
    val = asp_TCWebApi_get(id);
    if(val == NULL)
    {
    	ret->str = "";
    	ret->len = 0;
    }
    else
    {
    	ret->str = val;
		ret->len = strlen(val);
    }

}

static void TcWebApi_execute (asp_reent* reent, int id, const asp_text* params,  asp_text* ret)
{

    //fprintf(stderr, "%s ID: %d \n", __FUNCTION__, id);
    my_reent = reent;
    asp_TCWebApi_execute(id);
}
#endif

static void _webs_clean_write(char* buf)
{
	char *c;
	char wp[256];

	for (c = buf; *c; c++) {
		if (isprint(*c) &&
			*c != '"' && *c != '&' && *c != '<' && *c != '>'
			)
			websWrite(wp, "%c", *c);
		else
			websWrite(wp, "&#%d", *c);
	}
}

/* should be called before disable_other_wan */
static void
set_dsl_restart_flag (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char *wanVCFlag = get_param(g_var_post, "wanVCFlag");
	char transmode[8] = {0};
	int current_wanmode = -1;
	int do_restart = 1;

	if(wanVCFlag && !strcmp(wanVCFlag, "3")) // apply wan
	{
		tcapi_get("Wan_Common", "TransMode", transmode);
		current_wanmode = getTransferMode();

		switch (current_wanmode)
		{
			case TMode_ADSL:
				if (!strncmp(transmode, "ATM", 3))
					do_restart = 0;
				break;

			case TMode_VDSL:
				if (!strncmp(transmode, "PTM", 3))
					do_restart = 0;
				break;
		}

		if (do_restart)
		{
			tcapi_set("GUITemp_Entry0", "dsltmp_dsl_restart", "1");
		}
		else
		{
			tcapi_set("GUITemp_Entry0", "dsltmp_dsl_restart", "0");
		}
	}
}

static void
ej_should_wan_pvc_do_commit (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char *value = NULL;
	char *v_tmp = NULL;

	ret->str = "1";
	ret->len = 1;

#ifdef RTCONFIG_DUALWAN
	if (params[0].str && params[0].len)
	{
		value = (char*)asp_alloc(reent, params[0].len+1);
		memset(value, 0, params[0].len+1);
		memcpy(value, params[0].str, params[0].len);
		value[params[0].len]='\0';

		v_tmp = get_param(g_var_post, value); //get_param on web page
		if(v_tmp != NULL)
		{
			if (is_running_as_second_wan(atoi(v_tmp)))
			{
				ret->str = "0";
				ret->len = 1;
			}
		}
	}
#endif
}

static void
ej_should_dsl_do_commit (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char dsltmp_dsl_restart[8] = {0};

	ret->str = "0";
	ret->len = 1;

	tcapi_get("GUITemp_Entry0", "dsltmp_dsl_restart", dsltmp_dsl_restart);

	if (atoi(dsltmp_dsl_restart) == 1)
	{
		ret->str = "1";
	}
}

static void
qis_dsl_early_restart  (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char *dsltmp_transfer_mode = get_param(g_var_post, "dsltmp_transfer_mode");
	char lineState[32] = {0};
	char DSLMode[32] = {0};
	int do_restart = 0;
	int r_val = 0;

	tcapi_get("Info_Adsl", "lineState", lineState);
	tcapi_get("Wan_Common", "DSLMode", DSLMode);

	//cprintf("\n\n\n *****dsltmp_transfer_mode(%s),lineState(%s), xDSLmode(%s)*****\n\n\n", 
	//	dsltmp_transfer_mode, lineState, DSLMode);

	//only change between ATM and PTM
	if ((dsltmp_transfer_mode != NULL) && (!strcmp(lineState, "up")))
	{
		if ((!strcmp(dsltmp_transfer_mode, "ATM") && !strcmp(DSLMode, "VDSL")) ||
			(!strcmp(dsltmp_transfer_mode, "PTM") && !strcmp(DSLMode, "ADSL")))
		{
			if (!strcmp(dsltmp_transfer_mode, "PTM"))
				tcapi_set("Wan_Common","DSLMode","VDSL");
			else
				tcapi_set("Wan_Common","DSLMode","ADSL");
			do_restart = 1;
		}
	}
	//enable g.inp case
	if(tcapi_match("GUITemp_Entry0", "dsltmp_set_ginp", "1")) {
		do_restart = 1;
		tcapi_set("GUITemp_Entry0", "dsltmp_set_ginp", "0");
	}
	//Thailand 3BB case
	if(tcapi_match("GUITemp_Entry0", "dsltmp_cfg_th3bb", "1")) {
		do_restart = 1;
		tcapi_set("GUITemp_Entry0", "dsltmp_cfg_th3bb", "0");
	}
	//Germany VDSL profile case
	if(tcapi_match("GUITemp_Entry0", "dsltmp_cfg_de17a", "1")) {
		do_restart = 1;
		tcapi_set("GUITemp_Entry0", "dsltmp_cfg_de17a", "0");
	}

	if (do_restart == 1)
	{
		tcapi_set("GUITemp_Entry0", "dsltmp_dsl_early_restart", "1");
		r_val = tcapi_commit("Adsl_Entry");
		cprintf("commit Adsl_Entry return %d\n", r_val);
	}
	else
	{
		tcapi_set("GUITemp_Entry0", "dsltmp_dsl_early_restart", "0");
	}
}

extern void http_update_allowed_client(void);
static void update_http_clientlist(asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	http_update_allowed_client();
}


static void get_wl_channel_skiplist_5g(asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	skip_type_t skip_type = SKIP_TYPE_NONE;
	channel_list_t channel;

	char tmp[512];
	char countryCode[4]= {0};
	char Channel_str[256]= {0};
	char ProductName[32]= {0};
	char BootLoaderVer[16]= {0};
	char *c_country = countryCode;
	int useDFS = 0;

	tcapi_get("SysInfo_Entry", "ProductName", ProductName);
	tcapi_get("SysInfo_Entry", "BootLoaderVer", BootLoaderVer);
	tcapi_get("WLan_Common", "wl1_CountryCode", countryCode);	//5G
	
	useDFS = check_SupportDFS(ProductName, BootLoaderVer);	

	if (check_EnableDFSforAPAC(ProductName, BootLoaderVer) &&
		!strcasecmp(countryCode, REGION_APAC))
		c_country = REGION_EU;

	//skip channel according to bandwitdh limit
	if (!strcasecmp(c_country, REGION_EU))
	{
		//gen skip list for 20Mhz
		if (useDFS)
			skip_type |= SKIP_TYPE_WEATHER;

		bzero(tmp, sizeof(tmp));
		getChannelList_Skip(skip_type, &channel);
		convert_Channel2str(&channel, ",", Channel_str, sizeof(Channel_str));
		websWrite(tmp, "\"20\": [%s], ", Channel_str);

		//gen skip list for 40Mhz
		skip_type= SKIP_TYPE_CE_BW40;
		if (useDFS)
			skip_type |= SKIP_TYPE_WEATHER_BW4080;

		bzero(tmp, sizeof(tmp));
		getChannelList_Skip(skip_type, &channel);
		convert_Channel2str(&channel, ",", Channel_str, sizeof(Channel_str));
		websWrite(tmp, "\"40\": [%s], ", Channel_str);

		//gen skip list for 80Mhz
		skip_type = SKIP_TYPE_CE_BW80;
		if (useDFS)
			skip_type |= SKIP_TYPE_WEATHER_BW4080;

		bzero(tmp, sizeof(tmp));
		getChannelList_Skip(skip_type, &channel);
		convert_Channel2str(&channel, ",", Channel_str, sizeof(Channel_str));
		websWrite(tmp, "\"80\": [%s]", Channel_str);
	}
	else if (!strcasecmp(c_country, REGION_APAC) ||
		!strcasecmp(c_country, REGION_AU))
	{
		skip_type = SKIP_TYPE_SGAU_BW4080;
		websWrite(tmp, "\"20\": [%s], ", Channel_str);

		bzero(tmp, sizeof(tmp));
		getChannelList_Skip(skip_type, &channel);
		convert_Channel2str(&channel, ",", Channel_str, sizeof(Channel_str));
		websWrite(tmp, "\"40\": [%s], ", Channel_str);
		websWrite(tmp, "\"80\": [%s]", Channel_str);
	}
}


static void get_wl_channel_list_5g(asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char countryCode[4] ={0};
	char bandwidth_str[4]={0};
	skip_type_t skip_type = SKIP_TYPE_NONE;
	int bandwidth = 0;

	region_condition_t condition;
	channel_list_t channel;
	char Channel_str[256] ={0};
	char ProductName[32] ={0};
	char BootLoaderVer[16] ={0};
	int wifi_region;
	int useDFS = 0;

	tcapi_get("WLan_Common", "wl1_CountryCode", countryCode);	//5G
	tcapi_get("WLan_Common", "wl1_HT_BW", bandwidth_str);	//5G
	bandwidth =  atoi(bandwidth_str);

	//skip channel according to bandwitdh limit
#if 0
	if (!strcasecmp(countryCode, "CE"))
	{
		if (bandwidth == 2)	//40Mhz
		{
			skip_type = SKIP_TYPE_CE_BW40;
		}
		else if (bandwidth == 1 || bandwidth == 3)	//80Mhz
		{
			skip_type = SKIP_TYPE_CE_BW80;
		}
	}
	else if (!strcasecmp(countryCode, "SG") ||
		!strcasecmp(countryCode, "AU"))
	{
		if (bandwidth >= 1 && bandwidth <=3)	//40Mhz or 80 Mhz
		{
			skip_type = SKIP_TYPE_SGAU_BW4080;
		}
	}
#endif

	//get country region
	tcapi_get("SysInfo_Entry", "ProductName", ProductName);
	tcapi_get("SysInfo_Entry", "BootLoaderVer", BootLoaderVer);

	/* Mark DFS check code, currently dose not show DFS channel in GUI */
	useDFS = check_SupportDFS(ProductName, BootLoaderVer);	
	condition.EnableDFSforAPAC = check_EnableDFSforAPAC(ProductName, BootLoaderVer);
	condition.UseCEforAPAC = check_UseCEforAPAC(ProductName);
	condition.SupportDFS = useDFS;
	
	wifi_region = getCountryRegion5G(countryCode, &condition);

	if (useDFS)
		skip_type |= SKIP_TYPE_WEATHER;

	getChannelList5G(wifi_region, &channel, skip_type);
	convert_Channel2str(&channel, ",", Channel_str, sizeof(Channel_str));
	ret->str = Channel_str;
	ret->len = strlen(Channel_str);

	asp_send_response (NULL, ret->str, ret->len);
}

static void
ej_support_dfs (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char ProductName[32];
	char BootLoaderVer[16];
	char CountryCode[8];

	ret->str = "0";
	ret->len = 1;

	bzero(ProductName, sizeof(ProductName));
	bzero(BootLoaderVer, sizeof(BootLoaderVer));
	bzero(CountryCode, sizeof(CountryCode));

	//get country region
	tcapi_get("SysInfo_Entry", "ProductName", ProductName);
	tcapi_get("SysInfo_Entry", "BootLoaderVer", BootLoaderVer);
	tcapi_get("WLan_Common", "CountryCode", CountryCode);	//only 5G support DFS

	if (check_SupportDFS_by_Region(ProductName, BootLoaderVer, CountryCode) == 1)
	{
		ret->str = "1";
	}

	asp_send_response (NULL, ret->str, ret->len);
}


void init_asp_funcs(void)
{
    #ifndef TRENDCHIP
    append_asp_func ("Request.Form", Request_Form);
    append_asp_func ("TcWebApi_set", TcWebApi_set);
    append_asp_func ("TcWebApi_get", TcWebApi_get);
    append_asp_func ("TcWebApi_execute", TcWebApi_execute);
    #else
    /*new tcWebApi*/
    append_asp_func ("asp_Write", asp_Write);
    append_asp_func ("request_Form", request_Form);
    append_asp_func ("tcWebApi_Set", tcWebApi_Set);
    append_asp_func ("tcWebApi_Get", tcWebApi_Get);
    append_asp_func ("tcWebApi_Unset", tcWebApi_Unset);
    append_asp_func ("tcWebApi_Commit", tcWebApi_Commit);
    append_asp_func ("tcWebApi_Save", tcWebApi_Save);
/*krammer add for bug 1321*/
    append_asp_func("tcWebApi_CurrentDefaultRoute", tcWebApi_CurrentDefaultRoute);
    append_asp_func("get_parameter", get_post_parameter);
	append_asp_func("tcWebApi_MatchThenWrite", tcWebApi_MatchThenWrite);
	append_asp_func("tcWebApi_clean_get", tcWebApi_clean_get);
	append_asp_func("get_all_accounts", get_all_accounts);
	//Sam
#ifdef RTCONFIG_USB
	append_asp_func("get_folder_tree", get_folder_tree);
	append_asp_func("get_share_tree", get_share_tree);
	append_asp_func("disk_pool_mapping_info", disk_pool_mapping_info);
	append_asp_func("available_disk_names_and_sizes", available_disk_names_and_sizes);
	append_asp_func("get_AiDisk_status", get_AiDisk_status);
	append_asp_func("set_AiDisk_status", set_AiDisk_status);
	append_asp_func("set_share_mode", set_share_mode);
	append_asp_func("create_sharedfolder", create_sharedfolder);
	append_asp_func("delete_sharedfolder", delete_sharedfolder);
	append_asp_func("modify_sharedfolder", modify_sharedfolder);
	append_asp_func("get_permissions_of_account", get_permissions_of_account);
	append_asp_func("get_usb_accounts", get_usb_accounts);
	append_asp_func("create_account", create_account);
	append_asp_func("delete_account", delete_account);
	append_asp_func("modify_account", modify_account);
	append_asp_func("initial_account", initial_account);
	append_asp_func("set_account_permission", set_account_permission);
	append_asp_func("get_printer_info", get_printer_info);
	append_asp_func("safely_remove_disk", safely_remove_disk);
#ifdef RTCONFIG_CLOUDSYNC
	append_asp_func("cloud_status", cloud_status);
	append_asp_func("UI_cloud_status", UI_cloud_status);
	append_asp_func("UI_cloud_dropbox_status", UI_cloud_dropbox_status);
	append_asp_func("UI_cloud_ftpclient_status", UI_cloud_ftpclient_status);
	append_asp_func("UI_cloud_sambaclient_status", UI_cloud_sambaclient_status);
	append_asp_func("UI_cloud_usbclient_status", UI_cloud_usbclient_status);
	append_asp_func("UI_rs_status", UI_rs_status);
#endif
	append_asp_func("getWebdavInfo", ej_webdavInfo);
#ifdef RTCONFIG_MEDIA_SERVER
	append_asp_func("dms_info", dms_info);
#endif
	append_asp_func("apps_info", apps_info);
	append_asp_func("apps_action", apps_action);
#ifdef TCSUPPORT_SWAP_FILE
	append_asp_func ("do_swap", do_swap);
#endif
	append_asp_func ("modify_admin_account", modify_admin_account);
	append_asp_func ("modify_aidisk_account", modify_aidisk_account);
#endif
#ifdef RTCONFIG_OPENVPN
	append_asp_func("vpn_server_get_parameter", vpn_server_get_parameter);
	append_asp_func("vpn_crt_server", ej_vpn_crt_server);
	append_asp_func("vpn_crt_client", ej_vpn_crt_client);
	append_asp_func("vpnupload_cgi", do_vpnupload_cgi);
#endif
	append_asp_func("qis_do_dsl_iptv", qis_do_dsl_iptv);
	/* for ASUS Router (AiHome) APP */
	append_asp_func("do_apply_cgi", do_apply_cgi);
	append_asp_func("do_appGet_cgi", do_appGet_cgi);
	append_asp_func("do_appGet_image_path_cgi", do_appGet_image_path_cgi);
	append_asp_func("do_upload_cgi", do_upload_cgi);
	append_asp_func("wanstate", wanstate);
	append_asp_func("set_current_active_wan", set_current_active_wan);
	append_asp_func("wl_nband_info", wl_nband_info);
	append_asp_func("disable_other_wan", disable_other_wan);
	append_asp_func("wanlink", wanlink);
	append_asp_func("first_wanlink", first_wanlink);
	append_asp_func("secondary_wanlink", secondary_wanlink);
	append_asp_func("get_wan_unit", get_wan_unit_hook);
	append_asp_func("get_wan_primary_pvcunit", get_wan_primary_pvcunit_hook);
	append_asp_func("get_wan_secondary_pvcunit", get_wan_secondary_pvcunit_hook);
	append_asp_func("setting_lan", setting_lan);
	append_asp_func("login_state_hook", login_state_hook);
	append_asp_func("ajax_wanstate", ajax_wanstate);
	append_asp_func("secondary_ajax_wanstate", secondary_ajax_wanstate);
	append_asp_func("stop_service", stop_service);
	append_asp_func("sysinfo", sysinfo);
	append_asp_func("nvram_dump", ej_dump);
	append_asp_func("nvram_get_ddns", ej_nvram_get_ddns);
	append_asp_func ("tcWebApi_get_arp_list", tcWebApi_get_arp_list); /* Paul add 2013/6/17 */
	append_asp_func ("NetworkToolsNetstat", NetworkToolsNetstat);
	append_asp_func ("NetworkToolsAnalysis", NetworkToolsAnalysis);
	append_asp_func ("get_DSL_WAN_list", get_DSL_WAN_list);
	append_asp_func("WOL_Invoke", WOL_Invoke);  //Andy Chiu add 2014/10/13
	append_asp_func("ClientList_Refresh", ClientList_Refresh);	//Andy Chiu
	append_asp_func("ClientList_Update", ClientList_Update);	//Andy Chiu
	append_asp_func("set_primary_pvc", set_primary_pvc);
	append_asp_func("wl_sta_list_2g", wl_sta_list_2g);	//Andy Chiu, 2014/12/02
	append_asp_func("wl_sta_list_5g", wl_sta_list_5g);	//Andy Chiu, 2014/12/02
	append_asp_func("get_client_list", get_client_list);	//Andy Chiu, 2014/12/03
	append_asp_func("get_client_list_cache", get_client_list_cache);	//Andy Chiu, 2015/03/26
	append_asp_func("get_asus_dev_list", get_asus_dev_list);	//Andy Chiu, 2014/12/03
	append_asp_func("get_cl_userdef_list", get_cl_userdef_list);	//Andy Chiu, 2014/12/04
	append_asp_func("get_static_dhcp_list", get_static_dhcp_list);	//Andy Chiu, 2014/12/30
	append_asp_func("dhcpLeaseMacList", dhcpLeaseMacList);	//Andy Chiu, 2015/08/11	
#ifdef ASUS_LOGIN_SESSION
	append_asp_func("IP_dhcpLeaseInfo", IP_dhcpLeaseInfo);	//Andy Chiu, 2015/12/03	
#endif
#ifdef ASUS_DISK_UTILITY
	append_asp_func("disk_scan", disk_scan);	//Andy Chiu, 2015/07/22
	append_asp_func("disk_scan_result", disk_scan_result);	//Andy Chiu, 2015/07/23
#endif	
	append_asp_func ("load_parameters_to_generic", load_parameters_to_generic);
	append_asp_func ("load_parameters_from_generic", load_parameters_from_generic);
	append_asp_func ("load_MBSSID_parameters_to_generic", load_MBSSID_parameters_to_generic);
	append_asp_func ("load_MBSSID_parameters_from_generic", load_MBSSID_parameters_from_generic);
	append_asp_func ("uptime", ej_uptime);
	append_asp_func ("wl_get_2G_guestnetwork", wl_get_2G_guestnetwork);
	append_asp_func ("wl_get_5G_guestnetwork", wl_get_5G_guestnetwork);
	append_asp_func ("auto_detection", auto_detection);
	append_asp_func ("wps_info", wps_info);
	append_asp_func ("wl_auth_list", wl_auth_list);
	append_asp_func ("tcWebApi_char_to_ascii", tcWebApi_char_to_ascii);
	append_asp_func ("wl_scan_2g", ej_wl_scan_2g);
	append_asp_func ("wl_scan_5g", ej_wl_scan_5g);

	append_asp_func ("tcWebApi_UpdateNetdev", tcWebApi_UpdateNetdev); //Ren
	append_asp_func ("tcWebApi_UpdateBandwidth", tcWebApi_UpdateBandwidth); //Ren
	append_asp_func ("update_variables", update_variables); //Ren
	append_asp_func ("show_message", show_message); //debug purpose
	append_asp_func ("show_message2", show_message2); //debug purpose
	append_asp_func ("show_file_content", show_file_content); //Ren
	append_asp_func ("memory_usage", ej_memory_usage); //Ren
	append_asp_func ("cpu_usage", ej_cpu_usage); //Ren
	append_asp_func ("factory_telnet", factory_telnet); //Ren
	append_asp_func ("set_factory_mode", set_factory_mode);	//Andy Chiu, 2015/11/02
#ifdef RTCONFIG_SPECTRUM
	append_asp_func ("spectrum", ej_spectrum); //Ren
#endif
	append_asp_func ("check_acpw", ej_check_acpw); //Ren
	append_asp_func ("gen_modemlog", gen_modemlog);

#if defined(TCSUPPORT_IMPROVE_GUI_PERFM)
    append_asp_func ("tcWebApi_constSet", tcWebApi_constSet);
#endif
    append_asp_func ("tcWebApi_staticGet", tcWebApi_staticGet);

    append_asp_func ("tcWebApi_CommitWithoutSave", tcWebApi_CommitWithoutSave);
    append_asp_func ("tcWebApi_delay_CommitWithoutSave", tcWebApi_delay_CommitWithoutSave);
    append_asp_func ("should_wan_pvc_do_commit", ej_should_wan_pvc_do_commit);
    append_asp_func ("set_dsl_restart_flag", set_dsl_restart_flag);
    append_asp_func ("should_dsl_do_commit", ej_should_dsl_do_commit);
    append_asp_func ("update_http_clientlist", update_http_clientlist);
    append_asp_func ("get_wl_channel_list_5g", get_wl_channel_list_5g);
    append_asp_func ("get_wl_channel_skiplist_5g", get_wl_channel_skiplist_5g);
    append_asp_func ("qis_dsl_early_restart", qis_dsl_early_restart);
    append_asp_func ("support_dfs", ej_support_dfs);
#endif
#ifdef TCSUPPORT_DSL_LINE_DIAGNOSTIC
    append_asp_func ("stop_dsl_diag", stop_dsl_diag);
#endif
#ifdef ASUS_BUSYBOX_NEW
    append_asp_func ("get_isp_dhcp_opt_list", get_isp_dhcp_opt_list);	
#endif
#ifdef ASUS_LOGIN_SESSION
    append_asp_func ("do_login", do_login);
#endif
    append_asp_func ("get_iptv_wan_list_ad", get_iptv_wan_list_ad);
#if defined(TCSUPPORT_WAN_PTM) && defined(TCSUPPORT_MULTISERVICE_ON_WAN)
    append_asp_func ("get_iptv_wan_list_vd", get_iptv_wan_list_vd);
#endif
#if defined(TCSUPPORT_WAN_ETHER) && defined(TCSUPPORT_MULTISERVICE_ON_WAN)
    append_asp_func ("get_iptv_wan_list_eth", get_iptv_wan_list_eth);
#endif
#ifdef TCSUPPORT_WAN_ETHER_LAN
    append_asp_func ("available_eth_lan_wan_port", available_eth_lan_wan_port);
#endif
}

static void wait_fd (int fd)
{
	fd_set block_write_fdset,block_read_fdset;
	#ifdef TRENDCHIP
	struct timeval timeout={FAIL_SESSION_TIMEOUT,0};
	int ret=0;
	#endif

	while (1)
	{
		FD_ZERO(&block_write_fdset);
		FD_SET (fd, &block_write_fdset);
		FD_ZERO(&block_read_fdset);
		FD_SET (fd, &block_read_fdset);
	 	#ifdef TRENDCHIP
                /*
                   krammer change for bug 1094
                   we make select to have a timeout, if timeout, that means the session has some
                   thing wrong, so we break wait fd and dont do any asp until new session.
                */
            	if ((ret=select(fd + 1, &block_read_fdset,
                   &block_write_fdset, NULL,
                   &timeout)) <0 ) {
		#else
		if (select(fd + 1, &block_read_fdset,
                   &block_write_fdset, NULL,
                   0) <=0 ) {
              #endif
            /* what is the appropriate thing to do here on EBADF */
            if (errno == EINTR)
            {
                continue;   /* while(1) */
            }
            else if (errno != EBADF) {
                fprintf(stderr,"select error \n");
            }
        }
		#ifdef TRENDCHIP
		if(ret==0){/*timeout*/
			dontDoAsp=1;
			break;
		}
		#endif
		if (FD_ISSET(fd, &block_read_fdset))
			break;

		if (FD_ISSET(fd, &block_write_fdset))
			break;
	}

}


int asp_send_response (asp_reent* reent, const void* buffer, size_t len)
{

	int res=0;
	#ifdef TRENDCHIP
	 /*
           krammer add for bug 1094, this flag can block asp_send_response when
           wait_fd is timeout and change_ip flag is set.
        */
	if(dontDoAsp){
		return 0;
	}
	#endif

loop_write:

	len =len-res;
	buffer = buffer+res;

	wait_fd (fd_out);
#if  defined(TCSUPPORT_WEBSERVER_SSL)
	if(temp_req->ssl == NULL)
#endif
	{
	res = write(fd_out,buffer,len);
	}
#if defined(TCSUPPORT_WEBSERVER_SSL)
	else{
		if(len <=0)
			return 0;
		res = boa_sslWrite(temp_req->ssl, buffer,len);
		if(res<0)
		{
			dontDoAsp = 1;
		}
	}
#endif

  if(res <0 )
  {
  	return -1;
  }
  if(res<len)
  	goto loop_write;

  return 0;



}
#ifndef TRENDCHIP
void init_asp_constants (void)
{
  //sample append_asp_constant ("WAN_SETTING",					0);

}
#endif
#ifdef TRENDCHIP
/*____________________________________________________________________*
**	function name: asp_Write
**
**	description:
*     Output the string to web page.
*
**	parameters:
*     reent:not use
*     id:not use
*     params:params[0] will save the string user input
*     ret:not use
**	global:
*     none
**	return:
*     0:successful
*     -1:fail
**	call:
*     none
**	revision:
*     1.shnwind
* _______________________________________________________________*/

static void
asp_Write (asp_reent* reent, const asp_text* params,  asp_text* ret)
{

    asp_send_response (NULL,params[0].str,params[0].len);

}
/*____________________________________________________________________*
**	function name: request_Form
**
**	description:
*     get the value of item on page.
*
**	parameters:
*     reent:not use
*     id:not use
*     params:params[0] item name.
*     ret:not use
*
**	global:
*     none
**	return:
*     none
**	call:
*     none
**	revision:
*     1.shnwind
* _____________________________________________________________________*/
static void
request_Form (asp_reent* reent, const asp_text* params,  asp_text* ret)
{

    char *val,*tmp;

    /*ret->str = "");
    ret->len = 0;*/

    if (params[0].str && params[0].len)
    {
    	tmp = (char*)asp_alloc(reent,params[0].len+1);
    	memset(tmp,0,params[0].len+1);
    	memcpy(tmp,params[0].str,params[0].len);
    	val = get_param(g_var_post,tmp);
	//dbgprintf("request name %s value %s\n",tmp,val);
      if(val ==NULL)
    		return;
		decode_uri(val);
		ret->str = val;
		ret->len = strlen(val);
    }
}

/*____________________________________________________________________*
**	function name: tcWebApi _set
**
**	description:
*     write the value which is user input to cfg_manager.
*
**	parameters:
*     reent:not use
*     id:not use
*     params:params[0] node name.
*            params[1] attribute name.
*            params[2] value
*     ret:not use
*
**	global:
*     none
**	return:
*     none
**	call:
*     none
**	revision:
*     1.shnwind
* _____________________________________________________________________*/
static void
tcWebApi_Set (asp_reent* reent, const asp_text* params,  asp_text* ret)
{

	char *node,*attr,*value,*v_tmp;
	int r_val, i = 0;
	char *directSet[] = {"CurPage", "TransMode", "wan_pvc", ""};

	node = (char*)asp_alloc(reent,params[0].len+1);
	attr = (char*)asp_alloc(reent,params[1].len+1);
	value = (char*)asp_alloc(reent,params[2].len+1);
	memset(node,0,params[0].len+1);
	memset(attr,0,params[1].len+1);
	memset(value,0,params[2].len+1);
	memcpy(node,params[0].str,params[0].len);
	memcpy(attr,params[1].str,params[1].len);
	memcpy(value,params[2].str,params[2].len);
	node[params[0].len]='\0';
	attr[params[1].len]='\0';
	value[params[2].len]='\0';

	if(params[2].len == 0){
	  return;
	}

	v_tmp = get_param(g_var_post,value);//get_param on web page
	cprintf("set %s: %s=%s\n", node, attr, v_tmp);
	if(v_tmp != NULL){
		decode_uri(v_tmp);
		if(!strcmp(value, "pptpd_clientlist")
			|| !strcmp(value, "pptpd_sr_rulelist")
			|| !strcmp(value, "vpnc_clientlist")
			|| !strcmp(value, "qos_rulelist")
			|| !strcmp(value, "qos_bw_rulelist")
			|| !strcmp(value, "MULTIFILTER_DEVICENAME")
			|| !strcmp(value, "MULTIFILTER_MACFILTER_DAYTIME")
		) {
			r_val = tcapi_set_list(value, v_tmp);
		}
		else if(!strcmp(value, "vpn_server_clientlist")) {
			r_val = tcapi_set_multiattr(node, attr, v_tmp, strlen(v_tmp));
		}
		else {
			r_val=tcapi_set(node, attr,v_tmp);
		}
	}
	else{//If the value is not the name of an attribute, regard it as the value to be assigned
		while(strlen(directSet[i]))
			if(!strcmp(attr, directSet[i++]))
				r_val=tcapi_set(node, attr, value);
	}
}
void strQuotConvertHTML(char *oriStr,char *desStr)  {
    int i;
    int j = 0;
    for(i = 0;i < strlen(oriStr);i++){
    	if(oriStr[i] == '"'){
    		strcpy(&(desStr[j]),"&quot;");
    		j+=6;
    	}
    	else{
    		desStr[j] = oriStr[i];
    		j++;
    	}
    }
    desStr[j] = '\0';
}

//javi.B
void
load_parameters_to_generic(asp_reent* reent, const asp_text* params, asp_text* ret) {
	struct tcapi_tuple *t;
	char buf[640], freq[2], unitprefix[32], name[64];
	char *tmp;

	memset(freq, 0, sizeof(freq));

	tmp = get_param(g_var_post,"wl_unit");
	if(tmp == NULL){
		tcapi_get("WLan_Common", "wl_unit", freq);
	}
	else
	{
		memcpy(freq,tmp, sizeof(freq));
		tcapi_set("WLan_Common", "wl_unit", freq);	//After selecting the freq, save it.
	}
	sprintf(unitprefix, "wl%d_",atoi(freq));

	for (t = wlan_system_router_defaults; t->node; t++)
	{
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "%s%s", unitprefix, t->attribute);
		tcapi_get(t->node, name, buf);	//get the value from the individual parameters.
		if(strlen(buf)<1){	//Can not get the attribute correctly so use the default value.
			sprintf(buf,"%s", t->value);
		}
		tcapi_set(t->node, t->attribute, buf);	//save the value to the generic parameters.
	}

	for (t = wlan_configuration_router_defaults; t->node; t++)
	{
		if(strcmp( t->attribute, "BssidNum")==0){
			continue;
		}
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "%s%s", unitprefix, t->attribute);
		tcapi_get(t->node, name, buf);	//get the value from the individual parameters.
		if(strlen(buf)<1){	//Can not get the attribute correctly so use the default value.
			sprintf(buf,"%s", t->value);
		}
		tcapi_set(t->node, t->attribute, buf);	//save the value to the generic parameters.
	}
}


void
load_parameters_from_generic(asp_reent* reent, const asp_text* params, asp_text* ret) {	//javi
	struct tcapi_tuple *t;
	char buf[640], freq[2], unitprefix[32], name[64];

	memset(freq, 0, sizeof(freq));
	tcapi_get("WLan_Common", "wl_unit", freq);
	sprintf(unitprefix, "wl%d_",atoi(freq));

	for (t = wlan_system_router_defaults; t->node; t++)
	{
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		tcapi_get(t->node, t->attribute, buf);	//get the value from the generic parameters.
		if(strlen(buf)<1){	//Can not get the attribute correctly so use the default value.
			sprintf(buf,"%s", t->value);
		}
		sprintf(name, "%s%s", unitprefix, t->attribute);
		tcapi_set(t->node, name, buf);	//save the value to the individual parameters.
	}

	for (t = wlan_configuration_router_defaults; t->node; t++)
	{
		if(strcmp( t->attribute, "BssidNum")==0){
			continue;
		}

		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		tcapi_get(t->node, t->attribute, buf);	//get the value from the generic parameters.
		if(strlen(buf)<1){	//Can not get the attribute correctly so use the default value.
			sprintf(buf,"%s", t->value);
		}
		sprintf(name, "%s%s", unitprefix, t->attribute);
		tcapi_set(t->node, name, buf);	//save the value to the individual parameters.
	}
}


void
load_MBSSID_parameters_to_generic(asp_reent* reent, const asp_text* params, asp_text* ret) {	//javi
	struct tcapi_tuple *t;
	char buf[160], freq[2], sub_unit[4], prefix[32], name[64];
	char *tmp;

	memset(freq, 0, sizeof(freq));
	memset(sub_unit, 0, sizeof(sub_unit));
	tmp = get_param(g_var_post,"wl_unit");
	if(tmp == NULL){
		tcapi_get("WLan_Common", "wl_unit", freq);
	}
	else
	{
		memcpy(freq,tmp, sizeof(freq));
		tcapi_set("WLan_Common", "wl_unit", freq);	//After selecting the freq, save it.
	}

	tmp = get_param(g_var_post,"wl_subunit");
	if(tmp == NULL){
		tcapi_get("WLan_Common", "wl_subunit", sub_unit);
	}
	else
	{
		memcpy(sub_unit,tmp, sizeof(sub_unit));
		tcapi_set("WLan_Common", "wl_subunit", sub_unit);	//After selecting the sub-unit, save it.
	}

	if(! strcmp(sub_unit, "-1")){
		//wait to
	}
	else{
		sprintf(prefix, "wl%s.%s_", freq, sub_unit);
	}

	for (t = wlan_MBSSID_system_defaults; t->node; t++)
	{
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "%s%s", prefix, t->attribute);
		tcapi_get(t->node, name, buf);	//get the value from the individual parameters.
		if(strlen(buf)<1){	//Can not get the attribute correctly so use the default value.
			sprintf(buf,"%s", t->value);
		}
		tcapi_set(t->node, t->attribute, buf);	//save the value to the generic parameters.
	}
}

void
load_MBSSID_parameters_from_generic(asp_reent* reent, const asp_text* params, asp_text* ret) {	//javi
	struct tcapi_tuple *t;
	char buf[160], freq[2], sub_unit[4], prefix[32], name[64];

	memset(sub_unit, 0, sizeof(sub_unit));
	memset(freq, 0, sizeof(freq));
	tcapi_get("WLan_Common", "wl_unit", freq);
	tcapi_get("WLan_Common", "wl_subunit", sub_unit);
	sprintf(prefix, "wl%s.%s_", freq, sub_unit);

	for (t = wlan_MBSSID_system_defaults; t->node; t++)
	{
		if(strcmp(t->attribute, "bss_enabled") == 0)
			continue;

		//No restore the expire_tmp, because it should be handle and the expire time mechanizm.
		if(strcmp(t->attribute, "expire_tmp") == 0)
			continue;

		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		tcapi_get(t->node, t->attribute, buf);	//get the value from the generic parameters.
		if(strlen(buf)<1){	//Can not get the attribute correctly so use the default value.
			sprintf(buf,"%s", t->value);
		}
		sprintf(name, "%s%s", prefix, t->attribute);
		tcapi_set(t->node, name, buf);	//save the value to the individual parameters.
	}
}




/*
 * Reads file and returns contents
 * @param	fd	file descriptor
 * @return	contents of file or NULL if an error occurred
 */
char *
fd2str(int fd)
{
	char *buf = NULL;
	size_t count = 0, n;

	do {
		buf = realloc(buf, count + 512);
		n = read(fd, buf + count, 512);
		if (n < 0) {
			free(buf);
			buf = NULL;
		}
		count += n;
	} while (n == 512);

	close(fd);
	if (buf)
		buf[count] = '\0';
	return buf;
}

/*
 * Reads file and returns contents
 * @param	path	path to file
 * @return	contents of file or NULL if an error occurred
 */
char *
file2str(const char *path)
{
	int fd;

	if ((fd = open(path, O_RDONLY)) == -1) {
		perror(path);
		return NULL;
	}

	return fd2str(fd);
}

char *
rfctime(const time_t *timep)
{
	static char rfct[201];
	struct tm tm;
	char tmp[32], cmd[64];

	memset(rfct, 0, sizeof(rfct));
	memset(tmp, 0, sizeof(tmp));
	//it suppose to be convert after applying
	//time_zone_x_mapping();
	//setenv("TZ", nvram_safe_get_x("", "time_zone_x"), 1);
	tcapi_get("Timezone_Entry", "TZ", tmp);
	sprintf(cmd, "echo %s > /etc/TZ", tmp);
	system(cmd);
	memcpy(&tm, localtime(timep), sizeof(struct tm));
	strftime(rfct, 200, "%a, %d %b %Y %H:%M:%S %z", &tm);
	return rfct;
}

void
reltime(unsigned int seconds, char *cs)
{
#ifdef SHOWALL
	int days=0, hours=0, minutes=0;

	if (seconds > 60*60*24) {
		days = seconds / (60*60*24);
		seconds %= 60*60*24;
	}
	if (seconds > 60*60) {
		hours = seconds / (60*60);
		seconds %= 60*60;
	}
	if (seconds > 60) {
		minutes = seconds / 60;
		seconds %= 60;
	}
	sprintf(cs, "%d days, %d hours, %d minutes, %d seconds", days, hours, minutes, seconds);
#else
	sprintf(cs, "%d secs", seconds);
#endif
}

void ej_uptime(asp_reent* reent, const asp_text* params, asp_text* ret) {
	char buf[512];

	char *str = file2str("/proc/uptime");
	time_t tm;

	time(&tm);
	sprintf(buf, rfctime(&tm));

	if (str) {
		unsigned int up = atoi(str);
		free(str);
		char lease_buf[128];
		memset(lease_buf, 0, sizeof(lease_buf));
		reltime(up, lease_buf);
		sprintf(buf, "%s(%s since boot)", buf, lease_buf);
	}

	asp_send_response (NULL,buf,strlen(buf));
}

void
wl_get_2G_guestnetwork(asp_reent* reent, const asp_text* params, asp_text* ret) {
	struct tcapi_tuple *t;
	char word1[128], tmp[128];
	char unitprefix[8], subunitprefix[8], name[32];;
	int  unit;
	char buf[4096];
	int i = 1, j = 0;
	int str_length=0;
	char ascii_tmp[256];

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "[");

	unit = 0;	// 2.4G
	//set wlX_vifnames .B
	memset(word1, 0, sizeof(word1));
	for(i = 1; i < MAX_NO_MSSID;i++)	{
		sprintf(tmp, "wl%d.%d", unit, i);
		if(i > 1)
			strcat(word1, " ");
		strcat(word1, tmp);
	}
	snprintf(unitprefix, sizeof(unitprefix), "wl%d_", unit);
	sprintf(name, "%s%s", unitprefix, "vifnames");
	tcapi_set("WLan_Common", name, word1);
	//set wlX_vifnames .E

	for(i = 1; i < MAX_NO_MSSID;i++)	{
		if(i > 1)
			strcat(buf, ", ");

		strcat(buf, "[\"");
		j=0;

		for (t = wlan_MBSSID_system_defaults; t->node; t++)
		{
			if(j)
				strcat(buf, "\", \"");
			j++;
			memset(subunitprefix, 0, sizeof(subunitprefix));
			sprintf(subunitprefix, "wl%d.%d_", unit, i);
			sprintf(name, "%s%s", subunitprefix,  t->attribute);
			tcapi_get("WLan_Entry0", name, tmp);

			if(strlen(tmp)<1){	//Can not get the attribute correctly so use the default value.
				sprintf(tmp,"%s", t->value);
			}

			/* translate the values of ssid, wpa_psk, key1, key2, key3, key4 to ASCII */
			if((strcmp(t->attribute, "ssid")==0) ||
				(strcmp(t->attribute, "wpa_psk")==0) ||
				(strcmp(t->attribute, "key1")==0) ||
				(strcmp(t->attribute, "key2")==0) ||
				(strcmp(t->attribute, "key3")==0) ||
				(strcmp(t->attribute, "key4")==0)
			){
				/* each char expands to %XX at max */
				str_length = strlen(tmp) * sizeof(char)*3 + sizeof(char);
				char_all_to_ascii(ascii_tmp, tmp, str_length);

				strcat(buf, ascii_tmp);
			}
			else{
				strcat(buf, tmp);
			}

		}

		strcat(buf, "\"]");
	}

	strcat(buf, "]");
	asp_send_response (NULL,buf,strlen(buf));
	/* The example as the following:
	       [["0", "ASUS_Guest1", "open", "aes", "", "0", "1", "", "", "", "", "0", "off", "", "disabled", "", "", "", "", "", ""]
		["0", "ASUS_Guest2", "open", "aes", "", "0", "1", "", "", "", "", "0", "off", "", "disabled", "", "", "", "", "", ""]
		["0", "ASUS_Guest3", "open", "aes", "", "0", "1", "", "", "", "", "0", "off", "", "disabled", "", "", "", "", "", ""]]  */
}

void
wl_get_5G_guestnetwork(asp_reent* reent, const asp_text* params, asp_text* ret) {
	struct tcapi_tuple *t;
	char word1[128], tmp[128];
	char unitprefix[8], subunitprefix[8], name[32];;
	int  unit;
	char buf[4096];
	int i = 1, j = 0;
	int str_length=0;
	char ascii_tmp[256];

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "[");

	unit = 1;	// 5G
	//set wlX_vifnames .B
	memset(word1, 0, sizeof(word1));
	for(i = 1; i < MAX_NO_MSSID;i++)	{
		sprintf(tmp, "wl%d.%d", unit, i);
		if(i > 1)
			strcat(word1, " ");
		strcat(word1, tmp);
	}
	snprintf(unitprefix, sizeof(unitprefix), "wl%d_", unit);
	sprintf(name, "%s%s", unitprefix, "vifnames");
	tcapi_set("WLan_Common", name, word1);
	//set wlX_vifnames .E

	for(i = 1; i < MAX_NO_MSSID;i++)	{
		if(i > 1)
			strcat(buf, ", ");

		strcat(buf, "[\"");
		j=0;

		for (t = wlan_MBSSID_system_defaults; t->node; t++)
		{
			if(j)
				strcat(buf, "\", \"");
			j++;
			memset(subunitprefix, 0, sizeof(subunitprefix));
			sprintf(subunitprefix, "wl%d.%d_", unit, i);
			sprintf(name, "%s%s", subunitprefix,  t->attribute);
			tcapi_get("WLan_Entry0", name, tmp);

			if(strlen(tmp)<1){	//Can not get the attribute correctly so use the default value.
				sprintf(tmp,"%s", t->value);
			}

			/* translate the values of ssid, wpa_psk, key1, key2, key3, key4 to ASCII */
			if((strcmp(t->attribute, "ssid")==0) ||
				(strcmp(t->attribute, "wpa_psk")==0) ||
				(strcmp(t->attribute, "key1")==0) ||
				(strcmp(t->attribute, "key2")==0) ||
				(strcmp(t->attribute, "key3")==0) ||
				(strcmp(t->attribute, "key4")==0)
			){
				/* each char expands to %XX at max */
				str_length = strlen(tmp) * sizeof(char)*3 + sizeof(char);
				char_all_to_ascii(ascii_tmp, tmp, str_length);

				strcat(buf, ascii_tmp);
			}
			else{
				strcat(buf, tmp);
			}

		}

		strcat(buf, "\"]");
	}

	strcat(buf, "]");
	asp_send_response (NULL,buf,strlen(buf));
	/* The example as the following:
		       [["0", "ASUS_5G_Guest1", "open", "aes", "", "0", "1", "", "", "", "", "0", "off", "", "disabled", "", "", "", "", "", ""]
			["0", "ASUS_5G_Guest2", "open", "aes", "", "0", "1", "", "", "", "", "0", "off", "", "disabled", "", "", "", "", "", ""]
			["0", "ASUS_5G_Guest3", "open", "aes", "", "0", "1", "", "", "", "", "0", "off", "", "disabled", "", "", "", "", "", ""]]  */
}

void
auto_detection(asp_reent* reent, const asp_text* params, asp_text* ret) {

	system("killall -9 auto_det");
	system("/userfs/bin/auto_det &");

	//return 0;
}

void
wps_info(asp_reent* reent, const asp_text* params, asp_text* ret) {
	char value[64], tmp[256], buf[1024];
	char attri[64], prefix[8];
	char tmpstr[256];

	memset(value, 0, sizeof(value));
	tcapi_get("WLan_Common", "wl_unit", value);
	//sprintf(prefix, "wl%s_",value);
	memset(prefix, 0, sizeof(prefix));

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "<wps>\n");

	//0. WSC Status
	tcapi_get("Info_WLan", "wlanWPSStatus", value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", value);
	strcat(buf, tmp);

	// 1. WPSConfigured
	/* Wlan_Entry0, WPSConfStatus ==> 1:unconfigured 2:configured 	*/
	sprintf(attri, "%s%s", prefix, "WPSConfStatus");
	tcapi_get("WLan_Entry0", attri, value);
	if ( strcmp(value, "2") ==0){
		sprintf(tmp, "<wps_info>%s</wps_info>\n", "Yes");
	}
	else{
		sprintf(tmp, "<wps_info>%s</wps_info>\n", "No");
	}
	strcat(buf, tmp);

	// 2. WPSSSID
	sprintf(attri, "%s%s", prefix, "ssid");
	tcapi_get("WLan_Entry0", attri, value);
	memset(tmpstr, 0, sizeof(tmpstr));
	char_to_ascii(tmpstr, value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", tmpstr);
	strcat(buf, tmp);

	// 3. WPSAuthMode
	sprintf(attri, "%s%s", prefix, "auth_mode_x");
	tcapi_get("WLan_Entry0", attri, value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", value);
	strcat(buf, tmp);

	// 4. EncrypType
	sprintf(attri, "%s%s", prefix, "crypto");
	tcapi_get("WLan_Entry0", attri, value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", value);
	strcat(buf, tmp);

	//5. DefaultKeyIdx
	sprintf(attri, "%s%s", prefix, "key");
	tcapi_get("WLan_Entry0", attri, value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", value);
	strcat(buf, tmp);

	//6. WPAKey
	sprintf(attri, "%s%s", prefix, "wpa_psk");
	tcapi_get("WLan_Entry0", attri, value);
	memset(tmpstr, 0, sizeof(tmpstr));
	char_to_ascii(tmpstr, value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", tmpstr);
	strcat(buf, tmp);

	//7. AP PIN Code
	tcapi_get("WLan_Entry0", "WscVendorPinCode", value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", value);
	strcat(buf, tmp);

	//8. Saved WPAKey
	sprintf(attri, "%s%s", prefix, "wpa_psk");
	tcapi_get("WLan_Entry0", attri, value);
	memset(tmpstr, 0, sizeof(tmpstr));
	char_to_ascii(tmpstr, value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", tmpstr);
	strcat(buf, tmp);

	//9. WPS enable?
	tcapi_get("WLan_Common","wps_enable", value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", value);
	strcat(buf, tmp);

	//A. WPS mode
	sprintf(attri, "%s%s", prefix, "WPSMode");
	tcapi_get("WLan_Entry0", attri, value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", value);
	strcat(buf, tmp);

	//B. current auth mode
	sprintf(attri, "%s%s", prefix, "auth_mode_x");
	tcapi_get("WLan_Entry0", attri, value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", value);
	strcat(buf, tmp);

	//C. WPS band
	tcapi_get("WLan_Common", "wl_unit", value);
	sprintf(tmp, "<wps_info>%s</wps_info>\n", value);
	strcat(buf, tmp);

	strcat(buf, "</wps>");

	asp_send_response (NULL,buf,strlen(buf));
	//return 0;
}

void
wl_auth_list(asp_reent* reent, const asp_text* params, asp_text* ret) {
	FILE* fp;
	char buf[2056]={0};
	char tmp[512]={0};

	system("/usr/bin/ATE wl_auth_list");

	fp = fopen("/tmp/wifi_client_mac.txt","r");
	if (fp != NULL){
		while(fgets(tmp, 512, fp)){
			if (tmp != NULL){
				strcat(buf, tmp);
			}
		}
		asp_send_response (NULL, buf, strlen(buf));

		fclose(fp);
	}
}

/* Transfer all Char to ASCII */
int char_all_to_ascii(const char *output, const char *input, int outsize)
{
	char *src = (char *)input;
	char *dst = (char *)output;
	char *end = (char *)output + outsize - 1;

	if (src == NULL || dst == NULL || outsize <= 0)
		return 0;

	for ( ; *src && dst < end; src++) {
		if (dst + 3 > end)
			break;
		dst += sprintf(dst, "%%%.02X", *src);
	}
	if (dst <= end)
		*dst = '\0';

	return dst - output;
}

void
tcWebApi_char_to_ascii(asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *node,*attr,*show;
	int r_val;
	int str_length=0;
	static char val[4096];
	char tmp[1024];
	char *buf = tmp;

	node = (char*)asp_alloc(reent,params[0].len+1);
	attr = (char*)asp_alloc(reent,params[1].len+1);
	show = (char*)asp_alloc(reent,params[2].len+1);
	memset(node,0,params[0].len+1);
	memset(attr,0,params[1].len+1);
	memset(show,0,params[2].len+1);
	memcpy(node,params[0].str,params[0].len);
	memcpy(attr,params[1].str,params[1].len);
	memcpy(show,params[2].str,params[2].len);
	node[params[0].len]='\0';
	attr[params[1].len]='\0';
	show[params[2].len]='\0';

	if(!strcmp(attr, "pptpd_clientlist")) {
		r_val = tcapi_get_list(attr, val, sizeof(val));
	}
	else if(!strcmp(attr, "pptpd_sr_rulelist")) {
		r_val = tcapi_get_list(attr, val, sizeof(val));
	}
	else if(!strcmp(node, "VPNC_Entry") && !strcmp(attr, "clientlist")) {
		r_val = tcapi_get_list("vpnc_clientlist", val, sizeof(val));
	}
	else if(!strcmp(node, "OpenVPN_Common") && !strcmp(attr, "vpn_server_clientlist")) {
		r_val = tcapi_get_multiattr(node, attr, val, sizeof(val));
	}
	else if(!strcmp(attr, "qos_rulelist")) {
		r_val = tcapi_get_list(attr, val, sizeof(val));
	}
	else if(!strcmp(attr, "qos_bw_rulelist")) {
		r_val = tcapi_get_list(attr, val, sizeof(val));
	}
	else if(!strcmp(attr, "MULTIFILTER_DEVICENAME")) {
		r_val = tcapi_get_list(attr, val, sizeof(val));
	}
	else if(!strcmp(attr, "MULTIFILTER_MACFILTER_DAYTIME")) {
		r_val = tcapi_get_list(attr, val, sizeof(val));
	}
	else {
		r_val = tcapi_get(node, attr, val);
	}
	if(r_val < 0){
		strcpy(val,"");
	}

	/* each char expands to %XX at max */
	str_length = strlen(val) * sizeof(char)*3 + sizeof(char);
	if (str_length > sizeof(tmp)) {
		buf = (char *)malloc(str_length);
		if (buf == NULL) {
			csprintf("No memory.\n");
			return;
		}
	}

	char_all_to_ascii(buf, val, str_length);

	if(!strcmp(show,"s")){//show -> s
		asp_send_response (NULL,buf,strlen(buf));
	}
	else if(!strcmp(show,"h")){//hide -> h
		if(strlen(val))
			ret->str = val;
		ret->len = strlen(val);
	}

	if (buf != tmp)
		free(buf);

}

static void
ej_wl_scan_2g(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE* fp;
	char buf[8192]={0};
	char tmp[512]={0};

	system("/usr/bin/ATE wl_scan 0");

	fp = fopen("/tmp/wds_list.txt","r");
	if (fp != NULL){
		while(fgets(tmp, 512, fp)){
			if (tmp != NULL){
				strcat(buf, tmp);
			}
		}
		asp_send_response (NULL, buf, strlen(buf));

		fclose(fp);
	}
}

static void
ej_wl_scan_5g(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE* fp;
	char buf[8192]={0};
	char tmp[512]={0};

	system("/usr/bin/ATE wl_scan 1");

	fp = fopen("/tmp/wds_list.txt","r");
	if (fp != NULL){
		while(fgets(tmp, 512, fp)){
			if (tmp != NULL){
				strcat(buf, tmp);
			}
		}
		asp_send_response (NULL, buf, strlen(buf));

		fclose(fp);
	}
}

/* Paul add 2013/6/17 */
#define ARP_TABLE "/proc/net/arp"
static void tcWebApi_get_arp_list(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE *fp;
	char buf[80];
	char ipaddr[16], hwtype[16], flags[16], hwaddr[20], mask[16], device[16], state[16];
	char tmp[4096] = {0};

	fp = fopen(ARP_TABLE, "r");
	if(fp) {
		fgets(buf, 80, fp);
		while(fscanf(fp, "%s %s %s %s %s %s %s\n", ipaddr, hwtype, flags, hwaddr, mask, device, state) == 7) {
			//['ipaddr','hwaddr'],
			if(strcmp(device, "br0") == 0) {
				websWrite( tmp, "['%s','%s'],", ipaddr, hwaddr);
			}
		}
		fclose(fp);
	}
}

//Ren.B
#ifndef TCSUPPORT_WLAN_RT6856
static void init_wifiIfname()
{
	char buffer[128] = {0};
	char *pch = NULL;
	unsigned int num = 0;

	memset( &Wifi2Gifname, 0, sizeof(wifiIfName_t) );
	memset( &Wifi5Gifname, 0, sizeof(wifiIfName_t) );

	/** for 2G part **/
	num = 0;
	memset(buffer, 0, sizeof(buffer));
	if(tcapi_get("WLan_Common", "wl0_ifnames", buffer) == 0) //for 2G wifi interface names
	{
		if(buffer[0]!= '\0')
		{
			pch = strtok (buffer, " ");
			while(pch!=NULL)
			{
				strcpy(Wifi2Gifname.ifnames[num], pch);
				pch = strtok (NULL, " ");
				num++;
				if(num == MAX_IF_NUM)
				{
					break;
				}
			}
		}
	}
	else
	{
		printf( "[%s][%d]Cannot get attribute:WLan_Common wl0_ifnames\n", __FILE__, __LINE__ );
	}
	Wifi2Gifname.count = num;

	/** for 5G part **/
	num = 0;
	memset(buffer, 0, sizeof(buffer));
	if(tcapi_get("WLan_Common", "wl1_ifnames", buffer) == 0) //for 2G wifi interface names
	{
		if(buffer[0]!= '\0')
		{
			pch = strtok (buffer, " ");
			while(pch!=NULL)
			{
				strcpy(Wifi5Gifname.ifnames[num], pch);
				pch = strtok (NULL, " ");
				num++;
				if(num == MAX_IF_NUM)
				{
					break;
				}
			}
		}
	}
	else
	{
		printf( "[%s][%d]Cannot get attribute:WLan_Common wl1_ifnames\n", __FILE__, __LINE__ );
	}

	Wifi5Gifname.count = num;
}
#endif /* TCSUPPORT_WLAN_RT6856 */

static void init_netdevData()
{
	memset( &WiredData, 0, sizeof(WiredData) );
	memset( &InternetData, 0, sizeof(InternetData) );
	memset( &BridgeData, 0, sizeof(BridgeData) );
	memset( Wireless2GData, 0, sizeof(Wireless2GData) );
	memset( &Sum2GData, 0, sizeof(Sum2GData) );
	memset( Wireless5GData, 0, sizeof(Wireless5GData) );
	memset( &Sum5GData, 0, sizeof(Sum5GData) );
	memset( &ATMData, 0, sizeof(ATMData) );
	memset( PTMData, 0, sizeof(PTMData) );
	memset( &EthernetWANData, 0, sizeof(EthernetWANData) );
#ifdef TCSUPPORT_WLAN_RT6856
	//do nothing
#else
	init_wifiIfname();
#endif /* TCSUPPORT_WLAN_RT6856 */
}

void readATMData_MTK(void)
{
	FILE *fp = NULL;
	char buf[256] = {0};
	unsigned long in;
	unsigned long out;

	system("cat /proc/tc3162/tsarm_stats > /tmp/atm_traffic");

	fp = fopen( "/tmp/atm_traffic", "r" );
	if (fp)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			if (!strncmp(buf, "inBytes", 7))
			{
				sscanf(buf, "%*[^=]= 0x%lx,%*[^=]= 0x%lx", &in, &out);
				ATMData.rx = in;
				ATMData.tx = out;
			}
		}

		fclose(fp);
	}
	unlink("/tmp/atm_traffic");
}

#if defined(TCSUPPORT_CPU_MT7510)
void readPTMData_MTK_mt7510(void)
{
	FILE *fp = NULL;
	char buf[256] = {0};
	unsigned long in;
	unsigned long out;

	system("cat /proc/tc3162/ptm_b0_stats > /tmp/ptm_traffic");

	fp = fopen( "/tmp/ptm_traffic", "r" );
	if (fp)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			if (!strncmp(buf, "inOctets", 8))
			{
				sscanf(buf, "%*[^=]= 0x%lx,%*[^=]= 0x%*x", &in);
				PTMData[0].rx = in;
			}
			else if (!strncmp(buf, "outOctets", 9))
			{
				sscanf(buf, "%*[^=]= 0x%lx,%*[^=]= 0x%*x", &out);
				PTMData[0].tx = out;
			}
		}

		fclose(fp);
	}

	unlink("/tmp/ptm_traffic");
}
#else
void readPTMData_MTK(void)
{
	FILE *fp = NULL;
	char buf[256] = {0};
	unsigned long in;
	unsigned long out;

	system("cat /proc/tc3162/ptm_b0_stats > /tmp/ptm_traffic");

	fp = fopen( "/tmp/ptm_traffic", "r" );
	if (fp)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			if (!strncmp(buf, "inOctets", 8))
			{
				sscanf(buf, "%*[^=]= 0x%lx,%*[^=]= 0x%*x", &in);
				PTMData[0].rx = in;
			}
			else if (strstr(buf, "outOctets"))
			{
				sscanf(buf, "%*[^=]= 0x%*x,%*[^=]= 0x%lx", &out);
				PTMData[0].tx = out;
			}
		}

		fclose(fp);
	}

	unlink("/tmp/ptm_traffic");
}

#endif

#ifdef TCSUPPORT_WLAN_RT6856
void readWiFiData_RT6856(void)
{
	FILE *fp = NULL;
	char buf[256] = {0};
	char cmd[128] = {0};
	char *ptr = NULL;

	sprintf(cmd, "/userfs/bin/iwpriv show traffic > /dev/null" ); //must use its absolute path
	system(cmd);
	sleep(1);

	fp = fopen( "/tmp/wifi_traffic", "r" );
	if( fp )
	{
		while( fgets(buf, sizeof(buf), fp) )
		{
				if( (ptr = strstr( buf, "2GRxCount:" ))!= NULL )
			{
					ptr += 10;
					sscanf( ptr, "%llu%llu%llu%llu%llu", &Wireless2GData[0].rx, &Wireless2GData[1].rx, &Wireless2GData[2].rx, &Wireless2GData[3].rx, &Sum2GData.rx );
				}
				else if( (ptr = strstr( buf, "2GTxCount:" ))!= NULL )
				{
					ptr += 10;
					sscanf( ptr, "%llu%llu%llu%llu%llu", &Wireless2GData[0].tx, &Wireless2GData[1].tx, &Wireless2GData[2].tx, &Wireless2GData[3].tx, &Sum2GData.tx );
			}
				else if( (ptr = strstr( buf, "5GRxCount:" ))!= NULL )
			{
					ptr += 10;
					sscanf( ptr, "%llu%llu%llu%llu%llu", &Wireless5GData[0].rx, &Wireless5GData[1].rx, &Wireless5GData[2].rx, &Wireless5GData[3].rx, &Sum5GData.rx );
				}
				else if( (ptr = strstr( buf, "5GTxCount:" ))!= NULL )
				{
					ptr += 10;
					sscanf( ptr, "%llu%llu%llu%llu%llu", &Wireless5GData[0].tx, &Wireless5GData[1].tx, &Wireless5GData[2].tx, &Wireless5GData[3].tx, &Sum5GData.tx );
			}
		}

		fclose(fp);
	}
	else
	{
		printf( "[%s]Can not read /tmp/wifi_traffic\n", __FUNCTION__ );
	}

	#if 0 //debug
	{
		FILE *output = fopen("/tmp/output", "w");
		fprintf(output, "2GRX=%llu, %llu, %llu, %llu, total=%llu\n", Wireless2GData[0].rx, Wireless2GData[1].rx, Wireless2GData[2].rx, Wireless2GData[3].rx, Sum2GData.rx);
		fprintf(output, "2GTX=%llu, %llu, %llu, %llu, total=%llu\n", Wireless2GData[0].tx, Wireless2GData[1].tx, Wireless2GData[2].tx, Wireless2GData[3].tx, Sum2GData.tx);
		fprintf(output, "5GRX=%llu, %llu, %llu, %llu, total=%llu\n", Wireless5GData[0].rx, Wireless5GData[1].rx, Wireless5GData[2].rx, Wireless5GData[3].rx, Sum5GData.rx);
		fprintf(output, "5GTX=%llu, %llu, %llu, %llu, total=%llu\n", Wireless5GData[0].tx, Wireless5GData[1].tx, Wireless5GData[2].tx, Wireless5GData[3].tx, Sum5GData.tx);
		fclose(output);
	}
	#endif

}
#endif

static void parseWiFiData_MTK(netDev_t *data)
{
	FILE *fp = NULL;
	char buf[256] = {0};

	if (data == NULL)
		return;

	fp = fopen( "/tmp/wifi_traffic", "r" );
	if (fp)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			if (!strncmp(buf, "Bytes Received", 14))
			{
				sscanf(buf, "%*[^=]= %llu", &(data->rx));
			}
			else if (!strncmp(buf, "Bytes Sent", 10))
			{
				sscanf(buf, "%*[^=]= %llu", &(data->tx));
			}
		}

		fclose(fp);
	}
	unlink("/tmp/wifi_traffic");
}

void readWiFiData_MTK(void)
{
	char cmd[128] = {0};
	int i;

	//ra0 ~ ra3 record the same statisic ..
	for (i=0; i<1; i++)
	{
		//2.4G
		sprintf(cmd, "/userfs/bin/iwpriv ra%d stat > /tmp/wifi_traffic", i);
		system(cmd);
		parseWiFiData_MTK(&Wireless2GData[i]);

		//5G
		sprintf(cmd, "/userfs/bin/iwpriv rai%d stat > /tmp/wifi_traffic", i);
		system(cmd);
		parseWiFiData_MTK(&Wireless5GData[i]);
	}

	#if 0 //debug
	{
		FILE *output = fopen("/tmp/output", "w");
		fprintf(output, "2GRX=%llu, %llu, %llu, %llu, total=%llu\n", Wireless2GData[0].rx, Wireless2GData[1].rx, Wireless2GData[2].rx, Wireless2GData[3].rx, Sum2GData.rx);
		fprintf(output, "2GTX=%llu, %llu, %llu, %llu, total=%llu\n", Wireless2GData[0].tx, Wireless2GData[1].tx, Wireless2GData[2].tx, Wireless2GData[3].tx, Sum2GData.tx);
		fprintf(output, "5GRX=%llu, %llu, %llu, %llu, total=%llu\n", Wireless5GData[0].rx, Wireless5GData[1].rx, Wireless5GData[2].rx, Wireless5GData[3].rx, Sum5GData.rx);
		fprintf(output, "5GTX=%llu, %llu, %llu, %llu, total=%llu\n", Wireless5GData[0].tx, Wireless5GData[1].tx, Wireless5GData[2].tx, Wireless5GData[3].tx, Sum5GData.tx);
		fclose(output);
	}
	#endif

}

#if defined(TCSUPPORT_CPU_MT7510)
void readWiredData_MTK_mt7510(void)
{
	FILE *fp = NULL;
	char buf[256] = {0};
	char tmp[32] = {0};
	int port_no = 0;
	unsigned long in;
	unsigned long out;


	system("cat /proc/tc3162/gsw_stats > /tmp/ether_switch_traffic");

	fp = fopen( "/tmp/ether_switch_traffic", "r" );
	if (fp)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			sprintf(tmp, "[ Port %d ]", port_no);
			if (!strncmp(buf, tmp, 10))
			{
				while (fgets(buf, sizeof(buf), fp))
				{
					if (!strncmp(buf, "Tx Bytes", 8))
					{
						sscanf(buf, "%*[^=]= 0x%lx,%*[^=]= 0x%lx", &out, &in);
						if (port_no < 4)
						{
							WiredData.rx += in;
							WiredData.tx += out;
						}
						else
						{
							EthernetWANData.rx += in;
							EthernetWANData.tx += out;
						}

						port_no++;
						if (get_cur_lanwan_port() == port_no)
						{
							EthernetWANData.rx += in;
							EthernetWANData.tx += out;
						}

						break;
					}
				}
			}

			if (port_no >= 5)
				break;
		}

		fclose(fp);
	}
	unlink("/tmp/ether_switch_traffic");
}
#else
void readWiredData_MTK(void)
{
	FILE *fp = NULL;
	char buf[256] = {0};
	char tmp[32] = {0};
	int port_no = 0;
	unsigned long in;
	unsigned long out;


	system("cat /proc/tc3162/gsw_stats > /tmp/ether_switch_traffic");

	fp = fopen( "/tmp/ether_switch_traffic", "r" );
	if (fp)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			sprintf(tmp, "[ Port %d ]", port_no);
			if (!strncmp(buf, tmp, 10))
			{
				while (fgets(buf, sizeof(buf), fp))
				{
					if (!strncmp(buf, "Rx Good Bytes", 13))
					{
						sscanf(buf, "%*[^=]= 0x%x,%*[^=]= 0x%*x", &in);
						if (port_no < 4)
						{
							WiredData.rx += in;
						}
						else
						{
							EthernetWANData.rx += in;
						}
					}
					else if (!strncmp(buf, "Tx Good Bytes", 13))
					{
						sscanf(buf, "%*[^=]= 0x%x,%*[^=]= 0x%*x", &out);
						if (port_no < 4)
						{
							WiredData.tx += out;
						}
						else
						{
							EthernetWANData.tx += out;
						}
						port_no++;
						break;
					}
				}

				if (get_cur_lanwan_port() == port_no)
				{
					EthernetWANData.rx += in;
					EthernetWANData.tx += out;
				}
			}

			if (port_no >= 5)
				break;
		}

		fclose(fp);
	}
	unlink("/tmp/ether_switch_traffic");
}
#endif

void readWiredData_RTK( void )
{
	char buf[256] = {0};
	char *p = NULL;
	FILE *fp = NULL;

	system("cat /proc/tc3162/ether_switch_traffic > /tmp/ether_switch_traffic");

	fp = fopen("/tmp/ether_switch_traffic", "r" );
	if(fp)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			if((p = strstr(buf, "TotalInOctets:")) != NULL)
			{
				p += strlen("TotalInOctets:");
				sscanf(p, "%llu", &(WiredData.rx) );
			}
			else if((p = strstr(buf, "TotalOutOctets:")) != NULL)
			{
				p += strlen("TotalOutOctets:");
				sscanf(p, "%llu", &(WiredData.tx) );
			}
			else if((p = strstr(buf, "ifInOctets port:0")) != NULL)
			{
				p += strlen("ifInOctets port:0");
				sscanf(p, "%llu", &(EthernetWANData.rx) );
			}
			else if((p = strstr(buf, "ifOutOctets port:0")) != NULL)
			{
				p += strlen("ifOutOctets port:0");
				sscanf(p, "%llu", &(EthernetWANData.tx) );
			}
		}
		fclose(fp);
	}

}

void modifyTrafficData( void )
{
#if defined(TCSUPPORT_WAN_ETHER_LAN)
	WiredData.rx -= EthernetWANData.rx;
	WiredData.tx -= EthernetWANData.tx;
#endif

#ifdef TCSUPPORT_WLAN_RT6856
#else
	Sum2GData.rx = Wireless2GData[0].rx + Wireless2GData[1].rx + Wireless2GData[2].rx + Wireless2GData[3].rx;
	Sum2GData.tx = Wireless2GData[0].tx + Wireless2GData[1].tx + Wireless2GData[2].tx + Wireless2GData[3].tx;
	Sum5GData.rx = Wireless5GData[0].rx + Wireless5GData[1].rx + Wireless5GData[2].rx + Wireless5GData[3].rx;
	Sum5GData.tx = Wireless5GData[0].tx + Wireless5GData[1].tx + Wireless5GData[2].tx + Wireless5GData[3].tx;
#endif
}


static void tcWebApi_UpdateNetdev(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char tmp[256] = {0};

	init_netdevData();

	//read wired traffic
#if defined(TCSUPPORT_MTK_INTERNAL_ETHER_SWITCH) || defined(MT7530_SUPPORT)
	#if defined(TCSUPPORT_CPU_MT7510)
	readWiredData_MTK_mt7510();
	#else
	readWiredData_MTK();
	#endif
#else
	readWiredData_RTK();
#endif

	//read ATM traffic
	readATMData_MTK();

	//read PTM traffic
#if defined(TCSUPPORT_CPU_MT7510)	
	readPTMData_MTK_mt7510();
#else
	readPTMData_MTK();
#endif

	//read wireless traffic
#ifdef TCSUPPORT_WLAN_RT6856
	readWiFiData_RT6856();
#else
	readWiFiData_MTK();
#endif

	modifyTrafficData();

	memset(tmp, 0, sizeof(tmp));
	websWrite( tmp, "\nnetdev = {\n" );
	websWrite( tmp, "'%s':{rx:0x%llx,tx:0x%llx}\n", "WIRED" , WiredData.rx, WiredData.tx );
	websWrite( tmp, ",'%s':{rx:0x%llx,tx:0x%llx}\n", "WIRELESS0" , Sum2GData.rx, Sum2GData.tx );
	websWrite( tmp, ",'%s':{rx:0x%llx,tx:0x%llx}\n", "WIRELESS1" , Sum5GData.rx, Sum5GData.tx );
	websWrite( tmp, ",'%s':{rx:0x%llx,tx:0x%llx}\n", "BRIDGE" , BridgeData.rx, BridgeData.tx );

	//Paul add 2012/10/23
	websWrite( tmp, ",'%s':{rx:0x%llx,tx:0x%llx}\n", "ATM" , ATMData.rx, ATMData.tx );
	websWrite( tmp, ",'%s':{rx:0x%llx,tx:0x%llx}\n", "PTM0" , PTMData[0].rx, PTMData[0].tx );
	websWrite( tmp, ",'%s':{rx:0x%llx,tx:0x%llx}\n", "PTM1" , PTMData[1].rx, PTMData[1].tx );
	websWrite( tmp, ",'%s':{rx:0x%llx,tx:0x%llx}\n", "ETHERNETWAN" , EthernetWANData.rx, EthernetWANData.tx );
	websWrite( tmp, "}" );

}

static void tcWebApi_UpdateBandwidth(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE *fp = NULL;
	char buf[256] = {0};
	char tmp[256] = {0};
	char name[64] = {0};
	int sig;
	char *msg;

	msg = (char*)asp_alloc(reent,params[0].len+1);
	memset(msg,0,params[0].len+1);
	memcpy(msg,params[0].str,params[0].len);

	if(strcmp(msg, "daily") == 0)
	{
		sig =  SIGUSR2;
		strcpy(name, "/var/tmp/rstats-history.js" );
	}
	else
	{
		sig =  SIGUSR1;
		strcpy(name, "/var/tmp/rstats-speed.js" );
	}

	unlink(name);
	killall( "rstats", sig );
	f_wait_exists(name, 5);

	if( (fp = fopen( name, "r" ))  != NULL )
	{
		while (fgets(buf, sizeof(buf), fp)) {
			websWrite( tmp, buf );
		}

		fclose(fp);
	}
	else
	{
		FILE *prt;
		int nfd;
		/* Don't let /dev/console block */
		if((nfd = open("/dev/console", O_WRONLY | O_NONBLOCK)) > 0){
			if((prt = fdopen(nfd, "w")) != NULL){
				fprintf( prt, "===>>file[%s] not exists!!!\n", name );
				fclose(prt);
			}
			close(nfd);
		}
	}
	sleep(2);
	unlink(name);

}

static void update_variables(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char cmd[256] = {0};
	char *action_mode = get_param(g_var_post, "action_mode");
	char *action_script = get_param(g_var_post, "action_script");	//TODO: currently not support multiple service
	char *argv_service[] = {"service", action_script, NULL};
	int pid;

	if(!action_mode || !action_script)
		return;

	if(!strcmp(action_mode, "apply"))
	{
		if(strlen(action_script))
		{
			if( strstr(action_script, "webs_"))
			{
				//webs_update.sh or webs_upgrade.sh in "/usr/script"
				sprintf(cmd, "/usr/script/%s.sh", action_script+strlen("start_") );
				//Andy Chiu, 2015/02/13. use eval to launch script for https
				//system(cmd);
				eval(cmd);
			}
			else
			{
				_eval(argv_service, NULL, 0, &pid);
			}
		}
	}
}
//Ren.E

//debug purpose
static void show_message(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *msg;
	FILE *fp;
	int nfd;
	/* Don't let /dev/console block */
	if((nfd = open("/dev/console", O_WRONLY | O_NONBLOCK)) > 0){
		if((fp = fdopen(nfd, "w")) != NULL){

			msg = (char*)asp_alloc(reent,params[0].len+1);
			memset(msg,0,params[0].len+1);
			memcpy(msg,params[0].str,params[0].len);

			fprintf(fp, "show_message::[%s]\n", msg );

			fclose(fp);
		}
		close(nfd);
	}
}

//usage:show_message2("MLDproxy", "wan_MLD0")
//output:show_message2::[MLDproxy]=[Yes]
static void show_message2(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *msg, *value, *v_tmp;
	FILE *fp;
	int nfd;
	/* Don't let /dev/console block */
	if((nfd = open("/dev/console", O_WRONLY | O_NONBLOCK)) > 0){
		if((fp = fdopen(nfd, "w")) != NULL){

			msg = (char*)asp_alloc(reent,params[0].len+1);
			value = (char*)asp_alloc(reent,params[1].len+1);
			memset(msg,0,params[0].len+1);
			memset(value,0,params[1].len+1);
			memcpy(msg,params[0].str,params[0].len);
			memcpy(value,params[1].str,params[0].len);

			v_tmp = get_param(g_var_post,value);//get_param on web page

			fprintf(fp, "show_message2::[%s]=[%s]\n", msg, v_tmp );

			fclose(fp);
		}
		close(nfd);
	}
}

/**
Usage:

var file_content = '<% show_file_content("/var/tmp/filename"); %>';

**/
static void show_file_content(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *path;
	char buf[256] = {0};
	FILE *fp = NULL;

	path = (char*)asp_alloc(reent,params[0].len+1);
	memset(path,0,params[0].len+1);
	memcpy(path,params[0].str,params[0].len);

	memset(buf, 0, sizeof(buf));
	fp = fopen(path, "r");
	if(fp)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			asp_send_response( NULL, buf, strlen(buf));
		}
		fclose(fp);
	}
	else
	{
		asp_send_response( NULL, buf, strlen(buf));
	}
}

static void ej_memory_usage(asp_reent* reent, const asp_text* params, asp_text* ret){
	unsigned long total, used, mfree/*, shared, buffers, cached*/;
	char buf[80] = {0};
	FILE *fp;
#ifdef TCSUPPORT_SWAP_FILE
#else
	unsigned long sw_total = 0, sw_free = 0, sw_used = 0;
	int retValue = 0;
#endif

	fp = fopen("/proc/meminfo", "r");

	if(fp == NULL)
		return;

	fscanf(fp, "MemTotal: %lu %s\n", &total, buf);
	fscanf(fp, "MemFree: %lu %s\n", &mfree, buf);
	used = total - mfree;
#ifdef TCSUPPORT_SWAP_FILE
#else
	retValue = fscanf(fp, "SwapTotal: %lu %s\n", &sw_total, buf);
	while( retValue == 0 )
	{
		fgets(buf, sizeof(buf), fp);
		retValue = fscanf(fp, "SwapTotal: %lu %s\n", &sw_total, buf);
	}
	retValue = fscanf(fp, "SwapFree: %lu %s\n", &sw_free, buf);
	while( retValue == 0 )
	{
		fgets(buf, sizeof(buf), fp);
		retValue = fscanf(fp, "SwapFree: %lu %s\n", &sw_free, buf);
	}
	sw_used = sw_total - sw_free;
#endif
	fclose(fp);
	websWrite(buf, "Total =%lu \n", total);
	websWrite(buf, "Free =%lu \n", mfree);
	websWrite(buf, "Used =%lu \n", used);
#ifdef TCSUPPORT_SWAP_FILE
#else
	websWrite(buf, "SwapTotal =%lu \n", sw_total);
	websWrite(buf, "SwapFree =%lu \n", sw_free);
	websWrite(buf, "SwapUsed =%lu \n", sw_used);
#endif
}

unsigned long prev_total_0 = 0;
unsigned long prev_user_0 = 0;

static void ej_cpu_usage(asp_reent* reent, const asp_text* params, asp_text* ret){
	unsigned long total, user, nice, system, idle, io, irq, softirq;
	unsigned long total_0_diff = 0, user_0_diff = 0, cpu0_percentage;
	char buf[80] = {0};
	char name[10];
	FILE *fp;
	fp = fopen("/proc/stat", "r");

	if(fp == NULL)
		return;

	while(fscanf(fp, "%s %lu %lu %lu %lu %lu %lu %lu \n", name, &user, &nice, &system, &idle, &io, &irq, &softirq) != EOF){
		if(strcmp(name, "cpu") == 0){
						total = user + nice + system + idle + io + irq + softirq;
			total_0_diff = total - prev_total_0;
			user_0_diff = (system + user + nice + io + irq + softirq) - prev_user_0;
			prev_total_0 = total;
			prev_user_0 = system + user + nice + io + irq + softirq;
			break;
		}
	}

	fclose(fp);
	cpu0_percentage = (100*user_0_diff/total_0_diff);
	websWrite(buf, "cpu_percentage[0] = %lu; \n", cpu0_percentage);
}

static void factory_telnet(asp_reent* reent, const asp_text* params, asp_text* ret){
	system("killall utelnetd");
	system("utelnetd -l /bin/login -d");
}

/*******************************************************************
* NAME: set_factory_mode
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/11/02
* DESCRIPTION: set factory mode for ATE.
* INPUT:
* OUTPUT: 
* RETURN: 
* NOTE:
*******************************************************************/
static void set_factory_mode(asp_reent* reent, const asp_text* params, asp_text* ret){
#ifdef TCSUPPORT_MTK_INTERNAL_ETHER_SWITCH
	//switch switch LED from hardware control to GPIO mode
	system("echo 1 > /proc/tc3162/led_sw_hw2gpio");
#endif
	system("sys button disable");
	printf("1\n");
	system("echo 1 > /tmp/IsATEMode");
}


#ifdef RTCONFIG_SPECTRUM
static void ej_spectrum(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	int sig;

	if(tcapi_match("WebCurSet_Entry", "spectrum_hook_is_running", "1"))
	{
		//on running status, skip.
		return;
	}

	sig = SIGUSR1;
	system("/usr/script/check_spectrum.sh"); //check if spectrum is running.
	sleep(1);

	killall("spectrum", sig);
}
#endif

static void ej_check_acpw(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char result[2];
	char default_acpw[] = "admin"; //hardcode

	strcpy(result, "0");
	if(tcapi_match("Account_Entry0", "username", default_acpw) && tcapi_match("Account_Entry0", "web_passwd", default_acpw))
		strcpy(result, "1");
	else
		strcpy(result, "0");

	asp_send_response( NULL, result, strlen(result));
}


/*____________________________________________________________________*
**	function name: tcWebApi _get
**
**	description:
*     get the attribute value of specific node.
*
**	parameters:
*     reent:not use
*     id:not use
*     params:params[0] node name.
*            params[1] attribute name.
*            params[2] show or hide
*     ret:if params[2] == hide, use ret to return value.
*         if params[2] == show, use asp_send_response() to output string on page.
*
**	global:
*     none
**	return:
*     none
**	call:
*     none
**	revision:
*     1.shnwind
* _____________________________________________________________________*/
static void
tcWebApi_Get (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
#if !defined(TCSUPPORT_CT)
#if defined(TCSUPPORT_GUI_STRING_CONFIG) || defined(TCSUPPORT_GENERAL_MULTILANGUAGE)
#if defined(TCSUPPORT_GENERAL_MULTILANGUAGE)
	static char val[MAX_VALUE];
#else
	static char val[580];
#endif
#endif

	char *node,*attr,*show;
#if  defined(TCSUPPORT_GUI_STRING_CONFIG) || defined(TCSUPPORT_GENERAL_MULTILANGUAGE)
#if defined(TCSUPPORT_GENERAL_MULTILANGUAGE)
	static char retVal[MAX_STREAM];
#else
	char retVal[640];
#endif
#else
	char retVal[129];
#endif
	int r_val;

	node = (char*)asp_alloc(reent,params[0].len+1);
	attr = (char*)asp_alloc(reent,params[1].len+1);
	show = (char*)asp_alloc(reent,params[2].len+1);
	memset(node,0,params[0].len+1);
	memset(attr,0,params[1].len+1);
	memset(show,0,params[2].len+1);
	memcpy(node,params[0].str,params[0].len);
	memcpy(attr,params[1].str,params[1].len);
	memcpy(show,params[2].str,params[2].len);
	node[params[0].len]='\0';
	attr[params[1].len]='\0';
	show[params[2].len]='\0';
#ifdef RA_PARENTALCONTROL
	if(strcmp(node, "Parental") == 0 && strcmp(attr, "BrowserMAC") == 0){
		getClientMacAddr(reent->server_env->remote_ip_addr);
	} else
#endif/*RA_PARENTALCONTROL*/
	{
#if defined(TCSUPPORT_GENERAL_MULTILANGUAGE)
		if (!strcmp(node, "String_Entry"))
		{
			if(0 == getString(attr, val))
				strcpy(val, "N/A");
		}
		else
#endif
		{
			if(!strcmp(attr, "pptpd_clientlist")) {
				r_val = tcapi_get_list(attr, val, sizeof(val));
			}
			else if(!strcmp(attr, "pptpd_sr_rulelist")) {
				r_val = tcapi_get_list(attr, val, sizeof(val));
			}
			else if(!strcmp(node, "VPNC_Entry") && !strcmp(attr, "clientlist")) {
				r_val = tcapi_get_list("vpnc_clientlist", val, sizeof(val));
			}
			else if(!strcmp(attr, "qos_rulelist")) {
				r_val = tcapi_get_list(attr, val, sizeof(val));
			}
			else if(!strcmp(attr, "qos_bw_rulelist")) {
				r_val = tcapi_get_list(attr, val, sizeof(val));
			}
			else if(!strcmp(attr, "MULTIFILTER_DEVICENAME")) {
				r_val = tcapi_get_list(attr, val, sizeof(val));
			}
			else if(!strcmp(attr, "MULTIFILTER_MACFILTER_DAYTIME")) {
				r_val = tcapi_get_list(attr, val, sizeof(val));
			}
			else {
				r_val = tcapi_get(node, attr, val);
			}
			if(r_val < 0){
				strcpy(val,""); /* Paul modify 2013/2/7 */
			}
		}

		if(!strcmp(show,"s")){//show -> s
#if defined(TCSUPPORT_GUI_STRING_CONFIG) || defined(TCSUPPORT_GENERAL_MULTILANGUAGE)
			if(strcmp(node,"String_Entry"))          //not from String_Entry
		    	{
		    		strQuotConvertHTML(val,retVal);
		    	}
			else
			{
				memset(retVal, 0, sizeof(retVal));
				strcpy(retVal, val);
			}
#endif
			asp_send_response (NULL, retVal, strlen(retVal));
		}
		else if(!strcmp(show,"h")){//hide -> h
			if(strlen(val))
				ret->str = val;
			ret->len = strlen(val);
		}
	}
#endif
}
/*_____________________________________________________________________________*
**	function name: tcWebApi _unset
**
**	description:
*     delete the all attribute value of specific node.
*
**	parameters:
*     reent:not use
*     id:not use
*     params:params[0] node name.
*     ret:not use
*
**	global:
*     none
**	return:
*     0:successful
*     -1:fail
**	call:
*     none
**	revision:
*     1.shnwind
* ___________________________________________________________________________*/

static void
tcWebApi_Unset (asp_reent* reent, const asp_text* params,  asp_text* ret)
{

    char *node;
    int r_val;

    node = (char*)asp_alloc(reent,params[0].len+1);
    memset(node,0,params[0].len+1);
    memcpy(node,params[0].str,params[0].len);

    r_val=tcapi_unset(node);

}
/*___________________________________________________________________*
**	function name: tcWebApi_commit
**
**	description:
*     Write the value of specific attribute to file system.
*
**	parameters:
*     reent:not use
*     id:not use
*     params:params[0] node name.
*     ret:not use
*
**	global:
*     none
**	return:
*     0:successful
*     -1:fail
**	call:
*     none
**	revision:
*     1.shnwind
* __________________________________________________________________*/

static void
tcWebApi_Commit (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char *node;
	int r_val;
 #if  defined(TCSUPPORT_WEB_SAVE)
    char bootType[16] = {0};
 #endif
		char tmp[32];
	 	char *tmp_web;
		char currentWeb[64]={0};

    node = (char*)asp_alloc(reent,params[0].len+1);
    memset(node,0,params[0].len+1);
    memcpy(node,params[0].str,params[0].len);

#if defined(TCSUPPORT_WAN_ATM) || defined(TCSUPPORT_WAN_PTM) || defined(TCSUPPORT_WAN_ETHER)
//when someone uses webpage to commit wan, let CC have one more chance to do WAN_STOP or WAN_START for ATM/PTM.
	if(strstr(node,"Wan_PVC")){
		system("echo 1 > /tmp/commitByWeb");
	}
#endif

	r_val=tcapi_commit(node);
	cprintf("commit %s return %d\n", node, r_val);

		/* Paul modify, 2013/3/7 */
		if (!strcmp(node, "LanguageSwitch_Entry"))
		{
			if (1 == islangChanged()) {
				initandparserfile();
			}
		}

		/* Paul modify, 2013/3/12 */
		memset(tmp, 0, sizeof(tmp));
		r_val= tcapi_get("SysInfo_Entry", "x_Setting", tmp);
		if(r_val < 0 || !strcmp(tmp, "0")){
			if (strcmp(node, "LanguageSwitch_Entry"))
			{
				//wait to do. javi
				// if the commit from QIS_admin_pass.asp, we will not set x_Setting as 1.
				tmp_web = get_param(g_var_post, "current_page");

				if(tmp_web == NULL){
					printf("Can not get current_page!!!\n");
				}
				else{
					memcpy(currentWeb,tmp_web, sizeof(currentWeb));
					printf("currentWeb = %s\n", currentWeb);	//javi
				}

				if (strcmp(currentWeb, "QIS_admin_pass.asp") == 0){
					printf("This Web is QIS_admin_pass.asp, so we not set x_Setting as 1\n");
				}
				else{
					tcapi_set("SysInfo_Entry", "x_Setting", "1");
				}
				tcapi_save();
			}
		}

#if  defined(TCSUPPORT_WEB_SAVE)
    flag_save = 1;
    if(strcmp(node, SYSTEM_NODE) == 0) {
	tcapi_get(SYSTEM_NODE, REBOOT_TYPE, bootType);
	if(atoi(bootType) != NO_BOOT)
	     flag_save = 0;//If do system reboot or reset default, should not do save
    }
#endif

#ifdef TCSUPPORT_SYSLOG_ENHANCE
   	openlog("TCSysLog WEB", 0, LOG_LOCAL1);
	syslog(LOG_INFO, "Configuration changed: %s\n", node);
	closelog();
#endif

#ifdef RTCONFIG_TR069
	if(pids("tr069")) {
		dbgprintf("value change from web!\n");
		eval("sendtocli", "http://127.0.0.1:1234/web/value/change", "\"name=change\"");
	}
#endif
}

static void
tcWebApi_CommitWithoutSave (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char *node;
	int r_val;

	node = (char*)asp_alloc(reent,params[0].len+1);
	memset(node,0,params[0].len+1);
	memcpy(node,params[0].str,params[0].len);

	r_val = tcapi_commit(node);
	cprintf("commit %s return %d\n", node, r_val);

	//Andy Chiu, 2015/04/15. Set x_Setting as 1 after configuration
	tcapi_set("SysInfo_Entry", "x_Setting", "1");
}

static void
tcWebApi_delay_CommitWithoutSave (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char *node;
	int r_val;
	char *tmp = NULL;

	node = (char*)asp_alloc(reent,params[0].len+1);
	memset(node,0,params[0].len+1);
	memcpy(node,params[0].str,params[0].len);
	//dbgprintf("node name %s \n", node);

	if (params[1].str && params[1].len) {
		tmp = (char*)asp_alloc(reent,params[1].len+1);
		memset(tmp,0,params[1].len+1);
		memcpy(tmp,params[1].str,params[1].len);
		//dbgprintf("delay time=%d seconds\n", atoi(tmp));
	}
	sleep(atoi(tmp));

	r_val=tcapi_commit(node);
    //dbgprintf("commit ret = %d node %s\n",r_val,node);

}


/*____________________________________________________________________*
**	function name: tcWebApi _save
**
**	description:
*     Write all system parameters to flash
*
**	parameters:
*     reent:not use
*     id:not use
*     params:not use
*     ret:not use
*
**	global:
*     none
**	return:
*     none
**	call:
*     none
**	revision:
*     1.shnwind
* _____________________________________________________________________*/

static void
tcWebApi_Save (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
#if defined(TCSUPPORT_WEB_SAVE)
	flag_save = 0;
#endif
    tcapi_save();
}

static char
hex_to_decimal(char char1,char char2)
{
	return (((char1 >= 'A') ? (((char1 & 0xdf) - 'A') + 10) : (char1 - '0')) * 16) + (((char2 >= 'A') ? (((char2 & 0xdf) - 'A') + 10) : (char2 - '0')));
}

int
decode_uri(char *uri)
{
	char c, d;
  char *uri_old;

  uri_old = uri;
  while ((c = *uri_old)) {
    if (c == '%') {
        uri_old++;
        if ((c = *uri_old++) && (d = *uri_old++))
          *uri++ = hex_to_decimal(c, d);
        else
          return 0;       /* NULL in chars to be decoded */
    }else{
			*uri++ = c;
      uri_old++;
    }
  }
  *uri = '\0';
  return 1;
}

/*krammer add for bug 1321*/
/*____________________________________________________________________*
**	function name: tcWebApi_CurrentDefaultRoute
**
**	description:
*     get the current route pvc number
*
**	parameters:
*     reent:not use
*     id:not use
*     params:not use
*     ret:not use
*
**	global:
*     none
**	return:
*     current default route pvc number
**	call:tcapi_get, asp_send_response
*     none
**	revision:
*     1.krammer
* _____________________________________________________________________*/

static void
tcWebApi_CurrentDefaultRoute(asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char i, defaultRT;
	int r_val;
	char val[DEFAULT_RT_ATTR_LENGTH]={0};
	char wanPvc[16]={0};

	for(i=0; i<MAX_PVC_NUMBER; i++){
		sprintf(wanPvc,"%s%d",WAN_PVC,i);
		r_val=tcapi_get(wanPvc, DEFAULT_RT, val);
		if(r_val<0){
			continue;
		}
		if(!strcmp(val, "Yes")){
			defaultRT=i+TO_ASCII_OFFSET;
			asp_send_response (NULL,&defaultRT,1);
			return;
		}
	}
	/*can't fine default route, we use pvc 0 to be the default route*/
	defaultRT=0+TO_ASCII_OFFSET;
	asp_send_response (NULL,&defaultRT,1);
}

#endif

#if defined(TCSUPPORT_IMPROVE_GUI_PERFM)
/*____________________________________________________________________*
**      function name: tcWebApi_constSet
**
**      description:
*     write the const value which is user input to cfg_manager.
*
**      parameters:
*     reent:not use
*     id:not use
*     params:params[0] node name.
*            params[1] attribute name.
*            params[2] value
*     ret:not use
*
**      global:
*     none
**      return:
*     none
**      call:
*     none
**      revision:
*     1.shnwind
* _____________________________________________________________________*/
static void
tcWebApi_constSet (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
        char *node,*attr,*value;
        int r_val;

        node = (char*)asp_alloc(reent,params[0].len+1);
        attr = (char*)asp_alloc(reent,params[1].len+1);
        value = (char*)asp_alloc(reent,params[2].len+1);
        memset(node,0,params[0].len+1);
        memset(attr,0,params[1].len+1);
        memset(value,0,params[2].len+1);
        memcpy(node,params[0].str,params[0].len);
        memcpy(attr,params[1].str,params[1].len);
        memcpy(value,params[2].str,params[2].len);
        node[params[0].len]='\0';
        attr[params[1].len]='\0';
        value[params[2].len]='\0';

        // can alow set NULL
	/*
        if(params[2].len == 0)
        {
                return;
        }
	*/

        r_val=tcapi_set(node, attr, value);
}
#endif
/*____________________________________________________________________*
**      function name: tcWebApi_staticGet
**
**      description:
*     get the attribute value of specific node.
*
**      parameters:
*     reent:not use
*     id:not use
*     params:params[0] node name.
*            params[1] attribute name.
*            params[2] show or hide
*     ret:if params[2] == hide, use ret to return value.
*         if params[2] == show, use asp_send_response() to output string on page.
*
**      global:
*     none
**      return:
*     none
**      call:
*     none
**      revision:
*     1.shnwind
* _____________________________________________________________________*/
static void
tcWebApi_staticGet (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
        static char val[MAX_ATTRIBUTE_LEN];
        char *node,*attr,*show;
        char retVal[MAX_ATTRIBUTE_LEN];
        int r_val;

        node = (char*)asp_alloc(reent,params[0].len+1);
        attr = (char*)asp_alloc(reent,params[1].len+1);
        show = (char*)asp_alloc(reent,params[2].len+1);
        memset(node,0,params[0].len+1);
        memset(attr,0,params[1].len+1);
        memset(show,0,params[2].len+1);
        memcpy(node,params[0].str,params[0].len);
        memcpy(attr,params[1].str,params[1].len);
        memcpy(show,params[2].str,params[2].len);
        node[params[0].len]='\0';
        attr[params[1].len]='\0';
        show[params[2].len]='\0';
        r_val = tcapi_staticGet(node, attr, val);
        if(r_val < 0)
        {
                strcpy(val,""); /* Paul modify 2013/2/7 */
        }

        //show -> s
        if(!strcmp(show,"s"))
        {
                strQuotConvertHTML(val,retVal);
                asp_send_response (NULL,retVal,strlen(retVal));
        }
        //hide -> h
        else if(!strcmp(show,"h"))
        {
                if(strlen(val))
                        ret->str = val;
                ret->len = strlen(val);
        }
}


#ifdef RA_PARENTALCONTROL
/* Add for Time Restrict by richard.lai */
/*____________________________________________________________________*
**	function name: getClientMacAddr
**
**	description:
*     Get Client Mac Address.
*
**	parameters:
*     reent:not use
*     id:not use
*     params:not use
*     ret:not use
*
**	global:
*     none
**	return:
*     none
**	call:
*     none
**	revision:
*     1.shnwind
* _____________________________________________________________________*/
#define PROC_ARP "/proc/net/arp"
static void
getClientMacAddr (char * ip_addr) {

	static char val[129];
	char retVal[129];
	FILE *fp;
	char buf[80];
	char ipaddr[16], hwtype[16], flags[16], hwaddr[20], mask[16], device[16], state[16];

	fp = fopen(PROC_ARP, "r");
	if(fp) {
		fgets(buf, 80, fp);
		while(fscanf(fp, "%s %s %s %s %s %s %s\n", ipaddr, hwtype, flags, hwaddr, mask, device, state) == 7) {
			if(strcmp(ipaddr, ip_addr) == 0) {
				strcpy(val, hwaddr);
				strQuotConvertHTML(val, retVal);
				asp_send_response (NULL, retVal, strlen(retVal));
			}
		}
		fclose(fp);
	}
}
#endif/*RA_PARENTALCONTROL*/

static void
get_post_parameter (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *val,*tmp;

	if (params[0].str && params[0].len) {
		tmp = (char*)asp_alloc(reent,params[0].len+1);
		memset(tmp,0,params[0].len+1);
		memcpy(tmp,params[0].str,params[0].len);
		val = get_param(g_var_post,tmp);

		//dbgprintf("request name %s value %s\n",tmp,val);
		if(val ==NULL)
			return;

		decode_uri(val);
		//ret->str = val;
		//ret->len = strlen(val);
		asp_send_response(NULL, val, strlen(val));
	}
}

static void
get_all_accounts (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char tmp[256];
	int i;
#ifdef RTCONFIG_USB
	int acc_num;
	char **account_list = NULL;
	int result, first;

	if ((result = get_account_list(&acc_num, &account_list)) < 0){
		printf("Failed to get the account list!\n");
		return;// -1;
	}

	first = 1;
	for (i = 0; i < acc_num; ++i){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);//websWrite(wp, ", ");

		//websWrite(wp, "\"%s\"", account_list[i]);
		sprintf(tmp, "\"%s\"", account_list[i]);
		asp_send_response(NULL, tmp, strlen(tmp));
	}

	free_2_dimension_list(&acc_num, &account_list);
#endif

#ifdef RTCONFIG_OPENVPN
	do {
		openvpn_accnt_info_t account_info;

		get_openvpn_account(&account_info);

		for (i=0; i<account_info.count; i++)
		{
			sprintf(tmp, ", \"%s\"", account_info.account[i].name);
			asp_send_response(NULL, tmp, strlen(tmp));
		}
	} while(0);
#endif
	
	return;// 0;
}



#ifdef RTCONFIG_USB
/*____________________________________________________________________
	function name:
		get_folder_tree

	description:
		get folder tree and write to web page

	parameters:
		reent:not use
		params:not use
		ret:not use

	global:
		none
	return:
		none
	call:
		none
	revision:
		1.Sam
 _____________________________________________________________________*/
static void get_folder_tree(asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *layer_order = get_param(g_var_post, "layer_order");//websGetVar(wp, "layer_order", "");
	char *follow_info, *follow_info_end, backup;
	int layer = 0, first;
	int disk_count, partition_count, folder_count;
	int disk_order = -1, partition_order = -1;
	disk_info_t *disks_info, *follow_disk;
	partition_info_t *follow_partition;
	char tmp[256];

	if (strlen(layer_order) <= 0){
		fprintf(stderr, "No input \"layer_order\"!\n");
		return;
	}

	follow_info = index(layer_order, '_');
	while (follow_info != NULL && *follow_info != 0){
		++layer;
		++follow_info;
		if (*follow_info == 0)
			break;
		follow_info_end = follow_info;
		while (*follow_info_end != 0 && isdigit(*follow_info_end))
			++follow_info_end;
		backup = *follow_info_end;
		*follow_info_end = 0;

		if (layer == 1)
			disk_order = atoi(follow_info);
		else if (layer == 2)
			partition_order = atoi(follow_info);
		else{
			*follow_info_end = backup;
			fprintf(stderr, "Input \"%s\" is incorrect!\n", layer_order);
			return;
		}

		*follow_info_end = backup;
		follow_info = follow_info_end;
	}

	disks_info = read_disk_data();
	if (disks_info == NULL){
		fprintf(stderr, "Can't read the information of disks.\n");
		return;
	}

	first = 1;
	disk_count = 0;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next, ++disk_count){
		partition_count = 0;
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next, ++partition_count){
			if (layer != 0 && follow_partition->mount_point != NULL && strlen(follow_partition->mount_point) > 0){
				int i;
				char **folder_list;
				int result;
				result = get_all_folder(follow_partition->mount_point, &folder_count, &folder_list);
				if (result < 0){
					fprintf(stderr, "get_disk_tree: Can't get the folder list in \"%s\".\n", follow_partition->mount_point);

					folder_count = 0;
				}

				if (layer == 2 && disk_count == disk_order && partition_count == partition_order){
					for (i = 0; i < folder_count; ++i){
						if (first == 1)
							first = 0;
						else
							asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

						//websWrite(wp, "\"%s#%u#0\"", folder_list[i], i);
						sprintf(tmp, "\"%s#%u#0\"", folder_list[i], i);
						asp_send_response(NULL, tmp, strlen(tmp));
					}
				}
				else if (layer == 1 && disk_count == disk_order){
					if (first == 1)
						first = 0;
					else
						asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

					follow_info = rindex(follow_partition->mount_point, '/');
					//websWrite(wp, "\"%s#%u#%u\"", follow_info+1, partition_count, folder_count);
					sprintf(tmp, "\"%s#%u#%u\"", follow_info+1, partition_count, folder_count);
					asp_send_response(NULL, tmp, strlen(tmp));
				}

				free_2_dimension_list(&folder_count, &folder_list);
			}
		}
		if (layer == 0){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

			//websWrite(wp, "\"%s#%u#%u\"", follow_disk->tag, disk_count, partition_count);
			sprintf(tmp, "\"%s#%u#%u\"", follow_disk->tag, disk_count, partition_count);
			asp_send_response(NULL, tmp, strlen(tmp));
		}

		if (layer > 0 && disk_count == disk_order)
			break;
	}

	free_disk_data(&disks_info);

	return;
}

static void disk_pool_mapping_info (asp_reent* reent, const asp_text* params, asp_text* ret) {
	disk_info_t *disks_info, *follow_disk;
	partition_info_t *follow_partition;
	int first;
	char *Ptr;
	char tmp[1024];

	disks_info = read_disk_data();
	if (disks_info == NULL){
		//websWrite(wp, "%s", initial_disk_pool_mapping_info());
		sprintf(tmp, "function total_disk_sizes() { return [];}\n\
			function disk_interface_names() { return []; }\n\
			function pool_names() { return [];}\n\
			function pool_types() { return [];}\n\
			function pool_mirror_counts() { return [];}\n\
			function pool_status() { return [];}\n\
			function pool_kilobytes() { return [];}\n\
			function pool_encryption_password_is_missing() { return [];}\n\
			function pool_kilobytes_in_use() { return [];}\n\
			function pool_usage_kilobytes() { return [];}\n\
			function disk_usage() { return [];}\n\
			function per_pane_pool_usage_kilobytes(pool_num, disk_num) { return [];}\n");
		asp_send_response(NULL, tmp, strlen(tmp));
		return;// -1;
	}

	// websWrite(wp, "function total_disk_sizes(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function total_disk_sizes(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

		//websWrite(wp, "\"%llu\"", follow_disk->size_in_kilobytes);
		sprintf(tmp, "\"%llu\"", follow_disk->size_in_kilobytes);
		asp_send_response(NULL, tmp, strlen(tmp));
	}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	// websWrite(wp, "function disk_interface_names(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function disk_interface_names(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

		//websWrite(wp, "\"usb\"");
		asp_send_response(NULL, "\"usb\"", 5);
	}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	// websWrite(wp, "function pool_names(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function pool_names(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));

	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

			if (follow_partition->mount_point == NULL){
				//websWrite(wp, "\"%s\"", follow_partition->device);
				sprintf(tmp, "\"%s\"", follow_partition->device);
				asp_send_response(NULL, tmp, strlen(tmp));
				continue;
			}

			Ptr = rindex(follow_partition->mount_point, '/');
			if (Ptr == NULL){
				asp_send_response(NULL, "\"unknown\"", 9);	//websWrite(wp, "\"unknown\"");
				continue;
			}

			++Ptr;
			//websWrite(wp, "\"%s\"", Ptr);
			sprintf(tmp, "\"%s\"", Ptr);
			asp_send_response(NULL, tmp, strlen(tmp));
		}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	// websWrite(wp, "function pool_devices(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function pool_devices(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));

	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

			//websWrite(wp, "\"%s\"", follow_partition->device);
			sprintf(tmp, "\"%s\"", follow_partition->device);
			asp_send_response(NULL, tmp, strlen(tmp));
		}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n\
			}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	// websWrite(wp, "function pool_types(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function pool_types(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

			if (follow_partition->mount_point == NULL){
				//websWrite(wp, "\"unknown\"");
				sprintf(tmp, "\"unknown\"");
				asp_send_response(NULL, tmp, strlen(tmp));
				continue;
			}

			//websWrite(wp, "\"%s\"", follow_partition->file_system);
			sprintf(tmp, "\"%s\"", follow_partition->file_system);
			asp_send_response(NULL, tmp, strlen(tmp));
		}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n\
			}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	// websWrite(wp, "function pool_mirror_counts(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function pool_mirror_counts(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

			asp_send_response(NULL, "0", 1);	//websWrite(wp, "0");
		}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	// websWrite(wp, "function pool_status(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function pool_status(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

			if (follow_partition->mount_point == NULL){
				//websWrite(wp, "\"unmounted\"");
				sprintf(tmp, "\"unmounted\"");
				asp_send_response(NULL, tmp, strlen(tmp));
				continue;
			}

			//if (strcmp(follow_partition->file_system, "ntfs") == 0)
			//	websWrite(wp, "\"ro\"");
			//else
			asp_send_response(NULL, "\"rw\"", 4);	//websWrite(wp, "\"rw\"");
		}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	// websWrite(wp, "function pool_kilobytes(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function pool_kilobytes(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

			//websWrite(wp, "%llu", follow_partition->size_in_kilobytes);
			sprintf(tmp, "%llu", follow_partition->size_in_kilobytes);
			asp_send_response(NULL, tmp, strlen(tmp));
		}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n\
			}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	// websWrite(wp, "function pool_encryption_password_is_missing(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function pool_encryption_password_is_missing(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

			asp_send_response(NULL, "\"no\"", 4);	//websWrite(wp, "\"no\"");
		}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	// websWrite(wp, "function pool_kilobytes_in_use(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function pool_kilobytes_in_use(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

			//websWrite(wp, "%llu", follow_partition->used_kilobytes);
			sprintf(tmp, "%llu", follow_partition->used_kilobytes);
			asp_send_response(NULL, tmp, strlen(tmp));
		}

	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	u64 disk_used_kilobytes;

	// websWrite(wp, "function disk_usage(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function disk_usage(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

		disk_used_kilobytes = 0;
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next)
			disk_used_kilobytes += follow_partition->size_in_kilobytes;

		//websWrite(wp, "%llu", disk_used_kilobytes);
		sprintf(tmp, "%llu", disk_used_kilobytes);
		asp_send_response(NULL, tmp, strlen(tmp));
	}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	disk_info_t *follow_disk2;
	u32 disk_num, pool_num;
	//websWrite(wp, "function per_pane_pool_usage_kilobytes(pool_num, disk_num){\n");
	sprintf(tmp, "function per_pane_pool_usage_kilobytes(pool_num, disk_num){\n");
	asp_send_response(NULL, tmp, strlen(tmp));
	for (follow_disk = disks_info, pool_num = 0; follow_disk != NULL; follow_disk = follow_disk->next){
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next, ++pool_num){
			//websWrite(wp, "    if (pool_num == %d){\n", pool_num);
			sprintf(tmp, "\tif (pool_num == %d){\n", pool_num);
			asp_send_response(NULL, tmp, strlen(tmp));
			if (follow_partition->mount_point != NULL){
				for (follow_disk2 = disks_info, disk_num = 0; follow_disk2 != NULL; follow_disk2 = follow_disk2->next, ++disk_num){
					//websWrite(wp, "	if (disk_num == %d) {\n", disk_num);
					sprintf(tmp, "\tif (disk_num == %d) {\n", disk_num);
					asp_send_response(NULL, tmp, strlen(tmp));

					//if (strcmp(follow_disk2->tag, follow_disk->tag) == 0)
					if (follow_disk2->major == follow_disk->major && follow_disk2->minor == follow_disk->minor) {
						//websWrite(wp, "	    return [%llu];\n", follow_partition->size_in_kilobytes);
						sprintf(tmp, "\t\treturn [%llu];\n", follow_partition->size_in_kilobytes);
						asp_send_response(NULL, tmp, strlen(tmp));
					}
					else {
						//websWrite(wp, "	    return [0];\n");
						sprintf(tmp, "\t\treturn [0];\n");
						asp_send_response(NULL, tmp, strlen(tmp));
					}

					asp_send_response(NULL, "\t}\n", 3);	//websWrite(wp, "	}\n");
				}
			}
			else {
				//websWrite(wp, "	return [0];\n");
				sprintf(tmp, "\treturn [0];\n");
				asp_send_response(NULL, tmp, strlen(tmp));
			}
			asp_send_response(NULL, "\t}\n", 3);	//websWrite(wp, "	}\n");
		}
	}
	// websWrite(wp, "}\n\n");
	asp_send_response(NULL, "}\n\n", 3);
	free_disk_data(&disks_info);

	return;// 0;
}

static void available_disk_names_and_sizes (asp_reent* reent, const asp_text* params, asp_text* ret) {
	disk_info_t *disks_info, *follow_disk;
	int first;
	char tmp[1024];

	// websWrite(wp, "function available_disks(){ return [];}\n\n");
	// websWrite(wp, "function available_disk_sizes(){ return [];}\n\n");
	// websWrite(wp, "function claimed_disks(){ return [];}\n\n");
	// websWrite(wp, "function claimed_disk_interface_names(){ return [];}\n\n");
	// websWrite(wp, "function claimed_disk_model_info(){ return [];}\n\n");
	// websWrite(wp, "function claimed_disk_total_size(){ return [];}\n\n");
	// websWrite(wp, "function claimed_disk_total_mounted_number(){ return [];}\n\n");
	// websWrite(wp, "function blank_disks(){ return [];}\n\n");
	// websWrite(wp, "function blank_disk_interface_names(){ return [];}\n\n");
	// websWrite(wp, "function blank_disk_model_info(){ return [];}\n\n");
	// websWrite(wp, "function blank_disk_total_size(){ return [];}\n\n");
	// websWrite(wp, "function blank_disk_total_mounted_number(){ return [];}\n\n");
	sprintf(tmp, "function available_disks(){ return [];}\n\n\
		function available_disk_sizes(){ return [];}\n\n\
		function claimed_disks(){ return [];}\n\n\
		function claimed_disk_interface_names(){ return [];}\n\n\
		function claimed_disk_model_info(){ return [];}\n\n\
		function claimed_disk_total_size(){ return [];}\n\n\
		function claimed_disk_total_mounted_number(){ return [];}\n\n\
		function blank_disks(){ return [];}\n\n\
		function blank_disk_interface_names(){ return [];}\n\n\
		function blank_disk_model_info(){ return [];}\n\n\
		function blank_disk_total_size(){ return [];}\n\n\
		function blank_disk_total_mounted_number(){ return [];}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	disks_info = read_disk_data();
	if (disks_info == NULL){
		//websWrite(wp, "%s", initial_available_disk_names_and_sizes());
		sprintf(tmp, "function foreign_disks() { return [];}\n\
			function foreign_disk_interface_names() { return [];}\n\
			function foreign_disk_model_info() { return [];}\n\
			function foreign_disk_total_size() { return [];}\n\
			function foreign_disk_total_mounted_number() { return [];}\n");
		asp_send_response(NULL, tmp, strlen(tmp));
		return;// -1;
	}

	/* show name of the foreign disks */
	// websWrite(wp, "function foreign_disks(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function foreign_disks(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

		//websWrite(wp, "\"%s\"", follow_disk->tag);
		sprintf(tmp, "\"%s\"", follow_disk->tag);
		asp_send_response(NULL, tmp, strlen(tmp));
	}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	/* show interface of the foreign disks */
	// websWrite(wp, "function foreign_disk_interface_names(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function foreign_disk_interface_names(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

//		websWrite(wp, "\"USB\"");
		//websWrite(wp, "\"%s\"", follow_disk->port);
		sprintf(tmp, "\"%s\"", follow_disk->port);
		asp_send_response(NULL, tmp, strlen(tmp));
	}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	/* show model info of the foreign disks */
	// websWrite(wp, "function foreign_disk_model_info(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function foreign_disk_model_info(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

		asp_send_response(NULL, "\"", 1);	//websWrite(wp, "\"");

		if (follow_disk->vendor != NULL) {
			//websWrite(wp, "%s", follow_disk->vendor);
			sprintf(tmp, "%s", follow_disk->vendor);
			asp_send_response(NULL, tmp, strlen(tmp));
		}
		if (follow_disk->model != NULL){
			if (follow_disk->vendor != NULL)
				asp_send_response(NULL, " ", 1);	//websWrite(wp, " ");

			//websWrite(wp, "%s", follow_disk->model);
			sprintf(tmp, "%s", follow_disk->model);
			asp_send_response(NULL, tmp, strlen(tmp));
		}
		asp_send_response(NULL, "\"", 1);	//websWrite(wp, "\"");
	}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	/* show total_size of the foreign disks */
	// websWrite(wp, "function foreign_disk_total_size(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function foreign_disk_total_size(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

		//websWrite(wp, "\"%llu\"", follow_disk->size_in_kilobytes);
		sprintf(tmp, "\"%llu\"", follow_disk->size_in_kilobytes);
		asp_send_response(NULL, tmp, strlen(tmp));
	}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n\
			}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	/* show total number of the mounted partition in this foreign disk */
	// websWrite(wp, "function foreign_disk_total_mounted_number(){\n");
	// websWrite(wp, "    return [");
	sprintf(tmp, "function foreign_disk_total_mounted_number(){\n\treturn [");
	asp_send_response(NULL, tmp, strlen(tmp));
	first = 1;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);	//websWrite(wp, ", ");

		//websWrite(wp, "\"%u\"", follow_disk->mounted_number);
		sprintf(tmp, "\"%u\"", follow_disk->mounted_number);
		asp_send_response(NULL, tmp, strlen(tmp));
	}
	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(tmp, "];\n}\n\n");
	asp_send_response(NULL, tmp, strlen(tmp));
	free_disk_data(&disks_info);

	return;// 0;
}

static void get_AiDisk_status (asp_reent* reent, const asp_text* params, asp_text* ret) {
	disk_info_t *disks_info, *follow_disk;
	partition_info_t *follow_partition;
	int sh_num;
	char **folder_list = NULL;
	int first_pool, result, i;
	char tmp[1024];
	char enable_samba[4];
	char enable_ftp[4];
	char st_samba_mode[4];
	char st_ftp_mode[4];
	char st_webdav_mode[4];

	tcapi_get("Samba_Entry", "Active", enable_samba);
	tcapi_get("Samba_Entry", "enable_ftp", enable_ftp);
	tcapi_get("Samba_Entry", "st_samba_mode", st_samba_mode);
	tcapi_get("Samba_Entry", "st_ftp_mode", st_ftp_mode);
	tcapi_get("Samba_Entry", "st_webdav_mode", st_webdav_mode);

/*	websWrite(wp, "function get_cifs_status(){\n");
	// websWrite(wp, "    return %d;\n", atoi(nvram_safe_get("samba_running")));
	websWrite(wp, "    return %d;\n", atoi(nvram_safe_get("enable_samba")));
	websWrite(wp, "}\n\n");

	websWrite(wp, "function get_ftp_status(){\n");
	// websWrite(wp, "    return %d;\n", atoi(nvram_safe_get("ftp_running")));
	websWrite(wp, "    return %d;\n", atoi(nvram_safe_get("enable_ftp")));
	websWrite(wp, "}\n\n");

#ifdef RTCONFIG_WEBDAV_PENDING
	websWrite(wp, "function get_webdav_status(){\n");
	// websWrite(wp, "    return %d;\n", atoi(nvram_safe_get("ftp_running")));
	websWrite(wp, "    return %d;\n", atoi(nvram_safe_get("enable_webdav")));
	websWrite(wp, "}\n\n");
#endif

	websWrite(wp, "function get_dms_status(){\n");
	websWrite(wp, "    return %d;\n", pids("ushare"));
	websWrite(wp, "}\n\n");

	websWrite(wp, "function get_share_management_status(protocol){\n");
	websWrite(wp, "    if (protocol == \"cifs\")\n");
	websWrite(wp, "	return %d;\n", atoi(nvram_safe_get("st_samba_mode")));
	websWrite(wp, "    else if (protocol == \"ftp\")\n");
	websWrite(wp, "	return %d;\n", atoi(nvram_safe_get("st_ftp_mode")));
#ifdef RTCONFIG_WEBDAV_PENDING
	websWrite(wp, "    else if (protocol == \"webdav\")\n");
	websWrite(wp, "	return %d;\n", atoi(nvram_safe_get("st_webdav_mode")));
#endif
	websWrite(wp, "    else\n");
	websWrite(wp, "	return -1;\n");
	websWrite(wp, "}\n\n");*/
	sprintf(tmp, "function get_cifs_status(){\n\
		return %d;\n\
		}\n\n\
		function get_ftp_status(){\n\
		return %d;\n\
		}\n\n", strcmp(enable_samba, "Yes")? 0 : 1,
		strcmp(enable_ftp, "Yes")? 0 : 1);
	asp_send_response(NULL, tmp, strlen(tmp));
#ifdef RTCONFIG_WEBDAV_PENDING
	// websWrite(wp, "function get_webdav_status(){\n");

	// websWrite(wp, "    return %d;\n", atoi(nvram_safe_get("enable_webdav")));
	// websWrite(wp, "}\n\n");
#endif

	sprintf(tmp, "function get_dms_status(){\n\
		return %d;\n\
		}\n\n", pids("ushare"));
	asp_send_response(NULL, tmp, strlen(tmp));

	sprintf(tmp, "function get_share_management_status(protocol){\n\
		if (protocol == \"cifs\")\n\
		return %d;\n\
		else if (protocol == \"ftp\")\n\
		return %d;\n\
		else if (protocol == \"webdav\")\n\
		return %d;\n\
		else\n\
		return -1;\n\
		}\n\n", atoi(st_samba_mode), atoi(st_ftp_mode),	atoi(st_webdav_mode)
		);
	asp_send_response(NULL, tmp, strlen(tmp));

	disks_info = read_disk_data();
	if (disks_info == NULL){
		//websWrite(wp, "function get_sharedfolder_in_pool(poolName){}\n");
		sprintf(tmp, "function get_sharedfolder_in_pool(poolName){}\n");
		asp_send_response(NULL, tmp, strlen(tmp));
		return;// -1;
	}
	first_pool = 1;
	//websWrite(wp, "function get_sharedfolder_in_pool(poolName){\n");
	sprintf(tmp, "function get_sharedfolder_in_pool(poolName){\n");
	asp_send_response(NULL, tmp, strlen(tmp));
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
			if (follow_partition->mount_point != NULL && strlen(follow_partition->mount_point) > 0){
				asp_send_response(NULL, "\t", 1);//websWrite(wp, "    ");

				if (first_pool == 1)
					first_pool = 0;
				else
					asp_send_response(NULL, "else ", 5);//websWrite(wp, "else ");

#ifdef OLD_AIDISK
				//websWrite(wp, "if (poolName == \"%s\"){\n", rindex(follow_partition->mount_point, '/')+1);
				sprintf(tmp, "if (poolName == \"%s\"){\n", rindex(follow_partition->mount_point, '/')+1);
				asp_send_response(NULL, tmp, strlen(tmp));
#else
				//websWrite(wp, "if (poolName == \"%s\"){\n", follow_partition->device);
				sprintf(tmp, "if (poolName == \"%s\"){\n", follow_partition->device);
				asp_send_response(NULL, tmp, strlen(tmp));
#endif
				//websWrite(wp, "	return [\"\"");
				sprintf(tmp, "\treturn [\"\"");
				asp_send_response(NULL, tmp, strlen(tmp));

				result = get_all_folder(follow_partition->mount_point, &sh_num, &folder_list);
				if (result < 0){
					// websWrite(wp, "];\n");
					// websWrite(wp, "    }\n");
					sprintf(tmp, "];\n\t}\n");
					asp_send_response(NULL, tmp, strlen(tmp));

					printf("get_AiDisk_status: Can't get the folder list in \"%s\".\n", follow_partition->mount_point);

					free_2_dimension_list(&sh_num, &folder_list);

					continue;
				}

				for (i = 0; i < sh_num; ++i){
					asp_send_response(NULL, ", ", 2);//websWrite(wp, ", ");

					//websWrite(wp, "\"%s\"", folder_list[i]);
					sprintf(tmp, "\"%s\"", folder_list[i]);
					asp_send_response(NULL, tmp, strlen(tmp));
#if 0
//	tmp test
					printf("[httpd] chk folder list[%s]:\n", folder_list[i]);
					for (j=0; j<strlen(folder_list[i]); ++j)
					{
						printf("[%x] ", folder_list[i][j]);
					}
					printf("\nlen:(%d)\n", strlen(folder_list[i]));
//	tmp test ~
#endif

				}

				// websWrite(wp, "];\n");
				// websWrite(wp, "    }\n");
				sprintf(tmp, "];\n\t}\n");
				asp_send_response(NULL, tmp, strlen(tmp));
			}
		}

	asp_send_response(NULL, "}\n\n", 3);//websWrite(wp, "}\n\n");

	if (disks_info != NULL){
		free_2_dimension_list(&sh_num, &folder_list);
		free_disk_data(&disks_info);
	}

	return;// 0;
}

static void
create_sharedfolder (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *account = get_param(g_var_post, "account");//websGetVar(wp, "account", NULL);
	char *pool = get_param(g_var_post, "pool");//websGetVar(wp, "pool", "");
	char *folder = get_param(g_var_post, "folder");//websGetVar(wp, "folder", "");
	char mount_path[PATH_MAX];
	char tmp[1024];

	if (strlen(pool) <= 0){
		show_error_msg("Input7");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ncreate_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (strlen(folder) <= 0){
		show_error_msg("Input9");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ncreate_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	if (get_mount_path(pool, mount_path, PATH_MAX) < 0){
		fprintf(stderr, "Can't get the mount_path of %s.\n", pool);

		show_error_msg("System1");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ncreate_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (add_folder(account, mount_path, folder) < 0){
		show_error_msg("Action5");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ncreate_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	// if (notify_rc_for_nas("restart_ftpsamba") != 0){
		// show_error_msg("Action5");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }
	eval("/sbin/rc_call", "ftpsamba", "2");	//trigger ftp/samba restart

	// websWrite(wp, "<script>\n");
	// websWrite(wp, "create_sharedfolder_success();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\ncreate_sharedfolder_success();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	return;// 0;
}

static void
delete_sharedfolder (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *pool = get_param(g_var_post, "pool");//websGetVar(wp, "pool", "");
	char *folder = get_param(g_var_post, "folder");//websGetVar(wp, "folder", "");
	char mount_path[PATH_MAX];
	char tmp[1024];

	if (strlen(pool) <= 0){
		show_error_msg("Input7");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ndelete_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (strlen(folder) <= 0){
		show_error_msg("Input9");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ndelete_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	if (get_mount_path(pool, mount_path, PATH_MAX) < 0){
		show_error_msg("System1");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ndelete_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	//Andy Chiu, 2015/01/28. decode path form UTF-8.
	//con_dbgprintf("[%s, %d](%s, %s)\n", __FUNCTION__, __LINE__, mount_path, folder);
	char path[1024], buf[1024];
	sprintf(buf, "%s/%s", mount_path, folder);
	if(decode(buf, path) > 0)
		con_dbgprintf("[%s, %d]path:%s\n", __FUNCTION__, __LINE__, path);

	if(rmdir(path) < 0)
	{
		show_error_msg("Action6");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ndelete_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		con_dbgprintf("[%s, %d]error_msg(%s), errno\n", __FUNCTION__, __LINE__, error_msg, strerror(errno));
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#if 0
	if (del_folder(mount_path, folder) < 0){
		show_error_msg("Action6");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ndelete_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#endif
	// if (notify_rc_for_nas("restart_ftpsamba") != 0){
		// show_error_msg("Action6");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }
	eval("/sbin/rc_call", "ftpsamba", "2");	//trigger ftp/samba restart

	// websWrite(wp, "<script>\n");
	// websWrite(wp, "delete_sharedfolder_success();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\ndelete_sharedfolder_success();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	return;// 0;
}

inline int ishex(int x)
{
	return	(x >= '0' && x <= '9')	||
		(x >= 'a' && x <= 'f')	||
		(x >= 'A' && x <= 'F');
}

int decode(const char *s, char *dec)
{
	char *o;
	const char *end = s + strlen(s);
	int c;

	for (o = dec; s <= end; o++) {
		c = *s++;
		if (c == '+') c = ' ';
		else if (c == '%' && (	!ishex(*s++)	||
					!ishex(*s++)	||
					!sscanf(s - 2, "%2x", &c)))
			return -1;

		if (dec) *o = c;
	}

	return o - dec;
}

static void
modify_sharedfolder (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *pool = get_param(g_var_post, "pool");//websGetVar(wp, "pool", "");
	char *folder = get_param(g_var_post, "folder");//websGetVar(wp, "folder", "");
	char *new_folder = get_param(g_var_post, "new_folder");//websGetVar(wp, "new_folder", "");
	char mount_path[PATH_MAX];
	char tmp[1024];

	if (strlen(pool) <= 0){
		show_error_msg("Input7");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		con_dbgprintf("[%s, %d]error_msg(%s)\n", __FUNCTION__, __LINE__, error_msg);
		sprintf(tmp, "<script>\nmodify_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (strlen(folder) <= 0){
		show_error_msg("Input9");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nmodify_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		con_dbgprintf("[%s, %d]error_msg(%s)\n", __FUNCTION__, __LINE__, error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (strlen(new_folder) <= 0){
		show_error_msg("Input17");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nmodify_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		con_dbgprintf("[%s, %d]error_msg(%s)\n", __FUNCTION__, __LINE__, error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (get_mount_path(pool, mount_path, PATH_MAX) < 0){
		show_error_msg("System1");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nmodify_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		con_dbgprintf("[%s, %d]error_msg(%s)\n", __FUNCTION__, __LINE__, error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	//Andy Chiu, 2015/01/28. decode path form UTF-8.
	//con_dbgprintf("[%s, %d](%s, %s, %s)\n", __FUNCTION__, __LINE__, mount_path, folder, new_folder);
	
	char new_path[1024], old_path[1024], path[1024], buf[1024];
	sprintf(buf, "%s", new_folder);
	if(decode(buf, new_path) > 0) {
		con_dbgprintf("[%s, %d]new:%s\n", __FUNCTION__, __LINE__, new_path);
	}
	sprintf(buf, "%s", folder);
	if(decode(buf, old_path) > 0) {
		con_dbgprintf("[%s, %d]old:%s\n", __FUNCTION__, __LINE__, old_path);
	}
	sprintf(buf, "%s", mount_path);
	if(decode(buf, path) > 0) {
		con_dbgprintf("[%s, %d]mount_path:%s\n", __FUNCTION__, __LINE__, path);
	}

	//Carlos 2015/08/03, should be call mod_folder api
	if(mod_folder(mount_path, old_path, new_path) < 0)
	{
		show_error_msg("Action7");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nmodify_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		con_dbgprintf("[%s, %d]error_msg(%s), errno\n", __FUNCTION__, __LINE__, error_msg, strerror(errno));
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#if 0
	if (mod_folder(mount_path, folder, new_folder) < 0){
		show_error_msg("Action7");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nmodify_sharedfolder_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	// if (notify_rc_for_nas("restart_ftpsamba") != 0){
		// show_error_msg("Action7");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_sharedfolder_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }
#endif
	eval("/sbin/rc_call", "ftpsamba", "2");	//trigger ftp/samba restart

	// websWrite(wp, "<script>\n");
	// websWrite(wp, "modify_sharedfolder_success();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\nmodify_sharedfolder_success();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	return;// 0;
}

static void
get_permissions_of_account (asp_reent* reent, const asp_text* params, asp_text* ret) {
	disk_info_t *disks_info, *follow_disk;
	partition_info_t *follow_partition;
	int acc_num = 0, sh_num = 0;
	char *account, **account_list = NULL, **folder_list;
	int samba_right, ftp_right;
#ifdef RTCONFIG_WEBDAV_PENDING
	int webdav_right;
#endif
	int result, i, j;
	int first_pool, first_account;
	char tmp[1024];

	disks_info = read_disk_data();
	if (disks_info == NULL){
		//websWrite(wp, "function get_account_permissions_in_pool(account, pool){return [];}\n");
		sprintf(tmp, "function get_account_permissions_in_pool(account, pool){return [];}\n");
		asp_send_response(NULL, tmp, strlen(tmp));
		return;// -1;
	}

	result = get_account_list(&acc_num, &account_list);
	if (result < 0){
		printf("1. Can't get the account list.\n");
		free_2_dimension_list(&acc_num, &account_list);
		free_disk_data(&disks_info);
	}

	//websWrite(wp, "function get_account_permissions_in_pool(account, pool){\n");
	sprintf(tmp, "function get_account_permissions_in_pool(account, pool){\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	first_account = 1;
	for (i = -1; i < acc_num; ++i){
		asp_send_response(NULL, "\t", 1);//websWrite(wp, "    ");
		if (first_account == 1)
			first_account = 0;
		else
			asp_send_response(NULL, "else ", 5);//websWrite(wp, "else ");

		if(i == -1){ // share mode.
			account = NULL;

			//websWrite(wp, "if (account == null){\n");
			sprintf(tmp, "if (account == null){\n");
			asp_send_response(NULL, tmp, strlen(tmp));
		}
		else{
			account = account_list[i];

			//websWrite(wp, "if (account == \"%s\"){\n", account_list[i]);
			sprintf(tmp, "if (account == \"%s\"){\n", account_list[i]);
			asp_send_response(NULL, tmp, strlen(tmp));
		}

		first_pool = 1;
		for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next){
			for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next){
				if (follow_partition->mount_point != NULL && strlen(follow_partition->mount_point) > 0){
					asp_send_response(NULL, " ", 1);//websWrite(wp, "	");
					if (first_pool == 1)
						first_pool = 0;
					else
						asp_send_response(NULL, "else ", 5);//websWrite(wp, "else ");

#ifdef OLD_AIDISK
					//websWrite(wp, "if (pool == \"%s\"){\n", rindex(follow_partition->mount_point, '/')+1);
					sprintf(tmp, "if (pool == \"%s\"){\n", rindex(follow_partition->mount_point, '/')+1);
					asp_send_response(NULL, tmp, strlen(tmp));
#else
					//websWrite(wp, "if (pool == \"%s\"){\n", follow_partition->device);
					sprintf(tmp, "if (pool == \"%s\"){\n", follow_partition->device);
					asp_send_response(NULL, tmp, strlen(tmp));
#endif

					//websWrite(wp, "	    return [");
					sprintf(tmp, "\t\treturn [");
					asp_send_response(NULL, tmp, strlen(tmp));

					result = get_all_folder(follow_partition->mount_point, &sh_num, &folder_list);

					// Pool's permission.
					samba_right = get_permission(account, follow_partition->mount_point, NULL, "cifs");
					if (samba_right < 0 || samba_right > 3){
						printf("Can't get the CIFS permission abount the pool: %s!\n", follow_partition->device);

						if(account == NULL)// || !strcmp(account, nvram_safe_get("http_username")))
							samba_right = DEFAULT_SAMBA_RIGHT;
						else
							samba_right = 0;
					}

					ftp_right = get_permission(account, follow_partition->mount_point, NULL, "ftp");
					if (ftp_right < 0 || ftp_right > 3){
						printf("Can't get the FTP permission abount the pool: %s!\n", follow_partition->device);

						if(account == NULL)// || !strcmp(account, nvram_safe_get("http_username")))
							ftp_right = DEFAULT_FTP_RIGHT;
						else
							ftp_right = 0;
					}

#ifdef RTCONFIG_WEBDAV_PENDING
					webdav_right = get_permission(account, follow_partition->mount_point, NULL, "webdav");
					if (webdav_right < 0 || webdav_right > 3){
						printf("Can't get the WEBDAV  permission abount the pool: %s!\n", follow_partition->device);

						if(account == NULL || !strcmp(account, nvram_safe_get("http_username")))
							webdav_right = DEFAULT_WEBDAV_RIGHT;
						else
							webdav_right = 0;
					}
#endif

#ifdef RTCONFIG_WEBDAV_PENDING
					//websWrite(wp, "[\"\", %d, %d, %d]", samba_right, ftp_right, webdav_right);
					sprintf(tmp, "[\"\", %d, %d, %d]", samba_right, ftp_right, webdav_right);
					asp_send_response(NULL, tmp, strlen(tmp));
#else
					//websWrite(wp, "[\"\", %d, %d]", samba_right, ftp_right);
					sprintf(tmp, "[\"\", %d, %d]", samba_right, ftp_right);
					asp_send_response(NULL, tmp, strlen(tmp));
#endif
					if (result == 0 && sh_num > 0)
						asp_send_response(NULL, ",\n", 2);//websWrite(wp, ",\n");

					if (result != 0){
						// websWrite(wp, "];\n");
						// websWrite(wp, "	}\n");
						sprintf(tmp, "];\n\t}\n");
						asp_send_response(NULL, tmp, strlen(tmp));

						printf("get_permissions_of_account1: Can't get all folders in \"%s\".\n", follow_partition->mount_point);

						free_2_dimension_list(&sh_num, &folder_list);

						continue;
					}

					// Folder's permission.
					for (j = 0; j < sh_num; ++j){
						samba_right = get_permission(account, follow_partition->mount_point, folder_list[j], "cifs");
						ftp_right = get_permission(account, follow_partition->mount_point, folder_list[j], "ftp");
#ifdef RTCONFIG_WEBDAV_PENDING
						webdav_right = get_permission(account, follow_partition->mount_point, folder_list[j], "webdav");
#endif

						if (samba_right < 0 || samba_right > 3){
							printf("Can't get the CIFS permission abount \"%s\"!\n", folder_list[j]);

							if(account == NULL)// || !strcmp(account, nvram_safe_get("http_username")))
								samba_right = DEFAULT_SAMBA_RIGHT;
							else
								samba_right = 0;
						}

						if (ftp_right < 0 || ftp_right > 3){
							printf("Can't get the FTP permission abount \"%s\"!\n", folder_list[j]);

							if(account == NULL)// || !strcmp(account, nvram_safe_get("http_username")))
								ftp_right = DEFAULT_FTP_RIGHT;
							else
								ftp_right = 0;
						}

#ifdef RTCONFIG_WEBDAV_PENDING
						if (webdav_right < 0 || webdav_right > 3){
							printf("Can't get the WEBDAV permission abount \"%s\"!\n", folder_list[j]);

							if(account == NULL || !strcmp(account, nvram_safe_get("http_username")))
								webdav_right = DEFAULT_WEBDAV_RIGHT;
							else
								webdav_right = 0;
						}
#endif

#ifdef RTCONFIG_WEBDAV_PENDING
						//websWrite(wp, "		    [\"%s\", %d, %d, %d]", folder_list[j], samba_right, ftp_right, webdav_right);
						sprintf(tmp, "\t\t\t[\"%s\", %d, %d, %d]", folder_list[j], samba_right, ftp_right, webdav_right);
						asp_send_response(NULL, tmp, strlen(tmp));
#else
						//websWrite(wp, "		    [\"%s\", %d, %d]", folder_list[j], samba_right, ftp_right);
						sprintf(tmp, "\t\t\t[\"%s\", %d, %d]", folder_list[j], samba_right, ftp_right);
						asp_send_response(NULL, tmp, strlen(tmp));
#endif

						if (j != sh_num-1)
							asp_send_response(NULL, ",\n", 2);//websWrite(wp, ",\n");
					}
					free_2_dimension_list(&sh_num, &folder_list);

					// websWrite(wp, "];\n");
					// websWrite(wp, "	}\n");
					sprintf(tmp, "];\n\t}\n");
					asp_send_response(NULL, tmp, strlen(tmp));
				}
			}
		}

		asp_send_response(NULL, "\t}\n", 3);//websWrite(wp, "    }\n");

	}
	free_2_dimension_list(&acc_num, &account_list);

	asp_send_response(NULL, "}\n\n", 3);//websWrite(wp, "}\n\n");

	if (disks_info != NULL)
		free_disk_data(&disks_info);

	return;// 0;
}

static void
set_AiDisk_status (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *protocol = get_param(g_var_post, "protocol");//websGetVar(wp, "protocol", "");
	char *flag = get_param(g_var_post, "flag");//websGetVar(wp, "flag", "");
	int result = 0;
	char tmp[1024];

	if (strlen(protocol) <= 0){
		show_error_msg("Input1");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_AiDisk_status_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_AiDisk_status_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (strlen(flag) <= 0){
		show_error_msg("Input18");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_AiDisk_status_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_AiDisk_status_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (!strcmp(protocol, "cifs")){
		if (!strcmp(flag, "on")){
			tcapi_set("Samba_Entry", "Active", "Yes");// nvram_set("enable_samba", "1");
			tcapi_commit("Samba_Entry");// nvram_commit();
			// result = notify_rc_for_nas("restart_samba");
			eval("/sbin/rc_call", "samba", "2");
		}
		else if (!strcmp(flag, "off")){
			tcapi_set("Samba_Entry", "Active", "No");// nvram_set("enable_samba", "0");
			tcapi_commit("Samba_Entry");// nvram_commit();
			if (!pids("smbd"))
				goto SET_AIDISK_STATUS_SUCCESS;

			// result = notify_rc_for_nas("stop_samba");
			eval("/sbin/rc_call", "samba", "0");
		}
		else{
			show_error_msg("Input19");

			// websWrite(wp, "<script>\n");
			// websWrite(wp, "set_AiDisk_status_error(\'%s\');\n", error_msg);
			// websWrite(wp, "</script>\n");
			sprintf(tmp, "<script>\nset_AiDisk_status_error(\'%s\');\n</script>\n", error_msg);
			asp_send_response(NULL, tmp, strlen(tmp));

			clean_error_msg();
			return;// -1;
		}
	}
	else if (!strcmp(protocol, "ftp")){
		if (!strcmp(flag, "on")){
			tcapi_set("Samba_Entry", "enable_ftp", "Yes");// nvram_set("enable_ftp", "1");
			tcapi_commit("Samba_Entry");// nvram_commit();
			//result = notify_rc_for_nas("restart_ftpd");
			eval("/sbin/rc_call", "ftp", "2");
		}
		else if (!strcmp(flag, "off")){
			tcapi_set("Samba_Entry", "enable_ftp", "No");// nvram_set("enable_ftp", "0");
			tcapi_commit("Samba_Entry");// nvram_commit();
			if (!pids("vsftpd"))
				goto SET_AIDISK_STATUS_SUCCESS;

			//result = notify_rc_for_nas("stop_ftpd");
			eval("/sbin/rc_call", "ftp", "0");
		}
		else{
			show_error_msg("Input19");

			// websWrite(wp, "<script>\n");
			// websWrite(wp, "set_AiDisk_status_error(\'%s\');\n", error_msg);
			// websWrite(wp, "</script>\n");
			sprintf(tmp, "<script>\nset_AiDisk_status_error(\'%s\');\n</script>\n", error_msg);
			asp_send_response(NULL, tmp, strlen(tmp));

			clean_error_msg();
			return;// -1;
		}
	}
#ifdef RTCONFIG_WEBDAV_PENDING
	else if (!strcmp(protocol, "webdav")){
		if (!strcmp(flag, "on")){
			nvram_set("enable_webdav", "1");
			nvram_commit();
			result = notify_rc_for_nas("restart_webdav");
		}
		else if (!strcmp(flag, "off")){
			nvram_set("enable_webdav", "0");
			nvram_commit();
			if (!pids("vsftpd"))
				goto SET_AIDISK_STATUS_SUCCESS;

			result = notify_rc_for_nas("stop_webdav");
		}
		else{
			show_error_msg("Input19");

			websWrite(wp, "<script>\n");
			websWrite(wp, "set_AiDisk_status_error(\'%s\');\n", error_msg);
			websWrite(wp, "</script>\n");

			clean_error_msg();
			return;// -1;
		}
	}
#endif
	else{
		show_error_msg("Input2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_AiDisk_status_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_AiDisk_status_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	if (result != 0){
		show_error_msg("Action8");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_AiDisk_status_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_AiDisk_status_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

SET_AIDISK_STATUS_SUCCESS:
	// websWrite(wp, "<script>\n");
	//websWrite(wp, "set_AiDisk_status_success();\n");
	// websWrite(wp, "parent.resultOfSwitchAppStatus();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\nparent.resultOfSwitchAppStatus();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	tcapi_save();

	return;// 0;
}

/*******************************************************************
* NAME: _check_vars
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/06/25
* DESCRIPTION: check the var files in all disks
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void
_check_vars () {
	disk_info_t *disks_info, *follow_disk;
	partition_info_t *follow_partition;

	disks_info = read_disk_data();
	if (disks_info == NULL){
		return ;
	}

	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next)
			if (follow_partition->mount_point != NULL && strlen(follow_partition->mount_point) > 0){
				test_of_var_files(follow_partition->mount_point);
			}

	free_disk_data(&disks_info);

	return ;
}

static void
set_share_mode (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char tmp[4096] = {0};
	tcapi_get("Samba_Entry", "st_samba_mode", tmp);
	int samba_mode = atoi(tmp);
	tcapi_get("Samba_Entry", "st_ftp_mode", tmp);
	int ftp_mode = atoi(tmp);
#ifdef RTCONFIG_WEBDAV_PENDING
	tcapi_get("AiCloud_Entry", "st_webdav_mode", tmp);
	int webdav_mode = atoi(tmp);
#endif
	struct json_object *root=NULL;
	FILE *fp;
	fp = fopen("/tmp/boa-temp", "r");
	if(fp) {
		fgets(tmp, 4096, fp);
		fclose(fp);
	}
	decode_json_buffer(tmp);
	root = json_tokener_parse(tmp);
	char *dummyShareway = get_cgi_json(g_var_post, "dummyShareway", root);//websGetVar(wp, "dummyShareway", "");
	char *protocol = get_cgi_json(g_var_post, "protocol", root);//websGetVar(wp, "protocol", "");
	char *mode = get_cgi_json(g_var_post, "mode", root);//websGetVar(wp, "mode", "");
	// int result;

	if (dummyShareway && strlen(dummyShareway) > 0)
		tcapi_set("Samba_Entry", "dummyShareway", dummyShareway);//nvram_set("dummyShareway", dummyShareway);
	else
		tcapi_set("Samba_Entry", "dummyShareway", "0");//nvram_set("dummyShareway", "0");

	if (strlen(protocol) <= 0){
		show_error_msg("Input1");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_share_mode_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_share_mode_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		json_object_put(root);
		return;// -1;
	}
	if (strlen(mode) <= 0){
		show_error_msg("Input3");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_share_mode_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_share_mode_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		json_object_put(root);
		return;// -1;
	}
	if (!strcmp(mode, "share")){
		if (!strcmp(protocol, "cifs")){
			if (samba_mode == 1 || samba_mode == 3)
				goto SET_SHARE_MODE_SUCCESS;

			tcapi_set("Samba_Entry", "st_samba_mode", "1");//nvram_set("st_samba_mode", "1");	// for test
			tcapi_set("Samba_Entry", "st_samba_force_mode", "2");
		}
		else if (!strcmp(protocol, "ftp")){
			if (ftp_mode == 1)
				goto SET_SHARE_MODE_SUCCESS;

			tcapi_set("Samba_Entry", "st_ftp_mode", "1");//nvram_set("st_ftp_mode", "1");
			tcapi_set("Samba_Entry", "st_ftp_force_mode", "2");
		}
#ifdef RTCONFIG_WEBDAV_PENDING
		else if (!strcmp(protocol, "webdav")){
			if (webdav_mode == 1)
				goto SET_SHARE_MODE_SUCCESS;

			tcapi_set("AiCloud_Entry", "st_webdav_mode", "1");//nvram_set("st_webdav_mode", "1");
		}
#endif
		else{
			show_error_msg("Input2");

			// websWrite(wp, "<script>\n");
			// websWrite(wp, "set_share_mode_error(\'%s\');\n", error_msg);
			// websWrite(wp, "</script>\n");
			sprintf(tmp, "<script>\nset_share_mode_error(\'%s\');\n</script>\n", error_msg);
			asp_send_response(NULL, tmp, strlen(tmp));

			clean_error_msg();
			json_object_put(root);
			return;// -1;
		}
	}
	else if (!strcmp(mode, "account")){
		if (!strcmp(protocol, "cifs")){
			if (samba_mode == 2 || samba_mode == 4)
				goto SET_SHARE_MODE_SUCCESS;

			tcapi_set("Samba_Entry", "st_samba_mode", "4");//nvram_set("st_samba_mode", "4");
			tcapi_set("Samba_Entry", "st_samba_force_mode", "2");
		}
		else if (!strcmp(protocol, "ftp")){
			if (ftp_mode == 2)
				goto SET_SHARE_MODE_SUCCESS;

			tcapi_set("Samba_Entry", "st_ftp_mode", "2");//nvram_set("st_ftp_mode", "2");
			tcapi_set("Samba_Entry", "st_ftp_force_mode", "2");
		}
#ifdef RTCONFIG_WEBDAV_PENDING
		else if (!strcmp(protocol, "webdav")){
			if (webdav_mode == 2)
				goto SET_SHARE_MODE_SUCCESS;

			tcapi_set("Samba_Entry", "st_webdav_mode", "2");//nvram_set("st_webdav_mode", "2");
		}
#endif
		else {
			show_error_msg("Input2");

			// websWrite(wp, "<script>\n");
			// websWrite(wp, "set_share_mode_error(\'%s\');\n", error_msg);
			// websWrite(wp, "</script>\n");
			sprintf(tmp, "<script>\nset_share_mode_error(\'%s\');\n</script>\n", error_msg);
			asp_send_response(NULL, tmp, strlen(tmp));

			clean_error_msg();
			json_object_put(root);
			return;// -1;
		}
	}
	else{
		show_error_msg("Input4");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_share_mode_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_share_mode_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		json_object_put(root);
		return;// -1;
	}

	//Andy Chiu, 2015/06/25
	_check_vars();

	tcapi_commit("Samba_Entry");//nvram_commit();
	tcapi_save();

	not_ej_initial_folder_var_file();	// J++

	if (!strcmp(protocol, "cifs")) {
		//result = notify_rc_for_nas("restart_samba");
		eval("/sbin/rc_call", "samba", "2");
	}
	else if (!strcmp(protocol, "ftp")) {
		//result = notify_rc_for_nas("restart_ftpd");
		eval("/sbin/rc_call", "ftp", "2");
	}
#ifdef RTCONFIG_WEBDAV_PENDING
	else if (!strcmp(protocol, "webdav")) {
		//result = notify_rc_for_nas("restart_webdav");
		eval("/sbin/rc_call", "webdav", "2");
	}
#endif
	else {
		show_error_msg("Input2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_share_mode_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_share_mode_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		json_object_put(root);
		return;// -1;
	}

	// if (result != 0){
		// show_error_msg("Action8");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_share_mode_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		// sprintf(tmp, "<script>\nset_share_mode_error(\'%s\');\n</script>\n", error_msg);
		// asp_send_response(NULL, tmp, strlen(tmp));

		// clean_error_msg();
		// return;// -1;
	// }

SET_SHARE_MODE_SUCCESS:
	// websWrite(wp, "<script>\n");
	// websWrite(wp, "set_share_mode_success();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\nset_share_mode_success();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	json_object_put(root);
	return;// 0;
}

void not_ej_initial_folder_var_file()						// J++
{
	disk_info_t *disks_info, *follow_disk;
	partition_info_t *follow_partition;

	disks_info = read_disk_data();
	if (disks_info == NULL)
		return;

	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next)
			if (follow_partition->mount_point != NULL && strlen(follow_partition->mount_point) > 0) {
				initial_folder_list(follow_partition->mount_point);
//				initial_all_var_file(follow_partition->mount_point);
			}

	free_disk_data(&disks_info);
}

#ifdef RTCONFIG_USB
static void
get_usb_accounts (asp_reent* reent, const asp_text* params, asp_text* ret) {
	int acc_num;
	char **account_list = NULL;
	int result, i, first;
	char tmp[256];

	if ((result = get_account_list(&acc_num, &account_list)) < 0){
		printf("Failed to get the account list!\n");
		return;// -1;
	}

	first = 1;
	for (i = 0; i < acc_num; ++i){
		if (first == 1)
			first = 0;
		else
			asp_send_response(NULL, ", ", 2);//websWrite(wp, ", ");

		//websWrite(wp, "\"%s\"", account_list[i]);
		sprintf(tmp, "\"%s\"", account_list[i]);
		asp_send_response(NULL, tmp, strlen(tmp));
	}

	free_2_dimension_list(&acc_num, &account_list);

	return;// 0;
}
#endif

static void create_account (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *account = get_param(g_var_post, "account");//websGetVar(wp, "account", "");
	char *password = get_param(g_var_post, "password");//websGetVar(wp, "password", "");
	char tmp[1024];

	if (strlen(account) <= 0){
		show_error_msg("Input5");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ncreate_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (strlen(password) <= 0){
		show_error_msg("Input14");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ncreate_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	not_ej_initial_folder_var_file();	// J++

	if (add_account(account, password) < 0){
		show_error_msg("Action2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ncreate_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#ifdef RTCONFIG_WEBDAV_PENDING
	else if(add_webdav_account(account) < 0) {
		show_error_msg("Action2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ncreate_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#endif

	// if (notify_rc_for_nas("restart_ftpsamba") != 0){
		// show_error_msg("Action2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }

#ifdef RTCONFIG_WEBDAV_PENDING
	// if(notify_rc_for_nas("restart_webdav") != 0) {
		// show_error_msg("Action2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "create_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }
#endif

	// websWrite(wp, "<script>\n");
	// websWrite(wp, "create_account_success();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\ncreate_account_success();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));
	return;// 0;
}

static void delete_account (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *account = get_param(g_var_post, "account");//websGetVar(wp, "account", "");
	char tmp[1024];

	if (strlen(account) <= 0){
		show_error_msg("Input5");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ndelete_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	not_ej_initial_folder_var_file();	// J++

	if (del_account(account) < 0){
		show_error_msg("Action3");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ndelete_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#ifdef RTCONFIG_WEBDAV_PENDING
	else if(del_webdav_account(account)<0) {
		show_error_msg("Action3");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ndelete_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#endif

	// if (notify_rc_for_nas("restart_ftpsamba") != 0){
		// show_error_msg("Action3");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }

#ifdef RTCONFIG_WEBDAV_PENDING
	// if(notify_rc_for_nas("restart_webdav") != 0) {
		// show_error_msg("Action3");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "delete_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }
#endif

	// websWrite(wp, "<script>\n");
	// websWrite(wp, "delete_account_success();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\ndelete_account_success();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	return;// 0;
}

static void modify_account (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *account = get_param(g_var_post, "account");//websGetVar(wp, "account", "");
	char *new_account = get_param(g_var_post, "new_account");//websGetVar(wp, "new_account", "");
	char *new_password = get_param(g_var_post, "new_password");//websGetVar(wp, "new_password", "");
	char tmp[1024];

	if (strlen(account) <= 0){
		show_error_msg("Input5");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nmodify_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (strlen(new_account) <= 0 && strlen(new_password) <= 0){
		show_error_msg("Input16");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nmodify_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	if (mod_account(account, new_account, new_password) < 0){
		show_error_msg("Action4");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nmodify_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#ifdef RTCONFIG_WEBDAV_PENDING
	else if(mod_webdav_account(account, new_account)<0) {
		show_error_msg("Action4");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nmodify_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#endif

	// if (notify_rc_for_nas("restart_ftpsamba") != 0){
		// show_error_msg("Action4");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "modify_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }

#ifdef RTCONFIG_WEBDAV_PENDING
	// if(notify_rc_for_nas("restart_webdav") != 0) {
		// show_error_msg("Action4");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "mod_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }
#endif

	// websWrite(wp, "<script>\n");
	// websWrite(wp, "modify_account_success();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\nmodify_account_success();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	return;// 0;
}

static void
initial_account (asp_reent* reent, const asp_text* params, asp_text* ret) {
	disk_info_t *disks_info, *follow_disk;
	partition_info_t *follow_partition;
	char *command;
	int len, result;
	char tmp[1024];
	char sysuser[64] = {0};
	char syspwd[64] = {0};

	disks_info = read_disk_data();
	if (disks_info == NULL){
		show_error_msg("System2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "initial_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ninitial_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;
	}

	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next)
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next)
			if (follow_partition->mount_point != NULL && strlen(follow_partition->mount_point) > 0){
				len = strlen("rm -f ")+strlen(follow_partition->mount_point)+strlen("/.__*");
				command = (char *)malloc(sizeof(char)*(len+1));
				if (command == NULL){
					show_error_msg("System1");

					// websWrite(wp, "<script>\n");
					// websWrite(wp, "initial_account_error(\'%s\');\n", error_msg);
					// websWrite(wp, "</script>\n");
					sprintf(tmp, "<script>\ninitial_account_error(\'%s\');\n</script>\n", error_msg);
					asp_send_response(NULL, tmp, strlen(tmp));

					clean_error_msg();
					return;
				}
				sprintf(command, "rm -f %s/.__*", follow_partition->mount_point);
				command[len] = 0;

				result = system(command);
				free(command);

				initial_folder_list(follow_partition->mount_point);
				initial_all_var_file(follow_partition->mount_point);
			}

	free_disk_data(&disks_info);

	if( (tcapi_get("Account_Entry0", "username", sysuser) == 0) && (tcapi_get("Account_Entry0", "web_passwd", syspwd) == 0) )
		{
			tcapi_set("Samba_Entry", "acc_num", "0");
			tcapi_set("Samba_Entry", "acc_list", "");
		result = add_account(sysuser, syspwd);
	}
	else
	{
		result = -1;
	}

	if ( result < 0){
		show_error_msg("Action2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "initial_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ninitial_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;
	}
#ifdef RTCONFIG_WEBDAV_PENDING
	else if(add_webdav_account(nvram_safe_get("http_username"))<0) {
		show_error_msg("Action2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "init_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\ninitial_account_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;
	}
#endif

	// if (notify_rc_for_nas("restart_ftpsamba") != 0){
		// show_error_msg("Action2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "initial_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }

#ifdef RTCONFIG_WEBDAV_PENDING
	// if(notify_rc_for_nas("restart_webdav") != 0) {
		// show_error_msg("Action2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "initial_account_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }
#endif
	// websWrite(wp, "<script>\n");
	// websWrite(wp, "initial_account_success();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\ninitial_account_success();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	return;
}

static void
get_share_tree(asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *layer_order = get_param(g_var_post, "layer_order");//websGetVar(wp, "layer_order", "");
	char *follow_info, *follow_info_end, backup;
	int layer = 0, first;
	int disk_count, partition_count, share_count;
	int disk_order = -1, partition_order = -1;
	disk_info_t *disks_info, *follow_disk;
	partition_info_t *follow_partition;
	char tmp[1024];

	if (strlen(layer_order) <= 0){
		printf("No input \"layer_order\"!\n");
		return;// -1;
	}

	follow_info = index(layer_order, '_');
	while (follow_info != NULL && *follow_info != 0){
		++layer;
		++follow_info;
		if (*follow_info == 0)
			break;
		follow_info_end = follow_info;
		while (*follow_info_end != 0 && isdigit(*follow_info_end))
			++follow_info_end;
		backup = *follow_info_end;
		*follow_info_end = 0;

		if (layer == 1)
			disk_order = atoi(follow_info);
		else if (layer == 2)
			partition_order = atoi(follow_info);
		else{
			*follow_info_end = backup;
			printf("Input \"%s\" is incorrect!\n", layer_order);
			return;// -1;
		}

		*follow_info_end = backup;
		follow_info = follow_info_end;
	}

	disks_info = read_disk_data();
	if (disks_info == NULL){
		printf("Can't read the information of disks.\n");
		return;// -1;
	}

	first = 1;
	disk_count = 0;
	for (follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next, ++disk_count){
		partition_count = 0;
		for (follow_partition = follow_disk->partitions; follow_partition != NULL; follow_partition = follow_partition->next, ++partition_count){
			if (layer != 0 && follow_partition->mount_point != NULL && strlen(follow_partition->mount_point) > 0){
				int i;
				char **share_list;
				int result;
				result = get_folder_list(follow_partition->mount_point, &share_count, &share_list);
				if (result < 0){
					printf("get_disk_tree: Can't get the share list in \"%s\".\n", follow_partition->mount_point);

					share_count = 0;
				}

				if (layer == 2 && disk_count == disk_order && partition_count == partition_order){
					for (i = 0; i < share_count; ++i){
						if (first == 1)
							first = 0;
						else
							asp_send_response(NULL, ", ", 2);//websWrite(wp, ", ");

						//websWrite(wp, "\"%s#%u#0\"", share_list[i], i);
						sprintf(tmp, "\"%s#%u#0\"", share_list[i], i);
						asp_send_response(NULL, tmp, strlen(tmp));
					}
				}
				else if (layer == 1 && disk_count == disk_order){
					if (first == 1)
						first = 0;
					else
						asp_send_response(NULL, ", ", 2);//websWrite(wp, ", ");

					follow_info = rindex(follow_partition->mount_point, '/');
					//websWrite(wp, "\"%s#%u#%u\"", follow_info+1, partition_count, share_count);
					sprintf(tmp, "\"%s#%u#%u\"", follow_info+1, partition_count, share_count);
					asp_send_response(NULL, tmp, strlen(tmp));
				}

				free_2_dimension_list(&share_count, &share_list);
			}
		}
		if (layer == 0){
			if (first == 1)
				first = 0;
			else
				asp_send_response(NULL, ", ", 2);//websWrite(wp, ", ");

			//websWrite(wp, "\"%s#%u#%u\"", follow_disk->tag, disk_count, partition_count);
			sprintf(tmp, "\"%s#%u#%u\"", follow_disk->tag, disk_count, partition_count);
			asp_send_response(NULL, tmp, strlen(tmp));
		}

		if (layer > 0 && disk_count == disk_order)
			break;
	}

	free_disk_data(&disks_info);

	return;// 0;
}

static void
set_account_permission (asp_reent* reent, const asp_text* params, asp_text* ret) {
	char mount_path[PATH_MAX];
	char *account = get_param(g_var_post, "account");//websGetVar(wp, "account", NULL);
	char *pool = get_param(g_var_post, "pool");//websGetVar(wp, "pool", "");
	char *folder = get_param(g_var_post, "folder");//websGetVar(wp, "folder", NULL);
	char *protocol = get_param(g_var_post, "protocol");//websGetVar(wp, "protocol", "");
	char *permission = get_param(g_var_post, "permission");//websGetVar(wp, "permission", "");
#ifdef RTCONFIG_WEBDAV_PENDING
	char *webdavproxy = get_param(g_var_post, "acc_webdavproxy");//websGetVar(wp, "acc_webdavproxy", "");
#endif
	int right;
	char tmp[1024];

	//Carlos 2015/07/31, Add UTF-8 processor
	char buf[1024];
	sprintf(buf, "%s", folder);
	if(decode(buf, folder) > 0) {
		con_dbgprintf("[%s, %d]folder:%s\n", __FUNCTION__, __LINE__, folder);
	}
	
	if (test_if_exist_account(account) != 1){
		show_error_msg("Input6");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_account_permission_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	if (strlen(pool) <= 0){
		show_error_msg("Input7");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_account_permission_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	if (get_mount_path(pool, mount_path, PATH_MAX) < 0){
		fprintf(stderr, "Can't get the mount_path of %s.\n", pool);

		show_error_msg("System1");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_account_permission_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	if (strlen(protocol) <= 0){
		show_error_msg("Input1");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_account_permission_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	if (strcmp(protocol, "cifs") && strcmp(protocol, "ftp") && strcmp(protocol, "dms")
#ifdef RTCONFIG_WEBDAV_PENDING
&& strcmp(protocol, "webdav")
#endif
){
		show_error_msg("Input2");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_account_permission_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	if (strlen(permission) <= 0){
		show_error_msg("Input12");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_account_permission_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
	right = atoi(permission);
	if (right < 0 || right > 3){
		show_error_msg("Input13");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_account_permission_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}

	if (set_permission(account, mount_path, folder, protocol, right) < 0){
		show_error_msg("Action1");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");
		sprintf(tmp, "<script>\nset_account_permission_error(\'%s\');\n</script>\n", error_msg);
		asp_send_response(NULL, tmp, strlen(tmp));

		clean_error_msg();
		return;// -1;
	}
#ifdef RTCONFIG_WEBDAV_PENDING
	else {
		logmessage("wedavproxy right", "%s %s %s %s %d %s", account, mount_path, folder, protocol, right, webdavproxy);
		// modify permission for webdav proxy
		nvram_set("acc_webdavproxy", webdavproxy);
	}
#endif

#ifdef RTCONFIG_WEBDAV_PENDING
	if(strcmp(protocol, "webdav")==0) {
		if(notify_rc_for_nas("restart_webdav") != 0) {
			show_error_msg("Action1");

			// websWrite(wp, "<script>\n");
			// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
			// websWrite(wp, "</script>\n");
			sprintf(tmp, "<script>\nset_account_permission_error(\'%s\');\n</script>\n", error_msg);
			asp_send_response(NULL, tmp, strlen(tmp));

			clean_error_msg();
			return;// -1;
		}
	}
	else
#endif
	// if (notify_rc_for_nas("restart_ftpsamba") != 0){
		// show_error_msg("Action1");

		// websWrite(wp, "<script>\n");
		// websWrite(wp, "set_account_permission_error(\'%s\');\n", error_msg);
		// websWrite(wp, "</script>\n");

		// clean_error_msg();
		// return -1;
	// }
	// tcapi_commit("Samba_Entry");	//trigger ftp/samba restart
	eval("/sbin/rc_call", "ftpsamba", "2");

	// websWrite(wp, "<script>\n");
	// websWrite(wp, "set_account_permission_success();\n");
	// websWrite(wp, "</script>\n");
	sprintf(tmp, "<script>\nset_account_permission_success();\n</script>\n");
	asp_send_response(NULL, tmp, strlen(tmp));

	return;// 0;
}

#define MAX_PRINTER_NUM 2
static void
get_printer_info (asp_reent* reent, const asp_text* params, asp_text* ret) {
	int port_num = 0, first;
	char tmp[64], prefix[] = "usb_pathX";
	char printer_array[2][5][64];
	char wp[1024], value[64];

	memset(tmp, 0, sizeof(tmp));
	memset(value, 0, sizeof(value));
	for(port_num = 1; port_num <= MAX_PRINTER_NUM; ++port_num){
		snprintf(prefix, sizeof(prefix), "usb_path%d", port_num);

		memset(printer_array[port_num-1][0], 0, 64);
		memset(printer_array[port_num-1][1], 0, 64);
		memset(printer_array[port_num-1][2], 0, 64);
		memset(printer_array[port_num-1][3], 0, 64);
		memset(printer_array[port_num-1][4], 0, 64);

		tcapi_get("USB_Entry", prefix, value);
		if(!strcmp(value, "printer")){
		// if(nvram_match(prefix, "printer")){
			strncpy(printer_array[port_num-1][0], "printer", 64);
			// strncpy(printer_array[port_num-1][1], nvram_safe_get(strcat_r(prefix, "_manufacturer", tmp)), 64);
			// strncpy(printer_array[port_num-1][2], nvram_safe_get(strcat_r(prefix, "_product", tmp)), 64);
			// strncpy(printer_array[port_num-1][3], nvram_safe_get(strcat_r(prefix, "_serial", tmp)), 64);
			tcapi_get("USB_Entry", strcat_r(prefix, "_manufacturer", tmp), value);
			strncpy(printer_array[port_num-1][1], value, 64);
			tcapi_get("USB_Entry", strcat_r(prefix, "_product", tmp), value);
			strncpy(printer_array[port_num-1][2], value, 64);
			tcapi_get("USB_Entry", strcat_r(prefix, "_serial", tmp), value);
			strncpy(printer_array[port_num-1][3], value, 64);

			tcapi_get("USB_Entry", "u2ec_serial", value);
			if(!strcmp(printer_array[port_num-1][3], value))
			// if(!strcmp(printer_array[port_num-1][3], nvram_safe_get("u2ec_serial")))
				strcpy(printer_array[port_num-1][4], "VirtualPool");
			else
				strcpy(printer_array[port_num-1][4], "");
		}
	}

	// websWrite(wp, "function printer_manufacturers(){\n");
	// websWrite(wp, "    return [");
	sprintf(wp, "function printer_manufacturers(){\n\treturn [");
	asp_send_response(NULL, wp, strlen(wp));

	first = 1;
	for(port_num = 1; port_num <= MAX_PRINTER_NUM; ++port_num){
		if(strlen(printer_array[port_num-1][0]) > 0) {
			// websWrite(wp, "\"%s\"", printer_array[port_num-1][1]);
			sprintf(wp, "\"%s\"", printer_array[port_num-1][1]);
			asp_send_response(NULL, wp, strlen(wp));
		}
		else {
			// websWrite(wp, "\"\"");
			sprintf(wp, "\"\"");
			asp_send_response(NULL, wp, strlen(wp));
		}
		if(first){
			--first;
			// websWrite(wp, ", ");
			sprintf(wp, ", ");
			asp_send_response(NULL, wp, strlen(wp));
		}
	}

	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(wp, "];\n}\n\n");
	asp_send_response(NULL, wp, strlen(wp));

	// websWrite(wp, "function printer_models(){\n");
	// websWrite(wp, "    return [");
	sprintf(wp, "function printer_models(){\n\treturn [");
	asp_send_response(NULL, wp, strlen(wp));

	first = 1;
	for(port_num = 1; port_num <= MAX_PRINTER_NUM; ++port_num){
		if(strlen(printer_array[port_num-1][0]) > 0) {
			// websWrite(wp, "\"%s\"", printer_array[port_num-1][2]);
			sprintf(wp, "\"%s\"", printer_array[port_num-1][2]);
			asp_send_response(NULL, wp, strlen(wp));
		}
		else {
			// websWrite(wp, "\"\"");
			sprintf(wp, "\"\"");
			asp_send_response(NULL, wp, strlen(wp));
		}
		if(first){
			--first;
			// websWrite(wp, ", ");
			sprintf(wp, ", ");
			asp_send_response(NULL, wp, strlen(wp));
		}
	}

	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(wp, "];\n}\n\n");
	asp_send_response(NULL, wp, strlen(wp));

	// websWrite(wp, "function printer_serialn(){\n");
	// websWrite(wp, "    return [");
	sprintf(wp, "function printer_serialn(){\n\treturn [");
	asp_send_response(NULL, wp, strlen(wp));

	first = 1;
	for(port_num = 1; port_num <= MAX_PRINTER_NUM; ++port_num){
		if(strlen(printer_array[port_num-1][0]) > 0) {
			// websWrite(wp, "\"%s\"", printer_array[port_num-1][3]);
			sprintf(wp, "\"%s\"", printer_array[port_num-1][3]);
			asp_send_response(NULL, wp, strlen(wp));
		}
		else {
			// websWrite(wp, "\"\"");
			sprintf(wp, "\"\"");
			asp_send_response(NULL, wp, strlen(wp));
		}
		if(first){
			--first;
			// websWrite(wp, ", ");
			sprintf(wp, ", ");
			asp_send_response(NULL, wp, strlen(wp));
		}
	}

	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(wp, "];\n}\n\n");
	asp_send_response(NULL, wp, strlen(wp));

	// websWrite(wp, "function printer_pool(){\n");
	// websWrite(wp, "    return [");
	sprintf(wp, "function printer_pool(){\n\treturn [");
	asp_send_response(NULL, wp, strlen(wp));

	first = 1;
	for(port_num = 1; port_num <= MAX_PRINTER_NUM; ++port_num){
		if(strlen(printer_array[port_num-1][0]) > 0) {
			// websWrite(wp, "\"%s\"", printer_array[port_num-1][4]);
			sprintf(wp, "\"%s\"", printer_array[port_num-1][4]);
			asp_send_response(NULL, wp, strlen(wp));
		}
		else {
			// websWrite(wp, "\"\"");
			sprintf(wp, "\"\"");
			asp_send_response(NULL, wp, strlen(wp));
		}
		if(first){
			--first;
			// websWrite(wp, ", ");
			sprintf(wp, ", ");
			asp_send_response(NULL, wp, strlen(wp));
		}
	}

	// websWrite(wp, "];\n");
	// websWrite(wp, "}\n\n");
	sprintf(wp, "];\n}\n\n");
	asp_send_response(NULL, wp, strlen(wp));

	// return 0;
}


int
count_sddev_mountpoint()
{
	FILE *procpt;
	char line[256], devname[32], mpname[32], system_type[10], mount_mode[96];
	int dummy1, dummy2, count = 0;

	memset(line, 0, sizeof(line));
	memset(devname, 0, sizeof(devname));
	memset(mpname, 0, sizeof(mpname));
	memset(system_type, 0, sizeof(system_type));
	memset(mount_mode, 0, sizeof(mount_mode));
	if ((procpt = fopen("/proc/mounts", "r")) != NULL)
	while (fgets(line, sizeof(line), procpt))
	{
		if (sscanf(line, "%s %s %s %s %d %d", devname, mpname, system_type, mount_mode, &dummy1, &dummy2) != 6)
			continue;

		if (strstr(devname, "/dev/sd"))
			count++;
	}

	if (procpt)
		fclose(procpt);

	return count;
}

static void safely_remove_disk (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	int result;
	char *disk_port = get_param(g_var_post, "disk");//websGetVar(wp, "disk", "");
//	disk_info_t *disks_info = NULL, *follow_disk = NULL;
//	int disk_num = 0;
	int part_num = 0;
	char wp[256] = {0};

	csprintf("disk_port = %s\n", disk_port);

	result = eval("/sbin/ejusb", disk_port, "0");

	if (result != 0){
		show_error_msg("Action9");

		websWrite(wp, "<script>\n");
		websWrite(wp, "safely_remove_disk_error(\'%s\');\n", error_msg);
		websWrite(wp, "</script>\n");
		clean_error_msg();
		return;// -1;
	}

/*
	disks_info = read_disk_data();
	for(follow_disk = disks_info; follow_disk != NULL; follow_disk = follow_disk->next, ++disk_num)
		;
	free_disk_data(&disks_info);
	csprintf("disk_num = %d\n", disk_num);
*/

	part_num = count_sddev_mountpoint();
	csprintf("part_num = %d\n", part_num);

//	if (disk_num > 1)
	if (part_num > 0)
	{
		// result = eval("/sbin/check_proc_mounts_parts");
		// result = notify_rc_for_nas("restart_nasapps");
		eval("/sbin/rc_call", "nasapps", "2");
	}
	else
	{
		// result = notify_rc_for_nas("stop_nasapps");
		eval("/sbin/rc_call", "nasapps", "0");
	}

	websWrite(wp, "<script>\n");
	websWrite(wp, "safely_remove_disk_success(\'%s\');\n", error_msg);
	websWrite(wp, "</script>\n");

	return;// 0;
}

#ifdef RTCONFIG_CLOUDSYNC
static char *convert_cloudsync_status(const char *status_code){
	if(!strcmp(status_code, "STATUS:70"))
		return "INITIAL";
	else if(!strcmp(status_code, "STATUS:71"))
		return "SYNC";
	else if(!strcmp(status_code, "STATUS:72"))
		return "DOWNUP";
	else if(!strcmp(status_code, "STATUS:73"))
		return "UPLOAD";
	else if(!strcmp(status_code, "STATUS:74"))
		return "DOWNLOAD";
	else if(!strcmp(status_code, "STATUS:75"))
		return "STOP";
	else
		return "ERROR";
}
static void cloud_status (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE *fp = fopen("/tmp/smartsync/.logs/asuswebstorage", "r");
	char line[PATH_MAX], buf[PATH_MAX];
	int line_num;
	char status[16], mounted_path[PATH_MAX], target_obj[PATH_MAX], error_msg[PATH_MAX];
	char wp[256] = {0};

	// websWrite(wp, "cloud_sync=\"%s\";\n", nvram_safe_get("cloud_sync"));
	tcapi_get("AiCloud_Entry", "cloud_sync", buf);
	websWrite(wp, "cloud_sync=\"%s\";\n", buf);

	if(fp == NULL){
		websWrite(wp, "cloud_status=\"ERROR\";\n");
		websWrite(wp, "cloud_obj=\"\";\n");
		websWrite(wp, "cloud_msg=\"\";\n");
		return;
	}

	memset(status, 0, sizeof(status));
	memset(mounted_path, 0, PATH_MAX);
	memset(target_obj, 0, PATH_MAX);
	memset(error_msg, 0, PATH_MAX);

	memset(line, 0, PATH_MAX);
	line_num = 0;
	while(fgets(line, PATH_MAX, fp)){
		++line_num;
		line[strlen(line)-1] = 0;

		switch(line_num){
			case 1:
				strncpy(status, convert_cloudsync_status(line), 16);
				break;
			case 2:
				memset(buf, 0, PATH_MAX);
				char_to_ascii(buf, line);
				strcpy(mounted_path, buf);
				break;
			case 3:
				// memset(buf, 0, PATH_MAX);
				// char_to_ascii(buf, line);
				// strcpy(target_obj, buf);
				strcpy(target_obj, line); // support Chinese
				break;
			case 4:
				strcpy(error_msg, line);
				break;
		}

		memset(line, 0, PATH_MAX);
	}
	fclose(fp);

	if(!line_num){
		websWrite(wp, "cloud_status=\"ERROR\";\n");
		websWrite(wp, "cloud_obj=\"\";\n");
		websWrite(wp, "cloud_msg=\"\";\n");
	}
	else{
		websWrite(wp, "cloud_status=\"%s\";\n", status);
		websWrite(wp, "cloud_obj=\"%s\";\n", target_obj);
		websWrite(wp, "cloud_msg=\"%s\";\n", error_msg);
	}

	return;
}

void substr(char *dest, const char* src, unsigned int start, unsigned int cnt) {
	strncpy(dest, src + start, cnt);
	dest[cnt] = 0;
}

static void UI_cloud_dropbox_status(asp_reent* reent, const asp_text* params, asp_text* ret){
	FILE *fp = fopen("/tmp/smartsync/.logs/dropbox", "r");
	char line[PATH_MAX], buf[PATH_MAX], dest[PATH_MAX];
	int line_num;
	char status[16], mounted_path[PATH_MAX], target_obj[PATH_MAX], error_msg[PATH_MAX], full_capa[PATH_MAX], used_capa[PATH_MAX], rule_num[PATH_MAX];
	char wp[256] = {0};

	if(fp == NULL){
		websWrite(wp, "cloud_dropbox_status=\"WAITING\";\n"); //gauss change status fromm 'ERROR' to 'WAITING' 2014.11.4
		websWrite(wp, "cloud_dropbox_obj=\"\";\n");
		websWrite(wp, "cloud_dropbox_msg=\"\";\n");
		websWrite(wp, "cloud_dropbox_fullcapa=\"\";\n");
		websWrite(wp, "cloud_dropbox_usedcapa=\"\";\n");
		websWrite(wp, "cloud_dropbox_rule_num=\"\";\n");
		return 0;
	}

	memset(status, 0, 16);
	memset(mounted_path, 0, PATH_MAX);
	memset(target_obj, 0, PATH_MAX);
	memset(error_msg, 0, PATH_MAX);
	memset(full_capa, 0, PATH_MAX);
	memset(used_capa, 0, PATH_MAX);
	memset(rule_num, 0, PATH_MAX);

	memset(line, 0, PATH_MAX);
	line_num = 0;
	while(fgets(line, PATH_MAX, fp)){
		++line_num;
		line[strlen(line)-1] = 0;

		if(strstr(line, "STATUS") != NULL){
			strncpy(status, convert_cloudsync_status(line), 16);
		}
		else if(strstr(line, "MOUNT_PATH") != NULL){
			memset(buf, 0, PATH_MAX);
			substr(dest, line, 11, PATH_MAX);
			char_to_ascii(buf, dest);
			strcpy(mounted_path, buf);
		}
		else if(strstr(line, "FILENAME") != NULL){
			substr(dest, line, 9, PATH_MAX);
			strcpy(target_obj, dest); // support Chinese
		}
		else if(strstr(line, "ERR_MSG") != NULL){
			substr(dest, line, 8, PATH_MAX);
			strcpy(error_msg, dest);
		}
		else if(strstr(line, "TOTAL_SPACE") != NULL){
			substr(dest, line, 12, PATH_MAX);
			strcpy(full_capa, dest);
		}
		else if(strstr(line, "USED_SPACE") != NULL){
			substr(dest, line, 11, PATH_MAX);
			strcpy(used_capa, dest);
		}
		else if(strstr(line, "RULENUM") != NULL){
			substr(dest, line, 8, PATH_MAX);
			strcpy(rule_num, dest);
		}
		
		memset(line, 0, PATH_MAX);
	}
	fclose(fp);

	if(!line_num){
		websWrite(wp, "cloud_dropbox_status=\"ERROR\";\n");
		websWrite(wp, "cloud_dropbox_obj=\"\";\n");
		websWrite(wp, "cloud_dropbox_msg=\"\";\n");
		websWrite(wp, "cloud_dropbox_fullcapa=\"\";\n");
		websWrite(wp, "cloud_dropbox_usedcapa=\"\";\n");
		websWrite(wp, "cloud_dropbox_rule_num=\"\";\n");
	}
	else{
		websWrite(wp, "cloud_dropbox_status=\"%s\";\n", status);
		websWrite(wp, "cloud_dropbox_obj=\"%s\";\n", target_obj);
		if(!strcmp(status,"SYNC"))
		   strncpy(error_msg,"Sync has been completed",PATH_MAX);
		else if(!strcmp(status,"INITIAL"))
		   strncpy(error_msg,"Verifying",PATH_MAX);		
		websWrite(wp, "cloud_dropbox_msg=\"%s\";\n", error_msg);
		websWrite(wp, "cloud_dropbox_fullcapa=\"%s\";\n", full_capa);
		websWrite(wp, "cloud_dropbox_usedcapa=\"%s\";\n", used_capa);
		websWrite(wp, "cloud_dropbox_rule_num=\"%s\";\n", rule_num);
	}

	return 0;
}

static void UI_cloud_ftpclient_status(asp_reent* reent, const asp_text* params, asp_text* ret){
	FILE *fp = fopen("/tmp/smartsync/.logs/ftpclient", "r");
	char line[PATH_MAX], buf[PATH_MAX], dest[PATH_MAX];
	int line_num;
	char status[16], mounted_path[PATH_MAX], target_obj[PATH_MAX], error_msg[PATH_MAX], full_capa[PATH_MAX], used_capa[PATH_MAX], rule_num[PATH_MAX];
	char wp[256] = {0};

	if(fp == NULL){
		websWrite(wp, "cloud_ftpclient_status=\"WAITING\";\n"); //gauss change status fromm 'ERROR' to 'WAITING' 2014.11.4
		websWrite(wp, "cloud_ftpclient_obj=\"\";\n");
		websWrite(wp, "cloud_ftpclient_msg=\"\";\n");
		websWrite(wp, "cloud_ftpclient_fullcapa=\"\";\n");
		websWrite(wp, "cloud_ftpclient_usedcapa=\"\";\n");
		websWrite(wp, "cloud_ftpclient_rule_num=\"\";\n");
		return 0;
	}

	memset(status, 0, 16);
	memset(mounted_path, 0, PATH_MAX);
	memset(target_obj, 0, PATH_MAX);
	memset(error_msg, 0, PATH_MAX);
	memset(full_capa, 0, PATH_MAX);
	memset(used_capa, 0, PATH_MAX);
	memset(rule_num, 0, PATH_MAX);

	memset(line, 0, PATH_MAX);
	line_num = 0;
	while(fgets(line, PATH_MAX, fp)){
		++line_num;
		line[strlen(line)-1] = 0;

		if(strstr(line, "STATUS") != NULL){
			strncpy(status, convert_cloudsync_status(line), 16);
		}
		else if(strstr(line, "MOUNT_PATH") != NULL){
			memset(buf, 0, PATH_MAX);
			substr(dest, line, 11, PATH_MAX);
			char_to_ascii(buf, dest);
			strcpy(mounted_path, buf);
		}
		else if(strstr(line, "FILENAME") != NULL){
			substr(dest, line, 9, PATH_MAX);
			strcpy(target_obj, dest); // support Chinese
		}
		else if(strstr(line, "ERR_MSG") != NULL){
			substr(dest, line, 8, PATH_MAX);
			strcpy(error_msg, dest);
		}
		else if(strstr(line, "TOTAL_SPACE") != NULL){
			substr(dest, line, 12, PATH_MAX);
			strcpy(full_capa, dest);
		}
		else if(strstr(line, "USED_SPACE") != NULL){
			substr(dest, line, 11, PATH_MAX);
			strcpy(used_capa, dest);
		}
		else if(strstr(line, "RULENUM") != NULL){
			substr(dest, line, 8, PATH_MAX);
			strcpy(rule_num, dest);
		}
		
		memset(line, 0, PATH_MAX);
	}
	fclose(fp);

	if(!line_num){
		websWrite(wp, "cloud_ftpclient_status=\"ERROR\";\n");
		websWrite(wp, "cloud_ftpclient_obj=\"\";\n");
		websWrite(wp, "cloud_ftpclient_msg=\"\";\n");
		websWrite(wp, "cloud_ftpclient_fullcapa=\"\";\n");
		websWrite(wp, "cloud_ftpclient_usedcapa=\"\";\n");
		websWrite(wp, "cloud_ftpclient_rule_num=\"\";\n");
	}
	else{
		websWrite(wp, "cloud_ftpclient_status=\"%s\";\n", status);
		websWrite(wp, "cloud_ftpclient_obj=\"%s\";\n", target_obj);
		if(!strcmp(status,"SYNC"))
		   strncpy(error_msg,"Sync has been completed",PATH_MAX);
		else if(!strcmp(status,"INITIAL"))
		   strncpy(error_msg,"Verifying",PATH_MAX);		
		websWrite(wp, "cloud_ftpclient_msg=\"%s\";\n", error_msg);
		websWrite(wp, "cloud_ftpclient_fullcapa=\"%s\";\n", full_capa);
		websWrite(wp, "cloud_ftpclient_usedcapa=\"%s\";\n", used_capa);
		websWrite(wp, "cloud_ftpclient_rule_num=\"%s\";\n", rule_num);
	}

	return 0;
}

static void UI_cloud_sambaclient_status(asp_reent* reent, const asp_text* params, asp_text* ret){
	FILE *fp = fopen("/tmp/smartsync/.logs/sambaclient", "r");
	char line[PATH_MAX], buf[PATH_MAX], dest[PATH_MAX];
	int line_num;
	char status[16], mounted_path[PATH_MAX], target_obj[PATH_MAX], error_msg[PATH_MAX], full_capa[PATH_MAX], used_capa[PATH_MAX], rule_num[PATH_MAX];
	char wp[256] = {0};

	if(fp == NULL){
		websWrite(wp, "cloud_sambaclient_status=\"WAITING\";\n"); //gauss change status fromm 'ERROR' to 'WAITING' 2014.11.4
		websWrite(wp, "cloud_sambaclient_obj=\"\";\n");
		websWrite(wp, "cloud_sambaclient_msg=\"\";\n");
		websWrite(wp, "cloud_sambaclient_fullcapa=\"\";\n");
		websWrite(wp, "cloud_sambaclient_usedcapa=\"\";\n");
		websWrite(wp, "cloud_sambaclient_rule_num=\"\";\n");
		return 0;
	}

	memset(status, 0, 16);
	memset(mounted_path, 0, PATH_MAX);
	memset(target_obj, 0, PATH_MAX);
	memset(error_msg, 0, PATH_MAX);
	memset(full_capa, 0, PATH_MAX);
	memset(used_capa, 0, PATH_MAX);
	memset(rule_num, 0, PATH_MAX);

	memset(line, 0, PATH_MAX);
	line_num = 0;
	while(fgets(line, PATH_MAX, fp)){
		++line_num;
		line[strlen(line)-1] = 0;

		if(strstr(line, "STATUS") != NULL){
			strncpy(status, convert_cloudsync_status(line), 16);
		}
		else if(strstr(line, "MOUNT_PATH") != NULL){
			memset(buf, 0, PATH_MAX);
			substr(dest, line, 11, PATH_MAX);
			char_to_ascii(buf, dest);
			strcpy(mounted_path, buf);
		}
		else if(strstr(line, "FILENAME") != NULL){
			substr(dest, line, 9, PATH_MAX);
			strcpy(target_obj, dest); // support Chinese
		}
		else if(strstr(line, "ERR_MSG") != NULL){
			substr(dest, line, 8, PATH_MAX);
			strcpy(error_msg, dest);
		}
		else if(strstr(line, "TOTAL_SPACE") != NULL){
			substr(dest, line, 12, PATH_MAX);
			strcpy(full_capa, dest);
		}
		else if(strstr(line, "USED_SPACE") != NULL){
			substr(dest, line, 11, PATH_MAX);
			strcpy(used_capa, dest);
		}
		else if(strstr(line, "RULENUM") != NULL){
			substr(dest, line, 8, PATH_MAX);
			strcpy(rule_num, dest);
		}
		
		memset(line, 0, PATH_MAX);
	}
	fclose(fp);

	if(!line_num){
		websWrite(wp, "cloud_sambaclient_status=\"ERROR\";\n");
		websWrite(wp, "cloud_sambaclient_obj=\"\";\n");
		websWrite(wp, "cloud_sambaclient_msg=\"\";\n");
		websWrite(wp, "cloud_sambaclient_fullcapa=\"\";\n");
		websWrite(wp, "cloud_sambaclient_usedcapa=\"\";\n");
		websWrite(wp, "cloud_sambaclient_rule_num=\"\";\n");
	}
	else{
		websWrite(wp, "cloud_sambaclient_status=\"%s\";\n", status);
		websWrite(wp, "cloud_sambaclient_obj=\"%s\";\n", target_obj);
		if(!strcmp(status,"SYNC"))
		   strncpy(error_msg,"Sync has been completed",PATH_MAX);
		else if(!strcmp(status,"INITIAL"))
		   strncpy(error_msg,"Verifying",PATH_MAX);		
		websWrite(wp, "cloud_sambaclient_msg=\"%s\";\n", error_msg);
		websWrite(wp, "cloud_sambaclient_fullcapa=\"%s\";\n", full_capa);
		websWrite(wp, "cloud_sambaclient_usedcapa=\"%s\";\n", used_capa);
		websWrite(wp, "cloud_sambaclient_rule_num=\"%s\";\n", rule_num);
	}

	return 0;
}

static void UI_cloud_usbclient_status(asp_reent* reent, const asp_text* params, asp_text* ret){
	FILE *fp = fopen("/tmp/smartsync/.logs/usbclient", "r");
	char line[PATH_MAX], buf[PATH_MAX], dest[PATH_MAX];
	int line_num;
	char status[16], mounted_path[PATH_MAX], target_obj[PATH_MAX], error_msg[PATH_MAX], full_capa[PATH_MAX], used_capa[PATH_MAX], rule_num[PATH_MAX];
	char wp[256] = {0};

	if(fp == NULL){
		websWrite(wp, "cloud_usbclient_status=\"WAITING\";\n"); //gauss change status fromm 'ERROR' to 'WAITING' 2014.11.4
		websWrite(wp, "cloud_usbclient_obj=\"\";\n");
		websWrite(wp, "cloud_usbclient_msg=\"\";\n");
		websWrite(wp, "cloud_usbclient_fullcapa=\"\";\n");
		websWrite(wp, "cloud_usbclient_usedcapa=\"\";\n");
		websWrite(wp, "cloud_usbclient_rule_num=\"\";\n");
		return 0;
	}

	memset(status, 0, 16);
	memset(mounted_path, 0, PATH_MAX);
	memset(target_obj, 0, PATH_MAX);
	memset(error_msg, 0, PATH_MAX);
	memset(full_capa, 0, PATH_MAX);
	memset(used_capa, 0, PATH_MAX);
	memset(rule_num, 0, PATH_MAX);

	memset(line, 0, PATH_MAX);
	line_num = 0;
	while(fgets(line, PATH_MAX, fp)){
		++line_num;
		line[strlen(line)-1] = 0;

		if(strstr(line, "STATUS") != NULL){
			strncpy(status, convert_cloudsync_status(line), 16);
		}
		else if(strstr(line, "MOUNT_PATH") != NULL){
			memset(buf, 0, PATH_MAX);
			substr(dest, line, 11, PATH_MAX);
			char_to_ascii(buf, dest);
			strcpy(mounted_path, buf);
		}
		else if(strstr(line, "FILENAME") != NULL){
			substr(dest, line, 9, PATH_MAX);
			strcpy(target_obj, dest); // support Chinese
		}
		else if(strstr(line, "ERR_MSG") != NULL){
			substr(dest, line, 8, PATH_MAX);
			strcpy(error_msg, dest);
		}
		else if(strstr(line, "TOTAL_SPACE") != NULL){
			substr(dest, line, 12, PATH_MAX);
			strcpy(full_capa, dest);
		}
		else if(strstr(line, "USED_SPACE") != NULL){
			substr(dest, line, 11, PATH_MAX);
			strcpy(used_capa, dest);
		}
		else if(strstr(line, "RULENUM") != NULL){
			substr(dest, line, 8, PATH_MAX);
			strcpy(rule_num, dest);
		}
		
		memset(line, 0, PATH_MAX);
	}
	fclose(fp);

	if(!line_num){
		websWrite(wp, "cloud_usbclient_status=\"ERROR\";\n");
		websWrite(wp, "cloud_usbclient_obj=\"\";\n");
		websWrite(wp, "cloud_usbclient_msg=\"\";\n");
		websWrite(wp, "cloud_usbclient_fullcapa=\"\";\n");
		websWrite(wp, "cloud_usbclient_usedcapa=\"\";\n");
		websWrite(wp, "cloud_usbclient_rule_num=\"\";\n");
	}
	else{
		websWrite(wp, "cloud_usbclient_status=\"%s\";\n", status);
		websWrite(wp, "cloud_usbclient_obj=\"%s\";\n", target_obj);
		if(!strcmp(status,"SYNC"))
		   strncpy(error_msg,"Sync has been completed",PATH_MAX);
		else if(!strcmp(status,"INITIAL"))
		   strncpy(error_msg,"Verifying",PATH_MAX);		
		websWrite(wp, "cloud_usbclient_msg=\"%s\";\n", error_msg);
		websWrite(wp, "cloud_usbclient_fullcapa=\"%s\";\n", full_capa);
		websWrite(wp, "cloud_usbclient_usedcapa=\"%s\";\n", used_capa);
		websWrite(wp, "cloud_usbclient_rule_num=\"%s\";\n", rule_num);
	}

	return 0;
}

static void UI_cloud_status (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE *fp = fopen("/tmp/smartsync/.logs/asuswebstorage", "r");
	char line[PATH_MAX], buf[PATH_MAX], dest[PATH_MAX];
	int line_num;
	char status[16], mounted_path[PATH_MAX], target_obj[PATH_MAX], error_msg[PATH_MAX], full_capa[PATH_MAX], used_capa[PATH_MAX];
	char wp[256] = {0};

	if(fp == NULL){
		websWrite(wp, "cloud_status=\"ERROR\";\n");
		websWrite(wp, "cloud_obj=\"\";\n");
		websWrite(wp, "cloud_msg=\"\";\n");
		websWrite(wp, "cloud_fullcapa=\"\";\n");
		websWrite(wp, "cloud_usedcapa=\"\";\n");
		return;
	}

	memset(status, 0, sizeof(status));
	memset(mounted_path, 0, PATH_MAX);
	memset(target_obj, 0, PATH_MAX);
	memset(error_msg, 0, PATH_MAX);
	memset(full_capa, 0, PATH_MAX);
	memset(used_capa, 0, PATH_MAX);

	memset(line, 0, PATH_MAX);
	line_num = 0;
	while(fgets(line, PATH_MAX, fp)){
		++line_num;
		line[strlen(line)-1] = 0;

		if(strstr(line, "STATUS") != NULL){
			strncpy(status, convert_cloudsync_status(line), 16);
		}
		else if(strstr(line, "MOUNT_PATH") != NULL){
			memset(buf, 0, PATH_MAX);
			substr(dest, line, 11, PATH_MAX);
			char_to_ascii(buf, dest);
			strcpy(mounted_path, buf);
		}
		else if(strstr(line, "FILENAME") != NULL){
			substr(dest, line, 9, PATH_MAX);
			strcpy(target_obj, dest); // support Chinese
			break;
		}
		else if(strstr(line, "ERR_MSG") != NULL){
			substr(dest, line, 8, PATH_MAX);
			strcpy(error_msg, dest);
		}
		else if(strstr(line, "TOTAL_SPACE") != NULL){
			substr(dest, line, 12, PATH_MAX);
			strcpy(full_capa, dest);
		}
		else if(strstr(line, "USED_SPACE") != NULL){
			substr(dest, line, 11, PATH_MAX);
			strcpy(used_capa, dest);
		}

		memset(line, 0, PATH_MAX);
	}
	fclose(fp);

	if(!line_num){
		websWrite(wp, "cloud_status=\"ERROR\";\n");
		websWrite(wp, "cloud_obj=\"\";\n");
		websWrite(wp, "cloud_msg=\"\";\n");
		websWrite(wp, "cloud_fullcapa=\"\";\n");
		websWrite(wp, "cloud_usedcapa=\"\";\n");
	}
	else{
		websWrite(wp, "cloud_status=\"%s\";\n", status);
		websWrite(wp, "cloud_obj=\"%s\";\n", target_obj);
		websWrite(wp, "cloud_msg=\"%s\";\n", error_msg);
		websWrite(wp, "cloud_fullcapa=\"%s\";\n", full_capa);
		websWrite(wp, "cloud_usedcapa=\"%s\";\n", used_capa);
	}

	return;
}

static void UI_rs_status (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE *fp = fopen("/tmp/Cloud/log/WebDAV", "r");
  char line[PATH_MAX], buf[PATH_MAX], dest[PATH_MAX];
  int line_num;
  char rulenum[PATH_MAX], status[16], mounted_path[PATH_MAX], target_obj[PATH_MAX], error_msg[PATH_MAX], full_capa[PATH_MAX], used_capa[PATH_MAX];
	char wp[256] = {0};

        if(fp == NULL){
                websWrite(wp, "rs_rulenum=\"\";\n");
                websWrite(wp, "rs_status=\"ERROR\";\n");
                websWrite(wp, "rs_obj=\"\";\n");
                websWrite(wp, "rs_msg=\"\";\n");
                websWrite(wp, "rs_fullcapa=\"\";\n");
                websWrite(wp, "rs_usedcapa=\"\";\n");
                return;
        }

        memset(status, 0, sizeof(status));
        memset(rulenum, 0, PATH_MAX);
        memset(mounted_path, 0, PATH_MAX);
        memset(target_obj, 0, PATH_MAX);
        memset(error_msg, 0, PATH_MAX);
        memset(full_capa, 0, PATH_MAX);
        memset(used_capa, 0, PATH_MAX);

        memset(line, 0, PATH_MAX);
        line_num = 0;
        while(fgets(line, PATH_MAX, fp)){
                ++line_num;
                line[strlen(line)-1] = 0;

                if(strstr(line, "STATUS") != NULL){
                        strncpy(status, convert_cloudsync_status(line), 16);
                }
                else if(strstr(line, "RULENUM") != NULL){
                        substr(dest, line, 8, PATH_MAX);
                        strcpy(rulenum, dest);
                }
                else if(strstr(line, "MOUNT_PATH") != NULL){
                        memset(buf, 0, PATH_MAX);
                        substr(dest, line, 11, PATH_MAX);
                        char_to_ascii(buf, dest);
                        strcpy(mounted_path, buf);
                }
                else if(strstr(line, "FILENAME") != NULL){
                        substr(dest, line, 9, PATH_MAX);
                        strcpy(target_obj, dest); // support Chinese
                }
                else if(strstr(line, "ERR_MSG") != NULL){
                        substr(dest, line, 8, PATH_MAX);
                        strcpy(error_msg, dest);
                }
                else if(strstr(line, "TOTAL_SPACE") != NULL){
                        substr(dest, line, 12, PATH_MAX);
                        strcpy(full_capa, dest);
                }
                else if(strstr(line, "USED_SPACE") != NULL){
                        substr(dest, line, 11, PATH_MAX);
                        strcpy(used_capa, dest);
                }

                memset(line, 0, PATH_MAX);
        }
        fclose(fp);

        if(!line_num){
                websWrite(wp, "rs_rulenum=\"\";\n");
                websWrite(wp, "rs_status=\"ERROR\";\n");
                websWrite(wp, "rs_obj=\"\";\n");
                websWrite(wp, "rs_msg=\"\";\n");
                websWrite(wp, "rs_fullcapa=\"\";\n");
                websWrite(wp, "rs_usedcapa=\"\";\n");
        }
        else{
                websWrite(wp, "rs_rulenum=\"%s\";\n", rulenum);
                websWrite(wp, "rs_status=\"%s\";\n", status);
                websWrite(wp, "rs_obj=\"%s\";\n", target_obj);
                websWrite(wp, "rs_msg=\"%s\";\n", error_msg);
                websWrite(wp, "rs_fullcapa=\"%s\";\n", full_capa);
                websWrite(wp, "rs_usedcapa=\"%s\";\n", used_capa);
        }

        return;
}
#endif	//RTCONFIG_CLOUDSYNC

static void ej_webdavInfo(asp_reent* reent, const asp_text* params, asp_text* ret){
	char wp[256];
	char buf[MAXLEN_TCAPI_MSG];
	char NodeName[32] = {0};

	websWrite(wp, "// pktInfo=['PrinterInfo','SSID','NetMask','ProductID','FWVersion','OPMode','MACAddr','Regulation'];\n");
	websWrite(wp, "pktInfo=['','%s',", tcapi_get_string("WLan_Entry0", "wl0_ssid", buf));
	websWrite(wp, "'%s',", tcapi_get_string("Lan_Entry0", "netmask", buf));
	websWrite(wp, "'%s',", tcapi_get_string("SysInfo_Entry", "ProductName", buf));
	websWrite(wp, "'%s',", tcapi_get_string("DeviceInfo", "FwVer", buf));
	websWrite(wp, "'1',");	//sw_mode
	websWrite(wp, "'%s',", tcapi_get_string("Info_Ether", "mac", buf));
	websWrite(wp, "''];\n");

	websWrite(wp, "// webdavInfo=['Webdav','HTTPType','HTTPPort','DDNS','HostName','WAN0IPAddr','','xSetting','HTTPSPort'];\n");
	websWrite(wp, "webdavInfo=['%s',", tcapi_get_string("AiCloud_Entry", "enable_webdav", buf));
	websWrite(wp, "'%s',", tcapi_get_string("AiCloud_Entry", "st_webdav_mode", buf));
	websWrite(wp, "'%s',", tcapi_get_string("AiCloud_Entry", "webdav_http_port", buf));
	websWrite(wp, "'%s',", tcapi_get_string("Ddns_Entry", "Active", buf));
	websWrite(wp, "'%s',", tcapi_get_string("Ddns_Entry", "MYHOST", buf));
	snprintf(NodeName, sizeof(NodeName), "DeviceInfo_PVC%d", wan_primary_ifunit());
	websWrite(wp, "'%s',", tcapi_get_string(NodeName, "WanIP", buf));
	websWrite(wp, "'%s',", "");	//nvram_safe_get("")
	websWrite(wp, "'%s',", tcapi_get_string("SysInfo_Entry", "x_Setting", buf));
	websWrite(wp, "'%s',", tcapi_get_string("AiCloud_Entry", "webdav_https_port", buf));
	/* for ASUS Router (AiHome) APP */
#ifdef RTCONFIG_WEBDAV
	websWrite(wp, "'1'");
#else
	if(check_if_file_exist("/opt/etc/init.d/S50aicloud")) websWrite(wp, "'1'");
	else websWrite(wp, "'0'");
#endif
	websWrite(wp, "];\n");

	return;
}

static void apps_info(asp_reent* reent, const asp_text* params, asp_text* ret){
	apps_info_t *follow_apps_info, *apps_info_list;
	char *name;
	char wp[1024] = {0};

	// if (ejArgs(argc, argv, "%s", &name) < 1)
		// name = APP_OWNER_ALL;
	if (params[0].str && params[0].len) {
		name = (char*)asp_alloc(reent,params[0].len+1);
		memset(name, 0, params[0].len+1);
		memcpy(name, params[0].str, params[0].len);
	}
	else
		name = APP_OWNER_ALL;
	if (strcmp(name, APP_OWNER_ALL) != 0 &&
	    strcmp(name, APP_OWNER_ASUS) != 0 &&
	    strcmp(name, APP_OWNER_OTHERS) != 0) {
		websWrite(wp, "[]");
		return;
	}

	apps_info_list = follow_apps_info = get_apps_list(name);
	websWrite(wp, "[");
	while (follow_apps_info != NULL) {
		websWrite(wp, "[");
		websWrite(wp, "\"%s\", ", follow_apps_info->name ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->version ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->new_version ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->installed ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->enabled ? : FIELD_YES);
		websWrite(wp, "\"%s\", ", follow_apps_info->source ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->url ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->description ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->depends ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->optional_utility ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->new_optional_utility ? : "");
		websWrite(wp, "\"%s\", ", follow_apps_info->help_path ? : "");
		websWrite(wp, "\"%s\"",   follow_apps_info->new_file_name ? : "");

		follow_apps_info = follow_apps_info->next;
		websWrite(wp, "]%s\n", follow_apps_info ? "," : "");
	}
	websWrite(wp, "]");
	free_apps_list(&apps_info_list);

	return;
}

static void apps_action(asp_reent* reent, const asp_text* params, asp_text* ret){
	char *apps_action = get_param(g_var_post, "apps_action");//websGetVar(wp, "apps_action", "");
	char *apps_name = get_param(g_var_post, "apps_name");//websGetVar(wp, "apps_name", "");
	char *apps_flag = get_param(g_var_post, "apps_flag");//websGetVar(wp, "apps_flag", "");
	char command[128];

	if(!apps_action || strlen(apps_action) <= 0)
		return;

	// nvram_set("apps_state_action", apps_action);
	tcapi_set(APPS_DATA, "apps_state_action", apps_action);

	memset(command, 0, sizeof(command));

	if(!strcmp(apps_action, "install")){
		if(strlen(apps_name) <= 0 || strlen(apps_flag) <= 0)
			return;

		// sprintf(command, "start_apps_install %s %s", apps_name, apps_flag);
		sprintf(command, "app_install.sh %s %s", apps_name, apps_flag);
	}
	else if(!strcmp(apps_action, "stop")){
		// sprintf(command, "start_apps_stop");
		sprintf(command, "app_stop.sh");
	}
	else if(!strcmp(apps_action, "update")){
		// sprintf(command, "start_apps_update");
		sprintf(command, "app_update.sh");
	}
	else if(!strcmp(apps_action, "upgrade")){
		if(strlen(apps_name) <= 0)
			return;

		// sprintf(command, "start_apps_upgrade %s", apps_name);
		sprintf(command, "app_upgrade.sh %s", apps_name);
	}
	else if(!strcmp(apps_action, "remove")){
		if(strlen(apps_name) <= 0)
			return;

		// sprintf(command, "start_apps_remove %s", apps_name);
		sprintf(command, "app_remove.sh %s", apps_name);
	}
	else if(!strcmp(apps_action, "enable")){
		if(strlen(apps_name) <= 0 || strlen(apps_flag) <= 0)
			return;

		if(strcmp(apps_flag, "yes") && strcmp(apps_flag, "no"))
			return;

		// sprintf(command, "start_apps_enable %s %s", apps_name, apps_flag);
		sprintf(command, "app_set_enabled.sh %s %s", apps_name, apps_flag);
	}
	else if(!strcmp(apps_action, "switch")){
		if(strlen(apps_name) <= 0 || strlen(apps_flag) <= 0)
			return;

		// sprintf(command, "start_apps_switch %s %s", apps_name, apps_flag);
		sprintf(command, "app_switch.sh %s %s", apps_name, apps_flag);
	}
	else if(!strcmp(apps_action, "cancel")){
		// sprintf(command, "start_apps_cancel");
		sprintf(command, "app_cancel.sh");
	}

	// if(strlen(command) > 0)
		// notify_rc(command);
	if(strlen(command) > 0) {
		tcapi_set(APPS_DATA, "apps_state_autorun", "");
		tcapi_set(APPS_DATA, "apps_state_install", "");
		tcapi_set(APPS_DATA, "apps_state_remove", "");
		tcapi_set(APPS_DATA, "apps_state_switch", "");
		tcapi_set(APPS_DATA, "apps_state_stop", "");
		tcapi_set(APPS_DATA, "apps_state_enable", "");
		tcapi_set(APPS_DATA, "apps_state_update", "");
		tcapi_set(APPS_DATA, "apps_state_upgrade", "");
		tcapi_set(APPS_DATA, "apps_state_cancel", "");
		tcapi_set(APPS_DATA, "apps_state_error", "");
		strcat(command, " &");
		system(command);
	}

	return;
}

#ifdef RTCONFIG_MEDIA_SERVER
// dms_info: ["Scanning"] or [ "" ]

static void dms_info(asp_reent* reent, const asp_text* params, asp_text* ret){
	char dms_dbcwd[MAXLEN_TCAPI_MSG];
	char dms_scanfile[64], dms_status[32];
	FILE *fp;
	char wp[256] = {0};

	tcapi_get("Apps_Entry", "dms_dbcwd", dms_dbcwd);

	strcpy(dms_status, "");

	if(strlen(dms_dbcwd))
	{
		sprintf(dms_scanfile, "%s/scantag", dms_dbcwd);

		fp = fopen(dms_scanfile, "r");

		if(fp) {
			strcpy(dms_status, "Scanning");
			fclose(fp);
		}
	}

	websWrite(wp, "[\"%s\"]", dms_status);

	return;
}
#endif

#ifdef TCSUPPORT_SWAP_FILE
static void do_swap(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *swap_enable = get_param(g_var_post, "swap_enable");

	if(!swap_enable)
		return;

	if(!strcmp(swap_enable, "1"))
		eval("/sbin/rc_call", "swap", "1");
	else
		eval("/sbin/rc_call", "swap", "0");
}
#endif

#endif	//RTCONFIG_USB

#ifdef TCSUPPORT_SYSLOG_ENHANCE
void logmessage(char *logheader, char *fmt, ...)
{
	va_list args;
	char buf[512];

	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);
	openlog(logheader, 0, 0);
	syslog(0, buf);
	closelog();
	va_end(args);
}
#endif

/* Paul modify 2013/2/7 */
static void show_error_msg(const char *const msg_num){
	char msg_name[32];
	char str_type[4] = {0};
	int	nIndex = 0;
	char str_path[PATH_LENGTH] = {0};

	memset(msg_name, 0, 32);
	sprintf(msg_name, "ALERT_OF_ERROR_%s", msg_num);

	tcapi_get("LanguageSwitch_Entry", "Type", str_type);
	if (strlen(str_type))
		nIndex = atoi(str_type);

	if(nIndex == 1) //English
		sprintf(str_path, STRING_PATH, "EN");
	else if(nIndex == 2) //Brazil
		sprintf(str_path, STRING_PATH, "BR");
	else if(nIndex == 3) //Simplified Chinese
		sprintf(str_path, STRING_PATH, "CN");
	else if(nIndex == 4) //Cesky
		sprintf(str_path, STRING_PATH, "CZ");
	else if(nIndex == 5) //Dansk
		sprintf(str_path, STRING_PATH, "DA");
	else if(nIndex == 6) //Deutsch
		sprintf(str_path, STRING_PATH, "DE");
	else if(nIndex == 7) //Espanol
		sprintf(str_path, STRING_PATH, "ES");
	else if(nIndex == 8) //Finsk
		sprintf(str_path, STRING_PATH, "FI");
	else if(nIndex == 9) //Francais
		sprintf(str_path, STRING_PATH, "FR");
	else if(nIndex == 10) //Italiano
		sprintf(str_path, STRING_PATH, "IT");
	else if(nIndex == 11) //Malay
		sprintf(str_path, STRING_PATH, "MS");
	else if(nIndex == 12) //Norsk
		sprintf(str_path, STRING_PATH, "NO");
	else if(nIndex == 13) //Polski
		sprintf(str_path, STRING_PATH, "PL");
	else if(nIndex == 14) //Russian
		sprintf(str_path, STRING_PATH, "RU");
	else if(nIndex == 15) //Svensk
		sprintf(str_path, STRING_PATH, "SV");
	else if(nIndex == 16) //Thai
		sprintf(str_path, STRING_PATH, "TH");
	else if(nIndex == 17) //Turkey
		sprintf(str_path, STRING_PATH, "TR");
	else if(nIndex == 18) //Traditional Chinese
		sprintf(str_path, STRING_PATH, "TW");
	else if(nIndex == 19) //Ukraine
		sprintf(str_path, STRING_PATH, "UK");
	else
		sprintf(str_path, STRING_PATH, "EN");

	error_msg = get_msg_from_dict(str_path, msg_name);
#ifdef TCSUPPORT_SYSLOG_ENHANCE
	logmessage("asp_handler", "%s %s", msg_num, error_msg);
#endif
	return;
}

static void clean_error_msg(){
	if (error_msg_console != NULL)
		free(error_msg_console);

	if (error_msg != NULL)
		free(error_msg);

	return;
}

/* Paul modify 2013/2/7 */
static char *get_msg_from_dict(char *lang_path, const char *const msg_name){
// #define MAX_FILE_LENGTH 512
	char dict_file[MAX_FILE_LENGTH], *dict_info;
	char *follow_info, *follow_info_end, *target;
	int len;

	memset(dict_file, 0, MAX_FILE_LENGTH);
	sprintf(dict_file, "%s", lang_path);

	dict_info = read_whole_file(dict_file);
	if (dict_info == NULL){
		dbgprintf("No dictionary file, \"%s\".\n", dict_file);
		return NULL;
	}

	follow_info = strstr(dict_info, msg_name);
	if (follow_info == NULL){
		dbgprintf("No \"%s\" in the dictionary file.\n", msg_name);
		free(dict_info);
		return NULL;
	}

	follow_info += strlen(msg_name)+strlen("=");
	follow_info_end = follow_info;
	while (*follow_info_end != 0 && *follow_info_end != '\n')
		++follow_info_end;

	len = follow_info_end-follow_info;

	target = (char *)malloc(sizeof(char)*(len+1));
	if (target == NULL){
		dbgprintf("No memory \"target\".\n");
		free(dict_info);
		return NULL;
	}
	if (len > 0)
		strncpy(target, follow_info, len);
	target[len] = 0;

	free(dict_info);
	return target;
}

static void qis_do_dsl_iptv (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char dsltmp_cfg_iptv_pvclist[MAXLEN_TCAPI_MSG] = {0};
	char dsltmp_transfer_mode[MAXLEN_TCAPI_MSG] = {0};
	char *nvp, *b;
	char *p1, *p2, *p3, *p4, *p5;
	int i = 0;
	char node[MAXLEN_NODE_NAME];
	char cmd[256];
	char tmp[64];

	//LAN IPTV Settings
	tcapi_get("GUITemp_Entry0", "dsltmp_transfer_mode", dsltmp_transfer_mode);
	
	if(!strncmp(dsltmp_transfer_mode, "PTM", 3)) 	//PTM
	{
		if(tcapi_match("GUITemp_Entry0", "dsltmp_cfg_iptv_mr", "1")) {
			tcapi_set("IPTV_Entry", "vd_mr_enable_x", "1");
			tcapi_set("IPTV_Entry", "vd_wan_port", "1");
		}
		else {
			tcapi_set("IPTV_Entry", "vd_mr_enable_x", "0");
		}	
		
		if(tcapi_match("GUITemp_Entry0", "dsltmp_cfg_iptv_rmvlan", "1")) {
			tcapi_set("IPTV_Entry", "vd_rmvlan", "1");
		}
		else {
			tcapi_set("IPTV_Entry", "vd_rmvlan", "0");
		}
	}	
	else	//ATM
	{
		if(tcapi_match("GUITemp_Entry0", "dsltmp_cfg_iptv_mr", "1")) {
			tcapi_set("IPTV_Entry", "ad_mr_enable_x", "1");
			tcapi_set("IPTV_Entry", "ad_wan_port", "1");
		}
		else {
			tcapi_set("IPTV_Entry", "ad_mr_enable_x", "0");
		}

		if(tcapi_match("GUITemp_Entry0", "dsltmp_cfg_iptv_rmvlan", "1")) {
			tcapi_set("IPTV_Entry", "ad_rmvlan", "1");
		}
		else {
			tcapi_set("IPTV_Entry", "ad_rmvlan", "0");
		}
	}

	tcapi_commit("IPTV");

	//Set Wan
	if(tcapi_get("GUITemp_Entry0", "dsltmp_cfg_iptv_pvclist", dsltmp_cfg_iptv_pvclist) == TCAPI_PROCESS_OK
		&& tcapi_get("GUITemp_Entry0", "dsltmp_transfer_mode", dsltmp_transfer_mode) == TCAPI_PROCESS_OK
	) {
		nvp = dsltmp_cfg_iptv_pvclist;
		while( (b = strsep(&nvp, "<")) != NULL){
			if((vstrsep(b, ">", &p1, &p2, &p3, &p4, &p5) != 5))
				continue;
			++i;
			//dbgprintf("[%s, %d]set vpi/vci,proto,encap,vlan: [%s/%s][%s][%s][%s]\n", __FUNCTION__, __LINE__, p1,p2,p3,p4,p5);
			if(!strncmp(dsltmp_transfer_mode, "PTM", 3)) {
				snprintf(node, sizeof(node), "WanExt_PVC8e%d", i);
			}
			else {
				snprintf(node, sizeof(node), "Wan_PVC%d", i);
				tcapi_set(node, "VPI", p1);
				tcapi_set(node, "VCI", p2);

				if(strcmp(p4, "1"))	//LLC
				{
					//check protocol
					if(!strcmp(p3, "0"))		//pppoe0
						tcapi_set(node, "ENCAP", "PPPoE LLC");
					else if(!strcmp(p3, "1"))	//pppoa
						tcapi_set(node, "ENCAP", "PPPoA LLC");
					else if(!strcmp(p3, "2"))	//Automatic IP
						tcapi_set(node, "ENCAP", "1483 Bridged IP LLC");
//					else if(!strcmp(p3, "4"))	//ipoa
					else	//bridge				
					tcapi_set(node, "ENCAP", "1483 Bridged Only LLC");
				}	
				else	//VC-MUX
				{
					if(!strcmp(p3, "0"))		//pppoe
						tcapi_set(node, "ENCAP", "PPPoE VC-Mux");
					else if(!strcmp(p3, "1"))	//pppoa
						tcapi_set(node, "ENCAP", "PPPoA VC-Mux");
					else if(!strcmp(p3, "2"))	//Automatic IP
					tcapi_set(node, "ENCAP", "1483 Bridged IP VC-Mux");
//					else if(!strcmp(p3, "4"))	//ipoa
					else	//bridge				
						tcapi_set(node, "ENCAP", "1483 Bridged Only VC-Mux");
				}

				//QoS
				//tcapi_set(node,"QOS","rt-vbr");
				//tcapi_set(node,"PCR","300");
				//tcapi_set(node,"SCR","299");
				//tcapi_set(node,"MBS","32");
			}

			//dbgprintf("[%s, %d]node(%s)\n", __FUNCTION__, __LINE__, node);
			
			//proto
			if(!strcmp(p3, "0") || !strcmp(p3, "1"))		//ppp
			{
				tcapi_set(node, "ISP", "2");

				if(!tcapi_get("GUITemp_Entry0", "dsltmp_cfg_iptv_username", tmp) && tmp[0] != '\0')
					tcapi_set(node, "USERNAME", tmp);
				
				if(!tcapi_get("GUITemp_Entry0", "dsltmp_cfg_iptv_passwd", tmp) && tmp[0] != '\0')
					tcapi_set(node, "PASSWORD", tmp);

				tcapi_set(node, "WAN_MAC", "");
				tcapi_set(node, "AUTHEN", "AUTO");
				tcapi_set(node, "CONNECTION", "Connect_Keep_Alive");
				tcapi_set(node, "CLOSEIFIDLE", "0");
				tcapi_set(node, "MSS", "0");
				tcapi_set(node, "MTU", "1492");
				tcapi_set(node, "wan_pppoe_options", "");
				tcapi_set(node, "PPPGETIP", "Dynamic");
				tcapi_set(node, "MLDproxy", "No");
				tcapi_set(node, "DNS_type", "0");
				tcapi_set(node, "Primary_DNS", "");
				tcapi_set(node, "Secondary_DNS", "");
				tcapi_set(node, "RIPVERSION", "RIP1");
				tcapi_set(node, "DIRECTION", "None");
				tcapi_set(node, "DEFAULTROUTE", "No");
				tcapi_set(node, "PPPv6Mode", "");
				tcapi_set(node, "PPPv6PD", "Yes");
				tcapi_set(node, "LASTmss", "0");				
			}
			else  if(!strcmp(p3, "2"))	//dhcp
			{
				tcapi_set(node, "ISP", "0");
				if(tcapi_match("GUITemp_Entry0", "dsltmp_cfg_iptv_dns_enable", "1"))
				{					
					tcapi_set(node, "DNS_type", "1");
					if(!tcapi_get("GUITemp_Entry0", "dsltmp_cfg_iptv_dns1", tmp) && tmp[0] != '\0')
						tcapi_set(node, "Primary_DNS", tmp);

					if(!tcapi_get("GUITemp_Entry0", "dsltmp_cfg_iptv_dns2", tmp) && tmp[0] != '\0')
						tcapi_set(node, "Secondary_DNS", tmp);					
				}
			else
					tcapi_set(node, "DNS_type", "0");
				tcapi_set(node, "MLDproxy", "No");				
				tcapi_set(node, "DEFAULTROUTE", "No");				
				tcapi_set(node, "MTU", "1492");				
				tcapi_set(node, "RIPVERSION", "RIP1");				
				tcapi_set(node, "IPOE_DOT1X_STATUS", "Disabled");				
			}
			else	//bridge
			{
				dbgprintf("[%s, %d]\n", __FUNCTION__, __LINE__);
				tcapi_set(node, "ISP", "3");
				tcapi_set(node, "bridge_lan_port", "1");
			}
			
			//vlan
			if(!strcmp(p5, "")) {
				tcapi_set(node, "dot1q", "No");
			}
			else {
				tcapi_set(node, "dot1q", "Yes");
				tcapi_set(node, "VLANID", p5);
			}

			tcapi_set(node, "IPVERSION", "IPv4");
			tcapi_set(node, "Active", "Yes");
			tcapi_commit(node);
		}
#if 0
		if(i) {
			tcapi_set("IPTV_Entry", "switch_stb_x", "1");
		#ifdef TCSUPPORT_WAN_ETHER_LAN
			int wans_lanport = -1;
			tcapi_set("Dualwan_Entry", "wans_lanport", "4");
			wans_lanport = get_cur_lanwan_port();
			if(wans_lanport != -1) {
				set_lanwan_if(wans_lanport, 4);
				tcapi_commit("Wan_PVC12");
			}
		#endif
		}
		else {
#endif
		if(!i){
			//disable iptv pvc if re-run QIS but select internet service only.
			for(i = 1; i < 8; i++) {
			#ifdef TCSUPPORT_MULTISERVICE_ON_WAN
				if(!strncmp(dsltmp_transfer_mode, "PTM", 3)) {
					snprintf(node, sizeof(node), "WanExt_PVC8e%d", i);
					snprintf(cmd, sizeof(cmd), "/usr/script/wan_stop.sh 8 %d; /usr/bin/smuxctl rem nas8_%d", i, i);
				}
				else {
					snprintf(node, sizeof(node), "Wan_PVC%d", i);
					snprintf(cmd, sizeof(cmd), "/usr/script/wan_stop.sh %d 0", i);
				}
			#else
				snprintf(node, sizeof(node), "Wan_PVC%d", i);
				snprintf(cmd, sizeof(cmd), "/usr/script/wan_stop.sh %d", i);
			#endif
				if(tcapi_match(node, "Active", "Yes")) {
					system(cmd);
					tcapi_set(node, "Active", "No");
				}
			}
		}
	}
}

static void do_apply_cgi (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char value[32] = {0};
	char wp[32] = {0};
	request *req;
	asus_token_t *token = NULL;
	struct json_object *root=NULL;
	FILE *fp;
	char tmp[4096];
	int app_method_hit = 0;

	/* get full URL */
	fp = fopen("/tmp/boa-temp", "r");
	if(fp) {
		fgets(tmp, 4096, fp);
		fclose(fp);
	}

	decode_json_buffer(tmp); //f24834f2, add decode json buffer for app
	//_dprintf("[AiHome] do_apply_cgi: decode tmp=%s\n", tmp);
	root = json_tokener_parse(tmp);
	if (!root) {
		//return 0; /* Aicloud app can not use JSON format */
	}

	char *action_mode = get_cgi_json(g_var_post, "action_mode", root);
	char *current_url = get_cgi_json(g_var_post, "current_page", root);
	//_dprintf("[AiHome] do_apply_cgi: %s %s\n", action_mode, current_url);

	if(!action_mode || !reent || !(reent->server_env)) {
		json_object_put(root);
		return;
	}
	req = reent->server_env;

	if (!strcmp(action_mode, "apply")) {
		app_method_hit = appSet_attribute(wp, root);
		char *rc_service = get_cgi_json(g_var_post, "rc_service", root);
		//_dprintf("[AiHome] do_apply_cgi: rc_service=%s\n", rc_service);
		char *saveptr = NULL;
		char *rc_service_t = strtok_r(rc_service, ";", &saveptr);
		while (rc_service_t != NULL){
			if (appDo_rc_service(rc_service_t, wp, root))
				app_method_hit=2;
			rc_service_t = strtok_r(NULL, ";", &saveptr);
		}

		if (app_method_hit == 2) {
			websWrite(wp, "MODIFIED\n");
			websWrite(wp, "RUN SERVICE\n");
			tcapi_save();
		}
		else if (app_method_hit == 1) {
			websWrite(wp, "MODIFIED\n");
			tcapi_save();
		}
		else {
			websWrite(wp, "NOT MODIFIED\n");
		}

		char *action_script = get_cgi_json(g_var_post, "action_script", root);
		if(action_script) {
			/* [SET] [QoS] Parental Control Rule */
			if(!strcmp(action_script, "saveNvram")) {
				char *current_page = get_cgi_json(g_var_post, "current_page", root);

				if(current_page) {
#ifdef RTCONFIG_OPENVPN
					if(!strcmp(current_page, "Advanced_VPN_OpenVPN.asp")) {
						char *formname = get_cgi_json(g_var_post, "formname", root);

						if(formname) {
							if(!strcmp(formname, "openvpnTLSKeyForm")) {
								_save_openvpn_server_tls();
							}
							else if(!strcmp(formname, "openvpnStaticKeyForm")) {
								_save_openvpn_server_static();
							}
						}
					}
					else if(!strcmp(current_page, "Advanced_VPNClient_Content.asp")) {
						char *formname = get_cgi_json(g_var_post, "formname", root);

						if(formname) {
							if(!strcmp(formname, "openvpnManualForm")) {
								_save_openvpn_client_cert();
							}
						}
					}
#endif
				}
			}
		}
	}
	else if (!strcmp(action_mode," Refresh "))
	{
		int i=0, j=0;
		char *system_cmd = get_cgi_json(g_var_post, "SystemCmd", root);
		int len = strlen(system_cmd);

		for(i=0;i<len;i++){
			if (isalnum(system_cmd[i]) != 0 || system_cmd[i] == ':' || system_cmd[i] == '-' || system_cmd[i] == '_' || system_cmd[i] == '.' || isspace(system_cmd[i]) != 0)
				j++;
			else{
				_dprintf("[httpd] Invalid SystemCmd!\n");
				strcpy(SystemCmd, "");

				json_object_put(root);
				websRedirect_iframe(wp, current_url);

				return;
			}
		}
		if(!strcmp(current_url, "Main_Netstat_Content.asp") && (
			strncasecmp(system_cmd, "netstat", 7) == 0
		)){
			strncpy(SystemCmd, system_cmd, sizeof(SystemCmd));
		}
		else if(!strcmp(current_url, "Main_Analysis_Content.asp") && (
			   strncasecmp(system_cmd, "ping", 4) == 0
			|| strncasecmp(system_cmd, "traceroute", 10) == 0
			|| strncasecmp(system_cmd, "nslookup", 8) == 0
		)){
			strncpy(SystemCmd, system_cmd, sizeof(SystemCmd));
		}
		else if(!strcmp(current_url, "Main_WOL_Content.asp") && (
			strncasecmp(system_cmd, "ether-wake", 10) == 0
		)){
			strncpy(SystemCmd, system_cmd, sizeof(SystemCmd));
			sys_script("syscmd.sh");
		}
		else if(!strcmp(current_url, "Main_AdmStatus_Content.asp"))
		{
			if(strncasecmp(system_cmd, "run_telnetd", 11) == 0){
				strncpy(SystemCmd, system_cmd, sizeof(SystemCmd));
				sys_script("syscmd.sh");
			}else if(strncasecmp(system_cmd, "run_infosvr", 11) == 0){
				nvram_set("ateCommand_flag", "1");
			}else if(strncasecmp(system_cmd, "set_factory_mode", 16) == 0){
				strncpy(SystemCmd, system_cmd, sizeof(SystemCmd));
				sys_script("syscmd.sh");
			}
		}
		else{
			_dprintf("[httpd] Invalid SystemCmd!\n");
			strcpy(SystemCmd, "");
		}

		websWrite(wp, "MODIFIED\n");
		websWrite(wp, "RUN SERVICE\n");
		tcapi_save();
		json_object_put(root);
		websRedirect_iframe(wp, current_url);
		return;
	}
	/* [SET] [General] Restore */
	else if (!strcmp(action_mode, "Restore")||!strcmp(action_mode, "restore"))
	{
		tcapi_set("System_Entry", "reboot_type", "2");
		tcapi_commit("System_Entry");

		websWrite(wp, "MODIFIED\n");
		websWrite(wp, "RUN SERVICE\n");
		json_object_put(root);
		return;
	}
	/* [SET] [Wireless] WPS Method */
	else if (!strcmp(action_mode, "wps_apply"))
	{
		char *rc_service = get_cgi_json(g_var_post, "rc_service", root);
		if(!strcmp(rc_service, "start_wps_method")) {
			tcapi_commit("WLan_Entry0");

			websWrite(wp, "MODIFIED\n");
			websWrite(wp, "RUN SERVICE\n");
			tcapi_save();
			json_object_put(root);
			return;
		}
	}
	else if (!strcmp(action_mode, "change_wan_pvc"))
	{
		char *wan_pvc = get_param(g_var_post, "wan_pvc");

		if(wan_pvc) {
			csprintf("set wan_pvc=[%s]\n", wan_pvc);
			tcapi_set("WebCurSet_Entry", "wan_pvc", wan_pvc);
			tcapi_set("WebCurSet_Entry", "dev_pvc", wan_pvc);

			if(!strcmp(wan_pvc, "0"))
				tcapi_set("Wan_Common", "TransMode", "ATM");
			else if(!strcmp(wan_pvc, "8"))
				tcapi_set("Wan_Common", "TransMode", "PTM");
			else if(!strcmp(wan_pvc, "10"))
				tcapi_set("Wan_Common", "TransMode", "Ethernet");
			else if(!strcmp(wan_pvc, "11"))
				tcapi_set("Wan_Common", "TransMode", "USB");
			else if(!strcmp(wan_pvc, "12"))
				tcapi_set("Wan_Common", "TransMode", "LAN");
		}
	}
#ifdef RTCONFIG_DUALWAN
	else if (!strcmp(action_mode, "change_wan_unit"))
	{
		char *wan_unit = get_cgi_json(g_var_post, "wan_unit", root);

		if(wan_unit) {
			csprintf("wan_unit=[%s]\n", wan_unit);
			tcapi_set("WebCurSet_Entry", "wan_unit", wan_unit);

			memset(value, 0, sizeof(value));
			if(!strncmp(wan_unit, "1", 1))
				sprintf(value, "%d", wan_secondary_pvcunit());
			else
				sprintf(value, "%d", wan_primary_pvcunit());
			tcapi_set("WebCurSet_Entry", "wan_pvc", value);

			if(!strcmp(value, "0"))
				tcapi_set("Wan_Common", "TransMode", "ATM");
			else if(!strcmp(value, "8"))
				tcapi_set("Wan_Common", "TransMode", "PTM");
			else if(!strcmp(value, "10"))
				tcapi_set("Wan_Common", "TransMode", "Ethernet");
			else if(!strcmp(value, "11"))
				tcapi_set("Wan_Common", "TransMode", "USB");
			else if(!strcmp(value, "12"))
				tcapi_set("Wan_Common", "TransMode", "LAN");
		}
	}
#endif
	else if (!strcmp(action_mode, "mfp_monopolize")){
		struct in_addr addr;
		in_addr_t login_ip;

		token = SEARCH_TOKEN_LST_BY_CLITYPE(CLI_TYPE_BROWSER);
		if(!token)
			return;
		login_ip = inet_aton(token->ipaddr, &addr);

		fprintf(stderr, "[%s %d] run action_mode: mfp_monopolize\n", __func__, __LINE__);
		if (login_ip==0x100007f || login_ip==0x0)
			tcapi_set("USB_Entry", "mfp_ip_monopoly", "");
		else
			tcapi_set("USB_Entry", "mfp_ip_monopoly", token->ipaddr);

		int u2ec_fifo = open("/var/u2ec_fifo", O_WRONLY|O_NONBLOCK);
		write(u2ec_fifo, "m", 1);
		close(u2ec_fifo);
	}
#ifdef RTCONFIG_CLOUDSYNC
	else if (!strcmp(action_mode, "get_sharelink"))
	{
		FILE *fp;
		char buf[256] = {0};
		pid_t pid = 0;

                // action_para = websGetVar(wp, "share_link_param", "");
                // if(action_para){
                        // nvram_set("share_link_param", action_para);
                        // nvram_set("share_link_result", "");
                // }

		// action_para = websGetVar(wp, "share_link_host", "");
		// if(action_para){
                        // nvram_set("share_link_host", action_para);
			// nvram_commit();
                // }

		if ((fp = fopen("/tmp/lighttpd/lighttpd.pid", "r")) != NULL) {
			if (fgets(buf, sizeof(buf), fp) != NULL)
			pid = strtoul(buf, NULL, 0);
			fclose(fp);
			if (pid > 1 && kill(pid, SIGUSR2) == 0) {
				printf("[HTTPD] Signaling lighttpd OK!\n");
			}
			else{
				printf("[HTTPD] Signaling lighttpd FAIL!\n");
			}
		}
	}
#endif

	json_object_put(root);
}

/* for ASUS Router (AiHome) APP */
void
json_unescape(char *s)
{
	unsigned int c;

	while ((s = strpbrk(s, "%+"))) {
		/* Parse %xx */
		if (*s == '%') {
			sscanf(s + 1, "%02x", &c);
			*s++ = (char) c;
			strncpy(s, s + 2, strlen(s) + 1);
		}
		/* Space is special */
		else if (*s == '+')
			*s++ = ' ';
	}
}

void
decode_json_buffer(char *query)
{
	int len;
	char *q, *name, *value;

	/* Clear variables */
	if (!query) {
		//hdestroy_r(&htab);
		return;
	}

	/* Parse into individual assignments */
	q = query;
	len = strlen(query);

	for (q = query; q < (query + len);) {
		/* Unescape each assignment */
		json_unescape(name = value = q);

		/* Skip to next assignment */
		for (q += strlen(q); q < (query + len) && !*q; q++);
	}
}

static int appSet_attribute(char *wp, json_object *root)
{
	int app_method_hit = 0, tmpInt = -1;
	char tmp[4096], wan0_node[16], wan1_node[16];

	int pri_unit = wan_primary_ifunit();
	int sec_unit = wan_secondary_ifunit();
	if (pri_unit >= 0) {
		sprintf(wan0_node, "Wan_PVC%d", pri_unit);
	}
	if (sec_unit >= 0) {
		sprintf(wan1_node, "Wan_PVC%d", sec_unit);
	}

	/* AiHome [SET] [AiProtection] Router Security - Secure your router */
	/* not support
	char *wan1_upnp_enable = get_cgi_json(g_var_post, "wan1_upnp_enable", root);
	char *wrs_cc_enable = get_cgi_json(g_var_post, "wrs_cc_enable", root);
	char *wrs_vp_enable = get_cgi_json(g_var_post, "wrs_vp_enable", root);
	char *wrs_mals_enable = get_cgi_json(g_var_post, "wrs_mals_enable", root);
	 */
	char *wan0_upnp_enable = get_cgi_json(g_var_post, "wan0_upnp_enable", root);
	char *misc_http_x = get_cgi_json(g_var_post, "misc_http_x", root);
	char *misc_ping_x = get_cgi_json(g_var_post, "misc_ping_x", root);
	char *dmz_ip = get_cgi_json(g_var_post, "dmz_ip", root);
	char *autofw_enable_x = get_cgi_json(g_var_post, "autofw_enable_x", root);
	char *wps_enable = get_cgi_json(g_var_post, "wps_enable", root);
	char *wps_sta_pin = get_cgi_json(g_var_post, "wps_sta_pin", root);

	if(wan0_upnp_enable) {
		if(strcmp(wan0_upnp_enable, "1") == 0)
			tcapi_set("Upnpd_Entry", "Active", "Yes");
		else
			tcapi_set("Upnpd_Entry", "Active", "No");
	}
	if(misc_http_x)
		tcapi_set("Firewall_Entry", "misc_http_x", misc_http_x);
	if(misc_ping_x)
		tcapi_set("Firewall_Entry", "misc_ping_x", misc_ping_x);
	if(dmz_ip)
		tcapi_set("Dmz_PVC", "DMZ_IP", dmz_ip);
	if(autofw_enable_x)
		tcapi_set("portTriggering_Entry0", "AutoFW_Enable", autofw_enable_x);
	if(wps_enable)
		tcapi_set("WLan_Common", "wps_enable", wps_enable);
	if(wps_sta_pin)
		tcapi_set("WLan_Entry0", "enrolleePinCode", wps_sta_pin);
	if(wan0_upnp_enable || misc_http_x || misc_ping_x || dmz_ip || autofw_enable_x || wps_enable || wps_sta_pin)
		app_method_hit = 1; //"MODIFIED"

	/* DUT_RT_SetHttpWANAccess */
	char *misc_httpport_x = get_cgi_json(g_var_post, "misc_httpport_x", root);
	char *misc_httpsport_x = get_cgi_json(g_var_post, "misc_httpsport_x", root);
	if(misc_httpport_x)
		tcapi_set("Firewall_Entry", "misc_httpport_x", misc_httpport_x);
	if(misc_httpsport_x)
		tcapi_set("Firewall_Entry", "misc_httpsport_x", misc_httpsport_x);
	if(misc_httpport_x || misc_httpsport_x)
		app_method_hit = 1;

	/* [SET] [Client List] Custom Name: <$nickName>$mac>0>$type>> */
	/* not support
	char *custom_clientlist = get_cgi_json(g_var_post, "custom_clientlist", root);
	 */
	/* [SET] [Client List] User Icon: <$mac>$imagebase64 */
	/* not support
	char *custom_usericon = get_cgi_json(g_var_post, "custom_usericon", root);
	if(custom_usericon) {
		_dprintf("http: custom_usericon=%s\n", custom_usericon);
		(void)handle_upload_icon(value);
		nvram_modified = 1;
	} */

	/* [SET] [QoS] game first mode */
	char *qos_enable = get_cgi_json(g_var_post, "qos_enable", root);
	char *qos_type = get_cgi_json(g_var_post, "qos_type", root);
	char *qos_obw = get_cgi_json(g_var_post, "qos_obw", root);
	char *qos_ibw = get_cgi_json(g_var_post, "qos_ibw", root);
	char *ctf_disable = get_cgi_json(g_var_post, "ctf_disable", root);
	/* not support
	char *bwdpi_app_rulelist = get_cgi_json(g_var_post, "bwdpi_app_rulelist", root);
	char *ctf_fa_mode = get_cgi_json(g_var_post, "ctf_fa_mode", root);
	 */
	if(qos_enable)
		tcapi_set("QoS_Entry0", "qos_enable", qos_enable);
	if(qos_type)
		tcapi_set("QoS_Entry0", "qos_type", qos_type);
	if(qos_obw)
		tcapi_set("QoS_Entry0", "qos_obw", qos_obw);
	if(qos_ibw)
		tcapi_set("QoS_Entry0", "qos_ibw", qos_ibw);
	if(ctf_disable)
		tcapi_set("Misc_Entry", "hwnat_enable", ctf_disable);
	if(qos_enable || qos_type || qos_obw || qos_ibw || ctf_disable) {
		app_method_hit = 1;
	}

	/* [SET] [QoS] Bandwidth limiter */
	char *qos_rulelist = get_cgi_json(g_var_post, "qos_rulelist", root);
	char *qos_bw_rulelist = get_cgi_json(g_var_post, "qos_bw_rulelist", root);
	if(qos_rulelist)
		//tcapi_set("QoS_Entry0", "qos_rulelist", qos_rulelist);
		tcapi_set_list("qos_rulelist", qos_rulelist);
	if(qos_bw_rulelist)
		//tcapi_set("QoS_Entry0", "qos_bw_rulelist", qos_bw_rulelist);
		tcapi_set_list("qos_bw_rulelist", qos_bw_rulelist);
	if(qos_rulelist || qos_bw_rulelist) {
		app_method_hit = 1;
	}

	/* [SET] [USB] FTP - Enabled */
	char *enable_ftp = get_cgi_json(g_var_post, "enable_ftp", root);
	char *st_ftp_mode = get_cgi_json(g_var_post, "st_ftp_mode", root);
	if(enable_ftp) {
		if(strcmp(enable_ftp, "1") == 0)
			tcapi_set("Samba_Entry", "enable_ftp", "Yes");
		else
			tcapi_set("Samba_Entry", "enable_ftp", "No");
	}
	if(st_ftp_mode)
		tcapi_set("Samba_Entry", "st_ftp_mode", st_ftp_mode);
	if(enable_ftp || st_ftp_mode)
		app_method_hit = 1;

	/* [SET] [USB] samba - enabled */
	char *enable_samba = get_cgi_json(g_var_post, "enable_samba", root);
	char *st_samba_mode = get_cgi_json(g_var_post, "st_samba_mode", root);
	if(enable_samba) {
		if(strcmp(enable_samba, "1") == 0)
			tcapi_set("Samba_Entry", "Active", "Yes");
		else
			tcapi_set("Samba_Entry", "Active", "No");
	}
	if(st_samba_mode)
		tcapi_set("Samba_Entry", "st_samba_mode", st_samba_mode);
	if(enable_samba || st_samba_mode)
		app_method_hit = 1;

	/* [SET] [Guest Network] 2.4GHz - 1 */
	/* DUT_RT_SetGuestWireless: 2.4GHz */
	char *wl_unit = get_cgi_json(g_var_post, "wl_unit", root);
	char *wl_subunit = get_cgi_json(g_var_post, "wl_subunit", root);
	char *wl0_1_bss_enabled = get_cgi_json(g_var_post, "wl0.1_bss_enabled", root);
	char *wl0_1_ssid = get_cgi_json(g_var_post, "wl0.1_ssid", root);
	char *wl0_1_auth_mode_x = get_cgi_json(g_var_post, "wl0.1_auth_mode_x", root);
	char *wl0_1_crypto = get_cgi_json(g_var_post, "wl0.1_crypto", root);
	char *wl0_1_key = get_cgi_json(g_var_post, "wl0.1_key", root);
	char *wl0_1_wpa_psk = get_cgi_json(g_var_post, "wl0.1_wpa_psk", root);
	char *wl0_1_lanaccess = get_cgi_json(g_var_post, "wl0.1_lanaccess", root);
	char *wl0_1_expire = get_cgi_json(g_var_post, "wl0.1_expire", root);
	char *wl0_1_expire_tmp = get_cgi_json(g_var_post, "wl0.1_expire_tmp", root);
	char *wl0_1_macmode = get_cgi_json(g_var_post, "wl0.1_macmode", root);
	char *wl0_2_bss_enabled = get_cgi_json(g_var_post, "wl0.2_bss_enabled", root);
	char *wl0_2_ssid = get_cgi_json(g_var_post, "wl0.2_ssid", root);
	char *wl0_2_auth_mode_x = get_cgi_json(g_var_post, "wl0.2_auth_mode_x", root);
	char *wl0_2_crypto = get_cgi_json(g_var_post, "wl0.2_crypto", root);
	char *wl0_2_key = get_cgi_json(g_var_post, "wl0.2_key", root);
	char *wl0_2_wpa_psk = get_cgi_json(g_var_post, "wl0.2_wpa_psk", root);
	char *wl0_2_lanaccess = get_cgi_json(g_var_post, "wl0.2_lanaccess", root);
	char *wl0_2_expire = get_cgi_json(g_var_post, "wl0.2_expire", root);
	char *wl0_2_expire_tmp = get_cgi_json(g_var_post, "wl0.2_expire_tmp", root);
	char *wl0_2_macmode = get_cgi_json(g_var_post, "wl0.2_macmode", root);
	char *wl0_3_bss_enabled = get_cgi_json(g_var_post, "wl0.3_bss_enabled", root);
	char *wl0_3_ssid = get_cgi_json(g_var_post, "wl0.3_ssid", root);
	char *wl0_3_auth_mode_x = get_cgi_json(g_var_post, "wl0.3_auth_mode_x", root);
	char *wl0_3_crypto = get_cgi_json(g_var_post, "wl0.3_crypto", root);
	char *wl0_3_key = get_cgi_json(g_var_post, "wl0.3_key", root);
	char *wl0_3_wpa_psk = get_cgi_json(g_var_post, "wl0.3_wpa_psk", root);
	char *wl0_3_lanaccess = get_cgi_json(g_var_post, "wl0.3_lanaccess", root);
	char *wl0_3_expire = get_cgi_json(g_var_post, "wl0.3_expire", root);
	char *wl0_3_expire_tmp = get_cgi_json(g_var_post, "wl0.3_expire_tmp", root);
	char *wl0_3_macmode = get_cgi_json(g_var_post, "wl0.3_macmode", root);
	/* not support
	char *wl0_1_mbss = get_cgi_json(g_var_post, "wl0.1_mbss", root);
	 */
	if(wl_unit)
		tcapi_set("WLan_Common", "wl_unit", wl_unit);
	if(wl_subunit)
		tcapi_set("WLan_Common", "wl_subunit", wl_subunit);
	if(wl0_1_bss_enabled)
		tcapi_set("WLan_Entry0", "wl0.1_bss_enabled", wl0_1_bss_enabled);
	if(wl0_1_ssid)
		tcapi_set("WLan_Entry0", "wl0.1_ssid", wl0_1_ssid);
	if(wl0_1_auth_mode_x) {
		if(strcmp(wl0_1_auth_mode_x, "open") ==0)
			tcapi_set("WLan_Entry0", "wl0.1_auth_mode_x", "OPEN");
		else if(strcmp(wl0_1_auth_mode_x, "psk2") ==0)
			tcapi_set("WLan_Entry0", "wl0.1_auth_mode_x", "WPA2PSK");
		else if(strcmp(wl0_1_auth_mode_x, "pskpsk2") ==0)
			tcapi_set("WLan_Entry0", "wl0.1_auth_mode_x", "WPAPSKWPA2PSK");
		else if(strcmp(wl0_1_auth_mode_x, "wpa2") ==0)
			tcapi_set("WLan_Entry0", "wl0.1_auth_mode_x", "WPA2");
		else if(strcmp(wl0_1_auth_mode_x, "wpawpa2") ==0)
			tcapi_set("WLan_Entry0", "wl0.1_auth_mode_x", "WPA1WPA2");
	}
	if(wl0_1_crypto) {
		if(tcapi_match("WLan_Entry0", "wl0.1_auth_mode_x", "OPEN"))
			tcapi_set("WLan_Entry0", "wl0.1_crypto", "NONE");
		else if(strcmp(wl0_1_crypto, "aes") ==0)
			tcapi_set("WLan_Entry0", "wl0.1_crypto", "AES");
		else if(strcmp(wl0_1_crypto, "tkip+aes") ==0)
			tcapi_set("WLan_Entry0", "wl0.1_crypto", "TKIP+AES");
	}
	if(wl0_1_key)
		tcapi_set("WLan_Entry0", "wl0.1_key", wl0_1_key);
	if(wl0_1_wpa_psk)
		tcapi_set("WLan_Entry0", "wl0.1_wpa_psk", wl0_1_wpa_psk);
	if(wl0_1_lanaccess)
		tcapi_set("WLan_Entry0", "wl0.1_lanaccess", wl0_1_lanaccess);
	if(wl0_1_expire)
		tcapi_set("WLan_Entry0", "wl0.1_expire", wl0_1_expire);
	if(wl0_1_expire_tmp)
		tcapi_set("WLan_Entry0", "wl0.1_expire_tmp", wl0_1_expire_tmp);
	if(wl0_1_macmode)
		tcapi_set("ACL_Entry0", "wl0.1_wl_macmode", wl0_1_macmode);
	if(wl0_2_bss_enabled)
		tcapi_set("WLan_Entry0", "wl0.2_bss_enabled", wl0_2_bss_enabled);
	if(wl0_2_ssid)
		tcapi_set("WLan_Entry0", "wl0.2_ssid", wl0_2_ssid);
	if(wl0_2_auth_mode_x) {
		if(strcmp(wl0_2_auth_mode_x, "open") ==0)
			tcapi_set("WLan_Entry0", "wl0.2_auth_mode_x", "OPEN");
		else if(strcmp(wl0_2_auth_mode_x, "psk2") ==0)
			tcapi_set("WLan_Entry0", "wl0.2_auth_mode_x", "WPA2PSK");
		else if(strcmp(wl0_2_auth_mode_x, "pskpsk2") ==0)
			tcapi_set("WLan_Entry0", "wl0.2_auth_mode_x", "WPAPSKWPA2PSK");
		else if(strcmp(wl0_2_auth_mode_x, "wpa2") ==0)
			tcapi_set("WLan_Entry0", "wl0.2_auth_mode_x", "WPA2");
		else if(strcmp(wl0_2_auth_mode_x, "wpawpa2") ==0)
			tcapi_set("WLan_Entry0", "wl0.2_auth_mode_x", "WPA1WPA2");
	}
	if(wl0_2_crypto) {
		if(tcapi_match("WLan_Entry0", "wl0.2_auth_mode_x", "OPEN"))
			tcapi_set("WLan_Entry0", "wl0.2_crypto", "NONE");
		else if(strcmp(wl0_2_crypto, "aes") ==0)
			tcapi_set("WLan_Entry0", "wl0.2_crypto", "AES");
		else if(strcmp(wl0_2_crypto, "tkip+aes") ==0)
			tcapi_set("WLan_Entry0", "wl0.2_crypto", "TKIP+AES");
	}
	if(wl0_2_key)
		tcapi_set("WLan_Entry0", "wl0.2_key", wl0_2_key);
	if(wl0_2_wpa_psk)
		tcapi_set("WLan_Entry0", "wl0.2_wpa_psk", wl0_2_wpa_psk);
	if(wl0_2_lanaccess)
		tcapi_set("WLan_Entry0", "wl0.2_lanaccess", wl0_2_lanaccess);
	if(wl0_2_expire)
		tcapi_set("WLan_Entry0", "wl0.2_expire", wl0_2_expire);
	if(wl0_2_expire_tmp)
		tcapi_set("WLan_Entry0", "wl0.2_expire_tmp", wl0_2_expire_tmp);
	if(wl0_2_macmode)
		tcapi_set("ACL_Entry0", "wl0.2_wl_macmode", wl0_2_macmode);
	if(wl0_3_bss_enabled)
		tcapi_set("WLan_Entry0", "wl0.3_bss_enabled", wl0_3_bss_enabled);
	if(wl0_3_ssid)
		tcapi_set("WLan_Entry0", "wl0.3_ssid", wl0_3_ssid);
	if(wl0_3_auth_mode_x) {
		if(strcmp(wl0_3_auth_mode_x, "open") ==0)
			tcapi_set("WLan_Entry0", "wl0.3_auth_mode_x", "OPEN");
		else if(strcmp(wl0_3_auth_mode_x, "psk2") ==0)
			tcapi_set("WLan_Entry0", "wl0.3_auth_mode_x", "WPA2PSK");
		else if(strcmp(wl0_3_auth_mode_x, "pskpsk2") ==0)
			tcapi_set("WLan_Entry0", "wl0.3_auth_mode_x", "WPAPSKWPA2PSK");
		else if(strcmp(wl0_3_auth_mode_x, "wpa2") ==0)
			tcapi_set("WLan_Entry0", "wl0.3_auth_mode_x", "WPA2");
		else if(strcmp(wl0_3_auth_mode_x, "wpawpa2") ==0)
			tcapi_set("WLan_Entry0", "wl0.3_auth_mode_x", "WPA1WPA2");
	}
	if(wl0_3_crypto) {
		if(tcapi_match("WLan_Entry0", "wl0.3_auth_mode_x", "OPEN"))
			tcapi_set("WLan_Entry0", "wl0.3_crypto", "NONE");
		else if(strcmp(wl0_3_crypto, "aes") ==0)
			tcapi_set("WLan_Entry0", "wl0.3_crypto", "AES");
		else if(strcmp(wl0_3_crypto, "tkip+aes") ==0)
			tcapi_set("WLan_Entry0", "wl0.3_crypto", "TKIP+AES");
	}
	if(wl0_3_key)
		tcapi_set("WLan_Entry0", "wl0.3_key", wl0_3_key);
	if(wl0_3_wpa_psk)
		tcapi_set("WLan_Entry0", "wl0.3_wpa_psk", wl0_3_wpa_psk);
	if(wl0_3_lanaccess)
		tcapi_set("WLan_Entry0", "wl0.3_lanaccess", wl0_3_lanaccess);
	if(wl0_3_expire)
		tcapi_set("WLan_Entry0", "wl0.3_expire", wl0_3_expire);
	if(wl0_3_expire_tmp)
		tcapi_set("WLan_Entry0", "wl0.3_expire_tmp", wl0_3_expire_tmp);
	if(wl0_3_macmode)
		tcapi_set("ACL_Entry0", "wl0.3_wl_macmode", wl0_3_macmode);
	if(wl_unit || wl_subunit
		|| wl0_1_bss_enabled || wl0_1_ssid || wl0_1_auth_mode_x || wl0_1_crypto || wl0_1_key || wl0_1_wpa_psk || wl0_1_lanaccess || wl0_1_expire || wl0_1_expire_tmp || wl0_1_macmode
		|| wl0_2_bss_enabled || wl0_2_ssid || wl0_2_auth_mode_x || wl0_2_crypto || wl0_2_key || wl0_2_wpa_psk || wl0_2_lanaccess || wl0_2_expire || wl0_2_expire_tmp || wl0_2_macmode
		|| wl0_3_bss_enabled || wl0_3_ssid || wl0_3_auth_mode_x || wl0_3_crypto || wl0_3_key || wl0_3_wpa_psk || wl0_3_lanaccess || wl0_3_expire || wl0_3_expire_tmp || wl0_3_macmode)
		app_method_hit = 1;

	/* [SET] [Guest Network] 5GHz - 1 */
	/* DUT_RT_SetGuestWireless: 5GHz */
	char *wl1_1_bss_enabled = get_cgi_json(g_var_post, "wl1.1_bss_enabled", root);
	char *wl1_1_ssid = get_cgi_json(g_var_post, "wl1.1_ssid", root);
	char *wl1_1_auth_mode_x = get_cgi_json(g_var_post, "wl1.1_auth_mode_x", root);
	char *wl1_1_crypto = get_cgi_json(g_var_post, "wl1.1_crypto", root);
	char *wl1_1_key = get_cgi_json(g_var_post, "wl1.1_key", root);
	char *wl1_1_wpa_psk = get_cgi_json(g_var_post, "wl1.1_wpa_psk", root);
	char *wl1_1_lanaccess = get_cgi_json(g_var_post, "wl1.1_lanaccess", root);
	char *wl1_1_expire = get_cgi_json(g_var_post, "wl1.1_expire", root);
	char *wl1_1_expire_tmp = get_cgi_json(g_var_post, "wl1.1_expire_tmp", root);
	char *wl1_1_macmode = get_cgi_json(g_var_post, "wl1.1_macmode", root);
	char *wl1_2_bss_enabled = get_cgi_json(g_var_post, "wl1.2_bss_enabled", root);
	char *wl1_2_ssid = get_cgi_json(g_var_post, "wl1.2_ssid", root);
	char *wl1_2_auth_mode_x = get_cgi_json(g_var_post, "wl1.2_auth_mode_x", root);
	char *wl1_2_crypto = get_cgi_json(g_var_post, "wl1.2_crypto", root);
	char *wl1_2_key = get_cgi_json(g_var_post, "wl1.2_key", root);
	char *wl1_2_wpa_psk = get_cgi_json(g_var_post, "wl1.2_wpa_psk", root);
	char *wl1_2_lanaccess = get_cgi_json(g_var_post, "wl1.2_lanaccess", root);
	char *wl1_2_expire = get_cgi_json(g_var_post, "wl1.2_expire", root);
	char *wl1_2_expire_tmp = get_cgi_json(g_var_post, "wl1.2_expire_tmp", root);
	char *wl1_2_macmode = get_cgi_json(g_var_post, "wl1.2_macmode", root);
	char *wl1_3_bss_enabled = get_cgi_json(g_var_post, "wl1.3_bss_enabled", root);
	char *wl1_3_ssid = get_cgi_json(g_var_post, "wl1.3_ssid", root);
	char *wl1_3_auth_mode_x = get_cgi_json(g_var_post, "wl1.3_auth_mode_x", root);
	char *wl1_3_crypto = get_cgi_json(g_var_post, "wl1.3_crypto", root);
	char *wl1_3_key = get_cgi_json(g_var_post, "wl1.3_key", root);
	char *wl1_3_wpa_psk = get_cgi_json(g_var_post, "wl1.3_wpa_psk", root);
	char *wl1_3_lanaccess = get_cgi_json(g_var_post, "wl1.3_lanaccess", root);
	char *wl1_3_expire = get_cgi_json(g_var_post, "wl1.3_expire", root);
	char *wl1_3_expire_tmp = get_cgi_json(g_var_post, "wl1.3_expire_tmp", root);
	char *wl1_3_macmode = get_cgi_json(g_var_post, "wl1.3_macmode", root);
	/* not support
	char *wl1_1_macmode = get_cgi_json(g_var_post, "wl1.1_macmode", root);
	char *wl1_1_mbss = get_cgi_json(g_var_post, "wl1.1_mbss", root);
	 */
	if(wl1_1_bss_enabled)
		tcapi_set("WLan_Entry0", "wl1.1_bss_enabled", wl1_1_bss_enabled);
	if(wl1_1_ssid)
		tcapi_set("WLan_Entry0", "wl1.1_ssid", wl1_1_ssid);
	if(wl1_1_auth_mode_x) {
		if(strcmp(wl1_1_auth_mode_x, "open") ==0)
			tcapi_set("WLan_Entry0", "wl1.1_auth_mode_x", "OPEN");
		else if(strcmp(wl1_1_auth_mode_x, "psk2") ==0)
			tcapi_set("WLan_Entry0", "wl1.1_auth_mode_x", "WPA2PSK");
		else if(strcmp(wl1_1_auth_mode_x, "pskpsk2") ==0)
			tcapi_set("WLan_Entry0", "wl1.1_auth_mode_x", "WPAPSKWPA2PSK");
		else if(strcmp(wl1_1_auth_mode_x, "wpa2") ==0)
			tcapi_set("WLan_Entry0", "wl1.1_auth_mode_x", "WPA2");
		else if(strcmp(wl1_1_auth_mode_x, "wpawpa2") ==0)
			tcapi_set("WLan_Entry0", "wl1.1_auth_mode_x", "WPA1WPA2");
	}
	if(wl1_1_crypto) {
		if(tcapi_match("WLan_Entry0", "wl1.1_auth_mode_x", "OPEN"))
			tcapi_set("WLan_Entry0", "wl1.1_crypto", "NONE");
		else if(strcmp(wl1_1_crypto, "aes") ==0)
			tcapi_set("WLan_Entry0", "wl1.1_crypto", "AES");
		else if(strcmp(wl1_1_crypto, "tkip+aes") ==0)
			tcapi_set("WLan_Entry0", "wl1.1_crypto", "TKIP+AES");
	}
	if(wl1_1_key)
		tcapi_set("WLan_Entry0", "wl1.1_key", wl1_1_key);
	if(wl1_1_wpa_psk)
		tcapi_set("WLan_Entry0", "wl1.1_wpa_psk", wl1_1_wpa_psk);
	if(wl1_1_lanaccess)
		tcapi_set("WLan_Entry0", "wl1.1_lanaccess", wl1_1_lanaccess);
	if(wl1_1_expire)
		tcapi_set("WLan_Entry0", "wl1.1_expire", wl1_1_expire);
	if(wl1_1_expire_tmp)
		tcapi_set("WLan_Entry0", "wl1.1_expire_tmp", wl1_1_expire_tmp);
	if(wl1_1_macmode)
		tcapi_set("ACL_Entry0", "wl1.1_wl_macmode", wl1_1_macmode);
	if(wl1_2_bss_enabled)
		tcapi_set("WLan_Entry0", "wl1.2_bss_enabled", wl1_2_bss_enabled);
	if(wl1_2_ssid)
		tcapi_set("WLan_Entry0", "wl1.2_ssid", wl1_2_ssid);
	if(wl1_2_auth_mode_x) {
		if(strcmp(wl1_2_auth_mode_x, "open") ==0)
			tcapi_set("WLan_Entry0", "wl1.2_auth_mode_x", "OPEN");
		else if(strcmp(wl1_2_auth_mode_x, "psk2") ==0)
			tcapi_set("WLan_Entry0", "wl1.2_auth_mode_x", "WPA2PSK");
		else if(strcmp(wl1_2_auth_mode_x, "pskpsk2") ==0)
			tcapi_set("WLan_Entry0", "wl1.2_auth_mode_x", "WPAPSKWPA2PSK");
		else if(strcmp(wl1_2_auth_mode_x, "wpa2") ==0)
			tcapi_set("WLan_Entry0", "wl1.2_auth_mode_x", "WPA2");
		else if(strcmp(wl1_2_auth_mode_x, "wpawpa2") ==0)
			tcapi_set("WLan_Entry0", "wl1.2_auth_mode_x", "WPA1WPA2");
	}
	if(wl1_2_crypto) {
		if(tcapi_match("WLan_Entry0", "wl1.2_auth_mode_x", "OPEN"))
			tcapi_set("WLan_Entry0", "wl1.2_crypto", "NONE");
		else if(strcmp(wl1_2_crypto, "aes") ==0)
			tcapi_set("WLan_Entry0", "wl1.2_crypto", "AES");
		else if(strcmp(wl1_2_crypto, "tkip+aes") ==0)
			tcapi_set("WLan_Entry0", "wl1.2_crypto", "TKIP+AES");
	}
	if(wl1_2_key)
		tcapi_set("WLan_Entry0", "wl1.2_key", wl1_2_key);
	if(wl1_2_wpa_psk)
		tcapi_set("WLan_Entry0", "wl1.2_wpa_psk", wl1_2_wpa_psk);
	if(wl1_2_lanaccess)
		tcapi_set("WLan_Entry0", "wl1.2_lanaccess", wl1_2_lanaccess);
	if(wl1_2_expire)
		tcapi_set("WLan_Entry0", "wl1.2_expire", wl1_2_expire);
	if(wl1_2_expire_tmp)
		tcapi_set("WLan_Entry0", "wl1.2_expire_tmp", wl1_2_expire_tmp);
	if(wl1_2_macmode)
		tcapi_set("ACL_Entry0", "wl1.2_wl_macmode", wl1_2_macmode);
	if(wl1_3_bss_enabled)
		tcapi_set("WLan_Entry0", "wl1.3_bss_enabled", wl1_3_bss_enabled);
	if(wl1_3_ssid)
		tcapi_set("WLan_Entry0", "wl1.3_ssid", wl1_3_ssid);
	if(wl1_3_auth_mode_x) {
		if(strcmp(wl1_3_auth_mode_x, "open") ==0)
			tcapi_set("WLan_Entry0", "wl1.3_auth_mode_x", "OPEN");
		else if(strcmp(wl1_3_auth_mode_x, "psk2") ==0)
			tcapi_set("WLan_Entry0", "wl1.3_auth_mode_x", "WPA2PSK");
		else if(strcmp(wl1_3_auth_mode_x, "pskpsk2") ==0)
			tcapi_set("WLan_Entry0", "wl1.3_auth_mode_x", "WPAPSKWPA2PSK");
		else if(strcmp(wl1_3_auth_mode_x, "wpa2") ==0)
			tcapi_set("WLan_Entry0", "wl1.3_auth_mode_x", "WPA2");
		else if(strcmp(wl1_3_auth_mode_x, "wpawpa2") ==0)
			tcapi_set("WLan_Entry0", "wl1.3_auth_mode_x", "WPA1WPA2");
	}
	if(wl1_3_crypto) {
		if(tcapi_match("WLan_Entry0", "wl1.3_auth_mode_x", "OPEN"))
			tcapi_set("WLan_Entry0", "wl1.3_crypto", "NONE");
		else if(strcmp(wl1_3_crypto, "aes") ==0)
			tcapi_set("WLan_Entry0", "wl1.3_crypto", "AES");
		else if(strcmp(wl1_3_crypto, "tkip+aes") ==0)
			tcapi_set("WLan_Entry0", "wl1.3_crypto", "TKIP+AES");
	}
	if(wl1_3_key)
		tcapi_set("WLan_Entry0", "wl1.3_key", wl1_3_key);
	if(wl1_3_wpa_psk)
		tcapi_set("WLan_Entry0", "wl1.3_wpa_psk", wl1_3_wpa_psk);
	if(wl1_3_lanaccess)
		tcapi_set("WLan_Entry0", "wl1.3_lanaccess", wl1_3_lanaccess);
	if(wl1_3_expire)
		tcapi_set("WLan_Entry0", "wl1.3_expire", wl1_3_expire);
	if(wl1_3_expire_tmp)
		tcapi_set("WLan_Entry0", "wl1.3_expire_tmp", wl1_3_expire_tmp);
	if(wl1_3_macmode)
		tcapi_set("ACL_Entry0", "wl1.3_wl_macmode", wl1_3_macmode);
	if(wl1_1_bss_enabled || wl1_1_ssid || wl1_1_auth_mode_x || wl1_1_crypto || wl1_1_key || wl1_1_wpa_psk || wl1_1_lanaccess || wl1_1_expire || wl1_1_expire_tmp || wl1_1_macmode
		|| wl1_2_bss_enabled || wl1_2_ssid || wl1_2_auth_mode_x || wl1_2_crypto || wl1_2_key || wl1_2_wpa_psk || wl1_2_lanaccess || wl1_2_expire || wl1_2_expire_tmp || wl1_2_macmode
		|| wl1_3_bss_enabled || wl1_3_ssid || wl1_3_auth_mode_x || wl1_3_crypto || wl1_3_key || wl1_3_wpa_psk || wl1_3_lanaccess || wl1_3_expire || wl1_3_expire_tmp || wl1_3_macmode)
		app_method_hit = 1;

	/* [SET] [Wireless] General Setting */
	char *wl_closed = get_cgi_json(g_var_post, "wl_closed", root);
	char *wl_radio = get_cgi_json(g_var_post, "wl_radio", root);
	char *wl_ssid = get_cgi_json(g_var_post, "wl_ssid", root);
	char *wl_nmode_x = get_cgi_json(g_var_post, "wl_nmode_x", root);
	char *wl_bw = get_cgi_json(g_var_post, "wl_bw", root);
	/* not support
	char *acs_dfs = get_cgi_json(g_var_post, "acs_dfs", root);
	char *wl_chanspec = get_cgi_json(g_var_post, "wl_chanspec", root);
	 */
	if(wl_closed)
		tcapi_set("WLan_Entry0","HideSSID",wl_closed);
	if(wl_radio)
		tcapi_set("WLan_Entry0","radio_on",wl_radio);
	if(wl_ssid)
		tcapi_set("WLan_Entry0","ssid",wl_ssid);
	if(wl_nmode_x)
		tcapi_set("WLan_Common","WirelessMode",wl_nmode_x);
	if(wl_bw)
		tcapi_set("WLan_Common","HT_BW",wl_bw);
	if(wl_closed || wl_radio || wl_ssid || wl_nmode_x || wl_bw)
		app_method_hit = 1;

	/* DUT_RT_SetWirelessConfig: 2.4GHz */
	char *wl0_ssid = get_cgi_json(g_var_post, "wl0_ssid", root);
	char *wl0_closed = get_cgi_json(g_var_post, "wl0_closed", root);
	char *wl0_nmode_x = get_cgi_json(g_var_post, "wl0_nmode_x", root);
	char *wl0_bw = get_cgi_json(g_var_post, "wl0_bw", root);
	char *wl0_auth_mode_x = get_cgi_json(g_var_post, "wl0_auth_mode_x", root);
	char *wl0_crypto = get_cgi_json(g_var_post, "wl0_crypto", root);
	char *wl0_wpa_psk = get_cgi_json(g_var_post, "wl0_wpa_psk", root);
	if(wl0_ssid)
		tcapi_set("WLan_Entry0", "wl0_ssid", wl0_ssid);
	if(wl0_closed)
		tcapi_set("WLan_Entry0", "wl0_HideSSID", wl0_closed);
	if(wl0_nmode_x) { //APP=> 2:Legacy, 1:N only, 0:Auto
		tmpInt = atoi(wl0_nmode_x);
		switch (tmpInt) {
			case 2:
				tcapi_set("WLan_Common", "wl0_WirelessMode", "0"); /*2G=>0: Legacy*/
				break;
			case 1:
				tcapi_set("WLan_Common", "wl0_WirelessMode", "6"); /*2G=>6: N Only*/
				break;
			case 0:
				tcapi_set("WLan_Common", "wl0_WirelessMode", "9"); /*2G=>9: Auto*/
				break;
		}
	}
	if(wl0_bw)
		tcapi_set("WLan_Common", "wl0_HT_BW", wl0_bw);
	if(wl0_auth_mode_x) {
		if(strcmp(wl0_auth_mode_x, "open") ==0)
			tcapi_set("WLan_Entry0", "wl0_auth_mode_x", "OPEN");
		else if(strcmp(wl0_auth_mode_x, "psk2") ==0)
			tcapi_set("WLan_Entry0", "wl0_auth_mode_x", "WPA2PSK");
		else if(strcmp(wl0_auth_mode_x, "pskpsk2") ==0)
			tcapi_set("WLan_Entry0", "wl0_auth_mode_x", "WPAPSKWPA2PSK");
		else if(strcmp(wl0_auth_mode_x, "wpa2") ==0)
			tcapi_set("WLan_Entry0", "wl0_auth_mode_x", "WPA2");
		else if(strcmp(wl0_auth_mode_x, "wpawpa2") ==0)
			tcapi_set("WLan_Entry0", "wl0_auth_mode_x", "WPA1WPA2");
	}
	if(wl0_crypto) {
		if(tcapi_match("WLan_Entry0", "wl0_auth_mode_x", "OPEN"))
			tcapi_set("WLan_Entry0", "wl0_crypto", "NONE");
		else if(strcmp(wl0_crypto, "aes") ==0)
			tcapi_set("WLan_Entry0", "wl0_crypto", "AES");
		else if(strcmp(wl0_crypto, "tkip+aes") ==0)
			tcapi_set("WLan_Entry0", "wl0_crypto", "TKIP+AES");
	}
	if(wl0_wpa_psk)
		tcapi_set("WLan_Entry0", "wl0_wpa_psk", wl0_wpa_psk);
	if(wl0_ssid || wl0_closed || wl0_nmode_x || wl0_bw || wl0_auth_mode_x || wl0_crypto || wl0_wpa_psk)
		app_method_hit = 1;

	/* DUT_RT_SetWirelessConfig: 5GHz */
	char *wl1_ssid = get_cgi_json(g_var_post, "wl1_ssid", root);
	char *wl1_closed = get_cgi_json(g_var_post, "wl1_closed", root);
	char *wl1_nmode_x = get_cgi_json(g_var_post, "wl1_nmode_x", root);
	char *wl1_bw = get_cgi_json(g_var_post, "wl1_bw", root);
	char *wl1_auth_mode_x = get_cgi_json(g_var_post, "wl1_auth_mode_x", root);
	char *wl1_crypto = get_cgi_json(g_var_post, "wl1_crypto", root);
	char *wl1_wpa_psk = get_cgi_json(g_var_post, "wl1_wpa_psk", root);
	if(wl1_ssid)
		tcapi_set("WLan_Entry0", "wl1_ssid", wl1_ssid);
	if(wl1_closed)
		tcapi_set("WLan_Entry0", "wl1_HideSSID", wl1_closed);
	if(wl1_nmode_x) { //APP=>2:Legacy, 1:N only, 0:Auto
		tmpInt = atoi(wl1_nmode_x);
		switch (tmpInt) {
			case 2:
				tcapi_set("WLan_Common", "wl1_WirelessMode", "2"); /*5G=>2: Legacy*/
				break;
			case 1:
				tcapi_set("WLan_Common", "wl1_WirelessMode", "11"); /*5G=>11: N Only*/
				break;
			case 0:
				tcapi_set("WLan_Common", "wl1_WirelessMode", "8"); /*5G=>8: Auto*/
				break;
		}
	}
	if(wl1_bw)
		tcapi_set("WLan_Common", "wl1_HT_BW", wl1_bw);
	if(wl1_auth_mode_x) {
		if(strcmp(wl1_auth_mode_x, "open") ==0)
			tcapi_set("WLan_Entry0", "wl1_auth_mode_x", "OPEN");
		else if(strcmp(wl1_auth_mode_x, "psk2") ==0)
			tcapi_set("WLan_Entry0", "wl1_auth_mode_x", "WPA2PSK");
		else if(strcmp(wl1_auth_mode_x, "pskpsk2") ==0)
			tcapi_set("WLan_Entry0", "wl1_auth_mode_x", "WPAPSKWPA2PSK");
		else if(strcmp(wl1_auth_mode_x, "wpa2") ==0)
			tcapi_set("WLan_Entry0", "wl1_auth_mode_x", "WPA2");
		else if(strcmp(wl1_auth_mode_x, "wpawpa2") ==0)
			tcapi_set("WLan_Entry0", "wl1_auth_mode_x", "WPA1WPA2");
	}
	if(wl1_crypto) {
		if(tcapi_match("WLan_Entry0", "wl1_auth_mode_x", "OPEN"))
			tcapi_set("WLan_Entry0", "wl1_crypto", "NONE");
		else if(strcmp(wl1_crypto, "aes") ==0)
			tcapi_set("WLan_Entry0", "wl1_crypto", "AES");
		else if(strcmp(wl1_crypto, "tkip+aes") ==0)
			tcapi_set("WLan_Entry0", "wl1_crypto", "TKIP+AES");
	}
	if(wl1_wpa_psk)
		tcapi_set("WLan_Entry0", "wl1_wpa_psk", wl1_wpa_psk);
	if(wl1_ssid || wl1_closed || wl1_nmode_x || wl1_bw || wl1_auth_mode_x || wl1_crypto || wl1_wpa_psk)
		app_method_hit = 1;

	/* [SET] [Wireless] Mac filter */
	char *wl0_macmode = get_cgi_json(g_var_post, "wl0_macmode", root);
	char *wl1_macmode = get_cgi_json(g_var_post, "wl1_macmode", root);
	char *wl2_macmode = get_cgi_json(g_var_post, "wl2_macmode", root);
	char *wl0_maclist_x = get_cgi_json(g_var_post, "wl0_maclist_x", root);
	char *wl1_maclist_x = get_cgi_json(g_var_post, "wl1_maclist_x", root);
	char *wl2_maclist_x = get_cgi_json(g_var_post, "wl2_maclist_x", root);
	if(wl0_macmode)
		tcapi_set("ACL_Entry0", "wl0_wl_macmode",wl0_macmode);
	if(wl1_macmode)
		tcapi_set("ACL_Entry0", "wl1_wl_macmode",wl1_macmode);
	if(wl2_macmode)
		tcapi_set("ACL_Entry0", "wl2_wl_macmode",wl2_macmode);
	if(wl0_maclist_x)
		tcapi_set("ACL_Entry0","wl0_wl_maclist",wl0_maclist_x);
	if(wl1_maclist_x)
		tcapi_set("ACL_Entry0","wl1_wl_maclist",wl1_maclist_x);
	if(wl2_maclist_x)
		tcapi_set("ACL_Entry0","wl2_wl_maclist",wl2_maclist_x);
	if(wl0_macmode || wl1_macmode || wl2_macmode || wl0_maclist_x || wl1_maclist_x || wl2_maclist_x)
		app_method_hit = 1;

	/* [SET] [Wireless] radio on/off */
	char *wl0_radio = get_cgi_json(g_var_post, "wl0_radio", root);
	char *wl1_radio = get_cgi_json(g_var_post, "wl1_radio", root);
	if(wl0_radio)
		tcapi_set("WLan_Entry0", "wl0_radio_on", wl0_radio);
	if(wl1_radio)
		tcapi_set("WLan_Entry0", "wl1_radio_on", wl1_radio);
	if(wl0_radio || wl1_radio)
		app_method_hit = 1;

	/* [SET] [Wireless] Roaming assistant */
	char *wl0_user_rssi = get_cgi_json(g_var_post, "wl0_user_rssi", root);
	char *wl1_user_rssi = get_cgi_json(g_var_post, "wl1_user_rssi", root);
	char *wl2_user_rssi = get_cgi_json(g_var_post, "wl2_user_rssi", root);
	if(wl0_user_rssi)
		tcapi_set("WLan_Entry0", "wl0_user_rssi", wl0_user_rssi);
	if(wl1_user_rssi)
		tcapi_set("WLan_Entry0", "wl1_user_rssi", wl1_user_rssi);
	if(wl2_user_rssi)
		tcapi_set("WLan_Entry0", "wl2_user_rssi", wl2_user_rssi);
	if(wl0_user_rssi || wl1_user_rssi || wl2_user_rssi)
		app_method_hit = 1;

	/* [SET] [Wireless] Tx power adjustment */
	char *wl0_txpower = get_cgi_json(g_var_post, "wl0_txpower", root);
	char *wl1_txpower = get_cgi_json(g_var_post, "wl1_txpower", root);
	if(wl0_txpower)
		tcapi_set("WLan_Common", "wl0_TxPower", wl0_txpower);
	if(wl1_txpower)
		tcapi_set("WLan_Common", "wl1_TxPower", wl1_txpower);
	if(wl0_txpower || wl1_txpower)
		app_method_hit = 1;

	/* [SET] [CloudCheck] Enabled */
	/* not support
	char *enable_cloudcheck = get_cgi_json(g_var_post, "enable_cloudcheck", root);
	 */

	/* [SET] [Change Mode] Repeater mode setting */
	/* not support (only support SW_MODE_ROUTER mode) */

	/* [SET] [USB] samba - NTFS Sparse Files Support */
	/* not support
	char *usb_fs_ntfs_sparse = get_cgi_json(g_var_post, "usb_fs_ntfs_sparse", root);
	 */

	/* [SET] [System] HTTP / HTTPS */
	char *http_enable = get_cgi_json(g_var_post, "http_enable", root);
	char *https_lanport = get_cgi_json(g_var_post, "https_lanport", root);
	if(http_enable) {
		/**** in DSL model ****|***** in RT model ****
		http_enable            | http_enable
		1:HTTP                 | 0:HTTP
		2:HTTPS                | 1:HTTPS
		3:Both HTTP and HTTPS  | 2:Both HTTP and HTTPS
		***********************|**********************/
		tmpInt = atoi(http_enable);
		switch (tmpInt) {
			case 0:
				tcapi_set("Https_Entry", "http_enable", "1");
				break;
			case 1:
				tcapi_set("Https_Entry", "http_enable", "2");
				break;
			case 2:
				tcapi_set("Https_Entry", "http_enable", "3");
				break;
		}
	}
	if(https_lanport)
		tcapi_set("Https_Entry", "https_lanport", https_lanport);
	if(http_enable || https_lanport)
		app_method_hit = 1;

	/* [SET] [WAN] ASUS DDNS */
	char *ddns_enable_x = get_cgi_json(g_var_post, "ddns_enable_x", root);
	char *ddns_server_x = get_cgi_json(g_var_post, "ddns_server_x", root);
	char *ddns_hostname_x = get_cgi_json(g_var_post, "ddns_hostname_x", root);
	if(ddns_enable_x)
		tcapi_set("Ddns_Entry", "Active", ddns_enable_x);
	if(ddns_server_x)
		tcapi_set("Ddns_Entry", "SERVERNAME", ddns_server_x);
	if(ddns_hostname_x){
		tcapi_set("Ddns_Entry", "MYHOST", ddns_hostname_x);
	}
	if(ddns_enable_x || ddns_server_x || ddns_hostname_x)
		app_method_hit = 1;

	/* [SET] [WAN] Disconnect WAN */
	char *wan0_enable = get_cgi_json(g_var_post, "wan0_enable", root);
	char *wan1_enable = get_cgi_json(g_var_post, "wan1_enable", root);
	if((wan0_enable) && (pri_unit >= 0)) {
		tcapi_set(wan0_node, "Active", strcmp(wan0_enable, "1") ? "No" : "Yes");
		app_method_hit = 1;
	}
	if((wan1_enable) && (sec_unit >= 0)) {
		tcapi_set(wan1_node, "Active", strcmp(wan1_enable, "1") ? "No" : "Yes");
		app_method_hit = 1;
	}

	/* [SET] [WAN] PPTP VPN server */
	char *VPNServer_enable = get_cgi_json(g_var_post, "VPNServer_enable", root);
	char *pptpd_enable = get_cgi_json(g_var_post, "pptpd_enable", root);
	char *VPNServer_mode = get_cgi_json(g_var_post, "VPNServer_mode", root);
	char *pptpd_clientlist = get_cgi_json(g_var_post, "pptpd_clientlist", root);
	if(VPNServer_enable)
		tcapi_set("VPNControl_Entry", "VPNServer_enable", VPNServer_enable);
	if(pptpd_enable)
		tcapi_set("PPTP_Entry", "pptpd_enable", pptpd_enable);
	if(VPNServer_mode)
		tcapi_set("VPNControl_Entry", "VPNServer_mode", VPNServer_mode);
	if(pptpd_clientlist)
		//tcapi_set("PPTP_Entry", "pptpd_clientlist", pptpd_clientlist);
		tcapi_set_list("pptpd_clientlist", pptpd_clientlist);
	if(VPNServer_enable || pptpd_enable || VPNServer_mode || pptpd_clientlist)
		app_method_hit = 1;

	/* [SET] [Wireless] RF 4x4 to 1x1 */
	/* not support now */
	char *guard_mode = get_cgi_json(g_var_post, "guard_mode", root);
	if(guard_mode)
		tcapi_set("WLan_Common", "guard_mode", guard_mode);

	/* [SET] [QoS] Parental Control Rule */
	char *MULTIFILTER_ALL = get_cgi_json(g_var_post, "MULTIFILTER_ALL", root);
	char *MULTIFILTER_ENABLE = get_cgi_json(g_var_post, "MULTIFILTER_ENABLE", root);
	char *MULTIFILTER_MAC = get_cgi_json(g_var_post, "MULTIFILTER_MAC", root);
	char *MULTIFILTER_DEVICENAME = get_cgi_json(g_var_post, "MULTIFILTER_DEVICENAME", root);
	char *MULTIFILTER_MACFILTER_DAYTIME = get_cgi_json(g_var_post, "MULTIFILTER_MACFILTER_DAYTIME", root);
	if(MULTIFILTER_ALL)
		tcapi_set("Parental_Entry", "MULTIFILTER_ALL", MULTIFILTER_ALL);
	if(MULTIFILTER_ENABLE)
		tcapi_set("Parental_Entry", "MULTIFILTER_ENABLE", MULTIFILTER_ENABLE);
	if(MULTIFILTER_MAC)
		tcapi_set("Parental_Entry", "MULTIFILTER_MAC", MULTIFILTER_MAC);
	if(MULTIFILTER_DEVICENAME)
		//tcapi_set("Parental_Entry", "MULTIFILTER_DEVICENAME", MULTIFILTER_DEVICENAME);
		tcapi_set_list("MULTIFILTER_DEVICENAME", MULTIFILTER_DEVICENAME);
	if(MULTIFILTER_MACFILTER_DAYTIME)
		//tcapi_set("Parental_Entry", "MULTIFILTER_MACFILTER_DAYTIME", MULTIFILTER_MACFILTER_DAYTIME);
		tcapi_set_list("MULTIFILTER_MACFILTER_DAYTIME", MULTIFILTER_MACFILTER_DAYTIME);
	if(MULTIFILTER_ALL || MULTIFILTER_ENABLE || MULTIFILTER_MAC || MULTIFILTER_DEVICENAME || MULTIFILTER_MACFILTER_DAYTIME) {
		app_method_hit = 1;
	}

	/* [SET] [Wireless] WPS Method */
	char *wps_band = get_cgi_json(g_var_post, "wps_band", root);
	if(wps_band) {
		tcapi_set("WLan_Common","wl_unit", wps_band);
			app_method_hit = 1;
	}

	/* [SET] [QIS] */
	char *wan0_proto = get_cgi_json(g_var_post, "wan0_proto", root);
	char *wan1_proto = get_cgi_json(g_var_post, "wan1_proto", root);
	if((wan0_proto) && (pri_unit >= 0)) {
		sprintf(tmp, "wan%d_proto", pri_unit);
		tcapi_set("Wanduck_Common", tmp, wan0_proto);
		app_method_hit = 1;
	}
	if((wan1_proto) && (pri_unit >= 0)) {
		sprintf(tmp, "wan%d_proto", sec_unit);
		tcapi_set("Wanduck_Common", tmp, wan1_proto);
		app_method_hit = 1;
	}
	char *wan0_dhcpenable_x = get_cgi_json(g_var_post, "wan0_dhcpenable_x", root);
	char *wan1_dhcpenable_x = get_cgi_json(g_var_post, "wan1_dhcpenable_x", root);
	if((wan0_dhcpenable_x) && (pri_unit >= 0)) {
		tcapi_set(wan0_node, "ISP", strcmp(wan0_dhcpenable_x, "0") ? "0" : "1"); //ISP=0, Automatic; ISP=1, Static; ISP=2, PPPoE
		app_method_hit = 1;
	}
	if((wan1_dhcpenable_x) && (sec_unit >= 0)) {
		tcapi_set(wan1_node, "ISP", strcmp(wan1_dhcpenable_x, "0") ? "0" : "1"); //ISP=0, Automatic; ISP=1, Static; ISP=2, PPPoE
		app_method_hit = 1;
	}

	/* [SET] [QIS] Wan DHCP Setting */
	char *wan0_dnsenable_x = get_cgi_json(g_var_post, "wan0_dnsenable_x", root);
	char *wan0_dns1_x = get_cgi_json(g_var_post, "wan0_dns1_x", root);
	char *wan0_dns2_x = get_cgi_json(g_var_post, "wan0_dns2_x", root);
	if(pri_unit >= 0) {
		if(wan0_dnsenable_x)
			tcapi_set(wan0_node, "DNS_type", strcmp(wan0_dnsenable_x, "0") ? "0" : "1");
		if(wan0_dns1_x)
			tcapi_set(wan0_node, "Primary_DNS", wan0_dns1_x);
		if(wan0_dns2_x)
			tcapi_set(wan0_node, "Secondary_DNS", wan0_dns2_x);
		if(wan0_dnsenable_x || wan0_dns1_x || wan0_dns2_x)
			app_method_hit = 1;
	}
	char *wan1_dnsenable_x = get_cgi_json(g_var_post, "wan1_dnsenable_x", root);
	char *wan1_dns1_x = get_cgi_json(g_var_post, "wan1_dns1_x", root);
	char *wan1_dns2_x = get_cgi_json(g_var_post, "wan1_dns2_x", root);
	if(sec_unit >= 0) {
		if(wan1_dnsenable_x)
			tcapi_set(wan1_node, "DNS_type", strcmp(wan1_dnsenable_x, "0") ? "0" : "1");
		if(wan1_dns1_x)
			tcapi_set(wan1_node, "Primary_DNS", wan1_dns1_x);
		if(wan1_dns2_x)
			tcapi_set(wan1_node, "Secondary_DNS", wan1_dns2_x);
		if(wan1_dnsenable_x || wan1_dns1_x || wan1_dns2_x)
			app_method_hit = 1;
	}

	/* [SET] [QIS] Wan L2TP Setting */
	/* not support
	char *wan0_heartbeat_x = get_cgi_json(g_var_post, "wan0_heartbeat_x", root);
	 */
	/* [SET] [QIS] Wan PPPoE Setting */
	char *wan0_pppoe_username = get_cgi_json(g_var_post, "wan0_pppoe_username", root);
	char *wan0_pppoe_passwd = get_cgi_json(g_var_post, "wan0_pppoe_passwd", root);
	char *wan0_hwaddr_x = get_cgi_json(g_var_post, "wan0_hwaddr_x", root);
	if(pri_unit >= 0) {
		if(wan0_pppoe_username)
			tcapi_set(wan0_node, "USERNAME", wan0_pppoe_username);
		if(wan0_pppoe_passwd)
			tcapi_set(wan0_node, "PASSWORD", wan0_pppoe_passwd);
		if(wan0_hwaddr_x)
			tcapi_set(wan0_node, "WAN_MAC", wan0_hwaddr_x);
		if(wan0_pppoe_username || wan0_pppoe_passwd || wan0_hwaddr_x)
			app_method_hit = 1;
	}
	char *wan1_pppoe_username = get_cgi_json(g_var_post, "wan1_pppoe_username", root);
	char *wan1_pppoe_passwd = get_cgi_json(g_var_post, "wan1_pppoe_passwd", root);
	char *wan1_hwaddr_x = get_cgi_json(g_var_post, "wan1_hwaddr_x", root);
	if(sec_unit >= 0) {
		if(wan1_pppoe_username)
			tcapi_set(wan1_node, "USERNAME", wan1_pppoe_username);
		if(wan1_pppoe_passwd)
			tcapi_set(wan1_node, "PASSWORD", wan1_pppoe_passwd);
		if(wan1_hwaddr_x)
			tcapi_set(wan1_node, "WAN_MAC", wan1_hwaddr_x);
		if(wan1_pppoe_username || wan1_pppoe_passwd || wan1_hwaddr_x)
			app_method_hit = 1;
	}

	/* [SET] [QIS] Wan PPTP Setting */
	/* not support
	char *wan0_heartbeat_x = get_cgi_json(g_var_post, "wan0_heartbeat_x", root);
	 */
	/* [SET] [QIS] Wan Static Setting */
	char *wan0_ipaddr = get_cgi_json(g_var_post, "wan0_ipaddr", root);
	char *wan0_gateway = get_cgi_json(g_var_post, "wan0_gateway", root);
	char *wan0_netmask = get_cgi_json(g_var_post, "wan0_netmask", root);
	char *wan0_ipaddr_x = get_cgi_json(g_var_post, "wan0_ipaddr_x", root);
	char *wan0_gateway_x = get_cgi_json(g_var_post, "wan0_gateway_x", root);
	char *wan0_netmask_x = get_cgi_json(g_var_post, "wan0_netmask_x", root);
	if(pri_unit >= 0) {
		if(wan0_ipaddr) {
			sprintf(tmp, "wan%d_ipaddr", pri_unit);
			tcapi_set("Wanduck_Common", tmp, wan0_ipaddr);
			app_method_hit = 1;
		}
		else if(wan0_ipaddr_x) {
			sprintf(tmp, "wan%d_ipaddr", pri_unit);
			tcapi_set("Wanduck_Common", tmp, wan0_ipaddr_x);
			app_method_hit = 1;
		}
		if(wan0_gateway) {
			sprintf(tmp, "wan%d_gateway", pri_unit);
			tcapi_set("Wanduck_Common", tmp, wan0_gateway);
			app_method_hit = 1;
		}
		else if(wan0_gateway_x) {
			sprintf(tmp, "wan%d_gateway", pri_unit);
			tcapi_set("Wanduck_Common", tmp, wan0_gateway_x);
			app_method_hit = 1;
		}
		if(wan0_netmask) {
			sprintf(tmp, "wan%d_netmask", pri_unit);
			tcapi_set("Wanduck_Common", tmp, wan0_netmask);
			app_method_hit = 1;
		}
		else if(wan0_netmask_x) {
			sprintf(tmp, "wan%d_netmask", pri_unit);
			tcapi_set("Wanduck_Common", tmp, wan0_netmask_x);
			app_method_hit = 1;
		}
	}
	char *wan1_ipaddr = get_cgi_json(g_var_post, "wan1_ipaddr", root);
	char *wan1_gateway = get_cgi_json(g_var_post, "wan1_gateway", root);
	char *wan1_netmask = get_cgi_json(g_var_post, "wan1_netmask", root);
	char *wan1_ipaddr_x = get_cgi_json(g_var_post, "wan1_ipaddr_x", root);
	char *wan1_gateway_x = get_cgi_json(g_var_post, "wan1_gateway_x", root);
	char *wan1_netmask_x = get_cgi_json(g_var_post, "wan1_netmask_x", root);
	if(sec_unit >= 0) {
		if(wan1_ipaddr) {
			sprintf(tmp, "wan%d_ipaddr", sec_unit);
			tcapi_set("Wanduck_Common", tmp, wan1_ipaddr);
			app_method_hit = 1;
		}
		else if(wan1_ipaddr_x) {
			sprintf(tmp, "wan%d_ipaddr", sec_unit);
			tcapi_set("Wanduck_Common", tmp, wan1_ipaddr_x);
			app_method_hit = 1;
		}
		if(wan1_gateway) {
			sprintf(tmp, "wan%d_gateway", sec_unit);
			tcapi_set("Wanduck_Common", tmp, wan1_gateway);
			app_method_hit = 1;
		}
		else if(wan1_gateway_x) {
			sprintf(tmp, "wan%d_gateway", sec_unit);
			tcapi_set("Wanduck_Common", tmp, wan1_gateway_x);
			app_method_hit = 1;
		}
		if(wan1_netmask) {
			sprintf(tmp, "wan%d_netmask", sec_unit);
			tcapi_set("Wanduck_Common", tmp, wan1_netmask);
			app_method_hit = 1;
		}
		else if(wan1_netmask_x) {
			sprintf(tmp, "wan%d_netmask", sec_unit);
			tcapi_set("Wanduck_Common", tmp, wan1_netmask_x);
			app_method_hit = 1;
		}
	}

	/* DUT_RT_SetBandwidthMonitorMode */
	/* not support
	char *apps_analysis = get_cgi_json(g_var_post, "apps_analysis", root);
	 */

	/* Others */
	char *x_Setting = get_cgi_json(g_var_post, "x_Setting", root);
	if(x_Setting) {
		tcapi_set("SysInfo_Entry","x_Setting", x_Setting);
		app_method_hit = 1;
	}

	/* [SET] [QIS] Username/Passwd */
	char *http_username = get_cgi_json(g_var_post, "http_username", root);
	char *http_passwd = get_cgi_json(g_var_post, "http_passwd", root);
	if(http_username)
		tcapi_set("Account_Entry0", "username", http_username);
	if(http_passwd) {
		tcapi_set("Account_Entry0", "web_passwd", http_passwd);
		tcapi_set("Account_Entry0", "console_passwd", http_passwd);
	}
	if(http_username || http_passwd) {
		tcapi_commit("Account");
		app_method_hit = 2;
#ifdef RTCONFIG_USB
		/*modify_aidisk_account();*/
		{
			char acclist[256] = {0};
			char newpwd[128] = {0};
			char newuser[128] = {0};
			char *p = NULL;
			char charAccount[64] = {0};

			tcapi_get("Samba_Entry", "acc_list", acclist);
			tcapi_get("Account_Entry0", "username", newuser);
			tcapi_get("Account_Entry0", "web_passwd", newpwd);

			if(strlen(acclist) > 1)
			{
				p = strstr(acclist, ">");
				if(p)
				{
					*p = 0;
					ascii_to_char_safe(charAccount, acclist, 64);
					mod_account(charAccount, newuser, newpwd);
				}
			}
		}
#endif
	}

	return app_method_hit;
}

static int appDo_rc_service(char *rc_service, char *wp, json_object *root)
{
	int app_method_hit = 0;
	char wan0_node[16], wan1_node[16];

	int pri_unit = wan_primary_ifunit();
	int sec_unit = wan_secondary_ifunit();
	if (pri_unit >= 0) {
		sprintf(wan0_node, "Wan_PVC%d", pri_unit);
	}
	if (sec_unit >= 0) {
		sprintf(wan1_node, "Wan_PVC%d", sec_unit);
	}

	//_dprintf("[AiHome] appDo_rc_service: rc_service=%s\n", rc_service);
	if(rc_service) {
		if(!strcmp(rc_service, "enable_webdav")) {
			char *ddns_enable_x = get_cgi_json(g_var_post, "ddns_enable_x", root);
			char *ddns_server_x = get_cgi_json(g_var_post, "ddns_server_x", root);
			char *ddns_hostname_x = get_cgi_json(g_var_post, "ddns_hostname_x", root);
			char *enable_webdav = get_cgi_json(g_var_post, "enable_webdav", root);
			char *webdav_aidisk = get_cgi_json(g_var_post, "webdav_aidisk", root);
			char *webdav_proxy = get_cgi_json(g_var_post, "webdav_proxy", root);
			char *st_webdav_mode = get_cgi_json(g_var_post, "st_webdav_mode", root);

			if(ddns_enable_x || ddns_server_x || ddns_hostname_x) {
				tcapi_commit("Ddns");
				app_method_hit = 2; //"MODIFIED" and "RUN SERVICE"
			}

			if(enable_webdav)
				tcapi_set("AiCloud_Entry", "enable_webdav", enable_webdav);
			if(webdav_aidisk)
				tcapi_set("AiCloud_Entry", "webdav_aidisk", webdav_aidisk);
			if(webdav_proxy)
				tcapi_set("AiCloud_Entry", "webdav_proxy", webdav_proxy);
			if(st_webdav_mode)
				tcapi_set("AiCloud_Entry", "st_webdav_mode", st_webdav_mode);
			if(enable_webdav || webdav_aidisk || webdav_proxy || st_webdav_mode) {
				tcapi_commit("AiCloud_Entry");
				system("/sbin/rc_call webdav 2 &");	//restart
				app_method_hit = 2;
			}
		}
		else if(!strcmp(rc_service, "restart_cloudsync")) {
			char *enable_cloudsync = get_cgi_json(g_var_post, "enable_cloudsync", root);
			char *cloud_sync = get_cgi_json(g_var_post, "cloud_sync", root);

			if(enable_cloudsync)
				tcapi_set("AiCloud_Entry", "enable_cloudsync", enable_cloudsync);
			if(cloud_sync)
				tcapi_set("AiCloud_Entry", "cloud_sync", cloud_sync);
			if(enable_cloudsync || cloud_sync) {
				tcapi_commit("AiCloud_Entry");
				system("/sbin/rc_call cloudsync 2 &");	//restart
				app_method_hit = 2;
			}
		}
		/* [SET] [AiProtection] Router Security - Secure your router */
		/* [SET] [WAN] Disconnect WAN */
		else if(!strncmp(rc_service, "restart_wan_if", 14)) {
			/* not support wan1_upnp_enable */
			/* [SET] [QIS] */
			char *wan0_proto = get_cgi_json(g_var_post, "wan0_proto", root);
			char *wan1_proto = get_cgi_json(g_var_post, "wan1_proto", root);
			if(wan0_proto || wan1_proto) {
				tcapi_commit("Wanduck_Common");
				app_method_hit = 2;
			}
			char *wan0_enable = get_cgi_json(g_var_post, "wan0_enable", root);
			char *wan0_dhcpenable_x = get_cgi_json(g_var_post, "wan0_dhcpenable_x", root);
			char *wan0_dnsenable_x = get_cgi_json(g_var_post, "wan0_dnsenable_x", root);
			char *wan0_dns1_x = get_cgi_json(g_var_post, "wan0_dns1_x", root);
			char *wan0_dns2_x = get_cgi_json(g_var_post, "wan0_dns2_x", root);
			char *wan0_pppoe_username = get_cgi_json(g_var_post, "wan0_pppoe_username", root);
			char *wan0_pppoe_passwd = get_cgi_json(g_var_post, "wan0_pppoe_passwd", root);
			char *wan0_hwaddr_x = get_cgi_json(g_var_post, "wan0_hwaddr_x", root);
			char *wan0_ipaddr = get_cgi_json(g_var_post, "wan0_ipaddr", root);
			char *wan0_gateway = get_cgi_json(g_var_post, "wan0_gateway", root);
			char *wan0_netmask = get_cgi_json(g_var_post, "wan0_netmask", root);
			char *wan0_ipaddr_x = get_cgi_json(g_var_post, "wan0_ipaddr_x", root);
			char *wan0_gateway_x = get_cgi_json(g_var_post, "wan0_gateway_x", root);
			char *wan0_netmask_x = get_cgi_json(g_var_post, "wan0_netmask_x", root);
			if((wan0_enable || wan0_dhcpenable_x
				|| wan0_dnsenable_x || wan0_dns1_x || wan0_dns2_x
				|| wan0_pppoe_username || wan0_pppoe_passwd || wan0_hwaddr_x
				|| wan0_ipaddr || wan0_gateway || wan0_netmask
				|| wan0_ipaddr_x || wan0_gateway_x || wan0_netmask_x) && (pri_unit >= 0)) {
				tcapi_commit(wan0_node);
				app_method_hit = 2;
			}
			char *wan1_enable = get_cgi_json(g_var_post, "wan1_enable", root);
			char *wan1_dhcpenable_x = get_cgi_json(g_var_post, "wan1_dhcpenable_x", root);
			char *wan1_dnsenable_x = get_cgi_json(g_var_post, "wan1_dnsenable_x", root);
			char *wan1_dns1_x = get_cgi_json(g_var_post, "wan1_dns1_x", root);
			char *wan1_dns2_x = get_cgi_json(g_var_post, "wan1_dns2_x", root);
			char *wan1_pppoe_username = get_cgi_json(g_var_post, "wan1_pppoe_username", root);
			char *wan1_pppoe_passwd = get_cgi_json(g_var_post, "wan1_pppoe_passwd", root);
			char *wan1_hwaddr_x = get_cgi_json(g_var_post, "wan1_hwaddr_x", root);
			char *wan1_ipaddr = get_cgi_json(g_var_post, "wan1_ipaddr", root);
			char *wan1_gateway = get_cgi_json(g_var_post, "wan1_gateway", root);
			char *wan1_netmask = get_cgi_json(g_var_post, "wan1_netmask", root);
			char *wan1_ipaddr_x = get_cgi_json(g_var_post, "wan1_ipaddr_x", root);
			char *wan1_gateway_x = get_cgi_json(g_var_post, "wan1_gateway_x", root);
			char *wan1_netmask_x = get_cgi_json(g_var_post, "wan1_netmask_x", root);
			if((wan1_enable || wan1_dhcpenable_x
				|| wan1_dnsenable_x || wan1_dns1_x || wan1_dns2_x
				|| wan1_pppoe_username || wan1_pppoe_passwd || wan1_hwaddr_x
				|| wan1_ipaddr || wan1_gateway || wan1_netmask
				|| wan1_ipaddr_x || wan1_gateway_x || wan1_netmask_x) && (sec_unit >= 0)) {
				tcapi_commit(wan1_node);
				app_method_hit = 2;
			}
			tcapi_commit("Upnpd_Entry");
			tcapi_commit("Route");
		}
		else if(!strcmp(rc_service, "restart_time")) {
			tcapi_commit("Firewall");
			app_method_hit = 2;
		}
		else if(!strcmp(rc_service, "restart_firewall")) {
			tcapi_commit("Firewall");
			app_method_hit = 2;
		}
		else if(!strcmp(rc_service, "restart_wrs")) {
			/* not support [AiProtection][bwdpi] wrs_cc_enable, wrs_vp_enable, wrs_mals_enable */
		}
		else if(!strcmp(rc_service, "restart_wireless")) {
			/* [SET] [Wireless] Mac filter */
			char *wl0_macmode = get_cgi_json(g_var_post, "wl0_macmode", root);
			char *wl1_macmode = get_cgi_json(g_var_post, "wl1_macmode", root);
			char *wl2_macmode = get_cgi_json(g_var_post, "wl2_macmode", root);
			char *wl0_maclist_x = get_cgi_json(g_var_post, "wl0_maclist_x", root);
			char *wl1_maclist_x = get_cgi_json(g_var_post, "wl1_maclist_x", root);
			char *wl2_maclist_x = get_cgi_json(g_var_post, "wl2_maclist_x", root);
			if(wl0_macmode || wl1_macmode || wl2_macmode || wl0_maclist_x || wl1_maclist_x || wl2_maclist_x) {
				tcapi_commit("ACL_Entry0");
				app_method_hit = 2;
			}
			else {
			/* [SET] [AiProtection] Router Security - Secure your router */
			/* [SET] [Guest Network] 2.4GHz - 1 */
			/* [SET] [Wireless] General Setting */
			/* [SET] [Wireless] radio on/off */
			/* [SET] [Wireless] Roaming assistant */
			/* [SET] [Wireless] Tx power adjustment */
				tcapi_commit("WLan_Entry0");
				app_method_hit = 2;
			}
		}
		/* [SET] [CloudCheck] Enabled */
		else if(!strcmp(rc_service, "restart_cloudcheck")) {
			/* not support enable_cloudcheck */
		}
		/* [SET] [Firmware Update] download fw server info */
		else if(!strcmp(rc_service, "start_webs_update")) {
			eval("/usr/script/webs_update.sh");
			app_method_hit = 2;
		}
		/* [SET] [General] Firmware Upgrade */
		else if(!strcmp(rc_service, "start_webs_upgrade")) {
			system("/usr/script/webs_upgrade.sh");
			/*stop_service();*/
			killall_tk("clean_cache.sh");
			sleep(1);
			kill_pidfile_s("/var/run/cfg_manager_pid", SIGUSR2);
			sleep(1);
#ifdef RTCONFIG_USB
			system("ejusb 1 0");	//force ejusb whether usb apps enabled or not
			system("ejusb 2 0");
#endif
			system("rc_call service 0");
			/*start to upgrade*/
			tcapi_set("System_Entry","upgrade_fw_status","NONE");
			tcapi_set("System_Entry","upgrade_fw","1");
			tcapi_commit("System_Entry");
			app_method_hit = 2;
		}
		/* [SET] [Change Mode] Repeater mode setting */
		/* [SET] [General] Reboot */
		/* [SET] [USB] samba - NTFS Sparse Files Support */
		else if(!strcmp(rc_service, "reboot")) {
			system("reboot");
			app_method_hit = 2;
		}
		/* [SET] [QoS] Bandwidth limiter */
		else if(!strcmp(rc_service, "restart_qos")) {
			tcapi_commit("QoS_Entry0");
			app_method_hit = 2;
		}
		/* [SET] [System] HTTP / HTTPS */
		else if(!strcmp(rc_service, "restart_httpd")) {
			tcapi_commit("Https_Entry");
			app_method_hit = 2;
		}
		/* [SET] [USB] FTP - Enabled */
		else if(!strcmp(rc_service, "restart_ftpd")) {
			tcapi_commit("Samba_Entry");
			eval("/sbin/rc_call", "ftp", "2");
			app_method_hit = 2;
		}
		else if(!strcmp(rc_service, "stop_ftpd")) {
			tcapi_commit("Samba_Entry");
			eval("/sbin/rc_call", "ftp", "0");
			app_method_hit = 2;
		}
		/* [SET] [USB] samba - enabled */
		else if(!strcmp(rc_service, "restart_samba")) {
			tcapi_commit("Samba_Entry");
			eval("/sbin/rc_call", "samba", "2");
			app_method_hit = 2;
		}
		else if(!strcmp(rc_service, "stop_samba")) {
			tcapi_commit("Samba_Entry");
			eval("/sbin/rc_call", "samba", "0");
			app_method_hit = 2;
		}
		/* [SET] [WAN] ASUS DDNS */
		else if(!strcmp(rc_service, "restart_ddns")) {
			tcapi_commit("Ddns_Entry0");
			app_method_hit = 2;
		}
		/* [SET] [WAN] PPTP VPN server */
		else if(!strcmp(rc_service, "restart_vpnd")) {
			tcapi_commit("VPNControl_Entry");
			tcapi_commit("PPTP_Entry");
			system("/sbin/service restart_vpnd");
			tcapi_commit("Firewall");
			app_method_hit = 2;
		}
		/* [SET] [Wireless] RF 4x4 to 1x1 */
		else if(!strcmp(rc_service, "restart_eco_guard")) {
			/* not support guard_mode */
		}
		/* [SET] [QIS] Change lan ip conflict */
		else if(!strcmp(rc_service, "restart_subnet")) {
			char lan_ipaddr_t[16];
			char lan_netmask_t[16];
			unsigned int lan_ip_num;
			unsigned int lan_mask_num;
			unsigned int lan_subnet;
			char wan_ipaddr_t[16];
			char wan_netmask_t[16];
			unsigned int wan_ip_num;
			unsigned int wan_mask_num;
			unsigned int wan_subnet;
			const unsigned int MAX_SUBNET = 3232300800U;
			const unsigned int MIN_LAN_IP = 3232235521U;
			struct in_addr addr;
			unsigned int new_lan_ip_num;
			unsigned int new_dhcp_start_num;
			unsigned int new_dhcp_end_num;
			char new_lan_ip_str[16];
			char new_dhcp_start_str[16];
			char new_dhcp_end_str[16];
			// char tmp_lan[100], prefix_lan[] = "lanXXXXXXXXXX_";
			int unit;
			// char tmp_wan[100], prefix_wan[] = "wanXXXXXXXXXX_";
			char tmp_wanip[32], tmp_wanmask[32];
			char wp[4] = {0};

			app_method_hit = 2;
			// snprintf(prefix_lan, sizeof(prefix_lan), "lan_");

			memset(lan_ipaddr_t, 0, sizeof(lan_ipaddr_t));
			// strcpy(lan_ipaddr_t, nvram_safe_get(strcat_r(prefix_lan, "ipaddr", tmp_lan)));
			tcapi_get("Lan_Entry0", "IP", lan_ipaddr_t);
			memset(&addr, 0, sizeof(addr));
			lan_ip_num = ntohl(inet_aton(lan_ipaddr_t, &addr));
			lan_ip_num = ntohl(addr.s_addr);
			memset(lan_netmask_t, 0, sizeof(lan_netmask_t));
			// strcpy(lan_netmask_t, nvram_safe_get(strcat_r(prefix_lan, "netmask", tmp_lan)));
			tcapi_get("Lan_Entry0", "netmask", lan_netmask_t);
			memset(&addr, 0, sizeof(addr));
			lan_mask_num = ntohl(inet_aton(lan_netmask_t, &addr));
			lan_mask_num = ntohl(addr.s_addr);
			lan_subnet = lan_ip_num&lan_mask_num;
		dbg("http: get lan_subnet=%x!\n", lan_subnet);

#if 0 /* don't care about secondary WAN in error redirect page, and must consider DSL and USB WAN. */
			for(unit = WAN_UNIT_FIRST; unit < WAN_UNIT_MAX; ++unit){
				if(unit != wan_primary_ifunit()
						&& !nvram_match("wans_mode", "lb")
						)
					continue;

				int wan_type = get_dualwan_by_pvcunit(unit);
				if(wan_type != WANS_DUALWAN_IF_WAN
						&& wan_type != WANS_DUALWAN_IF_LAN)
					continue;

				break;
			}
			if(unit == WAN_UNIT_MAX){
		dbg("http: Can't get the WAN's unit!\n");
				websWrite(wp, "0");
				return;
			}
#else
			unit = wan_primary_ifunit();
#endif
			// wan_prefix(unit, prefix_wan);

			memset(wan_ipaddr_t, 0, sizeof(wan_ipaddr_t));
			// strcpy(wan_ipaddr_t, nvram_safe_get(strcat_r(prefix_lan, "ipaddr", tmp_wan)));
			sprintf(tmp_wanip, "wan%d_ipaddr", unit);
			tcapi_get("Wanduck_Common", tmp_wanip, wan_ipaddr_t);
			memset(&addr, 0, sizeof(addr));
			wan_ip_num = ntohl(inet_aton(wan_ipaddr_t, &addr));
			wan_ip_num = ntohl(addr.s_addr);
			memset(wan_netmask_t, 0, sizeof(wan_netmask_t));
			// strcpy(wan_netmask_t, nvram_safe_get(strcat_r(prefix_lan, "netmask", tmp_wan)));
			sprintf(tmp_wanmask, "wan%d_netmask", unit);
			tcapi_get("Wanduck_Common", tmp_wanmask, wan_netmask_t);
			memset(&addr, 0, sizeof(addr));
			wan_mask_num = ntohl(inet_aton(wan_netmask_t, &addr));
			wan_mask_num = ntohl(addr.s_addr);
			wan_subnet = wan_ip_num&wan_mask_num;
		dbg("http: get wan_subnet=%x!\n", wan_subnet);

			if(lan_subnet != wan_subnet){
		dbg("http: The subnets of WAN and LAN aren't the same already.!\n");
				//websWrite(wp, "0");
				return app_method_hit;
			}

			if(lan_subnet >= MAX_SUBNET)
				new_lan_ip_num = MIN_LAN_IP;
			else
				new_lan_ip_num = lan_ip_num+(~lan_mask_num)+1;

			/* To prevent Lan IP: 192.168.1.254, then new dhcp start to 192.168.1.255. (~lan_mask_num)-1 like 254 */
			if((new_lan_ip_num&(~lan_mask_num)) == ((~lan_mask_num)-1)) {
				new_dhcp_start_num = (new_lan_ip_num&lan_mask_num) + 2; /* prevent x.x.x.1 for router */
				new_dhcp_end_num = (new_lan_ip_num&lan_mask_num) + (~lan_mask_num)-2;
				dbg("http: get new_lan_ip_num&(~lan_mask_num)=%x!\n", (new_lan_ip_num&(~lan_mask_num)));
			}
			else {
				new_dhcp_start_num = new_lan_ip_num+1;
				new_dhcp_end_num = (new_lan_ip_num|(~inet_network(lan_netmask_t)))-1;
			}
		dbg("%u, %u, %u.\n", new_lan_ip_num, new_dhcp_start_num, new_dhcp_end_num);
			memset(&addr, 0, sizeof(addr));
			addr.s_addr = htonl(new_lan_ip_num);
			memset(new_lan_ip_str, 0, sizeof(new_lan_ip_str));
			strcpy(new_lan_ip_str, inet_ntoa(addr));
			memset(&addr, 0, sizeof(addr));
			addr.s_addr = htonl(new_dhcp_start_num);
			memset(new_dhcp_start_str, 0, sizeof(new_dhcp_start_str));
			strcpy(new_dhcp_start_str, inet_ntoa(addr));
			memset(&addr, 0, sizeof(addr));
			addr.s_addr = htonl(new_dhcp_end_num);
			memset(new_dhcp_end_str, 0, sizeof(new_dhcp_end_str));
			strcpy(new_dhcp_end_str, inet_ntoa(addr));
		dbg("%s, %s, %s.\n", new_lan_ip_str, new_dhcp_start_str, new_dhcp_end_str);

			// nvram_set(strcat_r(prefix_lan, "ipaddr", tmp_lan), new_lan_ip_str);
			tcapi_set("Lan_Entry0", "IP", new_lan_ip_str);
			// nvram_set(strcat_r(prefix_lan, "ipaddr_rt", tmp_lan), new_lan_ip_str); // Sync to lan_ipaddr_rt, added by jerry5.
			// nvram_set("dhcp_start", new_dhcp_start_str);
			tcapi_set("Dhcpd_Common", "start", new_dhcp_start_str);
			// nvram_set("dhcp_end", new_dhcp_end_str);
			tcapi_set("Dhcpd_Common", "end", new_dhcp_end_str);

			//websWrite(wp, "1");

			// nvram_commit();
			tcapi_save();	// commit, move to restart net_and_phy

			// notify_rc("restart_net_and_phy");
			//TODO:  many services may need to restart
			eval("/sbin/rc_call", "net_and_phy", "2");

			return app_method_hit;
		}
	}

	return app_method_hit;
}

/* Look for unquoted character within a string */
char *
unqstrstr_t(char *haystack, char *needle)
{
	char *cur;
	int q;

	for (cur = haystack, q = 0;
	     cur < &haystack[strlen(haystack)] && !(!q && !strncmp(needle, cur, strlen(needle)));
	     cur++) {
		if (*cur == '"')
			q ? q-- : q++;
	}
	return (cur < &haystack[strlen(haystack)]) ? cur : NULL;
}

char *
get_arg_t(char *args, char **next)
{
	char *arg, *end;

	/* Parse out arg, ... */
	if (!(end = unqstrstr_t(args, ","))) {
		end = args + strlen(args);
		*next = NULL;
	} else
		*next = end + 1;

	/* Skip whitespace and quotation marks on either end of arg */
	for (arg = args; isspace((int)*arg) || *arg == '"'; arg++);
	for (*end-- = '\0'; isspace((int)*end) || *end == '"'; end--)
		*end = '\0';

	return arg;
}

char *filter(char *string, char c) {
    char *from, *to;
    for(to = from = string;*from;++from){
		//boa_dbg("%s: *from=%c, c=%c\n", __FUNCTION__, *from, c);
        if(*from != c){
            *to++ = *from;
        }
    }
    *to = '\0';
    return string;
}

#if 0
int
ejArgs(int argc, char **argv, char *fmt, ...)
{
	va_list	ap;
	int arg;
	char *c;

	if (!argv)
		return 0;

	va_start(ap, fmt);
	for (arg = 0, c = fmt; c && *c && arg < argc;) {
		if (*c++ != '%')
			continue;
		switch (*c) {
		case 'd':
			*(va_arg(ap, int *)) = atoi(argv[arg]);
			break;
		case 's':
			*(va_arg(ap, char **)) = argv[arg];
			break;
		}
		arg++;
	}
	va_end(ap);

	return arg;
}
#endif

char* reverse_str(char *str)
{
  int i, n;
  char c;

  n = strlen(str);
  for(i=0; i<n/2; i++)
  {
    c = str[i];
    str[i] = str[n-i-1];
    str[n-i-1] = c;
  }

  return str;
}

static int
check_macrepeat(char *maclist,char *mac)
{
	int total=0;
	while (strstr(maclist,mac) != NULL)
	{
		maclist += strlen(maclist);
		total++;
	}
	return total;
}

static int
appGet_attribute(char *stream, char *name)
{
	int app_method_hit = 0;
	char value[MAX_VALUE], webs_state_info[8], exist_firmver[8], tmp[32], tmp2[64];
	int latest_firmver = 0, current_firmver = 0, tmpInt = -1;
	char *exist_firm;
	//_dprintf("appGet_attribute: name = %s\n", name);

	int pri_unit = wan_primary_ifunit();
	int sec_unit = wan_secondary_ifunit();

	/* Bandwidth limiter */
	if (strcmp(name, "qos_enable") == 0) {
		if(!tcapi_get("QoS_Entry0", "qos_enable", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2; /* DSL model not support */
		}
	}
	else if (strcmp(name, "qos_type") == 0) {
		if(!tcapi_get("QoS_Entry0", "qos_type", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "qos_rulelist") == 0) {
		//if(!tcapi_get("QoS_Entry0", "qos_rulelist", value)) {
		if(!tcapi_get_list("qos_rulelist", value, sizeof(value))) {
			appGet_encode(value);
			//websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "qos_bw_rulelist") == 0) {
		//if(!tcapi_get("QoS_Entry0", "qos_bw_rulelist", value)) {
		if(!tcapi_get_list("qos_bw_rulelist", value, sizeof(value))) {
			appGet_encode(value);
			//websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [Client List] custom name */
	else if (strcmp(name, "custom_clientlist") == 0) {
		//get_client_list_cache(); //<uuuuuu1>6C:62:6D:32:E2:70>0>13>><kkkkkkkkkk2>40:16:7E:8D:0B:0E>0>16>>
		/* not support */
		app_method_hit = 2;
	}
	/* firmware update - server info */
	/* DUT_RT_LiveUpdate_CheckResult */
	else if (strcmp(name, "webs_state_update") == 0) {
		if(!tcapi_get("WebCustom_Entry", "webs_state_update", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "webs_state_flag") == 0) {
		if(!tcapi_get("WebCustom_Entry", "webs_state_info", webs_state_info) && !tcapi_get("DeviceInfo", "FwVer", exist_firmver)) {
			latest_firmver = atoi(webs_state_info);
			if(latest_firmver > 0){
				exist_firm = filter(exist_firmver, '.');
				current_firmver = atoi(exist_firm);
				if(current_firmver < latest_firmver){
					websWrite(stream, "1"); /* need to upgrade firmware */
				}
				else {
					websWrite(stream, "0"); /* no need to upgrade firmware */
				}
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "webs_state_error") == 0) {
		if(!tcapi_get("WebCustom_Entry", "webs_state_error", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "webs_state_info") == 0) {
		if(!tcapi_get("WebCustom_Entry", "webs_state_info", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* game first mode */
	/* already exists: qos_enable, qos_type */
	else if (strcmp(name, "qos_obw") == 0) {
		if(!tcapi_get("QoS_Entry0", "qos_obw", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "qos_ibw") == 0) {
		if(!tcapi_get("QoS_Entry0", "qos_ibw", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "bwdpi_app_rulelist") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	else if (strcmp(name, "ctf_disable") == 0) {
		if(!tcapi_get("Misc_Entry", "hwnat_enable", value)) {
			if(strcmp(value, "1") == 0)
				websWrite(stream, "0");
			else
				websWrite(stream, "1");
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "ctf_fa_mode") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	/* Guest network - 2.4GHz multi ssid info */
	/* Guest network - choice 2.4GHz multi ssid enabled */
	/* DUT_RT_GetGuestWireless: 2.4GHz */
	else if (strcmp(name, "wl0.1_bss_enabled") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.1_bss_enabled", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_ssid") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.1_ssid", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_auth_mode_x") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.1_auth_mode_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_crypto") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.1_crypto", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_key") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.1_key", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_wpa_psk") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.1_wpa_psk", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_lanaccess") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.1_lanaccess", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_expire") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.1_expire", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_expire_tmp") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.1_expire_tmp", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_macmode") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl0.1_wl_macmode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.1_mbss") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	else if (strcmp(name, "wl0.2_bss_enabled") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.2_bss_enabled", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_ssid") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.2_ssid", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_auth_mode_x") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.2_auth_mode_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_crypto") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.2_crypto", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_key") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.2_key", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_wpa_psk") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.2_wpa_psk", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_lanaccess") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.2_lanaccess", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_expire") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.2_expire", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_expire_tmp") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.2_expire_tmp", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_macmode") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl0.2_wl_macmode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.2_mbss") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	else if (strcmp(name, "wl0.3_bss_enabled") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.3_bss_enabled", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_ssid") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.3_ssid", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_auth_mode_x") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.3_auth_mode_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_crypto") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.3_crypto", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_key") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.3_key", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_wpa_psk") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.3_wpa_psk", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_lanaccess") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.3_lanaccess", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_expire") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.3_expire", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_expire_tmp") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0.3_expire_tmp", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_macmode") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl0.3_wl_macmode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0.3_mbss") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	/* Guest network - 5GHz multi ssid info */
	/* Guest network - choice 5GHz multi ssid enabled */
	/* DUT_RT_GetGuestWireless: 5GHz */
	else if (strcmp(name, "wl1.1_bss_enabled") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.1_bss_enabled", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_ssid") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.1_ssid", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_auth_mode_x") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.1_auth_mode_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_crypto") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.1_crypto", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_key") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.1_key", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_wpa_psk") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.1_wpa_psk", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_lanaccess") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.1_lanaccess", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_expire") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.1_expire", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_expire_tmp") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.1_expire_tmp", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_macmode") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl1.1_wl_macmode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.1_mbss") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	else if (strcmp(name, "wl1.2_bss_enabled") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.2_bss_enabled", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_ssid") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.2_ssid", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_auth_mode_x") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.2_auth_mode_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_crypto") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.2_crypto", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_key") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.2_key", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_wpa_psk") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.2_wpa_psk", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_lanaccess") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.2_lanaccess", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_expire") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.2_expire", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_expire_tmp") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.2_expire_tmp", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_macmode") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl1.2_wl_macmode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.2_mbss") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	else if (strcmp(name, "wl1.3_bss_enabled") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.3_bss_enabled", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_ssid") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.3_ssid", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_auth_mode_x") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.3_auth_mode_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_crypto") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.3_crypto", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_key") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.3_key", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_wpa_psk") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.3_wpa_psk", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_lanaccess") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.3_lanaccess", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_expire") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.3_expire", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_expire_tmp") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1.3_expire_tmp", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_macmode") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl1.3_wl_macmode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1.3_mbss") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	/* Parental Control Rule */
	/* DUT_RT_GetParentalControl */
	else if (strcmp(name, "MULTIFILTER_ALL") == 0) {
		if(!tcapi_get("Parental_Entry", "MULTIFILTER_ALL", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "MULTIFILTER_ENABLE") == 0) {
		if(!tcapi_get("Parental_Entry", "MULTIFILTER_ENABLE", value)) {
			appGet_encode(value);
			//websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "MULTIFILTER_DEVICENAME") == 0) {
		//if(!tcapi_get("Parental_Entry", "MULTIFILTER_DEVICENAME", value)) {
		if(!tcapi_get_list("MULTIFILTER_DEVICENAME", value, sizeof(value))) {
			appGet_encode(value);
			//websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "MULTIFILTER_MAC") == 0) {
		if(!tcapi_get("Parental_Entry", "MULTIFILTER_MAC", value)) {
			appGet_encode(value);
			//websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "MULTIFILTER_MACFILTER_DAYTIME") == 0) {
		//if(!tcapi_get("Parental_Entry", "MULTIFILTER_MACFILTER_DAYTIME", value)) {
		if(!tcapi_get_list("MULTIFILTER_MACFILTER_DAYTIME", value, sizeof(value))) {
			appGet_encode(value);
			//websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [System] HTTP / HTTPS */
	else if (strcmp(name, "http_enable") == 0) {
		if(!tcapi_get("Https_Entry", "http_enable", value)) {
			/**** in DSL model ****|***** in RT model ****
			http_enable            | http_enable
			1:HTTP                 | 0:HTTP
			2:HTTPS                | 1:HTTPS
			3:Both HTTP and HTTPS  | 2:Both HTTP and HTTPS
			***********************|**********************/
			tmpInt = atoi(value);
			switch (tmpInt) {
				case 1:
					websWrite(stream, "0");
					app_method_hit = 1;
					break;
				case 2:
					websWrite(stream, "1");
					app_method_hit = 1;
					break;
				case 3:
					websWrite(stream, "2");
					app_method_hit = 1;
					break;
				default:
					app_method_hit = 2;
					break;
			}
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "https_lanport") == 0) {
		if(!tcapi_get("Https_Entry", "https_lanport", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [WAN] ASUS DDNS */
	else if (strcmp(name, "ddns_enable_x") == 0) {
		if(!tcapi_get("Ddns_Entry", "Active", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "ddns_server_x") == 0) {
		if(!tcapi_get("Ddns_Entry", "SERVERNAME", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "ddns_hostname_x") == 0) {
		if(!tcapi_get("Ddns_Entry", "MYHOST", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [WAN] Detect Internet status */
	else if (strcmp(name, "link_internet") == 0) {
		if(!tcapi_get("Wanduck_Common", "link_internet", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [WAN] Disconnect WAN */
	else if (strcmp(name, "wan0_enable") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", pri_unit);
			if(!tcapi_get(tmp, "Active", value)) {
				websWrite(stream, strcmp(value, "Yes") ? "0" : "1");
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan1_enable") == 0) {
		if (sec_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", sec_unit);
			if(!tcapi_get(tmp, "Active", value)) {
				websWrite(stream, strcmp(value, "Yes") ? "0" : "1");
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	/* PPTP VPN server */
	else if (strcmp(name, "VPNServer_enable") == 0) {
		if(!tcapi_get("VPNControl_Entry", "VPNServer_enable", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "pptpd_enable") == 0) {
		if(!tcapi_get("PPTP_Entry", "pptpd_enable", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "VPNServer_mode") == 0) {
		if(!tcapi_get("VPNControl_Entry", "VPNServer_mode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "pptpd_clientlist") == 0) {
		//if(!tcapi_get("PPTP_Entry", "pptpd_clientlist", value)) {
		if(!tcapi_get_list("pptpd_clientlist", value, sizeof(value))) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* router device name */
	/* DUT_RT_GetDeviceName */
	else if (strcmp(name, "dms_friendly_name") == 0) {
		if(!tcapi_get("DMS_Basic", "FriendlyName", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "computer_name") == 0) {
		if(!tcapi_get("Samba_Entry", "NetBiosName", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "st_samba_workgroup") == 0) {
		if(!tcapi_get("Samba_Entry", "st_samba_workgroup", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [AiProtection] Router Security - DMZ disabled */
	else if (strcmp(name, "dmz_ip") == 0) {
		if(!tcapi_get("Dmz_PVC", "DMZ_IP", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [AiProtection] Router Security - Infected Device Prevention and Blocking */
	else if (strcmp(name, "wrs_cc_enable") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	/* [GET] [AiProtection] Router Security - Malicious Website Blocking enabled */
	else if (strcmp(name, "wrs_mals_enable") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	/* [GET] [AiProtection] Router Security - Ping from Wan disabled */
	else if (strcmp(name, "misc_ping_x") == 0) {
		if(!tcapi_get("Firewall_Entry", "misc_ping_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [AiProtection] Router Security - Port forwarding disabled */
	else if (strcmp(name, "vts_enable_x") == 0) {
		if(!tcapi_get("VirServer_PVC0_Entry0", "VirServer_Enable", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [AiProtection] Router Security - Port trigger disabled */
	else if (strcmp(name, "autofw_enable_x") == 0) {
		if(!tcapi_get("portTriggering_Entry0", "AutoFW_Enable", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [AiProtection] Router Security - upnp server disabled */
	else if (strcmp(name, "wan0_upnp_enable") == 0) {
		if(!tcapi_get("Upnpd_Entry", "Active", value)) {
			if(strcmp(value, "Yes") == 0)
				websWrite(stream, "1");
			else
				websWrite(stream, "0");
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wan1_upnp_enable") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	/* [GET] [AiProtection] Router Security - Vulnerability Protection enabled */
	else if (strcmp(name, "wrs_vp_enable") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	/* [GET] [AiProtection] Router Security - Web access from wan disabled */
	/* [GET] [AiProtection] Router Security - Wireless encryption enabled */
	/* DUT_RT_GetHttpWANAccess */
	else if (strcmp(name, "misc_http_x") == 0) {
		if(!tcapi_get("Firewall_Entry", "misc_http_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "misc_httpport_x") == 0) {
		if(!tcapi_get("Firewall_Entry", "misc_httpport_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "misc_httpsport_x") == 0) {
		if(!tcapi_get("Firewall_Entry", "misc_httpsport_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [AiProtection] Router Security - WPS Disabled status */
	/* DUT_RT_GetWirelessWPS */
	else if (strcmp(name, "wps_band") == 0) {
		if(!tcapi_get("WLan_Common", "wl_unit", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wps_enable") == 0) {
		if(!tcapi_get("WLan_Common", "wps_enable", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wps_sta_pin") == 0) {
		if(!tcapi_get("WLan_Entry0", "enrolleePinCode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* Wireless 4x4 to 1x1 */
	else if (strcmp(name, "guard_mode") == 0) {
		/* not support now */
		if(!tcapi_get("WLan_Common", "guard_mode", value)) {
			websWrite(stream, value);
		}
		app_method_hit = 2;
	}
	/* Wireless Mac filter */
	else if (strcmp(name, "wl0_macmode") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl0_wl_macmode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_macmode") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl1_wl_macmode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl2_macmode") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl2_wl_macmode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0_maclist") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl0_wl_maclist", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_maclist") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl1_wl_maclist", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl2_maclist") == 0) {
		if(!tcapi_get("ACL_Entry0", "wl2_wl_maclist", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* wireless radio on/off */
	else if (strcmp(name, "wl0_radio") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0_radio_on", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_radio") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1_radio_on", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* [GET] [Wireless] Roaming assistant */
	else if (strcmp(name, "wl0_user_rssi") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0_user_rssi", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_user_rssi") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1_user_rssi", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl2_user_rssi") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl2_user_rssi", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* DUT_RT_GetWirelessConfig: 2.4GHz */
	else if (strcmp(name, "wl0_ssid") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0_ssid", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0_closed") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0_HideSSID", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0_nmode_x") == 0) { //2G=> 0:Legacy, 6:N only, 9:Auto
		if(!tcapi_get("WLan_Common", "wl0_WirelessMode", value)) {
			tmpInt = atoi(value);
			switch (tmpInt) {
				case 0:
					websWrite(stream, "2"); /*2: Legacy*/
					app_method_hit = 1;
					break;
				case 6:
					websWrite(stream, "1"); /*1: N Only*/
					app_method_hit = 1;
					break;
				case 9:
					websWrite(stream, "0"); /*0: Auto*/
					app_method_hit = 1;
					break;
				default:
					app_method_hit = 2;
					break;
			}
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0_bw") == 0) {
		if(!tcapi_get("WLan_Common", "wl0_HT_BW", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if ((strcmp(name, "wl0_auth_mode") == 0) || (strcmp(name, "wl0_auth_mode_x") == 0)) {
		if(!tcapi_get("WLan_Entry0", "wl0_auth_mode_x", value)) {
			_dprintf("app_call: wl0_auth_mode_x=%s\n", value);
			if(strcmp(value, "OPEN") ==0)
				websWrite(stream, "open");
			else if(strcmp(value, "WPA2PSK") ==0)
				websWrite(stream, "psk2");
			else if(strcmp(value, "WPAPSKWPA2PSK") ==0)
				websWrite(stream, "pskpsk2");
			else if(strcmp(value, "WPA2") ==0)
				websWrite(stream, "wpa2");
			else if(strcmp(value, "WPA1WPA2") ==0)
				websWrite(stream, "wpawpa2");
			else
				websWrite(stream, "");
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0_crypto") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0_crypto", value)) {
			_dprintf("app_call: wl0_crypto=%s\n", value);
			if(strcmp(value, "AES") ==0)
				websWrite(stream, "aes");
			else if(strcmp(value, "TKIP+AES") ==0)
				websWrite(stream, "tkip+aes");
			else
				websWrite(stream, "");
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl0_wpa_psk") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl0_wpa_psk", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* DUT_RT_GetWirelessConfig: 5GHz */
	else if (strcmp(name, "wl1_ssid") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1_ssid", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_closed") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1_HideSSID", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_nmode_x") == 0) { //5G=> 2:Legacy, 11: N only, 8:Auto
		if(!tcapi_get("WLan_Common", "wl1_WirelessMode", value)) {
			tmpInt = atoi(value);
			switch (tmpInt) {
				case 2:
					websWrite(stream, "2"); /*2: Legacy*/
					app_method_hit = 1;
					break;
				case 11:
					websWrite(stream, "1"); /*1: N Only*/
					app_method_hit = 1;
					break;
				case 8:
					websWrite(stream, "0"); /*0: Auto*/
					app_method_hit = 1;
					break;
				default:
					app_method_hit = 2;
					break;
			}
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_bw") == 0) {
		if(!tcapi_get("WLan_Common", "wl1_HT_BW", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_auth_mode_x") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1_auth_mode_x", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_crypto") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1_crypto", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_wpa_psk") == 0) {
		if(!tcapi_get("WLan_Entry0", "wl1_wpa_psk", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* DUT_RT_TotalWirelessIf */
	else if (strcmp(name, "wl_ifnames") == 0) {
		if(!tcapi_get("WLan_Common", "wl_ifnames", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* DUT_RT_GetWanConfig */
	else if (strcmp(name, "wan0_proto") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "wan%d_proto", pri_unit);
			if(!tcapi_get("Wanduck_Common", tmp, value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan1_proto") == 0) {
		if (sec_unit >= 0) {
			sprintf(tmp, "wan%d_proto", sec_unit);
			if(!tcapi_get("Wanduck_Common", tmp, value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan0_heartbeat_x") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	else if (strcmp(name, "wan1_heartbeat_x") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	else if (strcmp(name, "wan0_dhcpenable_x") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", pri_unit);
			if(!tcapi_get(tmp, "ISP", value)) {
				websWrite(stream, strcmp(value, "0") ? "0" : "1"); //ISP=0, Automatic; ISP=1, Static; ISP=2, PPPoE 
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan1_dhcpenable_x") == 0) {
		if (sec_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", sec_unit);
			if(!tcapi_get(tmp, "ISP", value)) {
				websWrite(stream, strcmp(value, "0") ? "0" : "1"); //ISP=0, Automatic; ISP=1, Static; ISP=2, PPPoE 
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if ((strcmp(name, "wan0_ipaddr") == 0) || (strcmp(name, "wan0_ipaddr_x") == 0)) {
		if (pri_unit >= 0) {
			sprintf(tmp, "wan%d_ipaddr", pri_unit);
			if(!tcapi_get("Wanduck_Common", tmp, value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if ((strcmp(name, "wan1_ipaddr") == 0) || (strcmp(name, "wan1_ipaddr_x") == 0)) {
		if (sec_unit >= 0) {
			sprintf(tmp, "wan%d_ipaddr", sec_unit);
			if(!tcapi_get("Wanduck_Common", tmp, value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if ((strcmp(name, "wan0_gateway") == 0) || (strcmp(name, "wan0_gateway_x") == 0)) {
		if (pri_unit >= 0) {
			sprintf(tmp, "wan%d_proto", pri_unit);
			if(!tcapi_get("Wanduck_Common", tmp, value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if ((strcmp(name, "wan1_gateway") == 0) || (strcmp(name, "wan1_gateway_x") == 0)) {
		if (sec_unit >= 0) {
			sprintf(tmp, "wan%d_gateway", sec_unit);
			if(!tcapi_get("Wanduck_Common", tmp, value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if ((strcmp(name, "wan0_netmask") == 0) || (strcmp(name, "wan0_netmask_x") == 0)) {
		if (pri_unit >= 0) {
			sprintf(tmp, "wan%d_netmask", pri_unit);
			if(!tcapi_get("Wanduck_Common", tmp, value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if ((strcmp(name, "wan1_netmask") == 0) || (strcmp(name, "wan1_netmask_x") == 0)) {
		if (sec_unit >= 0) {
			sprintf(tmp, "wan%d_netmask", sec_unit);
			if(!tcapi_get("Wanduck_Common", tmp, value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan0_dnsenable_x") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", pri_unit);
			if(!tcapi_get(tmp, "DNS_type", value)) {
				websWrite(stream, strcmp(value, "0") ? "0" : "1");
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan1_dnsenable_x") == 0) {
		if (sec_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", sec_unit);
			if(!tcapi_get(tmp, "DNS_type", value)) {
				websWrite(stream, strcmp(value, "0") ? "0" : "1");
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan0_dns_x") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", pri_unit);
			if(!tcapi_get(tmp, "Primary_DNS", value)) {
				if(!tcapi_get(tmp, "Secondary_DNS", tmp2)) {
					strcat(value, " ");
					strcat(value, tmp2);
					websWrite(stream, value);
				}
				else
					websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan1_dns_x") == 0) {
		if (sec_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", sec_unit);
			if(!tcapi_get(tmp, "Primary_DNS", value)) {
				if(!tcapi_get(tmp, "Secondary_DNS", tmp2)) {
					strcat(value, " ");
					strcat(value, tmp2);
					websWrite(stream, value);
				}
				else
					websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan0_pppoe_username") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", pri_unit);
			if(!tcapi_get(tmp, "USERNAME", value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan1_pppoe_username") == 0) {
		if (sec_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", sec_unit);
			if(!tcapi_get(tmp, "USERNAME", value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan0_pppoe_passwd") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", pri_unit);
			if(!tcapi_get(tmp, "PASSWORD", value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan1_pppoe_passwd") == 0) {
		if (sec_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", sec_unit);
			if(!tcapi_get(tmp, "PASSWORD", value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if ((strcmp(name, "wan0_hwaddr") == 0) || (strcmp(name, "wan0_hwaddr_x") == 0)
		|| (strcmp(name, "wan1_hwaddr") == 0) || (strcmp(name, "wan1_hwaddr_x") == 0)
	) {
		if(!tcapi_get("Wan_PVC0", "UniqueMacAddr", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* DUT_RT_TotalWanIf */
	else if (strcmp(name, "wan_ifnames") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "wan%d_proto", pri_unit);
			if(!tcapi_get("Wanduck_Common", tmp, tmp2) && (strcmp(tmp2, "pppoe") == 0)) {
				sprintf(tmp, "wan%d_pppoe_ifname", pri_unit);
			}
			else {
				sprintf(tmp, "wan%d_ifname", pri_unit);
			}
			if(!tcapi_get("Wanduck_Common", tmp, value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	/* DUT_RT_FirmwareExtNumber */
	else if (strcmp(name, "extendno") == 0) {
		if(!tcapi_get("DeviceInfo", "extendno", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* firmware version */
	/* DUT_RT_FirmwareInfo */
	else if (strcmp(name, "firmver") == 0) {
		if(!tcapi_get("DeviceInfo", "firmver", value)) { //tcapi_get("DeviceInfo", "FwVer", value)
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "buildno") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	/* DUT_RT_GetWirelessTxPower */
	else if (strcmp(name, "wl0_txpower") == 0) {
		if(!tcapi_get("WLan_Common", "wl0_TxPower", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wl1_txpower") == 0) {
		if(!tcapi_get("WLan_Common", "wl1_TxPower", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* DUT_RT_GetOperationMode */
	/*
	Router:	             sw_mode: 1, wlc_express: 0, wlc_psta: 0
	Repeater:            sw_mode: 2, wlc_express: 0, wlc_psta: 0
	Express Way 2.4 GHz: sw_mode: 2, wlc_express: 1, wlc_psta: 0
	Express Way 5 GHz:   sw_mode: 2, wlc_express: 2, wlc_psta: 0
	AP mode:             sw_mode: 3, wlc_express: 0, wlc_psta: 0
	Media Bridge:        sw_mode: 3, wlc_express: 0, wlc_psta: 1
	*/
	else if (strcmp(name, "sw_mode") == 0) {
		websWrite(stream, "1"); /*only support SW_MODE_ROUTER mode*/
		app_method_hit = 1;
	}
	else if (strcmp(name, "wlc_psta") == 0) {
		websWrite(stream, "0"); /*only support SW_MODE_ROUTER mode*/
		app_method_hit = 1;
	}
	else if (strcmp(name, "wlc_express") == 0) {
		websWrite(stream, "0"); /*only support SW_MODE_ROUTER mode*/
		app_method_hit = 1;
	}
	/* DUT_RT_GetRCSupport */
	else if (strcmp(name, "rc_support") == 0) {
		if(!tcapi_get("SysInfo_Entry", "rc_support", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* DUT_RT_GetSambaService */
	else if (strcmp(name, "enable_samba") == 0) {
		if(!tcapi_get("Samba_Entry", "Active", value)) {
			if(strcmp(value, "Yes") == 0)
				websWrite(stream, "1");
			else
				websWrite(stream, "0");
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "st_samba_mode") == 0) {
		if(!tcapi_get("Samba_Entry", "st_samba_mode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* DUT_RT_GetFTPService */
	else if (strcmp(name, "enable_ftp") == 0) {
		if(!tcapi_get("Samba_Entry", "enable_ftp", value)) {
			if(strcmp(value, "Yes") == 0)
				websWrite(stream, "1");
			else
				websWrite(stream, "0");
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "st_ftp_mode") == 0) {
		if(!tcapi_get("Samba_Entry", "st_ftp_mode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	/* DUT_RT_GetBandwidthMonitorMode */
	else if (strcmp(name, "apps_analysis") == 0) {
		/* not support */
		app_method_hit = 2;
	}
	/* Others */
	else if (strcmp(name, "productid") == 0) {
		if(!tcapi_get("SysInfo_Entry", "ProductName", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "x_Setting") == 0) {
		if(!tcapi_get("SysInfo_Entry", "x_Setting", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "nat_redirect_enable") == 0) {
		if(!tcapi_get("SysInfo_Entry", "nat_redirect_enable", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "nat_state") == 0) {
		if(!tcapi_get("Wanduck_Common", "nat_state", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wans_cap") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wans_cap", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wans_routing_rulelist") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wans_routing_rulelist", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wans_mode") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wans_mode", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wans_dualwan") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wans_dualwan", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wans_lb_ratio") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wans_lb_ratio", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wans_routing_enable") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wans_routing_enable", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wans_lanport") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wans_lanport", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wandog_interval") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wandog_interval", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wandog_maxfail") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wandog_maxfail", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wandog_fb_count") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wandog_fb_count", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wandog_delay") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wandog_delay", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wandog_target") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wandog_target", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wandog_enable") == 0) {
		if(!tcapi_get("Dualwan_Entry", "wandog_enable", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wan_unit") == 0) {
		if(!tcapi_get("WebCurSet_Entry", "wan_unit", value)) {
			websWrite(stream, value);
			app_method_hit = 1;
		}
		else {
			app_method_hit = 2;
		}
	}
	else if (strcmp(name, "wan0_primary") == 0) {
		websWrite(stream, "1");
		app_method_hit = 1;
	}
	else if (strcmp(name, "wan1_primary") == 0) {
		websWrite(stream, "0");
		app_method_hit = 1;
	}
	else if (strcmp(name, "wan0_pppoe_mtu") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", pri_unit);
			if(!tcapi_get(tmp, "MTU", value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan1_pppoe_mtu") == 0) {
		if (sec_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", sec_unit);
			if(!tcapi_get(tmp, "MTU", value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan0_6rd_prefix") == 0) {
		if (pri_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", pri_unit);
			if(!tcapi_get(tmp, "PREFIX6", value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}
	else if (strcmp(name, "wan1_6rd_prefix") == 0) {
		if (sec_unit >= 0) {
			sprintf(tmp, "Wan_PVC%d", sec_unit);
			if(!tcapi_get(tmp, "PREFIX6", value)) {
				websWrite(stream, value);
				app_method_hit = 1;
			}
			else {
				app_method_hit = 2;
			}
		}
	}

	return app_method_hit;
}

static void
appGet_encode(char *str)
{
	char *c, wp[1024] = {0};;

	for (c = str; *c; c++) {
		if (isprint(*c) &&
		    *c != '"' && *c != '&' && *c != '<' && *c != '>' && *c != '\\'){
			websWrite(wp, "%c", *c);
		}
		else if(*c == '\\'){
			websWrite(wp, "%c", *c);	
		}
		else {
			websWrite(wp, "&#%d", *c);
		}
	}
}

static void
app_call(char *func, char *stream)
{
	char *args, *end, *next;
	int argc;
	char * argv[32]={NULL};
	int app_method_hit = 0;
	char value[MAX_VALUE];

	/* Parse out ( args ) */
	if (!(args = strchr(func, '(')))
		return;
	if (!(end = unqstrstr_t(func, ")")))
		return;
	*args++ = *end = '\0';
	/* Set up argv list */
	for (argc = 0; argc < 16 && args && *args; argc++, args = next) {
		if (!(argv[argc] = get_arg_t(args, &next)))
			break;
	}
	//_dprintf("app_call: func = %s, argv[0] = %s\n", func, argv[0]);

	if(strcmp(func, "nvram_get") == 0){
		websWrite(stream,"\"%s\":", argv[0]);
		websWrite(stream,"\"" );
	}else if(argv[0] != NULL && strcmp(argv[0], "appobj") == 0){
		websWrite(stream,"\"%s\":", func);
		websWrite(stream,"{" );
	}else if(argv[0] != NULL){
		websWrite(stream,"\"%s-%s\":", func, argv[0]);
	}else{
		websWrite(stream,"\"%s\":", func);
	}

	/* get corresponding attribute value */
	if (strcmp(func, "nvram_get") == 0) {
		app_method_hit = appGet_attribute(stream, argv[0]);
	}
	/* [GET] [Client List] Netwrokmapd */
	else if (strcmp(func, "get_clientlist") == 0) {
		//get_client_list();
		int i, shm_client_info_id;
		void *shared_client_info=(void *) 0;
		char output_buf[2048];
		char maclist_buf[4096]=",\"maclist\":";
		char mac_buf[32], dev_name[32];
		char *brackets_h = "[";
		char *brackets_d = "]";
		char *dot = ",";
		char ipaddr[16];
		P_CLIENT_DETAIL_INFO_TABLE p_client_info_tab;
		int lock;
		char devname[LINE_SIZE], character;
		int j, len;
		int first_mac=1, first_info=1;

		lock = file_lock("networkmap");
		shm_client_info_id = shmget((key_t)1001, sizeof(CLIENT_DETAIL_INFO_TABLE), 0666|IPC_CREAT);
		if (shm_client_info_id == -1){
		    fprintf(stderr,"shmget failed\n");
		    file_unlock(lock);
		    goto get_api_fail;
		}

		shared_client_info = shmat(shm_client_info_id,(void *) 0,0);
		if (shared_client_info == (void *)-1){
			fprintf(stderr,"shmat failed\n");
			file_unlock(lock);
			goto get_api_fail;
		}

		p_client_info_tab = (P_CLIENT_DETAIL_INFO_TABLE)shared_client_info;
		for(i=0; i<p_client_info_tab->ip_mac_num; i++) {

			memset(dev_name, 0, 32);
			memset(output_buf, 0, 2048);
			memset(ipaddr, 0, 16);
			memset(mac_buf, 0, 32);
			memset(devname, 0, LINE_SIZE);

		    if(strcmp((const char *)p_client_info_tab->user_define[i], ""))
				strcpy(dev_name, (const char *)p_client_info_tab->user_define[i]);
			else
				strcpy(dev_name, (const char *)p_client_info_tab->device_name[i]);

		    if(p_client_info_tab->exist[i]==1) {
				len = strlen(dev_name);
				for (j=0; (j < len) && (j < LINE_SIZE-1); j++) {
					character = dev_name[j];
					if ((isalnum(character)) || (character == ' ') || (character == '-') || (character == '_'))
						devname[j] = character;
					else
						devname[j] = ' ';
				}

				sprintf(ipaddr, "%d.%d.%d.%d", p_client_info_tab->ip_addr[i][0],p_client_info_tab->ip_addr[i][1],
				p_client_info_tab->ip_addr[i][2],p_client_info_tab->ip_addr[i][3]);

				sprintf(mac_buf, "\"%02X:%02X:%02X:%02X:%02X:%02X\"",
				p_client_info_tab->mac_addr[i][0],p_client_info_tab->mac_addr[i][1],
				p_client_info_tab->mac_addr[i][2],p_client_info_tab->mac_addr[i][3],
				p_client_info_tab->mac_addr[i][4],p_client_info_tab->mac_addr[i][5]
				);
				if(first_mac == 1){
					first_mac = 0;
					strcat(maclist_buf,brackets_h);
				}else{
					strcat(maclist_buf,dot);
				}
				strcat(maclist_buf,mac_buf);

				if(first_info == 1){
					first_info = 0;			
				}else{
					websWrite(stream, ",\n");
				}

				tcapi_get("Lan_Entry", "IP", value);
				sprintf(output_buf, "%s:{\"type\":\"%d\",\"name\":\"%s\",\"ip\":\"%s\",\"mac\":%s,\"from\":\"networkmapd\",\"macRepeat\":\"%d\",\"isGateway\":\"%s\",\"isWebServer\":\"%d\",\"isPrinter\":\"%d\",\"isITunes\":\"%d\",\"isOnline\":\"true\"}",
				mac_buf,
				p_client_info_tab->type[i],
				devname,
				ipaddr,
				mac_buf,
				check_macrepeat(maclist_buf, mac_buf),
				!strcmp(value, ipaddr) ? "true" : "false",
				p_client_info_tab->http[i],
				p_client_info_tab->printer[i],
				p_client_info_tab->itune[i]
				);
				websWrite(stream, output_buf);
		    }
		}
		shmdt(shared_client_info);
		file_unlock(lock);
		strcat(maclist_buf,brackets_d);
		websWrite(stream, maclist_buf);

		app_method_hit = 1;
	}
	/* [GET] [Client List] Wireless 2.4GHz */
	else if (strcmp(func, "wl_sta_list_2g") == 0) {
		//wl_sta_list_2g(); //wl_sta_list(0);
		FILE* fp;
		char tmp[512]={0};

		system("/usr/bin/ATE wl_sta_list_app 2g");

		fp = fopen("/var/tmp/wl_lst_2g_app","r");
		if (fp != NULL){
			while(fgets(tmp, 512, fp)){
				if (tmp != NULL){
					websWrite(stream, "%s", tmp);
					//strcat(buf, tmp);
				}
			}
			fclose(fp);
		}
		app_method_hit = 1;
	}
	/* [GET] [Client List] Wireless 5GHz */
	else if (strcmp(func, "wl_sta_list_5g") == 0) {
		//wl_sta_list_5g(); //wl_sta_list(1);
		FILE* fp;
		char tmp[512]={0};

		system("/usr/bin/ATE wl_sta_list_app 5g");

		fp = fopen("/var/tmp/wl_lst_5g_app","r");
		if (fp != NULL){
			while(fgets(tmp, 512, fp)){
				if (tmp != NULL){
					websWrite(stream, "%s", tmp);
					//strcat(buf, tmp);
				}
			}
			fclose(fp);
		}
		app_method_hit = 1;
	}
	/* [GET] [Client List] Wireless 5GHz */
	else if (strcmp(func, "wl_sta_list_5g_2") == 0) {
		//not support
		app_method_hit = 2;
	}
	/* Client List User Icon */
	else if (strcmp(func, "get_upload_icon") == 0) {
		//ej_get_upload_icon();
#if 0
		char *client_mac = get_param(g_var_post, "clientmac");
		char *client_mac_tmp = NULL;
		int from_app = 0;

		if (ejArgs(argc, argv, "%s", &client_mac_tmp) < 1) {
			//_dprintf("name = NULL\n");
		}else if(!strcmp(client_mac, "")){
			client_mac = client_mac_tmp;
			from_app = 1;
		}

		if(strcmp(client_mac, "")) {
			char file_name[32];
			memset(file_name, 0, 32);

			//Check folder exist or not
			if(!check_if_dir_exist(JFFS_USERICON))
				mkdir(JFFS_USERICON, 0755);

			//Write upload icon value
			sprintf(file_name, "/jffs/usericon/%s.log", client_mac);
			if(check_if_file_exist(file_name)) {
				dump_file(stream, file_name);
			}
			else {
				websWrite(stream, "NoIcon");
			}
		}
		else {
			websWrite(stream, "NoIcon");
		}
#endif
		websWrite(stream, "\"NoIcon\"");

		app_method_hit = 2;
	}
	/* CPU Usage */
	else if (strcmp(func, "cpu_usage") == 0) {
		//ej_cpu_usage();
		unsigned long total, user, nice, system, idle, io, irq, softirq;
		char name[10];
		FILE *fp;
		fp = fopen("/proc/stat", "r");
		int i = 0, firstRow=1;

		if(fp == NULL)
			goto get_api_fail;

		while(fscanf(fp, "%s %lu %lu %lu %lu %lu %lu %lu \n", name, &user, &nice, &system, &idle, &io, &irq, &softirq) != EOF){
			if(strncmp(name, "cpu", 3) == 0){
				if(i == 0){
					i++;
					continue;
				}
				total = user + nice + system + idle + io + irq + softirq;
				if (firstRow == 1)
					firstRow = 0;
				else
					websWrite(stream, ",");

				websWrite(stream, "\"cpu%d_total\":\"%lu\",\"cpu%d_usage\":\"%lu\"", i, total, i, total - idle);
				i++;
			}
		}
		fclose(fp);

		app_method_hit = 1;
	}
	/* Network Traffic */
	else if (strcmp(func, "netdev") == 0) {
		//tcWebApi_UpdateNetdev(); //ej_netdev() -> ASUSWRT
		init_netdevData();

		//read wired traffic
#if defined(TCSUPPORT_MTK_INTERNAL_ETHER_SWITCH) || defined(MT7530_SUPPORT)
		#if defined(TCSUPPORT_CPU_MT7510)
		readWiredData_MTK_mt7510();
		#else
		readWiredData_MTK();
		#endif
#else
		readWiredData_RTK();
#endif
		//read ATM traffic
		readATMData_MTK();
		//read PTM traffic
#if defined(TCSUPPORT_CPU_MT7510)
		readPTMData_MTK_mt7510();
#else
		readPTMData_MTK();
#endif
		//read wireless traffic
#ifdef TCSUPPORT_WLAN_RT6856
		readWiFiData_RT6856();
#else
		readWiFiData_MTK();
#endif
		modifyTrafficData();

		//websWrite( stream, "\nnetdev = {\n" ); //only for browser
		websWrite( stream, "\"%s_rx\":\"0x%llx\",\"%s_tx\":\"0x%llx\"", "WIRED" , WiredData.rx, "WIRED", WiredData.tx );
		websWrite( stream, ",\"%s_rx\":\"0x%llx\",\"%s_tx\":\"0x%llx\"", "WIRELESS0" , Sum2GData.rx, "WIRELESS0", Sum2GData.tx );
		websWrite( stream, ",\"%s_rx\":\"0x%llx\",\"%s_tx\":\"0x%llx\"", "WIRELESS1" , Sum5GData.rx, "WIRELESS1", Sum5GData.tx );
		websWrite( stream, ",\"%s_rx\":\"0x%llx\",\"%s_tx\":\"0x%llx\"", "BRIDGE" , BridgeData.rx, "BRIDGE", BridgeData.tx );

		//Paul add 2012/10/23
		//websWrite( stream, ",\"%s_rx\":\"0x%llx\",\"%s_tx\":\"0x%llx\"", "ATM" , ATMData.rx, "ATM", ATMData.tx );
		//websWrite( stream, ",\"%s_rx\":\"0x%llx\",\"%s_tx\":\"0x%llx\"", "PTM0" , PTMData[0].rx, "PTM0", PTMData[0].tx );
		//websWrite( stream, ",\"%s_rx\":\"0x%llx\",\"%s_tx\":\"0x%llx\"", "PTM1" , PTMData[1].rx, "PTM1", PTMData[1].tx );
		//websWrite( stream, ",\"%s_rx\":\"0x%llx\",\"%s_tx\":\"0x%llx\"", "ETHERNETWAN" , EthernetWANData.rx, "ETHERNETWAN", EthernetWANData.tx );
		websWrite( stream, ",\"%s_rx\":\"0x%llx\",\"%s_tx\":\"0x%llx\"", "INTERNET" , ATMData.rx+PTMData[0].rx+PTMData[1].rx+EthernetWANData.rx, "INTERNET", ATMData.tx+PTMData[0].tx+PTMData[1].tx+EthernetWANData.tx );
		//websWrite( stream, "}" ); //only for browser

		app_method_hit = 1;
	}
	/* [GET] [QIS] detect wanlink status */
	/* [GET] [QIS] wanlink status */
	else if (strcmp(func, "wanlink_state") == 0) {
		//wanlink(); //wanlink_state_hook() -> ASUSWRT
		char tmp[32], prefix[32];
		int wan_state = -1, wan_sbstate = -1, wan_auxstate = -1;
		int unit, status = 0;
		char *statusstr[2] = { "Disconnected", "Connected" };
		char *type;
		char wan_proto[8];
		char ip[18] = "0.0.0.0";
		char netmask[18] = "0.0.0.0";
		char gateway[18] = "0.0.0.0";
		char dns[64] = "0.0.0.0";
		unsigned int lease = 0, expires = 0;
		char *xtype = "";
		char xip[18] = "0.0.0.0";
		char xnetmask[18] = "0.0.0.0";
		char xgateway[18] = "0.0.0.0";
		char xdns[32] = "0.0.0.0";
		unsigned int xlease = 0, xexpires = 0;

		unit = wan_primary_ifunit();

		memset(tmp, 0, sizeof(tmp));
		memset(wan_proto, 0, sizeof(wan_proto));
		wan_prefix(unit, prefix);

		wan_state = tcapi_get_int(WANDUCK, strcat_r(prefix, "state_t", tmp));
		wan_sbstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "sbstate_t", tmp));
		wan_auxstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "auxstate_t", tmp));

		websWrite(stream, "\"wanstate\":\"%d\",\n", wan_state);
		websWrite(stream, "\"wansbstate\":\"%d\",\n", wan_sbstate);
		websWrite(stream, "\"wanauxstate\":\"%d\",\n", wan_auxstate);
		websWrite(stream, "\"autodet_state\":\"0\",\n"); //not support autodet_state, so set 0 //tcWebApi_get("AutoPVC_Common","dsltmp_autodet_state","s")
		websWrite(stream, "\"autodet_auxstate\":\"0\",\n"); //not support autodet_auxstate, so set 0

		// wan_proto = nvram_safe_get(strcat_r(prefix, "proto", tmp));
		tcapi_get(WANDUCK, strcat_r(prefix, "proto", tmp), wan_proto);

		if (dualwan_unit__usbif(unit)) {
			if(wan_state == WAN_STATE_INITIALIZING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_CONNECTING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_DISCONNECTED){
				status = 0;
			}
			else if(wan_state == WAN_STATE_STOPPED){
				status = 0;
			}
			else if(wan_state == WAN_STATE_DISABLED){
				status = 0;
			}
			else{
				status = 1;
			}
		}
		else if(wan_state == WAN_STATE_DISABLED){
			status = 0;
		}
	// DSLTODO, need a better integration
#ifdef RTCONFIG_DSL
		// if dualwan & enable lan port as wan
		// it always report disconnected
		//Some AUXSTATE is displayed for reference only
		else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
			status = 0;
		}
#else
		//Some AUXSTATE is displayed for reference only
		else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
			status = 0;
		}
#endif
		else if(wan_auxstate == WAN_AUXSTATE_NO_INTERNET_ACTIVITY&&(tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOINTERNET)) {
			status = 0;
		}
		else if(!strcmp(wan_proto, "pppoe")
				// || !strcmp(wan_proto, "pptp")
				// || !strcmp(wan_proto, "l2tp")
				)
		{
			if(wan_state == WAN_STATE_INITIALIZING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_CONNECTING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_DISCONNECTED){
				status = 0;
			}
			else if(wan_state == WAN_STATE_STOPPED && wan_sbstate != WAN_STOPPED_REASON_PPP_LACK_ACTIVITY){
				status = 0;
			}
			else{
				status = 1;
			}
		}
		else{
			if(wan_state == WAN_STATE_STOPPED && wan_sbstate == WAN_STOPPED_REASON_INVALID_IPADDR){
				status = 0;
			}
			else if(wan_state == WAN_STATE_INITIALIZING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_CONNECTING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_DISCONNECTED){
				status = 0;
			}
			else {
				lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
				// treat short lease time as disconnected
				if(!strcmp(wan_proto, "dhcp") &&	//DHCP
					lease <= 60 &&
					is_private_subnet(nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)))
				) {
					status = 0;
				}
				else {
					status = 1;
				}
			}
		}

#ifdef RTCONFIG_USB_MODEM
		if (dualwan_unit__usbif(unit))
			type = "USB Modem";
		else
#endif
			type = wan_proto;

		if(status != 0){
			strncpy(ip, nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)), sizeof(ip)-1);
			strncpy(netmask, nvram_safe_get(strcat_r(prefix, "netmask", tmp)), sizeof(netmask)-1);
			strncpy(gateway, nvram_safe_get(strcat_r(prefix, "gateway", tmp)), sizeof(gateway)-1);
			strncpy(dns, nvram_safe_get(strcat_r(prefix, "dns", dns)), sizeof(dns)-1);
			lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
			if (lease > 0)
				expires = nvram_get_int(strcat_r(prefix, "expires", tmp)) - uptime();
		}

		if(!strcmp(argv[0],"appobj")){
			websWrite(stream, "\"wanlink_status\":\"%d\",\n", status);
			websWrite(stream, "\"wanlink_statusstr\":\"%s\",\n", statusstr[status]);
			websWrite(stream, "\"wanlink_type\":\"%s\",\n", type);
			websWrite(stream, "\"wanlink_ipaddr\":\"%s\",\n", ip);
			websWrite(stream, "\"wanlink_netmask\":\"%s\",\n", netmask);
			websWrite(stream, "\"wanlink_gateway\":\"%s\",\n", gateway);
			websWrite(stream, "\"wanlink_dns\":\"%s\",\n", dns);
			websWrite(stream, "\"wanlink_lease\":\"%d\",\n", lease);
			websWrite(stream, "\"wanlink_expires\":\"%d\",\n", expires);
			websWrite(stream, "\"is_private_subnet\":\"%d\",\n", is_private_subnet(ip));
		}else if(!strcmp(argv[0],"status"))
			websWrite(stream, "%d", status);
		else if(!strcmp(argv[0],"statusstr"))
			websWrite(stream, "%s", statusstr[status]);
		else if(!strcmp(argv[0],"type"))
			websWrite(stream, "%s", type);
		else if(!strcmp(argv[0],"ipaddr"))
			websWrite(stream, "%s", ip);
		else if(!strcmp(argv[0],"netmask"))
			websWrite(stream, "%s", netmask);
		else if(!strcmp(argv[0],"gateway"))
			websWrite(stream, "%s", gateway);
		else if(!strcmp(argv[0],"dns"))
			websWrite(stream, "%s", dns);
		else if(!strcmp(argv[0],"lease"))
			websWrite(stream, "%d", lease);
		else if(!strcmp(argv[0],"expires"))
			websWrite(stream, "%d", expires);
		else if(!strcmp(argv[0],"private_subnet"))
			websWrite(stream, "%d", is_private_subnet(ip));

#if 0//Note: not support VPN + DHCP Connection currently
		if (strcmp(wan_proto, "pppoe") == 0
			//|| strcmp(wan_proto, "pptp") == 0
			//|| strcmp(wan_proto, "l2tp") == 0
			)
		{
			char node[MAXLEN_NODE_NAME];
			int dhcpenable = 0;
			sprintf(node, "Wan_PVC%d", unit);
			tcapi_get(node, "PPPGETIP", tmp);
			dhcpenable = strcmp(tmp, "Dynamic") ? 0 : 1;
			xtype = (dhcpenable == 0) ? "static" :
				(strcmp(wan_proto, "pppoe") == 0 && tcapi_match(node, "vpndhcp", "0")) ? "" : /* zeroconf */
				"dhcp";
			strncpy(xip, nvram_safe_get(strcat_r(prefix, "xipaddr", tmp)), sizeof(xip)-1);
			strncpy(xnetmask, nvram_safe_get(strcat_r(prefix, "xnetmask", tmp)), sizeof(xnetmask)-1);
			strncpy(xgateway, nvram_safe_get(strcat_r(prefix, "xgateway", tmp)), sizeof(xgateway)-1);
			strncpy(xdns, nvram_safe_get(strcat_r(prefix, "xdns", tmp)), sizeof(xdns)-1);
			xlease = nvram_get_int(strcat_r(prefix, "xlease", tmp));
			if (xlease > 0)
				xexpires = nvram_get_int(strcat_r(prefix, "xexpires", tmp)) - uptime();
		}
#endif
		if(!strcmp(argv[0],"appobj")){
			websWrite(stream, "\"wanlink_xtype\":\"%s\",\n", xtype);
			websWrite(stream, "\"wanlink_xipaddr\":\"%s\",\n", xip);
			websWrite(stream, "\"wanlink_xnetmask\":\"%s\",\n", xnetmask);
			websWrite(stream, "\"wanlink_xgateway\":\"%s\",\n", xgateway);
			websWrite(stream, "\"wanlink_xdns\":\"%s\",\n", xdns);
			websWrite(stream, "\"wanlink_xlease\":\"%d\",\n", xlease);
			websWrite(stream, "\"wanlink_xexpires\":\"%d\"\n", xexpires);
		}else if(!strcmp(argv[0],"xtype"))
			websWrite(stream, "%s", xtype);
		else if(!strcmp(argv[0],"xipaddr"))
			websWrite(stream, "%s", xip);
		else if(!strcmp(argv[0],"xnetmask"))
			websWrite(stream, "%s", xnetmask);
		else if(!strcmp(argv[0],"xgateway"))
			websWrite(stream, "%s", xgateway);
		else if(!strcmp(argv[0],"xdns"))
			websWrite(stream, "%s", xdns);
		else if(!strcmp(argv[0],"xlease"))
			websWrite(stream, "%d", xlease);
		else if(!strcmp(argv[0],"xexpires"))
			websWrite(stream, "%d", xexpires);

		app_method_hit = 1;
	}
	/* RAM Usage */
	else if (strcmp(func, "memory_usage") == 0) {
		//ej_memory_usage();
		unsigned long total, used, mfree/*, shared, buffers, cached*/;
		char buf[80] = {0};
		FILE *fp;

		fp = fopen("/proc/meminfo", "r");
		if(fp == NULL)
			goto get_api_fail;

		fscanf(fp, "MemTotal: %lu %s\n", &total, buf);
		fscanf(fp, "MemFree: %lu %s\n", &mfree, buf);
		used = total - mfree;
		fclose(fp);
		websWrite(stream, "\"mem_total\":\"%lu\",\"mem_free\":\"%lu\",\"mem_used\":\"%lu\"", total, mfree, used);	

		app_method_hit = 1;
	}
	/* [GET] [AiProtection] Router Security - Anonymous login to FTP share disabled */
	/* [GET] [AiProtection] Router Security - Disable guest login for Network Place Share */
	else if (strcmp(func, "check_ftp_samba_anonymous") == 0) {
		//ej_check_ftp_samba_anonymous();
		int samba_mode=0, ftp_mode=0;
		if (strcmp(argv[0], "cifs") == 0) {
			if ((tcapi_get("Samba_Entry", "st_samba_force_mode", value) != TCAPI_PROCESS_OK) && (tcapi_get_int("Samba_Entry", "st_samba_mode") == 1)){
				samba_mode = 4;
			}else{
				samba_mode = tcapi_get_int("Samba_Entry", "st_samba_mode");
			}
			if(samba_mode == 2 || samba_mode == 4){
				websWrite(stream, "\"1\"");
			}else{
				websWrite(stream, "\"0\"");
			}
		}else if (strcmp(argv[0], "ftp") == 0){
			if ((tcapi_get("Samba_Entry", "st_ftp_force_mode", value) != TCAPI_PROCESS_OK) && (tcapi_get_int("Samba_Entry", "st_ftp_mode") == 1)){
				ftp_mode = 2;
			}else{
				ftp_mode = tcapi_get_int("Samba_Entry", "st_ftp_mode");
			}
			if (ftp_mode == 2){
				websWrite(stream, "\"1\"");
			}else{
				websWrite(stream, "\"0\"");
			}
		}
		app_method_hit = 1;
	}
	/* [GET] [AiProtection] Router Security - Default username or password changed */
	else if (strcmp(func, "check_acorpw") == 0) {
		//ej_check_acorpw(); -> ASUSWRT //ej_check_acpw(); -> DSL ASUSWRT
		char default_acpw[] = "admin"; //hardcode

		if(tcapi_match("Account_Entry0", "username", default_acpw) || tcapi_match("Account_Entry0", "web_passwd", default_acpw))
			websWrite(stream, "\"1\"");
		else
			websWrite(stream, "\"0\"");

		app_method_hit = 1;
	}
	/* [GET] [AiProtection] Router Security - Wireless encryption enabled */
	else if (strcmp(func, "check_wireless_encryption") == 0) {
		//ej_check_wireless_encryption();
		int unit=0, guest_id=0;
		char auth_mode[32];
		char prefix[32], bss_enabled[4];

		for(unit=0; unit<2; unit++) {
			for(guest_id=0; guest_id < MAX_NO_MSSID; guest_id++){
				if(guest_id){	//the Guest Network
					snprintf(prefix, sizeof(prefix), "wl%d.%d_auth_mode_x", unit, guest_id);
					tcapi_get("WLan_Entry0", prefix, auth_mode);
					snprintf(prefix, sizeof(prefix), "wl%d.%d_bss_enabled", unit, guest_id);
					tcapi_get("WLan_Entry0", prefix, bss_enabled);
				}
				else{	//bring up the primary of 2.4G and 5G.
					snprintf(prefix, sizeof(prefix), "wl%d_auth_mode_x", unit);
					tcapi_get("WLan_Entry0", prefix, auth_mode);
					snprintf(prefix, sizeof(prefix), "wl%d_radio_on", unit);
					tcapi_get("WLan_Entry0", prefix, bss_enabled);
				}
				if(strcmp(bss_enabled,"1") == 0
					&& strcmp(auth_mode,"WPA2PSK") != 0
					&& strcmp(auth_mode,"WPAPSKWPA2PSK") != 0
					&& strcmp(auth_mode,"WPA2") != 0
					&& strcmp(auth_mode,"WPA1WPA2") != 0)
				{
					websWrite(stream, "\"0\"");
					app_method_hit = 1;
					goto get_api_fail;
				}
			}
		}
		websWrite(stream, "\"1\"");
		app_method_hit = 1;
	}
	/* [GET] [AiProtection] Router Security - Wireless password strength check */
	else if (strcmp(func, "check_passwd_strength") == 0) {
		//ej_check_passwd_strength();
		int unit=0, guest_id=0;
		int nScore_total=0, nScore=0;
		char pwd[32];
		char prefix[32];
		int nLength=0, nConsecAlphaUC=0, nConsecCharType=0, nAlphaUC=0, nConsecAlphaLC=0, nAlphaLC=0, nMidChar=0, nConsecNumber=0, nNumber=0, nConsecSymbol=0, nSymbol=0, nRepChar=0, nUnqChar=0, nSeqAlpha=0, nSeqNumber=0, nSeqChar = 0, nSeqSymbol=0;
		int nMultMidChar=2, nMultLength=4, nMultNumber=4, nMultSymbol=6, nMultConsecAlphaUC=2, nMultConsecAlphaLC=2, nMultConsecNumber=2, nMultSeqAlpha=3, nMultSeqNumber=3, nMultSeqSymbol=3;
		int a=0, b=0, s=0, x=0;
		int nTmpAlphaUC = -1, nTmpAlphaLC = -1, nTmpNumber = -1, nTmpSymbol = -1;
		double nRepInc = 0.0;
		char sAlphas[] = "abcdefghijklmnopqrstuvwxyz";
		char sNumerics[] = "01234567890";
		char sSymbols[] = "~!@#$%^&*()_+";
		char pwd_s[128] = {0};
		char *pwd_st=NULL, *arrPwd=NULL;
		char auth_mode[32];
		char sFwd[4], sFwd_t[4], sRev[4];
		if(!strcmp(argv[0], "wl_key")){
			for(unit=0; unit < 2; unit++) {
				for(guest_id=0; guest_id < MAX_NO_MSSID; guest_id++){
					if(guest_id){	//the Guest Network
						snprintf(prefix, sizeof(prefix), "wl%d.%d_wpa_psk", unit, guest_id);
						tcapi_get("WLan_Entry0", prefix, pwd);
						snprintf(prefix, sizeof(prefix), "wl%d.%d_auth_mode_x", unit, guest_id);
						tcapi_get("WLan_Entry0", prefix, auth_mode);
					}
					else{	//bring up the primary of 2.4G and 5G.
						snprintf(prefix, sizeof(prefix), "wl%d_wpa_psk", unit);
						tcapi_get("WLan_Entry0", prefix, pwd);
						snprintf(prefix, sizeof(prefix), "wl%d_auth_mode_x", unit);
						tcapi_get("WLan_Entry0", prefix, auth_mode);
					}
					nLength=0; nConsecAlphaUC=0; nConsecCharType=0; nAlphaUC=0; nConsecAlphaLC=0; nAlphaLC=0; nMidChar=0; nConsecNumber=0; nNumber=0; nConsecSymbol=0; nSymbol=0; nRepChar=0; nUnqChar=0; nSeqAlpha=0; nSeqNumber=0; nSeqChar = 0; nSeqSymbol=0;
					nTmpAlphaUC = -1; nTmpAlphaLC = -1; nTmpNumber = -1; nTmpSymbol = -1;
				   if (strlen(pwd) && (!strcmp(auth_mode,"OPEN") || !strcmp(auth_mode,"WPA2PSK") || !strcmp(auth_mode,"WPAPSKWPA2PSK") || !strcmp(auth_mode,"WPA2") || !strcmp(auth_mode,"WPA1WPA2"))){
					nScore=0;
					nLength=0;
					pwd_st = pwd;
					arrPwd = pwd;
					nLength = strlen(pwd);
					nScore = nLength * nMultLength;

					/* Main calculation for strength:
							Loop through password to check for Symbol, Numeric, Lowercase and Uppercase pattern matches */
					for (a=0; a <nLength; a++){
						if(isupper(arrPwd[a])){
							if(nTmpAlphaUC != -1){
								if((nTmpAlphaUC + 1) == a){
									nConsecAlphaUC++;
									nConsecCharType++;
								}
							}
							nTmpAlphaUC = a;
							nAlphaUC++;
						}
						else if(islower(arrPwd[a])){
							if(nTmpAlphaLC != -1){
								if((nTmpAlphaLC + 1) == a){
									nConsecAlphaLC++;
									nConsecCharType++;
								}
							}
							nTmpAlphaLC = a;
							nAlphaLC++;
						}
						else if(isdigit(arrPwd[a])){
							if(a > 0 && a < (nLength - 1)){
								nMidChar++;
							}
							if(nTmpNumber != -1){
								if((nTmpNumber + 1) == a){
									nConsecNumber++;
									nConsecCharType++;
								}
							}
							nTmpNumber = a;
							nNumber++;
						}
						else if(!isalnum(arrPwd[a])){
							if(a > 0 && a < (nLength - 1))
							{
								nMidChar++;
							}
							if(nTmpSymbol != -1){
								if((nTmpSymbol + 1) == a){
									nConsecSymbol++;
									nConsecCharType++;
								}
							}
							nTmpSymbol = a;
							nSymbol++;
						}

						/* Internal loop through password to check for repeat characters */
						int bCharExists = 0;
						for (b=0; b < nLength; b++){
							if (arrPwd[a] == arrPwd[b] && a != b){ /* repeat character exists */
								bCharExists = 1;
								/*
								Calculate icrement deduction based on proximity to identical characters
								Deduction is incremented each time a new match is discovered
								Deduction amount is based on total password length divided by the
								difference of distance between currently selected match
								*/
								nRepInc += abs(nLength/(b-a));
							}
						}
						if (bCharExists == 1) {
							nRepChar++;
							nUnqChar = nLength-nRepChar;
							nRepInc = (nUnqChar > 0) ? ceil(nRepInc/(double)nUnqChar) : ceil(nRepInc); 
						}
					}

					for(x = 0; x < nLength; x++){
						pwd_s[x]= tolower(*pwd_st);
						pwd_st++;
					}

					/* Check for sequential alpha string patterns (forward and reverse) */
					for (s=0; s < 23; s++){
						memset(sFwd, 0, sizeof(sFwd));
						memset(sFwd_t, 0, sizeof(sFwd_t));
						memset(sRev, 0, sizeof(sRev));
						if(sAlphas+s != '\0'){
							strncpy(sFwd, sAlphas+s, 3);
							strncpy(sFwd_t, sFwd, 3);
						}
						strcpy(sRev, reverse_str(sFwd));

						if(strstr(pwd_s, sFwd_t) != NULL || strstr(pwd_s, sRev) != NULL){
							nSeqAlpha++;
							nSeqChar++;
						}
					}
					/* Check for sequential numeric string patterns (forward and reverse) */
					for (s=0; s < 8; s++) {
						memset(sFwd, 0, sizeof(sFwd));
						memset(sFwd_t, 0, sizeof(sFwd_t));
						memset(sRev, 0, sizeof(sRev));
						if(sNumerics+s != '\0'){
							strncpy(sFwd, sNumerics+s, 3);
							strncpy(sFwd_t, sFwd, 3);
						}
						strcpy(sRev, reverse_str(sFwd));
						if(strstr(pwd_s, sFwd_t) != NULL || strstr(pwd_s, sRev) != NULL){
							nSeqNumber++;
							nSeqChar++;
						}
					}
					/* Check for sequential symbol string patterns (forward and reverse) */
					for (s=0; s < 8; s++) {
						memset(sFwd, 0, sizeof(sFwd));
						memset(sFwd_t, 0, sizeof(sFwd_t));
						memset(sRev, 0, sizeof(sRev));
						if(sSymbols+s != '\0'){
							strncpy(sFwd, sSymbols+s, 3);
							strncpy(sFwd_t, sFwd, 3);
						}
						strcpy(sRev, reverse_str(sFwd));
						if(strstr(pwd_s, sFwd_t) != NULL || strstr(pwd_s, sRev) != NULL){
							nSeqSymbol++;
							nSeqChar++;
						}
					}
					/* Modify overall score value based on usage vs requirements */

					/* General point assignment */
					if (nAlphaUC > 0 && nAlphaUC < nLength){
						nScore = nScore + ((nLength - nAlphaUC) * 2);
					}
					if (nAlphaLC > 0 && nAlphaLC < nLength){
						nScore = nScore + ((nLength - nAlphaLC) * 2);
					}
					if (nNumber > 0 && nNumber < nLength){
						nScore = nScore + (nNumber * nMultNumber);
					}
					if (nSymbol > 0){
						nScore = nScore + (nSymbol * nMultSymbol);
					}
					if (nMidChar > 0){
						nScore = nScore + (nMidChar * nMultMidChar);
					}
					/* Point deductions for poor practices */
					if ((nAlphaLC > 0 || nAlphaUC > 0) && nSymbol == 0 && nNumber == 0) {  // Only Letters
						nScore = nScore - nLength;
					}
					if (nAlphaLC == 0 && nAlphaUC == 0 && nSymbol == 0 && nNumber > 0) {  // Only Numbers
						nScore = nScore - nLength;
					}
					if (nRepChar > 0) {  // Same character exists more than once
						nScore = nScore - nRepInc;
					}
					if (nConsecAlphaUC > 0) {  // Consecutive Uppercase Letters exist
					nScore = nScore - (nConsecAlphaUC * nMultConsecAlphaUC);
					}
					if (nConsecAlphaLC > 0) {  // Consecutive Lowercase Letters exist
						nScore = nScore - (nConsecAlphaLC * nMultConsecAlphaLC);
					}
					if (nConsecNumber > 0) {  // Consecutive Numbers exist
						nScore = nScore - (nConsecNumber * nMultConsecNumber);
					}
					if (nSeqAlpha > 0) {  // Sequential alpha strings exist (3 characters or more)
						nScore = nScore - (nSeqAlpha * nMultSeqAlpha);
					}
					if (nSeqNumber > 0) {  // Sequential numeric strings exist (3 characters or more)
						nScore = nScore - (nSeqNumber * nMultSeqNumber);
					}
					if (nSeqSymbol > 0) {  // Sequential symbol strings exist (3 characters or more)
						nScore = nScore - (nSeqSymbol * nMultSeqSymbol);
					}

					/* Determine complexity based on overall score */
					if (nScore > 100)
					{
						nScore = 100;
					}else if(nScore < 0)
					{
						nScore = 0;
					}
					nScore_total = nScore + nScore_total;
				   }else{
					nScore = 0;
				   }
				}
			}
		}
		websWrite(stream, "\"%d\"", (int)(nScore_total/unit));

		app_method_hit = 1;
	}
	/* [GET] [QIS] start_autodet */
	else if (strcmp(func, "start_autodet") == 0) {
#if 0 //ASUSWRT
		nvram_set("autodet_state", "");
		notify_rc_after_period_wait("start_autodet", 0);
#else
		system("killall -9 auto_det");
		system("/userfs/bin/auto_det &");
#endif
		app_method_hit = 1;
	}
	/* [GET] [WAN] WAN IP address */
	else if (strcmp(func, "wanlink") == 0) {
		//wanlink(); //wanlink_hook() -> ASUSWRT
		char tmp[32], prefix[32];
		int wan_state = -1, wan_sbstate = -1, wan_auxstate = -1;
		int unit, status = 0;
		char *statusstr[2] = { "Disconnected", "Connected" };
		char *type;
		char wan_proto[8];
		char ip[18] = "0.0.0.0";
		char netmask[18] = "0.0.0.0";
		char gateway[18] = "0.0.0.0";
		char dns[64] = "0.0.0.0";
		unsigned int lease = 0, expires = 0;
		char *xtype = "";
		char xip[18] = "0.0.0.0";
		char xnetmask[18] = "0.0.0.0";
		char xgateway[18] = "0.0.0.0";
		char xdns[32] = "0.0.0.0";
		unsigned int xlease = 0, xexpires = 0;

		unit = wan_primary_ifunit();

		memset(tmp, 0, sizeof(tmp));
		memset(wan_proto, 0, sizeof(wan_proto));
		wan_prefix(unit, prefix);

		wan_state = tcapi_get_int(WANDUCK, strcat_r(prefix, "state_t", tmp));
		wan_sbstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "sbstate_t", tmp));
		wan_auxstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "auxstate_t", tmp));

		// wan_proto = nvram_safe_get(strcat_r(prefix, "proto", tmp));
		tcapi_get(WANDUCK, strcat_r(prefix, "proto", tmp), wan_proto);

		if (dualwan_unit__usbif(unit)) {
			if(wan_state == WAN_STATE_INITIALIZING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_CONNECTING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_DISCONNECTED){
				status = 0;
			}
			else if(wan_state == WAN_STATE_STOPPED){
				status = 0;
			}
			else if(wan_state == WAN_STATE_DISABLED){
				status = 0;
			}
			else{
				status = 1;
			}
		}
		else if(wan_state == WAN_STATE_DISABLED){
			status = 0;
		}
	// DSLTODO, need a better integration
#ifdef RTCONFIG_DSL
		// if dualwan & enable lan port as wan
		// it always report disconnected
		//Some AUXSTATE is displayed for reference only
		else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
			status = 0;
		}
#else
		//Some AUXSTATE is displayed for reference only
		else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
			status = 0;
		}
#endif
		else if(wan_auxstate == WAN_AUXSTATE_NO_INTERNET_ACTIVITY&&(tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOINTERNET)) {
			status = 0;
		}
		else if(!strcmp(wan_proto, "pppoe")
				// || !strcmp(wan_proto, "pptp")
				// || !strcmp(wan_proto, "l2tp")
				)
		{
			if(wan_state == WAN_STATE_INITIALIZING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_CONNECTING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_DISCONNECTED){
				status = 0;
			}
			else if(wan_state == WAN_STATE_STOPPED && wan_sbstate != WAN_STOPPED_REASON_PPP_LACK_ACTIVITY){
				status = 0;
			}
			else{
				status = 1;
			}
		}
		else{
			if(wan_state == WAN_STATE_STOPPED && wan_sbstate == WAN_STOPPED_REASON_INVALID_IPADDR){
				status = 0;
			}
			else if(wan_state == WAN_STATE_INITIALIZING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_CONNECTING){
				status = 0;
			}
			else if(wan_state == WAN_STATE_DISCONNECTED){
				status = 0;
			}
			else {
				lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
				// treat short lease time as disconnected
				if(!strcmp(wan_proto, "dhcp") &&	//DHCP
					lease <= 60 &&
					is_private_subnet(nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)))
				) {
					status = 0;
				}
				else {
					status = 1;
				}
			}
		}

#ifdef RTCONFIG_USB_MODEM
		if (dualwan_unit__usbif(unit))
			type = "USB Modem";
		else
#endif
			type = wan_proto;

		if(status != 0){
			strncpy(ip, nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)), sizeof(ip)-1);
			strncpy(netmask, nvram_safe_get(strcat_r(prefix, "netmask", tmp)), sizeof(netmask)-1);
			strncpy(gateway, nvram_safe_get(strcat_r(prefix, "gateway", tmp)), sizeof(gateway)-1);
			strncpy(dns, nvram_safe_get(strcat_r(prefix, "dns", dns)), sizeof(dns)-1);
			lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
			if (lease > 0)
				expires = nvram_get_int(strcat_r(prefix, "expires", tmp)) - uptime();
		}

		if(argv[0] == NULL){
			websWrite(stream, "function wanlink_status() { return %d;}\n", status);
			websWrite(stream, "function wanlink_statusstr() { return '%s';}\n", statusstr[status]);
			websWrite(stream, "function wanlink_type() { return '%s';}\n", type);
			websWrite(stream, "function wanlink_ipaddr() { return '%s';}\n", ip);
			websWrite(stream, "function wanlink_netmask() { return '%s';}\n", netmask);
			websWrite(stream, "function wanlink_gateway() { return '%s';}\n", gateway);
			websWrite(stream, "function wanlink_dns() { return '%s';}\n", dns);
			websWrite(stream, "function wanlink_lease() { return %d;}\n", lease);
			websWrite(stream, "function wanlink_expires() { return %d;}\n", expires);
			websWrite(stream, "function is_private_subnet() { return '%d';}\n", is_private_subnet(ip));
		}else if(!strcmp(argv[0],"status"))
			websWrite(stream, "\"%d\"", status);
		else if(!strcmp(argv[0],"statusstr"))
			websWrite(stream, "\"%s\"", statusstr[status]);
		else if(!strcmp(argv[0],"type"))
			websWrite(stream, "\"%s\"", type);
		else if(!strcmp(argv[0],"ipaddr"))
			websWrite(stream, "\"%s\"", ip);
		else if(!strcmp(argv[0],"netmask"))
			websWrite(stream, "\"%s\"", netmask);
		else if(!strcmp(argv[0],"gateway"))
			websWrite(stream, "\"%s\"", gateway);
		else if(!strcmp(argv[0],"dns"))
			websWrite(stream, "\"%s\"", dns);
		else if(!strcmp(argv[0],"lease"))
			websWrite(stream, "\"%d\"", lease);
		else if(!strcmp(argv[0],"expires"))
			websWrite(stream, "\"%d\"", expires);
		else if(!strcmp(argv[0],"private_subnet"))
			websWrite(stream, "\"%d\"", is_private_subnet(ip));

#if 0//Note: not support VPN + DHCP Connection currently
		if (strcmp(wan_proto, "pppoe") == 0
			//|| strcmp(wan_proto, "pptp") == 0
			//|| strcmp(wan_proto, "l2tp") == 0
			)
		{
			char node[MAXLEN_NODE_NAME];
			int dhcpenable = 0;
			sprintf(node, "Wan_PVC%d", unit);
			tcapi_get(node, "PPPGETIP", tmp);
			dhcpenable = strcmp(tmp, "Dynamic") ? 0 : 1;
			xtype = (dhcpenable == 0) ? "static" :
				(strcmp(wan_proto, "pppoe") == 0 && tcapi_match(node, "vpndhcp", "0")) ? "" : /* zeroconf */
				"dhcp";
			strncpy(xip, nvram_safe_get(strcat_r(prefix, "xipaddr", tmp)), sizeof(xip)-1);
			strncpy(xnetmask, nvram_safe_get(strcat_r(prefix, "xnetmask", tmp)), sizeof(xnetmask)-1);
			strncpy(xgateway, nvram_safe_get(strcat_r(prefix, "xgateway", tmp)), sizeof(xgateway)-1);
			strncpy(xdns, nvram_safe_get(strcat_r(prefix, "xdns", tmp)), sizeof(xdns)-1);
			xlease = nvram_get_int(strcat_r(prefix, "xlease", tmp));
			if (xlease > 0)
				xexpires = nvram_get_int(strcat_r(prefix, "xexpires", tmp)) - uptime();
		}
#endif

		if(argv[0] == NULL){
			websWrite(stream, "function wanlink_xtype() { return '%s';}\n", xtype);
			websWrite(stream, "function wanlink_xipaddr() { return '%s';}\n", xip);
			websWrite(stream, "function wanlink_xnetmask() { return '%s';}\n", xnetmask);
			websWrite(stream, "function wanlink_xgateway() { return '%s';}\n", xgateway);
			websWrite(stream, "function wanlink_xdns() { return '%s';}\n", xdns);
			websWrite(stream, "function wanlink_xlease() { return %d;}\n", xlease);
			websWrite(stream, "function wanlink_xexpires() { return %d;}\n", xexpires);
		}else if(!strcmp(argv[0],"xtype"))
			websWrite(stream, "\"%s\"", xtype);
		else if(!strcmp(argv[0],"xipaddr"))
			websWrite(stream, "\"%s\"", xip);
		else if(!strcmp(argv[0],"xnetmask"))
			websWrite(stream, "\"%s\"", xnetmask);
		else if(!strcmp(argv[0],"xgateway"))
			websWrite(stream, "\"%s\"", xgateway);
		else if(!strcmp(argv[0],"xdns"))
			websWrite(stream, "\"%s\"", xdns);
		else if(!strcmp(argv[0],"xlease"))
			websWrite(stream, "\"%d\"", xlease);
		else if(!strcmp(argv[0],"xexpires"))
			websWrite(stream, "\"%d\"", xexpires);

		app_method_hit = 1;
	}
	/* DUT_RT_CheckAsusModel */
	else if (strcmp(func, "check_asus_model") == 0) 
	{
#ifdef RTCONFIG_HTTPS
		if (check_model_name()){
			websWrite(stream, "\"1\"");
		}else{
			websWrite(stream, "\"0\"");
		}
#else
		websWrite(stream, "\"1\"");
#endif
		app_method_hit = 1;
	}
	/* DUT_RT_DHCPLeaseList */
	else if (strcmp(func, "dhcpLeaseMacList") == 0) 
	{
		//IP_dhcpLeaseInfo()
		char buf[256]={0},mac[17], ip[16],expire[10], hostname[32];
		char *str;

		
		FILE *fp = fopen("/etc/udhcp_lease", "r");
		if(fp)
		{
			websWrite(stream, "[");
			while (fgets(buf, 256, fp)){
				memset(mac, 0, sizeof(mac));
				memset(ip, 0, sizeof(ip));
				memset(expire, 0, sizeof(expire));
				memset(hostname, 0, sizeof(hostname));
				sscanf(buf, "%s %s %s %s",mac, ip, expire, hostname);

				// Get the whole hostname including space
				if(strlen(hostname)) {
					str = strstr(buf, hostname);
					if(str && (*(str+strlen(str)-1) == 0x0a || *(str+strlen(str)-1) == 0x0d)){
						*(str+strlen(str)-1)=0x0;
						snprintf(hostname, sizeof(hostname), "%s", str);
					}
					char_to_ascii_safe(buf, hostname, strlen(hostname));
					websWrite(stream,"[\"%s\", \"%s\"],", ip, buf);
				}
				else
				{
					//error record in buf ex. [00:00:00:00:00:00 192.168.1.108 3935]
				}
			}
			fclose(fp);
			websWrite(stream, "[\"\",\"\"]]");
		}
		else
		{
			websWrite(stream, "[[\"\", \"\"]]");
		}

		app_method_hit = 1;
	}
	/* DUT_RT_BWDPI_ClientList */
	else if (strcmp(func, "bwdpi_device_info") == 0) 
	{ //ej_bwdpi_device() -> ASUSWRT
#if 0
		int retval = 0;
		char *hwaddr;

		// get device info of someone.
		hwaddr = websGetVar(wp, "client", "");
		
		//_dprintf("[httpd] history: hwaddr=%s.\n", hwaddr);
		get_device_hook(hwaddr, &retval, wp);
#endif
		websWrite(stream, "\"\"");

		app_method_hit = 2;
	}
	/* DUT_RT_GetBandwidthMonitor */
	else if (strcmp(func, "bwdpi_status") == 0) 
	{ //ej_bwdpi_status() -> ASUSWRT
		websWrite(stream, "\"\"");

		app_method_hit = 2;
	}

get_api_fail:
	if (app_method_hit == 0)
		//websWrite(stream,"");       //Not Support
		_dprintf("app_call: %s(%s): Not yet support\n", func, argv[0]);
	else if (app_method_hit == 2)
		//websWrite(stream,"");       //DSL model not support
		_dprintf("app_call: %s(%s): DSL model not support\n", func, argv[0]);

	if(strcmp(func, "nvram_get") == 0)
		websWrite(stream,"\"" );
	else if(argv[0] != NULL && strcmp(argv[0], "appobj") == 0)
		websWrite(stream,"}" );
}

static void do_appGet_cgi (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *pattern = NULL, *saveptr = NULL;
	int firstRow=1;
	char wp[1024] = {0}; //If too small, may cause boa crash

	pattern = get_param(g_var_post, "hook");
	//_dprintf("do_appGet_cgi: pattern_t=%s\n", pattern);
	char *pattern_t = strtok_r(pattern, ";", &saveptr);

	websWrite(wp,"{\n" );

	while (pattern_t != NULL){
		if (firstRow == 1)
			firstRow = 0;
		else
			websWrite(wp, ",\n");

		//websWrite(wp,"\"%s\":", pattern_t);

		app_call(pattern_t, wp);

		pattern_t = strtok_r(NULL, ";", &saveptr);
	}

	websWrite(wp,"\n}\n" );
}

static void do_appGet_image_path_cgi (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char wp[128] = {0};

	websWrite(wp,"{\n" );

	websWrite(wp, "\"IMAGE_MODEL_PRODUCT\":\"%s\",\n", IMAGE_MODEL_PRODUCT);
	websWrite(wp, "\"IMAGE_WANUNPLUG\":\"\",\n"); //not support now
	websWrite(wp, "\"IMAGE_ROUTER_MODE\":\"%s\",\n", IMAGE_ROUTER_MODE);
	websWrite(wp, "\"IMAGE_REPEATER_MODE\":\"%s\",\n", IMAGE_REPEATER_MODE);
	websWrite(wp, "\"IMAGE_AP_MODE\":\"%s\",\n", IMAGE_AP_MODE);
	websWrite(wp, "\"IMAGE_MEDIA_BRIDGE_MODE\":\"\"\n"); //not support now

	websWrite(wp,"\n}\n" );
}

static void do_upload_cgi (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	//dbgprintf("[%s, %d]\n", __FUNCTION__, __LINE__);
	eval("/usr/script/upload_config.sh");
	system("reboot");
}

static void wanstate (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	int unit;
	char tmp[32], prefix[32] = "wanXXXXXXXXXX_";
	// int wan_state = -1, wan_sbstate = -1, wan_auxstate = -1;
	char value[4], wp[256];

	memset(tmp, 0, sizeof(tmp));
	memset(prefix, 0, sizeof(prefix));
	memset(value, 0, sizeof(value));
	//current unit
	unit = wan_primary_ifunit();
	wan_prefix(unit, prefix);

	// wan_state = nvram_get_int(strcat_r(prefix, "state_t", tmp));
	// wan_sbstate = nvram_get_int(strcat_r(prefix, "sbstate_t", tmp));
	// wan_auxstate = nvram_get_int(strcat_r(prefix, "auxstate_t", tmp));

	// websWrite(wp, "wanstate = %d;\n", wan_state);
	// websWrite(wp, "wansbstate = %d;\n", wan_sbstate);
	// websWrite(wp, "wanauxstate = %d;\n", wan_auxstate);

	strcat_r(prefix, "state_t", tmp);
	if(!tcapi_get(WANDUCK, tmp, value)) {
		sprintf(wp, "wanstate = %s;\n", value);
		asp_send_response(NULL, wp, strlen(wp));
	}

	strcat_r(prefix, "sbstate_t", tmp);
	if(!tcapi_get(WANDUCK, tmp, value)) {
		sprintf(wp, "wansbstate = %s;\n", value);
		asp_send_response(NULL, wp, strlen(wp));
	}

	strcat_r(prefix, "auxstate_t", tmp);
	if(!tcapi_get(WANDUCK, tmp, value)) {
		sprintf(wp, "wanauxstate = %s;\n", value);
		asp_send_response(NULL, wp, strlen(wp));
	}

	// return 0;
}

static void set_current_active_wan (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char unit[4];
	if( tcapi_get_int("Wanduck_Common", "wan_primary") >=  WAN_UNIT_PTM0 ) {
		snprintf(unit, sizeof(unit), "%d", tcapi_get_int("Wanduck_Common", "wan_primary"));
		tcapi_set("WebCurSet_Entry", "wan_pvc", unit);
		tcapi_set("WebCurSet_Entry", "wan_pvc_ext", "0");
		dbgprintf("[%s(%d)] force set wan_pvc = %s, wan_pvc_ext = 0\n", __FUNCTION__, __LINE__, unit);
	} else {
		tcapi_set("WebCurSet_Entry", "wan_pvc", "0");
		dbgprintf("[%s(%d)] wan_pvc = 0\n", __FUNCTION__, __LINE__);
	}

	return;
	
}

static void wl_nband_info (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char buf[12];
	char value[MAX_VALUE];
	char *wl2g = NULL;
	char *wl5g = NULL;
	memset(buf, 0, sizeof(buf));
	tcapi_get("SysInfo_Entry", "rc_support", value);
	wl2g = strstr(value, "2.4G");
	wl5g = strstr(value, "5G");
	
	sprintf(buf, "[");
	if(wl2g != NULL) {
		strcat(buf, "\"2\"");
	}
	if(wl5g != NULL) {
		strcat(buf, ",\"1\"");
	}
	strcat(buf, "]");
	asp_send_response (NULL,buf,strlen(buf));
	
}
//for single wan
static void disable_other_wan (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *wanVCFlag = get_param(g_var_post, "wanVCFlag");
	char tmp[16] = {0};
	int CurSet_pvc = -1;
	int i;
	char prefix[16];
	char *active = NULL;
	char cmd[64] ={0};
	char transmode[8] = {0};
#ifdef TCSUPPORT_MULTISERVICE_ON_WAN
	//char *szServiceNum = get_param(g_var_post, "service_num");
	//int service_num = szServiceNum? atoi(szServiceNum): -1;
	int j;
#endif
	char brif[8];

	if(wanVCFlag && !strcmp(wanVCFlag, "3")) {	// apply wan
		if(!tcapi_get("WebCurSet_Entry", "wan_pvc", tmp)) {
			CurSet_pvc = atoi(tmp);

			tcapi_get("Wan_Common", "TransMode", transmode);
			if( !strncmp(transmode, "ATM", 3) )
				active = get_param(g_var_post, "wan_VCStatus");
			else
				active = get_param(g_var_post, "wan_TransStatus");
			if(active && strcmp(active, "Yes"))	//Current apply PVC is not Enable, then don't disable others Sam 2013/9/4
				return;

#ifdef RTCONFIG_DUALWAN
			tcapi_get("Dualwan_Entry", "wans_dualwan", tmp);
			if(strstr(tmp, "none") == NULL)	//dualwan enable
				if(CurSet_pvc == WAN_UNIT_ETH || CurSet_pvc == WAN_UNIT_USB || CurSet_pvc == WAN_UNIT_ETH_LAN)	//DSL can be changed
					return;
#endif

#ifdef TCSUPPORT_WAN_PTM
			//save DSLMode
			if( !strncmp(transmode, "ATM", 3) ) {
				tcapi_set("Wan_Common", "DSLMode", "ADSL");
#ifdef RTCONFIG_DUALWAN
				if(strstr(tmp, "none") == NULL) {	//dualwan enable
					if(tcapi_match("Wanduck_Common", "wan_primary", "8"))
						tcapi_set("Wanduck_Common", "wan_primary", "0");
					else if(tcapi_match("Wanduck_Common", "wan_secondary", "8"))
						tcapi_set("Wanduck_Common", "wan_secondary", "0");
				}
#endif
			}
			else if( !strncmp(transmode, "PTM", 3) ) {
				tcapi_set("Wan_Common", "DSLMode", "VDSL");
#ifdef RTCONFIG_DUALWAN
				if(strstr(tmp, "none") == NULL) {	//dualwan enable
					if(tcapi_match("Wanduck_Common", "wan_primary", "0"))
						tcapi_set("Wanduck_Common", "wan_primary", "8");
					else if(tcapi_match("Wanduck_Common", "wan_secondary", "0"))
						tcapi_set("Wanduck_Common", "wan_secondary", "8");
				}
#endif
			}
#endif

			for(i = 0; i < WAN_UNIT_MAX; i++) {
				if(i == CurSet_pvc)
					continue;
				else if(CurSet_pvc >= 0 && CurSet_pvc < 8 && i >= 0 && i < 8)	//pvc 0~7 can active at the same time
					continue;
			#if defined(RTCONFIG_DUALWAN)
				else if( (i == WAN_UNIT_ETH || i == WAN_UNIT_USB || i == WAN_UNIT_ETH_LAN) && (strstr(tmp, "none") == NULL))
					continue;	//DSL can be changed, but if dualwan enable, don't disable ETH or USB
			#endif
				else {
				#ifdef TCSUPPORT_MULTISERVICE_ON_WAN
					if(i == WAN_UNIT_PTM0 || i == WAN_UNIT_ETH) {	//multiservice only for PTM, WAN
						for(j = 0; j < 8; j++) {
							sprintf(prefix, "WanExt_PVC%de%d", i, j);
							snprintf(cmd, sizeof(cmd)-1, "/usr/script/wan_stop.sh %d %d", i, j);
							if(tcapi_match(prefix, "Active", "Yes")) {
								system(cmd);
								tcapi_set(prefix, "Active", "No");

								if(tcapi_match(prefix, "ISP", "3"))	//reset bridge
								{
									snprintf(brif, sizeof(brif), "br%d", tcapi_get_int(prefix, "bridge_lan_port"));
									snprintf(cmd, sizeof(cmd), "brctl delif %s nas%d_%d", brif, i, j);
									system(cmd);
									snprintf(cmd, sizeof(cmd), "brctl delif %s lan%d", brif, tcapi_get_int(prefix, "bridge_lan_port"));
									system(cmd);									
									snprintf(cmd, sizeof(cmd), "brctl addif br0 lan%d", tcapi_get_int(prefix, "bridge_lan_port"));
									system(cmd);									
								}
								
								memset(cmd, 0, sizeof(cmd));
								sprintf(cmd, "/usr/bin/smuxctl rem nas%d_%d", i, j);
								system(cmd);
							}
						}
					}
					else {
						sprintf(prefix, "Wan_PVC%d", i);
						snprintf(cmd, sizeof(cmd)-1, "/usr/script/wan_stop.sh %d %d", i, 0);
						if(tcapi_match(prefix, "Active", "Yes")) {

							if(tcapi_match(prefix, "ISP", "3"))	//reset bridge
							{
								snprintf(brif, sizeof(brif), "br%d", tcapi_get_int(prefix, "bridge_lan_port"));
								snprintf(cmd, sizeof(cmd), "brctl delif %s nas%d", brif, i);
								system(cmd);
								snprintf(cmd, sizeof(cmd), "brctl delif %s lan%d", brif, tcapi_get_int(prefix, "bridge_lan_port"));
								system(cmd);									
								snprintf(cmd, sizeof(cmd), "brctl addif br0 lan%d", tcapi_get_int(prefix, "bridge_lan_port"));
								system(cmd);									
							}
							
							system(cmd);
							tcapi_set(prefix, "Active", "No");
						#if RTCONFIG_USB_MODEM
							if(i == WAN_UNIT_USB)
							{
								tcapi_set("USBModem_Entry", "modem_enable", "0");
							}
						#endif /* RTCONFIG_USB_MODEM */
						}
					}
				#else
					sprintf(prefix, "Wan_PVC%d", i);
					snprintf(cmd, sizeof(cmd)-1, "/usr/script/wan_stop.sh %d", i);
					if(tcapi_match(prefix, "Active", "Yes")) {
						system(cmd);
						tcapi_set(prefix, "Active", "No");
					#if RTCONFIG_USB_MODEM
						if(i == WAN_UNIT_USB)
						{
							tcapi_set("USBModem_Entry", "modem_enable", "0");
						}
					#endif /* RTCONFIG_USB_MODEM */
					}
				#endif
				#if defined(TCSUPPORT_WAN_ETHER_LAN) && defined(TCSUPPORT_MTK_INTERNAL_ETHER_SWITCH)
					if(i == WAN_UNIT_ETH_LAN) {
						set_lanwan_if(get_cur_lanwan_port(), -1);
					}
				#endif
				}
			}

#ifdef RTCONFIG_DUALWAN
			//PVC changed, then also update primary of wans_dualwan
			tcapi_get("Dualwan_Entry", "wans_dualwan", tmp);
			if(strstr(tmp, "none") != NULL)	//dualwan disable
			{
				switch(get_dualwan_by_pvcunit(CurSet_pvc)) {
					case WANS_DUALWAN_IF_DSL:
						tcapi_set("Dualwan_Entry", "wans_dualwan", "dsl none");
						break;
					case WANS_DUALWAN_IF_WAN:
						tcapi_set("Dualwan_Entry", "wans_dualwan", "wan none");
						break;
					case WANS_DUALWAN_IF_USB:
						tcapi_set("Dualwan_Entry", "wans_dualwan", "usb none");
						break;
					case WANS_DUALWAN_IF_LAN:
						tcapi_set("Dualwan_Entry", "wans_dualwan", "lan none");
						break;
				}
			}
#endif

			//commit Adsl should after setting Active
			#ifdef TCSUPPORT_DSL_POWER_SAVING
			if( tcapi_match("Adsl_Entry", "dslx_power_saving", "1") ) {
				if(CurSet_pvc == WAN_UNIT_ATM || CurSet_pvc == WAN_UNIT_PTM0 || CurSet_pvc == WAN_UNIT_PTM1)	//DSL WAN, "open" command should not effect DSL if already up previously
					tcapi_commit("Adsl");
				else
					eval("wan", "adsl", "close");
			}
			#endif
		}
	}
}

static void wanlink (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char tmp[32], prefix[32] = "wanXXXXXXXXXX_";
	int wan_state = -1, wan_sbstate = -1, wan_auxstate = -1;
	int unit, status = 0;
	char *statusstr[2] = { "Disconnected", "Connected" };
	char *type;
	char wan_proto[8];
	char ip[18] = "0.0.0.0";
	char netmask[18] = "0.0.0.0";
	char gateway[18] = "0.0.0.0";
	char dns[64] = "0.0.0.0";
	unsigned int lease = 0, expires = 0;
	char *xtype = "";
	char xip[18] = "0.0.0.0";
	char xnetmask[18] = "0.0.0.0";
	char xgateway[18] = "0.0.0.0";
	char xdns[32] = "0.0.0.0";
	unsigned int xlease = 0, xexpires = 0;
	char wp[256] = {0};

	unit = wan_primary_ifunit();

	memset(tmp, 0, sizeof(tmp));
	memset(wan_proto, 0, sizeof(wan_proto));
	wan_prefix(unit, prefix);

	wan_state = tcapi_get_int(WANDUCK, strcat_r(prefix, "state_t", tmp));
	wan_sbstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "sbstate_t", tmp));
	wan_auxstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "auxstate_t", tmp));

	// wan_proto = nvram_safe_get(strcat_r(prefix, "proto", tmp));
	tcapi_get(WANDUCK, strcat_r(prefix, "proto", tmp), wan_proto);

	if (dualwan_unit__usbif(unit)) {
		if(wan_state == WAN_STATE_INITIALIZING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_CONNECTING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISCONNECTED){
			status = 0;
		}
		else if(wan_state == WAN_STATE_STOPPED){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISABLED){
			status = 0;
		}
		else{
			status = 1;
		}
	}
	else if(wan_state == WAN_STATE_DISABLED){
		status = 0;
	}
// DSLTODO, need a better integration
#ifdef RTCONFIG_DSL
	// if dualwan & enable lan port as wan
	// it always report disconnected
	//Some AUXSTATE is displayed for reference only
	else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
		status = 0;
	}
#else
	//Some AUXSTATE is displayed for reference only
	else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
		status = 0;
	}
#endif
	else if(wan_auxstate == WAN_AUXSTATE_NO_INTERNET_ACTIVITY&&(tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOINTERNET)) {
		status = 0;
	}
	else if(!strcmp(wan_proto, "pppoe")
			// || !strcmp(wan_proto, "pptp")
			// || !strcmp(wan_proto, "l2tp")
			)
	{
		if(wan_state == WAN_STATE_INITIALIZING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_CONNECTING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISCONNECTED){
			status = 0;
		}
		else if(wan_state == WAN_STATE_STOPPED && wan_sbstate != WAN_STOPPED_REASON_PPP_LACK_ACTIVITY){
			status = 0;
		}
		else{
			status = 1;
		}
	}
	else{
		if(wan_state == WAN_STATE_STOPPED && wan_sbstate == WAN_STOPPED_REASON_INVALID_IPADDR){
			status = 0;
		}
		else if(wan_state == WAN_STATE_INITIALIZING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_CONNECTING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISCONNECTED){
			status = 0;
		}
		else {
			lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
			// treat short lease time as disconnected
			if(!strcmp(wan_proto, "dhcp") &&	//DHCP
				lease <= 60 &&
				is_private_subnet(nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)))
			) {
				status = 0;
			}
			else {
				status = 1;
			}
		}
	}

#ifdef RTCONFIG_USB_MODEM
	if (dualwan_unit__usbif(unit))
		type = "USB Modem";
	else
#endif
		type = wan_proto;

	if(status != 0){
		strncpy(ip, nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)), sizeof(ip)-1);
		strncpy(netmask, nvram_safe_get(strcat_r(prefix, "netmask", tmp)), sizeof(netmask)-1);
		strncpy(gateway, nvram_safe_get(strcat_r(prefix, "gateway", tmp)), sizeof(gateway)-1);
		strncpy(dns, nvram_safe_get(strcat_r(prefix, "dns", dns)), sizeof(dns)-1);
		lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
		if (lease > 0)
			expires = nvram_get_int(strcat_r(prefix, "expires", tmp)) - uptime();
	}

	websWrite(wp, "function wanlink_status() { return %d;}\n", status);
	websWrite(wp, "function wanlink_statusstr() { return '%s';}\n", statusstr[status]);
	websWrite(wp, "function wanlink_type() { return '%s';}\n", type);
	websWrite(wp, "function wanlink_ipaddr() { return '%s';}\n", ip);
	websWrite(wp, "function wanlink_netmask() { return '%s';}\n", netmask);
	websWrite(wp, "function wanlink_gateway() { return '%s';}\n", gateway);
	websWrite(wp, "function wanlink_dns() { return '%s';}\n", dns);
	websWrite(wp, "function wanlink_lease() { return %d;}\n", lease);
	websWrite(wp, "function wanlink_expires() { return %d;}\n", expires);
	websWrite(wp, "function is_private_subnet() { return '%d';}\n", is_private_subnet(ip));

#if 0//Note: not support VPN + DHCP Connection currently
	if (strcmp(wan_proto, "pppoe") == 0
		//|| strcmp(wan_proto, "pptp") == 0
		//|| strcmp(wan_proto, "l2tp") == 0
		)
	{
		char node[MAXLEN_NODE_NAME];
		int dhcpenable = 0;
		sprintf(node, "Wan_PVC%d", unit);
		tcapi_get(node, "PPPGETIP", tmp);
		dhcpenable = strcmp(tmp, "Dynamic") ? 0 : 1;
		xtype = (dhcpenable == 0) ? "static" :
			(strcmp(wan_proto, "pppoe") == 0 && tcapi_match(node, "vpndhcp", "0")) ? "" : /* zeroconf */
			"dhcp";
		strncpy(xip, nvram_safe_get(strcat_r(prefix, "xipaddr", tmp)), sizeof(xip)-1);
		strncpy(xnetmask, nvram_safe_get(strcat_r(prefix, "xnetmask", tmp)), sizeof(xnetmask)-1);
		strncpy(xgateway, nvram_safe_get(strcat_r(prefix, "xgateway", tmp)), sizeof(xgateway)-1);
		strncpy(xdns, nvram_safe_get(strcat_r(prefix, "xdns", tmp)), sizeof(xdns)-1);
		xlease = nvram_get_int(strcat_r(prefix, "xlease", tmp));
		if (xlease > 0)
			xexpires = nvram_get_int(strcat_r(prefix, "xexpires", tmp)) - uptime();
	}
#endif

	websWrite(wp, "function wanlink_xtype() { return '%s';}\n", xtype);
	websWrite(wp, "function wanlink_xipaddr() { return '%s';}\n", xip);
	websWrite(wp, "function wanlink_xnetmask() { return '%s';}\n", xnetmask);
	websWrite(wp, "function wanlink_xgateway() { return '%s';}\n", xgateway);
	websWrite(wp, "function wanlink_xdns() { return '%s';}\n", xdns);
	websWrite(wp, "function wanlink_xlease() { return %d;}\n", xlease);
	websWrite(wp, "function wanlink_xexpires() { return %d;}\n", xexpires);

	return;
}

static void first_wanlink (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char tmp[32], prefix[32] = "wanXXXXXXXXXX_";
	int wan_state = -1, wan_sbstate = -1, wan_auxstate = -1;
	int unit, status = 0;
	char *statusstr[2] = { "Disconnected", "Connected" };
	char *type;
	char wan_proto[8];
	char ip[18] = "0.0.0.0";
	char netmask[18] = "0.0.0.0";
	char gateway[18] = "0.0.0.0";
	char dns[64] = "0.0.0.0";
	unsigned int lease = 0, expires = 0;
	char *xtype = "";
	char xip[18] = "0.0.0.0";
	char xnetmask[18] = "0.0.0.0";
	char xgateway[18] = "0.0.0.0";
	char xdns[32] = "0.0.0.0";
	unsigned int xlease = 0, xexpires = 0;
	char wp[256] = {0};

	unit = wan_primary_pvcunit();

	memset(tmp, 0, sizeof(tmp));
	memset(wan_proto, 0, sizeof(wan_proto));
	wan_prefix(unit, prefix);

	wan_state = tcapi_get_int(WANDUCK, strcat_r(prefix, "state_t", tmp));
	wan_sbstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "sbstate_t", tmp));
	wan_auxstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "auxstate_t", tmp));

	// wan_proto = nvram_safe_get(strcat_r(prefix, "proto", tmp));
	tcapi_get(WANDUCK, strcat_r(prefix, "proto", tmp), wan_proto);

	if (dualwan_unit__usbif(unit)) {
		if(wan_state == WAN_STATE_INITIALIZING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_CONNECTING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISCONNECTED){
			status = 0;
		}
		else if(wan_state == WAN_STATE_STOPPED){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISABLED){
			status = 0;
		}
		else{
			status = 1;
		}
	}
	else if(wan_state == WAN_STATE_DISABLED){
		status = 0;
	}
// DSLTODO, need a better integration
#ifdef RTCONFIG_DSL
	// if dualwan & enable lan port as wan
	// it always report disconnected
	//Some AUXSTATE is displayed for reference only
	else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
		status = 0;
	}
#else
	//Some AUXSTATE is displayed for reference only
	else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
		status = 0;
	}
#endif
	else if(wan_auxstate == WAN_AUXSTATE_NO_INTERNET_ACTIVITY&&(tcapi_get_int(WANDUCK, "web_redirect")&WEBREDIRECT_FLAG_NOINTERNET)) {
		status = 0;
	}
	else if(!strcmp(wan_proto, "pppoe")
			// || !strcmp(wan_proto, "pptp")
			// || !strcmp(wan_proto, "l2tp")
			)
	{
		if(wan_state == WAN_STATE_INITIALIZING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_CONNECTING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISCONNECTED){
			status = 0;
		}
		else if(wan_state == WAN_STATE_STOPPED && wan_sbstate != WAN_STOPPED_REASON_PPP_LACK_ACTIVITY){
			status = 0;
		}
		else{
			status = 1;
		}
	}
	else{
		if(wan_state == WAN_STATE_STOPPED && wan_sbstate == WAN_STOPPED_REASON_INVALID_IPADDR){
			status = 0;
		}
		else if(wan_state == WAN_STATE_INITIALIZING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_CONNECTING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISCONNECTED){
			status = 0;
		}
		else {
			lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
			// treat short lease time as disconnected
			if(!strcmp(wan_proto, "dhcp") &&	//DHCP
				lease <= 60 &&
				is_private_subnet(nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)))
			) {
				status = 0;
			}
			else {
				status = 1;
			}
		}
	}

#ifdef RTCONFIG_USB_MODEM
	if (dualwan_unit__usbif(unit))
		type = "USB Modem";
	else
#endif
		type = wan_proto;

	if(status != 0){
		strncpy(ip, nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)), sizeof(ip)-1);
		strncpy(netmask, nvram_safe_get(strcat_r(prefix, "netmask", tmp)), sizeof(netmask)-1);
		strncpy(gateway, nvram_safe_get(strcat_r(prefix, "gateway", tmp)), sizeof(gateway)-1);
		strncpy(dns, nvram_safe_get(strcat_r(prefix, "dns", dns)), sizeof(dns)-1);
		lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
		if (lease > 0)
			expires = nvram_get_int(strcat_r(prefix, "expires", tmp)) - uptime();
	}

	websWrite(wp, "function first_wanlink_status() { return %d;}\n", status);
	websWrite(wp, "function first_wanlink_statusstr() { return '%s';}\n", statusstr[status]);
	websWrite(wp, "function first_wanlink_type() { return '%s';}\n", type);
	websWrite(wp, "function first_wanlink_ipaddr() { return '%s';}\n", ip);
	websWrite(wp, "function first_wanlink_netmask() { return '%s';}\n", netmask);
	websWrite(wp, "function first_wanlink_gateway() { return '%s';}\n", gateway);
	websWrite(wp, "function first_wanlink_dns() { return '%s';}\n", dns);
	websWrite(wp, "function first_wanlink_lease() { return %d;}\n", lease);
	websWrite(wp, "function first_wanlink_expires() { return %d;}\n", expires);

#if 0//Note: not support VPN + DHCP Connection currently
	if (strcmp(wan_proto, "pppoe") == 0
		//|| strcmp(wan_proto, "pptp") == 0
		//|| strcmp(wan_proto, "l2tp") == 0
		)
	{
		char node[MAXLEN_NODE_NAME];
		int dhcpenable = 0;
		sprintf(node, "Wan_PVC%d", unit);
		tcapi_get(node, "PPPGETIP", tmp);
		dhcpenable = strcmp(tmp, "Dynamic") ? 0 : 1;
		xtype = (dhcpenable == 0) ? "static" :
			(strcmp(wan_proto, "pppoe") == 0 && tcapi_match(node, "vpndhcp", "0")) ? "" : /* zeroconf */
			"dhcp";
		strncpy(xip, nvram_safe_get(strcat_r(prefix, "xipaddr", tmp)), sizeof(xip)-1);
		strncpy(xnetmask, nvram_safe_get(strcat_r(prefix, "xnetmask", tmp)), sizeof(xnetmask)-1);
		strncpy(xgateway, nvram_safe_get(strcat_r(prefix, "xgateway", tmp)), sizeof(xgateway)-1);
		strncpy(xdns, nvram_safe_get(strcat_r(prefix, "xdns", tmp)), sizeof(xdns)-1);
		xlease = nvram_get_int(strcat_r(prefix, "xlease", tmp));
		if (xlease > 0)
			xexpires = nvram_get_int(strcat_r(prefix, "xexpires", tmp)) - uptime();
	}
#endif

	websWrite(wp, "function first_wanlink_xtype() { return '%s';}\n", xtype);
	websWrite(wp, "function first_wanlink_xipaddr() { return '%s';}\n", xip);
	websWrite(wp, "function first_wanlink_xnetmask() { return '%s';}\n", xnetmask);
	websWrite(wp, "function first_wanlink_xgateway() { return '%s';}\n", xgateway);
	websWrite(wp, "function first_wanlink_xdns() { return '%s';}\n", xdns);
	websWrite(wp, "function first_wanlink_xlease() { return %d;}\n", xlease);
	websWrite(wp, "function first_wanlink_xexpires() { return %d;}\n", xexpires);

	return;
}

static void secondary_wanlink (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char wp[256] = {0};
#ifdef RTCONFIG_DUALWAN
	char tmp[100], prefix[] = "wanXXXXXXXXXX_";
	int wan_state = -1, wan_sbstate = -1, wan_auxstate = -1;
	int unit, status = 0;
	char *statusstr[2] = { "Disconnected", "Connected" };
	char *type;
	char wan_proto[8];
	char ip[18] = "0.0.0.0";
	char netmask[18] = "0.0.0.0";
	char gateway[18] = "0.0.0.0";
	char dns[64] = "0.0.0.0";
	unsigned int lease = 0, expires = 0;
	char *xtype = "";
	char xip[64] = "0.0.0.0";
	char xnetmask[18] = "0.0.0.0";
	char xgateway[18] = "0.0.0.0";
	char xdns[18] = "0.0.0.0";
	unsigned int xlease = 0, xexpires = 0;

	/* current unit */
	unit = wan_secondary_pvcunit();
	wan_prefix(unit, prefix);

	wan_state = nvram_get_int(strcat_r(prefix, "state_t", tmp));
	wan_sbstate = nvram_get_int(strcat_r(prefix, "sbstate_t", tmp));
	wan_auxstate = nvram_get_int(strcat_r(prefix, "auxstate_t", tmp));

	//wan_proto = nvram_safe_get(strcat_r(prefix, "proto", tmp));
	memset(wan_proto, 0, sizeof(wan_proto));
	tcapi_get(WANDUCK, strcat_r(prefix, "proto", tmp), wan_proto);

	if (dualwan_unit__usbif(unit)) {
		if(wan_state == WAN_STATE_INITIALIZING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_CONNECTING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISCONNECTED){
			status = 0;
		}
		else if(wan_state == WAN_STATE_STOPPED){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISABLED){
			status = 0;
		}
		else{
			status = 1;
		}
	}
	else if(wan_state == WAN_STATE_DISABLED){
		status = 0;
	}
// DSLTODO, need a better integration
#ifdef RTCONFIG_DSL
	// if dualwan & enable lan port as wan
	// it always report disconnected
	//Some AUXSTATE is displayed for reference only
	else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (nvram_get_int("web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
		status = 0;
	}
#else
	//Some AUXSTATE is displayed for reference only
	else if(wan_auxstate == WAN_AUXSTATE_NOPHY && (nvram_get_int("web_redirect")&WEBREDIRECT_FLAG_NOLINK)) {
		status = 0;
	}
#endif
	else if(wan_auxstate == WAN_AUXSTATE_NO_INTERNET_ACTIVITY&&(nvram_get_int("web_redirect")&WEBREDIRECT_FLAG_NOINTERNET)) {
		status = 0;
	}
	else if(!strcmp(wan_proto, "pppoe")
			//|| !strcmp(wan_proto, "pptp")
			//|| !strcmp(wan_proto, "l2tp")
			)
	{
		if(wan_state == WAN_STATE_INITIALIZING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_CONNECTING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISCONNECTED){
			status = 0;
		}
		else if(wan_state == WAN_STATE_STOPPED && wan_sbstate != WAN_STOPPED_REASON_PPP_LACK_ACTIVITY){
			status = 0;
		}
		else{
			status = 1;
		}
	}
	else{
		if(wan_state == WAN_STATE_STOPPED && wan_sbstate == WAN_STOPPED_REASON_INVALID_IPADDR){
			status = 0;
		}
		else if(wan_state == WAN_STATE_INITIALIZING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_CONNECTING){
			status = 0;
		}
		else if(wan_state == WAN_STATE_DISCONNECTED){
			status = 0;
		}
		else {
			lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
			// treat short lease time as disconnected
			if(!strcmp(wan_proto, "dhcp") &&	//DHCP
				lease <= 60 &&
				is_private_subnet(nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)))
			) {
				status = 0;
			}
			else {
				status = 1;
			}
		}
	}

	if (dualwan_unit__usbif(unit))
		type = "USB Modem";
	else
		type = wan_proto;

	if(status != 0){
		strncpy(ip, nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)), sizeof(ip)-1);
		strncpy(netmask, nvram_safe_get(strcat_r(prefix, "netmask", tmp)), sizeof(netmask)-1);
		strncpy(gateway, nvram_safe_get(strcat_r(prefix, "gateway", tmp)), sizeof(gateway)-1);
		strncpy(dns, nvram_safe_get(strcat_r(prefix, "dns", tmp)), sizeof(dns)-1);
		lease = nvram_get_int(strcat_r(prefix, "lease", tmp));
		if (lease > 0)
			expires = nvram_get_int(strcat_r(prefix, "expires", tmp)) - uptime();
	}

	websWrite(wp, "function secondary_wanlink_status() { return %d;}\n", status);
	websWrite(wp, "function secondary_wanlink_statusstr() { return '%s';}\n", statusstr[status]);
	websWrite(wp, "function secondary_wanlink_type() { return '%s';}\n", type);
	websWrite(wp, "function secondary_wanlink_ipaddr() { return '%s';}\n", ip);
	websWrite(wp, "function secondary_wanlink_netmask() { return '%s';}\n", netmask);
	websWrite(wp, "function secondary_wanlink_gateway() { return '%s';}\n", gateway);
	websWrite(wp, "function secondary_wanlink_dns() { return '%s';}\n", nvram_safe_get(strcat_r(prefix, "dns", tmp)));
	websWrite(wp, "function secondary_wanlink_lease() { return %d;}\n", lease);
	websWrite(wp, "function secondary_wanlink_expires() { return %d;}\n", expires);
	websWrite(wp, "function is_private_subnet() { return %d;}\n", is_private_subnet(nvram_safe_get(strcat_r(prefix, "ipaddr", tmp))));

#if 0//Note: not support VPN + DHCP Connection currently
	if (strcmp(wan_proto, "pppoe") == 0
			//|| strcmp(wan_proto, "pptp") == 0
			//|| strcmp(wan_proto, "l2tp") == 0
		)
	{
		char node[MAXLEN_NODE_NAME];
		int dhcpenable = 0;
		sprintf(node, "Wan_PVC%d", unit);
		tcapi_get(node, "PPPGETIP", tmp);
		dhcpenable = strcmp(tmp, "Dynamic") ? 0 : 1;
		xtype = (dhcpenable == 0) ? "static" :
			(strcmp(wan_proto, "pppoe") == 0 && tcapi_match(node, "vpndhcp", "0")) ? "" : /* zeroconf */
			"dhcp";
		strncpy(xip, nvram_safe_get(strcat_r(prefix, "xipaddr", tmp)), sizeof(xip)-1);
		strncpy(xnetmask, nvram_safe_get(strcat_r(prefix, "xnetmask", tmp)), sizeof(xnetmask)-1);
		strncpy(xgateway, nvram_safe_get(strcat_r(prefix, "xgateway", tmp)), sizeof(xgateway)-1);
		strncpy(xdns, nvram_safe_get(strcat_r(prefix, "xdns", tmp)), sizeof(xdns)-1);
		xlease = nvram_get_int(strcat_r(prefix, "xlease", tmp));
		if (xlease > 0)
			xexpires = nvram_get_int(strcat_r(prefix, "xexpires", tmp)) - uptime();
	}
#endif

	websWrite(wp, "function secondary_wanlink_xtype() { return '%s';}\n", xtype);
	websWrite(wp, "function secondary_wanlink_xipaddr() { return '%s';}\n", xip);
	websWrite(wp, "function secondary_wanlink_xnetmask() { return '%s';}\n", xnetmask);
	websWrite(wp, "function secondary_wanlink_xgateway() { return '%s';}\n", xgateway);
	websWrite(wp, "function secondary_wanlink_xdns() { return '%s';}\n", xdns);
	websWrite(wp, "function secondary_wanlink_xlease() { return %d;}\n", xlease);
	websWrite(wp, "function secondary_wanlink_xexpires() { return %d;}\n", xexpires);
#else
	websWrite(wp, "function secondary_wanlink_status() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_statusstr() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_type() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_ipaddr() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_netmask() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_gateway() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_dns() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_lease() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_expires() { return -1;}\n");
	websWrite(wp, "function is_private_subnet() { return -1;}\n");

	websWrite(wp, "function secondary_wanlink_xtype() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_xipaddr() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_xnetmask() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_xgateway() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_xdns() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_xlease() { return -1;}\n");
	websWrite(wp, "function secondary_wanlink_xexpires() { return -1;}\n");
#endif
	return;
}

static void get_wan_unit_hook (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	int unit;
	char wp[32] = {0};

	unit = wan_primary_ifunit();

	websWrite(wp, "%d", unit);

	return;
}
static void get_wan_primary_pvcunit_hook (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char wp[32] = {0};
#ifdef RTCONFIG_DUALWAN
	websWrite(wp, "%d", wan_primary_pvcunit());
#else
	websWrite(wp, "%d", wan_primary_ifunit());
#endif
	return;
}

static void get_wan_secondary_pvcunit_hook (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char wp[32] = {0};
#ifdef RTCONFIG_DUALWAN
	websWrite(wp, "%d", wan_secondary_pvcunit());
#else
	websWrite(wp, "%d", wan_secondary_ifunit());
#endif
	return;
}

static void setting_lan (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char lan_ipaddr_t[16];
	char lan_netmask_t[16];
	unsigned int lan_ip_num;
	unsigned int lan_mask_num;
	unsigned int lan_subnet;
	char wan_ipaddr_t[16];
	char wan_netmask_t[16];
	unsigned int wan_ip_num;
	unsigned int wan_mask_num;
	unsigned int wan_subnet;
	const unsigned int MAX_SUBNET = 3232300800U;
	const unsigned int MIN_LAN_IP = 3232235521U;
	struct in_addr addr;
	unsigned int new_lan_ip_num;
	unsigned int new_dhcp_start_num;
	unsigned int new_dhcp_end_num;
	char new_lan_ip_str[16];
	char new_dhcp_start_str[16];
	char new_dhcp_end_str[16];
	// char tmp_lan[100], prefix_lan[] = "lanXXXXXXXXXX_";
	int unit;
	// char tmp_wan[100], prefix_wan[] = "wanXXXXXXXXXX_";
	char tmp_wanip[32], tmp_wanmask[32];
	char wp[4] = {0};

	// snprintf(prefix_lan, sizeof(prefix_lan), "lan_");

	memset(lan_ipaddr_t, 0, sizeof(lan_ipaddr_t));
	// strcpy(lan_ipaddr_t, nvram_safe_get(strcat_r(prefix_lan, "ipaddr", tmp_lan)));
	tcapi_get("Lan_Entry0", "IP", lan_ipaddr_t);
	memset(&addr, 0, sizeof(addr));
	lan_ip_num = ntohl(inet_aton(lan_ipaddr_t, &addr));
	lan_ip_num = ntohl(addr.s_addr);
	memset(lan_netmask_t, 0, sizeof(lan_netmask_t));
	// strcpy(lan_netmask_t, nvram_safe_get(strcat_r(prefix_lan, "netmask", tmp_lan)));
	tcapi_get("Lan_Entry0", "netmask", lan_netmask_t);
	memset(&addr, 0, sizeof(addr));
	lan_mask_num = ntohl(inet_aton(lan_netmask_t, &addr));
	lan_mask_num = ntohl(addr.s_addr);
	lan_subnet = lan_ip_num&lan_mask_num;
dbg("http: get lan_subnet=%x!\n", lan_subnet);

#if 0 /* don't care about secondary WAN in error redirect page, and must consider DSL and USB WAN. */
	for(unit = WAN_UNIT_FIRST; unit < WAN_UNIT_MAX; ++unit){
		if(unit != wan_primary_ifunit()
				&& !nvram_match("wans_mode", "lb")
				)
			continue;

		int wan_type = get_dualwan_by_pvcunit(unit);
		if(wan_type != WANS_DUALWAN_IF_WAN
				&& wan_type != WANS_DUALWAN_IF_LAN)
			continue;

		break;
	}
	if(unit == WAN_UNIT_MAX){
dbg("http: Can't get the WAN's unit!\n");
		websWrite(wp, "0");
		return;
	}
#else
	unit = wan_primary_ifunit();
#endif
	// wan_prefix(unit, prefix_wan);

	memset(wan_ipaddr_t, 0, sizeof(wan_ipaddr_t));
	// strcpy(wan_ipaddr_t, nvram_safe_get(strcat_r(prefix_lan, "ipaddr", tmp_wan)));
	sprintf(tmp_wanip, "wan%d_ipaddr", unit);
	tcapi_get("Wanduck_Common", tmp_wanip, wan_ipaddr_t);
	memset(&addr, 0, sizeof(addr));
	wan_ip_num = ntohl(inet_aton(wan_ipaddr_t, &addr));
	wan_ip_num = ntohl(addr.s_addr);
	memset(wan_netmask_t, 0, sizeof(wan_netmask_t));
	// strcpy(wan_netmask_t, nvram_safe_get(strcat_r(prefix_lan, "netmask", tmp_wan)));
	sprintf(tmp_wanmask, "wan%d_netmask", unit);
	tcapi_get("Wanduck_Common", tmp_wanmask, wan_netmask_t);
	memset(&addr, 0, sizeof(addr));
	wan_mask_num = ntohl(inet_aton(wan_netmask_t, &addr));
	wan_mask_num = ntohl(addr.s_addr);
	wan_subnet = wan_ip_num&wan_mask_num;
dbg("http: get wan_subnet=%x!\n", wan_subnet);

	if(lan_subnet != wan_subnet){
dbg("http: The subnets of WAN and LAN aren't the same already.!\n");
		websWrite(wp, "0");
		return;
	}

	if(lan_subnet >= MAX_SUBNET)
		new_lan_ip_num = MIN_LAN_IP;
	else
		new_lan_ip_num = lan_ip_num+(~lan_mask_num)+1;

	/* To prevent Lan IP: 192.168.1.254, then new dhcp start to 192.168.1.255. (~lan_mask_num)-1 like 254 */
	if((new_lan_ip_num&(~lan_mask_num)) == ((~lan_mask_num)-1)) {
		new_dhcp_start_num = (new_lan_ip_num&lan_mask_num) + 2; /* prevent x.x.x.1 for router */
		new_dhcp_end_num = (new_lan_ip_num&lan_mask_num) + (~lan_mask_num)-2;
		dbg("http: get new_lan_ip_num&(~lan_mask_num)=%x!\n", (new_lan_ip_num&(~lan_mask_num)));
	}
	else {
		new_dhcp_start_num = new_lan_ip_num+1;
		new_dhcp_end_num = (new_lan_ip_num|(~inet_network(lan_netmask_t)))-1;
	}
dbg("%u, %u, %u.\n", new_lan_ip_num, new_dhcp_start_num, new_dhcp_end_num);
	memset(&addr, 0, sizeof(addr));
	addr.s_addr = htonl(new_lan_ip_num);
	memset(new_lan_ip_str, 0, sizeof(new_lan_ip_str));
	strcpy(new_lan_ip_str, inet_ntoa(addr));
	memset(&addr, 0, sizeof(addr));
	addr.s_addr = htonl(new_dhcp_start_num);
	memset(new_dhcp_start_str, 0, sizeof(new_dhcp_start_str));
	strcpy(new_dhcp_start_str, inet_ntoa(addr));
	memset(&addr, 0, sizeof(addr));
	addr.s_addr = htonl(new_dhcp_end_num);
	memset(new_dhcp_end_str, 0, sizeof(new_dhcp_end_str));
	strcpy(new_dhcp_end_str, inet_ntoa(addr));
dbg("%s, %s, %s.\n", new_lan_ip_str, new_dhcp_start_str, new_dhcp_end_str);

	// nvram_set(strcat_r(prefix_lan, "ipaddr", tmp_lan), new_lan_ip_str);
	tcapi_set("Lan_Entry0", "IP", new_lan_ip_str);
	// nvram_set(strcat_r(prefix_lan, "ipaddr_rt", tmp_lan), new_lan_ip_str); // Sync to lan_ipaddr_rt, added by jerry5.
	// nvram_set("dhcp_start", new_dhcp_start_str);
	tcapi_set("Dhcpd_Common", "start", new_dhcp_start_str);
	// nvram_set("dhcp_end", new_dhcp_end_str);
	tcapi_set("Dhcpd_Common", "end", new_dhcp_end_str);

	websWrite(wp, "1");

	// nvram_commit();
	tcapi_save();	// commit, move to restart net_and_phy

	// notify_rc("restart_net_and_phy");
	//TODO:  many services may need to restart
	eval("/sbin/rc_call", "net_and_phy", "2");

	return;
}

static void login_state_hook (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	// unsigned int ip, login_ip, login_port;
	unsigned int login_ip = 0;
	char ip_str[16];
	time_t login_timestamp;
	// struct in_addr now_ip_addr, login_ip_addr;
	struct in_addr login_ip_addr;
	time_t now;
	const int MAX = 80;
	const int VALUELEN = 18;
	char buffer[MAX], values[6][VALUELEN];
	char wp[256] = {0};
	asus_token_t *token = NULL;

	if(!reent || !(reent->server_env))
		return;

	// ip = getpeerip(wp);
	//csprintf("ip = %u\n",ip);

	// now_ip_addr.s_addr = ip;
	memset(ip_str, 0, sizeof(ip_str));
	// strcpy(ip_str, inet_ntoa(now_ip_addr));
	strcpy(ip_str, reent->server_env->remote_ip_addr);
//	time(&now);
	now = uptime();

	// login_ip = (unsigned int)atoll(nvram_safe_get("login_ip"));
	// login_ip_addr.s_addr = login_ip;
	token = SEARCH_TOKEN_LST_BY_CLITYPE(CLI_TYPE_BROWSER);
	if(!token)
		return;
	
	if(inet_aton(token->ipaddr, &login_ip_addr))
		login_ip = login_ip_addr.s_addr;

	login_timestamp = token->login_timestamp;

	FILE *fp = fopen("/proc/net/arp", "r");
	if (fp){
		memset(buffer, 0, MAX);
		memset(values, 0, 6*VALUELEN);

		while (fgets(buffer, MAX, fp)){
			if (strstr(buffer, "br0") && !strstr(buffer, "00:00:00:00:00:00")){
				if (sscanf(buffer, "%s%s%s%s%s%s", values[0], values[1], values[2], values[3], values[4], values[5]) == 6){
					if (!strcmp(values[0], ip_str)){
						break;
					}
				}

				memset(values, 0, 6*VALUELEN);
			}

			memset(buffer, 0, MAX);
		}

		fclose(fp);
	}

	// if (ip != 0 && login_ip == ip && login_port != 0 && login_port == http_port) {
	if (!strcmp(ip_str, token->ipaddr) ) {
		websWrite(wp, "function is_logined() { return 1; }\n");
		websWrite(wp, "function login_ip_dec() { return '%u'; }\n", login_ip);
		websWrite(wp, "function login_ip_str() { return '%s'; }\n", token->ipaddr);
		websWrite(wp, "function login_ip_str_now() { return '%s'; }\n", ip_str);

		if (values[3] != NULL)
			websWrite(wp, "function login_mac_str() { return '%s'; }\n", values[3]);
		else
			websWrite(wp, "function login_mac_str() { return ''; }\n");
		token->login_timestamp = uptime();
	}
	else{
		websWrite(wp, "function is_logined() { return 0; }\n");
		websWrite(wp, "function login_ip_dec() { return '%u'; }\n", login_ip);

		if ((unsigned long)(now-login_timestamp) > 60)	//one minitues
			websWrite(wp, "function login_ip_str() { return '0.0.0.0'; }\n");
		else
			websWrite(wp, "function login_ip_str() { return '%s'; }\n", token->ipaddr);

		websWrite(wp, "function login_ip_str_now() { return '%s'; }\n", ip_str);

		if (values[3] != NULL)
			websWrite(wp, "function login_mac_str() { return '%s'; }\n", values[3]);
		else
			websWrite(wp, "function login_mac_str() { return ''; }\n");
	}

	return;// 0;
}

static void ajax_wanstate (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	int unit;
	char tmp[32], prefix[16];
	int wan_state = -1, wan_sbstate = -1, wan_auxstate = -1;
	char wp[256] = {0};


	memset(tmp, 0, sizeof(tmp));
	memset(prefix, 0, sizeof(prefix));

	/* current unit */
#ifdef RTCONFIG_DUALWAN
	tcapi_get("Dualwan_Entry", "wans_dualwan", tmp);
	if(!strstr(tmp, "none"))	//dualwan enable
		unit = wan_primary_pvcunit();
	else
#endif
	unit = wan_primary_ifunit();
	wan_prefix(unit, prefix);

	wan_state = tcapi_get_int(WANDUCK, strcat_r(prefix, "state_t", tmp));
	wan_sbstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "sbstate_t", tmp));
	wan_auxstate = tcapi_get_int(WANDUCK, strcat_r(prefix, "auxstate_t", tmp));

	websWrite(wp, "<wan>%d</wan>\n", wan_state);
	websWrite(wp, "<wan>%d</wan>\n", wan_sbstate);
	websWrite(wp, "<wan>%d</wan>\n", wan_auxstate);

	return;
}

static void secondary_ajax_wanstate (asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char wp[256] = {0};
#ifdef RTCONFIG_DUALWAN
	int unit;
	char tmp[100], prefix[] = "wanXXXXXXXXXX_";
	int wan_state = -1, wan_sbstate = -1, wan_auxstate = -1;

	/* current unit */
	unit = wan_secondary_pvcunit();
	wan_prefix(unit, prefix);

	wan_state = nvram_get_int(strcat_r(prefix, "state_t", tmp));
	wan_sbstate = nvram_get_int(strcat_r(prefix, "sbstate_t", tmp));
	wan_auxstate = nvram_get_int(strcat_r(prefix, "auxstate_t", tmp));

	websWrite(wp, "<secondary_wan>%d</secondary_wan>\n", wan_state);
	websWrite(wp, "<secondary_wan>%d</secondary_wan>\n", wan_sbstate);
	websWrite(wp, "<secondary_wan>%d</secondary_wan>\n", wan_auxstate);
#else
	websWrite(wp, "<secondary_wan>-1</secondary_wan>\n");
	websWrite(wp, "<secondary_wan>-1</secondary_wan>\n");
	websWrite(wp, "<secondary_wan>-1</secondary_wan>\n");
#endif

	return;
}

static void stop_service (asp_reent* reent, const asp_text* params, asp_text* ret) {
	killall_tk("clean_cache.sh");
	sleep(1);

	kill_pidfile_s("/var/run/cfg_manager_pid", SIGUSR2);
	sleep(1);
#ifdef RTCONFIG_USB
	system("ejusb 1 0");	//force ejusb whether usb apps enabled or not
	system("ejusb 2 0");
#endif
	system("rc_call service 0");

	return;
}

static void sysinfo(asp_reent* reent, const asp_text* params, asp_text* ret) {
	char *type;
	char result[8];
	char wp[256] = {0};

	strcpy(result, "-1");

	//if (ejArgs(argc, argv, "%s", &type) < 1) {
		//websError(wp, 400, "Insufficient args\n");
		//return;
	//}
	if (params[0].str && params[0].len) {
		type = (char*)asp_alloc(reent,params[0].len+1);
		memset(type, 0, params[0].len+1);
		memcpy(type, params[0].str, params[0].len);
	}
	else
		return;
	//dbg("[sysinfo] type=%s\n", type);

	if(strncmp(type,"pid",3) == 0 ){
		char service[32];
		sscanf(type, "pid.%31s", service);

		if (strlen(service))
			sprintf(result, "%d", pidof(service));

		//dbg("[sysinfo] service=%s, result=%d\n", service, result);
	}

	websWrite(wp, result);
	return;
}

static void dump_fb_fail_content( void )
{
	char buf[MAX_LINE_SIZE] = {0};
	char tmp_val[80] = {0};
	long long_tmp = 0;
	char *str = NULL;
	unsigned int up_seconds = 0;
	char up_time[128] = {0};
	int activePVC = 0;
	char wanPVC[16] = {0};
	int transferMode = 0; //0:ATM, 1:PTM, 2:Ethernet WAN
	char tmp_wan_conn_type[32] = {0};
	char tmp1[8] = {0}, tmp2[8] = {0};

	str = file2str("/proc/uptime");
	if(str)
	{
		up_seconds = atoi(str);
		free(str);
	}
	reltime(up_seconds, up_time);

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("SysInfo_Entry", "ProductName", tmp_val);
	sprintf(buf, "Model: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("DeviceInfo", "FwVer", tmp_val);
	sprintf(buf, "Firmware Version: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("SysInfo_Entry", "InnerVersion", tmp_val);
	sprintf(buf, "Inner Version: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Info_Adsl", "fwVer", tmp_val);
	sprintf(buf, "DSL Driver Version: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "MODULATIONTYPE", tmp_val);
	sprintf(buf, "Configured DSL modulation: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "ANNEXTYPEA", tmp_val);
	sprintf(buf, "Configured Annex mode: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_testlab", tmp_val);
	sprintf(buf, "Special setting for: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_snrm_offset", tmp_val);
	sprintf(buf, "Stability Adjustment(ADSL): %s", tmp_val);
	if(strcmp(tmp_val, "0") == 0)
	{
		strcat(buf, "(Disabled)\n");
	}
	else
	{
		long_tmp = atoi(tmp_val);
		sprintf(tmp_val, "(%ld dB)\n", long_tmp/512);
		strcat(buf, tmp_val);
	}
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "adsl_rx_agc", tmp_val);
	sprintf(buf, "Rx AGC(ADSL): %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_RT63365) || defined(TCSUPPORT_CPU_RT63368)
	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_adsl_esnp", tmp_val);
	sprintf(buf, "ESNP - Enhanced Sudden Noise Protection(ADSL): %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));
#endif /* TCSUPPORT_CPU_MT7510 */

#ifdef TCSUPPORT_WAN_PTM
	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "vdsl_snrm_offset", tmp_val);
	sprintf(buf, "Stability Adjustment(VDSL): %s", tmp_val);
	if(strcmp(tmp_val, "32767") == 0)
	{
		strcat(buf, "(Disabled)\n");
	}
	else
	{
		long_tmp = atoi(tmp_val);
		sprintf(tmp_val, "(%ld dB)\n", long_tmp/512);
		strcat(buf, tmp_val);
	}
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "vdsl_tx_gain_off", tmp_val);
	sprintf(buf, "Tx Power Control(VDSL): %s", tmp_val);
	if(strcmp(tmp_val, "32767") == 0)
	{
		strcat(buf, "(Disabled)\n");
	}
	else
	{
		long_tmp = atoi(tmp_val);
		sprintf(tmp_val, "(%ld dB)\n", long_tmp/10);
		strcat(buf, tmp_val);
	}
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "vdsl_rx_agc", tmp_val);
	sprintf(buf, "Rx AGC(VDSL): %s", tmp_val);
	if(strcmp(tmp_val, "65535") == 0)
	{
		strcat(buf, "(Default)\n");
	}
	else if(strcmp(tmp_val, "394") == 0)
	{
		strcat(buf, "(Stable)\n");
	}
	else if(strcmp(tmp_val, "476") == 0)
	{
		strcat(buf, "(Balance)\n");
	}
	else if(strcmp(tmp_val, "550") == 0)
	{
		strcat(buf, "(High Performance)\n");
	}
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "vdsl_upbo", tmp_val);
	sprintf(buf, "UPBO - Upstream Power Back Off(VDSL): %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_vdsl_esnp", tmp_val);
	sprintf(buf, "ESNP - Enhanced Sudden Noise Protection(VDSL): %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));
#endif /* TCSUPPORT_WAN_PTM */

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_sra", tmp_val);
	sprintf(buf, "SRA (Seamless Rate Adaptation): %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_bitswap", tmp_val);
	sprintf(buf, "Bitswap (ADSL): %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

#ifdef TCSUPPORT_WAN_PTM
	#ifdef TCSUPPORT_CPU_MT7510
	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_vdsl_bitswap", tmp_val);
	sprintf(buf, "Bitswap (VDSL): %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_vdsl_vectoring", tmp_val);
	sprintf(buf, "G.vector: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_vdsl_nonstd_vectoring", tmp_val);
	sprintf(buf, "Non-Standard G.vector: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));
	#endif /* TCSUPPORT_CPU_MT7510 */

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "vdsl_profile", tmp_val);
	sprintf(buf, "VDSL Profile: %s", tmp_val);
	if(strcmp(tmp_val, "0") == 0)
	{
		strcat(buf, "0(30a multi mode)\n");
	}
	else if(strcmp(tmp_val, "1") == 0)
	{
		strcat(buf, "1(17a multi mode)\n");
	}
	else
	{
		strcat(buf, tmp_val);
		strcat(buf, "(unknown mode)\n");
	}
	asp_send_response(NULL, buf, strlen(buf));
#endif /* TCSUPPORT_WAN_PTM */

#ifdef TCSUPPORT_CPU_RT65168
	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Adsl_Entry", "dslx_ginp", tmp_val);
	sprintf(buf, "G.INP: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));
#endif /* TCSUPPORT_CPU_RT65168 */

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("WebCurSet_Entry", "loss_sync", tmp_val);
	if(strcmp(tmp_val, "1") == 0)
	{
		//please refer to [advanced.c] and ask [Javier Su] why output "2".
		sprintf(buf, "Monitor line stability: %s\n", "2");
	}
	else
	{
		sprintf(buf, "Monitor line stability: %s\n", tmp_val);
	}
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("WebCurSet_Entry", "dslup_counter", tmp_val);
	sprintf(buf, " / %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("DslDiag_Entry", "dslx_diag_enable", tmp_val);
	snprintf(buf, sizeof(buf), "DSL Line Diagnostic: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("DslDiag_Entry", "dslx_diag_duration", tmp_val);
	if(strcmp(tmp_val, "0") == 0)
	{
		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("GUITemp_Entry0", "fb_tmp_availability", tmp_val);
		if(strncmp(tmp_val, "Occasional_interruptions", 4) == 0)
		{
			snprintf(tmp_val, sizeof(tmp_val), "86400");
		}
		else if(strncmp(tmp_val, "Frequent_interruptions", 4) == 0)
		{
			snprintf(tmp_val, sizeof(tmp_val), "43200");
		}
		else
		{
			snprintf(tmp_val, sizeof(tmp_val), "3600");
		}
	}
	snprintf(buf, sizeof(buf), "Diagnostic Duration: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	sprintf(buf, "System Up time: %s\n", up_time);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Info_Adsl", "ADSLUpTime", tmp_val);
	sprintf(buf, "DSL Up time: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Info_Adsl", "lineState", tmp_val);
	sprintf(buf, "DSL Link Status: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	if(strcmp(tmp_val, "up") == 0)
	{
		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "Opmode", tmp_val);
		sprintf(buf, "Current DSL modulation: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "AdslType", tmp_val);
		sprintf(buf, "Current Annex mode: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "CurrentProfiles", tmp_val);
		sprintf(buf, "Current Profile: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "SNRMarginDown", tmp_val);
		sprintf(buf, "SNR Down: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "SNRMarginUp", tmp_val);
		sprintf(buf, "SNR Up: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "AttenDown", tmp_val);
		sprintf(buf, "Line Attenuation Down: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "AttenUp", tmp_val);
		sprintf(buf, "Line Attenuation Up: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "TCM", tmp_val);
		sprintf(buf, "TCM: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "PathModeDown", tmp_val);
		sprintf(buf, "Path Mode Down: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "InterleaveDepthDown", tmp_val);
		sprintf(buf, "Interleave Depth Down: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "PathModeUp", tmp_val);
		sprintf(buf, "Path Mode Up: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "InterleaveDepthUp", tmp_val);
		sprintf(buf, "Interleave Depth Up: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "DataRateDown", tmp_val);
		sprintf(buf, "Data Rate Down: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "DataRateUp", tmp_val);
		sprintf(buf, "Data Rate Up: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "AttainDown", tmp_val);
		sprintf(buf, "MAX Rate Down: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "AttainUp", tmp_val);
		sprintf(buf, "MAX Rate Up: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "CRCDown", tmp_val);
		sprintf(buf, "CRC Down: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "CRCUp", tmp_val);
		sprintf(buf, "CRC Up: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "PowerDown", tmp_val);
		sprintf(buf, "Power Down: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "PowerUp", tmp_val);
		sprintf(buf, "Power Up: %s\n\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string("Info_Adsl", "FarEndVendorID", tmp_val);
		sprintf(buf, "Far end vendor: %s\n\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));
	}

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("GUITemp_Entry0", "fb_tmp_country", tmp_val);
	sprintf(buf, "Country: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("Timezone_Entry", "TZ", tmp_val);
	sprintf(buf, "Time Zone: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("GUITemp_Entry0", "fb_tmp_ISP", tmp_val);
	sprintf(buf, "ISP: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("GUITemp_Entry0", "fb_tmp_Subscribed_Info", tmp_val);
	sprintf(buf, "Subscribed Package: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("GUITemp_Entry0", "fb_tmp_email", tmp_val);
	sprintf(buf, "E-mail: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	activePVC = wan_primary_ifunit();
	sprintf(wanPVC, "Wan_PVC%d", activePVC);
	switch(activePVC)
	{
		/* ATM */
		case 0:
			transferMode = 0;
			break;
		/* PTM */
		case 8:
			/* FALL-THROUGH */
		case 9:
			transferMode = 1;
			break;
		/* Ethernet WAN */
		case 10:
			transferMode = 2;
			break;
		default:
			transferMode = 0;
			break;
	}

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string(wanPVC, "ISP", tmp_val);
	if(atoi(tmp_val) == 0){
		sprintf(tmp_wan_conn_type, "Automatic IP");
	}
	else if(atoi(tmp_val) == 1){
		sprintf(tmp_wan_conn_type, "Static IP");
	}
	else if(atoi(tmp_val) == 2){
	if(transferMode == 0){
		sprintf(tmp_wan_conn_type, "PPPoA/PPPoE");
	}
	else{
		sprintf(tmp_wan_conn_type, "PPPoE");
	}
	}
	else if(atoi(tmp_val) == 3){
		sprintf(tmp_wan_conn_type, "BRIDGE");
	}

	if(transferMode == 1){
		sprintf(buf, "WAN Connection Type: %s\n", tmp_wan_conn_type);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string(wanPVC, "VLANID", tmp_val);
		sprintf(buf, "VLAN ID: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));
	}
	else if(transferMode == 0){
		tcapi_get_string(wanPVC, "VPI", tmp1);
		tcapi_get_string(wanPVC, "VCI", tmp2);
		sprintf(buf, "VPI/VCI: %s/%s\n", tmp1, tmp2);
		asp_send_response(NULL, buf, strlen(buf));

		sprintf(buf, "WAN Connection Type: %s\n", tmp_wan_conn_type);
		asp_send_response(NULL, buf, strlen(buf));

		memset(tmp_val, 0, sizeof(tmp_val));
		tcapi_get_string(wanPVC, "ENCAP", tmp_val);
		sprintf(buf, "Encapsulation Mode: %s\n", tmp_val);
		asp_send_response(NULL, buf, strlen(buf));
	}

	memset(tmp_val, 0, sizeof(tmp_val));
	tcapi_get_string("GUITemp_Entry0", "fb_tmp_availability", tmp_val);
	sprintf(buf, "DSL connection: %s\n", tmp_val);
	asp_send_response(NULL, buf, strlen(buf));

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "\nComments:\n");
	asp_send_response(NULL, buf, strlen(buf));
	memset(buf, 0, sizeof(buf));
	tcapi_get_string("GUITemp_Entry0", "fb_tmp_comment0", buf);
	if(strlen(buf) > 0)
	{
		asp_send_response(NULL, buf, strlen(buf));
		memset(buf, 0, sizeof(buf));
		tcapi_get_string("GUITemp_Entry0", "fb_tmp_comment1", buf);
		asp_send_response(NULL, buf, strlen(buf));
		memset(buf, 0, sizeof(buf));
		tcapi_get_string("GUITemp_Entry0", "fb_tmp_comment2", buf);
		asp_send_response(NULL, buf, strlen(buf));
		memset(buf, 0, sizeof(buf));
		tcapi_get_string("GUITemp_Entry0", "fb_tmp_comment3", buf);
		asp_send_response(NULL, buf, strlen(buf));
		memset(buf, 0, sizeof(buf));
		tcapi_get_string("GUITemp_Entry0", "fb_tmp_comment4", buf);
		asp_send_response(NULL, buf, strlen(buf));
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "\n\n");
		asp_send_response(NULL, buf, strlen(buf));
	}

}

static void ej_dump(asp_reent* reent, const asp_text* params, asp_text* ret) {
	char filename[PATH_MAX];
	char *file;
	char wp[MAX_LINE_SIZE] = {0};

	if (params[0].str && params[0].len) {
		file = (char*)asp_alloc(reent,params[0].len+1);
		memset(file, 0, params[0].len+1);
		memcpy(file, params[0].str, params[0].len);
	}
	else
		return;

	if (strcmp(file, "syslog.log")==0)
	{
		sprintf(filename, "/var/log/%s", file);
		dump_file(wp, filename);
	}
	else if(!strcmp(file, "cloudsync.log")){
		sprintf(filename, "/tmp/smartsync/.logs/system.log");
		dump_file(wp, filename);
		sprintf(filename, "/tmp/%s", file);
		dump_file(wp, filename);
	}
	else if(!strcmp(file, "clouddisk.log")){
		sprintf(filename, "/tmp/lighttpd/syslog.log");
		dump_file(wp, filename);
		sprintf(filename, "/tmp/%s", file);
		dump_file(wp, filename);
	}
	else if(!strcmp(file, "fb_fail_content")){
		sprintf(filename, "/tmp/xdslissuestracking");
		if(f_exists(filename))
		{
			char cmd[128] = {0};
			sprintf(cmd, "sed -i '/PIN Code:/d' %s", filename);
			system(cmd);
			sprintf(cmd, "sed -i '/MAC Address:/d' %s", filename);
			system(cmd);
			sprintf(cmd, "sed -i '/E-mail:/d' %s", filename);
			system(cmd);
			dump_file(wp, filename);
			unlink(filename);
		}
		else
		{
			dump_fb_fail_content();
		}
	}
	else if(!strcmp(file,"ISP_List")) {
		sprintf(filename, "/etc/ISP_List.txt");
		dump_file(wp, filename);
	}
	else if(!strcmp(file,"ISP_List_IPTV")) {
		sprintf(filename, "/etc/ISP_List_IPTV.txt");
		dump_file(wp, filename);
	}
	else if(!strcmp(file,"ISP_PTM_List")) {
		sprintf(filename, "/etc/ISP_PTM_List.txt");
		dump_file(wp, filename);
	}
	else if(!strcmp(file,"ISP_PTM_List_IPTV")) {
		sprintf(filename, "/etc/ISP_PTM_List_IPTV.txt");
		dump_file(wp, filename);
	}
	else if (strcmp(file, "iptable.log")==0)
	{
		get_nat_vserver_table(wp);
	}
#ifdef RTCONFIG_OPENVPN
	else if(!strncmp(file, "vpn_crt_", 8)) {
		sprintf(filename, "/%s/%s", OVPN_FS_PATH, file);
		if(f_exists(filename))
			clean_dump_file(wp, filename);
	}
	else if(!strcmp(file, "openvpn_connected")){
		int unit = tcapi_get_int("WebCurSet_Entry","openvpn_id") - SERVER_IF_START;
		parse_openvpn_status(unit);
		sprintf(filename, "/etc/openvpn/server%d/client_status", unit);
		dump_file(wp, filename);
	}
#endif
#ifdef TCSUPPORT_WEBMON
	else if(!strcmp(file, "web_history.log")){
		get_web_history_table(wp);
		sprintf(filename, "/tmp/web_history.txt");
		dump_file(wp, filename);
	}
#endif
	else {
		sprintf(filename, "/tmp/%s", file);
		dump_file(wp, filename);
	}

	return;
}

void websRedirect_iframe(char *wp, char *url)
{
        char value[8] = {0};

        websWrite(wp, "<html><head>\r\n");

        if(strchr(url, '>') || strchr(url, '<'))
        {
                websWrite(wp,"<script>parent.location.href='/index.asp';</script>\n");
        }
        else
        {
#ifdef RTCONFIG_HTTPS
                if(do_ssl){
                        websWrite(wp, "<meta http-equiv=\"refresh\" content=\"0; url=https://%s/%s\">\r\n", gethost, url);
                }else
#endif
                {
                        tcapi_get("Lan_Entry", "IP", value);
                        websWrite(wp, "<meta http-equiv=\"refresh\" content=\"0; url=http://%s/%s\">\r\n", value, url);
                }
        }

        websWrite(wp, "<meta http-equiv=\"Content-Type\" content=\"text/html\">\r\n");
        websWrite(wp, "</head></html>\r\n");
}

void sys_script(char *name) {
	char scmd[64];

	sprintf(scmd, "/tmp/%s", name);
	//dbg("run %s %d %s\n", name, strlen(name), scmd);	// tmp test

     //handle special scirpt first
     if (strcmp(name,"syscmd.sh")==0)
     {
	   if (strcmp(SystemCmd, "")!=0)
	   {
		sprintf(SystemCmd, "%s > /tmp/syscmd.log 2>&1 && echo 'XU6J03M6' >> /tmp/syscmd.log &\n", SystemCmd);	// oleg patch
		system(SystemCmd);
		strcpy(SystemCmd, ""); // decrease loading time.
	   }
	   else
	   {
		system("echo  > /tmp/syscmd.log\n");
	   }
     }
}

void websApply(char *wp, char *url)
{
#ifdef TRANSLATE_ON_FLY
	do_ej(url, wp);
	websDone(wp, 200);
#else   // define TRANSLATE_ON_FLY

	FILE *fp;
	char buf[MAX_LINE_SIZE];

	fp = fopen(url, "r");

	if (fp==NULL) return;

	while (fgets(buf, sizeof(buf), fp))
	{
		websWrite(wp, buf);
	}

	fclose(fp);
#endif
}

int gen_modemlog(void)
{
	char value[8] = {0};
	char cmd[128] = {0};

	tcapi_get("PushMail_Entry", "PM_attach_modemlog", value);

	if( strcmp(value, "1") == 0 )
	{
		sprintf(cmd, "/usr/script/3ginfo.sh > %s", "/tmp/xdsl/modemlog.txt");
		system(cmd);
	}
	return 0;
}

int dump_file(char *wp, char *filename) {
	FILE *fp;
	char buf[MAX_LINE_SIZE];

	fp = fopen(filename, "r");
	if (fp==NULL)
		return -1;

	while (fgets(buf, MAX_LINE_SIZE, fp)!=NULL)
		asp_send_response(NULL, buf, strlen(buf));

	fclose(fp);

	return 0;
}

int clean_dump_file(char *wp, char *filename) {
	FILE *fp;
	char buf[MAX_LINE_SIZE];

	fp = fopen(filename, "r");
	if (fp==NULL)
		return -1;

	while (fgets(buf, MAX_LINE_SIZE, fp)!=NULL)
		_webs_clean_write(buf);

	fclose(fp);

	return 0;
}

static void ej_nvram_get_ddns(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *sid, *name, *c;
	char buf[MAXLEN_TCAPI_MSG] = {0};
	char wp[128] = {0};

	//if (ejArgs(argc, argv, "%s %s", &sid, &name) < 2) {
		//websError(wp, 400, "Insufficient args\n");
		//return -1;
	//}
	if (params[0].str && params[0].len) {
		sid = (char*)asp_alloc(reent,params[0].len+1);
		memset(sid, 0, params[0].len+1);
		memcpy(sid, params[0].str, params[0].len);
	}
	else
		return;
	if (params[1].str && params[1].len) {
		name = (char*)asp_alloc(reent,params[1].len+1);
		memset(name, 0, params[1].len+1);
		memcpy(name, params[1].str, params[1].len);
	}
	else
		return;

	tcapi_get(sid, name, buf);
	for (c = buf; *c; c++) {
		if (isprint(*c) &&
		    *c != '"' && *c != '&' && *c != '<' && *c != '>')
			websWrite(wp, "%c", *c);
		else
			websWrite(wp, "&#%d", *c);
	}
	if (strcmp(name,"ddns_return_code")==0) {
		if(!tcapi_match(sid, "ddns_return_code", "ddns_query")) {
			tcapi_set(sid, "ddns_return_code","");
		}
	}

	return;
}

static void modify_admin_account(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *new_password = get_param(g_var_post, "uiViewTools_Password");
	if(new_password)
		mod_account("admin", NULL, new_password);	//already restart samba in mod_account
}

static void modify_aidisk_account(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char acclist[256] = {0};
	char newpwd[128] = {0};
	char newuser[128] = {0};
	char *p = NULL;
	char charAccount[64] = {0};

	tcapi_get("Samba_Entry", "acc_list", acclist);
	tcapi_get("Account_Entry0", "username", newuser);
	tcapi_get("Account_Entry0", "web_passwd", newpwd);

	if(strlen(acclist) > 1)
	{
		p = strstr(acclist, ">");
		if(p)
		{
			*p = 0;
			ascii_to_char_safe(charAccount, acclist, 64);
			mod_account(charAccount, newuser, newpwd);
		}
	}
}

static void NetworkToolsNetstat(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *cmdMethod = get_param(g_var_post, "cmdMethod");
	char *NetOption = get_param(g_var_post, "NetOption");
	//char *targetip = get_param(g_var_post, "targetip");
	//char *ExtOption = get_param(g_var_post, "ExtOption");
	char *ResolveName = get_param(g_var_post, "ResolveName");
	char cmd[MAX_LINE_SIZE] = {0};
	char *logstr = "> /tmp/nettool.log 2>&1 && echo 'XU6J03M6' >> /tmp/nettool.log &";

	if(!cmdMethod
		|| !NetOption
		|| !ResolveName
	)
		return;

	if(!strcmp(cmdMethod, "netstat")) {
		if((MAX_LINE_SIZE - strlen(cmd) - 1) > strlen(cmdMethod) + 1) {
			strcat(cmd, cmdMethod);
			strcat(cmd, " ");
		}
	}
	else
		return;

	if (strchr(NetOption, '&') || strchr(NetOption, ';') || strchr(NetOption, '%')
		|| strchr(NetOption, '|') || strchr(NetOption, '\n') || strchr(NetOption, '\r'))
		return;

	if (strchr(ResolveName, '&') || strchr(ResolveName, ';') || strchr(ResolveName, '%')
		|| strchr(ResolveName, '|') || strchr(ResolveName, '\n') || strchr(ResolveName, '\r'))
		return;

	if((MAX_LINE_SIZE - strlen(cmd) - 1) > strlen(NetOption) + 1) {
		strcat(cmd, NetOption);
		strcat(cmd, " ");
	}

	if(strcmp(ResolveName, "1")) {
		if(MAX_LINE_SIZE - strlen(cmd) - 1 > 2)
			strcat(cmd, "-n");
	}

	if(MAX_LINE_SIZE - strlen(cmd) - 1 > strlen(logstr)) {
		strcat(cmd, logstr);
		system(cmd);
	}
	else
		f_write_string("/tmp/nettool.log", " ", 0, 0);

}

static void NetworkToolsAnalysis(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *cmdMethod = get_param(g_var_post, "cmdMethod");
	char *destIP = get_param(g_var_post, "destIP");
	char *pingCNT = get_param(g_var_post, "pingCNT");
	char cmd[MAX_LINE_SIZE] = {0};
	char *logstr = "> /tmp/nettool.log 2>&1 && echo 'XU6J03M6' >> /tmp/nettool.log &";

	if(!cmdMethod
		|| !destIP
		|| !pingCNT
	)
		return;

	if(strcmp(cmdMethod, "ping")
		&& strcmp(cmdMethod, "traceroute")
		&& strcmp(cmdMethod, "nslookup")
	)
		return;

	if (strchr(destIP, '&') || strchr(destIP, ';') || strchr(destIP, '%')
		|| strchr(destIP, '|') || strchr(destIP, '\n') || strchr(destIP, '\r'))
		return;

	if (strchr(pingCNT, '&') || strchr(pingCNT, ';') || strchr(pingCNT, '%')
		|| strchr(pingCNT, '|') || strchr(pingCNT, '\n') || strchr(pingCNT, '\r'))
		return;

	if((MAX_LINE_SIZE - strlen(cmd) - 1) > strlen(cmdMethod) + 1) {
		strcat(cmd, cmdMethod);
		strcat(cmd, " ");
	}

	if(strlen(pingCNT) > 0 && !strcmp(cmdMethod, "ping")) {
		if((MAX_LINE_SIZE - strlen(cmd) - 1) > strlen(pingCNT) + 4) {
			strcat(cmd, "-c ");
			strcat(cmd, pingCNT);
			strcat(cmd, " ");
		}

		//Andy Chiu, 2015/06/01.
		unlink("/tmp/nettool.log");
		sprintf(cmd, "%s -o %s ",  cmd, "/tmp/nettool.log");
	}

	if((MAX_LINE_SIZE - strlen(cmd) - 1) > strlen(destIP)) {
		strcat(cmd, destIP);
	}

	if(MAX_LINE_SIZE - strlen(cmd) - 1 > strlen(logstr)) {
		strcat(cmd, logstr);
		system(cmd);
	}
	else
		f_write_string("/tmp/nettool.log", " ", 0, 0);

}

static void get_DSL_WAN_list(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char wp[256] = {0};
	char node[32] = {0};
	char value[32] = {0};
	int i;
	int pvc = tcapi_get_int("WebCurSet_Entry", "wan_pvc");

	websWrite(wp, "\n");
	if(tcapi_match("Wan_Common", "TransMode", "ATM")) {
		for(i=0; i<8; i++) {
			memset(node, 0, sizeof(node));
			sprintf(node, "Wan_PVC%d", i);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "Active", value);
			websWrite(wp, "[\"%s\", ", value);

			websWrite(wp, "\"PVC %d\", ", i + 1);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "VPI", value);
			websWrite(wp, "\"%s\", ", value);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "VCI", value);
			websWrite(wp, "\"%s\", ", value);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "ISP", value);
			websWrite(wp, "\"%s\", ", value);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "ENCAP", value);
			websWrite(wp, "\"%s\", ", value);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "QOS", value);
			websWrite(wp, "\"%s\"]", value);

			if(i==7)
				websWrite(wp, "\n");
			else
				websWrite(wp, ",\n");
		}
	}
	else {
#if (defined(TCSUPPORT_WAN_ETHER) || defined(TCSUPPORT_WAN_PTM)) && defined(TCSUPPORT_MULTISERVICE_ON_WAN)
		for(i=0; i<8; i++) {
			memset(node, 0, sizeof(node));
			sprintf(node, "WanExt_PVC%de%d", pvc, i);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "Active", value);
			websWrite(wp, "[\"%s\", ", value);

			websWrite(wp, "\"%d\", ", i);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "dot1q", value);
			websWrite(wp, "\"%s\", ", value);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "VLANID", value);
			websWrite(wp, "\"%s\", ", value);

			memset(value, 0, sizeof(value));
			tcapi_get(node, "ISP", value);
			websWrite(wp, "\"%s\"]", value);

			if(i==7)
				websWrite(wp, "\n");
			else
				websWrite(wp, ",\n");
		}
#else
		memset(node, 0, sizeof(node));
		sprintf(node, "Wan_PVC%d", pvc);

		memset(value, 0, sizeof(value));
		tcapi_get(node, "Active", value);
		websWrite(wp, "[\"%s\", ", value);

		websWrite(wp, "\"%s\", ", tcapi_get_string("Wan_Common", "TransMode", value));

		memset(value, 0, sizeof(value));
		tcapi_get(node, "dot1q", value);
		websWrite(wp, "\"%s\", ", value);

		memset(value, 0, sizeof(value));
		tcapi_get(node, "VLANID", value);
		websWrite(wp, "\"%s\", ", value);

		memset(value, 0, sizeof(value));
		tcapi_get(node, "ISP", value);
		websWrite(wp, "\"%s\"]\n", value);
#endif
	}
}

/*******************************************************************
* NAME: WOL_Invoke
* AUTHOR: Andy Chiu
* CREATE DATE: 2014/10/08
* DESCRIPTION: invoke wake-on-lan function
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void WOL_Invoke(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	//get the MAC address of destination
	char *destMac = get_param(g_var_post, "destMac");
	char cmd[MAX_LINE_SIZE] = {0};

	//check the pointer of the mac address of destination
	if(!destMac)
		return;

	//generate the command line
	sprintf(cmd, "ether-wake -i %s %s", IF_LAN, destMac);

	//execute the command line
	system(cmd);
}

/*******************************************************************
* NAME: ClientList_Refresh
* AUTHOR: Andy Chiu
* CREATE DATE: 2014/10/23
* DESCRIPTION: refresh the client list
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void ClientList_Refresh(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char tmp[128];
	unlink(cl_cache_path);
	tcapi_set("ClientList_Common", "cache_size", "0");

	//check networkmap
	if(access("/var/run/networkmap.pid", F_OK))	//	networkmap is not running
	{
		//launch networkmap
		system("/userfs/bin/networkmap");
	}
	else
	{
		//send signal to networkmap to do refresh
		sprintf(tmp, "killall -%d networkmap", SIGUSR1);
		system(tmp);
	}
}

/*******************************************************************
* NAME: ClientList_Update
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/03/26
* DESCRIPTION: update the client list
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void ClientList_Update(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char tmp[128];
	//check networkmap
	if(access("/var/run/networkmap.pid", F_OK))	//	networkmap is not running
	{
		//launch networkmap
		system("/userfs/bin/networkmap");
	}
	else
	{
		//send signal to networkmap to do refresh
		sprintf(tmp, "killall -%d networkmap", SIGUSR1);
		system(tmp);
	}
}

/*******************************************************************
* NAME: set_primary_pvc
* AUTHOR: Renjie Lee
* CREATE DATE: 2014/11/05
* DESCRIPTION: set dev_pvc to current primary wan unit
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void set_primary_pvc(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char unit[8] = {0};

	sprintf(unit, "%d", wan_primary_ifunit());

	tcapi_set("WebCurSet_Entry", "dev_pvc", unit);
}


/*******************************************************************
* NAME: wl_sta_list_2g
* AUTHOR: Andy Chiu
* CREATE DATE: 2014/12/03
* DESCRIPTION: get the 2g wifi station list
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void wl_sta_list_2g(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE* fp;
	char buf[1024]={0};
	char tmp[512]={0};

	system("/usr/bin/ATE wl_sta_list 2g");

	fp = fopen("/var/tmp/wl_lst_2g","r");
	if (fp != NULL){
		while(fgets(tmp, 512, fp)){
			if (tmp != NULL){
				websWrite(buf, "%s", tmp);
				//strcat(buf, tmp);
			}
		}
		fclose(fp);
	}
}


/*******************************************************************
* NAME: wl_sta_list_2g
* AUTHOR:
Andy Chiu
* CREATE DATE: 2014/12/03
* DESCRIPTION: get the 5g wifi station list
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void wl_sta_list_5g(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE* fp;
	char buf[1024]={0};
	char tmp[512]={0};

	system("/usr/bin/ATE wl_sta_list 5g");

	fp = fopen("/var/tmp/wl_lst_5g","r");
	if (fp != NULL){
		while(fgets(tmp, 512, fp)){
			if (tmp != NULL){
				websWrite(buf, "%s", tmp);
				//strcat(buf, tmp);
			}
		}
		fclose(fp);
	}
}

/*******************************************************************
* NAME: get_client_list
* AUTHOR: Andy Chiu
* CREATE DATE: 2014/12/03
* DESCRIPTION: get client list
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void get_client_list(asp_reent* reent, const asp_text* params, asp_text* ret)
{
#if 0
	FILE* fp;
	char buf[1024]={0};
	char tmp[512]={0};

	fp = fopen("/var/tmp/cl.log","r");
	if (fp != NULL){
		while(fgets(tmp, 512, fp)){
			if (tmp != NULL){
				websWrite(buf, "%s", tmp);
			}
		}
		fclose(fp);
	}
#else
	int i, shm_client_info_id;
	void *shared_client_info=(void *) 0;
	char wp[1024] = {0}, output_buf[256];
	P_CLIENT_DETAIL_INFO_TABLE p_client_info_tab;
	int lock;
	char devname[LINE_SIZE], character;
	int j, len;
	FILE *fp = NULL;

	lock = file_lock("networkmap");
	shm_client_info_id = shmget((key_t)1001, sizeof(CLIENT_DETAIL_INFO_TABLE), 0666|IPC_CREAT);
	if (shm_client_info_id == -1){
	    fprintf(stderr,"shmget failed\n");
	    file_unlock(lock);
	    return;
	}

	shared_client_info = shmat(shm_client_info_id,(void *) 0,0);
	if (shared_client_info == (void *)-1){
		fprintf(stderr,"shmat failed\n");
		file_unlock(lock);
		return;
	}

	tcapi_get("ClientList_Common", "scan", output_buf);
	if(!(atoi(output_buf)))
		fp = fopen(cl_cache_path, "w");

	p_client_info_tab = (P_CLIENT_DETAIL_INFO_TABLE)shared_client_info;
	for(i=0; i<p_client_info_tab->ip_mac_num; i++) {
		memset(output_buf, 0, 256);
		memset(devname, 0, LINE_SIZE);

	    if(p_client_info_tab->exist[i]==1) {
		len = strlen( (char *) p_client_info_tab->device_name[i]);
		for (j=0; (j < len) && (j < LINE_SIZE-1); j++) {
			character = p_client_info_tab->device_name[i][j];
			if ((isalnum(character)) || (character == ' ') || (character == '-') || (character == '_'))
				devname[j] = character;
			else
				devname[j] = ' ';
		}

		sprintf(output_buf, "<%d.%d.%d.%d>%02X:%02X:%02X:%02X:%02X:%02X>%s>%d>%d>%d>%d",
			p_client_info_tab->ip_addr[i][0],p_client_info_tab->ip_addr[i][1],
			p_client_info_tab->ip_addr[i][2],p_client_info_tab->ip_addr[i][3],
			p_client_info_tab->mac_addr[i][0],p_client_info_tab->mac_addr[i][1],
			p_client_info_tab->mac_addr[i][2],p_client_info_tab->mac_addr[i][3],
			p_client_info_tab->mac_addr[i][4],p_client_info_tab->mac_addr[i][5],
			devname,
			p_client_info_tab->type[i],
			p_client_info_tab->http[i],
			p_client_info_tab->printer[i],
			p_client_info_tab->itune[i]
		);
		websWrite(wp, "%s", output_buf);
		if(fp)
		{
			fwrite(output_buf, 1, strlen(output_buf), fp);
		}
//		if(i < p_client_info_tab->ip_mac_num-1)
//			websWrite(wp, ",");
	    }
	}
	shmdt(shared_client_info);
	file_unlock(lock);
	if(fp)
	{
		fclose(fp);
		//Sync cache size
		tcapi_get("ClientList_Common", "size", output_buf);
		tcapi_set("ClientList_Common", "cache_size", output_buf);
	}
#endif
	return;
}

/*******************************************************************
* NAME: get_client_list_cache
* AUTHOR: Andy Chiu
* CREATE DATE: 2014/12/03
* DESCRIPTION: get client list cache
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void get_client_list_cache(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE* fp;
	char buf[1024]={0};
	char tmp[512]={0};

	fp = fopen(cl_cache_path,"r");
	if (fp != NULL){
		while(fgets(tmp, 512, fp)){
			if (tmp != NULL){
				websWrite(buf, "%s", tmp);
			}
		}
		fclose(fp);
	}
}

/*******************************************************************
* NAME: get_asus_dev_list
* AUTHOR: Andy Chiu
* CREATE DATE: 2014/12/03
* DESCRIPTION: get asus devices list
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void get_asus_dev_list(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE* fp;
	char buf[1024]={0};
	char tmp[512]={0};

	fp = fopen("/var/tmp/asusdev.lst","r");
	if (fp != NULL){
		while(fgets(tmp, 512, fp)){
			if (tmp != NULL){
				websWrite(buf, "%s", tmp);
			}
		}
		fclose(fp);
	}
}

/*******************************************************************
* NAME: get_cl_userdef_list
* AUTHOR: Andy Chiu
* CREATE DATE: 2014/12/03
* DESCRIPTION: get user defined item of client list
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void get_cl_userdef_list(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	int count, i, attr_num;
	char buf[1024], attr[32], tmp[512];

	//get size
	memset(buf, 0, sizeof(buf));
	tcapi_get("UserDefCL_Common", "size", buf);
	count = atoi(buf);

	if(count > 0)
	{
		//count the valid number of attributes
		attr_num = ((count - 1) / 8) + 1;

		for(i = 0; i < attr_num; ++i)
		{
			sprintf(attr, "dev%d", i);
			memset(tmp, 0, sizeof(tmp));
			if(!tcapi_get("UserDefCL_Entry", attr, tmp))
				websWrite(buf, "%s", tmp);

		}
	}

}

/*******************************************************************
* NAME: get_static_dhcp_list
* AUTHOR: Andy Chiu
* CREATE DATE: 2014/12/30
* DESCRIPTION: get static ip of dhcp list
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void get_static_dhcp_list(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	int i;
	char buf[1024], node[32], ip[32], mac[32];
	int max;

	//get the max size of static dhcp list
	memset(buf, 0, sizeof(buf));
	tcapi_get("Dhcpd", "MaxStaticNum", buf);
	max = atoi(buf);

	if(max > 0)
	{
		for(i = 0; i < max; ++i)
		{
			sprintf(node, "Dhcpd_Entry%d", i);
			memset(ip, 0, sizeof(ip));
			memset(mac, 0, sizeof(mac));
			if(!tcapi_get(node, "IP", ip) && !tcapi_get(node, "MAC", mac))
			{
				if(i)
					websWrite(buf, "<");
				websWrite(buf, "%s>%s", ip, mac);
			}
		}
	}
}

/*******************************************************************
* NAME: dhcpLeaseMacList
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/08/11
* DESCRIPTION: get dhcp lease mac list
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void dhcpLeaseMacList(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char buf[256]={0},mac[17], ip[16],expire[10], hostname[32];
	char wp[1024];
	char *str;

	
	FILE *fp = fopen("/etc/udhcp_lease", "r");
	if(fp)
	{
		websWrite(wp, "[");
		while (fgets(buf, 256, fp)){
			memset(mac, 0, sizeof(mac));
			memset(ip, 0, sizeof(ip));
			memset(expire, 0, sizeof(expire));
			memset(hostname, 0, sizeof(hostname));
			sscanf(buf, "%s %s %s %s",mac, ip, expire, hostname);

			// Get the whole hostname including space
			if(strlen(hostname)) {
				str = strstr(buf, hostname);
				if(str && (*(str+strlen(str)-1) == 0x0a || *(str+strlen(str)-1) == 0x0d)){
					*(str+strlen(str)-1)=0x0;
					snprintf(hostname, sizeof(hostname), "%s", str);
				}
				char_to_ascii_safe(buf, hostname, strlen(hostname));
				websWrite(wp,"['%s', '%s'],", mac, buf);
			}
			else
			{
				//error record in buf ex. [00:00:00:00:00:00 192.168.1.108 3935]
			}
		}
		fclose(fp);
		websWrite(wp, "['','']]");
	}
	else
	{
		websWrite(wp, "[['', '']]");
	}
}

#ifdef ASUS_LOGIN_SESSION
/*******************************************************************
* NAME: IP_dhcpLeaseInfo
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/12/03
* DESCRIPTION: get dhcp lease info ['ip', 'host name'], ['', '']
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void IP_dhcpLeaseInfo(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char buf[256]={0},mac[17], ip[16],expire[10], hostname[32];
	char wp[1024];
	char *str;

	
	FILE *fp = fopen("/etc/udhcp_lease", "r");
	if(fp)
	{
		websWrite(wp, "[");
		while (fgets(buf, 256, fp)){
			memset(mac, 0, sizeof(mac));
			memset(ip, 0, sizeof(ip));
			memset(expire, 0, sizeof(expire));
			memset(hostname, 0, sizeof(hostname));
			sscanf(buf, "%s %s %s %s",mac, ip, expire, hostname);

			// Get the whole hostname including space
			if(strlen(hostname)) {
				str = strstr(buf, hostname);
				if(str && (*(str+strlen(str)-1) == 0x0a || *(str+strlen(str)-1) == 0x0d)){
					*(str+strlen(str)-1)=0x0;
					snprintf(hostname, sizeof(hostname), "%s", str);
				}
				char_to_ascii_safe(buf, hostname, strlen(hostname));
				websWrite(wp,"['%s', '%s'],", ip, buf);
			}
			else
			{
				//error record in buf ex. [00:00:00:00:00:00 192.168.1.108 3935]
			}
		}
		fclose(fp);
		websWrite(wp, "['','']]");
	}
	else
	{
		websWrite(wp, "[['', '']]");
	}
}
#endif

/*******************************************************************
* NAME: tcWebApi_MatchThenWrite
* AUTHOR: Sam Wen
* CREATE DATE: 2015/06/03
* DESCRIPTION:
* 	same as:
* 		<% if tcWebApi_Get(.....) = "...." then asp_write("xxx") end if %>
* INPUT:
* 	reent:	for memory handle
* 	params:	params[0] node name.
* 			params[1] attribute name.
* 			params[2] string to compare
* 			params[3] string to response to web page if value of attribute match params[2]
* 	ret:	return params[3] if value of attribute match params[2], else return NULL and len=0
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void tcWebApi_MatchThenWrite (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char *node,*attr,*comp,*resp;

	node = (char*)asp_alloc(reent,params[0].len+1);
	attr = (char*)asp_alloc(reent,params[1].len+1);
	comp = (char*)asp_alloc(reent,params[2].len+1);
	resp = (char*)asp_alloc(reent,params[3].len+1);
	memset(node,0,params[0].len+1);
	memset(attr,0,params[1].len+1);
	memset(comp,0,params[2].len+1);
	memset(resp,0,params[3].len+1);
	memcpy(node,params[0].str,params[0].len);
	memcpy(attr,params[1].str,params[1].len);
	memcpy(comp,params[2].str,params[2].len);
	memcpy(resp,params[3].str,params[3].len);
	node[params[0].len]='\0';
	attr[params[1].len]='\0';
	comp[params[2].len]='\0';
	resp[params[3].len]='\0';

	if(tcapi_match(node, attr, comp)) {
		asp_send_response (NULL, resp, params[3].len);
		ret->str = params[3].str;
		ret->len = params[3].len;
	}
	else {
		ret->str = NULL;
		ret->len = 0;
	}
}

static void tcWebApi_clean_get (asp_reent* reent, const asp_text* params,  asp_text* ret)
{
	char *node,*attr, *c;
	char buf[MAXLEN_TCAPI_MSG] = {0};
	char wp[128] = {0};

	node = (char*)asp_alloc(reent,params[0].len+1);
	attr = (char*)asp_alloc(reent,params[1].len+1);
	memset(node,0,params[0].len+1);
	memset(attr,0,params[1].len+1);
	memcpy(node,params[0].str,params[0].len);
	memcpy(attr,params[1].str,params[1].len);
	node[params[0].len]='\0';
	attr[params[1].len]='\0';

	if(tcapi_get(node, attr, buf))	//error
		return;

	for (c = buf; *c; c++) {
		if (isprint(*c) &&
		    *c != '"' && *c != '&' && *c != '<' && *c != '>')
			websWrite(wp, "%c", *c);
		else
			websWrite(wp, "&#%d", *c);
	}
}

#ifdef RTCONFIG_OPENVPN
static void vpn_server_get_parameter(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	tcapi_set("WebCurSet_Entry", "openvpn_id", "21");
}

static void ej_vpn_crt_server(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char buf[4000];
	char name[32];
	int idx = 0;
	char wp[256] = {0};

	for (idx = 1; idx <= MAX_OVPN_SERVER; idx++) {
		//ca
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_server%d_ca", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		//crt
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_server%d_crt", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		//key
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_server%d_key", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		//dh
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_server%d_dh", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		//static
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_server%d_static", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		//crl
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_server%d_crl", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		websWrite(wp, "\n");
	}
}

static void ej_vpn_crt_client(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char buf[4000];
	char name[32];
	int idx = 0;
	char wp[256] = {0};

	for (idx = 1; idx <= MAX_OVPN_CLIENT; idx++) {
		//ca
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_client%d_ca", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		//crt
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_client%d_crt", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		//key
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_client%d_key", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		//static
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_client%d_static", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		//crl
		memset(buf, 0, sizeof(buf));
		memset(name, 0, sizeof(name));
		sprintf(name, "vpn_crt_client%d_crl", idx);
		get_parsed_crt(name, buf, sizeof(buf));
		websWrite(wp, "%s=['", name);
		_webs_clean_write(buf);
		websWrite(wp, "'];\n");

		websWrite(wp, "\n");
	}
}

static void do_vpnupload_cgi(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *filetype = get_param(g_var_post, "vpn_upload_type");
	char *vpn_upload_unit = get_param(g_var_post, "vpn_upload_unit");
	char *bfilename = get_param(g_var_post, "bfilename");
	char *bdata_start = get_param(g_var_post, "bdata_start");
	char *bdata_end = get_param(g_var_post, "bdata_end");
	long unit, data_start, data_end, data_len;
	FILE* fpSrc = NULL;
	FILE* fpDst = NULL;
	char filepath[128];
	char buf[4096];
	char node[MAXLEN_NODE_NAME];
	char attr[MAXLEN_ATTR_NAME];
	int state;

	if(!filetype || !vpn_upload_unit || !bfilename || !bdata_start || !bdata_end)
		return;

	unit = strtol(vpn_upload_unit, NULL, 10);
	data_start = strtol(bdata_start, NULL, 10);
	data_end = strtol(bdata_end, NULL, 10);
	if(data_end > data_start)
		data_len = data_end - data_start + 1;
	else
		return;

	_dprintf("upload file: %s\nsize: %ld, type: %s, unit: %ld\n", bfilename, data_len, filetype, unit);

	fpSrc = fopen("/tmp/boa-temp", "rb");
	if (fpSrc==NULL) goto err;
	snprintf(filepath, sizeof(filepath), "/tmp/%s", bfilename);
	fpDst = fopen(filepath, "wb");
	if (fpDst==NULL) goto err;
	if(fseek(fpSrc, data_start, SEEK_SET)) goto err;
	while (data_len > 0)
	{
		if (data_len > sizeof(buf))
		{
			fread(buf, 1, sizeof(buf), fpSrc);
			fwrite(buf, 1, sizeof(buf), fpDst);
			data_len -= sizeof(buf);
		}
		else
		{
			fread(buf, 1, data_len, fpSrc);
			fwrite(buf, 1, data_len, fpDst);
			data_len = 0;
		}
	}
	fclose(fpSrc);
	fclose(fpDst);

#if defined(TCSUPPORT_ADD_JFFS) || defined(TCSUPPORT_SQUASHFS_ADD_YAFFS)
	if(!check_if_dir_exist(OVPN_FS_PATH))
		mkdir(OVPN_FS_PATH, S_IRWXU);
#endif

	if(1) {
		sprintf(node, "OpenVPN_Entry%ld", unit+CLIENT_IF_START);
		if(!strcmp(filetype, "ovpn")) {
			reset_client_setting(unit);
			state = read_config_file(filepath, unit);
			tcapi_set_int("OpenVPN_Common", "vpn_upload_state", state);
			tcapi_save();
		}
		else if(!strcmp(filetype, "ca")) {
			sprintf(attr, "vpn_crt_client%ld_ca", unit);
			set_crt_parsed(attr, filepath);
			state = tcapi_get_int("OpenVPN_Common", "vpn_upload_state");
			tcapi_set_int("OpenVPN_Common", "vpn_upload_state", state & (~VPN_UPLOAD_NEED_CA_CERT));
		}
		else if(!strcmp(filetype, "cert")) {
			sprintf(attr, "vpn_crt_client%ld_crt", unit);
			set_crt_parsed(attr, filepath);
			state = tcapi_get_int("OpenVPN_Common", "vpn_upload_state");
			tcapi_set_int("OpenVPN_Common", "vpn_upload_state", state & (~VPN_UPLOAD_NEED_CERT));
		}
		else if(!strcmp(filetype, "key")) {
			sprintf(attr, "vpn_crt_client%ld_key", unit);
			set_crt_parsed(attr, filepath);
			state = tcapi_get_int("OpenVPN_Common", "vpn_upload_state");
			tcapi_set_int("OpenVPN_Common", "vpn_upload_state", state & (~VPN_UPLOAD_NEED_KEY));
		}
		else if(!strcmp(filetype, "static")) {
			sprintf(attr, "vpn_crt_client%ld_static", unit);
			set_crt_parsed(attr, filepath);
			state = tcapi_get_int("OpenVPN_Common", "vpn_upload_state");
			tcapi_set_int("OpenVPN_Common", "vpn_upload_state", state & (~VPN_UPLOAD_NEED_STATIC));
		}
		else if(!strcmp(filetype, "ccrl")) {
			sprintf(attr, "vpn_crt_client%ld_crl", unit);
			set_crt_parsed(attr, filepath);
			state = tcapi_get_int("OpenVPN_Common", "vpn_upload_state");
			tcapi_set_int("OpenVPN_Common", "vpn_upload_state", state & (~VPN_UPLOAD_NEED_CRL));
		}
		else if(!strcmp(filetype, "scrl")) {
			sprintf(attr, "vpn_crt_server%ld_crl", unit);
			set_crt_parsed(attr, filepath);
		}
	}

	unlink(filepath);

	return;

err:
	if (fpSrc)
		fclose(fpSrc);
	if (fpDst)
		fclose(fpDst);

}

static void _save_openvpn_client_cert()
{
	char *vpn_client_unit = get_param(g_var_post, "vpn_client_unit");
	char *vpn_crt_client_ca = get_param(g_var_post, "vpn_crt_client_ca");
	char *vpn_crt_client_crt = get_param(g_var_post, "vpn_crt_client_crt");
	char *vpn_crt_client_key = get_param(g_var_post, "vpn_crt_client_key");
	char *vpn_crt_client_static = get_param(g_var_post, "vpn_crt_client_static");
	char *vpn_crt_client_crl = get_param(g_var_post, "vpn_crt_client_crl");
	int unit;
	char name[32] = {0};
	char file_path[64] = {0};
	FILE *fp;

	if(!vpn_client_unit
		|| !vpn_crt_client_ca
		|| !vpn_crt_client_crt
		|| !vpn_crt_client_key
		|| !vpn_crt_client_static
		|| !vpn_crt_client_crl)
		return;

	unit = atoi(vpn_client_unit);
	decode_uri(vpn_crt_client_ca);
	decode_uri(vpn_crt_client_crt);
	decode_uri(vpn_crt_client_key);
	decode_uri(vpn_crt_client_static);
	decode_uri(vpn_crt_client_crl);

	if(unit >= 1 && unit <= MAX_OVPN_CLIENT) {
		//ca
		snprintf(name, sizeof(name), "vpn_crt_client%d_ca", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_client_ca, strlen(vpn_crt_client_ca), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
		//crt
		snprintf(name, sizeof(name), "vpn_crt_client%d_crt", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_client_crt, strlen(vpn_crt_client_crt), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
		//key
		snprintf(name, sizeof(name), "vpn_crt_client%d_key", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_client_key, strlen(vpn_crt_client_key), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
		//static
		snprintf(name, sizeof(name), "vpn_crt_client%d_static", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_client_static, strlen(vpn_crt_client_static), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
		//crl
		snprintf(name, sizeof(name), "vpn_crt_client%d_crl", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_client_crl, strlen(vpn_crt_client_crl), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
	}
}

static void _save_openvpn_server_tls()
{
	//NOTE: only support one server. UI fixed vpn_crt_server1_xxx currently
	char *vpn_server_unit = "1";//get_param(g_var_post, "vpn_server_unit");
	char *vpn_crt_server1_ca = get_param(g_var_post, "vpn_crt_server1_ca");
	char *vpn_crt_server1_crt = get_param(g_var_post, "vpn_crt_server1_crt");
	char *vpn_crt_server1_key = get_param(g_var_post, "vpn_crt_server1_key");
	char *vpn_crt_server1_dh = get_param(g_var_post, "vpn_crt_server1_dh");
	char *vpn_crt_server1_static = get_param(g_var_post, "vpn_crt_server1_static");
	char *vpn_crt_server1_crl = get_param(g_var_post, "vpn_crt_server1_crl");
	int unit;
	char name[32] = {0};
	char file_path[64] = {0};
	FILE *fp;

	if(!vpn_server_unit
		|| !vpn_crt_server1_ca
		|| !vpn_crt_server1_crt
		|| !vpn_crt_server1_key
		|| !vpn_crt_server1_dh
		|| !vpn_crt_server1_static
		|| !vpn_crt_server1_crl)
		return;

	unit = atoi(vpn_server_unit);
	decode_uri(vpn_crt_server1_ca);
	decode_uri(vpn_crt_server1_crt);
	decode_uri(vpn_crt_server1_key);
	decode_uri(vpn_crt_server1_dh);
	decode_uri(vpn_crt_server1_static);
	decode_uri(vpn_crt_server1_crl);

	if(unit >= 1 && unit <= MAX_OVPN_SERVER) {
		//ca
		snprintf(name, sizeof(name), "vpn_crt_server%d_ca", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_server1_ca, strlen(vpn_crt_server1_ca), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
		//crt
		snprintf(name, sizeof(name), "vpn_crt_server%d_crt", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_server1_crt, strlen(vpn_crt_server1_crt), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
		//key
		snprintf(name, sizeof(name), "vpn_crt_server%d_key", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_server1_key, strlen(vpn_crt_server1_key), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
		//dh
		snprintf(name, sizeof(name), "vpn_crt_server%d_dh", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_server1_dh, strlen(vpn_crt_server1_dh), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
		//static
		snprintf(name, sizeof(name), "vpn_crt_server%d_static", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_server1_static, strlen(vpn_crt_server1_static), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
		//crl
		snprintf(name, sizeof(name), "vpn_crt_server%d_crl", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_server1_crl, strlen(vpn_crt_server1_crl), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
	}
}

static void _save_openvpn_server_static()
{
	//NOTE: only support one server. UI fixed vpn_crt_server1_xxx currently
	char *vpn_server_unit = "1";//get_param(g_var_post, "vpn_server_unit");
	char *vpn_crt_server1_static = get_param(g_var_post, "vpn_crt_server1_static");
	int unit;
	char name[32] = {0};
	char file_path[64] = {0};
	FILE *fp;

	if(!vpn_server_unit
		|| !vpn_crt_server1_static
	)
		return;

	unit = atoi(vpn_server_unit);
	decode_uri(vpn_crt_server1_static);
	if(unit >= 1 && unit <= MAX_OVPN_SERVER) {
		//static
		snprintf(name, sizeof(name), "vpn_crt_server%d_static", unit);
		snprintf(file_path, sizeof(file_path), "/tmp/%s", name);
		fp = fopen(file_path, "w");
		if(fp) {
			fwrite(vpn_crt_server1_static, strlen(vpn_crt_server1_static), sizeof(char), fp);
			fclose(fp);
			chmod(file_path, S_IRUSR|S_IWUSR);
			set_crt_parsed(name, file_path);
		}
		unlink(file_path);
	}
}
#endif

#ifdef ASUS_DISK_UTILITY
/*******************************************************************
* NAME: disk_scan
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/07/22
* DESCRIPTION: do disk scan manually
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void disk_scan(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE *fp;
	char buf[64], cmd[128];

	if(access("/var/run/disk_monitor.pid", F_OK))
	{
		//diskmon is not exist, launch it
		system("/sbin/disk_monitor &");
		sleep(1);
	}
	
	//check diskmon pid
	fp = fopen("/var/run/disk_monitor.pid", "r");
	if(fp)
	{
		memset(buf, 0, sizeof(buf));
		fread(buf, 1, 64, fp);
		fclose(fp);
	}
	else
		return;

	if(buf[0] != '\0')
	{
		sprintf(cmd, "kill -SIGUSR2 %s", buf);
		system(cmd);
	}
		
}

/*******************************************************************
* NAME: disk_scan_result
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/07/23
* DESCRIPTION: get the result of disk scaning manually
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void disk_scan_result(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	FILE *fp;
	char buf[64], buf2[1024], wp[1024];

	//get usb port id
	memset(buf, 0, sizeof(buf));
	tcapi_get("USB_Entry", "scan_port", buf);

	//get the disk name
	sprintf(buf2, "usb_path%s_fs_path0", buf);
	tcapi_get("USB_Entry", buf2, buf);

	//open the result file
	sprintf(buf2, "/tmp/fsck_ret/%s", buf);
	fp = fopen(buf2, "r");
	if(fp)
	{
		while(fgets(buf2, 1024, fp))
		{
			websWrite(wp, buf2);
		}
		fclose(fp);
	}
}
#endif
#ifdef TCSUPPORT_DSL_LINE_DIAGNOSTIC
static void stop_dsl_diag(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	if(tcapi_match("DslDiag_Entry", "dslx_diag_enable", "1") && tcapi_match("DslDiag_Entry", "dslx_diag_state", "1")) {
		tcapi_set("DslDiag_Entry", "dslx_diag_enable", "0");
		tcapi_set("DslDiag_Entry", "dslx_diag_state", "5");
		tcapi_commit("DslDiag");
	}
}
#endif

#ifdef ASUS_BUSYBOX_NEW
static void get_isp_dhcp_opt_list(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *name;
	FILE* fpIsp;
	char bufIsp[512], wp[1024];

	fpIsp = fopen("/tmp/etc/ISP_DHCP_Opt_List.txt","r");
	if (fpIsp != NULL)
	{
	  // read out UTF-8 3 bytes header
	        fread(bufIsp,3,1,fpIsp);
		while(!feof(fpIsp))
		{
			char* ret_fgets;
			ret_fgets = fgets(bufIsp,sizeof(bufIsp),fpIsp);
			if (ret_fgets != NULL)
			{
				websWrite(wp, bufIsp);
				websWrite(wp, "\n");
			}
		}
		fclose(fpIsp);
	}
	return 0;	
}
#endif

#ifdef ASUS_LOGIN_SESSION
/*******************************************************************
* NAME: http_login_header
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/12/11
* DESCRIPTION: send login http header
* INPUT: token: string
* OUTPUT:
* RETURN:
* NOTE: Andy Chiu, 2016/01/04. Add AiHOMEAPILevel, Httpd_AiHome_Ver and Model_Name.
*	      Andy Chiu, 2016/01/07. Modify send reply function. use req to detect which way to send packe
*******************************************************************/
int http_login_header(const char *token, request *req)
{
	char buf[2048] = {0}, tmp[256] = {0}, product_name[64] = {0};
	char *p = buf;
	char HTTP_OK[] = "HTTP/1.0 200 OK\r\n";
	strcat(p, HTTP_OK);
	p += strlen(HTTP_OK);

	char DATA[] = "Date: "
		"                             "
		"\r\n";
	rfc822_time_buf(DATA + 6, 0);
	strcat(p, DATA);
	p += strlen(DATA);

	//Add AiHOMEAPILevel
	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, sizeof(tmp), "AiHOMEAPILevel: %d\r\n", EXTEND_AIHOME_API_LEVEL);
	strcat(p, tmp);
	p += strlen(tmp);

	//Add Httpd_AiHome_Ver
	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, sizeof(tmp), "Httpd_AiHome_Ver: %d\r\n", EXTEND_HTTPD_AIHOME_VER);
	strcat(p, tmp);
	p += strlen(tmp);

	//Add Model_Name
	memset(tmp, 0, sizeof(tmp));
	tcapi_get("SysInfo_Entry", "ProductName", product_name);
	snprintf(tmp, sizeof(tmp), "Model_Name: %s\r\n", product_name);
	strcat(p, tmp);
	p += strlen(tmp);	

	char CONTENT_TYPE[64];
	if(charset){
		sprintf(CONTENT_TYPE,"Content-type: text/html;charset=%s\r\n",charset);
	}
	else{
		/*default char set: ISO-8859-1*/
		sprintf(CONTENT_TYPE,"Content-type: text/html;charset=ISO-8859-1\r\n");
	}
/*pork 20090309 added over*/
	strcat(p, CONTENT_TYPE);
	p += strlen(CONTENT_TYPE);

	char set_cookie[256];
	if(token)
	{
		sprintf(set_cookie, "Set-Cookie: asus_token=%s; HttpOnly;\r\n", token);
	}

	strcat(p, set_cookie);
	p += strlen(set_cookie);
	
	strcat(p, "Connection: close\r\n\r\n");
	
	if(req)
	{
		if(req_write(req,  buf))
		return -1;
	}
	else
	{
		if(asp_send_response(NULL, buf, strlen(buf)))
			return -1;
	}

	return 0;
}

/*******************************************************************
* NAME: send_login_packet
* AUTHOR: Andy Chiu
* CREATE DATE: 2016/01/06
* DESCRIPTION: send login packet
* INPUT: req: pointer of request; error_status: error code; cur_page: path of current page; next_page: path of next_page
* OUTPUT:
* RETURN:
* NOTE: Andy Chiu, 2016/01/07. Modify send reply function. websWrite() would make boa crash.
*******************************************************************/
void send_login_packet(request *req, const int error_status, char *cur_page, char *next_page)
{
	int fromapp = 0;
	char wp[2048], *page = NULL;
	char dst_page[256] = {0};
	asus_token_t *token = NULL;
	char *buff[256] = {0};
	CLIENT_TYPE cli_type;

	if(!req)
		return;

	SQUASH_KA(req);	
	req->response_status = R_REQUEST_OK;
	
	cli_type = check_client_type(req->header_user_agent);

	//send login header. If error_status is zero, auth is successful.
	if(!error_status)
	{
		token = add_asus_token(cli_type, generate_token(buff, sizeof(buff)), req->remote_ip_addr);
	}

	//auth success but can't generate token.
	if(!error_status && !token)
		return;
	
	http_login_header(token? token->token: NULL, NULL);

	//Andy Chiu, 2016/01/05. Check APP
	if(cli_type != CLI_TYPE_BROWSER)
			fromapp = 1;		

	//send content
	if(!fromapp)
	{	
		//send content
		websWrite(wp, "<HTML><HEAD>\n");

		page = error_status? cur_page: next_page;
		
		if(page)
		{
			decode_uri(page);
			
			//check the page path.
			if(!strstr(page, "Main_Login.asp") && !strstr(page, "QIS"))
			{		
				if(strncmp(page, "cgi-bin", 7) && strncmp(page, "/cgi-bin", 8))
					strcpy(dst_page, "cgi-bin/");
				else
					strcpy(dst_page, "");
			}
			
			if(page[0] == '/')
				strncat(dst_page, page + 1, sizeof(dst_page) - strlen(dst_page) - 1);
			else
				strncat(dst_page, page, sizeof(dst_page) - strlen(dst_page) - 1);
				

			//Andy Chiu, 2016/01/04. Modify the redirect page function without java script for security issue.
			if(!error_status)
			{
				websWrite(wp, "<meta http-equiv=\"refresh\" content=\"0; url=%s\">\n", dst_page);
			}
			else
			{
				websWrite(wp, "<meta http-equiv=\"refresh\" content=\"0; url=%s?error_status=%d", dst_page, error_status);
				if(next_page)
				{
					websWrite(wp, "&next_page=%s", next_page);
				}
				websWrite(wp, "\">\n");
				
			}
		}
		websWrite(wp, "</HEAD></HTML>\n");
	}
	else
	{
		if(!error_status)
		{
			websWrite(wp, "{\n\"asus_token\":\"%s\"\n}\n", token->token);
		}
		else
		{
			websWrite(wp, "{\n\"error_status\":\"%d\"\n}\n", error_status);
		}
	}
}

/*******************************************************************
* NAME: do_login
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/12/11
* DESCRIPTION: handle login
* INPUT:
* OUTPUT:
* RETURN:
* NOTE: Andy Chiu, 2016/01/04. Modify the redirect web page function.
*	      Andy Chiu, 2016/01/05. Add APP reply
*******************************************************************/
static void do_login(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char *decode_buf = NULL;
	int len;
	s_param **auth;
	int err_code = 0;
	char username[129], password[129];
	char *login_auth = get_param(g_var_post, "login_authorization");	
	time_t dt;
	char log[128] = {0};
	login_retry_t *retry = NULL;
	request *req;
	long cur_time;

	if(!reent || !reent->server_env)
		return;
	
	req = reent->server_env;
	
	//Get Account 
	tcapi_get("Account_Entry0", "username", username);
	tcapi_get("Account_Entry0", "web_passwd", password);
	
	
	//decode login_auth by base64
	decode_uri(login_auth);
	decode_buf = rfc822_base64(login_auth, strlen(login_auth), &len);
	
	//parser the decoded string.
	auth = param_line (decode_buf, ':', ';');
	
	if(auth)
	{
		if(!strcmp(auth[0]->name, username) && !strcmp(auth[0]->value, password))
		{
			snprintf(log, sizeof(log), "User [%s] logged in from [%s] via WEB\n", auth[0]->name, reent->server_env->remote_ip_addr);
			openlog("TCSysLog WEB", 0, LOG_LOCAL2);
			syslog(LOG_INFO, log);
			closelog();

			//remove item in retry list
			rm_retry_in_list(req->remote_ip_addr);			
		}
		else {
			err_code = ACCOUNTFAIL;
			cur_time = uptime();

			retry = search_retry_list(req->remote_ip_addr);
			//dump_retry(retry);
			
			if(!retry)
			{
				retry = add_login_retry(req->remote_ip_addr);
			}
			else
			{
				if(retry->last_login_timestamp > 0 && ((cur_time - retry->last_login_timestamp) > 60))
				{
					retry->try_num = 0;
				}
				retry->last_login_timestamp = cur_time;
			}

			++ (retry->try_num);
			if(retry->try_num >= DEFAULT_LOGIN_MAX_NUM)
			{
				retry->try_num = 0;
				retry->is_lock = 1;
#ifdef RTCONFIG_NOTIFICATION_CENTER
				NOTIFY_EVENT_T *e = initial_nt_event();
				e->event = ADMIN_LOGIN_FAIL_LAN_WEB_EVENT;
				snprintf(e->msg, sizeof(e->msg), "%s", reent->server_env->remote_ip_addr);
				send_trigger_event(e);
				nt_event_free(e);
#endif
				err_code = LOGINLOCK;
			}
			
			snprintf(log, sizeof(log), "User [%s] from [%s] failed to log in via WEB.\n", auth[0]->name, reent->server_env->remote_ip_addr);
			openlog("TCSysLog WEB", 0, LOG_LOCAL2);
			syslog(LOG_INFO, log);
			closelog();
		}
		
		free_param(auth);
	}
	free(decode_buf);
	
	send_login_packet(reent->server_env, err_code, get_param(g_var_post, "current_page"), get_param(g_var_post, "next_page"));
}

static int get_nat_vserver_table(char *wp)
{
	FILE *fp;
	char *nat_argv[] = {"iptables", "-t", "nat", "-nxL", NULL};
	char line[256], tmp[256];
	char target[16], proto[16];
	char src[sizeof("255.255.255.255")];
	char dst[sizeof("255.255.255.255")];
	char *range, *host, *port, *ptr, *val;
	int ret = 0;

	/* dump nat table including VSERVER and VUPNP chains */
	_eval(nat_argv, ">/tmp/vserver.log", 10, NULL);

	websWrite(wp, "Source          Destination     Proto. Port range  Redirect to     Local port\n");
	/*	 255.255.255.255 other  65535:65535 255.255.255.255 65535:65535 */

	fp = fopen("/tmp/vserver.log", "r");
	if (fp == NULL)
		return ret;

	while (fgets(line, sizeof(line), fp) != NULL)
	{
		dbgprintf("%s: %s\n", __FUNCTION__, line);

		tmp[0] = '\0';
		if (sscanf(line,
		    "%15s%*[ \t]"		// target
		    "%15s%*[ \t]"		// prot
		    "%*s%*[ \t]"		// opt
		    "%15[^/]/%*d%*[ \t]"	// source
		    "%15[^/]/%*d%*[ \t]"	// destination
		    "%255[^\n]",		// options
		    target, proto, src, dst, tmp) < 4) continue;

		/* TODO: add port trigger, portmap, etc support */
		if (strcmp(target, "DNAT") != 0)
			continue;

		/* uppercase proto */
		for (ptr = proto; *ptr; ptr++)
			*ptr = toupper(*ptr);
#ifdef NATSRC_SUPPORT
		/* parse source */
		if (strcmp(src, "0.0.0.0") == 0)
			strcpy(src, "ALL");
#endif
		/* parse destination */
		if (strcmp(dst, "0.0.0.0") == 0)
			strcpy(dst, "ALL");

		/* parse options */
		port = host = range = "";
		ptr = tmp;
		while ((val = strsep(&ptr, " ")) != NULL) {
			if (strncmp(val, "dpt:", 4) == 0)
				range = val + 4;
			else if (strncmp(val, "dpts:", 5) == 0)
				range = val + 5;
			else if (strncmp(val, "to:", 3) == 0) {
				port = host = val + 3;
				strsep(&port, ":");
			}
		}

		websWrite(wp, 
			"%-15s %-15s %-6s %-11s %-15s %-11s\n",
			src, dst, proto, range, host, port ? : range);
	}
	fclose(fp);
	unlink("/tmp/vserver.log");

	return ret;
}

#ifdef TCSUPPORT_WEBMON
void StampToDate(unsigned long timestamp, char *date)
{
	struct tm *local;
	time_t now;

	now = timestamp;
	local = localtime(&now);
	strftime(date, 30, "%Y-%m-%d %H:%M:%S", local);
}

void ipSearchMac(char *ipaddr, char *macaddr)
{
	FILE *fp=NULL;
	char s[512], addr[32], hwtype[32], hwaddr[32], flags[32], iface[32];
	int find = 0;

	if((fp = popen("/sbin/arp -n", "r")) != NULL) {
		while(fgets(s, sizeof(s), fp)) {
			if(sscanf(s, "%s\t\t%s\t%s\t%s\t%s", addr, hwtype, hwaddr, flags, iface) != 5) continue;
			if(strcmp(ipaddr, addr) == 0){
				sprintf(macaddr, "%s", hwaddr);
				find = 1;
				break;
			}
		}
		pclose(fp);

		if(find == 0)
			tcdbg_printf("%s: cannot find MAC, ip = %s\n", __FUNCTION__, ipaddr);
	}
}

void get_web_history_table(char *wp)
{
	FILE *f=NULL, *fp=NULL;
	char s[512], ip[64], mac[32], val[256];
	unsigned long time;
	char date[30];
	int i = 0, find = 0;

	fp = fopen("/tmp/web_history.txt", "w");
	if(fp)
	{
		fprintf(fp, "Access Time\t\tIP Address\tMAC Address\t\tDomain Name\r\n");
		if ((f = fopen("/proc/webmon_recent_domains", "r")) != NULL) {
			while(fgets(s, sizeof(s), f)) {
				find = 0;
				if(sscanf(s, "%lu\t\t%s\t%s", &time, ip, val) != 3) continue;
				for(i=0; i<512; i++)
				{
					if(strlen(landev[i].ipaddr) == 0) break;
					if(strcmp(ip, landev[i].ipaddr) == 0){
						sprintf(mac, "%s", landev[i].mac);
						find = 1;
						break;
					}
					//tcdbg_printf("%s: found, landev[%d].ipaddr=%s\n", __FUNCTION__, i, landev[i].ipaddr);
				}
				if(find == 0) {
					ipSearchMac(ip, mac);
					i = i % 512; //more than 512, it'll re-start
					sprintf(landev[i].ipaddr, "%s", ip);
					sprintf(landev[i].mac, "%s", mac);
					//tcdbg_printf("%s: No records, find landev[%d].ipaddr=%s, landev[%d].mac=%s\n", __FUNCTION__, i, landev[i].ipaddr, i, landev[i].mac);
				}
				StampToDate(time, date);
				if(strlen(mac))
					fprintf(fp, "%s\t%s\t%s\t%s\r\n", date, ip, mac, val);
				else
					fprintf(fp, "%s\t%s\t%s\t%s\r\n", date, ip, ip, val);
			}
			fclose(f);
		}
		fclose(fp);
	}
}
#endif

#endif


/*******************************************************************
* NAME: get_iptv_wan_list_ad
* AUTHOR: Andy Chiu
* CREATE DATE: 2016/05/09
* DESCRIPTION: get active wan pvc list of ADSL
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void get_iptv_wan_list_ad(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char node_name[64] = {0};	
	char buf[1024]={0};
	char tmp[512]={0}, tmp2[64];
	int i, flag = 0;

	for(i = 0; i < 8; ++i)
	{
		snprintf(node_name, sizeof(node_name), "Wan_PVC%d", i);
		if(tcapi_match(node_name, "Active", "Yes"))
		{
			if(flag)
				snprintf(tmp2, sizeof(tmp2), ",%d", i);
			else
			{
				flag = 1;
				snprintf(tmp2, sizeof(tmp2), "%d", i);
			}
			strcat(tmp, tmp2);
		}
	}

	websWrite(buf, "%s", tmp);
}

#if defined(TCSUPPORT_WAN_PTM) && defined(TCSUPPORT_MULTISERVICE_ON_WAN)
/*******************************************************************
* NAME: get_iptv_wan_list_vd
* AUTHOR: Andy Chiu
* CREATE DATE: 2016/05/09
* DESCRIPTION: get active wan pvc list of VDSL
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void get_iptv_wan_list_vd(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char node_name[64] = {0};	
	char buf[1024]={0};
	char tmp[512]={0}, tmp2[64];
	int i, flag = 0;

	for(i = 0; i < 8; ++i)
	{
		snprintf(node_name, sizeof(node_name), "WanExt_PVC8e%d", i);
		if(tcapi_match(node_name, "Active", "Yes"))
		{
			if(flag)
				snprintf(tmp2, sizeof(tmp2), ",%d", i);
			else
			{
				flag = 1;
				snprintf(tmp2, sizeof(tmp2), "%d", i);
			}
			strcat(tmp, tmp2);
		}
	}

	websWrite(buf, "%s", tmp);
}
#endif

#if defined(TCSUPPORT_WAN_ETHER) && defined(TCSUPPORT_MULTISERVICE_ON_WAN)
/*******************************************************************
* NAME: get_iptv_wan_list_eth
* AUTHOR: Andy Chiu
* CREATE DATE: 2016/05/09
* DESCRIPTION: get active wan pvc list of Ethernet WAN
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void get_iptv_wan_list_eth(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char node_name[64] = {0};	
	char buf[1024]={0};
	char tmp[512]={0}, tmp2[64];
	int i, flag = 0;

	for(i = 0; i < 8; ++i)
	{
		snprintf(node_name, sizeof(node_name), "WanExt_PVC10e%d", i);
		if(tcapi_match(node_name, "Active", "Yes"))
		{
			if(flag)
				snprintf(tmp2, sizeof(tmp2), ",%d", i);
			else
			{
				flag = 1;
				snprintf(tmp2, sizeof(tmp2), "%d", i);
			}
			strcat(tmp, tmp2);
		}
	}

	websWrite(buf, "%s", tmp);
}
#endif

#ifdef TCSUPPORT_WAN_ETHER_LAN
/*******************************************************************
* NAME: available_eth_lan_wan_port
* AUTHOR: Andy Chiu
* CREATE DATE: 2016/05/11
* DESCRIPTION: get available LAN port for ethernet LAN WAN mode
* INPUT:
* OUTPUT:
* RETURN:
* NOTE:
*******************************************************************/
static void available_eth_lan_wan_port(asp_reent* reent, const asp_text* params, asp_text* ret)
{
	char tmp[128], tmp2[64], node[32],  buf[2048];
	int pvc = -1, i, j, lan[4], brlan, flag;
	const char *wan[] = {"wan_primary", "wan_secondary"};
	
	//init lan port list
	memset(lan, sizeof(lan), 0);

	if(!tcapi_get("Dualwan_Entry", "wans_dualwan", tmp) && strstr(tmp, "dsl"))
	{		
		for(j = 0; j < 2; ++j)
		{
			if(!tcapi_get("Wanduck_Common", wan[j], tmp) && (!strcmp(tmp, "0") || !strcmp(tmp, "8")))
			{
				pvc = atoi(tmp);
				for(i = 0; i < 8; ++i)
				{
					if(!pvc)	//ATM
					{
						snprintf(node, sizeof(node), "Wan_PVC%d", i);
					}
					else if(pvc == 8)	//PTM
					{
						snprintf(node, sizeof(node), "Wan_PVC8e%d", i);
					}

					if(tcapi_match(node, "Active", "Yes") && tcapi_match(node, "ISP", "3"))	//active and bridge mode
					{
						brlan = tcapi_get_int(node, "bridge_lan_port");
						if(brlan > 0 && brlan <= 4)
						{
							lan[brlan -1] = 1;
						}
					}
				}

				flag = 0;
				memset(tmp, sizeof(tmp), 0);
				
				for(i = 0; i < 4; ++i)
				{
					if(!lan[i])
					{
						if(!flag)
						{
							snprintf(tmp, sizeof(tmp), "%d", i + 1);
							flag = 1;
						}		
						else
						{
							snprintf(tmp2, sizeof(tmp2), ",%d", i + 1);
							strcat(tmp, tmp2);
						}
					}
				}

				websWrite(buf, "%s", tmp);
				return;			
			}
		}
	}
	else
		websWrite(buf, "1,2,3,4");
		
}
#endif
