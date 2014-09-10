/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#define _RTW_MLME_EXT_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <wifi.h>
#include <rtw_mlme_ext.h>
#include <wlan_bssdef.h>
#include <mlme_osdep.h>
#include <recv_osdep.h>

#ifdef CONFIG_BT_COEXIST
#ifdef CONFIG_RTL8723A
#include <rtl8723a_hal.h>
#elif defined(CONFIG_RTL8723B)
#include <rtl8723b_hal.h>
#endif
#endif

struct mlme_handler mlme_sta_tbl[]={
	{WIFI_ASSOCREQ,		"OnAssocReq",	&OnAssocReq},
	{WIFI_ASSOCRSP,		"OnAssocRsp",	&OnAssocRsp},
	{WIFI_REASSOCREQ,	"OnReAssocReq",	&OnAssocReq},
	{WIFI_REASSOCRSP,	"OnReAssocRsp",	&OnAssocRsp},
	{WIFI_PROBEREQ,		"OnProbeReq",	&OnProbeReq},
	{WIFI_PROBERSP,		"OnProbeRsp",		&OnProbeRsp},

	/*----------------------------------------------------------
					below 2 are reserved
	-----------------------------------------------------------*/
	{0,					"DoReserved",		&DoReserved},
	{0,					"DoReserved",		&DoReserved},
	{WIFI_BEACON,		"OnBeacon",		&OnBeacon},
	{WIFI_ATIM,			"OnATIM",		&OnAtim},
	{WIFI_DISASSOC,		"OnDisassoc",		&OnDisassoc},
	{WIFI_AUTH,			"OnAuth",		&OnAuthClient},
	{WIFI_DEAUTH,		"OnDeAuth",		&OnDeAuth},
	{WIFI_ACTION,		"OnAction",		&OnAction},
};

#ifdef _CONFIG_NATIVEAP_MLME_
struct mlme_handler mlme_ap_tbl[]={
	{WIFI_ASSOCREQ,		"OnAssocReq",	&OnAssocReq},
	{WIFI_ASSOCRSP,		"OnAssocRsp",	&OnAssocRsp},
	{WIFI_REASSOCREQ,	"OnReAssocReq",	&OnAssocReq},
	{WIFI_REASSOCRSP,	"OnReAssocRsp",	&OnAssocRsp},
	{WIFI_PROBEREQ,		"OnProbeReq",	&OnProbeReq},
	{WIFI_PROBERSP,		"OnProbeRsp",		&OnProbeRsp},

	/*----------------------------------------------------------
					below 2 are reserved
	-----------------------------------------------------------*/
	{0,					"DoReserved",		&DoReserved},
	{0,					"DoReserved",		&DoReserved},
	{WIFI_BEACON,		"OnBeacon",		&OnBeacon},
	{WIFI_ATIM,			"OnATIM",		&OnAtim},
	{WIFI_DISASSOC,		"OnDisassoc",		&OnDisassoc},
	{WIFI_AUTH,			"OnAuth",		&OnAuth},
	{WIFI_DEAUTH,		"OnDeAuth",		&OnDeAuth},
	{WIFI_ACTION,		"OnAction",		&OnAction},
};
#endif

struct action_handler OnAction_tbl[]={
	{RTW_WLAN_CATEGORY_SPECTRUM_MGMT,	 "ACTION_SPECTRUM_MGMT", &DoReserved},
	{RTW_WLAN_CATEGORY_QOS, "ACTION_QOS", &OnAction_qos},
	{RTW_WLAN_CATEGORY_DLS, "ACTION_DLS", &OnAction_dls},
	{RTW_WLAN_CATEGORY_BACK, "ACTION_BACK", &OnAction_back},
	{RTW_WLAN_CATEGORY_PUBLIC, "ACTION_PUBLIC", &OnAction_public},
	{RTW_WLAN_CATEGORY_RADIO_MEASUREMENT, "ACTION_RADIO_MEASUREMENT", &DoReserved},
	{RTW_WLAN_CATEGORY_FT, "ACTION_FT",	&DoReserved},
	{RTW_WLAN_CATEGORY_HT,	"ACTION_HT",	&OnAction_ht},
	{RTW_WLAN_CATEGORY_SA_QUERY, "ACTION_SA_QUERY", &DoReserved},
	{RTW_WLAN_CATEGORY_WMM, "ACTION_WMM", &OnAction_wmm},
	{RTW_WLAN_CATEGORY_P2P, "ACTION_P2P", &OnAction_p2p},
};


u8	null_addr[ETH_ALEN]= {0,0,0,0,0,0};

/**************************************************
OUI definitions for the vendor specific IE
***************************************************/
unsigned char	RTW_WPA_OUI[] = {0x00, 0x50, 0xf2, 0x01};
unsigned char WMM_OUI[] = {0x00, 0x50, 0xf2, 0x02};
unsigned char	WPS_OUI[] = {0x00, 0x50, 0xf2, 0x04};
unsigned char	P2P_OUI[] = {0x50,0x6F,0x9A,0x09};
unsigned char	WFD_OUI[] = {0x50,0x6F,0x9A,0x0A};

unsigned char	WMM_INFO_OUI[] = {0x00, 0x50, 0xf2, 0x02, 0x00, 0x01};
unsigned char	WMM_PARA_OUI[] = {0x00, 0x50, 0xf2, 0x02, 0x01, 0x01};

unsigned char WPA_TKIP_CIPHER[4] = {0x00, 0x50, 0xf2, 0x02};
unsigned char RSN_TKIP_CIPHER[4] = {0x00, 0x0f, 0xac, 0x02};

extern unsigned char REALTEK_96B_IE[];

/********************************************************
MCS rate definitions
*********************************************************/

unsigned char	MCS_rate_2R[16] = {0xff, 0xff, 0x0, 0x0, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
unsigned char	MCS_rate_1R[16] = {0xff, 0x00, 0x0, 0x0, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

/********************************************************
ChannelPlan definitions
*********************************************************/
/*static RT_CHANNEL_PLAN	DefaultChannelPlan[RT_CHANNEL_DOMAIN_MAX] = {
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,100,104,108,112,116,132,136,140,149,153,157,161,165},32},					// 0x00, RT_CHANNEL_DOMAIN_FCC
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165},31},						// 0x01, RT_CHANNEL_DOMAIN_IC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140},32},						// 0x02, RT_CHANNEL_DOMAIN_ETSI
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},																					// 0x03, RT_CHANNEL_DOMAIN_SPAIN
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},																					// 0x04, RT_CHANNEL_DOMAIN_FRANCE
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},																					// 0x05, RT_CHANNEL_DOMAIN_MKK
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},																					// 0x06, RT_CHANNEL_DOMAIN_MKK1
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64},21},																// 0x07, RT_CHANNEL_DOMAIN_ISRAEL
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},															// 0x08, RT_CHANNEL_DOMAIN_TELEC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14},14},																				// 0x09, RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},																					// 0x0A, RT_CHANNEL_DOMAIN_WORLD_WIDE_13
	{{1,2,3,4,5,6,7,8,9,10,11,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165},26},									// 0x0B, RT_CHANNEL_DOMAIN_TAIWAN
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,149,153,157,161,165},18},																// 0x0C, RT_CHANNEL_DOMAIN_CHINA
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,149,153,157,161,165},24},												// 0x0D, RT_CHANNEL_DOMAIN_SINGAPORE_INDIA_MEXICO
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,149,153,157,161,165},31},						// 0x0E, RT_CHANNEL_DOMAIN_KOREA
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64},19},																	// 0x0F, RT_CHANNEL_DOMAIN_TURKEY
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140},32},						// 0x10, RT_CHANNEL_DOMAIN_JAPAN
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,149,153,157,161,165},20},															// 0x11, RT_CHANNEL_DOMAIN_FCC_NO_DFS
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48},17},																		// 0x12, RT_CHANNEL_DOMAIN_JAPAN_NO_DFS
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165},37},	// 0x13, RT_CHANNEL_DOMAIN_WORLD_WIDE_5G
	{{1,2,3,4,5,6,7,8,9,10,11,56,60,64,149,153,157,161,165},19},																// 0x14, RT_CHANNEL_DOMAIN_TAIWAN_NO_DFS
};*/

static RT_CHANNEL_PLAN_2G	RTW_ChannelPlan2G[RT_CHANNEL_DOMAIN_2G_MAX] = {
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},		// 0x00, RT_CHANNEL_DOMAIN_2G_WORLD , Passive scan CH 12, 13
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},		// 0x01, RT_CHANNEL_DOMAIN_2G_ETSI1
	{{1,2,3,4,5,6,7,8,9,10,11},11},			// 0x02, RT_CHANNEL_DOMAIN_2G_FCC1
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14},14},	// 0x03, RT_CHANNEL_DOMAIN_2G_MIKK1
	{{10,11,12,13},4},						// 0x04, RT_CHANNEL_DOMAIN_2G_ETSI2
};

static RT_CHANNEL_PLAN_5G	RTW_ChannelPlan5G[RT_CHANNEL_DOMAIN_5G_MAX] = {
	{{},0},																					// 0x00, RT_CHANNEL_DOMAIN_5G_NULL
	{{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140},19},						// 0x01, RT_CHANNEL_DOMAIN_5G_ETSI1
	{{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165},24},	// 0x02, RT_CHANNEL_DOMAIN_5G_ETSI2
	{{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,149,153,157,161,165},22},			// 0x03, RT_CHANNEL_DOMAIN_5G_ETSI3
	{{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165},24},	// 0x04, RT_CHANNEL_DOMAIN_5G_FCC1
	{{36,40,44,48,149,153,157,161,165},9},														// 0x05, RT_CHANNEL_DOMAIN_5G_FCC2
	{{36,40,44,48,52,56,60,64,149,153,157,161,165},13},											// 0x06, RT_CHANNEL_DOMAIN_5G_FCC3
	{{36,40,44,48,52,56,60,64,149,153,157,161},12},												// 0x07, RT_CHANNEL_DOMAIN_5G_FCC4
	{{149,153,157,161,165},5},																	// 0x08, RT_CHANNEL_DOMAIN_5G_FCC5
	{{36,40,44,48,52,56,60,64},8},																// 0x09, RT_CHANNEL_DOMAIN_5G_FCC6
	{{36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165},20},					// 0x0A, RT_CHANNEL_DOMAIN_5G_FCC7_IC1
	{{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,149,153,157,161,165},20},					// 0x0B, RT_CHANNEL_DOMAIN_5G_KCC1
	{{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140},19},						// 0x0C, RT_CHANNEL_DOMAIN_5G_MKK1
	{{36,40,44,48,52,56,60,64},8},																// 0x0D, RT_CHANNEL_DOMAIN_5G_MKK2
	{{100,104,108,112,116,120,124,128,132,136,140},11},											// 0x0E, RT_CHANNEL_DOMAIN_5G_MKK3
	{{56,60,64,100,104,108,112,116,136,140,149,153,157,161,165},15},								// 0x0F, RT_CHANNEL_DOMAIN_5G_NCC1
	{{56,60,64,149,153,157,161,165},8},															// 0x10, RT_CHANNEL_DOMAIN_5G_NCC2

	//===== Driver self defined for old channel plan Compatible ,Remember to modify if have new channel plan definition =====
	{{36,40,44,48,52,56,60,64,100,104,108,112,116,132,136,140,149,153,157,161,165},21},				// 0x11, RT_CHANNEL_DOMAIN_5G_FCC
	{{36,40,44,48},4},																			// 0x12, RT_CHANNEL_DOMAIN_5G_JAPAN_NO_DFS
};

static RT_CHANNEL_PLAN_MAP	RTW_ChannelPlanMap[RT_CHANNEL_DOMAIN_MAX] = {
	//===== 0x00 ~ 0x1F , Old Define =====
	{0x02,0x11},	//0x00, RT_CHANNEL_DOMAIN_FCC
	{0x02,0x0A},	//0x01, RT_CHANNEL_DOMAIN_IC
	{0x01,0x01},	//0x02, RT_CHANNEL_DOMAIN_ETSI
	{0x01,0x00},	//0x03, RT_CHANNEL_DOMAIN_SPAIN
	{0x04,0x00},	//0x04, RT_CHANNEL_DOMAIN_FRANCE
	{0x03,0x00},	//0x05, RT_CHANNEL_DOMAIN_MKK
	{0x03,0x00},	//0x06, RT_CHANNEL_DOMAIN_MKK1
	{0x01,0x09},	//0x07, RT_CHANNEL_DOMAIN_ISRAEL
	{0x03,0x09},	//0x08, RT_CHANNEL_DOMAIN_TELEC
	{0x03,0x00},	//0x09, RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN
	{0x00,0x00},	//0x0A, RT_CHANNEL_DOMAIN_WORLD_WIDE_13
	{0x02,0x0F},	//0x0B, RT_CHANNEL_DOMAIN_TAIWAN
	{0x01,0x08},	//0x0C, RT_CHANNEL_DOMAIN_CHINA
	{0x02,0x06},	//0x0D, RT_CHANNEL_DOMAIN_SINGAPORE_INDIA_MEXICO
	{0x02,0x0B},	//0x0E, RT_CHANNEL_DOMAIN_KOREA
	{0x02,0x09},	//0x0F, RT_CHANNEL_DOMAIN_TURKEY
#ifdef CONFIG_PLATFORM_HUAWEI_K3V2
	{0x01,0x02},	//0x10, RT_CHANNEL_DOMAIN_JAPAN
#else
	{0x01,0x01},	//0x10, RT_CHANNEL_DOMAIN_JAPAN
#endif
	{0x02,0x05},	//0x11, RT_CHANNEL_DOMAIN_FCC_NO_DFS
	{0x01,0x12},	//0x12, RT_CHANNEL_DOMAIN_JAPAN_NO_DFS
	{0x00,0x04},	//0x13, RT_CHANNEL_DOMAIN_WORLD_WIDE_5G
	{0x02,0x10},	//0x14, RT_CHANNEL_DOMAIN_TAIWAN_NO_DFS
	{0x00,0x00},	//0x15,
	{0x00,0x00},	//0x16,
	{0x00,0x00},	//0x17,
	{0x00,0x00},	//0x18,
	{0x00,0x00},	//0x19,
	{0x00,0x00},	//0x1A,
	{0x00,0x00},	//0x1B,
	{0x00,0x00},	//0x1C,
	{0x00,0x00},	//0x1D,
	{0x00,0x00},	//0x1E,
	{0x00,0x00},	//0x1F,

	//===== 0x20 ~ 0x7F ,New Define =====
	{0x00,0x00},	//0x20, RT_CHANNEL_DOMAIN_WORLD_NULL
	{0x01,0x00},	//0x21, RT_CHANNEL_DOMAIN_ETSI1_NULL
	{0x02,0x00},	//0x22, RT_CHANNEL_DOMAIN_FCC1_NULL
	{0x03,0x00},	//0x23, RT_CHANNEL_DOMAIN_MKK1_NULL
	{0x04,0x00},	//0x24, RT_CHANNEL_DOMAIN_ETSI2_NULL
	{0x02,0x04},	//0x25, RT_CHANNEL_DOMAIN_FCC1_FCC1
	{0x00,0x01},	//0x26, RT_CHANNEL_DOMAIN_WORLD_ETSI1
	{0x03,0x0C},	//0x27, RT_CHANNEL_DOMAIN_MKK1_MKK1
	{0x00,0x0B},	//0x28, RT_CHANNEL_DOMAIN_WORLD_KCC1
	{0x00,0x05},	//0x29, RT_CHANNEL_DOMAIN_WORLD_FCC2
	{0x00,0x00},	//0x2A,
	{0x00,0x00},	//0x2B,
	{0x00,0x00},	//0x2C,
	{0x00,0x00},	//0x2D,
	{0x00,0x00},	//0x2E,
	{0x00,0x00},	//0x2F,
	{0x00,0x06},	//0x30, RT_CHANNEL_DOMAIN_WORLD_FCC3
	{0x00,0x07},	//0x31, RT_CHANNEL_DOMAIN_WORLD_FCC4
	{0x00,0x08},	//0x32, RT_CHANNEL_DOMAIN_WORLD_FCC5
	{0x00,0x09},	//0x33, RT_CHANNEL_DOMAIN_WORLD_FCC6
	{0x02,0x0A},	//0x34, RT_CHANNEL_DOMAIN_FCC1_FCC7
	{0x00,0x02},	//0x35, RT_CHANNEL_DOMAIN_WORLD_ETSI2
	{0x00,0x03},	//0x36, RT_CHANNEL_DOMAIN_WORLD_ETSI3
	{0x03,0x0D},	//0x37, RT_CHANNEL_DOMAIN_MKK1_MKK2
	{0x03,0x0E},	//0x38, RT_CHANNEL_DOMAIN_MKK1_MKK3
	{0x02,0x0F},	//0x39, RT_CHANNEL_DOMAIN_FCC1_NCC1
	{0x00,0x00},	//0x3A,
	{0x00,0x00},	//0x3B,
	{0x00,0x00},	//0x3C,
	{0x00,0x00},	//0x3D,
	{0x00,0x00},	//0x3E,
	{0x00,0x00},	//0x3F,
	{0x02,0x10},	//0x40, RT_CHANNEL_DOMAIN_FCC1_NCC2
};

static RT_CHANNEL_PLAN_MAP	RTW_CHANNEL_PLAN_MAP_REALTEK_DEFINE = {0x03,0x02}; //use the conbination for max channel numbers

/*
 * Search the @param channel_num in given @param channel_set
 * @ch_set: the given channel set
 * @ch: the given channel number
 *
 * return the index of channel_num in channel_set, -1 if not found
*/
int rtw_ch_set_search_ch(RT_CHANNEL_INFO *ch_set, const u32 ch)
{
	int i;
	for(i=0;ch_set[i].ChannelNum!=0;i++){
		if(ch == ch_set[i].ChannelNum)
			break;
	}

	if(i >= ch_set[i].ChannelNum)
		return -1;
	return i;
}

/****************************************************************************

Following are the initialization functions for WiFi MLME

*****************************************************************************/

int init_hw_mlme_ext(_adapter *padapter)
{
	struct	mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;

	//set_opmode_cmd(padapter, infra_client_with_mlme);//removed

	set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);

	return _SUCCESS;
}

static void init_mlme_ext_priv_value(_adapter* padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

#ifdef CONFIG_TDLS
	u8 i;
#endif

	//unsigned char default_channel_set[MAX_CHANNEL_NUM] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 0};
	unsigned char	mixed_datarate[NumRates] = {_1M_RATE_, _2M_RATE_, _5M_RATE_, _11M_RATE_, _6M_RATE_,_9M_RATE_, _12M_RATE_, _18M_RATE_, _24M_RATE_, _36M_RATE_, _48M_RATE_, _54M_RATE_, 0xff};
	unsigned char	mixed_basicrate[NumRates] ={_1M_RATE_, _2M_RATE_, _5M_RATE_, _11M_RATE_, _6M_RATE_, _12M_RATE_, _24M_RATE_, 0xff,};

	ATOMIC_SET(&pmlmeext->event_seq, 0);
	pmlmeext->mgnt_seq = 0;//reset to zero when disconnect at client mode

	pmlmeext->check_ap_processing = _FALSE; //for check whether ap alive
	pmlmeext->cur_channel = padapter->registrypriv.channel;
	pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
	pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
	pmlmeext->retry = 0;

	pmlmeext->cur_wireless_mode = padapter->registrypriv.wireless_mode;

	//_rtw_memcpy(pmlmeext->channel_set, DefaultChannelPlan[padapter->mlmepriv.ChannelPlan].Channel, DefaultChannelPlan[padapter->mlmepriv.ChannelPlan].Len);
	//_rtw_memcpy(pmlmeext->channel_set, default_channel_set, MAX_CHANNEL_NUM);
	_rtw_memcpy(pmlmeext->datarate, mixed_datarate, NumRates);
	_rtw_memcpy(pmlmeext->basicrate, mixed_basicrate, NumRates);

	if(pmlmeext->cur_channel > 14)
		pmlmeext->tx_rate = IEEE80211_OFDM_RATE_6MB;
	else
		pmlmeext->tx_rate = IEEE80211_CCK_RATE_1MB;

	pmlmeext->sitesurvey_res.state = SCAN_DISABLE;
	pmlmeext->sitesurvey_res.channel_idx = 0;
	pmlmeext->sitesurvey_res.bss_cnt = 0;

        pmlmeext->scan_abort = _FALSE;
	pmlmeinfo->state = WIFI_FW_NULL_STATE;
	pmlmeinfo->reauth_count = 0;
	pmlmeinfo->reassoc_count = 0;
	pmlmeinfo->link_count = 0;
	pmlmeinfo->auth_seq = 0;
	pmlmeinfo->auth_algo = dot11AuthAlgrthm_Open;
	pmlmeinfo->key_index = 0;
	pmlmeinfo->iv = 0;

	pmlmeinfo->enc_algo = _NO_PRIVACY_;
	pmlmeinfo->authModeToggle = 0;

	_rtw_memset(pmlmeinfo->chg_txt, 0, 128);

	pmlmeinfo->slotTime = SHORT_SLOT_TIME;
	pmlmeinfo->preamble_mode = PREAMBLE_AUTO;

	pmlmeinfo->dialogToken = 0;
	pmlmeext->action_public_rxseq = 0xffff;
	pmlmeext->action_public_dialog_token = 0xff;
}

static u8 init_channel_set(_adapter* padapter, u8 ChannelPlan, RT_CHANNEL_INFO *channel_set)
{
	u8	index,chanset_size = 0;
	u8	b5GBand = _FALSE, b2_4GBand = _FALSE;
	u8	Index2G = 0, Index5G=0;

	_rtw_memset(channel_set, 0, sizeof(RT_CHANNEL_INFO)*MAX_CHANNEL_NUM);

	if(ChannelPlan >= RT_CHANNEL_DOMAIN_MAX && ChannelPlan != RT_CHANNEL_DOMAIN_REALTEK_DEFINE)
	{
		DBG_871X("ChannelPlan ID %x error !!!!!\n",ChannelPlan);
		return chanset_size;
	}

	if(padapter->registrypriv.wireless_mode & WIRELESS_11G)
	{
		b2_4GBand = _TRUE;
		if(RT_CHANNEL_DOMAIN_REALTEK_DEFINE == ChannelPlan)
			Index2G = RTW_CHANNEL_PLAN_MAP_REALTEK_DEFINE.Index2G;
		else
			Index2G = RTW_ChannelPlanMap[ChannelPlan].Index2G;
	}

	if(padapter->registrypriv.wireless_mode & WIRELESS_11A)
	{
		b5GBand = _TRUE;
		if(RT_CHANNEL_DOMAIN_REALTEK_DEFINE == ChannelPlan)
			Index5G = RTW_CHANNEL_PLAN_MAP_REALTEK_DEFINE.Index5G;
		else
			Index5G = RTW_ChannelPlanMap[ChannelPlan].Index5G;
	}

	if(b2_4GBand)
	{
		for(index=0;index<RTW_ChannelPlan2G[Index2G].Len;index++)
		{
			channel_set[chanset_size].ChannelNum = RTW_ChannelPlan2G[Index2G].Channel[index];

			if(RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN == ChannelPlan) //Channel 1~11 is active, and 12~14 is passive
			{
				if(channel_set[chanset_size].ChannelNum >= 1 && channel_set[chanset_size].ChannelNum <= 11)
					channel_set[chanset_size].ScanType = SCAN_ACTIVE;
				else if((channel_set[chanset_size].ChannelNum  >= 12 && channel_set[chanset_size].ChannelNum  <= 14))
					channel_set[chanset_size].ScanType  = SCAN_PASSIVE;
			}
			else if(RT_CHANNEL_DOMAIN_WORLD_WIDE_13 == ChannelPlan ||
				RT_CHANNEL_DOMAIN_WORLD_WIDE_5G == ChannelPlan ||
				RT_CHANNEL_DOMAIN_2G_WORLD == Index2G)// channel 12~13, passive scan
			{
				if(channel_set[chanset_size].ChannelNum <= 11)
					channel_set[chanset_size].ScanType = SCAN_ACTIVE;
				else
					channel_set[chanset_size].ScanType = SCAN_PASSIVE;
			}
			else
			{
				channel_set[chanset_size].ScanType = SCAN_ACTIVE;
			}

			chanset_size++;
		}
	}

	if(b5GBand)
	{
		for(index=0;index<RTW_ChannelPlan5G[Index5G].Len;index++)
		{
#ifdef CONFIG_DFS
			channel_set[chanset_size].ChannelNum = RTW_ChannelPlan5G[Index5G].Channel[index];
			if ( channel_set[chanset_size].ChannelNum <= 48
				|| channel_set[chanset_size].ChannelNum >= 149 )
			{
				if(RT_CHANNEL_DOMAIN_WORLD_WIDE_5G == ChannelPlan)//passive scan for all 5G channels
					channel_set[chanset_size].ScanType = SCAN_PASSIVE;
				else
					channel_set[chanset_size].ScanType = SCAN_ACTIVE;
			}
			else
			{
				channel_set[chanset_size].ScanType = SCAN_PASSIVE;
			}
			chanset_size++;
#else /* CONFIG_DFS */
			if ( RTW_ChannelPlan5G[Index5G].Channel[index] <= 48
				|| RTW_ChannelPlan5G[Index5G].Channel[index] >= 149 ) {
				channel_set[chanset_size].ChannelNum = RTW_ChannelPlan5G[Index5G].Channel[index];
				if(RT_CHANNEL_DOMAIN_WORLD_WIDE_5G == ChannelPlan)//passive scan for all 5G channels
					channel_set[chanset_size].ScanType = SCAN_PASSIVE;
				else
					channel_set[chanset_size].ScanType = SCAN_ACTIVE;
				DBG_871X("%s(): channel_set[%d].ChannelNum = %d\n", __FUNCTION__, chanset_size, channel_set[chanset_size].ChannelNum);
				chanset_size++;
			}
#endif /* CONFIG_DFS */
		}
	}

	return chanset_size;
}

int	init_mlme_ext_priv(_adapter* padapter)
{
	int	res = _SUCCESS;
	struct registry_priv* pregistrypriv = &padapter->registrypriv;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	// We don't need to memset padapter->XXX to zero, because adapter is allocated by rtw_zvmalloc().
	//_rtw_memset((u8 *)pmlmeext, 0, sizeof(struct mlme_ext_priv));

	pmlmeext->padapter = padapter;

	//fill_fwpriv(padapter, &(pmlmeext->fwpriv));

	init_mlme_ext_priv_value(padapter);
	pmlmeinfo->bAcceptAddbaReq = pregistrypriv->bAcceptAddbaReq;

	init_mlme_ext_timer(padapter);

#ifdef CONFIG_AP_MODE
	init_mlme_ap_info(padapter);
#endif

	pmlmeext->max_chan_nums = init_channel_set(padapter, pmlmepriv->ChannelPlan,pmlmeext->channel_set);

	pmlmeext->chan_scan_time = SURVEY_TO;
	pmlmeext->mlmeext_init = _TRUE;

	set_fwstate(pmlmepriv, WIFI_STATION_STATE);
	return res;

}

void free_mlme_ext_priv (struct mlme_ext_priv *pmlmeext)
{
	_adapter *padapter = pmlmeext->padapter;

	if (!padapter)
		return;

	if (padapter->bDriverStopped == _TRUE)
	{
		_cancel_timer_ex(&pmlmeext->survey_timer);
		_cancel_timer_ex(&pmlmeext->link_timer);
		//_cancel_timer_ex(&pmlmeext->ADDBA_timer);
	}
}

static u8 cmp_pkt_chnl_diff(_adapter *padapter,u8* pframe,uint packet_len)
{	// if the channel is same, return 0. else return channel differential
	uint len;
	u8 channel;
	u8 *p;
	p = rtw_get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len, packet_len - _BEACON_IE_OFFSET_);
	if (p)
	{
		channel = *(p + 2);
		if(padapter->mlmeextpriv.cur_channel >= channel)
		{
			return (padapter->mlmeextpriv.cur_channel - channel);
		}
		else
		{
			return (channel-padapter->mlmeextpriv.cur_channel);
		}
	}
	else
	{
		return 0;
	}
}

static void _mgt_dispatcher(_adapter *padapter, struct mlme_handler *ptable, union recv_frame *precv_frame)
{
	u8 bc_addr[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff};
	u8 *pframe = precv_frame->u.hdr.rx_data;

	  if(ptable->func)
        {
       	 //receive the frames that ra(a1) is my address or ra(a1) is bc address.
		if (!_rtw_memcmp(GetAddr1Ptr(pframe), myid(&padapter->eeprompriv), ETH_ALEN) &&
			!_rtw_memcmp(GetAddr1Ptr(pframe), bc_addr, ETH_ALEN))
		{
			return;
		}

		ptable->func(padapter, precv_frame);
        }

}

void mgt_dispatcher(_adapter *padapter, union recv_frame *precv_frame)
{
	int index;
	struct mlme_handler *ptable;
#ifdef CONFIG_AP_MODE
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
#endif //CONFIG_AP_MODE
	u8 bc_addr[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff};
	u8 *pframe = precv_frame->u.hdr.rx_data;

	RT_TRACE(_module_rtl871x_mlme_c_, _drv_info_,
		 ("+mgt_dispatcher: type(0x%x) subtype(0x%x)\n",
		  GetFrameType(pframe), GetFrameSubType(pframe)));

#if 0
	{
		u8 *pbuf;
		pbuf = GetAddr1Ptr(pframe);
		DBG_871X("A1-%x:%x:%x:%x:%x:%x\n", *pbuf, *(pbuf+1), *(pbuf+2), *(pbuf+3), *(pbuf+4), *(pbuf+5));
		pbuf = GetAddr2Ptr(pframe);
		DBG_871X("A2-%x:%x:%x:%x:%x:%x\n", *pbuf, *(pbuf+1), *(pbuf+2), *(pbuf+3), *(pbuf+4), *(pbuf+5));
		pbuf = GetAddr3Ptr(pframe);
		DBG_871X("A3-%x:%x:%x:%x:%x:%x\n", *pbuf, *(pbuf+1), *(pbuf+2), *(pbuf+3), *(pbuf+4), *(pbuf+5));
	}
#endif

	if (GetFrameType(pframe) != WIFI_MGT_TYPE)
	{
		RT_TRACE(_module_rtl871x_mlme_c_, _drv_err_, ("mgt_dispatcher: type(0x%x) error!\n", GetFrameType(pframe)));
		return;
	}

	//receive the frames that ra(a1) is my address or ra(a1) is bc address.
	if (!_rtw_memcmp(GetAddr1Ptr(pframe), myid(&padapter->eeprompriv), ETH_ALEN) &&
		!_rtw_memcmp(GetAddr1Ptr(pframe), bc_addr, ETH_ALEN))
	{
		return;
	}

	ptable = mlme_sta_tbl;

	index = GetFrameSubType(pframe) >> 4;

#ifdef CONFIG_TDLS
	if((index << 4)==WIFI_ACTION){
		//category==public (4), action==TDLS_DISCOVERY_RESPONSE
		if(*(pframe+24)==0x04 && *(pframe+25)==TDLS_DISCOVERY_RESPONSE){
			DBG_871X("recv tdls discovery response frame\n");
			On_TDLS_Dis_Rsp(padapter, precv_frame);
		}
	}
#endif //CONFIG_TDLS

	if (index > 13)
	{
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("Currently we do not support reserved sub-fr-type=%d\n", index));
		return;
	}
	ptable += index;

#if 0//gtest
	sa = get_sa(pframe);
	psta = search_assoc_sta(sa, padapter);
	// only check last cache seq number for management frame
	if (psta != NULL) {
		if (GetRetry(pframe)) {
			if (GetTupleCache(pframe) == psta->rxcache->nonqos_seq){
				RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("drop due to decache!\n"));
				return;
			}
		}
		psta->rxcache->nonqos_seq = GetTupleCache(pframe);
	}
#else

	if(GetRetry(pframe))
	{
		//RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("drop due to decache!\n"));
		//return;
	}
#endif

#ifdef CONFIG_AP_MODE
	switch (GetFrameSubType(pframe))
	{
		case WIFI_AUTH:
			if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
				ptable->func = &OnAuth;
			else
				ptable->func = &OnAuthClient;
			//pass through
		case WIFI_ASSOCREQ:
		case WIFI_REASSOCREQ:
			_mgt_dispatcher(padapter, ptable, precv_frame);
#ifdef CONFIG_HOSTAPD_MLME
			if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
				rtw_hostapd_mlme_rx(padapter, precv_frame);
#endif
			break;
		case WIFI_PROBEREQ:
			if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
			{
#ifdef CONFIG_HOSTAPD_MLME
				rtw_hostapd_mlme_rx(padapter, precv_frame);
#else
				_mgt_dispatcher(padapter, ptable, precv_frame);
#endif
			}
			else
				_mgt_dispatcher(padapter, ptable, precv_frame);
			break;
		case WIFI_BEACON:
			_mgt_dispatcher(padapter, ptable, precv_frame);
			break;
		case WIFI_ACTION:
			//if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
			_mgt_dispatcher(padapter, ptable, precv_frame);
			break;
		default:
			_mgt_dispatcher(padapter, ptable, precv_frame);
			if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
				rtw_hostapd_mlme_rx(padapter, precv_frame);
			break;
	}
#else

	_mgt_dispatcher(padapter, ptable, precv_frame);

#endif

}

#ifdef CONFIG_P2P
u32 p2p_listen_state_process(_adapter *padapter, unsigned char *da)
{
	issue_probersp_p2p( padapter, da);
	return _SUCCESS;
}
#endif //CONFIG_P2P


/****************************************************************************

Following are the callback functions for each subtype of the management frames

*****************************************************************************/

unsigned int OnProbeReq(_adapter *padapter, union recv_frame *precv_frame)
{
	unsigned int	ielen;
	unsigned char	*p;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 	*cur = &(pmlmeinfo->network);
	u8 *pframe = precv_frame->u.hdr.rx_data;
	uint len = precv_frame->u.hdr.len;
	u8 is_valid_p2p_probereq = _FALSE;

#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
	struct rx_pkt_attrib	*pattrib = &precv_frame->u.hdr.attrib;
	u8 wifi_test_chk_rate = 1;

	if (	!rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE) &&
		!rtw_p2p_chk_state(pwdinfo, P2P_STATE_IDLE) &&
		!rtw_p2p_chk_role(pwdinfo, P2P_ROLE_CLIENT) &&
		!rtw_p2p_chk_state(pwdinfo, P2P_STATE_FIND_PHASE_SEARCH) &&
		!rtw_p2p_chk_state(pwdinfo, P2P_STATE_SCAN)
	   )
	{
		//	Commented by Albert 2011/03/17
		//	mcs_rate = 0 -> CCK 1M rate
		//	mcs_rate = 1 -> CCK 2M rate
		//	mcs_rate = 2 -> CCK 5.5M rate
		//	mcs_rate = 3 -> CCK 11M rate
		//	In the P2P mode, the driver should not support the CCK rate

		//	Commented by Kurt 2012/10/16
		//	IOT issue: Google Nexus7 use 1M rate to send p2p_probe_req after GO nego completed and Nexus7 is client
#ifdef CONFIG_WIFI_TEST
		if ( pattrib->mcs_rate <= 3 )
		{
			wifi_test_chk_rate = 0;
		}
#endif //CONFIG_WIFI_TEST

		if( wifi_test_chk_rate == 1 )
		{
			if((is_valid_p2p_probereq = process_probe_req_p2p_ie(pwdinfo, pframe, len)) == _TRUE)
			{
				if(rtw_p2p_chk_role(pwdinfo, P2P_ROLE_DEVICE))
				{
					p2p_listen_state_process( padapter,  get_sa(pframe));

					return _SUCCESS;
				}

				if(rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO))
				{
					goto _continue;
				}
			}
		}
	}

_continue:
#endif //CONFIG_P2P

	if(check_fwstate(pmlmepriv, WIFI_STATION_STATE))
	{
		return _SUCCESS;
	}

	if(check_fwstate(pmlmepriv, _FW_LINKED) == _FALSE &&
		check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE|WIFI_AP_STATE)==_FALSE)
	{
		return _SUCCESS;
	}


	//DBG_871X("+OnProbeReq\n");

#ifdef CONFIG_CONCURRENT_MODE
	if(((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE) &&
		check_buddy_fwstate(padapter, _FW_UNDER_LINKING|_FW_UNDER_SURVEY))
	{
		//don't process probe req
		return _SUCCESS;
	}
#endif

	p = rtw_get_ie(pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_, _SSID_IE_, (int *)&ielen,
			len - WLAN_HDR_A3_LEN - _PROBEREQ_IE_OFFSET_);


	//check (wildcard) SSID
	if (p != NULL)
	{
		if(is_valid_p2p_probereq == _TRUE)
		{
			goto _issue_probersp;
		}

		if ( (ielen != 0 && _FALSE ==_rtw_memcmp((void *)(p+2), (void *)cur->Ssid.Ssid, cur->Ssid.SsidLength))
			|| (ielen == 0 && pmlmeinfo->hidden_ssid_mode)
		)
		{
			return _SUCCESS;
		}

_issue_probersp:

		if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE &&
			pmlmepriv->cur_network.join_res == _TRUE)
		{
			//DBG_871X("+issue_probersp during ap mode\n");
			issue_probersp(padapter, get_sa(pframe), is_valid_p2p_probereq);
		}

	}

	return _SUCCESS;

}

unsigned int OnProbeRsp(_adapter *padapter, union recv_frame *precv_frame)
{
	struct sta_info		*psta;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct sta_priv		*pstapriv = &padapter->stapriv;
	u8	*pframe = precv_frame->u.hdr.rx_data;
#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &padapter->wdinfo;
#endif


#ifdef CONFIG_P2P
	if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_TX_PROVISION_DIS_REQ))
	{
		if ( _TRUE == pwdinfo->tx_prov_disc_info.benable )
		{
			if( _rtw_memcmp( pwdinfo->tx_prov_disc_info.peerIFAddr, GetAddr2Ptr(pframe), ETH_ALEN ) )
			{
				if(rtw_p2p_chk_role(pwdinfo, P2P_ROLE_CLIENT))
				{
					pwdinfo->tx_prov_disc_info.benable = _FALSE;
					issue_p2p_provision_request( padapter,
												pwdinfo->tx_prov_disc_info.ssid.Ssid,
												pwdinfo->tx_prov_disc_info.ssid.SsidLength,
												pwdinfo->tx_prov_disc_info.peerDevAddr );
				}
				else if ( rtw_p2p_chk_role(pwdinfo, P2P_ROLE_DEVICE) || rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO) )
				{
					pwdinfo->tx_prov_disc_info.benable = _FALSE;
					issue_p2p_provision_request( padapter,
												NULL,
												0,
												pwdinfo->tx_prov_disc_info.peerDevAddr );
				}
			}
		}
		return _SUCCESS;
	}
	else if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_GONEGO_ING))
	{
		if ( _TRUE == pwdinfo->nego_req_info.benable )
		{
			DBG_871X( "[%s] P2P State is GONEGO ING!\n", __FUNCTION__ );
			if( _rtw_memcmp( pwdinfo->nego_req_info.peerDevAddr, GetAddr2Ptr(pframe), ETH_ALEN ) )
			{
				pwdinfo->nego_req_info.benable = _FALSE;
				issue_p2p_GO_request( padapter, pwdinfo->nego_req_info.peerDevAddr);
			}
		}
	}
	else if( rtw_p2p_chk_state(pwdinfo, P2P_STATE_TX_INVITE_REQ ) )
	{
		if ( _TRUE == pwdinfo->invitereq_info.benable )
		{
			DBG_871X( "[%s] P2P_STATE_TX_INVITE_REQ!\n", __FUNCTION__ );
			if( _rtw_memcmp( pwdinfo->invitereq_info.peer_macaddr, GetAddr2Ptr(pframe), ETH_ALEN ) )
			{
				pwdinfo->invitereq_info.benable = _FALSE;
				issue_p2p_invitation_request( padapter, pwdinfo->invitereq_info.peer_macaddr );
			}
		}
	}
#endif


	if (pmlmeext->sitesurvey_res.state == SCAN_PROCESS)
	{
		report_survey_event(padapter, precv_frame);
#ifdef CONFIG_CONCURRENT_MODE
		report_survey_event(padapter->pbuddy_adapter, precv_frame);
#endif
#ifdef CONFIG_DUALMAC_CONCURRENT
		dc_report_survey_event(padapter, precv_frame);
#endif
		return _SUCCESS;
	}

	#if 0 //move to validate_recv_mgnt_frame
	if (_rtw_memcmp(GetAddr3Ptr(pframe), get_my_bssid(&pmlmeinfo->network), ETH_ALEN))
	{
		if (pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS)
		{
			if ((psta = rtw_get_stainfo(pstapriv, GetAddr2Ptr(pframe))) != NULL)
			{
				psta->sta_stats.rx_mgnt_pkts++;
			}
		}
	}
	#endif

	return _SUCCESS;

}

unsigned int OnBeacon(_adapter *padapter, union recv_frame *precv_frame)
{
	int cam_idx;
	struct sta_info	*psta;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct sta_priv	*pstapriv = &padapter->stapriv;
	u8 *pframe = precv_frame->u.hdr.rx_data;
	uint len = precv_frame->u.hdr.len;
	WLAN_BSSID_EX *pbss;
	int ret = _SUCCESS;

	if (pmlmeext->sitesurvey_res.state == SCAN_PROCESS)
	{
		report_survey_event(padapter, precv_frame);
#ifdef CONFIG_CONCURRENT_MODE
		report_survey_event(padapter->pbuddy_adapter, precv_frame);
#endif

#ifdef CONFIG_DUALMAC_CONCURRENT
		dc_report_survey_event(padapter, precv_frame);
#endif

		return _SUCCESS;
	}

	if (_rtw_memcmp(GetAddr3Ptr(pframe), get_my_bssid(&pmlmeinfo->network), ETH_ALEN))
	{
		if (pmlmeinfo->state & WIFI_FW_AUTH_NULL)
		{
#ifndef CONFIG_AUTH_DIRECT_WITHOUT_BCN
			//we should update current network before auth, or some IE is wrong
			pbss = (WLAN_BSSID_EX*)rtw_malloc(sizeof(WLAN_BSSID_EX));
			if (pbss) {
				if (collect_bss_info(padapter, precv_frame, pbss) == _SUCCESS) {
					update_network(&(pmlmepriv->cur_network.network), pbss, padapter, _TRUE);
					rtw_get_bcn_info(&(pmlmepriv->cur_network));
				}
				rtw_mfree((u8*)pbss, sizeof(WLAN_BSSID_EX));
			}
#endif
			//check the vendor of the assoc AP
			pmlmeinfo->assoc_AP_vendor = check_assoc_AP(pframe+sizeof(struct rtw_ieee80211_hdr_3addr), len-sizeof(struct rtw_ieee80211_hdr_3addr));

			//update TSF Value
			update_TSF(pmlmeext, pframe, len);

#ifndef CONFIG_AUTH_DIRECT_WITHOUT_BCN
			//start auth
			start_clnt_auth(padapter);
#endif

			return _SUCCESS;
		}

		if(((pmlmeinfo->state&0x03) == WIFI_FW_STATION_STATE) && (pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS))
		{
			if ((psta = rtw_get_stainfo(pstapriv, GetAddr2Ptr(pframe))) != NULL)
			{
				#ifdef CONFIG_PATCH_JOIN_WRONG_CHANNEL
				//Merge from 8712 FW code
				if (cmp_pkt_chnl_diff(padapter,pframe,len) != 0)
				{            // join wrong channel, deauth and reconnect
					issue_deauth(padapter, (&(pmlmeinfo->network))->MacAddress, WLAN_REASON_DEAUTH_LEAVING);

					report_del_sta_event(padapter,(&(pmlmeinfo->network))->MacAddress, WLAN_REASON_JOIN_WRONG_CHANNEL);
					pmlmeinfo->state &= (~WIFI_FW_ASSOC_SUCCESS);
					return _SUCCESS;
				}
				#endif //CONFIG_PATCH_JOIN_WRONG_CHANNEL

				ret = rtw_check_bcn_info(padapter, pframe, len);
				if (!ret) {
						DBG_871X_LEVEL(_drv_always_, "ap has changed, disconnect now\n ");
						receive_disconnect(padapter, pmlmeinfo->network.MacAddress , 65535);
						return _SUCCESS;
				}
				//update WMM, ERP in the beacon
				//todo: the timer is used instead of the number of the beacon received
				if ((sta_rx_pkts(psta) & 0xf) == 0)
				{
					//DBG_871X("update_bcn_info\n");
					update_beacon_info(padapter, pframe, len, psta);
				}

#ifdef CONFIG_DFS
				process_csa_ie(padapter, pframe, len);	//channel switch announcement
#endif //CONFIG_DFS

#ifdef CONFIG_P2P
				process_p2p_ps_ie(padapter, (pframe + WLAN_HDR_A3_LEN), (len - WLAN_HDR_A3_LEN));
#endif //CONFIG_P2P

				#if 0 //move to validate_recv_mgnt_frame
				psta->sta_stats.rx_mgnt_pkts++;
				#endif
			}
		}
		else if((pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE)
		{
			if ((psta = rtw_get_stainfo(pstapriv, GetAddr2Ptr(pframe))) != NULL)
			{
				//update WMM, ERP in the beacon
				//todo: the timer is used instead of the number of the beacon received
				if ((sta_rx_pkts(psta) & 0xf) == 0)
				{
					//DBG_871X("update_bcn_info\n");
					update_beacon_info(padapter, pframe, len, psta);
				}

				#if 0 //move to validate_recv_mgnt_frame
				psta->sta_stats.rx_mgnt_pkts++;
				#endif
			}
			else
			{
				//allocate a new CAM entry for IBSS station
				if ((cam_idx = allocate_fw_sta_entry(padapter)) == NUM_STA)
				{
					goto _END_ONBEACON_;
				}

				//get supported rate
				if (update_sta_support_rate(padapter, (pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_), (len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_), cam_idx) == _FAIL)
				{
					pmlmeinfo->FW_sta_info[cam_idx].status = 0;
					goto _END_ONBEACON_;
				}

				//update TSF Value
				update_TSF(pmlmeext, pframe, len);

				//report sta add event
				report_add_sta_event(padapter, GetAddr2Ptr(pframe), cam_idx);
			}
		}
	}

_END_ONBEACON_:

	return _SUCCESS;

}

unsigned int OnAuth(_adapter *padapter, union recv_frame *precv_frame)
{
#ifdef CONFIG_AP_MODE
	_irqL irqL;
	unsigned int	auth_mode, seq, ie_len;
	unsigned char	*sa, *p;
	u16	algorithm;
	int	status;
	static struct sta_info stat;
	struct	sta_info	*pstat=NULL;
	struct	sta_priv *pstapriv = &padapter->stapriv;
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u8 *pframe = precv_frame->u.hdr.rx_data;
	uint len = precv_frame->u.hdr.len;


#ifdef CONFIG_CONCURRENT_MODE
	if(((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE) &&
		check_buddy_fwstate(padapter, _FW_UNDER_LINKING|_FW_UNDER_SURVEY))
	{
		//don't process auth request;
		return _SUCCESS;
	}
#endif //CONFIG_CONCURRENT_MODE

	if((pmlmeinfo->state&0x03) != WIFI_FW_AP_STATE)
		return _FAIL;

	DBG_871X("+OnAuth asoc_sta_count:%d\n", pstapriv->asoc_sta_count);

	sa = GetAddr2Ptr(pframe);

	auth_mode = psecuritypriv->dot11AuthAlgrthm;
	seq = cpu_to_le16(*(u16*)((SIZE_PTR)pframe + WLAN_HDR_A3_LEN + 2));
	algorithm = cpu_to_le16(*(u16*)((SIZE_PTR)pframe + WLAN_HDR_A3_LEN));

	if (GetPrivacy(pframe))
	{
#if 0 //TODO: SW rtw_wep_decrypt
		if (SWCRYPTO)
		{
			status = rtw_wep_decrypt(priv, pframe, pfrinfo->pktlen,
				priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm);
			if (status == FALSE)
			{
				SAVE_INT_AND_CLI(flags);
				RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,"wep-decrypt a Auth frame error!\n");
				status = _STATS_CHALLENGE_FAIL_;
				goto auth_fail;
			}
		}

		seq = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + 4 + 2));
		algorithm = cpu_to_le16(*(unsigned short *)((unsigned int)pframe + WLAN_HDR_A3_LEN + 4));
#endif
	}


	DBG_871X("auth alg=%x, seq=%X\n", algorithm, seq);

	if (auth_mode == 2 &&
			psecuritypriv->dot11PrivacyAlgrthm != _WEP40_ &&
			psecuritypriv->dot11PrivacyAlgrthm != _WEP104_)
		auth_mode = 0;

	if ((algorithm > 0 && auth_mode == 0) ||	// rx a shared-key auth but shared not enabled
		(algorithm == 0 && auth_mode == 1) )	// rx a open-system auth but shared-key is enabled
	{
		DBG_871X("auth rejected due to bad alg [alg=%d, auth_mib=%d] %02X%02X%02X%02X%02X%02X\n",
			algorithm, auth_mode, sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);

		status = _STATS_NO_SUPP_ALG_;

		goto auth_fail;
	}

#if 0 //ACL control
	phead = &priv->wlan_acl_list;
	plist = phead->next;
	//check sa
	if (acl_mode == 1)		// 1: positive check, only those on acl_list can be connected.
		res = FAIL;
	else
		res = SUCCESS;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct rtw_wlan_acl_node, list);
		plist = plist->next;
		if (!memcmp((void *)sa, paclnode->addr, 6)) {
			if (paclnode->mode & 2) { // deny
				res = FAIL;
				break;
			}
			else {
				res = SUCCESS;
				break;
			}
		}
	}

	if (res != SUCCESS) {
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,"auth abort because ACL!\n");
		return FAIL;
	}
#else
	if(rtw_access_ctrl(padapter, sa) == _FALSE)
	{
		status = _STATS_UNABLE_HANDLE_STA_;
		goto auth_fail;
	}
#endif

	pstat = rtw_get_stainfo(pstapriv, sa);
	if (pstat == NULL)
	{
		// allocate a new one
		DBG_871X("going to alloc stainfo for sa=%02X%02X%02X%02X%02X%02X\n",  sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
		pstat = rtw_alloc_stainfo(pstapriv, sa);
		if (pstat == NULL)
		{
			DBG_871X(" Exceed the upper limit of supported clients...\n");
			status = _STATS_UNABLE_HANDLE_STA_;
			goto auth_fail;
		}

		pstat->state = WIFI_FW_AUTH_NULL;
		pstat->auth_seq = 0;

		//pstat->flags = 0;
		//pstat->capability = 0;
	}
	else
	{
		_enter_critical_bh(&pstapriv->asoc_list_lock, &irqL);
		if(rtw_is_list_empty(&pstat->asoc_list)==_FALSE)
		{
			rtw_list_delete(&pstat->asoc_list);
			if (pstat->expire_to > 0)
			{
				//TODO: STA re_auth within expire_to
			}
		}
		_exit_critical_bh(&pstapriv->asoc_list_lock, &irqL);

		if (seq==1) {
			//TODO: STA re_auth and auth timeout
		}
	}

	_enter_critical_bh(&pstapriv->auth_list_lock, &irqL);
	if (rtw_is_list_empty(&pstat->auth_list))
	{
		rtw_list_insert_tail(&pstat->auth_list, &pstapriv->auth_list);
	}
	_exit_critical_bh(&pstapriv->auth_list_lock, &irqL);

	if (pstat->auth_seq == 0)
		pstat->expire_to = pstapriv->auth_to;

	if ((pstat->auth_seq + 1) != seq)
	{
		DBG_871X("(1)auth rejected because out of seq [rx_seq=%d, exp_seq=%d]!\n",
			seq, pstat->auth_seq+1);
		status = _STATS_OUT_OF_AUTH_SEQ_;
		goto auth_fail;
	}

	if (algorithm==0 && (auth_mode == 0 || auth_mode == 2))
	{
		if (seq == 1)
		{
			pstat->state &= ~WIFI_FW_AUTH_NULL;
			pstat->state |= WIFI_FW_AUTH_SUCCESS;
			pstat->expire_to = pstapriv->assoc_to;
			pstat->authalg = algorithm;
		}
		else
		{
			DBG_871X("(2)auth rejected because out of seq [rx_seq=%d, exp_seq=%d]!\n",
				seq, pstat->auth_seq+1);
			status = _STATS_OUT_OF_AUTH_SEQ_;
			goto auth_fail;
		}
	}
	else // shared system or auto authentication
	{
		if (seq == 1)
		{
			//prepare for the challenging txt...

			//get_random_bytes((void *)pstat->chg_txt, 128);//TODO:

			pstat->state &= ~WIFI_FW_AUTH_NULL;
			pstat->state |= WIFI_FW_AUTH_STATE;
			pstat->authalg = algorithm;
			pstat->auth_seq = 2;
		}
		else if (seq == 3)
		{
			//checking for challenging txt...
			DBG_871X("checking for challenging txt...\n");

			p = rtw_get_ie(pframe + WLAN_HDR_A3_LEN + 4 + _AUTH_IE_OFFSET_ , _CHLGETXT_IE_, (int *)&ie_len,
					len - WLAN_HDR_A3_LEN - _AUTH_IE_OFFSET_ - 4);

			if((p==NULL) || (ie_len<=0))
			{
				DBG_871X("auth rejected because challenge failure!(1)\n");
				status = _STATS_CHALLENGE_FAIL_;
				goto auth_fail;
			}

			if (_rtw_memcmp((void *)(p + 2), pstat->chg_txt, 128))
			{
				pstat->state &= (~WIFI_FW_AUTH_STATE);
				pstat->state |= WIFI_FW_AUTH_SUCCESS;
				// challenging txt is correct...
				pstat->expire_to =  pstapriv->assoc_to;
			}
			else
			{
				DBG_871X("auth rejected because challenge failure!\n");
				status = _STATS_CHALLENGE_FAIL_;
				goto auth_fail;
			}
		}
		else
		{
			DBG_871X("(3)auth rejected because out of seq [rx_seq=%d, exp_seq=%d]!\n",
				seq, pstat->auth_seq+1);
			status = _STATS_OUT_OF_AUTH_SEQ_;
			goto auth_fail;
		}
	}


	// Now, we are going to issue_auth...
	pstat->auth_seq = seq + 1;

	/* soft ap power save */
	pmlmeext->onauth_time = rtw_get_current_time();
#ifdef CONFIG_NATIVEAP_MLME
	issue_auth(padapter, pstat, (unsigned short)(_STATS_SUCCESSFUL_));
#endif

	if (pstat->state & WIFI_FW_AUTH_SUCCESS)
		pstat->auth_seq = 0;


	return _SUCCESS;

auth_fail:

	if(pstat)
		rtw_free_stainfo(padapter , pstat);

	pstat = &stat;
	_rtw_memset((char *)pstat, '\0', sizeof(stat));
	pstat->auth_seq = 2;
	_rtw_memcpy(pstat->hwaddr, sa, 6);

#ifdef CONFIG_NATIVEAP_MLME
	issue_auth(padapter, pstat, (unsigned short)status);
#endif

#endif
	return _FAIL;

}

unsigned int OnAuthClient(_adapter *padapter, union recv_frame *precv_frame)
{
	unsigned int	seq, len, status, algthm, offset;
	unsigned char	*p;
	unsigned int	go2asoc = 0;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u8 *pframe = precv_frame->u.hdr.rx_data;
	uint pkt_len = precv_frame->u.hdr.len;

	DBG_871X("%s\n", __FUNCTION__);

	//check A1 matches or not
	if (!_rtw_memcmp(myid(&(padapter->eeprompriv)), get_da(pframe), ETH_ALEN))
		return _SUCCESS;

	if (!(pmlmeinfo->state & WIFI_FW_AUTH_STATE))
		return _SUCCESS;

	offset = (GetPrivacy(pframe))? 4: 0;

	algthm 	= le16_to_cpu(*(unsigned short *)((SIZE_PTR)pframe + WLAN_HDR_A3_LEN + offset));
	seq 	= le16_to_cpu(*(unsigned short *)((SIZE_PTR)pframe + WLAN_HDR_A3_LEN + offset + 2));
	status 	= le16_to_cpu(*(unsigned short *)((SIZE_PTR)pframe + WLAN_HDR_A3_LEN + offset + 4));

	if (status != 0)
	{
		DBG_871X("clnt auth fail, status: %d\n", status);
		if(status == 13)//&& pmlmeinfo->auth_algo == dot11AuthAlgrthm_Auto)
		{
			if(pmlmeinfo->auth_algo == dot11AuthAlgrthm_Shared)
				pmlmeinfo->auth_algo = dot11AuthAlgrthm_Open;
			else
				pmlmeinfo->auth_algo = dot11AuthAlgrthm_Shared;
			//pmlmeinfo->reauth_count = 0;
		}

		set_link_timer(pmlmeext, 1);
		goto authclnt_fail;
	}

	if (seq == 2)
	{
		if (pmlmeinfo->auth_algo == dot11AuthAlgrthm_Shared)
		{
			 // legendary shared system
			p = rtw_get_ie(pframe + WLAN_HDR_A3_LEN + _AUTH_IE_OFFSET_, _CHLGETXT_IE_, (int *)&len,
				pkt_len - WLAN_HDR_A3_LEN - _AUTH_IE_OFFSET_);

			if (p == NULL)
			{
				//DBG_871X("marc: no challenge text?\n");
				goto authclnt_fail;
			}

			_rtw_memcpy((void *)(pmlmeinfo->chg_txt), (void *)(p + 2), len);
			pmlmeinfo->auth_seq = 3;
			issue_auth(padapter, NULL, 0);
			set_link_timer(pmlmeext, REAUTH_TO);

			return _SUCCESS;
		}
		else
		{
			// open system
			go2asoc = 1;
		}
	}
	else if (seq == 4)
	{
		if (pmlmeinfo->auth_algo == dot11AuthAlgrthm_Shared)
		{
			go2asoc = 1;
		}
		else
		{
			goto authclnt_fail;
		}
	}
	else
	{
		// this is also illegal
		//DBG_871X("marc: clnt auth failed due to illegal seq=%x\n", seq);
		goto authclnt_fail;
	}

	if (go2asoc)
	{
		DBG_871X_LEVEL(_drv_always_, "auth success, start assoc\n");
		start_clnt_assoc(padapter);
		return _SUCCESS;
	}

authclnt_fail:

	//pmlmeinfo->state &= ~(WIFI_FW_AUTH_STATE);

	return _FAIL;

}

unsigned int OnAssocReq(_adapter *padapter, union recv_frame *precv_frame)
{
#ifdef CONFIG_AP_MODE
	_irqL irqL;
	u16 capab_info, listen_interval;
	struct rtw_ieee802_11_elems elems;
	struct sta_info	*pstat;
	unsigned char		reassoc, *p, *pos, *wpa_ie;
	unsigned char WMM_IE[] = {0x00, 0x50, 0xf2, 0x02, 0x00, 0x01};
	int		i, ie_len, wpa_ie_len, left;
	unsigned char		supportRate[16];
	int					supportRateNum;
	unsigned short		status = _STATS_SUCCESSFUL_;
	unsigned short		frame_type, ie_offset=0;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 	*cur = &(pmlmeinfo->network);
	struct sta_priv *pstapriv = &padapter->stapriv;
	u8 *pframe = precv_frame->u.hdr.rx_data;
	uint pkt_len = precv_frame->u.hdr.len;
#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
	u8 p2p_status_code = P2P_STATUS_SUCCESS;
	u8 *p2pie;
	u32 p2pielen = 0;
#ifdef CONFIG_WFD
	u8	wfd_ie[ 128 ] = { 0x00 };
	u32	wfd_ielen = 0;
#endif // CONFIG_WFD
#endif //CONFIG_P2P

#ifdef CONFIG_CONCURRENT_MODE
	if(((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE) &&
		check_buddy_fwstate(padapter, _FW_UNDER_LINKING|_FW_UNDER_SURVEY))
	{
		//don't process assoc request;
		return _SUCCESS;
	}
#endif //CONFIG_CONCURRENT_MODE

	if((pmlmeinfo->state&0x03) != WIFI_FW_AP_STATE)
		return _FAIL;

	frame_type = GetFrameSubType(pframe);
	if (frame_type == WIFI_ASSOCREQ)
	{
		reassoc = 0;
		ie_offset = _ASOCREQ_IE_OFFSET_;
	}
	else // WIFI_REASSOCREQ
	{
		reassoc = 1;
		ie_offset = _REASOCREQ_IE_OFFSET_;
	}


	if (pkt_len < IEEE80211_3ADDR_LEN + ie_offset) {
		DBG_871X("handle_assoc(reassoc=%d) - too short payload (len=%lu)"
		       "\n", reassoc, (unsigned long)pkt_len);
		return _FAIL;
	}

	pstat = rtw_get_stainfo(pstapriv, GetAddr2Ptr(pframe));
	if (pstat == (struct sta_info *)NULL)
	{
		status = _RSON_CLS2_;
		goto asoc_class2_error;
	}

	capab_info = RTW_GET_LE16(pframe + WLAN_HDR_A3_LEN);
	//capab_info = le16_to_cpu(*(unsigned short *)(pframe + WLAN_HDR_A3_LEN));
	//listen_interval = le16_to_cpu(*(unsigned short *)(pframe + WLAN_HDR_A3_LEN+2));
	listen_interval = RTW_GET_LE16(pframe + WLAN_HDR_A3_LEN+2);

	left = pkt_len - (IEEE80211_3ADDR_LEN + ie_offset);
	pos = pframe + (IEEE80211_3ADDR_LEN + ie_offset);


	DBG_871X("%s\n", __FUNCTION__);

	// check if this stat has been successfully authenticated/assocated
	if (!((pstat->state) & WIFI_FW_AUTH_SUCCESS))
	{
		if (!((pstat->state) & WIFI_FW_ASSOC_SUCCESS))
		{
			status = _RSON_CLS2_;
			goto asoc_class2_error;
		}
		else
		{
			pstat->state &= (~WIFI_FW_ASSOC_SUCCESS);
			pstat->state |= WIFI_FW_ASSOC_STATE;
		}
	}
	else
	{
		pstat->state &= (~WIFI_FW_AUTH_SUCCESS);
		pstat->state |= WIFI_FW_ASSOC_STATE;
	}


#if 0// todo:tkip_countermeasures
	if (hapd->tkip_countermeasures) {
		resp = WLAN_REASON_MICHAEL_MIC_FAILURE;
		goto fail;
	}
#endif

	pstat->capability = capab_info;

#if 0//todo:
	//check listen_interval
	if (listen_interval > hapd->conf->max_listen_interval) {
		hostapd_logger(hapd, mgmt->sa, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "Too large Listen Interval (%d)",
			       listen_interval);
		resp = WLAN_STATUS_ASSOC_DENIED_LISTEN_INT_TOO_LARGE;
		goto fail;
	}

	pstat->listen_interval = listen_interval;
#endif

	//now parse all ieee802_11 ie to point to elems
	if (rtw_ieee802_11_parse_elems(pos, left, &elems, 1) == ParseFailed ||
	    !elems.ssid) {
		DBG_871X("STA " MAC_FMT " sent invalid association request\n",
		       MAC_ARG(pstat->hwaddr));
		status = _STATS_FAILURE_;
		goto OnAssocReqFail;
	}


	// now we should check all the fields...
	// checking SSID
	p = rtw_get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _SSID_IE_, &ie_len,
		pkt_len - WLAN_HDR_A3_LEN - ie_offset);
	if (p == NULL)
	{
		status = _STATS_FAILURE_;
	}

	if (ie_len == 0) // broadcast ssid, however it is not allowed in assocreq
		status = _STATS_FAILURE_;
	else
	{
		// check if ssid match
		if (!_rtw_memcmp((void *)(p+2), cur->Ssid.Ssid, cur->Ssid.SsidLength))
			status = _STATS_FAILURE_;

		if (ie_len != cur->Ssid.SsidLength)
			status = _STATS_FAILURE_;
	}

	if(_STATS_SUCCESSFUL_ != status)
		goto OnAssocReqFail;

	// check if the supported rate is ok
	p = rtw_get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _SUPPORTEDRATES_IE_, &ie_len, pkt_len - WLAN_HDR_A3_LEN - ie_offset);
	if (p == NULL) {
		DBG_871X("Rx a sta assoc-req which supported rate is empty!\n");
		// use our own rate set as statoin used
		//_rtw_memcpy(supportRate, AP_BSSRATE, AP_BSSRATE_LEN);
		//supportRateNum = AP_BSSRATE_LEN;

		status = _STATS_FAILURE_;
		goto OnAssocReqFail;
	}
	else {
		_rtw_memcpy(supportRate, p+2, ie_len);
		supportRateNum = ie_len;

		p = rtw_get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _EXT_SUPPORTEDRATES_IE_ , &ie_len,
				pkt_len - WLAN_HDR_A3_LEN - ie_offset);
		if (p !=  NULL) {

			if(supportRateNum<=sizeof(supportRate))
			{
				_rtw_memcpy(supportRate+supportRateNum, p+2, ie_len);
				supportRateNum += ie_len;
			}
		}
	}

	//todo: mask supportRate between AP & STA -> move to update raid
	//get_matched_rate(pmlmeext, supportRate, &supportRateNum, 0);

	//update station supportRate
	pstat->bssratelen = supportRateNum;
	_rtw_memcpy(pstat->bssrateset, supportRate, supportRateNum);
	UpdateBrateTblForSoftAP(pstat->bssrateset, pstat->bssratelen);

	//check RSN/WPA/WPS
	pstat->dot8021xalg = 0;
      	pstat->wpa_psk = 0;
	pstat->wpa_group_cipher = 0;
	pstat->wpa2_group_cipher = 0;
	pstat->wpa_pairwise_cipher = 0;
	pstat->wpa2_pairwise_cipher = 0;
	_rtw_memset(pstat->wpa_ie, 0, sizeof(pstat->wpa_ie));
	if((psecuritypriv->wpa_psk & BIT(1)) && elems.rsn_ie) {

		int group_cipher=0, pairwise_cipher=0;

		wpa_ie = elems.rsn_ie;
		wpa_ie_len = elems.rsn_ie_len;

		if(rtw_parse_wpa2_ie(wpa_ie-2, wpa_ie_len+2, &group_cipher, &pairwise_cipher, NULL) == _SUCCESS)
		{
			pstat->dot8021xalg = 1;//psk,  todo:802.1x
			pstat->wpa_psk |= BIT(1);

			pstat->wpa2_group_cipher = group_cipher&psecuritypriv->wpa2_group_cipher;
			pstat->wpa2_pairwise_cipher = pairwise_cipher&psecuritypriv->wpa2_pairwise_cipher;

			if(!pstat->wpa2_group_cipher)
				status = WLAN_STATUS_GROUP_CIPHER_NOT_VALID;

			if(!pstat->wpa2_pairwise_cipher)
				status = WLAN_STATUS_PAIRWISE_CIPHER_NOT_VALID;
		}
		else
		{
			status = WLAN_STATUS_INVALID_IE;
		}

	} else if ((psecuritypriv->wpa_psk & BIT(0)) && elems.wpa_ie) {

		int group_cipher=0, pairwise_cipher=0;

		wpa_ie = elems.wpa_ie;
		wpa_ie_len = elems.wpa_ie_len;

		if(rtw_parse_wpa_ie(wpa_ie-2, wpa_ie_len+2, &group_cipher, &pairwise_cipher, NULL) == _SUCCESS)
		{
			pstat->dot8021xalg = 1;//psk,  todo:802.1x
			pstat->wpa_psk |= BIT(0);

			pstat->wpa_group_cipher = group_cipher&psecuritypriv->wpa_group_cipher;
			pstat->wpa_pairwise_cipher = pairwise_cipher&psecuritypriv->wpa_pairwise_cipher;

			if(!pstat->wpa_group_cipher)
				status = WLAN_STATUS_GROUP_CIPHER_NOT_VALID;

			if(!pstat->wpa_pairwise_cipher)
				status = WLAN_STATUS_PAIRWISE_CIPHER_NOT_VALID;

		}
		else
		{
			status = WLAN_STATUS_INVALID_IE;
		}

	} else {
		wpa_ie = NULL;
		wpa_ie_len = 0;
	}

	if(_STATS_SUCCESSFUL_ != status)
		goto OnAssocReqFail;

	pstat->flags &= ~(WLAN_STA_WPS | WLAN_STA_MAYBE_WPS);
	//if (hapd->conf->wps_state && wpa_ie == NULL) { //todo: to check ap if supporting WPS
	if(wpa_ie == NULL) {
		if (elems.wps_ie) {
			DBG_871X("STA included WPS IE in "
				   "(Re)Association Request - assume WPS is "
				   "used\n");
			pstat->flags |= WLAN_STA_WPS;
			//wpabuf_free(sta->wps_ie);
			//sta->wps_ie = wpabuf_alloc_copy(elems.wps_ie + 4,
			//				elems.wps_ie_len - 4);
		} else {
			DBG_871X("STA did not include WPA/RSN IE "
				   "in (Re)Association Request - possible WPS "
				   "use\n");
			pstat->flags |= WLAN_STA_MAYBE_WPS;
		}


		// AP support WPA/RSN, and sta is going to do WPS, but AP is not ready
		// that the selected registrar of AP is _FLASE
		if((psecuritypriv->wpa_psk >0)
			&& (pstat->flags & (WLAN_STA_WPS|WLAN_STA_MAYBE_WPS)))
		{
			if(pmlmepriv->wps_beacon_ie)
			{
				u8 selected_registrar = 0;

				rtw_get_wps_attr_content(pmlmepriv->wps_beacon_ie, pmlmepriv->wps_beacon_ie_len, WPS_ATTR_SELECTED_REGISTRAR , &selected_registrar, NULL);

				if(!selected_registrar)
				{
					DBG_871X("selected_registrar is _FALSE , or AP is not ready to do WPS\n");

					status = _STATS_UNABLE_HANDLE_STA_;

					goto OnAssocReqFail;
				}
			}
		}

	}
	else
	{
		int copy_len;

		if(psecuritypriv->wpa_psk == 0)
		{
			DBG_871X("STA " MAC_FMT ": WPA/RSN IE in association "
		       	"request, but AP don't support WPA/RSN\n", MAC_ARG(pstat->hwaddr));

			status = WLAN_STATUS_INVALID_IE;

			goto OnAssocReqFail;

		}

		if (elems.wps_ie) {
			DBG_871X("STA included WPS IE in "
				   "(Re)Association Request - WPS is "
				   "used\n");
			pstat->flags |= WLAN_STA_WPS;
			copy_len=0;
		}
		else
		{
			copy_len = ((wpa_ie_len+2) > sizeof(pstat->wpa_ie)) ? (sizeof(pstat->wpa_ie)):(wpa_ie_len+2);
		}


		if(copy_len>0)
			_rtw_memcpy(pstat->wpa_ie, wpa_ie-2, copy_len);

	}


	// check if there is WMM IE & support WWM-PS
	pstat->flags &= ~WLAN_STA_WME;
	pstat->qos_option = 0;
	pstat->qos_info = 0;
	pstat->has_legacy_ac = _TRUE;
	pstat->uapsd_vo = 0;
	pstat->uapsd_vi = 0;
	pstat->uapsd_be = 0;
	pstat->uapsd_bk = 0;
	if (pmlmepriv->qospriv.qos_option)
	{
		p = pframe + WLAN_HDR_A3_LEN + ie_offset; ie_len = 0;
		for (;;)
		{
			p = rtw_get_ie(p, _VENDOR_SPECIFIC_IE_, &ie_len, pkt_len - WLAN_HDR_A3_LEN - ie_offset);
			if (p != NULL) {
				if (_rtw_memcmp(p+2, WMM_IE, 6)) {

					pstat->flags |= WLAN_STA_WME;

					pstat->qos_option = 1;
					pstat->qos_info = *(p+8);

					pstat->max_sp_len = (pstat->qos_info>>5)&0x3;

					if((pstat->qos_info&0xf) !=0xf)
						pstat->has_legacy_ac = _TRUE;
					else
						pstat->has_legacy_ac = _FALSE;

					if(pstat->qos_info&0xf)
					{
						if(pstat->qos_info&BIT(0))
							pstat->uapsd_vo = BIT(0)|BIT(1);
						else
							pstat->uapsd_vo = 0;

						if(pstat->qos_info&BIT(1))
							pstat->uapsd_vi = BIT(0)|BIT(1);
						else
							pstat->uapsd_vi = 0;

						if(pstat->qos_info&BIT(2))
							pstat->uapsd_bk = BIT(0)|BIT(1);
						else
							pstat->uapsd_bk = 0;

						if(pstat->qos_info&BIT(3))
							pstat->uapsd_be = BIT(0)|BIT(1);
						else
							pstat->uapsd_be = 0;

					}

					break;
				}
			}
			else {
				break;
			}
			p = p + ie_len + 2;
		}
	}


#ifdef CONFIG_80211N_HT
	/* save HT capabilities in the sta object */
	_rtw_memset(&pstat->htpriv.ht_cap, 0, sizeof(struct rtw_ieee80211_ht_cap));
	if (elems.ht_capabilities && elems.ht_capabilities_len >= sizeof(struct rtw_ieee80211_ht_cap))
	{
		pstat->flags |= WLAN_STA_HT;

		pstat->flags |= WLAN_STA_WME;

		_rtw_memcpy(&pstat->htpriv.ht_cap, elems.ht_capabilities, sizeof(struct rtw_ieee80211_ht_cap));

	} else
		pstat->flags &= ~WLAN_STA_HT;


	if((pmlmepriv->htpriv.ht_option == _FALSE) && (pstat->flags&WLAN_STA_HT))
	{
		status = _STATS_FAILURE_;
		goto OnAssocReqFail;
	}


	if ((pstat->flags & WLAN_STA_HT) &&
		    ((pstat->wpa2_pairwise_cipher&WPA_CIPHER_TKIP) ||
		      (pstat->wpa_pairwise_cipher&WPA_CIPHER_TKIP)))
	{
		DBG_871X("HT: " MAC_FMT " tried to "
				   "use TKIP with HT association\n", MAC_ARG(pstat->hwaddr));

		//status = WLAN_STATUS_CIPHER_REJECTED_PER_POLICY;
		//goto OnAssocReqFail;
	}
#endif /* CONFIG_80211N_HT */

       //
       //if (hapd->iface->current_mode->mode == HOSTAPD_MODE_IEEE80211G)//?
	pstat->flags |= WLAN_STA_NONERP;
	for (i = 0; i < pstat->bssratelen; i++) {
		if ((pstat->bssrateset[i] & 0x7f) > 22) {
			pstat->flags &= ~WLAN_STA_NONERP;
			break;
		}
	}

	if (pstat->capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
		pstat->flags |= WLAN_STA_SHORT_PREAMBLE;
	else
		pstat->flags &= ~WLAN_STA_SHORT_PREAMBLE;



	if (status != _STATS_SUCCESSFUL_)
		goto OnAssocReqFail;

#ifdef CONFIG_P2P
	pstat->is_p2p_device = _FALSE;
	if(rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO))
	{
		if( (p2pie=rtw_get_p2p_ie(pframe + WLAN_HDR_A3_LEN + ie_offset , pkt_len - WLAN_HDR_A3_LEN - ie_offset , NULL, &p2pielen)))
		{
			pstat->is_p2p_device = _TRUE;
			if((p2p_status_code=(u8)process_assoc_req_p2p_ie(pwdinfo, pframe, pkt_len, pstat))>0)
			{
				pstat->p2p_status_code = p2p_status_code;
				status = _STATS_CAP_FAIL_;
				goto OnAssocReqFail;
			}
		}
#ifdef CONFIG_WFD
		if(rtw_get_wfd_ie(pframe + WLAN_HDR_A3_LEN + ie_offset , pkt_len - WLAN_HDR_A3_LEN - ie_offset , wfd_ie, &wfd_ielen ))
		{
			u8	attr_content[ 10 ] = { 0x00 };
			u32	attr_contentlen = 0;

			DBG_8192C( "[%s] WFD IE Found!!\n", __FUNCTION__ );
			rtw_get_wfd_attr_content( wfd_ie, wfd_ielen, WFD_ATTR_DEVICE_INFO, attr_content, &attr_contentlen);
			if ( attr_contentlen )
			{
				pwdinfo->wfd_info->peer_rtsp_ctrlport = RTW_GET_BE16( attr_content + 2 );
				DBG_8192C( "[%s] Peer PORT NUM = %d\n", __FUNCTION__, pwdinfo->wfd_info->peer_rtsp_ctrlport );
			}
		}
#endif
	}
	pstat->p2p_status_code = p2p_status_code;
#endif //CONFIG_P2P

	//TODO: identify_proprietary_vendor_ie();
	// Realtek proprietary IE
	// identify if this is Broadcom sta
	// identify if this is ralink sta
	// Customer proprietary IE



	/* get a unique AID */
	if (pstat->aid > 0) {
		DBG_871X("  old AID %d\n", pstat->aid);
	} else {
		for (pstat->aid = 1; pstat->aid <= NUM_STA; pstat->aid++)
			if (pstapriv->sta_aid[pstat->aid - 1] == NULL)
				break;

		//if (pstat->aid > NUM_STA) {
		if (pstat->aid > pstapriv->max_num_sta) {

			pstat->aid = 0;

			DBG_871X("  no room for more AIDs\n");

			status = WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA;

			goto OnAssocReqFail;


		} else {
			pstapriv->sta_aid[pstat->aid - 1] = pstat;
			DBG_871X("allocate new AID = (%d)\n", pstat->aid);
		}
	}


	pstat->state &= (~WIFI_FW_ASSOC_STATE);
	pstat->state |= WIFI_FW_ASSOC_SUCCESS;

	_enter_critical_bh(&pstapriv->auth_list_lock, &irqL);
	if (!rtw_is_list_empty(&pstat->auth_list))
	{
		rtw_list_delete(&pstat->auth_list);
	}
	_exit_critical_bh(&pstapriv->auth_list_lock, &irqL);

	_enter_critical_bh(&pstapriv->asoc_list_lock, &irqL);
	if (rtw_is_list_empty(&pstat->asoc_list))
	{
		pstat->expire_to = pstapriv->expire_to;
		rtw_list_insert_tail(&pstat->asoc_list, &pstapriv->asoc_list);
	}
	_exit_critical_bh(&pstapriv->asoc_list_lock, &irqL);

	// now the station is qualified to join our BSS...
	if(pstat && (pstat->state & WIFI_FW_ASSOC_SUCCESS) && (_STATS_SUCCESSFUL_==status))
	{
#ifdef CONFIG_NATIVEAP_MLME
		//.1 bss_cap_update & sta_info_update
		bss_cap_update_on_sta_join(padapter, pstat);
		sta_info_update(padapter, pstat);

		//.2 - report to upper layer
		DBG_871X("indicate_sta_join_event to upper layer - hostapd\n");
		{
			#ifdef CONFIG_IOCTL_CFG80211
			struct wireless_dev *pwdev = padapter->rtw_wdev;
			if(pwdev->iftype == NL80211_IFTYPE_AP
#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)) || defined(COMPAT_KERNEL_RELEASE))
				|| pwdev->iftype == NL80211_IFTYPE_P2P_GO
#endif
				) {
				#ifdef COMPAT_KERNEL_RELEASE
				rtw_cfg80211_indicate_sta_assoc(padapter, pframe, pkt_len);
				#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)) && !defined(CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER)
				rtw_cfg80211_indicate_sta_assoc(padapter, pframe, pkt_len);
				#else //(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)) && !defined(CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER)
				_enter_critical_bh(&pstat->lock, &irqL);
				if(pstat->passoc_req)
				{
					rtw_mfree(pstat->passoc_req, pstat->assoc_req_len);
					pstat->passoc_req = NULL;
					pstat->assoc_req_len = 0;
				}

				pstat->passoc_req =  rtw_zmalloc(pkt_len);
				if(pstat->passoc_req)
				{
					_rtw_memcpy(pstat->passoc_req, pframe, pkt_len);
					pstat->assoc_req_len = pkt_len;
				}
				_exit_critical_bh(&pstat->lock, &irqL);
				#endif //(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)) && !defined(CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER)
			}
			else
			#endif //CONFIG_IOCTL_CFG80211
			{
				rtw_indicate_sta_assoc_event(padapter, pstat);
			}
		}

		//.3-(1) report sta add event
		report_add_sta_event(padapter, pstat->hwaddr, pstat->aid);

		if (frame_type == WIFI_ASSOCREQ)
			issue_asocrsp(padapter, status, pstat, WIFI_ASSOCRSP);
		else
			issue_asocrsp(padapter, status, pstat, WIFI_REASSOCRSP);

#endif
	}

	return _SUCCESS;

asoc_class2_error:

#ifdef CONFIG_NATIVEAP_MLME
	issue_deauth(padapter, (void *)GetAddr2Ptr(pframe), status);
#endif

	return _FAIL;

OnAssocReqFail:


#ifdef CONFIG_NATIVEAP_MLME
	pstat->aid = 0;
	if (frame_type == WIFI_ASSOCREQ)
		issue_asocrsp(padapter, status, pstat, WIFI_ASSOCRSP);
	else
		issue_asocrsp(padapter, status, pstat, WIFI_REASSOCRSP);
#endif


#endif /* CONFIG_AP_MODE */

	return _FAIL;

}

unsigned int OnAssocRsp(_adapter *padapter, union recv_frame *precv_frame)
{
	uint i;
	int res;
	unsigned short	status;
	PNDIS_802_11_VARIABLE_IEs	pIE;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	//WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	u8 *pframe = precv_frame->u.hdr.rx_data;
	uint pkt_len = precv_frame->u.hdr.len;
	PNDIS_802_11_VARIABLE_IEs	pWapiIE = NULL;

	DBG_871X("%s\n", __FUNCTION__);

	//check A1 matches or not
	if (!_rtw_memcmp(myid(&(padapter->eeprompriv)), get_da(pframe), ETH_ALEN))
		return _SUCCESS;

	if (!(pmlmeinfo->state & (WIFI_FW_AUTH_SUCCESS | WIFI_FW_ASSOC_STATE)))
		return _SUCCESS;

	if (pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS)
		return _SUCCESS;

	_cancel_timer_ex(&pmlmeext->link_timer);

	//status
	if ((status = le16_to_cpu(*(unsigned short *)(pframe + WLAN_HDR_A3_LEN + 2))) > 0)
	{
		DBG_871X("assoc reject, status code: %d\n", status);
		pmlmeinfo->state = WIFI_FW_NULL_STATE;
		res = -4;
		goto report_assoc_result;
	}

	//get capabilities
	pmlmeinfo->capability = le16_to_cpu(*(unsigned short *)(pframe + WLAN_HDR_A3_LEN));

	//set slot time
	pmlmeinfo->slotTime = (pmlmeinfo->capability & BIT(10))? 9: 20;

	//AID
	res = pmlmeinfo->aid = (int)(le16_to_cpu(*(unsigned short *)(pframe + WLAN_HDR_A3_LEN + 4))&0x3fff);

	//following are moved to join event callback function
	//to handle HT, WMM, rate adaptive, update MAC reg
	//for not to handle the synchronous IO in the tasklet
	for (i = (6 + WLAN_HDR_A3_LEN); i < pkt_len;)
	{
		pIE = (PNDIS_802_11_VARIABLE_IEs)(pframe + i);

		switch (pIE->ElementID)
		{
			case _VENDOR_SPECIFIC_IE_:
				if (_rtw_memcmp(pIE->data, WMM_PARA_OUI, 6))	//WMM
				{
					WMM_param_handler(padapter, pIE);
				}
#if defined(CONFIG_P2P) && defined(CONFIG_WFD)
				else if ( _rtw_memcmp(pIE->data, WFD_OUI, 4))		//WFD
				{
					DBG_871X( "[%s] Found WFD IE\n", __FUNCTION__ );
					WFD_info_handler( padapter, pIE );
				}
#endif
				break;

#ifdef CONFIG_WAPI_SUPPORT
			case _WAPI_IE_:
				pWapiIE = pIE;
				break;
#endif

			case _HT_CAPABILITY_IE_:	//HT caps
				HT_caps_handler(padapter, pIE);
				break;

			case _HT_EXTRA_INFO_IE_:	//HT info
				HT_info_handler(padapter, pIE);
				break;

			case _ERPINFO_IE_:
				ERP_IE_handler(padapter, pIE);

			default:
				break;
		}

		i += (pIE->Length + 2);
	}

#ifdef CONFIG_WAPI_SUPPORT
	rtw_wapi_on_assoc_ok(padapter, pIE);
#endif

	pmlmeinfo->state &= (~WIFI_FW_ASSOC_STATE);
	pmlmeinfo->state |= WIFI_FW_ASSOC_SUCCESS;

	//Update Basic Rate Table for spec, 2010-12-28 , by thomas
	UpdateBrateTbl(padapter, pmlmeinfo->network.SupportedRates);

report_assoc_result:

	report_join_res(padapter, res);

	return _SUCCESS;
}

unsigned int OnDeAuth(_adapter *padapter, union recv_frame *precv_frame)
{
	unsigned short	reason;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u8 *pframe = precv_frame->u.hdr.rx_data;

#ifdef CONFIG_WOWLAN
	//block here for wowlan system suspend only, Added by YJ,121218
	if(adapter_to_pwrctl(padapter)->wowlan_mode){
		return _SUCCESS;
	}
#endif
	//check A3
	if (!(_rtw_memcmp(GetAddr3Ptr(pframe), get_my_bssid(&pmlmeinfo->network), ETH_ALEN)))
		return _SUCCESS;

	reason = le16_to_cpu(*(unsigned short *)(pframe + WLAN_HDR_A3_LEN));

	DBG_871X("%s Reason code(%d)\n", __FUNCTION__,reason);

#ifdef CONFIG_AP_MODE
	if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
	{
		_irqL irqL;
		struct sta_info *psta;
		struct sta_priv *pstapriv = &padapter->stapriv;

		//_enter_critical_bh(&(pstapriv->sta_hash_lock), &irqL);
		//rtw_free_stainfo(padapter, psta);
		//_exit_critical_bh(&(pstapriv->sta_hash_lock), &irqL);

		DBG_871X_LEVEL(_drv_always_, "ap recv deauth reason code(%d) sta:%pM\n",
			       	reason, GetAddr2Ptr(pframe));

		psta = rtw_get_stainfo(pstapriv, GetAddr2Ptr(pframe));
		if(psta)
		{
			u8 updated;

			_enter_critical_bh(&pstapriv->asoc_list_lock, &irqL);
			if(rtw_is_list_empty(&psta->asoc_list)==_FALSE)
			{
				rtw_list_delete(&psta->asoc_list);
				updated = ap_free_sta(padapter, psta, _FALSE);

			}
			_exit_critical_bh(&pstapriv->asoc_list_lock, &irqL);

			associated_clients_update(padapter, updated);
		}


		return _SUCCESS;
	}
	else
#endif
	{
		DBG_871X_LEVEL(_drv_always_, "sta recv deauth reason code(%d) sta:%pM\n",
			       	reason, GetAddr3Ptr(pframe));

		receive_disconnect(padapter, GetAddr3Ptr(pframe) ,reason);
	}
	pmlmepriv->LinkDetectInfo.bBusyTraffic = _FALSE;
	return _SUCCESS;

}

unsigned int OnDisassoc(_adapter *padapter, union recv_frame *precv_frame)
{
	unsigned short	reason;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u8 *pframe = precv_frame->u.hdr.rx_data;

	//check A3
	if (!(_rtw_memcmp(GetAddr3Ptr(pframe), get_my_bssid(&pmlmeinfo->network), ETH_ALEN)))
		return _SUCCESS;

	reason = le16_to_cpu(*(unsigned short *)(pframe + WLAN_HDR_A3_LEN));

        DBG_871X("%s Reason code(%d)\n", __FUNCTION__,reason);

#ifdef CONFIG_AP_MODE
	if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
	{
		_irqL irqL;
		struct sta_info *psta;
		struct sta_priv *pstapriv = &padapter->stapriv;

		//_enter_critical_bh(&(pstapriv->sta_hash_lock), &irqL);
		//rtw_free_stainfo(padapter, psta);
		//_exit_critical_bh(&(pstapriv->sta_hash_lock), &irqL);

		DBG_871X_LEVEL(_drv_always_, "ap recv disassoc reason code(%d) sta:%pM\n",
				reason, GetAddr2Ptr(pframe));

		psta = rtw_get_stainfo(pstapriv, GetAddr2Ptr(pframe));
		if(psta)
		{
			u8 updated;

			_enter_critical_bh(&pstapriv->asoc_list_lock, &irqL);
			if(rtw_is_list_empty(&psta->asoc_list)==_FALSE)
			{
				rtw_list_delete(&psta->asoc_list);
				updated = ap_free_sta(padapter, psta, _FALSE);

			}
			_exit_critical_bh(&pstapriv->asoc_list_lock, &irqL);

			associated_clients_update(padapter, updated);
		}

		return _SUCCESS;
	}
	else
#endif
	{
		DBG_871X_LEVEL(_drv_always_, "ap recv disassoc reason code(%d) sta:%pM\n",
				reason, GetAddr3Ptr(pframe));

		receive_disconnect(padapter, GetAddr3Ptr(pframe), reason);
	}
	pmlmepriv->LinkDetectInfo.bBusyTraffic = _FALSE;
	return _SUCCESS;

}

unsigned int OnAtim(_adapter *padapter, union recv_frame *precv_frame)
{
	DBG_871X("%s\n", __FUNCTION__);
	return _SUCCESS;
}

unsigned int OnAction_qos(_adapter *padapter, union recv_frame *precv_frame)
{
	return _SUCCESS;
}

unsigned int OnAction_dls(_adapter *padapter, union recv_frame *precv_frame)
{
	return _SUCCESS;
}

unsigned int OnAction_back(_adapter *padapter, union recv_frame *precv_frame)
{
	u8 *addr;
	struct sta_info *psta=NULL;
	struct recv_reorder_ctrl *preorder_ctrl;
	unsigned char		*frame_body;
	unsigned char		category, action;
	unsigned short	tid, status, reason_code = 0;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u8 *pframe = precv_frame->u.hdr.rx_data;
	struct sta_priv *pstapriv = &padapter->stapriv;

	//check RA matches or not
	if (!_rtw_memcmp(myid(&(padapter->eeprompriv)), GetAddr1Ptr(pframe), ETH_ALEN))//for if1, sta/ap mode
		return _SUCCESS;

/*
	//check A1 matches or not
	if (!_rtw_memcmp(myid(&(padapter->eeprompriv)), get_da(pframe), ETH_ALEN))
		return _SUCCESS;
*/
	DBG_871X("%s\n", __FUNCTION__);

	if((pmlmeinfo->state&0x03) != WIFI_FW_AP_STATE)
		if (!(pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS))
			return _SUCCESS;

	addr = GetAddr2Ptr(pframe);
	psta = rtw_get_stainfo(pstapriv, addr);

	if(psta==NULL)
		return _SUCCESS;

	frame_body = (unsigned char *)(pframe + sizeof(struct rtw_ieee80211_hdr_3addr));

	category = frame_body[0];
	if (category == RTW_WLAN_CATEGORY_BACK)// representing Block Ack
	{
#ifdef CONFIG_TDLS
		if((psta->tdls_sta_state & TDLS_LINKED_STATE) &&
			(psta->htpriv.ht_option==_TRUE) &&
			(psta->htpriv.ampdu_enable==_TRUE) )
		{
			//do nothing; just don't want to return _SUCCESS;
		}
		else
#endif //CONFIG_TDLS
		if (!pmlmeinfo->HT_enable)
		{
			return _SUCCESS;
		}

		action = frame_body[1];
		DBG_871X("%s, action=%d\n", __FUNCTION__, action);
		switch (action)
		{
			case RTW_WLAN_ACTION_ADDBA_REQ: //ADDBA request

				_rtw_memcpy(&(pmlmeinfo->ADDBA_req), &(frame_body[2]), sizeof(struct ADDBA_request));
				//process_addba_req(padapter, (u8*)&(pmlmeinfo->ADDBA_req), GetAddr3Ptr(pframe));
				process_addba_req(padapter, (u8*)&(pmlmeinfo->ADDBA_req), addr);

				if(pmlmeinfo->bAcceptAddbaReq == _TRUE)
				{
					issue_action_BA(padapter, addr, RTW_WLAN_ACTION_ADDBA_RESP, 0);
				}
				else
				{
					issue_action_BA(padapter, addr, RTW_WLAN_ACTION_ADDBA_RESP, 37);//reject ADDBA Req
				}

				break;

			case RTW_WLAN_ACTION_ADDBA_RESP: //ADDBA response

				//status = frame_body[3] | (frame_body[4] << 8); //endian issue
				status = RTW_GET_LE16(&frame_body[3]);
				tid = ((frame_body[5] >> 2) & 0x7);

				if (status == 0)
				{	//successful
					DBG_871X("agg_enable for TID=%d\n", tid);
					psta->htpriv.agg_enable_bitmap |= 1 << tid;
					psta->htpriv.candidate_tid_bitmap &= ~BIT(tid);
				}
				else
				{
					psta->htpriv.agg_enable_bitmap &= ~BIT(tid);
				}

				//DBG_871X("marc: ADDBA RSP: %x\n", pmlmeinfo->agg_enable_bitmap);
				break;

			case RTW_WLAN_ACTION_DELBA: //DELBA
				if ((frame_body[3] & BIT(3)) == 0)
				{
					psta->htpriv.agg_enable_bitmap &= ~(1 << ((frame_body[3] >> 4) & 0xf));
					psta->htpriv.candidate_tid_bitmap &= ~(1 << ((frame_body[3] >> 4) & 0xf));

					//reason_code = frame_body[4] | (frame_body[5] << 8);
					reason_code = RTW_GET_LE16(&frame_body[4]);
				}
				else if((frame_body[3] & BIT(3)) == BIT(3))
				{
					tid = (frame_body[3] >> 4) & 0x0F;

					preorder_ctrl =  &psta->recvreorder_ctrl[tid];
					preorder_ctrl->enable = _FALSE;
					preorder_ctrl->indicate_seq = 0xffff;
					#ifdef DBG_RX_SEQ
					DBG_871X("DBG_RX_SEQ %s:%d indicate_seq:%u \n", __FUNCTION__, __LINE__,
						preorder_ctrl->indicate_seq);
					#endif
				}

				DBG_871X("%s(): DELBA: %x(%x)\n", __FUNCTION__,pmlmeinfo->agg_enable_bitmap, reason_code);
				//todo: how to notify the host while receiving DELETE BA
				break;

			default:
				break;
		}
	}

	return _SUCCESS;
}

#ifdef CONFIG_P2P

static void get_channel_cnt_24g_5gl_5gh(  struct mlme_ext_priv *pmlmeext, u8* p24g_cnt, u8* p5gl_cnt, u8* p5gh_cnt )
{
	int	i = 0;

	*p24g_cnt = 0;
	*p5gl_cnt = 0;
	*p5gh_cnt = 0;

	for( i = 0; i < pmlmeext->max_chan_nums; i++ )
	{
		if ( pmlmeext->channel_set[ i ].ChannelNum <= 14 )
		{
			(*p24g_cnt)++;
		}
		else if ( ( pmlmeext->channel_set[ i ].ChannelNum > 14 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 48 ) )
		{
			//	Just include the channel 36, 40, 44, 48 channels for 5G low
			(*p5gl_cnt)++;
		}
		else if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 149 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 161 ) )
		{
			//	Just include the channel 149, 153, 157, 161 channels for 5G high
			(*p5gh_cnt)++;
		}
	}
}

void issue_p2p_GO_request(_adapter *padapter, u8* raddr)
{

	unsigned char category = RTW_WLAN_CATEGORY_PUBLIC;
	u8			action = P2P_PUB_ACTION_ACTION;
	u32			p2poui = cpu_to_be32(P2POUI);
	u8			oui_subtype = P2P_GO_NEGO_REQ;
	u8			wpsie[ 255 ] = { 0x00 }, p2pie[ 255 ] = { 0x00 };
	u8			wpsielen = 0, p2pielen = 0, i;
	u8			channel_cnt_24g = 0, channel_cnt_5gl = 0, channel_cnt_5gh = 0;
	u16			len_channellist_attr = 0;
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD

	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct wifidirect_info	*pwdinfo = &( padapter->wdinfo);


	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	DBG_871X( "[%s] In\n", __FUNCTION__ );
	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, raddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, myid(&(padapter->eeprompriv)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 4, (unsigned char *) &(p2poui), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(oui_subtype), &(pattrib->pktlen));
	pwdinfo->negotiation_dialog_token = 1;	//	Initialize the dialog value
	pframe = rtw_set_fixed_ie(pframe, 1, &pwdinfo->negotiation_dialog_token, &(pattrib->pktlen));



	//	WPS Section
	wpsielen = 0;
	//	WPS OUI
	*(u32*) ( wpsie ) = cpu_to_be32( WPSOUI );
	wpsielen += 4;

	//	WPS version
	//	Type:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_VER1 );
	wpsielen += 2;

	//	Length:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0001 );
	wpsielen += 2;

	//	Value:
	wpsie[wpsielen++] = WPS_VERSION_1;	//	Version 1.0

	//	Device Password ID
	//	Type:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_DEVICE_PWID );
	wpsielen += 2;

	//	Length:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0002 );
	wpsielen += 2;

	//	Value:

	if ( pwdinfo->ui_got_wps_info == P2P_GOT_WPSINFO_PEER_DISPLAY_PIN )
	{
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_DPID_USER_SPEC );
	}
	else if ( pwdinfo->ui_got_wps_info == P2P_GOT_WPSINFO_SELF_DISPLAY_PIN )
	{
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_DPID_REGISTRAR_SPEC );
	}
	else if ( pwdinfo->ui_got_wps_info == P2P_GOT_WPSINFO_PBC )
	{
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_DPID_PBC );
	}

	wpsielen += 2;

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, wpsielen, (unsigned char *) wpsie, &pattrib->pktlen );


	//	P2P IE Section.

	//	P2P OUI
	p2pielen = 0;
	p2pie[ p2pielen++ ] = 0x50;
	p2pie[ p2pielen++ ] = 0x6F;
	p2pie[ p2pielen++ ] = 0x9A;
	p2pie[ p2pielen++ ] = 0x09;	//	WFA P2P v1.0

	//	Commented by Albert 20110306
	//	According to the P2P Specification, the group negoitation request frame should contain 9 P2P attributes
	//	1. P2P Capability
	//	2. Group Owner Intent
	//	3. Configuration Timeout
	//	4. Listen Channel
	//	5. Extended Listen Timing
	//	6. Intended P2P Interface Address
	//	7. Channel List
	//	8. P2P Device Info
	//	9. Operating Channel


	//	P2P Capability
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CAPABILITY;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0002 );
	p2pielen += 2;

	//	Value:
	//	Device Capability Bitmap, 1 byte
	p2pie[ p2pielen++ ] = DMP_P2P_DEVCAP_SUPPORT;

	//	Group Capability Bitmap, 1 byte
	if ( pwdinfo->persistent_supported )
	{
		p2pie[ p2pielen++ ] = P2P_GRPCAP_CROSS_CONN | P2P_GRPCAP_PERSISTENT_GROUP;
	}
	else
	{
		p2pie[ p2pielen++ ] = P2P_GRPCAP_CROSS_CONN;
	}


	//	Group Owner Intent
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_GO_INTENT;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0001 );
	p2pielen += 2;

	//	Value:
	//	Todo the tie breaker bit.
	p2pie[ p2pielen++ ] = ( ( pwdinfo->intent << 1 ) | BIT(0) );

	//	Configuration Timeout
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CONF_TIMEOUT;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0002 );
	p2pielen += 2;

	//	Value:
	p2pie[ p2pielen++ ] = 200;	//	2 seconds needed to be the P2P GO
	p2pie[ p2pielen++ ] = 200;	//	2 seconds needed to be the P2P Client


	//	Listen Channel
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_LISTEN_CH;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0005 );
	p2pielen += 2;

	//	Value:
	//	Country String
	p2pie[ p2pielen++ ] = 'U';
	p2pie[ p2pielen++ ] = 'S';

	//	The third byte should be set to 0x04.
	//	Described in the "Operating Channel Attribute" section.
	p2pie[ p2pielen++ ] = 0x04;

	//	Operating Class
	p2pie[ p2pielen++ ] = 0x51;	//	Copy from SD7

	//	Channel Number
	p2pie[ p2pielen++ ] = pwdinfo->listen_channel;	//	listening channel number


	//	Extended Listen Timing ATTR
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_EX_LISTEN_TIMING;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0004 );
	p2pielen += 2;

	//	Value:
	//	Availability Period
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0xFFFF );
	p2pielen += 2;

	//	Availability Interval
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0xFFFF );
	p2pielen += 2;


	//	Intended P2P Interface Address
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_INTENTED_IF_ADDR;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( ETH_ALEN );
	p2pielen += 2;

	//	Value:
	_rtw_memcpy( p2pie + p2pielen, myid( &padapter->eeprompriv ), ETH_ALEN );
	p2pielen += ETH_ALEN;


	//	Channel List
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CH_LIST;

	get_channel_cnt_24g_5gl_5gh( pmlmeext, &channel_cnt_24g, &channel_cnt_5gl, &channel_cnt_5gh );
	if ( ( channel_cnt_24g ) && ( channel_cnt_5gl ) && ( channel_cnt_5gh ))
	{
		//	This Wi-Fi supports both 2.4G and 5G channels.
		//	So, there will be three Operating Class.
		//	9-> 3 ( Country String ) + 3 x ( 1 ( Operating Class ) + 1 ( Number of Channels ) )
		len_channellist_attr = 9 + channel_cnt_24g + channel_cnt_5gl + channel_cnt_5gh;
	}
	else	 if ( ( channel_cnt_24g ) && ( ( channel_cnt_5gl ) || ( channel_cnt_5gh ) ) )
	{
		//	This Wi-Fi supports both 2.4G and 5G channels.
		//	So, there will be tw Operating Class.
		//	7-> 3 ( Country String ) + 2 x ( 1 ( Operating Class ) + 1 ( Number of Channels ) )
		if ( channel_cnt_5gl )
			len_channellist_attr = 7 + channel_cnt_24g + channel_cnt_5gl;
		else
			len_channellist_attr = 7 + channel_cnt_24g + channel_cnt_5gh;
	}
	else
	{
		//	5-> 3 ( Country String ) + 1 ( Operating Class ) + 1 ( Number of Channels )
		len_channellist_attr = 5 + channel_cnt_24g;
	}

	//	Length:
#ifdef CONFIG_CONCURRENT_MODE
	if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
	{
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 5 + 1 );
	}
	else
	{
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( len_channellist_attr );
	}
#else

	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( len_channellist_attr );

#endif
	p2pielen += 2;

	//	Value:
	//	Country String
	p2pie[ p2pielen++ ] = 'U';
	p2pie[ p2pielen++ ] = 'S';

	//	The third byte should be set to 0x04.
	//	Described in the "Operating Channel Attribute" section.
	p2pie[ p2pielen++ ] = 0x04;

	//	Channel Entry List

#ifdef CONFIG_CONCURRENT_MODE
	if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
	{
		_adapter *pbuddy_adapter = padapter->pbuddy_adapter;
		struct mlme_ext_priv	*pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;

		//	Operating Class
		if ( pbuddy_mlmeext->cur_channel > 14 )
		{
			if ( pbuddy_mlmeext->cur_channel >= 149 )
			{
				p2pie[ p2pielen++ ] = 0x7c;
			}
			else
			{
				p2pie[ p2pielen++ ] = 0x73;
			}
		}
		else
		{
			p2pie[ p2pielen++ ] = 0x51;
		}

		//	Number of Channels
		//	Just support 1 channel and this channel is AP's channel
		p2pie[ p2pielen++ ] = 1;

		//	Channel List
		p2pie[ p2pielen++ ] = pbuddy_mlmeext->cur_channel;
	}
	else
	{
		if ( channel_cnt_24g )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x51;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_24g;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( pmlmeext->channel_set[ i ].ChannelNum <= 14 )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}

		if ( channel_cnt_5gl )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x73;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_5gl;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 36 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 48 ) )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}

		if ( channel_cnt_5gh )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x7c;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_5gh;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 149 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 161 ) )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}
	}
#else // CONFIG_CONCURRENT_MODE

	if ( channel_cnt_24g )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x51;
		//	Number of Channels
		p2pie[ p2pielen++ ] = channel_cnt_24g;

		//	Channel List
		for( i = 0; i < pmlmeext->max_chan_nums; i++ )
		{
			if ( pmlmeext->channel_set[ i ].ChannelNum <= 14 )
			{
				p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
			}
		}
	}

	if ( channel_cnt_5gl )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x73;
		//	Number of Channels
		p2pie[ p2pielen++ ] = channel_cnt_5gl;

		//	Channel List
		for( i = 0; i < pmlmeext->max_chan_nums; i++ )
		{
			if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 36 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 48 ) )
			{
				p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
			}
		}
	}

	if ( channel_cnt_5gh )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x7c;
		//	Number of Channels
		p2pie[ p2pielen++ ] = channel_cnt_5gh;

		//	Channel List
		for( i = 0; i < pmlmeext->max_chan_nums; i++ )
		{
			if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 149 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 161 ) )
			{
				p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
			}
		}
	}
#endif // CONFIG_CONCURRENT_MODE

	//	Device Info
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_DEVICE_INFO;

	//	Length:
	//	21 -> P2P Device Address (6bytes) + Config Methods (2bytes) + Primary Device Type (8bytes)
	//	+ NumofSecondDevType (1byte) + WPS Device Name ID field (2bytes) + WPS Device Name Len field (2bytes)
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 21 + pwdinfo->device_name_len );
	p2pielen += 2;

	//	Value:
	//	P2P Device Address
	_rtw_memcpy( p2pie + p2pielen, myid( &padapter->eeprompriv ), ETH_ALEN );
	p2pielen += ETH_ALEN;

	//	Config Method
	//	This field should be big endian. Noted by P2P specification.

	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( pwdinfo->supported_wps_cm );

	p2pielen += 2;

	//	Primary Device Type
	//	Category ID
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_PDT_CID_MULIT_MEDIA );
	p2pielen += 2;

	//	OUI
	*(u32*) ( p2pie + p2pielen ) = cpu_to_be32( WPSOUI );
	p2pielen += 4;

	//	Sub Category ID
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_PDT_SCID_MEDIA_SERVER );
	p2pielen += 2;

	//	Number of Secondary Device Types
	p2pie[ p2pielen++ ] = 0x00;	//	No Secondary Device Type List

	//	Device Name
	//	Type:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_ATTR_DEVICE_NAME );
	p2pielen += 2;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( pwdinfo->device_name_len );
	p2pielen += 2;

	//	Value:
	_rtw_memcpy( p2pie + p2pielen, pwdinfo->device_name , pwdinfo->device_name_len );
	p2pielen += pwdinfo->device_name_len;


	//	Operating Channel
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_OPERATING_CH;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0005 );
	p2pielen += 2;

	//	Value:
	//	Country String
	p2pie[ p2pielen++ ] = 'U';
	p2pie[ p2pielen++ ] = 'S';

	//	The third byte should be set to 0x04.
	//	Described in the "Operating Channel Attribute" section.
	p2pie[ p2pielen++ ] = 0x04;

	//	Operating Class
	if ( pwdinfo->operating_channel <= 14 )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x51;
	}
	else if ( ( pwdinfo->operating_channel >= 36 ) && ( pwdinfo->operating_channel <= 48 ) )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x73;
	}
	else
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x7c;
	}

	//	Channel Number
	p2pie[ p2pielen++ ] = pwdinfo->operating_channel;	//	operating channel number

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, p2pielen, (unsigned char *) p2pie, &pattrib->pktlen );

#ifdef CONFIG_WFD
	wfdielen = build_nego_req_wfd_ie(pwdinfo, pframe);
	pframe += wfdielen;
	pattrib->pktlen += wfdielen;
#endif //CONFIG_WFD

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

	return;

}


void issue_p2p_GO_response(_adapter *padapter, u8* raddr, u8* frame_body,uint len, u8 result)
{

	unsigned char category = RTW_WLAN_CATEGORY_PUBLIC;
	u8			action = P2P_PUB_ACTION_ACTION;
	u32			p2poui = cpu_to_be32(P2POUI);
	u8			oui_subtype = P2P_GO_NEGO_RESP;
	u8			wpsie[ 255 ] = { 0x00 }, p2pie[ 255 ] = { 0x00 };
	u8			p2pielen = 0, i;
	uint			wpsielen = 0;
	u16			wps_devicepassword_id = 0x0000;
	uint			wps_devicepassword_id_len = 0;
	u8			channel_cnt_24g = 0, channel_cnt_5gl = 0, channel_cnt_5gh;
	u16			len_channellist_attr = 0;

	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct wifidirect_info	*pwdinfo = &( padapter->wdinfo);

#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	DBG_871X( "[%s] In\n", __FUNCTION__ );
	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, raddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, myid(&(padapter->eeprompriv)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 4, (unsigned char *) &(p2poui), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(oui_subtype), &(pattrib->pktlen));
	pwdinfo->negotiation_dialog_token = frame_body[7];	//	The Dialog Token of provisioning discovery request frame.
	pframe = rtw_set_fixed_ie(pframe, 1, &(pwdinfo->negotiation_dialog_token), &(pattrib->pktlen));

	//	Commented by Albert 20110328
	//	Try to get the device password ID from the WPS IE of group negotiation request frame
	//	WiFi Direct test plan 5.1.15
	rtw_get_wps_ie( frame_body + _PUBLIC_ACTION_IE_OFFSET_, len - _PUBLIC_ACTION_IE_OFFSET_, wpsie, &wpsielen);
	rtw_get_wps_attr_content( wpsie, wpsielen, WPS_ATTR_DEVICE_PWID, (u8*) &wps_devicepassword_id, &wps_devicepassword_id_len);
	wps_devicepassword_id = be16_to_cpu( wps_devicepassword_id );

	_rtw_memset( wpsie, 0x00, 255 );
	wpsielen = 0;

	//	WPS Section
	wpsielen = 0;
	//	WPS OUI
	*(u32*) ( wpsie ) = cpu_to_be32( WPSOUI );
	wpsielen += 4;

	//	WPS version
	//	Type:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_VER1 );
	wpsielen += 2;

	//	Length:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0001 );
	wpsielen += 2;

	//	Value:
	wpsie[wpsielen++] = WPS_VERSION_1;	//	Version 1.0

	//	Device Password ID
	//	Type:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_DEVICE_PWID );
	wpsielen += 2;

	//	Length:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0002 );
	wpsielen += 2;

	//	Value:
	if ( wps_devicepassword_id == WPS_DPID_USER_SPEC )
	{
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_DPID_REGISTRAR_SPEC );
	}
	else if ( wps_devicepassword_id == WPS_DPID_REGISTRAR_SPEC )
	{
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_DPID_USER_SPEC );
	}
	else
	{
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_DPID_PBC );
	}
	wpsielen += 2;

	//	Commented by Kurt 20120113
	//	If some device wants to do p2p handshake without sending prov_disc_req
	//	We have to get peer_req_cm from here.
	if(_rtw_memcmp( pwdinfo->rx_prov_disc_info.strconfig_method_desc_of_prov_disc_req, "000", 3) )
	{
		if ( wps_devicepassword_id == WPS_DPID_USER_SPEC )
		{
			_rtw_memcpy( pwdinfo->rx_prov_disc_info.strconfig_method_desc_of_prov_disc_req, "dis", 3 );
		}
		else if ( wps_devicepassword_id == WPS_DPID_REGISTRAR_SPEC )
		{
			_rtw_memcpy( pwdinfo->rx_prov_disc_info.strconfig_method_desc_of_prov_disc_req, "pad", 3 );
		}
		else
		{
			_rtw_memcpy( pwdinfo->rx_prov_disc_info.strconfig_method_desc_of_prov_disc_req, "pbc", 3 );
		}
	}

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, wpsielen, (unsigned char *) wpsie, &pattrib->pktlen );


	//	P2P IE Section.

	//	P2P OUI
	p2pielen = 0;
	p2pie[ p2pielen++ ] = 0x50;
	p2pie[ p2pielen++ ] = 0x6F;
	p2pie[ p2pielen++ ] = 0x9A;
	p2pie[ p2pielen++ ] = 0x09;	//	WFA P2P v1.0

	//	Commented by Albert 20100908
	//	According to the P2P Specification, the group negoitation response frame should contain 9 P2P attributes
	//	1. Status
	//	2. P2P Capability
	//	3. Group Owner Intent
	//	4. Configuration Timeout
	//	5. Operating Channel
	//	6. Intended P2P Interface Address
	//	7. Channel List
	//	8. Device Info
	//	9. Group ID	( Only GO )


	//	ToDo:

	//	P2P Status
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_STATUS;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0001 );
	p2pielen += 2;

	//	Value:
	p2pie[ p2pielen++ ] = result;

	//	P2P Capability
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CAPABILITY;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0002 );
	p2pielen += 2;

	//	Value:
	//	Device Capability Bitmap, 1 byte

	if ( rtw_p2p_chk_role(pwdinfo, P2P_ROLE_CLIENT) )
	{
		//	Commented by Albert 2011/03/08
		//	According to the P2P specification
		//	if the sending device will be client, the P2P Capability should be reserved of group negotation response frame
		p2pie[ p2pielen++ ] = 0;
	}
	else
	{
		//	Be group owner or meet the error case
		p2pie[ p2pielen++ ] = DMP_P2P_DEVCAP_SUPPORT;
	}

	//	Group Capability Bitmap, 1 byte
	if ( pwdinfo->persistent_supported )
	{
		p2pie[ p2pielen++ ] = P2P_GRPCAP_CROSS_CONN | P2P_GRPCAP_PERSISTENT_GROUP;
	}
	else
	{
		p2pie[ p2pielen++ ] = P2P_GRPCAP_CROSS_CONN;
	}

	//	Group Owner Intent
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_GO_INTENT;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0001 );
	p2pielen += 2;

	//	Value:
	if ( pwdinfo->peer_intent & 0x01 )
	{
		//	Peer's tie breaker bit is 1, our tie breaker bit should be 0
		p2pie[ p2pielen++ ] = ( pwdinfo->intent << 1 );
	}
	else
	{
		//	Peer's tie breaker bit is 0, our tie breaker bit should be 1
		p2pie[ p2pielen++ ] = ( ( pwdinfo->intent << 1 ) | BIT(0) );
	}


	//	Configuration Timeout
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CONF_TIMEOUT;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0002 );
	p2pielen += 2;

	//	Value:
	p2pie[ p2pielen++ ] = 200;	//	2 seconds needed to be the P2P GO
	p2pie[ p2pielen++ ] = 200;	//	2 seconds needed to be the P2P Client

	//	Operating Channel
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_OPERATING_CH;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0005 );
	p2pielen += 2;

	//	Value:
	//	Country String
	p2pie[ p2pielen++ ] = 'U';
	p2pie[ p2pielen++ ] = 'S';

	//	The third byte should be set to 0x04.
	//	Described in the "Operating Channel Attribute" section.
	p2pie[ p2pielen++ ] = 0x04;

	//	Operating Class
	if ( pwdinfo->operating_channel <= 14 )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x51;
	}
	else if ( ( pwdinfo->operating_channel >= 36 ) && ( pwdinfo->operating_channel <= 48 ) )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x73;
	}
	else
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x7c;
	}

	//	Channel Number
	p2pie[ p2pielen++ ] = pwdinfo->operating_channel;	//	operating channel number

	//	Intended P2P Interface Address
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_INTENTED_IF_ADDR;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( ETH_ALEN );
	p2pielen += 2;

	//	Value:
	_rtw_memcpy( p2pie + p2pielen, myid( &padapter->eeprompriv ), ETH_ALEN );
	p2pielen += ETH_ALEN;

	//	Channel List
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CH_LIST;

	get_channel_cnt_24g_5gl_5gh( pmlmeext, &channel_cnt_24g, &channel_cnt_5gl, &channel_cnt_5gh );
	if ( ( channel_cnt_24g ) && ( channel_cnt_5gl ) && ( channel_cnt_5gh ))
	{
		//	This Wi-Fi supports both 2.4G and 5G channels.
		//	So, there will be three Operating Class.
		//	9-> 3 ( Country String ) + 3 x ( 1 ( Operating Class ) + 1 ( Number of Channels ) )
		len_channellist_attr = 9 + channel_cnt_24g + channel_cnt_5gl + channel_cnt_5gh;
	}
	else	 if ( ( channel_cnt_24g ) && ( ( channel_cnt_5gl ) || ( channel_cnt_5gh ) ) )
	{
		//	This Wi-Fi supports both 2.4G and 5G channels.
		//	So, there will be tw Operating Class.
		//	7-> 3 ( Country String ) + 2 x ( 1 ( Operating Class ) + 1 ( Number of Channels ) )
		if ( channel_cnt_5gl )
			len_channellist_attr = 7 + channel_cnt_24g + channel_cnt_5gl;
		else
			len_channellist_attr = 7 + channel_cnt_24g + channel_cnt_5gh;
	}
	else
	{
		//	5-> 3 ( Country String ) + 1 ( Operating Class ) + 1 ( Number of Channels )
		len_channellist_attr = 5 + channel_cnt_24g;
	}

	//	Length:
#ifdef CONFIG_CONCURRENT_MODE
	if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
	{
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 5 + 1 );
	}
	else
	{
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( len_channellist_attr );
	}
#else

	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( len_channellist_attr );

 #endif
	p2pielen += 2;

	//	Value:
	//	Country String
	p2pie[ p2pielen++ ] = 'U';
	p2pie[ p2pielen++ ] = 'S';

	//	The third byte should be set to 0x04.
	//	Described in the "Operating Channel Attribute" section.
	p2pie[ p2pielen++ ] = 0x04;

	//	Channel Entry List

#ifdef CONFIG_CONCURRENT_MODE
	if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
	{
		_adapter *pbuddy_adapter = padapter->pbuddy_adapter;
		struct mlme_ext_priv	*pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;

		//	Operating Class
		if ( pbuddy_mlmeext->cur_channel > 14 )
		{
			if ( pbuddy_mlmeext->cur_channel >= 149 )
			{
				p2pie[ p2pielen++ ] = 0x7c;
			}
			else
			{
				p2pie[ p2pielen++ ] = 0x73;
			}
		}
		else
		{
			p2pie[ p2pielen++ ] = 0x51;
		}

		//	Number of Channels
		//	Just support 1 channel and this channel is AP's channel
		p2pie[ p2pielen++ ] = 1;

		//	Channel List
		p2pie[ p2pielen++ ] = pbuddy_mlmeext->cur_channel;
	}
	else
	{
		if ( channel_cnt_24g )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x51;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_24g;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( pmlmeext->channel_set[ i ].ChannelNum <= 14 )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}

		if ( channel_cnt_5gl )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x73;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_5gl;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 36 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 48 ) )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}

		if ( channel_cnt_5gh )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x7c;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_5gh;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 149 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 161 ) )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}
	}
#else // CONFIG_CONCURRENT_MODE

	if ( channel_cnt_24g )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x51;
		//	Number of Channels
		p2pie[ p2pielen++ ] = channel_cnt_24g;

		//	Channel List
		for( i = 0; i < pmlmeext->max_chan_nums; i++ )
		{
			if ( pmlmeext->channel_set[ i ].ChannelNum <= 14 )
			{
				p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
			}
		}
	}

	if ( channel_cnt_5gl )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x73;
		//	Number of Channels
		p2pie[ p2pielen++ ] = channel_cnt_5gl;

		//	Channel List
		for( i = 0; i < pmlmeext->max_chan_nums; i++ )
		{
			if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 36 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 48 ) )
			{
				p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
			}
		}
	}

	if ( channel_cnt_5gh )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x7c;
		//	Number of Channels
		p2pie[ p2pielen++ ] = channel_cnt_5gh;

		//	Channel List
		for( i = 0; i < pmlmeext->max_chan_nums; i++ )
		{
			if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 149 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 161 ) )
			{
				p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
			}
		}
	}
#endif // CONFIG_CONCURRENT_MODE


	//	Device Info
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_DEVICE_INFO;

	//	Length:
	//	21 -> P2P Device Address (6bytes) + Config Methods (2bytes) + Primary Device Type (8bytes)
	//	+ NumofSecondDevType (1byte) + WPS Device Name ID field (2bytes) + WPS Device Name Len field (2bytes)
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 21 + pwdinfo->device_name_len );
	p2pielen += 2;

	//	Value:
	//	P2P Device Address
	_rtw_memcpy( p2pie + p2pielen, myid( &padapter->eeprompriv ), ETH_ALEN );
	p2pielen += ETH_ALEN;

	//	Config Method
	//	This field should be big endian. Noted by P2P specification.

	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( pwdinfo->supported_wps_cm );

	p2pielen += 2;

	//	Primary Device Type
	//	Category ID
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_PDT_CID_MULIT_MEDIA );
	p2pielen += 2;

	//	OUI
	*(u32*) ( p2pie + p2pielen ) = cpu_to_be32( WPSOUI );
	p2pielen += 4;

	//	Sub Category ID
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_PDT_SCID_MEDIA_SERVER );
	p2pielen += 2;

	//	Number of Secondary Device Types
	p2pie[ p2pielen++ ] = 0x00;	//	No Secondary Device Type List

	//	Device Name
	//	Type:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_ATTR_DEVICE_NAME );
	p2pielen += 2;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( pwdinfo->device_name_len );
	p2pielen += 2;

	//	Value:
	_rtw_memcpy( p2pie + p2pielen, pwdinfo->device_name , pwdinfo->device_name_len );
	p2pielen += pwdinfo->device_name_len;

	if ( rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO) )
	{
		//	Group ID Attribute
		//	Type:
		p2pie[ p2pielen++ ] = P2P_ATTR_GROUP_ID;

		//	Length:
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( ETH_ALEN + pwdinfo->nego_ssidlen );
		p2pielen += 2;

		//	Value:
		//	p2P Device Address
		_rtw_memcpy( p2pie + p2pielen , pwdinfo->device_addr, ETH_ALEN );
		p2pielen += ETH_ALEN;

		//	SSID
		_rtw_memcpy( p2pie + p2pielen, pwdinfo->nego_ssid, pwdinfo->nego_ssidlen );
		p2pielen += pwdinfo->nego_ssidlen;

	}

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, p2pielen, (unsigned char *) p2pie, &pattrib->pktlen );

#ifdef CONFIG_WFD
	wfdielen = build_nego_resp_wfd_ie(pwdinfo, pframe);
	pframe += wfdielen;
	pattrib->pktlen += wfdielen;
#endif //CONFIG_WFD

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

	return;

}

void issue_p2p_GO_confirm(_adapter *padapter, u8* raddr, u8 result)
{

	unsigned char category = RTW_WLAN_CATEGORY_PUBLIC;
	u8			action = P2P_PUB_ACTION_ACTION;
	u32			p2poui = cpu_to_be32(P2POUI);
	u8			oui_subtype = P2P_GO_NEGO_CONF;
	u8			wpsie[ 255 ] = { 0x00 }, p2pie[ 255 ] = { 0x00 };
	u8			wpsielen = 0, p2pielen = 0;

	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct wifidirect_info	*pwdinfo = &( padapter->wdinfo);
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	DBG_871X( "[%s] In\n", __FUNCTION__ );
	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, raddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, myid(&(padapter->eeprompriv)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 4, (unsigned char *) &(p2poui), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(oui_subtype), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(pwdinfo->negotiation_dialog_token), &(pattrib->pktlen));



	//	P2P IE Section.

	//	P2P OUI
	p2pielen = 0;
	p2pie[ p2pielen++ ] = 0x50;
	p2pie[ p2pielen++ ] = 0x6F;
	p2pie[ p2pielen++ ] = 0x9A;
	p2pie[ p2pielen++ ] = 0x09;	//	WFA P2P v1.0

	//	Commented by Albert 20110306
	//	According to the P2P Specification, the group negoitation request frame should contain 5 P2P attributes
	//	1. Status
	//	2. P2P Capability
	//	3. Operating Channel
	//	4. Channel List
	//	5. Group ID	( if this WiFi is GO )

	//	P2P Status
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_STATUS;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0001 );
	p2pielen += 2;

	//	Value:
	p2pie[ p2pielen++ ] = result;

	//	P2P Capability
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CAPABILITY;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0002 );
	p2pielen += 2;

	//	Value:
	//	Device Capability Bitmap, 1 byte
	p2pie[ p2pielen++ ] = DMP_P2P_DEVCAP_SUPPORT;

	//	Group Capability Bitmap, 1 byte
	if ( pwdinfo->persistent_supported )
	{
		p2pie[ p2pielen++ ] = P2P_GRPCAP_CROSS_CONN | P2P_GRPCAP_PERSISTENT_GROUP;
	}
	else
	{
		p2pie[ p2pielen++ ] = P2P_GRPCAP_CROSS_CONN;
	}


	//	Operating Channel
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_OPERATING_CH;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0005 );
	p2pielen += 2;

	//	Value:
	//	Country String
	p2pie[ p2pielen++ ] = 'U';
	p2pie[ p2pielen++ ] = 'S';

	//	The third byte should be set to 0x04.
	//	Described in the "Operating Channel Attribute" section.
	p2pie[ p2pielen++ ] = 0x04;


	if ( rtw_p2p_chk_role(pwdinfo, P2P_ROLE_CLIENT) )
	{
		if ( pwdinfo->peer_operating_ch <= 14 )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x51;
		}
		else if ( ( pwdinfo->peer_operating_ch >= 36 ) && ( pwdinfo->peer_operating_ch <= 48 ) )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x73;
		}
		else
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x7c;
		}

		p2pie[ p2pielen++ ] = pwdinfo->peer_operating_ch;
	}
	else
	{
		if ( pwdinfo->operating_channel <= 14 )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x51;
		}
		else if ( ( pwdinfo->operating_channel >= 36 ) && ( pwdinfo->operating_channel <= 48 ) )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x73;
		}
		else
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x7c;
		}

		//	Channel Number
		p2pie[ p2pielen++ ] = pwdinfo->operating_channel;		//	Use the listen channel as the operating channel
	}


	//	Channel List
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CH_LIST;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( pwdinfo->channel_list_attr_len );
	p2pielen += 2;

	//	Value:
	_rtw_memcpy( p2pie + p2pielen, pwdinfo->channel_list_attr, pwdinfo->channel_list_attr_len );
	p2pielen += pwdinfo->channel_list_attr_len;

	if (rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO) )
	{
		//	Group ID Attribute
		//	Type:
		p2pie[ p2pielen++ ] = P2P_ATTR_GROUP_ID;

		//	Length:
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( ETH_ALEN + pwdinfo->nego_ssidlen );
		p2pielen += 2;

		//	Value:
		//	p2P Device Address
		_rtw_memcpy( p2pie + p2pielen , pwdinfo->device_addr, ETH_ALEN );
		p2pielen += ETH_ALEN;

		//	SSID
		_rtw_memcpy( p2pie + p2pielen, pwdinfo->nego_ssid, pwdinfo->nego_ssidlen );
		p2pielen += pwdinfo->nego_ssidlen;
	}

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, p2pielen, (unsigned char *) p2pie, &pattrib->pktlen );

#ifdef CONFIG_WFD
	wfdielen = build_nego_confirm_wfd_ie(pwdinfo, pframe);
	pframe += wfdielen;
	pattrib->pktlen += wfdielen;
#endif //CONFIG_WFD

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

	return;

}

void issue_p2p_invitation_request(_adapter *padapter, u8* raddr )
{

	unsigned char category = RTW_WLAN_CATEGORY_PUBLIC;
	u8			action = P2P_PUB_ACTION_ACTION;
	u32			p2poui = cpu_to_be32(P2POUI);
	u8			oui_subtype = P2P_INVIT_REQ;
	u8			p2pie[ 255 ] = { 0x00 };
	u8			p2pielen = 0, i;
	u8			dialogToken = 3;
	u8			channel_cnt_24g = 0, channel_cnt_5gl = 0, channel_cnt_5gh = 0;
	u16			len_channellist_attr = 0;
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD
#ifdef CONFIG_CONCURRENT_MODE
	_adapter				*pbuddy_adapter = padapter->pbuddy_adapter;
	struct wifidirect_info	*pbuddy_wdinfo = &pbuddy_adapter->wdinfo;
	struct mlme_priv		*pbuddy_mlmepriv = &pbuddy_adapter->mlmepriv;
	struct mlme_ext_priv	*pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;
#endif

	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct wifidirect_info	*pwdinfo = &( padapter->wdinfo);


	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, raddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, raddr,  ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 4, (unsigned char *) &(p2poui), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(oui_subtype), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(dialogToken), &(pattrib->pktlen));

	//	P2P IE Section.

	//	P2P OUI
	p2pielen = 0;
	p2pie[ p2pielen++ ] = 0x50;
	p2pie[ p2pielen++ ] = 0x6F;
	p2pie[ p2pielen++ ] = 0x9A;
	p2pie[ p2pielen++ ] = 0x09;	//	WFA P2P v1.0

	//	Commented by Albert 20101011
	//	According to the P2P Specification, the P2P Invitation request frame should contain 7 P2P attributes
	//	1. Configuration Timeout
	//	2. Invitation Flags
	//	3. Operating Channel	( Only GO )
	//	4. P2P Group BSSID	( Should be included if I am the GO )
	//	5. Channel List
	//	6. P2P Group ID
	//	7. P2P Device Info

	//	Configuration Timeout
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CONF_TIMEOUT;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0002 );
	p2pielen += 2;

	//	Value:
	p2pie[ p2pielen++ ] = 200;	//	2 seconds needed to be the P2P GO
	p2pie[ p2pielen++ ] = 200;	//	2 seconds needed to be the P2P Client

	//	Invitation Flags
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_INVITATION_FLAGS;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0001 );
	p2pielen += 2;

	//	Value:
	p2pie[ p2pielen++ ] = P2P_INVITATION_FLAGS_PERSISTENT;


	//	Operating Channel
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_OPERATING_CH;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0005 );
	p2pielen += 2;

	//	Value:
	//	Country String
	p2pie[ p2pielen++ ] = 'U';
	p2pie[ p2pielen++ ] = 'S';

	//	The third byte should be set to 0x04.
	//	Described in the "Operating Channel Attribute" section.
	p2pie[ p2pielen++ ] = 0x04;

	//	Operating Class
	if ( pwdinfo->invitereq_info.operating_ch <= 14 )
		p2pie[ p2pielen++ ] = 0x51;
	else if ( ( pwdinfo->invitereq_info.operating_ch >= 36 ) && ( pwdinfo->invitereq_info.operating_ch <= 48 ) )
		p2pie[ p2pielen++ ] = 0x73;
	else
		p2pie[ p2pielen++ ] = 0x7c;

	//	Channel Number
	p2pie[ p2pielen++ ] = pwdinfo->invitereq_info.operating_ch;	//	operating channel number

	if ( _rtw_memcmp( myid( &padapter->eeprompriv ), pwdinfo->invitereq_info.go_bssid, ETH_ALEN ) )
	{
		//	P2P Group BSSID
		//	Type:
		p2pie[ p2pielen++ ] = P2P_ATTR_GROUP_BSSID;

		//	Length:
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( ETH_ALEN );
		p2pielen += 2;

		//	Value:
		//	P2P Device Address for GO
		_rtw_memcpy( p2pie + p2pielen, pwdinfo->invitereq_info.go_bssid, ETH_ALEN );
		p2pielen += ETH_ALEN;
	}

	//	Channel List
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CH_LIST;

	get_channel_cnt_24g_5gl_5gh( pmlmeext, &channel_cnt_24g, &channel_cnt_5gl, &channel_cnt_5gh );
	if ( ( channel_cnt_24g ) && ( channel_cnt_5gl ) && ( channel_cnt_5gh ))
	{
		//	This Wi-Fi supports both 2.4G and 5G channels.
		//	So, there will be three Operating Class.
		//	9-> 3 ( Country String ) + 3 x ( 1 ( Operating Class ) + 1 ( Number of Channels ) )
		len_channellist_attr = 9 + channel_cnt_24g + channel_cnt_5gl + channel_cnt_5gh;
	}
	else	 if ( ( channel_cnt_24g ) && ( ( channel_cnt_5gl ) || ( channel_cnt_5gh ) ) )
	{
		//	This Wi-Fi supports both 2.4G and 5G channels.
		//	So, there will be tw Operating Class.
		//	7-> 3 ( Country String ) + 2 x ( 1 ( Operating Class ) + 1 ( Number of Channels ) )
		if ( channel_cnt_5gl )
			len_channellist_attr = 7 + channel_cnt_24g + channel_cnt_5gl;
		else
			len_channellist_attr = 7 + channel_cnt_24g + channel_cnt_5gh;
	}
	else
	{
		//	5-> 3 ( Country String ) + 1 ( Operating Class ) + 1 ( Number of Channels )
		len_channellist_attr = 5 + channel_cnt_24g;
	}

	//	Length:
#ifdef CONFIG_CONCURRENT_MODE
	if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
	{
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 5 + 1 );
	}
	else
	{
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( len_channellist_attr );
	}
#else

	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( len_channellist_attr );

 #endif
	p2pielen += 2;

	//	Value:
	//	Country String
	p2pie[ p2pielen++ ] = 'U';
	p2pie[ p2pielen++ ] = 'S';

	//	The third byte should be set to 0x04.
	//	Described in the "Operating Channel Attribute" section.
	p2pie[ p2pielen++ ] = 0x04;

	//	Channel Entry List
#ifdef CONFIG_CONCURRENT_MODE
	if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
	{
		_adapter *pbuddy_adapter = padapter->pbuddy_adapter;
		struct mlme_ext_priv	*pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;

		//	Operating Class
		if ( pbuddy_mlmeext->cur_channel > 14 )
		{
			if ( pbuddy_mlmeext->cur_channel >= 149 )
			{
				p2pie[ p2pielen++ ] = 0x7c;
			}
			else
			{
				p2pie[ p2pielen++ ] = 0x73;
			}
		}
		else
		{
			p2pie[ p2pielen++ ] = 0x51;
		}

		//	Number of Channels
		//	Just support 1 channel and this channel is AP's channel
		p2pie[ p2pielen++ ] = 1;

		//	Channel List
		p2pie[ p2pielen++ ] = pbuddy_mlmeext->cur_channel;
	}
	else
	{
		if ( channel_cnt_24g )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x51;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_24g;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( pmlmeext->channel_set[ i ].ChannelNum <= 14 )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}

		if ( channel_cnt_5gl )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x73;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_5gl;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 36 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 48 ) )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}

		if ( channel_cnt_5gh )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x7c;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_5gh;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 149 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 161 ) )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}
	}
#else // CONFIG_CONCURRENT_MODE

	if ( channel_cnt_24g )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x51;
		//	Number of Channels
		p2pie[ p2pielen++ ] = channel_cnt_24g;

		//	Channel List
		for( i = 0; i < pmlmeext->max_chan_nums; i++ )
		{
			if ( pmlmeext->channel_set[ i ].ChannelNum <= 14 )
			{
				p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
			}
		}
	}

	if ( channel_cnt_5gl )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x73;
		//	Number of Channels
		p2pie[ p2pielen++ ] = channel_cnt_5gl;

		//	Channel List
		for( i = 0; i < pmlmeext->max_chan_nums; i++ )
		{
			if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 36 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 48 ) )
			{
				p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
			}
		}
	}

	if ( channel_cnt_5gh )
	{
		//	Operating Class
		p2pie[ p2pielen++ ] = 0x7c;
		//	Number of Channels
		p2pie[ p2pielen++ ] = channel_cnt_5gh;

		//	Channel List
		for( i = 0; i < pmlmeext->max_chan_nums; i++ )
		{
			if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 149 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 161 ) )
			{
				p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
			}
		}
	}
#endif // CONFIG_CONCURRENT_MODE


	//	P2P Group ID
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_GROUP_ID;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 6 + pwdinfo->invitereq_info.ssidlen );
	p2pielen += 2;

	//	Value:
	//	P2P Device Address for GO
	_rtw_memcpy( p2pie + p2pielen, pwdinfo->invitereq_info.go_bssid, ETH_ALEN );
	p2pielen += ETH_ALEN;

	//	SSID
	_rtw_memcpy( p2pie + p2pielen, pwdinfo->invitereq_info.go_ssid, pwdinfo->invitereq_info.ssidlen );
	p2pielen += pwdinfo->invitereq_info.ssidlen;


	//	Device Info
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_DEVICE_INFO;

	//	Length:
	//	21 -> P2P Device Address (6bytes) + Config Methods (2bytes) + Primary Device Type (8bytes)
	//	+ NumofSecondDevType (1byte) + WPS Device Name ID field (2bytes) + WPS Device Name Len field (2bytes)
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 21 + pwdinfo->device_name_len );
	p2pielen += 2;

	//	Value:
	//	P2P Device Address
	_rtw_memcpy( p2pie + p2pielen, myid( &padapter->eeprompriv ), ETH_ALEN );
	p2pielen += ETH_ALEN;

	//	Config Method
	//	This field should be big endian. Noted by P2P specification.
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_CONFIG_METHOD_DISPLAY );
	p2pielen += 2;

	//	Primary Device Type
	//	Category ID
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_PDT_CID_MULIT_MEDIA );
	p2pielen += 2;

	//	OUI
	*(u32*) ( p2pie + p2pielen ) = cpu_to_be32( WPSOUI );
	p2pielen += 4;

	//	Sub Category ID
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_PDT_SCID_MEDIA_SERVER );
	p2pielen += 2;

	//	Number of Secondary Device Types
	p2pie[ p2pielen++ ] = 0x00;	//	No Secondary Device Type List

	//	Device Name
	//	Type:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_ATTR_DEVICE_NAME );
	p2pielen += 2;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( pwdinfo->device_name_len );
	p2pielen += 2;

	//	Value:
	_rtw_memcpy( p2pie + p2pielen, pwdinfo->device_name, pwdinfo->device_name_len );
	p2pielen += pwdinfo->device_name_len;

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, p2pielen, (unsigned char *) p2pie, &pattrib->pktlen );

#ifdef CONFIG_WFD
	wfdielen = build_invitation_req_wfd_ie(pwdinfo, pframe);
	pframe += wfdielen;
	pattrib->pktlen += wfdielen;
#endif //CONFIG_WFD

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

	return;

}

void issue_p2p_invitation_response(_adapter *padapter, u8* raddr, u8 dialogToken, u8 status_code)
{

	unsigned char category = RTW_WLAN_CATEGORY_PUBLIC;
	u8			action = P2P_PUB_ACTION_ACTION;
	u32			p2poui = cpu_to_be32(P2POUI);
	u8			oui_subtype = P2P_INVIT_RESP;
	u8			p2pie[ 255 ] = { 0x00 };
	u8			p2pielen = 0, i;
	u8			channel_cnt_24g = 0, channel_cnt_5gl = 0, channel_cnt_5gh = 0;
	u16			len_channellist_attr = 0;
#ifdef CONFIG_CONCURRENT_MODE
	_adapter				*pbuddy_adapter = padapter->pbuddy_adapter;
	struct wifidirect_info	*pbuddy_wdinfo = &pbuddy_adapter->wdinfo;
	struct mlme_priv		*pbuddy_mlmepriv = &pbuddy_adapter->mlmepriv;
	struct mlme_ext_priv	*pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;
#endif
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD

	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct wifidirect_info	*pwdinfo = &( padapter->wdinfo);


	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, raddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, raddr,  ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 4, (unsigned char *) &(p2poui), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(oui_subtype), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(dialogToken), &(pattrib->pktlen));

	//	P2P IE Section.

	//	P2P OUI
	p2pielen = 0;
	p2pie[ p2pielen++ ] = 0x50;
	p2pie[ p2pielen++ ] = 0x6F;
	p2pie[ p2pielen++ ] = 0x9A;
	p2pie[ p2pielen++ ] = 0x09;	//	WFA P2P v1.0

	//	Commented by Albert 20101005
	//	According to the P2P Specification, the P2P Invitation response frame should contain 5 P2P attributes
	//	1. Status
	//	2. Configuration Timeout
	//	3. Operating Channel	( Only GO )
	//	4. P2P Group BSSID	( Only GO )
	//	5. Channel List

	//	P2P Status
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_STATUS;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0001 );
	p2pielen += 2;

	//	Value:
	//	When status code is P2P_STATUS_FAIL_INFO_UNAVAILABLE.
	//	Sent the event receiving the P2P Invitation Req frame to DMP UI.
	//	DMP had to compare the MAC address to find out the profile.
	//	So, the WiFi driver will send the P2P_STATUS_FAIL_INFO_UNAVAILABLE to NB.
	//	If the UI found the corresponding profile, the WiFi driver sends the P2P Invitation Req
	//	to NB to rebuild the persistent group.
	p2pie[ p2pielen++ ] = status_code;

	//	Configuration Timeout
	//	Type:
	p2pie[ p2pielen++ ] = P2P_ATTR_CONF_TIMEOUT;

	//	Length:
	*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0002 );
	p2pielen += 2;

	//	Value:
	p2pie[ p2pielen++ ] = 200;	//	2 seconds needed to be the P2P GO
	p2pie[ p2pielen++ ] = 200;	//	2 seconds needed to be the P2P Client

	if( status_code == P2P_STATUS_SUCCESS )
	{
		if( rtw_p2p_chk_role( pwdinfo, P2P_ROLE_GO ) )
		{
			//	The P2P Invitation request frame asks this Wi-Fi device to be the P2P GO
			//	In this case, the P2P Invitation response frame should carry the two more P2P attributes.
			//	First one is operating channel attribute.
			//	Second one is P2P Group BSSID attribute.

			//	Operating Channel
			//	Type:
			p2pie[ p2pielen++ ] = P2P_ATTR_OPERATING_CH;

			//	Length:
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0005 );
			p2pielen += 2;

			//	Value:
			//	Country String
			p2pie[ p2pielen++ ] = 'U';
			p2pie[ p2pielen++ ] = 'S';

			//	The third byte should be set to 0x04.
			//	Described in the "Operating Channel Attribute" section.
			p2pie[ p2pielen++ ] = 0x04;

			//	Operating Class
			p2pie[ p2pielen++ ] = 0x51;	//	Copy from SD7

			//	Channel Number
			p2pie[ p2pielen++ ] = pwdinfo->operating_channel;	//	operating channel number


			//	P2P Group BSSID
			//	Type:
			p2pie[ p2pielen++ ] = P2P_ATTR_GROUP_BSSID;

			//	Length:
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( ETH_ALEN );
			p2pielen += 2;

			//	Value:
			//	P2P Device Address for GO
			_rtw_memcpy( p2pie + p2pielen, myid( &padapter->eeprompriv ), ETH_ALEN );
			p2pielen += ETH_ALEN;

		}

		//	Channel List
		//	Type:
		p2pie[ p2pielen++ ] = P2P_ATTR_CH_LIST;

		get_channel_cnt_24g_5gl_5gh( pmlmeext, &channel_cnt_24g, &channel_cnt_5gl, &channel_cnt_5gh );
		if ( ( channel_cnt_24g ) && ( channel_cnt_5gl ) && ( channel_cnt_5gh ))
		{
			//	This Wi-Fi supports both 2.4G and 5G channels.
			//	So, there will be three Operating Class.
			//	9-> 3 ( Country String ) + 3 x ( 1 ( Operating Class ) + 1 ( Number of Channels ) )
			len_channellist_attr = 9 + channel_cnt_24g + channel_cnt_5gl + channel_cnt_5gh;
		}
		else	 if ( ( channel_cnt_24g ) && ( ( channel_cnt_5gl ) || ( channel_cnt_5gh ) ) )
		{
			//	This Wi-Fi supports both 2.4G and 5G channels.
			//	So, there will be tw Operating Class.
			//	7-> 3 ( Country String ) + 2 x ( 1 ( Operating Class ) + 1 ( Number of Channels ) )
			if ( channel_cnt_5gl )
				len_channellist_attr = 7 + channel_cnt_24g + channel_cnt_5gl;
			else
				len_channellist_attr = 7 + channel_cnt_24g + channel_cnt_5gh;
		}
		else
		{
			//	5-> 3 ( Country String ) + 1 ( Operating Class ) + 1 ( Number of Channels )
			len_channellist_attr = 5 + channel_cnt_24g;
		}

		//	Length:
#ifdef CONFIG_CONCURRENT_MODE
		if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
		{
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 5 + 1 );
		}
		else
		{
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( len_channellist_attr );
		}
#else

		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( len_channellist_attr );

#endif
		p2pielen += 2;

		//	Value:
		//	Country String
		p2pie[ p2pielen++ ] = 'U';
		p2pie[ p2pielen++ ] = 'S';

		//	The third byte should be set to 0x04.
		//	Described in the "Operating Channel Attribute" section.
		p2pie[ p2pielen++ ] = 0x04;

		//	Channel Entry List
#ifdef CONFIG_CONCURRENT_MODE
		if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
		{
			_adapter *pbuddy_adapter = padapter->pbuddy_adapter;
			struct mlme_ext_priv	*pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;

			//	Operating Class
			if ( pbuddy_mlmeext->cur_channel > 14 )
			{
				if ( pbuddy_mlmeext->cur_channel >= 149 )
				{
					p2pie[ p2pielen++ ] = 0x7c;
				}
				else
				{
					p2pie[ p2pielen++ ] = 0x73;
				}
			}
			else
			{
				p2pie[ p2pielen++ ] = 0x51;
			}

			//	Number of Channels
			//	Just support 1 channel and this channel is AP's channel
			p2pie[ p2pielen++ ] = 1;

			//	Channel List
			p2pie[ p2pielen++ ] = pbuddy_mlmeext->cur_channel;
		}
		else
		{
			if ( channel_cnt_24g )
			{
				//	Operating Class
				p2pie[ p2pielen++ ] = 0x51;
				//	Number of Channels
				p2pie[ p2pielen++ ] = channel_cnt_24g;

				//	Channel List
				for( i = 0; i < pmlmeext->max_chan_nums; i++ )
				{
					if ( pmlmeext->channel_set[ i ].ChannelNum <= 14 )
					{
						p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
					}
				}
			}

			if ( channel_cnt_5gl )
			{
				//	Operating Class
				p2pie[ p2pielen++ ] = 0x73;
				//	Number of Channels
				p2pie[ p2pielen++ ] = channel_cnt_5gl;

				//	Channel List
				for( i = 0; i < pmlmeext->max_chan_nums; i++ )
				{
					if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 36 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 48 ) )
					{
						p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
					}
				}
			}

			if ( channel_cnt_5gh )
			{
				//	Operating Class
				p2pie[ p2pielen++ ] = 0x7c;
				//	Number of Channels
				p2pie[ p2pielen++ ] = channel_cnt_5gh;

				//	Channel List
				for( i = 0; i < pmlmeext->max_chan_nums; i++ )
				{
					if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 149 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 161 ) )
					{
						p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
					}
				}
			}
		}
#else // CONFIG_CONCURRENT_MODE

		if ( channel_cnt_24g )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x51;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_24g;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( pmlmeext->channel_set[ i ].ChannelNum <= 14 )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}

		if ( channel_cnt_5gl )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x73;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_5gl;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 36 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 48 ) )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}

		if ( channel_cnt_5gh )
		{
			//	Operating Class
			p2pie[ p2pielen++ ] = 0x7c;
			//	Number of Channels
			p2pie[ p2pielen++ ] = channel_cnt_5gh;

			//	Channel List
			for( i = 0; i < pmlmeext->max_chan_nums; i++ )
			{
				if ( ( pmlmeext->channel_set[ i ].ChannelNum >= 149 ) && ( pmlmeext->channel_set[ i ].ChannelNum <= 161 ) )
				{
					p2pie[ p2pielen++ ] = pmlmeext->channel_set[ i ].ChannelNum;
				}
			}
		}
#endif // CONFIG_CONCURRENT_MODE
	}

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, p2pielen, (unsigned char *) p2pie, &pattrib->pktlen );

#ifdef CONFIG_WFD
	wfdielen = build_invitation_resp_wfd_ie(pwdinfo, pframe);
	pframe += wfdielen;
	pattrib->pktlen += wfdielen;
#endif //CONFIG_WFD

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

	return;

}

void issue_p2p_provision_request(_adapter *padapter, u8* pssid, u8 ussidlen, u8* pdev_raddr )
{
	unsigned char category = RTW_WLAN_CATEGORY_PUBLIC;
	u8			action = P2P_PUB_ACTION_ACTION;
	u8			dialogToken = 1;
	u32			p2poui = cpu_to_be32(P2POUI);
	u8			oui_subtype = P2P_PROVISION_DISC_REQ;
	u8			wpsie[ 100 ] = { 0x00 };
	u8			wpsielen = 0;
	u32			p2pielen = 0;
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD

	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);


	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	DBG_871X( "[%s] In\n", __FUNCTION__ );
	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, pdev_raddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, pdev_raddr, ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 4, (unsigned char *) &(p2poui), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(oui_subtype), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(dialogToken), &(pattrib->pktlen));

	p2pielen = build_prov_disc_request_p2p_ie( pwdinfo, pframe, pssid, ussidlen, pdev_raddr );

	pframe += p2pielen;
	pattrib->pktlen += p2pielen;

	wpsielen = 0;
	//	WPS OUI
	*(u32*) ( wpsie ) = cpu_to_be32( WPSOUI );
	wpsielen += 4;

	//	WPS version
	//	Type:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_VER1 );
	wpsielen += 2;

	//	Length:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0001 );
	wpsielen += 2;

	//	Value:
	wpsie[wpsielen++] = WPS_VERSION_1;	//	Version 1.0

	//	Config Method
	//	Type:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_CONF_METHOD );
	wpsielen += 2;

	//	Length:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0002 );
	wpsielen += 2;

	//	Value:
	*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( pwdinfo->tx_prov_disc_info.wps_config_method_request );
	wpsielen += 2;

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, wpsielen, (unsigned char *) wpsie, &pattrib->pktlen );


#ifdef CONFIG_WFD
	wfdielen = build_provdisc_req_wfd_ie(pwdinfo, pframe);
	pframe += wfdielen;
	pattrib->pktlen += wfdielen;
#endif //CONFIG_WFD

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

	return;

}


u8 is_matched_in_profilelist( u8* peermacaddr, struct profile_info* profileinfo )
{
	u8 i, match_result = 0;

	DBG_871X( "[%s] peermac = %.2X %.2X %.2X %.2X %.2X %.2X\n", __FUNCTION__,
	  	    peermacaddr[0], peermacaddr[1],peermacaddr[2],peermacaddr[3],peermacaddr[4],peermacaddr[5]);

	for( i = 0; i < P2P_MAX_PERSISTENT_GROUP_NUM; i++, profileinfo++ )
	{
	       DBG_871X( "[%s] profileinfo_mac = %.2X %.2X %.2X %.2X %.2X %.2X\n", __FUNCTION__,
		   	    profileinfo->peermac[0], profileinfo->peermac[1],profileinfo->peermac[2],profileinfo->peermac[3],profileinfo->peermac[4],profileinfo->peermac[5]);
		if ( _rtw_memcmp( peermacaddr, profileinfo->peermac, ETH_ALEN ) )
		{
			match_result = 1;
			DBG_871X( "[%s] Match!\n", __FUNCTION__ );
			break;
		}
	}

	return (match_result );
}

void issue_probersp_p2p(_adapter *padapter, unsigned char *da)
{
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	unsigned char					*mac;
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	//WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	u16					beacon_interval = 100;
	u16					capInfo = 0;
	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
	u8					wpsie[255] = { 0x00 };
	u32					wpsielen = 0, p2pielen = 0;
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD
#ifdef CONFIG_IOCTL_CFG80211
	struct cfg80211_wifidirect_info *pcfg80211_wdinfo = &padapter->cfg80211_wdinfo;
	struct ieee80211_channel *ieee_ch = &pcfg80211_wdinfo->remain_on_ch_channel;
	u8 listen_channel = (u8) ieee80211_frequency_to_channel(ieee_ch->center_freq);
#endif //CONFIG_IOCTL_CFG80211
#ifdef CONFIG_INTEL_WIDI
	u8 zero_array_check[L2SDTA_SERVICE_VE_LEN] = { 0x00 };
#endif //CONFIG_INTEL_WIDI

	//DBG_871X("%s\n", __FUNCTION__);

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	mac = myid(&(padapter->eeprompriv));

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_rtw_memcpy(pwlanhdr->addr1, da, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);

	//	Use the device address for BSSID field.
	_rtw_memcpy(pwlanhdr->addr3, mac, ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(fctrl, WIFI_PROBERSP);

	pattrib->hdrlen = sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = pattrib->hdrlen;
	pframe += pattrib->hdrlen;

	//timestamp will be inserted by hardware
	pframe += 8;
	pattrib->pktlen += 8;

	// beacon interval: 2 bytes
	_rtw_memcpy(pframe, (unsigned char *) &beacon_interval, 2);
	pframe += 2;
	pattrib->pktlen += 2;

	//	capability info: 2 bytes
	//	ESS and IBSS bits must be 0 (defined in the 3.1.2.1.1 of WiFi Direct Spec)
	capInfo |= cap_ShortPremble;
	capInfo |= cap_ShortSlot;

	_rtw_memcpy(pframe, (unsigned char *) &capInfo, 2);
	pframe += 2;
	pattrib->pktlen += 2;


	// SSID
	pframe = rtw_set_ie(pframe, _SSID_IE_, 7, pwdinfo->p2p_wildcard_ssid, &pattrib->pktlen);

	// supported rates...
	//	Use the OFDM rate in the P2P probe response frame. ( 6(B), 9(B), 12, 18, 24, 36, 48, 54 )
	pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_, 8, pwdinfo->support_rate, &pattrib->pktlen);

	// DS parameter set
#ifdef CONFIG_IOCTL_CFG80211
	if(wdev_to_priv(padapter->rtw_wdev)->p2p_enabled && listen_channel !=0 )
	{
		pframe = rtw_set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&listen_channel, &pattrib->pktlen);
	}
	else
#endif //CONFIG_IOCTL_CFG80211
	{
		pframe = rtw_set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&pwdinfo->listen_channel, &pattrib->pktlen);
	}


#ifdef CONFIG_IOCTL_CFG80211
	if(wdev_to_priv(padapter->rtw_wdev)->p2p_enabled)
	{
		if( pmlmepriv->wps_probe_resp_ie != NULL && pmlmepriv->p2p_probe_resp_ie != NULL )
		{
			//WPS IE
			_rtw_memcpy(pframe, pmlmepriv->wps_probe_resp_ie, pmlmepriv->wps_probe_resp_ie_len);
			pattrib->pktlen += pmlmepriv->wps_probe_resp_ie_len;
			pframe += pmlmepriv->wps_probe_resp_ie_len;

			//P2P IE
			_rtw_memcpy(pframe, pmlmepriv->p2p_probe_resp_ie, pmlmepriv->p2p_probe_resp_ie_len);
			pattrib->pktlen += pmlmepriv->p2p_probe_resp_ie_len;
			pframe += pmlmepriv->p2p_probe_resp_ie_len;
		}
	}
	else
#endif //CONFIG_IOCTL_CFG80211
	{

		//	Todo: WPS IE
		//	Noted by Albert 20100907
		//	According to the WPS specification, all the WPS attribute is presented by Big Endian.

		wpsielen = 0;
		//	WPS OUI
		*(u32*) ( wpsie ) = cpu_to_be32( WPSOUI );
		wpsielen += 4;

		//	WPS version
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_VER1 );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0001 );
		wpsielen += 2;

		//	Value:
		wpsie[wpsielen++] = WPS_VERSION_1;	//	Version 1.0

#ifdef CONFIG_INTEL_WIDI
		//	Commented by Kurt
		//	Appended WiDi info. only if we did issued_probereq_widi(), and then we saved ven. ext. in pmlmepriv->sa_ext.
		if(  _rtw_memcmp(pmlmepriv->sa_ext, zero_array_check, L2SDTA_SERVICE_VE_LEN) == _FALSE )
		{
			//Sec dev type
			*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_SEC_DEV_TYPE_LIST );
			wpsielen += 2;

			//	Length:
			*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0008 );
			wpsielen += 2;

			//	Value:
			//	Category ID
			*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_PDT_CID_DISPLAYS );
			wpsielen += 2;

			//	OUI
			*(u32*) ( wpsie + wpsielen ) = cpu_to_be32( INTEL_DEV_TYPE_OUI );
			wpsielen += 4;

			*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_PDT_SCID_WIDI_CONSUMER_SINK );
			wpsielen += 2;

			//	Vendor Extension
			_rtw_memcpy( wpsie + wpsielen, pmlmepriv->sa_ext, L2SDTA_SERVICE_VE_LEN );
			wpsielen += L2SDTA_SERVICE_VE_LEN;
		}
#endif //CONFIG_INTEL_WIDI

		//	WiFi Simple Config State
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_SIMPLE_CONF_STATE );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0001 );
		wpsielen += 2;

		//	Value:
		wpsie[wpsielen++] = WPS_WSC_STATE_NOT_CONFIG;	//	Not Configured.

		//	Response Type
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_RESP_TYPE );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0001 );
		wpsielen += 2;

		//	Value:
		wpsie[wpsielen++] = WPS_RESPONSE_TYPE_8021X;

		//	UUID-E
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_UUID_E );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0010 );
		wpsielen += 2;

		//	Value:
		_rtw_memcpy( wpsie + wpsielen, myid( &padapter->eeprompriv ), ETH_ALEN );
		wpsielen += 0x10;

		//	Manufacturer
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_MANUFACTURER );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0007 );
		wpsielen += 2;

		//	Value:
		_rtw_memcpy( wpsie + wpsielen, "Realtek", 7 );
		wpsielen += 7;

		//	Model Name
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_MODEL_NAME );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0006 );
		wpsielen += 2;

		//	Value:
		_rtw_memcpy( wpsie + wpsielen, "8192CU", 6 );
		wpsielen += 6;

		//	Model Number
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_MODEL_NUMBER );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0001 );
		wpsielen += 2;

		//	Value:
		wpsie[ wpsielen++ ] = 0x31;		//	character 1

		//	Serial Number
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_SERIAL_NUMBER );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( ETH_ALEN );
		wpsielen += 2;

		//	Value:
		_rtw_memcpy( wpsie + wpsielen, "123456" , ETH_ALEN );
		wpsielen += ETH_ALEN;

		//	Primary Device Type
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_PRIMARY_DEV_TYPE );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0008 );
		wpsielen += 2;

		//	Value:
		//	Category ID
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_PDT_CID_MULIT_MEDIA );
		wpsielen += 2;

		//	OUI
		*(u32*) ( wpsie + wpsielen ) = cpu_to_be32( WPSOUI );
		wpsielen += 4;

		//	Sub Category ID
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_PDT_SCID_MEDIA_SERVER );
		wpsielen += 2;

		//	Device Name
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_DEVICE_NAME );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( pwdinfo->device_name_len );
		wpsielen += 2;

		//	Value:
		_rtw_memcpy( wpsie + wpsielen, pwdinfo->device_name, pwdinfo->device_name_len );
		wpsielen += pwdinfo->device_name_len;

		//	Config Method
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_CONF_METHOD );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0002 );
		wpsielen += 2;

		//	Value:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( pwdinfo->supported_wps_cm );
		wpsielen += 2;


		pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, wpsielen, (unsigned char *) wpsie, &pattrib->pktlen );


		p2pielen = build_probe_resp_p2p_ie(pwdinfo, pframe);
		pframe += p2pielen;
		pattrib->pktlen += p2pielen;
	}

#ifdef CONFIG_WFD
#ifdef CONFIG_IOCTL_CFG80211
	if ( _TRUE == pwdinfo->wfd_info->wfd_enable )
#endif
	{
		wfdielen = build_probe_resp_wfd_ie(pwdinfo, pframe, 0);
		pframe += wfdielen;
		pattrib->pktlen += wfdielen;
	}
#endif //CONFIG_WFD

	pattrib->last_txcmdsz = pattrib->pktlen;


	dump_mgntframe(padapter, pmgntframe);

	return;

}

void issue_probereq_p2p(_adapter *padapter)
{
	struct xmit_frame		*pmgntframe;
	struct pkt_attrib		*pattrib;
	unsigned char			*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short		*fctrl;
	unsigned char			*mac;
	unsigned char			bssrate[NumRates];
	struct xmit_priv		*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	int	bssrate_len = 0;
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
	u8					wpsie[255] = { 0x00 }, p2pie[ 255 ] = { 0x00 };
	u16					wpsielen = 0, p2pielen = 0;
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD
#ifdef CONFIG_IOCTL_CFG80211
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
#endif //CONFIG_IOCTL_CFG80211

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);


	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	mac = myid(&(padapter->eeprompriv));

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	//	broadcast probe request frame
	_rtw_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, bc_addr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_PROBEREQ);

	pframe += sizeof (struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof (struct rtw_ieee80211_hdr_3addr);

	if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_TX_PROVISION_DIS_REQ))
	{
		pframe = rtw_set_ie(pframe, _SSID_IE_, pwdinfo->tx_prov_disc_info.ssid.SsidLength, pwdinfo->tx_prov_disc_info.ssid.Ssid, &(pattrib->pktlen));
	}
	else
	{
		pframe = rtw_set_ie(pframe, _SSID_IE_, P2P_WILDCARD_SSID_LEN, pwdinfo->p2p_wildcard_ssid, &(pattrib->pktlen));
	}
	//	Use the OFDM rate in the P2P probe request frame. ( 6(B), 9(B), 12(B), 24(B), 36, 48, 54 )
	pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_, 8, pwdinfo->support_rate, &pattrib->pktlen);

#ifdef CONFIG_IOCTL_CFG80211
	if(wdev_to_priv(padapter->rtw_wdev)->p2p_enabled)
	{
		if( pmlmepriv->wps_probe_req_ie != NULL && pmlmepriv->p2p_probe_req_ie != NULL )
		{
			//WPS IE
			_rtw_memcpy(pframe, pmlmepriv->wps_probe_req_ie, pmlmepriv->wps_probe_req_ie_len);
			pattrib->pktlen += pmlmepriv->wps_probe_req_ie_len;
			pframe += pmlmepriv->wps_probe_req_ie_len;

			//P2P IE
			_rtw_memcpy(pframe, pmlmepriv->p2p_probe_req_ie, pmlmepriv->p2p_probe_req_ie_len);
			pattrib->pktlen += pmlmepriv->p2p_probe_req_ie_len;
			pframe += pmlmepriv->p2p_probe_req_ie_len;
		}
	}
	else
#endif //CONFIG_IOCTL_CFG80211
	{

		//	WPS IE
		//	Noted by Albert 20110221
		//	According to the WPS specification, all the WPS attribute is presented by Big Endian.

		wpsielen = 0;
		//	WPS OUI
		*(u32*) ( wpsie ) = cpu_to_be32( WPSOUI );
		wpsielen += 4;

		//	WPS version
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_VER1 );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0001 );
		wpsielen += 2;

		//	Value:
		wpsie[wpsielen++] = WPS_VERSION_1;	//	Version 1.0

		//	Device Name
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_DEVICE_NAME );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( pwdinfo->device_name_len );
		wpsielen += 2;

		//	Value:
		_rtw_memcpy( wpsie + wpsielen, pwdinfo->device_name, pwdinfo->device_name_len );
		wpsielen += pwdinfo->device_name_len;

		//	Primary Device Type
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_PRIMARY_DEV_TYPE );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0008 );
		wpsielen += 2;

		//	Value:
		//	Category ID
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_PDT_CID_RTK_WIDI );
		wpsielen += 2;

		//	OUI
		*(u32*) ( wpsie + wpsielen ) = cpu_to_be32( WPSOUI );
		wpsielen += 4;

		//	Sub Category ID
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_PDT_SCID_RTK_DMP );
		wpsielen += 2;

		//	Device Password ID
		//	Type:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_ATTR_DEVICE_PWID );
		wpsielen += 2;

		//	Length:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( 0x0002 );
		wpsielen += 2;

		//	Value:
		*(u16*) ( wpsie + wpsielen ) = cpu_to_be16( WPS_DPID_REGISTRAR_SPEC );	//	Registrar-specified
		wpsielen += 2;

		pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, wpsielen, (unsigned char *) wpsie, &pattrib->pktlen );

		//	P2P OUI
		p2pielen = 0;
		p2pie[ p2pielen++ ] = 0x50;
		p2pie[ p2pielen++ ] = 0x6F;
		p2pie[ p2pielen++ ] = 0x9A;
		p2pie[ p2pielen++ ] = 0x09;	//	WFA P2P v1.0

		//	Commented by Albert 20110221
		//	According to the P2P Specification, the probe request frame should contain 5 P2P attributes
		//	1. P2P Capability
		//	2. P2P Device ID if this probe request wants to find the specific P2P device
		//	3. Listen Channel
		//	4. Extended Listen Timing
		//	5. Operating Channel if this WiFi is working as the group owner now

		//	P2P Capability
		//	Type:
		p2pie[ p2pielen++ ] = P2P_ATTR_CAPABILITY;

		//	Length:
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0002 );
		p2pielen += 2;

		//	Value:
		//	Device Capability Bitmap, 1 byte
		p2pie[ p2pielen++ ] = DMP_P2P_DEVCAP_SUPPORT;

		//	Group Capability Bitmap, 1 byte
		if ( pwdinfo->persistent_supported )
			p2pie[ p2pielen++ ] = P2P_GRPCAP_PERSISTENT_GROUP | DMP_P2P_GRPCAP_SUPPORT;
		else
			p2pie[ p2pielen++ ] = DMP_P2P_GRPCAP_SUPPORT;

		//	Listen Channel
		//	Type:
		p2pie[ p2pielen++ ] = P2P_ATTR_LISTEN_CH;

		//	Length:
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0005 );
		p2pielen += 2;

		//	Value:
		//	Country String
		p2pie[ p2pielen++ ] = 'U';
		p2pie[ p2pielen++ ] = 'S';

		//	The third byte should be set to 0x04.
		//	Described in the "Operating Channel Attribute" section.
		p2pie[ p2pielen++ ] = 0x04;

		//	Operating Class
		p2pie[ p2pielen++ ] = 0x51;	//	Copy from SD7

		//	Channel Number
		p2pie[ p2pielen++ ] = pwdinfo->listen_channel;	//	listen channel


		//	Extended Listen Timing
		//	Type:
		p2pie[ p2pielen++ ] = P2P_ATTR_EX_LISTEN_TIMING;

		//	Length:
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0004 );
		p2pielen += 2;

		//	Value:
		//	Availability Period
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0xFFFF );
		p2pielen += 2;

		//	Availability Interval
		*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0xFFFF );
		p2pielen += 2;

		if ( rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO) )
		{
			//	Operating Channel (if this WiFi is working as the group owner now)
			//	Type:
			p2pie[ p2pielen++ ] = P2P_ATTR_OPERATING_CH;

			//	Length:
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0005 );
			p2pielen += 2;

			//	Value:
			//	Country String
			p2pie[ p2pielen++ ] = 'U';
			p2pie[ p2pielen++ ] = 'S';

			//	The third byte should be set to 0x04.
			//	Described in the "Operating Channel Attribute" section.
			p2pie[ p2pielen++ ] = 0x04;

			//	Operating Class
			p2pie[ p2pielen++ ] = 0x51;	//	Copy from SD7

			//	Channel Number
			p2pie[ p2pielen++ ] = pwdinfo->operating_channel;	//	operating channel number

		}

		pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, p2pielen, (unsigned char *) p2pie, &pattrib->pktlen );

	}

#ifdef CONFIG_WFD
#ifdef CONFIG_IOCTL_CFG80211
	if ( _TRUE == pwdinfo->wfd_info->wfd_enable )
#endif
	{
		wfdielen = build_probe_req_wfd_ie(pwdinfo, pframe);
		pframe += wfdielen;
		pattrib->pktlen += wfdielen;
	}
#endif //CONFIG_WFD

	pattrib->last_txcmdsz = pattrib->pktlen;

	RT_TRACE(_module_rtl871x_mlme_c_,_drv_info_,("issuing probe_req, tx_len=%d\n", pattrib->last_txcmdsz));

	dump_mgntframe(padapter, pmgntframe);

	return;
}

#endif //CONFIG_P2P

unsigned int OnAction_public(_adapter *padapter, union recv_frame *precv_frame)
{
	unsigned char		*frame_body;
	unsigned char		category, action, dialogToken=0;
	u8 *pframe = precv_frame->u.hdr.rx_data;
	uint len = precv_frame->u.hdr.len;
#ifdef CONFIG_P2P
	u8 *p2p_ie;
	u32	p2p_ielen, wps_ielen;
	struct	wifidirect_info	*pwdinfo = &( padapter->wdinfo );
	u8	result = P2P_STATUS_SUCCESS;
	u8	empty_addr[ETH_ALEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif //CONFIG_P2P
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);

	u16 seq_ctrl = ( (precv_frame->u.hdr.attrib.seq_num&0xffff) << 4) |
		(precv_frame->u.hdr.attrib.frag_num & 0xf);

	//check RA matches or not
	if (!_rtw_memcmp(myid(&(padapter->eeprompriv)), GetAddr1Ptr(pframe), ETH_ALEN))//for if1, sta/ap mode
		return _SUCCESS;

	frame_body = (unsigned char *)(pframe + sizeof(struct rtw_ieee80211_hdr_3addr));

	dialogToken = frame_body[7];

	//recv_decache check
	if(GetRetry(pframe))
	{
		if((seq_ctrl == pmlmeext->action_public_rxseq)
			&& (dialogToken == pmlmeext->action_public_dialog_token))
		{
			DBG_871X("recv_Action_public_decache, seq_ctrl=0x%x, rxseq=0x%x\n", seq_ctrl, pmlmeext->action_public_rxseq);

			return _FAIL;
		}
	}
	pmlmeext->action_public_rxseq = seq_ctrl;
	pmlmeext->action_public_dialog_token = dialogToken;

	category = frame_body[0];
	if(category != RTW_WLAN_CATEGORY_PUBLIC)
		return _SUCCESS;

	action = frame_body[ 1 ];
	if ( action == ACT_PUBLIC_P2P )	//	IEEE 802.11 P2P Public Action usage.
	{
#ifdef CONFIG_P2P
		//	Do nothing if the driver doesn't enable the P2P function.
#ifndef CONFIG_IOCTL_CFG80211
		if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE) || rtw_p2p_chk_state(pwdinfo, P2P_STATE_IDLE))
			return _SUCCESS;
#endif //CONFIG_IOCTL_CFG80211

		//	Commented by Albert 20100908
		//	Low byte -> High byte is 0x50, 0x6F, 0x9A, 0x09 for P2P OUI.
		//	But the P2POUT is defined as 0x506F9A09 -> should use the cpu_to_be32
		if ( cpu_to_be32( *( ( u32* ) ( frame_body + 2 ) ) ) == P2POUI )
		{
#ifdef CONFIG_IOCTL_CFG80211
			if(wdev_to_priv(padapter->rtw_wdev)->p2p_enabled)
			{
				rtw_cfg80211_rx_p2p_action_public(padapter, pframe, len);
			}
			else
#endif //CONFIG_IOCTL_CFG80211
			{
				len -= sizeof(struct rtw_ieee80211_hdr_3addr);

				switch( frame_body[ 6 ] )//OUI Subtype
				{
					case P2P_GO_NEGO_REQ:
					{
						DBG_871X( "[%s] Got GO Nego Req Frame\n", __FUNCTION__);
						_rtw_memset( &pwdinfo->groupid_info, 0x00, sizeof( struct group_id_info ) );

						if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_GONEGO_FAIL))
						{
							//	Commented by Albert 20110526
							//	In this case, this means the previous nego fail doesn't be reset yet.
							_cancel_timer_ex( &pwdinfo->restore_p2p_state_timer );
							//	Restore the previous p2p state
							rtw_p2p_set_state(pwdinfo, rtw_p2p_pre_state(pwdinfo));
							DBG_871X( "[%s] Restore the previous p2p state to %d\n", __FUNCTION__, rtw_p2p_state(pwdinfo) );
						}
#ifdef CONFIG_CONCURRENT_MODE
						if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
						{
							_cancel_timer_ex( &pwdinfo->ap_p2p_switch_timer );
						}
#endif // CONFIG_CONCURRENT_MODE

						//	Commented by Kurt 20110902
						//Add if statement to avoid receiving duplicate prov disc req. such that pre_p2p_state would be covered.
						if(!rtw_p2p_chk_state(pwdinfo, P2P_STATE_GONEGO_ING))
							rtw_p2p_set_pre_state(pwdinfo, rtw_p2p_state(pwdinfo));

						//	Commented by Kurt 20120113
						//	Get peer_dev_addr here if peer doesn't issue prov_disc frame.
						if( _rtw_memcmp(pwdinfo->rx_prov_disc_info.peerDevAddr, empty_addr, ETH_ALEN) );
							_rtw_memcpy(pwdinfo->rx_prov_disc_info.peerDevAddr, GetAddr2Ptr(pframe), ETH_ALEN);

						result = process_p2p_group_negotation_req( pwdinfo, frame_body, len );
						issue_p2p_GO_response( padapter, GetAddr2Ptr(pframe), frame_body, len, result );
#ifdef CONFIG_INTEL_WIDI
						if( (padapter->mlmepriv.widi_state == INTEL_WIDI_STATE_LISTEN) && (padapter->mlmepriv.widi_state != INTEL_WIDI_STATE_WFD_CONNECTION) )
						{
							padapter->mlmepriv.widi_state = INTEL_WIDI_STATE_WFD_CONNECTION;
							intel_widi_wk_cmd(padapter, INTEL_WIDI_LISTEN_STOP_WK, NULL);
						}
#endif //CONFIG_INTEL_WIDI

						//	Commented by Albert 20110718
						//	No matter negotiating or negotiation failure, the driver should set up the restore P2P state timer.
#ifdef CONFIG_CONCURRENT_MODE
						//	Commented by Albert 20120107
						_set_timer( &pwdinfo->restore_p2p_state_timer, 3000 );
#else // CONFIG_CONCURRENT_MODE
						_set_timer( &pwdinfo->restore_p2p_state_timer, 5000 );
#endif // CONFIG_CONCURRENT_MODE
						break;
					}
					case P2P_GO_NEGO_RESP:
					{
						DBG_871X( "[%s] Got GO Nego Resp Frame\n", __FUNCTION__);

						if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_GONEGO_ING))
						{
							//	Commented by Albert 20110425
							//	The restore timer is enabled when issuing the nego request frame of rtw_p2p_connect function.
							_cancel_timer_ex( &pwdinfo->restore_p2p_state_timer );
							pwdinfo->nego_req_info.benable = _FALSE;
							result = process_p2p_group_negotation_resp( pwdinfo, frame_body, len);

							issue_p2p_GO_confirm( pwdinfo->padapter, GetAddr2Ptr(pframe), result);

							//	Reset the dialog token for group negotiation frames.
							pwdinfo->negotiation_dialog_token = 1;

							if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_GONEGO_FAIL))
							{
								_set_timer( &pwdinfo->restore_p2p_state_timer, 5000 );
							}
						}
						else
						{
							DBG_871X( "[%s] Skipped GO Nego Resp Frame (p2p_state != P2P_STATE_GONEGO_ING)\n", __FUNCTION__);
						}

						break;
					}
					case P2P_GO_NEGO_CONF:
					{
						DBG_871X( "[%s] Got GO Nego Confirm Frame\n", __FUNCTION__);
						process_p2p_group_negotation_confirm( pwdinfo, frame_body, len);
						break;
					}
					case P2P_INVIT_REQ:
					{
						//	Added by Albert 2010/10/05
						//	Received the P2P Invite Request frame.

						DBG_871X( "[%s] Got invite request frame!\n", __FUNCTION__ );
						if ( (p2p_ie=rtw_get_p2p_ie( frame_body + _PUBLIC_ACTION_IE_OFFSET_, len - _PUBLIC_ACTION_IE_OFFSET_, NULL, &p2p_ielen)) )
						{
							//	Parse the necessary information from the P2P Invitation Request frame.
							//	For example: The MAC address of sending this P2P Invitation Request frame.
							u32	attr_contentlen = 0;
							u8	status_code = P2P_STATUS_FAIL_INFO_UNAVAILABLE;
							u8	match_result = 0;
							struct group_id_info group_id;
							u8	invitation_flag = 0;

							rtw_get_p2p_attr_content( p2p_ie, p2p_ielen, P2P_ATTR_INVITATION_FLAGS, &invitation_flag, &attr_contentlen);
							if ( attr_contentlen )
							{

								rtw_get_p2p_attr_content( p2p_ie, p2p_ielen, P2P_ATTR_GROUP_BSSID, pwdinfo->p2p_peer_interface_addr, &attr_contentlen);
								//	Commented by Albert 20120510
								//	Copy to the pwdinfo->p2p_peer_interface_addr.
								//	So that the WFD UI ( or Sigma ) can get the peer interface address by using the following command.
								//	#> iwpriv wlan0 p2p_get peer_ifa
								//	After having the peer interface address, the sigma can find the correct conf file for wpa_supplicant.

								if ( attr_contentlen )
								{
									DBG_871X( "[%s] GO's BSSID = %.2X %.2X %.2X %.2X %.2X %.2X\n", __FUNCTION__,
											pwdinfo->p2p_peer_interface_addr[0], pwdinfo->p2p_peer_interface_addr[1],
											pwdinfo->p2p_peer_interface_addr[2], pwdinfo->p2p_peer_interface_addr[3],
											pwdinfo->p2p_peer_interface_addr[4], pwdinfo->p2p_peer_interface_addr[5] );
								}

								if ( invitation_flag & P2P_INVITATION_FLAGS_PERSISTENT )
								{
									//	Re-invoke the persistent group.

									_rtw_memset( &group_id, 0x00, sizeof( struct group_id_info ) );
									rtw_get_p2p_attr_content( p2p_ie, p2p_ielen, P2P_ATTR_GROUP_ID, ( u8* ) &group_id, &attr_contentlen);
									if ( attr_contentlen )
									{
										if ( _rtw_memcmp( group_id.go_device_addr, myid( &padapter->eeprompriv ), ETH_ALEN ) )
										{
											//	The p2p device sending this p2p invitation request wants this Wi-Fi device to be the persistent GO.
											rtw_p2p_set_state(pwdinfo, P2P_STATE_RECV_INVITE_REQ_GO );
											rtw_p2p_set_role( pwdinfo, P2P_ROLE_GO );
											status_code = P2P_STATUS_SUCCESS;
										}
										else
										{
											//	The p2p device sending this p2p invitation request wants to be the persistent GO.
											if ( is_matched_in_profilelist( pwdinfo->p2p_peer_interface_addr, &pwdinfo->profileinfo[ 0 ] ) )
											{
												rtw_p2p_set_state(pwdinfo, P2P_STATE_RECV_INVITE_REQ_MATCH );
												rtw_p2p_set_role( pwdinfo, P2P_ROLE_CLIENT );
												status_code = P2P_STATUS_SUCCESS;
											}
											else
											{
												rtw_p2p_set_state(pwdinfo, P2P_STATE_RECV_INVITE_REQ_DISMATCH );
												#ifdef CONFIG_INTEL_WIDI
												_rtw_memcpy( pwdinfo->p2p_peer_device_addr, group_id.go_device_addr , ETH_ALEN );
												rtw_p2p_set_role( pwdinfo, P2P_ROLE_CLIENT );
												#endif //CONFIG_INTEL_WIDI

												status_code = P2P_STATUS_FAIL_UNKNOWN_P2PGROUP;
											}
										}
									}
									else
									{
										DBG_871X( "[%s] P2P Group ID Attribute NOT FOUND!\n", __FUNCTION__ );
										status_code = P2P_STATUS_FAIL_INFO_UNAVAILABLE;
									}
								}
								else
								{
									//	Received the invitation to join a P2P group.

									_rtw_memset( &group_id, 0x00, sizeof( struct group_id_info ) );
									rtw_get_p2p_attr_content( p2p_ie, p2p_ielen, P2P_ATTR_GROUP_ID, ( u8* ) &group_id, &attr_contentlen);
									if ( attr_contentlen )
									{
										if ( _rtw_memcmp( group_id.go_device_addr, myid( &padapter->eeprompriv ), ETH_ALEN ) )
										{
											//	In this case, the GO can't be myself.
											rtw_p2p_set_state(pwdinfo, P2P_STATE_RECV_INVITE_REQ_DISMATCH );
											status_code = P2P_STATUS_FAIL_INFO_UNAVAILABLE;
										}
										else
										{
											//	The p2p device sending this p2p invitation request wants to join an existing P2P group
											//	Commented by Albert 2012/06/28
											//	In this case, this Wi-Fi device should use the iwpriv command to get the peer device address.
											//	The peer device address should be the destination address for the provisioning discovery request.
											//	Then, this Wi-Fi device should use the iwpriv command to get the peer interface address.
											//	The peer interface address should be the address for WPS mac address
											_rtw_memcpy( pwdinfo->p2p_peer_device_addr, group_id.go_device_addr , ETH_ALEN );
											rtw_p2p_set_role( pwdinfo, P2P_ROLE_CLIENT );
											rtw_p2p_set_state(pwdinfo, P2P_STATE_RECV_INVITE_REQ_JOIN );
											status_code = P2P_STATUS_SUCCESS;
										}
									}
									else
									{
										DBG_871X( "[%s] P2P Group ID Attribute NOT FOUND!\n", __FUNCTION__ );
										status_code = P2P_STATUS_FAIL_INFO_UNAVAILABLE;
									}
								}
							}
							else
							{
								DBG_871X( "[%s] P2P Invitation Flags Attribute NOT FOUND!\n", __FUNCTION__ );
								status_code = P2P_STATUS_FAIL_INFO_UNAVAILABLE;
							}

							DBG_871X( "[%s] status_code = %d\n", __FUNCTION__, status_code );

							pwdinfo->inviteresp_info.token = frame_body[ 7 ];
							issue_p2p_invitation_response( padapter, GetAddr2Ptr(pframe), pwdinfo->inviteresp_info.token, status_code );
						}
#ifdef CONFIG_INTEL_WIDI
						if( (padapter->mlmepriv.widi_state == INTEL_WIDI_STATE_LISTEN) && (padapter->mlmepriv.widi_state != INTEL_WIDI_STATE_WFD_CONNECTION) )
						{
							padapter->mlmepriv.widi_state = INTEL_WIDI_STATE_WFD_CONNECTION;
							intel_widi_wk_cmd(padapter, INTEL_WIDI_LISTEN_STOP_WK, NULL);
						}
#endif //CONFIG_INTEL_WIDI
						break;
					}
					case P2P_INVIT_RESP:
					{
						u8	attr_content = 0x00;
						u32	attr_contentlen = 0;

						DBG_871X( "[%s] Got invite response frame!\n", __FUNCTION__ );
						_cancel_timer_ex( &pwdinfo->restore_p2p_state_timer );
						if ( (p2p_ie=rtw_get_p2p_ie( frame_body + _PUBLIC_ACTION_IE_OFFSET_, len - _PUBLIC_ACTION_IE_OFFSET_, NULL, &p2p_ielen)) )
						{
							rtw_get_p2p_attr_content(p2p_ie, p2p_ielen, P2P_ATTR_STATUS, &attr_content, &attr_contentlen);

							if ( attr_contentlen == 1 )
							{
								DBG_871X( "[%s] Status = %d\n", __FUNCTION__, attr_content );
								pwdinfo->invitereq_info.benable = _FALSE;

								if ( attr_content == P2P_STATUS_SUCCESS )
								{
									rtw_p2p_set_role(pwdinfo, P2P_ROLE_CLIENT);
								}
								else
								{
									rtw_p2p_set_role(pwdinfo, P2P_ROLE_DEVICE);
								}
							}
							else
							{
								rtw_p2p_set_role(pwdinfo, P2P_ROLE_DEVICE);
							}
						}

						rtw_p2p_set_state( pwdinfo, P2P_STATE_RX_INVITE_RESP );
						break;
					}
					case P2P_DEVDISC_REQ:

						process_p2p_devdisc_req(pwdinfo, pframe, len);

						break;

					case P2P_DEVDISC_RESP:

						process_p2p_devdisc_resp(pwdinfo, pframe, len);

						break;

					case P2P_PROVISION_DISC_REQ:
						DBG_871X( "[%s] Got Provisioning Discovery Request Frame\n", __FUNCTION__ );
						process_p2p_provdisc_req(pwdinfo, pframe, len);
						_rtw_memcpy(pwdinfo->rx_prov_disc_info.peerDevAddr, GetAddr2Ptr(pframe), ETH_ALEN);

						//20110902 Kurt
						//Add the following statement to avoid receiving duplicate prov disc req. such that pre_p2p_state would be covered.
						if(!rtw_p2p_chk_state(pwdinfo, P2P_STATE_RX_PROVISION_DIS_REQ))
							rtw_p2p_set_pre_state(pwdinfo, rtw_p2p_state(pwdinfo));

						rtw_p2p_set_state(pwdinfo, P2P_STATE_RX_PROVISION_DIS_REQ);
						_set_timer( &pwdinfo->restore_p2p_state_timer, P2P_PROVISION_TIMEOUT );
#ifdef CONFIG_INTEL_WIDI
						if( (padapter->mlmepriv.widi_state == INTEL_WIDI_STATE_LISTEN) && (padapter->mlmepriv.widi_state != INTEL_WIDI_STATE_WFD_CONNECTION) )
						{
							padapter->mlmepriv.widi_state = INTEL_WIDI_STATE_WFD_CONNECTION;
							intel_widi_wk_cmd(padapter, INTEL_WIDI_LISTEN_STOP_WK, NULL);
						}
#endif //CONFIG_INTEL_WIDI
						break;

					case P2P_PROVISION_DISC_RESP:
						//	Commented by Albert 20110707
						//	Should we check the pwdinfo->tx_prov_disc_info.bsent flag here??
						DBG_871X( "[%s] Got Provisioning Discovery Response Frame\n", __FUNCTION__ );
						//	Commented by Albert 20110426
						//	The restore timer is enabled when issuing the provisioing request frame in rtw_p2p_prov_disc function.
						_cancel_timer_ex( &pwdinfo->restore_p2p_state_timer );
						rtw_p2p_set_state(pwdinfo, P2P_STATE_RX_PROVISION_DIS_RSP);
						process_p2p_provdisc_resp(pwdinfo, pframe);
						_set_timer( &pwdinfo->restore_p2p_state_timer, P2P_PROVISION_TIMEOUT );
						break;

				}
			}
		}
#endif //CONFIG_P2P
	}

	return _SUCCESS;
}

unsigned int OnAction_ht(_adapter *padapter, union recv_frame *precv_frame)
{
	return _SUCCESS;
}

unsigned int OnAction_wmm(_adapter *padapter, union recv_frame *precv_frame)
{
	return _SUCCESS;
}

unsigned int OnAction_p2p(_adapter *padapter, union recv_frame *precv_frame)
{
#ifdef CONFIG_P2P
	u8 *frame_body;
	u8 category, OUI_Subtype, dialogToken=0;
	u8 *pframe = precv_frame->u.hdr.rx_data;
	uint len = precv_frame->u.hdr.len;
	struct	wifidirect_info	*pwdinfo = &( padapter->wdinfo );


	DBG_871X("%s\n", __FUNCTION__);

	//check RA matches or not
	if (!_rtw_memcmp(myid(&(padapter->eeprompriv)), GetAddr1Ptr(pframe), ETH_ALEN))//for if1, sta/ap mode
		return _SUCCESS;

	frame_body = (unsigned char *)(pframe + sizeof(struct rtw_ieee80211_hdr_3addr));

	category = frame_body[0];
	if(category != RTW_WLAN_CATEGORY_P2P)
		return _SUCCESS;

	if ( cpu_to_be32( *( ( u32* ) ( frame_body + 1 ) ) ) != P2POUI )
		return _SUCCESS;

#ifdef CONFIG_IOCTL_CFG80211
	if(wdev_to_priv(padapter->rtw_wdev)->p2p_enabled)
	{
		rtw_cfg80211_rx_action_p2p(padapter, pframe, len);
		return _SUCCESS;
	}
	else
#endif //CONFIG_IOCTL_CFG80211
	{
		len -= sizeof(struct rtw_ieee80211_hdr_3addr);
		OUI_Subtype = frame_body[5];
		dialogToken = frame_body[6];

		switch(OUI_Subtype)
		{
			case P2P_NOTICE_OF_ABSENCE:

				break;

			case P2P_PRESENCE_REQUEST:

				process_p2p_presence_req(pwdinfo, pframe, len);

				break;

			case P2P_PRESENCE_RESPONSE:

				break;

			case P2P_GO_DISC_REQUEST:

				break;

			default:
				break;

		}
	}
#endif //CONFIG_P2P

	return _SUCCESS;

}

unsigned int OnAction(_adapter *padapter, union recv_frame *precv_frame)
{
	int i;
	unsigned char	category;
	struct action_handler *ptable;
	unsigned char	*frame_body;
	u8 *pframe = precv_frame->u.hdr.rx_data;

	frame_body = (unsigned char *)(pframe + sizeof(struct rtw_ieee80211_hdr_3addr));

	category = frame_body[0];

	for(i = 0; i < sizeof(OnAction_tbl)/sizeof(struct action_handler); i++)
	{
		ptable = &OnAction_tbl[i];

		if(category == ptable->num)
			ptable->func(padapter, precv_frame);

	}

	return _SUCCESS;

}

unsigned int DoReserved(_adapter *padapter, union recv_frame *precv_frame)
{

	//DBG_871X("rcvd mgt frame(%x, %x)\n", (GetFrameSubType(pframe) >> 4), *(unsigned int *)GetAddr1Ptr(pframe));
	return _SUCCESS;
}

struct xmit_frame *alloc_mgtxmitframe(struct xmit_priv *pxmitpriv)
{
	struct xmit_frame			*pmgntframe;
	struct xmit_buf				*pxmitbuf;

	if ((pmgntframe = rtw_alloc_xmitframe(pxmitpriv)) == NULL)
	{
		DBG_871X("%s, alloc xmitframe fail\n", __FUNCTION__);
		return NULL;
	}

	if ((pxmitbuf = rtw_alloc_xmitbuf_ext(pxmitpriv)) == NULL)
	{
		DBG_871X("%s, alloc xmitbuf fail\n", __FUNCTION__);
		rtw_free_xmitframe(pxmitpriv, pmgntframe);
		return NULL;
	}

	pmgntframe->frame_tag = MGNT_FRAMETAG;

	pmgntframe->pxmitbuf = pxmitbuf;

	pmgntframe->buf_addr = pxmitbuf->pbuf;

	pxmitbuf->priv_data = pmgntframe;

	return pmgntframe;

}


/****************************************************************************

Following are some TX fuctions for WiFi MLME

*****************************************************************************/

void update_mgnt_tx_rate(_adapter *padapter, u8 rate)
{
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);

	pmlmeext->tx_rate = rate;
	DBG_871X("%s(): rate = %x\n",__FUNCTION__, rate);
}

void update_mgntframe_attrib(_adapter *padapter, struct pkt_attrib *pattrib)
{
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);

	_rtw_memset((u8 *)(pattrib), 0, sizeof(struct pkt_attrib));

	pattrib->hdrlen = 24;
	pattrib->nr_frags = 1;
	pattrib->priority = 7;
	pattrib->mac_id = 0;
	pattrib->qsel = 0x12;

	pattrib->pktlen = 0;

	if(pmlmeext->cur_wireless_mode & WIRELESS_11B)
		pattrib->raid = 6;//b mode
	else
		pattrib->raid = 5;//a/g mode

	pattrib->encrypt = _NO_PRIVACY_;
	pattrib->bswenc = _FALSE;

	pattrib->qos_en = _FALSE;
	pattrib->ht_en = _FALSE;
	pattrib->bwmode = HT_CHANNEL_WIDTH_20;
	pattrib->ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
	pattrib->sgi = _FALSE;

	pattrib->seqnum = pmlmeext->mgnt_seq;

	pattrib->retry_ctrl = _TRUE;

}

void dump_mgntframe(_adapter *padapter, struct xmit_frame *pmgntframe)
{
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	struct xmit_buf *pxmitbuf = pmgntframe->pxmitbuf;

	if(padapter->bSurpriseRemoved == _TRUE ||
		padapter->bDriverStopped == _TRUE)
	{
		rtw_free_xmitframe(pxmitpriv, pmgntframe);
		rtw_free_xmitbuf(pxmitpriv, pxmitbuf);
		return;
	}

	rtw_hal_mgnt_xmit(padapter, pmgntframe);
}

s32 dump_mgntframe_and_wait(_adapter *padapter, struct xmit_frame *pmgntframe, int timeout_ms)
{
	s32 ret = _FAIL;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	struct xmit_buf *pxmitbuf = pmgntframe->pxmitbuf;
	struct submit_ctx sctx;

	if(padapter->bSurpriseRemoved == _TRUE ||
		padapter->bDriverStopped == _TRUE)
	{
		rtw_free_xmitframe(pxmitpriv, pmgntframe);
		rtw_free_xmitbuf(pxmitpriv, pxmitbuf);
		return ret;
	}

	rtw_sctx_init(&sctx, timeout_ms);
	pxmitbuf->sctx = &sctx;

	ret = rtw_hal_mgnt_xmit(padapter, pmgntframe);

	if (ret == _SUCCESS)
		ret = rtw_sctx_wait(&sctx);

	 return ret;
}

s32 dump_mgntframe_and_wait_ack(_adapter *padapter, struct xmit_frame *pmgntframe)
{
#ifdef CONFIG_XMIT_ACK
	s32 ret = _FAIL;
	u32 timeout_ms = 500;//  500ms
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
#ifdef CONFIG_CONCURRENT_MODE
	if (padapter->pbuddy_adapter && !padapter->isprimary)
		pxmitpriv = &(padapter->pbuddy_adapter->xmitpriv);
#endif

	if(padapter->bSurpriseRemoved == _TRUE ||
		padapter->bDriverStopped == _TRUE)
		return ret;

	_enter_critical_mutex(&pxmitpriv->ack_tx_mutex, NULL);
	pxmitpriv->ack_tx = _TRUE;

	pmgntframe->ack_report = 1;
	if(rtw_hal_mgnt_xmit(padapter, pmgntframe) == _SUCCESS)
	{
		if(rtw_ack_tx_wait(pxmitpriv, timeout_ms))
			ret = _SUCCESS;
	}

	pxmitpriv->ack_tx = _FALSE;
	_exit_critical_mutex(&pxmitpriv->ack_tx_mutex, NULL);

	 return ret;
#else //!CONFIG_XMIT_ACK
	dump_mgntframe(padapter, pmgntframe);
	return _SUCCESS;
#endif //!CONFIG_XMIT_ACK
}

int update_hidden_ssid(u8 *ies, u32 ies_len, u8 hidden_ssid_mode)
{
	u8 *ssid_ie;
	sint ssid_len_ori;
	int len_diff = 0;

	ssid_ie = rtw_get_ie(ies,  WLAN_EID_SSID, &ssid_len_ori, ies_len);

	//DBG_871X("%s hidden_ssid_mode:%u, ssid_ie:%p, ssid_len_ori:%d\n", __FUNCTION__, hidden_ssid_mode, ssid_ie, ssid_len_ori);

	if(ssid_ie && ssid_len_ori>0)
	{
		switch(hidden_ssid_mode)
		{
			case 1:
			{
				u8 *next_ie = ssid_ie + 2 + ssid_len_ori;
				u32 remain_len = 0;

				remain_len = ies_len -(next_ie-ies);

				ssid_ie[1] = 0;
				_rtw_memcpy(ssid_ie+2, next_ie, remain_len);
				len_diff -= ssid_len_ori;

				break;
			}
			case 2:
				_rtw_memset(&ssid_ie[2], 0, ssid_len_ori);
				break;
			default:
				break;
		}
	}

	return len_diff;
}

void issue_beacon(_adapter *padapter)
{
	struct xmit_frame	*pmgntframe;
	struct pkt_attrib	*pattrib;
	unsigned char	*pframe;
	struct rtw_ieee80211_hdr *pwlanhdr;
	unsigned short *fctrl;
	unsigned int	rate_len;
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	_irqL irqL;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
#endif //#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
#endif //CONFIG_P2P


	//DBG_871X("%s\n", __FUNCTION__);

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		DBG_871X("%s, alloc mgnt frame fail\n", __FUNCTION__);
		return;
	}
#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	_enter_critical_bh(&pmlmepriv->bcn_update_lock, &irqL);
#endif //#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);
	pattrib->qsel = 0x10;

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;


	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(cur_network), ETH_ALEN);

	SetSeqNum(pwlanhdr, 0/*pmlmeext->mgnt_seq*/);
	//pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_BEACON);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof (struct rtw_ieee80211_hdr_3addr);

	if( (pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
	{
		//DBG_871X("ie len=%d\n", cur_network->IELength);
#ifdef CONFIG_P2P
		// for P2P : Primary Device Type & Device Name
		u32 wpsielen=0, insert_len=0;
		u8 *wpsie=NULL;
		wpsie = rtw_get_wps_ie(cur_network->IEs+_FIXED_IE_LENGTH_, cur_network->IELength-_FIXED_IE_LENGTH_, NULL, &wpsielen);

		if(rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO) && wpsie && wpsielen>0)
		{
			uint wps_offset, remainder_ielen;
			u8 *premainder_ie, *pframe_wscie;

			wps_offset = (uint)(wpsie - cur_network->IEs);

			premainder_ie = wpsie + wpsielen;

			remainder_ielen = cur_network->IELength - wps_offset - wpsielen;

#ifdef CONFIG_IOCTL_CFG80211
			if(wdev_to_priv(padapter->rtw_wdev)->p2p_enabled)
			{
				if(pmlmepriv->wps_beacon_ie && pmlmepriv->wps_beacon_ie_len>0)
				{
					_rtw_memcpy(pframe, cur_network->IEs, wps_offset);
					pframe += wps_offset;
					pattrib->pktlen += wps_offset;

					_rtw_memcpy(pframe, pmlmepriv->wps_beacon_ie, pmlmepriv->wps_beacon_ie_len);
					pframe += pmlmepriv->wps_beacon_ie_len;
					pattrib->pktlen += pmlmepriv->wps_beacon_ie_len;

					//copy remainder_ie to pframe
					_rtw_memcpy(pframe, premainder_ie, remainder_ielen);
					pframe += remainder_ielen;
					pattrib->pktlen += remainder_ielen;
				}
				else
				{
					_rtw_memcpy(pframe, cur_network->IEs, cur_network->IELength);
					pframe += cur_network->IELength;
					pattrib->pktlen += cur_network->IELength;
				}
			}
			else
#endif //CONFIG_IOCTL_CFG80211
			{
				pframe_wscie = pframe + wps_offset;
				_rtw_memcpy(pframe, cur_network->IEs, wps_offset+wpsielen);
				pframe += (wps_offset + wpsielen);
				pattrib->pktlen += (wps_offset + wpsielen);

				//now pframe is end of wsc ie, insert Primary Device Type & Device Name
				//	Primary Device Type
				//	Type:
				*(u16*) ( pframe + insert_len) = cpu_to_be16( WPS_ATTR_PRIMARY_DEV_TYPE );
				insert_len += 2;

				//	Length:
				*(u16*) ( pframe + insert_len ) = cpu_to_be16( 0x0008 );
				insert_len += 2;

				//	Value:
				//	Category ID
				*(u16*) ( pframe + insert_len ) = cpu_to_be16( WPS_PDT_CID_MULIT_MEDIA );
				insert_len += 2;

				//	OUI
				*(u32*) ( pframe + insert_len ) = cpu_to_be32( WPSOUI );
				insert_len += 4;

				//	Sub Category ID
				*(u16*) ( pframe + insert_len ) = cpu_to_be16( WPS_PDT_SCID_MEDIA_SERVER );
				insert_len += 2;


				//	Device Name
				//	Type:
				*(u16*) ( pframe + insert_len ) = cpu_to_be16( WPS_ATTR_DEVICE_NAME );
				insert_len += 2;

				//	Length:
				*(u16*) ( pframe + insert_len ) = cpu_to_be16( pwdinfo->device_name_len );
				insert_len += 2;

				//	Value:
				_rtw_memcpy( pframe + insert_len, pwdinfo->device_name, pwdinfo->device_name_len );
				insert_len += pwdinfo->device_name_len;


				//update wsc ie length
				*(pframe_wscie+1) = (wpsielen -2) + insert_len;

				//pframe move to end
				pframe+=insert_len;
				pattrib->pktlen += insert_len;

				//copy remainder_ie to pframe
				_rtw_memcpy(pframe, premainder_ie, remainder_ielen);
				pframe += remainder_ielen;
				pattrib->pktlen += remainder_ielen;
			}
		}
		else
#endif //CONFIG_P2P
		{
			int len_diff;
			_rtw_memcpy(pframe, cur_network->IEs, cur_network->IELength);
			len_diff = update_hidden_ssid(
				pframe+_BEACON_IE_OFFSET_
				, cur_network->IELength-_BEACON_IE_OFFSET_
				, pmlmeinfo->hidden_ssid_mode
			);
			pframe += (cur_network->IELength+len_diff);
			pattrib->pktlen += (cur_network->IELength+len_diff);
		}

		{
			u8 *wps_ie;
			uint wps_ielen;
			u8 sr = 0;
			wps_ie = rtw_get_wps_ie(pmgntframe->buf_addr+TXDESC_OFFSET+sizeof (struct rtw_ieee80211_hdr_3addr)+_BEACON_IE_OFFSET_,
				pattrib->pktlen-sizeof (struct rtw_ieee80211_hdr_3addr)-_BEACON_IE_OFFSET_, NULL, &wps_ielen);
			if (wps_ie && wps_ielen>0) {
				rtw_get_wps_attr_content(wps_ie,  wps_ielen, WPS_ATTR_SELECTED_REGISTRAR, (u8*)(&sr), NULL);
			}
			if (sr != 0)
				set_fwstate(pmlmepriv, WIFI_UNDER_WPS);
			else
				_clr_fwstate_(pmlmepriv, WIFI_UNDER_WPS);
		}

#ifdef CONFIG_P2P
		if(rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO))
		{
			u32 len;
#ifdef CONFIG_IOCTL_CFG80211
			if(wdev_to_priv(padapter->rtw_wdev)->p2p_enabled)
			{
				len = pmlmepriv->p2p_beacon_ie_len;
				if(pmlmepriv->p2p_beacon_ie && len>0)
					_rtw_memcpy(pframe, pmlmepriv->p2p_beacon_ie, len);
			}
			else
#endif //CONFIG_IOCTL_CFG80211
			{
				len = build_beacon_p2p_ie(pwdinfo, pframe);
			}

			pframe += len;
			pattrib->pktlen += len;
#ifdef CONFIG_WFD
			len = build_beacon_wfd_ie( pwdinfo, pframe );
			pframe += len;
			pattrib->pktlen += len;
#endif //CONFIG_WFD
		}
#endif //CONFIG_P2P

		goto _issue_bcn;

	}

	//below for ad-hoc mode

	//timestamp will be inserted by hardware
	pframe += 8;
	pattrib->pktlen += 8;

	// beacon interval: 2 bytes

	_rtw_memcpy(pframe, (unsigned char *)(rtw_get_beacon_interval_from_ie(cur_network->IEs)), 2);

	pframe += 2;
	pattrib->pktlen += 2;

	// capability info: 2 bytes

	_rtw_memcpy(pframe, (unsigned char *)(rtw_get_capability_from_ie(cur_network->IEs)), 2);

	pframe += 2;
	pattrib->pktlen += 2;

	// SSID
	pframe = rtw_set_ie(pframe, _SSID_IE_, cur_network->Ssid.SsidLength, cur_network->Ssid.Ssid, &pattrib->pktlen);

	// supported rates...
	rate_len = rtw_get_rateset_len(cur_network->SupportedRates);
	pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_, ((rate_len > 8)? 8: rate_len), cur_network->SupportedRates, &pattrib->pktlen);

	// DS parameter set
	pframe = rtw_set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&(cur_network->Configuration.DSConfig), &pattrib->pktlen);

	//if( (pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE)
	{
		u8 erpinfo=0;
		u32 ATIMWindow;
		// IBSS Parameter Set...
		//ATIMWindow = cur->Configuration.ATIMWindow;
		ATIMWindow = 0;
		pframe = rtw_set_ie(pframe, _IBSS_PARA_IE_, 2, (unsigned char *)(&ATIMWindow), &pattrib->pktlen);

		//ERP IE
		pframe = rtw_set_ie(pframe, _ERPINFO_IE_, 1, &erpinfo, &pattrib->pktlen);
	}


	// EXTERNDED SUPPORTED RATE
	if (rate_len > 8)
	{
		pframe = rtw_set_ie(pframe, _EXT_SUPPORTEDRATES_IE_, (rate_len - 8), (cur_network->SupportedRates + 8), &pattrib->pktlen);
	}


	//todo:HT for adhoc

_issue_bcn:

#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	pmlmepriv->update_bcn = _FALSE;

	_exit_critical_bh(&pmlmepriv->bcn_update_lock, &irqL);
#endif //#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)

	if ((pattrib->pktlen + TXDESC_SIZE) > 512)
	{
		DBG_871X("beacon frame too large\n");
		return;
	}

	pattrib->last_txcmdsz = pattrib->pktlen;

	//DBG_871X("issue bcn_sz=%d\n", pattrib->last_txcmdsz);

	dump_mgntframe(padapter, pmgntframe);

}

void issue_probersp(_adapter *padapter, unsigned char *da, u8 is_valid_p2p_probereq)
{
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	unsigned char					*mac, *bssid;
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	u8 *pwps_ie;
	uint wps_ielen;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
#endif //#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	unsigned int	rate_len;
#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD
#endif //CONFIG_P2P

	//DBG_871X("%s\n", __FUNCTION__);

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		DBG_871X("%s, alloc mgnt frame fail\n", __FUNCTION__);
		return;
	}


	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	mac = myid(&(padapter->eeprompriv));
	bssid = cur_network->MacAddress;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_rtw_memcpy(pwlanhdr->addr1, da, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, bssid, ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(fctrl, WIFI_PROBERSP);

	pattrib->hdrlen = sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = pattrib->hdrlen;
	pframe += pattrib->hdrlen;


	if(cur_network->IELength>MAX_IE_SZ)
		return;

#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	if( (pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
	{
		pwps_ie = rtw_get_wps_ie(cur_network->IEs+_FIXED_IE_LENGTH_, cur_network->IELength-_FIXED_IE_LENGTH_, NULL, &wps_ielen);

		//inerset & update wps_probe_resp_ie
		if((pmlmepriv->wps_probe_resp_ie!=NULL) && pwps_ie && (wps_ielen>0))
		{
			uint wps_offset, remainder_ielen;
			u8 *premainder_ie;

			wps_offset = (uint)(pwps_ie - cur_network->IEs);

			premainder_ie = pwps_ie + wps_ielen;

			remainder_ielen = cur_network->IELength - wps_offset - wps_ielen;

			_rtw_memcpy(pframe, cur_network->IEs, wps_offset);
			pframe += wps_offset;
			pattrib->pktlen += wps_offset;

			wps_ielen = (uint)pmlmepriv->wps_probe_resp_ie[1];//to get ie data len
			if((wps_offset+wps_ielen+2)<=MAX_IE_SZ)
			{
				_rtw_memcpy(pframe, pmlmepriv->wps_probe_resp_ie, wps_ielen+2);
				pframe += wps_ielen+2;
				pattrib->pktlen += wps_ielen+2;
			}

			if((wps_offset+wps_ielen+2+remainder_ielen)<=MAX_IE_SZ)
			{
				_rtw_memcpy(pframe, premainder_ie, remainder_ielen);
				pframe += remainder_ielen;
				pattrib->pktlen += remainder_ielen;
			}
		}
		else
		{
			_rtw_memcpy(pframe, cur_network->IEs, cur_network->IELength);
			pframe += cur_network->IELength;
			pattrib->pktlen += cur_network->IELength;
		}

	}
	else
#endif
	{

		//timestamp will be inserted by hardware
		pframe += 8;
		pattrib->pktlen += 8;

		// beacon interval: 2 bytes

		_rtw_memcpy(pframe, (unsigned char *)(rtw_get_beacon_interval_from_ie(cur_network->IEs)), 2);

		pframe += 2;
		pattrib->pktlen += 2;

		// capability info: 2 bytes

		_rtw_memcpy(pframe, (unsigned char *)(rtw_get_capability_from_ie(cur_network->IEs)), 2);

		pframe += 2;
		pattrib->pktlen += 2;

		//below for ad-hoc mode

		// SSID
		pframe = rtw_set_ie(pframe, _SSID_IE_, cur_network->Ssid.SsidLength, cur_network->Ssid.Ssid, &pattrib->pktlen);

		// supported rates...
		rate_len = rtw_get_rateset_len(cur_network->SupportedRates);
		pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_, ((rate_len > 8)? 8: rate_len), cur_network->SupportedRates, &pattrib->pktlen);

		// DS parameter set
		pframe =rtw_set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&(cur_network->Configuration.DSConfig), &pattrib->pktlen);

		if( (pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE)
		{
			u8 erpinfo=0;
			u32 ATIMWindow;
			// IBSS Parameter Set...
			//ATIMWindow = cur->Configuration.ATIMWindow;
			ATIMWindow = 0;
			pframe = rtw_set_ie(pframe, _IBSS_PARA_IE_, 2, (unsigned char *)(&ATIMWindow), &pattrib->pktlen);

			//ERP IE
			pframe = rtw_set_ie(pframe, _ERPINFO_IE_, 1, &erpinfo, &pattrib->pktlen);
		}


		// EXTERNDED SUPPORTED RATE
		if (rate_len > 8)
		{
			pframe = rtw_set_ie(pframe, _EXT_SUPPORTEDRATES_IE_, (rate_len - 8), (cur_network->SupportedRates + 8), &pattrib->pktlen);
		}


		//todo:HT for adhoc

	}

#ifdef CONFIG_P2P
	if(rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO) && is_valid_p2p_probereq)
	{
		u32 len;
#ifdef CONFIG_IOCTL_CFG80211
		if(wdev_to_priv(padapter->rtw_wdev)->p2p_enabled)
		{
			//if pwdinfo->role == P2P_ROLE_DEVICE will call issue_probersp_p2p()
			len = pmlmepriv->p2p_go_probe_resp_ie_len;
			if(pmlmepriv->p2p_go_probe_resp_ie && len>0)
				_rtw_memcpy(pframe, pmlmepriv->p2p_go_probe_resp_ie, len);
		}
		else
#endif //CONFIG_IOCTL_CFG80211
		{
			len = build_probe_resp_p2p_ie(pwdinfo, pframe);
		}

		pframe += len;
		pattrib->pktlen += len;

#ifdef CONFIG_WFD
	wfdielen = build_probe_resp_wfd_ie(pwdinfo, pframe, 0);
	pframe += wfdielen;
	pattrib->pktlen += wfdielen;
#endif //CONFIG_WFD

	}
#endif //CONFIG_P2P


	pattrib->last_txcmdsz = pattrib->pktlen;


	dump_mgntframe(padapter, pmgntframe);

	return;

}

void issue_probereq(_adapter *padapter, NDIS_802_11_SSID *pssid, u8 blnbc)
{
	struct xmit_frame		*pmgntframe;
	struct pkt_attrib		*pattrib;
	unsigned char			*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short		*fctrl;
	unsigned char			*mac;
	unsigned char			bssrate[NumRates];
	struct xmit_priv		*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	int	bssrate_len = 0;
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	RT_TRACE(_module_rtl871x_mlme_c_,_drv_notice_,("+issue_probereq\n"));

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);


	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	mac = myid(&(padapter->eeprompriv));

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	if ( 0 == blnbc )
	{
		//	unicast probe request frame
		_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
	}
	else
	{
		//	broadcast probe request frame
		_rtw_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, bc_addr, ETH_ALEN);
	}

	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_PROBEREQ);

	pframe += sizeof (struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof (struct rtw_ieee80211_hdr_3addr);

	if(pssid)
		pframe = rtw_set_ie(pframe, _SSID_IE_, pssid->SsidLength, pssid->Ssid, &(pattrib->pktlen));
	else
		pframe = rtw_set_ie(pframe, _SSID_IE_, 0, NULL, &(pattrib->pktlen));

	get_rate_set(padapter, bssrate, &bssrate_len);

	if (bssrate_len > 8)
	{
		pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_ , 8, bssrate, &(pattrib->pktlen));
		pframe = rtw_set_ie(pframe, _EXT_SUPPORTEDRATES_IE_ , (bssrate_len - 8), (bssrate + 8), &(pattrib->pktlen));
	}
	else
	{
		pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_ , bssrate_len , bssrate, &(pattrib->pktlen));
	}

#if 0
	//add wps_ie for wps2.0
	if(pmlmepriv->probereq_wpsie_len>0 && pmlmepriv->probereq_wpsie_len<MAX_WPS_IE_LEN)
	{
		_rtw_memcpy(pframe, pmlmepriv->probereq_wpsie, pmlmepriv->probereq_wpsie_len);
		pframe += pmlmepriv->probereq_wpsie_len;
		pattrib->pktlen += pmlmepriv->probereq_wpsie_len;
		//pmlmepriv->probereq_wpsie_len = 0 ;//reset to zero
	}
#else
	//add wps_ie for wps2.0
	if(pmlmepriv->wps_probe_req_ie_len>0 && pmlmepriv->wps_probe_req_ie)
	{
		_rtw_memcpy(pframe, pmlmepriv->wps_probe_req_ie, pmlmepriv->wps_probe_req_ie_len);
		pframe += pmlmepriv->wps_probe_req_ie_len;
		pattrib->pktlen += pmlmepriv->wps_probe_req_ie_len;
		//pmlmepriv->wps_probe_req_ie_len = 0 ;//reset to zero
	}
#endif

	pattrib->last_txcmdsz = pattrib->pktlen;

	RT_TRACE(_module_rtl871x_mlme_c_,_drv_notice_,("issuing probe_req, tx_len=%d\n", pattrib->last_txcmdsz));

	dump_mgntframe(padapter, pmgntframe);

	return;
}

// if psta == NULL, indiate we are station(client) now...
void issue_auth(_adapter *padapter, struct sta_info *psta, unsigned short status)
{
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	unsigned int					val32;
	unsigned short				val16;
	int use_shared_key = 0;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

/*
	if( (psta == NULL) && (pmlmeinfo->auth_seq != 3) ) {
		// Because of AP's not receiving deauth before
		// AP may: 1)not response auth or 2)deauth us after link is complete
		// issue deauth before issuing auth to deal with the situation
		issue_deauth(padapter, (&(pmlmeinfo->network))->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
	}
*/

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_AUTH);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);


	if(psta)// for AP mode
	{
#ifdef CONFIG_NATIVEAP_MLME

		_rtw_memcpy(pwlanhdr->addr1, psta->hwaddr, ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, myid(&(padapter->eeprompriv)), ETH_ALEN);


		// setting auth algo number
		val16 = (u16)psta->authalg;

		if(status != _STATS_SUCCESSFUL_)
			val16 = 0;

		if (val16)	{
			val16 = cpu_to_le16(val16);
			use_shared_key = 1;
		}

		pframe = rtw_set_fixed_ie(pframe, _AUTH_ALGM_NUM_, (unsigned char *)&val16, &(pattrib->pktlen));

		// setting auth seq number
		val16 =(u16)psta->auth_seq;
		val16 = cpu_to_le16(val16);
		pframe = rtw_set_fixed_ie(pframe, _AUTH_SEQ_NUM_, (unsigned char *)&val16, &(pattrib->pktlen));

		// setting status code...
		val16 = status;
		val16 = cpu_to_le16(val16);
		pframe = rtw_set_fixed_ie(pframe, _STATUS_CODE_, (unsigned char *)&val16, &(pattrib->pktlen));

		// added challenging text...
		if ((psta->auth_seq == 2) && (psta->state & WIFI_FW_AUTH_STATE) && (use_shared_key==1))
		{
			pframe = rtw_set_ie(pframe, _CHLGETXT_IE_, 128, psta->chg_txt, &(pattrib->pktlen));
		}
#endif
	}
	else
	{
		_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&pmlmeinfo->network), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr2, myid(&padapter->eeprompriv), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&pmlmeinfo->network), ETH_ALEN);

		// setting auth algo number
		val16 = (pmlmeinfo->auth_algo == dot11AuthAlgrthm_Shared)? 1: 0;// 0:OPEN System, 1:Shared key
		if (val16)	{
			val16 = cpu_to_le16(val16);
			use_shared_key = 1;
		}
		//DBG_871X("%s auth_algo= %s auth_seq=%d\n",__FUNCTION__,(pmlmeinfo->auth_algo==0)?"OPEN":"SHARED",pmlmeinfo->auth_seq);

		//setting IV for auth seq #3
		if ((pmlmeinfo->auth_seq == 3) && (pmlmeinfo->state & WIFI_FW_AUTH_STATE) && (use_shared_key==1))
		{
			//DBG_871X("==> iv(%d),key_index(%d)\n",pmlmeinfo->iv,pmlmeinfo->key_index);
			val32 = ((pmlmeinfo->iv++) | (pmlmeinfo->key_index << 30));
			val32 = cpu_to_le32(val32);
			pframe = rtw_set_fixed_ie(pframe, 4, (unsigned char *)&val32, &(pattrib->pktlen));

			pattrib->iv_len = 4;
		}

		pframe = rtw_set_fixed_ie(pframe, _AUTH_ALGM_NUM_, (unsigned char *)&val16, &(pattrib->pktlen));

		// setting auth seq number
		val16 = pmlmeinfo->auth_seq;
		val16 = cpu_to_le16(val16);
		pframe = rtw_set_fixed_ie(pframe, _AUTH_SEQ_NUM_, (unsigned char *)&val16, &(pattrib->pktlen));


		// setting status code...
		val16 = status;
		val16 = cpu_to_le16(val16);
		pframe = rtw_set_fixed_ie(pframe, _STATUS_CODE_, (unsigned char *)&val16, &(pattrib->pktlen));

		// then checking to see if sending challenging text...
		if ((pmlmeinfo->auth_seq == 3) && (pmlmeinfo->state & WIFI_FW_AUTH_STATE) && (use_shared_key==1))
		{
			pframe = rtw_set_ie(pframe, _CHLGETXT_IE_, 128, pmlmeinfo->chg_txt, &(pattrib->pktlen));

			SetPrivacy(fctrl);

			pattrib->hdrlen = sizeof(struct rtw_ieee80211_hdr_3addr);

			pattrib->encrypt = _WEP40_;

			pattrib->icv_len = 4;

			pattrib->pktlen += pattrib->icv_len;

		}

	}

	pattrib->last_txcmdsz = pattrib->pktlen;

	rtw_wep_encrypt(padapter, (u8 *)pmgntframe);
	DBG_871X("%s\n", __FUNCTION__);
	/* dont scan under linking state */
	adapter_to_pwrctl(padapter)->DelayLPSLastTimeStamp = rtw_get_current_time();
	dump_mgntframe(padapter, pmgntframe);

	return;
}


void issue_asocrsp(_adapter *padapter, unsigned short status, struct sta_info *pstat, int pkt_type)
{
#ifdef CONFIG_AP_MODE
	struct xmit_frame	*pmgntframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	struct pkt_attrib *pattrib;
	unsigned char	*pbuf, *pframe;
	unsigned short val;
	unsigned short *fctrl;
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX *pnetwork = &(pmlmeinfo->network);
	u8 *ie = pnetwork->IEs;
#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD

#endif //CONFIG_P2P

	DBG_871X("%s\n", __FUNCTION__);

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);


	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy((void *)GetAddr1Ptr(pwlanhdr), pstat->hwaddr, ETH_ALEN);
	_rtw_memcpy((void *)GetAddr2Ptr(pwlanhdr), myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy((void *)GetAddr3Ptr(pwlanhdr), get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);


	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	if ((pkt_type == WIFI_ASSOCRSP) || (pkt_type == WIFI_REASSOCRSP))
		SetFrameSubType(pwlanhdr, pkt_type);
	else
		return;

	pattrib->hdrlen = sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen += pattrib->hdrlen;
	pframe += pattrib->hdrlen;

	//capability
	val = *(unsigned short *)rtw_get_capability_from_ie(ie);

	pframe = rtw_set_fixed_ie(pframe, _CAPABILITY_ , (unsigned char *)&val, &(pattrib->pktlen));

	status = cpu_to_le16(status);
	pframe = rtw_set_fixed_ie(pframe , _STATUS_CODE_ , (unsigned char *)&status, &(pattrib->pktlen));

	val = cpu_to_le16(pstat->aid | BIT(14) | BIT(15));
	pframe = rtw_set_fixed_ie(pframe, _ASOC_ID_ , (unsigned char *)&val, &(pattrib->pktlen));

	if (pstat->bssratelen <= 8)
	{
		pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_, pstat->bssratelen, pstat->bssrateset, &(pattrib->pktlen));
	}
	else
	{
		pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_, 8, pstat->bssrateset, &(pattrib->pktlen));
		pframe = rtw_set_ie(pframe, _EXT_SUPPORTEDRATES_IE_, (pstat->bssratelen-8), pstat->bssrateset+8, &(pattrib->pktlen));
	}

#ifdef CONFIG_80211N_HT
	if ((pstat->flags & WLAN_STA_HT) && (pmlmepriv->htpriv.ht_option))
	{
		uint ie_len=0;

		//FILL HT CAP INFO IE
		//p = hostapd_eid_ht_capabilities_info(hapd, p);
		pbuf = rtw_get_ie(ie + _BEACON_IE_OFFSET_, _HT_CAPABILITY_IE_, &ie_len, (pnetwork->IELength - _BEACON_IE_OFFSET_));
		if(pbuf && ie_len>0)
		{
			_rtw_memcpy(pframe, pbuf, ie_len+2);
			pframe += (ie_len+2);
			pattrib->pktlen +=(ie_len+2);
		}

		//FILL HT ADD INFO IE
		//p = hostapd_eid_ht_operation(hapd, p);
		pbuf = rtw_get_ie(ie + _BEACON_IE_OFFSET_, _HT_ADD_INFO_IE_, &ie_len, (pnetwork->IELength - _BEACON_IE_OFFSET_));
		if(pbuf && ie_len>0)
		{
			_rtw_memcpy(pframe, pbuf, ie_len+2);
			pframe += (ie_len+2);
			pattrib->pktlen +=(ie_len+2);
		}

	}
#endif

	//FILL WMM IE
	if ((pstat->flags & WLAN_STA_WME) && (pmlmepriv->qospriv.qos_option))
	{
		uint ie_len=0;
		unsigned char WMM_PARA_IE[] = {0x00, 0x50, 0xf2, 0x02, 0x01, 0x01};

		for (pbuf = ie + _BEACON_IE_OFFSET_; ;pbuf+= (ie_len + 2))
		{
			pbuf = rtw_get_ie(pbuf, _VENDOR_SPECIFIC_IE_, &ie_len, (pnetwork->IELength - _BEACON_IE_OFFSET_ - (ie_len + 2)));
			if(pbuf && _rtw_memcmp(pbuf+2, WMM_PARA_IE, 6))
			{
				_rtw_memcpy(pframe, pbuf, ie_len+2);
				pframe += (ie_len+2);
				pattrib->pktlen +=(ie_len+2);

				break;
			}

			if ((pbuf == NULL) || (ie_len == 0))
			{
				break;
			}
		}

	}


	if (pmlmeinfo->assoc_AP_vendor == HT_IOT_PEER_REALTEK)
	{
		pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, 6 , REALTEK_96B_IE, &(pattrib->pktlen));
	}

	//add WPS IE ie for wps 2.0
	if(pmlmepriv->wps_assoc_resp_ie && pmlmepriv->wps_assoc_resp_ie_len>0)
	{
		_rtw_memcpy(pframe, pmlmepriv->wps_assoc_resp_ie, pmlmepriv->wps_assoc_resp_ie_len);

		pframe += pmlmepriv->wps_assoc_resp_ie_len;
		pattrib->pktlen += pmlmepriv->wps_assoc_resp_ie_len;
	}

#ifdef CONFIG_P2P
	if(rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO) && (pstat->is_p2p_device == _TRUE))
	{
		u32 len;

		len = build_assoc_resp_p2p_ie(pwdinfo, pframe, pstat->p2p_status_code);

		pframe += len;
		pattrib->pktlen += len;
	}
#ifdef CONFIG_WFD
	if(rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO))
	{
		wfdielen = build_assoc_resp_wfd_ie(pwdinfo, pframe);
		pframe += wfdielen;
		pattrib->pktlen += wfdielen;
	}
#endif //CONFIG_WFD

#endif //CONFIG_P2P

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

#endif
}

void issue_assocreq(_adapter *padapter)
{
	struct xmit_frame				*pmgntframe;
	struct pkt_attrib				*pattrib;
	unsigned char					*pframe, *p;
	struct rtw_ieee80211_hdr			*pwlanhdr;
	unsigned short				*fctrl;
	unsigned short				val16;
	unsigned int					i, j, ie_len, index=0;
	unsigned char					rf_type, bssrate[NumRates], sta_bssrate[NumRates];
	PNDIS_802_11_VARIABLE_IEs	pIE;
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	struct xmit_priv		*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	int	bssrate_len = 0, sta_bssrate_len = 0;
#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &(padapter->wdinfo);
	u8					p2pie[ 255 ] = { 0x00 };
	u16					p2pielen = 0;
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif //CONFIG_WFD

#endif //CONFIG_P2P

#ifdef CONFIG_DFS
	u16	cap;
#endif //CONFIG_DFS

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);


	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ASSOCREQ);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	//caps

#ifdef CONFIG_DFS
	_rtw_memcpy(&cap, rtw_get_capability_from_ie(pmlmeinfo->network.IEs), 2);
	cap |= BIT(8);
	_rtw_memcpy(pframe, &cap, 2);
#else
	_rtw_memcpy(pframe, rtw_get_capability_from_ie(pmlmeinfo->network.IEs), 2);
#endif //CONFIG_DFS

	pframe += 2;
	pattrib->pktlen += 2;

	//listen interval
	//todo: listen interval for power saving
	val16 = cpu_to_le16(3);
	_rtw_memcpy(pframe ,(unsigned char *)&val16, 2);
	pframe += 2;
	pattrib->pktlen += 2;

	//SSID
	pframe = rtw_set_ie(pframe, _SSID_IE_,  pmlmeinfo->network.Ssid.SsidLength, pmlmeinfo->network.Ssid.Ssid, &(pattrib->pktlen));

	//supported rate & extended supported rate

#if 1	// Check if the AP's supported rates are also supported by STA.
	get_rate_set(padapter, sta_bssrate, &sta_bssrate_len);
	//DBG_871X("sta_bssrate_len=%d\n", sta_bssrate_len);

	if(pmlmeext->cur_channel == 14)// for JAPAN, channel 14 can only uses B Mode(CCK)
	{
		sta_bssrate_len = 4;
	}


	//for (i = 0; i < sta_bssrate_len; i++) {
	//	DBG_871X("sta_bssrate[%d]=%02X\n", i, sta_bssrate[i]);
	//}

	for (i = 0; i < NDIS_802_11_LENGTH_RATES_EX; i++) {
		if (pmlmeinfo->network.SupportedRates[i] == 0) break;
		DBG_871X("network.SupportedRates[%d]=%02X\n", i, pmlmeinfo->network.SupportedRates[i]);
	}


	for (i = 0; i < NDIS_802_11_LENGTH_RATES_EX; i++) {
		if (pmlmeinfo->network.SupportedRates[i] == 0) break;


		// Check if the AP's supported rates are also supported by STA.
		for (j=0; j < sta_bssrate_len; j++) {
			 // Avoid the proprietary data rate (22Mbps) of Handlink WSG-4000 AP
			if ( (pmlmeinfo->network.SupportedRates[i]|IEEE80211_BASIC_RATE_MASK)
					== (sta_bssrate[j]|IEEE80211_BASIC_RATE_MASK)) {
				//DBG_871X("match i = %d, j=%d\n", i, j);
				break;
			} else {
				//DBG_871X("not match: %02X != %02X\n", (pmlmeinfo->network.SupportedRates[i]|IEEE80211_BASIC_RATE_MASK), (sta_bssrate[j]|IEEE80211_BASIC_RATE_MASK));
			}
		}

		if (j == sta_bssrate_len) {
			// the rate is not supported by STA
			DBG_871X("%s(): the rate[%d]=%02X is not supported by STA!\n",__FUNCTION__, i, pmlmeinfo->network.SupportedRates[i]);
		} else {
			// the rate is supported by STA
			bssrate[index++] = pmlmeinfo->network.SupportedRates[i];
		}
	}

	bssrate_len = index;
	DBG_871X("bssrate_len = %d\n", bssrate_len);

#else	// Check if the AP's supported rates are also supported by STA.
#if 0
	get_rate_set(padapter, bssrate, &bssrate_len);
#else
	for (bssrate_len = 0; bssrate_len < NumRates; bssrate_len++) {
		if (pmlmeinfo->network.SupportedRates[bssrate_len] == 0) break;

		if (pmlmeinfo->network.SupportedRates[bssrate_len] == 0x2C) // Avoid the proprietary data rate (22Mbps) of Handlink WSG-4000 AP
			break;

		bssrate[bssrate_len] = pmlmeinfo->network.SupportedRates[bssrate_len];
	}
#endif
#endif	// Check if the AP's supported rates are also supported by STA.

	if(bssrate_len == 0)
	{
		rtw_free_xmitbuf(pxmitpriv, pmgntframe->pxmitbuf);

		rtw_free_xmitframe(pxmitpriv, pmgntframe);

		return; //don't connect to AP if no joint supported rate
	}


	if (bssrate_len > 8)
	{
		pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_ , 8, bssrate, &(pattrib->pktlen));
		pframe = rtw_set_ie(pframe, _EXT_SUPPORTEDRATES_IE_ , (bssrate_len - 8), (bssrate + 8), &(pattrib->pktlen));
	}
	else
	{
		pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_ , bssrate_len , bssrate, &(pattrib->pktlen));
	}

	//RSN
	p = rtw_get_ie((pmlmeinfo->network.IEs + sizeof(NDIS_802_11_FIXED_IEs)), _RSN_IE_2_, &ie_len, (pmlmeinfo->network.IELength - sizeof(NDIS_802_11_FIXED_IEs)));
	if (p != NULL)
	{
		pframe = rtw_set_ie(pframe, _RSN_IE_2_, ie_len, (p + 2), &(pattrib->pktlen));
	}

#ifdef CONFIG_80211N_HT
	//HT caps
	if(padapter->mlmepriv.htpriv.ht_option==_TRUE)
	{
		p = rtw_get_ie((pmlmeinfo->network.IEs + sizeof(NDIS_802_11_FIXED_IEs)), _HT_CAPABILITY_IE_, &ie_len, (pmlmeinfo->network.IELength - sizeof(NDIS_802_11_FIXED_IEs)));
		if ((p != NULL) && (!(is_ap_in_tkip(padapter))) && (!(is_ap_in_wep(padapter))))
		{
			_rtw_memcpy(&(pmlmeinfo->HT_caps), (p + 2), sizeof(struct HT_caps_element));

			//to disable 40M Hz support while gd_bw_40MHz_en = 0
			if (pregpriv->cbw40_enable == 0)
			{
				pmlmeinfo->HT_caps.u.HT_cap_element.HT_caps_info &= (~(BIT(6) | BIT(1)));
			}
			else
			{
				pmlmeinfo->HT_caps.u.HT_cap_element.HT_caps_info |= BIT(1);
			}

			//todo: disable SM power save mode
			pmlmeinfo->HT_caps.u.HT_cap_element.HT_caps_info |= 0x000c;

			rtw_hal_get_hwreg(padapter, HW_VAR_RF_TYPE, (u8 *)(&rf_type));
			//switch (pregpriv->rf_config)
			switch(rf_type)
			{
				case RF_1T1R:

					if(pregpriv->rx_stbc)
						pmlmeinfo->HT_caps.u.HT_cap_element.HT_caps_info |= cpu_to_le16(0x0100);//RX STBC One spatial stream

					_rtw_memcpy(pmlmeinfo->HT_caps.u.HT_cap_element.MCS_rate, MCS_rate_1R, 16);
					break;

				case RF_2T2R:
				case RF_1T2R:
				default:

					if((pregpriv->rx_stbc == 0x3) ||//enable for 2.4/5 GHz
						((pmlmeext->cur_wireless_mode & WIRELESS_11_24N) && (pregpriv->rx_stbc == 0x1)) || //enable for 2.4GHz
						((pmlmeext->cur_wireless_mode & WIRELESS_11_5N) && (pregpriv->rx_stbc == 0x2))) //enable for 5GHz
					{
						DBG_871X("declare supporting RX STBC\n");
						pmlmeinfo->HT_caps.u.HT_cap_element.HT_caps_info |= cpu_to_le16(0x0200);//RX STBC two spatial stream
					}

					_rtw_memcpy(pmlmeinfo->HT_caps.u.HT_cap_element.MCS_rate, MCS_rate_2R, 16);
					break;
			}
#ifdef RTL8192C_RECONFIG_TO_1T1R
			{
				if(pregpriv->rx_stbc)
					pmlmeinfo->HT_caps.u.HT_cap_element.HT_caps_info |= cpu_to_le16(0x0100);//RX STBC One spatial stream

				_rtw_memcpy(pmlmeinfo->HT_caps.u.HT_cap_element.MCS_rate, MCS_rate_1R, 16);
			}
#endif
			pmlmeinfo->HT_caps.u.HT_cap_element.HT_caps_info = cpu_to_le16(pmlmeinfo->HT_caps.u.HT_cap_element.HT_caps_info);

#ifdef CONFIG_BT_COEXIST
			if (BT_1Ant(padapter) == _TRUE)
			{
				// set to 8K
				pmlmeinfo->HT_caps.u.HT_cap_element.AMPDU_para &= (u8)~IEEE80211_HT_CAP_AMPDU_FACTOR;
//				pmlmeinfo->HT_caps.u.HT_cap_element.AMPDU_para |= MAX_AMPDU_FACTOR_8K;
			}
#endif
#ifdef CONFIG_DONT_CARE_TP
			/* set Max AMPDU to 8K */
			pmlmeinfo->HT_caps.u.HT_cap_element.AMPDU_para &= (u8)~IEEE80211_HT_CAP_AMPDU_FACTOR;
			/* set AMSDU to 3839 bytes */
			pmlmeinfo->HT_caps.u.HT_cap_element.HT_caps_info &= ~IEEE80211_HT_CAP_MAX_AMSDU;
#endif
			pframe = rtw_set_ie(pframe, _HT_CAPABILITY_IE_, ie_len , (u8 *)(&(pmlmeinfo->HT_caps)), &(pattrib->pktlen));

		}
	}
#endif

	//vendor specific IE, such as WPA, WMM, WPS
	for (i = sizeof(NDIS_802_11_FIXED_IEs); i < pmlmeinfo->network.IELength;)
	{
		pIE = (PNDIS_802_11_VARIABLE_IEs)(pmlmeinfo->network.IEs + i);

		switch (pIE->ElementID)
		{
			case _VENDOR_SPECIFIC_IE_:
				if ((_rtw_memcmp(pIE->data, RTW_WPA_OUI, 4)) ||
						(_rtw_memcmp(pIE->data, WMM_OUI, 4)) ||
						(_rtw_memcmp(pIE->data, WPS_OUI, 4)))
				{

					//Commented by Kurt 20110629
					//In some older APs, WPS handshake
					//would be fail if we append vender extensions informations to AP
					if(_rtw_memcmp(pIE->data, WPS_OUI, 4)){
						pIE->Length=14;
					}

					pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, pIE->Length, pIE->data, &(pattrib->pktlen));
				}
				break;

			default:
				break;
		}

		i += (pIE->Length + 2);
	}

	if (pmlmeinfo->assoc_AP_vendor == HT_IOT_PEER_REALTEK)
	{
		pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, 6 , REALTEK_96B_IE, &(pattrib->pktlen));
	}

#ifdef CONFIG_P2P

#ifdef CONFIG_IOCTL_CFG80211
	if(wdev_to_priv(padapter->rtw_wdev)->p2p_enabled)
	{
		if(pmlmepriv->p2p_assoc_req_ie && pmlmepriv->p2p_assoc_req_ie_len>0)
		{
			_rtw_memcpy(pframe, pmlmepriv->p2p_assoc_req_ie, pmlmepriv->p2p_assoc_req_ie_len);
			pframe += pmlmepriv->p2p_assoc_req_ie_len;
			pattrib->pktlen += pmlmepriv->p2p_assoc_req_ie_len;
		}
	}
	else
#endif //CONFIG_IOCTL_CFG80211
	{
		if(!rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE) && !rtw_p2p_chk_state(pwdinfo, P2P_STATE_IDLE))
		{
			//	Should add the P2P IE in the association request frame.
			//	P2P OUI

			p2pielen = 0;
			p2pie[ p2pielen++ ] = 0x50;
			p2pie[ p2pielen++ ] = 0x6F;
			p2pie[ p2pielen++ ] = 0x9A;
			p2pie[ p2pielen++ ] = 0x09;	//	WFA P2P v1.0

			//	Commented by Albert 20101109
			//	According to the P2P Specification, the association request frame should contain 3 P2P attributes
			//	1. P2P Capability
			//	2. Extended Listen Timing
			//	3. Device Info
			//	Commented by Albert 20110516
			//	4. P2P Interface

			//	P2P Capability
			//	Type:
			p2pie[ p2pielen++ ] = P2P_ATTR_CAPABILITY;

			//	Length:
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0002 );
			p2pielen += 2;

			//	Value:
			//	Device Capability Bitmap, 1 byte
			p2pie[ p2pielen++ ] = DMP_P2P_DEVCAP_SUPPORT;

			//	Group Capability Bitmap, 1 byte
			if ( pwdinfo->persistent_supported )
				p2pie[ p2pielen++ ] = P2P_GRPCAP_PERSISTENT_GROUP | DMP_P2P_GRPCAP_SUPPORT;
			else
				p2pie[ p2pielen++ ] = DMP_P2P_GRPCAP_SUPPORT;

			//	Extended Listen Timing
			//	Type:
			p2pie[ p2pielen++ ] = P2P_ATTR_EX_LISTEN_TIMING;

			//	Length:
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x0004 );
			p2pielen += 2;

			//	Value:
			//	Availability Period
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0xFFFF );
			p2pielen += 2;

			//	Availability Interval
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0xFFFF );
			p2pielen += 2;

			//	Device Info
			//	Type:
			p2pie[ p2pielen++ ] = P2P_ATTR_DEVICE_INFO;

			//	Length:
			//	21 -> P2P Device Address (6bytes) + Config Methods (2bytes) + Primary Device Type (8bytes)
			//	+ NumofSecondDevType (1byte) + WPS Device Name ID field (2bytes) + WPS Device Name Len field (2bytes)
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 21 + pwdinfo->device_name_len );
			p2pielen += 2;

			//	Value:
			//	P2P Device Address
			_rtw_memcpy( p2pie + p2pielen, myid( &padapter->eeprompriv ), ETH_ALEN );
			p2pielen += ETH_ALEN;

			//	Config Method
			//	This field should be big endian. Noted by P2P specification.
			if ( ( pwdinfo->ui_got_wps_info == P2P_GOT_WPSINFO_PEER_DISPLAY_PIN ) ||
				( pwdinfo->ui_got_wps_info == P2P_GOT_WPSINFO_SELF_DISPLAY_PIN ) )
			{
				*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_CONFIG_METHOD_DISPLAY );
			}
			else
			{
				*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_CONFIG_METHOD_PBC );
			}

			p2pielen += 2;

			//	Primary Device Type
			//	Category ID
			*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_PDT_CID_MULIT_MEDIA );
			p2pielen += 2;

			//	OUI
			*(u32*) ( p2pie + p2pielen ) = cpu_to_be32( WPSOUI );
			p2pielen += 4;

			//	Sub Category ID
			*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_PDT_SCID_MEDIA_SERVER );
			p2pielen += 2;

			//	Number of Secondary Device Types
			p2pie[ p2pielen++ ] = 0x00;	//	No Secondary Device Type List

			//	Device Name
			//	Type:
			*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( WPS_ATTR_DEVICE_NAME );
			p2pielen += 2;

			//	Length:
			*(u16*) ( p2pie + p2pielen ) = cpu_to_be16( pwdinfo->device_name_len );
			p2pielen += 2;

			//	Value:
			_rtw_memcpy( p2pie + p2pielen, pwdinfo->device_name, pwdinfo->device_name_len );
			p2pielen += pwdinfo->device_name_len;

			//	P2P Interface
			//	Type:
			p2pie[ p2pielen++ ] = P2P_ATTR_INTERFACE;

			//	Length:
			*(u16*) ( p2pie + p2pielen ) = cpu_to_le16( 0x000D );
			p2pielen += 2;

			//	Value:
			_rtw_memcpy( p2pie + p2pielen, pwdinfo->device_addr, ETH_ALEN );	//	P2P Device Address
			p2pielen += ETH_ALEN;

			p2pie[ p2pielen++ ] = 1;	//	P2P Interface Address Count

			_rtw_memcpy( p2pie + p2pielen, pwdinfo->device_addr, ETH_ALEN );	//	P2P Interface Address List
			p2pielen += ETH_ALEN;

			pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, p2pielen, (unsigned char *) p2pie, &pattrib->pktlen );

#ifdef CONFIG_WFD
			wfdielen = build_assoc_req_wfd_ie(pwdinfo, pframe);
			pframe += wfdielen;
			pattrib->pktlen += wfdielen;
#endif //CONFIG_WFD
		}
	}

#endif //CONFIG_P2P

#ifdef CONFIG_WAPI_SUPPORT
	rtw_build_assoc_req_wapi_ie(padapter, pframe, pattrib);
#endif

	pattrib->last_txcmdsz = pattrib->pktlen;
	dump_mgntframe(padapter, pmgntframe);

	return;
}

void issue_nulldata(_adapter *padapter, unsigned int power_mode)
{
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	struct xmit_priv	*pxmitpriv;
	struct mlme_ext_priv	*pmlmeext;
	struct mlme_ext_info	*pmlmeinfo;

	//DBG_871X("%s:%d\n", __FUNCTION__, power_mode);

	if(!padapter)
		return;

	pxmitpriv = &(padapter->xmitpriv);
	pmlmeext = &(padapter->mlmeextpriv);
	pmlmeinfo = &(pmlmeext->mlmext_info);

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);
	pattrib->retry_ctrl = _FALSE;

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	if((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
	{
		SetFrDs(fctrl);
	}
	else if((pmlmeinfo->state&0x03) == WIFI_FW_STATION_STATE)
	{
		SetToDs(fctrl);
	}

	if (power_mode)
	{
		SetPwrMgt(fctrl);
	}

	_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_DATA_NULL);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pattrib->last_txcmdsz = pattrib->pktlen;
	dump_mgntframe(padapter, pmgntframe);

	return;
}


void issue_qos_nulldata(_adapter *padapter, unsigned char *da, u16 tid)
{
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl, *qc;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	DBG_871X("%s\n", __FUNCTION__);

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	pattrib->hdrlen +=2;
	pattrib->qos_en = _TRUE;
	pattrib->eosp = 1;
	pattrib->ack_policy = 0;
	pattrib->mdata = 0;

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	if((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
	{
		SetFrDs(fctrl);
	}
	else if((pmlmeinfo->state&0x03) == WIFI_FW_STATION_STATE)
	{
		SetToDs(fctrl);
	}

	if(pattrib->mdata)
		SetMData(fctrl);

	qc = (unsigned short *)(pframe + pattrib->hdrlen - 2);

	SetPriority(qc, tid);

	SetEOSP(qc, pattrib->eosp);

	SetAckpolicy(qc, pattrib->ack_policy);

	_rtw_memcpy(pwlanhdr->addr1, da, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_QOS_DATA_NULL);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr_qos);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr_qos);

	pattrib->last_txcmdsz = pattrib->pktlen;
	dump_mgntframe(padapter, pmgntframe);

}

void issue_deauth(_adapter *padapter, unsigned char *da, unsigned short reason)
{
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	DBG_871X("%s to "MAC_FMT"\n", __func__, MAC_ARG(da));

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

 	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);
	pattrib->retry_ctrl = _FALSE;

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, da, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_DEAUTH);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	reason = cpu_to_le16(reason);
	pframe = rtw_set_fixed_ie(pframe, _RSON_CODE_ , (unsigned char *)&reason, &(pattrib->pktlen));

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);
}

void issue_action_BA(_adapter *padapter, unsigned char *raddr, unsigned char action, unsigned short status)
{
	u8	category = RTW_WLAN_CATEGORY_BACK;
	u16	start_seq;
	u16	BA_para_set;
	u16	reason_code;
	u16	BA_timeout_value;
	u16	BA_starting_seqctrl;
	HT_CAP_AMPDU_FACTOR max_rx_ampdu_factor;
	struct xmit_frame		*pmgntframe;
	struct pkt_attrib		*pattrib;
	u8					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	struct xmit_priv		*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct sta_info		*psta;
	struct sta_priv		*pstapriv = &padapter->stapriv;
	struct registry_priv	 	*pregpriv = &padapter->registrypriv;
#ifdef CONFIG_BT_COEXIST
	u8 tendaAPMac[] = {0xC8, 0x3A, 0x35};
#endif


	DBG_871X("%s, category=%d, action=%d, status=%d\n", __FUNCTION__, category, action, status);

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	//_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr1, raddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));

	status = cpu_to_le16(status);


	if (category == 3)
	{
		switch (action)
		{
			case 0: //ADDBA req
				do {
					pmlmeinfo->dialogToken++;
				} while (pmlmeinfo->dialogToken == 0);
				pframe = rtw_set_fixed_ie(pframe, 1, &(pmlmeinfo->dialogToken), &(pattrib->pktlen));

#ifdef CONFIG_BT_COEXIST
				if ((BT_1Ant(padapter) == _TRUE) &&
					((pmlmeinfo->assoc_AP_vendor != broadcomAP) ||
					 (_rtw_memcmp(raddr, tendaAPMac, 3) == _FALSE)))
				{
					// A-MSDU NOT Supported
					BA_para_set = 0;
					// immediate Block Ack
					BA_para_set |= (1 << 1) & IEEE80211_ADDBA_PARAM_POLICY_MASK;
					// TID
					BA_para_set |= (status << 2) & IEEE80211_ADDBA_PARAM_TID_MASK;
					// max buffer size is 8 MSDU
					BA_para_set |= (8 << 6) & RTW_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK;
				}
				else
#endif
				{
					#if defined(CONFIG_RTL8188E) && defined(CONFIG_SDIO_HCI)
					BA_para_set = (0x0802 | ((status & 0xf) << 2)); //immediate ack & 16 buffer size
					#else
					BA_para_set = (0x1002 | ((status & 0xf) << 2)); //immediate ack & 64 buffer size
					#endif
				}
				//sys_mib.BA_para_set = 0x0802; //immediate ack & 32 buffer size
				BA_para_set = cpu_to_le16(BA_para_set);
				pframe = rtw_set_fixed_ie(pframe, 2, (unsigned char *)(&(BA_para_set)), &(pattrib->pktlen));

				//BA_timeout_value = 0xffff;//max: 65535 TUs(~ 65 ms)
				BA_timeout_value = 5000;//~ 5ms
				BA_timeout_value = cpu_to_le16(BA_timeout_value);
				pframe = rtw_set_fixed_ie(pframe, 2, (unsigned char *)(&(BA_timeout_value)), &(pattrib->pktlen));

				//if ((psta = rtw_get_stainfo(pstapriv, pmlmeinfo->network.MacAddress)) != NULL)
				if ((psta = rtw_get_stainfo(pstapriv, raddr)) != NULL)
				{
					start_seq = (psta->sta_xmitpriv.txseq_tid[status & 0x07]&0xfff) + 1;

					DBG_871X("BA_starting_seqctrl = %d for TID=%d\n", start_seq, status & 0x07);

					psta->BA_starting_seqctrl[status & 0x07] = start_seq;

					BA_starting_seqctrl = start_seq << 4;
				}

				BA_starting_seqctrl = cpu_to_le16(BA_starting_seqctrl);
				pframe = rtw_set_fixed_ie(pframe, 2, (unsigned char *)(&(BA_starting_seqctrl)), &(pattrib->pktlen));
				break;

			case 1: //ADDBA rsp
				pframe = rtw_set_fixed_ie(pframe, 1, &(pmlmeinfo->ADDBA_req.dialog_token), &(pattrib->pktlen));
				pframe = rtw_set_fixed_ie(pframe, 2, (unsigned char *)(&status), &(pattrib->pktlen));
				/*
				//BA_para_set = cpu_to_le16((le16_to_cpu(pmlmeinfo->ADDBA_req.BA_para_set) & 0x3f) | 0x1000); //64 buffer size
				#if defined(CONFIG_RTL8188E )&& defined (CONFIG_SDIO_HCI)
				BA_para_set = ((le16_to_cpu(pmlmeinfo->ADDBA_req.BA_para_set) & 0x3f) | 0x0800); //32buffer size
				#else
				BA_para_set = ((le16_to_cpu(pmlmeinfo->ADDBA_req.BA_para_set) & 0x3f) | 0x1000); //64 buffer size
				#endif
				*/
				rtw_hal_get_def_var(padapter, HW_VAR_MAX_RX_AMPDU_FACTOR, &max_rx_ampdu_factor);
				if(MAX_AMPDU_FACTOR_64K == max_rx_ampdu_factor)
					BA_para_set = ((le16_to_cpu(pmlmeinfo->ADDBA_req.BA_para_set) & 0x3f) | 0x1000); //64 buffer size
				else if(MAX_AMPDU_FACTOR_32K == max_rx_ampdu_factor)
					BA_para_set = ((le16_to_cpu(pmlmeinfo->ADDBA_req.BA_para_set) & 0x3f) | 0x0800); //32 buffer size
				else if(MAX_AMPDU_FACTOR_16K == max_rx_ampdu_factor)
					BA_para_set = ((le16_to_cpu(pmlmeinfo->ADDBA_req.BA_para_set) & 0x3f) | 0x0400); //16 buffer size
				else if(MAX_AMPDU_FACTOR_8K == max_rx_ampdu_factor)
					BA_para_set = ((le16_to_cpu(pmlmeinfo->ADDBA_req.BA_para_set) & 0x3f) | 0x0200); //8 buffer size
				else
					BA_para_set = ((le16_to_cpu(pmlmeinfo->ADDBA_req.BA_para_set) & 0x3f) | 0x1000); //64 buffer size

#if 0//def CONFIG_BT_COEXIST
				if ((BT_1Ant(padapter) == _TRUE) &&
					((pmlmeinfo->assoc_AP_vendor != broadcomAP) ||
					 (_rtw_memcmp(raddr, tendaAPMac, 3) == _FALSE)))
				{
					// max buffer size is 8 MSDU
					BA_para_set &= ~RTW_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK;
					BA_para_set |= (8 << 6) & RTW_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK;
				}
#endif

				if(pregpriv->ampdu_amsdu==0)//disabled
					BA_para_set = cpu_to_le16(BA_para_set & ~BIT(0));
				else if(pregpriv->ampdu_amsdu==1)//enabled
					BA_para_set = cpu_to_le16(BA_para_set | BIT(0));
				else //auto
					BA_para_set = cpu_to_le16(BA_para_set);

#ifdef CONFIG_DONT_CARE_TP
				// max buffer size is 8k
				BA_para_set &= ~RTW_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK;
				BA_para_set |= (8 << 6) & RTW_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK;

				/* disable amsdu */
				BA_para_set = cpu_to_le16(BA_para_set & ~BIT(0));
#endif

#ifdef CONFIG_BT_COEXIST
				if(BT_BtCtrlAggregateBufSize(padapter))
				{
					u8	adjAggBufSize=0;

					adjAggBufSize = BT_GetAdjustAggregateBufSize(padapter);

					DBG_871X("[BTCoex], Set BA para in OnADDBAReq() for field buf size=%d\n", adjAggBufSize);

					// max buffer size is 8k
					BA_para_set &= ~RTW_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK;
					BA_para_set |= (adjAggBufSize << 6) & RTW_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK;

					/* disable amsdu */
					BA_para_set = cpu_to_le16(BA_para_set & ~BIT(0));
				}
#endif
				pmlmeinfo->CurRxBufSize = (BA_para_set >> 6) & 0x3FF;
				DBG_871X("%s Set addba rsp rx buf size=%d\n", __func__, pmlmeinfo->CurRxBufSize);

				pframe = rtw_set_fixed_ie(pframe, 2, (unsigned char *)(&(BA_para_set)), &(pattrib->pktlen));
				pframe = rtw_set_fixed_ie(pframe, 2, (unsigned char *)(&(pmlmeinfo->ADDBA_req.BA_timeout_value)), &(pattrib->pktlen));
				break;
			case 2://DELBA
				BA_para_set = (status & 0x1F) << 3;
				BA_para_set = cpu_to_le16(BA_para_set);
				pframe = rtw_set_fixed_ie(pframe, 2, (unsigned char *)(&(BA_para_set)), &(pattrib->pktlen));

				reason_code = 37;//Requested from peer STA as it does not want to use the mechanism
				reason_code = cpu_to_le16(reason_code);
				pframe = rtw_set_fixed_ie(pframe, 2, (unsigned char *)(&(reason_code)), &(pattrib->pktlen));
				break;
			default:
				break;
		}
	}

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);
}

static void issue_action_BSSCoexistPacket(_adapter *padapter)
{
	_irqL	irqL;
	_list		*plist, *phead;
	unsigned char category, action;
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char				*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short			*fctrl;
	struct	wlan_network	*pnetwork = NULL;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	_queue		*queue	= &(pmlmepriv->scanned_queue);
	u8 InfoContent[16] = {0};
	u8 ICS[8][15];

	if((pmlmepriv->num_FortyMHzIntolerant==0) || (pmlmepriv->num_sta_no_ht==0))
		return;

	if(_TRUE == pmlmeinfo->bwmode_updated)
		return;


	DBG_871X("%s\n", __FUNCTION__);


	category = RTW_WLAN_CATEGORY_PUBLIC;
	action = ACT_PUBLIC_BSSCOEXIST;

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));


	//
	if(pmlmepriv->num_FortyMHzIntolerant>0)
	{
		u8 iedata=0;

		iedata |= BIT(2);//20 MHz BSS Width Request

		pframe = rtw_set_ie(pframe, EID_BSSCoexistence,  1, &iedata, &(pattrib->pktlen));

	}


	//
	_rtw_memset(ICS, 0, sizeof(ICS));
	if(pmlmepriv->num_sta_no_ht>0)
	{
		int i;

		_enter_critical_bh(&(pmlmepriv->scanned_queue.lock), &irqL);

		phead = get_list_head(queue);
		plist = get_next(phead);

		while(1)
		{
			int len;
			u8 *p;
			WLAN_BSSID_EX *pbss_network;

			if (rtw_end_of_queue_search(phead,plist)== _TRUE)
				break;

			pnetwork = LIST_CONTAINOR(plist, struct wlan_network, list);

			plist = get_next(plist);

			pbss_network = (WLAN_BSSID_EX *)&pnetwork->network;

			p = rtw_get_ie(pbss_network->IEs + _FIXED_IE_LENGTH_, _HT_CAPABILITY_IE_, &len, pbss_network->IELength - _FIXED_IE_LENGTH_);
			if((p==NULL) || (len==0))//non-HT
			{
				if((pbss_network->Configuration.DSConfig<=0) || (pbss_network->Configuration.DSConfig>14))
					continue;

				ICS[0][pbss_network->Configuration.DSConfig]=1;

				if(ICS[0][0] == 0)
					ICS[0][0] = 1;
			}

		}

		_exit_critical_bh(&(pmlmepriv->scanned_queue.lock), &irqL);


		for(i= 0;i<8;i++)
		{
			if(ICS[i][0] == 1)
			{
				int j, k = 0;

				InfoContent[k] = i;
				//SET_BSS_INTOLERANT_ELE_REG_CLASS(InfoContent,i);
				k++;

				for(j=1;j<=14;j++)
				{
					if(ICS[i][j]==1)
					{
						if(k<16)
						{
							InfoContent[k] = j; //channel number
							//SET_BSS_INTOLERANT_ELE_CHANNEL(InfoContent+k, j);
							k++;
						}
					}
				}

				pframe = rtw_set_ie(pframe, EID_BSSIntolerantChlReport, k, InfoContent, &(pattrib->pktlen));

			}

		}


	}


	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

}

unsigned int send_delba(_adapter *padapter, u8 initiator, u8 *addr)
{
	struct sta_priv *pstapriv = &padapter->stapriv;
	struct sta_info *psta = NULL;
	//struct recv_reorder_ctrl *preorder_ctrl;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u16 tid;

	if((pmlmeinfo->state&0x03) != WIFI_FW_AP_STATE)
		if (!(pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS))
			return _SUCCESS;

	psta = rtw_get_stainfo(pstapriv, addr);
	if(psta==NULL)
		return _SUCCESS;

	//DBG_871X("%s:%s\n", __FUNCTION__, (initiator==0)?"RX_DIR":"TX_DIR");

	if(initiator==0) // recipient
	{
		for(tid = 0;tid<MAXTID;tid++)
		{
			if(psta->recvreorder_ctrl[tid].enable == _TRUE)
			{
				DBG_871X("rx agg disable tid(%d)\n",tid);
				issue_action_BA(padapter, addr, RTW_WLAN_ACTION_DELBA, (((tid <<1) |initiator)&0x1F));
				psta->recvreorder_ctrl[tid].enable = _FALSE;
				psta->recvreorder_ctrl[tid].indicate_seq = 0xffff;
				#ifdef DBG_RX_SEQ
				DBG_871X("DBG_RX_SEQ %s:%d indicate_seq:%u \n", __FUNCTION__, __LINE__,
					psta->recvreorder_ctrl[tid].indicate_seq);
				#endif
			}
		}
	}
	else if(initiator == 1)// originator
	{
		//DBG_871X("tx agg_enable_bitmap(0x%08x)\n", psta->htpriv.agg_enable_bitmap);
		for(tid = 0;tid<MAXTID;tid++)
		{
			if(psta->htpriv.agg_enable_bitmap & BIT(tid))
			{
				DBG_871X("tx agg disable tid(%d)\n",tid);
				issue_action_BA(padapter, addr, RTW_WLAN_ACTION_DELBA, (((tid <<1) |initiator)&0x1F) );
				psta->htpriv.agg_enable_bitmap &= ~BIT(tid);
				psta->htpriv.candidate_tid_bitmap &= ~BIT(tid);

			}
		}
	}

	return _SUCCESS;

}

unsigned int send_beacon(_adapter *padapter)
{
	u8	bxmitok = _FALSE;
	int	issue=0;
	int poll = 0;
//#ifdef CONFIG_CONCURRENT_MODE
	//struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	//struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	//_adapter *pbuddy_adapter = padapter->pbuddy_adapter;
	//struct mlme_priv *pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);
//#endif

#ifdef CONFIG_PCI_HCI

	//DBG_871X("%s\n", __FUNCTION__);

	issue_beacon(padapter);

	return _SUCCESS;

#endif

#if defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
	u32 start = rtw_get_current_time();

	rtw_hal_set_hwreg(padapter, HW_VAR_BCN_VALID, NULL);
	rtw_hal_set_hwreg(padapter, HW_VAR_DL_BCN_SEL, NULL);

	do{
		issue_beacon(padapter);
		issue++;
		do {
			rtw_yield_os();
			rtw_hal_get_hwreg(padapter, HW_VAR_BCN_VALID, (u8 *)(&bxmitok));
			poll++;
		}while((poll%10)!=0 && _FALSE == bxmitok && !padapter->bSurpriseRemoved && !padapter->bDriverStopped);

	}while(_FALSE == bxmitok && issue<100 && !padapter->bSurpriseRemoved && !padapter->bDriverStopped);

	if(padapter->bSurpriseRemoved || padapter->bDriverStopped)
	{
		return _FAIL;
	}
	if(_FALSE == bxmitok)
	{
		DBG_871X("%s fail! %u ms\n", __FUNCTION__, rtw_get_passing_time_ms(start));
		return _FAIL;
	}
	else
	{
		u32 passing_time = rtw_get_passing_time_ms(start);

		if(passing_time > 100 || issue > 3)
			DBG_871X("%s success, issue:%d, poll:%d, %u ms\n", __FUNCTION__, issue, poll, rtw_get_passing_time_ms(start));
		//else
		//	DBG_871X("%s success, issue:%d, poll:%d, %u ms\n", __FUNCTION__, issue, poll, rtw_get_passing_time_ms(start));

		return _SUCCESS;
	}

#endif

}

/****************************************************************************

Following are some utitity fuctions for WiFi MLME

*****************************************************************************/

BOOLEAN IsLegal5GChannel(
	IN PADAPTER			Adapter,
	IN u8			channel)
{

	int i=0;
	u8 Channel_5G[45] = {36,38,40,42,44,46,48,50,52,54,56,58,
		60,62,64,100,102,104,106,108,110,112,114,116,118,120,122,
		124,126,128,130,132,134,136,138,140,149,151,153,155,157,159,
		161,163,165};
	for(i=0;i<sizeof(Channel_5G);i++)
		if(channel == Channel_5G[i])
			return _TRUE;
	return _FALSE;
}

void site_survey(_adapter *padapter)
{
	unsigned char		survey_channel = 0, val8;
	RT_SCAN_TYPE	ScanType = SCAN_PASSIVE;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u32 initialgain = 0;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

#ifdef CONFIG_P2P

#ifdef CONFIG_CONCURRENT_MODE

#ifdef CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
	u8 stay_buddy_ch = 0;
#endif //CONFIG_STA_MODE_SCAN_UNDER_AP_MODE

	PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;
	struct mlme_ext_priv *pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;

#endif //CONFIG_CONCURRENT_MODE
	struct wifidirect_info *pwdinfo= &(padapter->wdinfo);
	static unsigned char  prev_survey_channel = 0;
	static unsigned int p2p_scan_count = 0;

	if(rtw_p2p_findphase_ex_is_social(pwdinfo))
	{
		//	Commented by Albert 2011/06/03
		//	The driver is in the find phase, it should go through the social channel.
		int ch_set_idx;
		survey_channel = pwdinfo->social_chan[pmlmeext->sitesurvey_res.channel_idx];
		ch_set_idx = rtw_ch_set_search_ch(pmlmeext->channel_set, survey_channel);
		if (ch_set_idx >= 0)
			ScanType = pmlmeext->channel_set[ch_set_idx].ScanType;
		else
			ScanType = SCAN_ACTIVE;
	}
	else
#endif //CONFIG_P2P
	{
		struct rtw_ieee80211_channel *ch;
		if (pmlmeext->sitesurvey_res.channel_idx < pmlmeext->sitesurvey_res.ch_num) {
			ch = &pmlmeext->sitesurvey_res.ch[pmlmeext->sitesurvey_res.channel_idx];
			survey_channel = ch->hw_value;
			ScanType = (ch->flags & RTW_IEEE80211_CHAN_PASSIVE_SCAN) ? SCAN_PASSIVE : SCAN_ACTIVE;
		}
	}

	if (0)
	DBG_871X(FUNC_ADPT_FMT" ch:%u(cnt:%u,idx:%d) at %dms, %c%c%c\n"
		, FUNC_ADPT_ARG(padapter)
		, survey_channel
		, pwdinfo->find_phase_state_exchange_cnt, pmlmeext->sitesurvey_res.channel_idx
		, rtw_get_passing_time_ms(padapter->mlmepriv.scan_start_time)
		, ScanType?'A':'P', pmlmeext->sitesurvey_res.scan_mode?'A':'P'
		, pmlmeext->sitesurvey_res.ssid[0].SsidLength?'S':' '
	);

	if(survey_channel != 0)
	{
		//PAUSE 4-AC Queue when site_survey
		//rtw_hal_get_hwreg(padapter, HW_VAR_TXPAUSE, (u8 *)(&val8));
		//val8 |= 0x0f;
		//rtw_hal_set_hwreg(padapter, HW_VAR_TXPAUSE, (u8 *)(&val8));
#ifdef CONFIG_CONCURRENT_MODE
#ifdef CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
		if((padapter->pbuddy_adapter->mlmeextpriv.mlmext_info.state&0x03) == WIFI_FW_AP_STATE)
		{
			if( pmlmeinfo->scan_cnt == RTW_SCAN_NUM_OF_CH )
			{
				pmlmeinfo->scan_cnt = 0;
				survey_channel = pbuddy_mlmeext->cur_channel;
				ScanType = SCAN_ACTIVE;
				stay_buddy_ch = 1;
			}
			else
			{
				if( pmlmeinfo->scan_cnt == 0 )
					stay_buddy_ch = 2;
				pmlmeinfo->scan_cnt++;
			}
		}
#endif //CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
#endif

		if(pmlmeext->sitesurvey_res.channel_idx == 0)
		{
			set_channel_bwmode(padapter, survey_channel, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);
		}
		else
		{
			SelectChannel(padapter, survey_channel);
		}

#ifdef CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
		if( stay_buddy_ch == 1 )
		{
			val8 = 0; //survey done
			padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_MLME_SITESURVEY, (u8 *)(&val8));

			if(check_buddy_mlmeinfo_state(padapter, WIFI_FW_AP_STATE) &&
				check_buddy_fwstate(padapter, _FW_LINKED))
			{
				update_beacon(padapter->pbuddy_adapter, 0, NULL, _TRUE);
			}
		}
		else if( stay_buddy_ch == 2 )
		{
			val8 = 1; //under site survey
			padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_MLME_SITESURVEY, (u8 *)(&val8));
		}
#endif //CONFIG_STA_MODE_SCAN_UNDER_AP_MODE

		if(ScanType == SCAN_ACTIVE) //obey the channel plan setting...
		{
			#ifdef CONFIG_P2P
			if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_SCAN) ||
				rtw_p2p_chk_state(pwdinfo, P2P_STATE_FIND_PHASE_SEARCH)
			)
			{
				issue_probereq_p2p(padapter);
				issue_probereq_p2p(padapter);
				issue_probereq_p2p(padapter);
			}
			else
			#endif //CONFIG_P2P
			{
				int i;
				for(i=0;i<RTW_SSID_SCAN_AMOUNT;i++){
					if(pmlmeext->sitesurvey_res.ssid[i].SsidLength) {
						//todo: to issue two probe req???
						issue_probereq(padapter, &(pmlmeext->sitesurvey_res.ssid[i]),1);
						//rtw_msleep_os(SURVEY_TO>>1);
						issue_probereq(padapter, &(pmlmeext->sitesurvey_res.ssid[i]),1);
					}
				}

				if(pmlmeext->sitesurvey_res.scan_mode == SCAN_ACTIVE) {
					//todo: to issue two probe req???
					issue_probereq(padapter, NULL, 1);
					//rtw_msleep_os(SURVEY_TO>>1);
					issue_probereq(padapter, NULL, 1);
				}
			}
		}

#ifdef CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
		if( stay_buddy_ch == 1 )
			set_survey_timer(pmlmeext, pmlmeext->chan_scan_time * RTW_STAY_AP_CH_MILLISECOND );
		else
#endif //CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
		{
#if (!(defined ANDROID_2X) && (defined CONFIG_PLATFORM_SPRD))
			if(ScanType == SCAN_PASSIVE)
				set_survey_timer(pmlmeext, SURVEY_TO_PASSIVE);
			else
#endif
				set_survey_timer(pmlmeext, pmlmeext->chan_scan_time);
		}

	}
	else
	{

		//	channel number is 0 or this channel is not valid.

#ifdef CONFIG_CONCURRENT_MODE
		u8 cur_channel;
		u8 cur_bwmode;
		u8 cur_ch_offset;

		if (rtw_get_ch_setting_union(padapter, &cur_channel, &cur_bwmode, &cur_ch_offset) != 0)
		{
			if (0)
			DBG_871X(FUNC_ADPT_FMT" back to linked union - ch:%u, bw:%u, offset:%u\n",
				FUNC_ADPT_ARG(padapter), cur_channel, cur_bwmode, cur_ch_offset);
		}
#ifdef CONFIG_IOCTL_CFG80211
		else if(padapter->pbuddy_adapter
			&& wdev_to_priv(pbuddy_adapter->rtw_wdev)->p2p_enabled
			&& rtw_p2p_chk_state(&pbuddy_adapter->wdinfo, P2P_STATE_LISTEN)
			)
		{
			cur_channel = pbuddy_adapter->wdinfo.listen_channel;
			cur_bwmode = pbuddy_mlmeext->cur_bwmode;
			cur_ch_offset = pbuddy_mlmeext->cur_ch_offset;
		}
#endif
		else
		{
			cur_channel = pmlmeext->cur_channel;
			cur_bwmode = pmlmeext->cur_bwmode;
			cur_ch_offset = pmlmeext->cur_ch_offset;
		}
#endif


#ifdef CONFIG_P2P
		if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_SCAN) || rtw_p2p_chk_state(pwdinfo, P2P_STATE_FIND_PHASE_SEARCH))
		{
			#ifdef CONFIG_DBG_P2P
			DBG_871X( "[%s] find phase exchange cnt = %d\n", __FUNCTION__, pwdinfo->find_phase_state_exchange_cnt );
			#endif
		}

		if(rtw_p2p_findphase_ex_is_needed(pwdinfo))
		{
			//	Set the P2P State to the listen state of find phase and set the current channel to the listen channel
			set_channel_bwmode(padapter, pwdinfo->listen_channel, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);
			rtw_p2p_set_state(pwdinfo, P2P_STATE_FIND_PHASE_LISTEN);
			pmlmeext->sitesurvey_res.state = SCAN_DISABLE;

			initialgain = 0xff; //restore RX GAIN
			rtw_hal_set_hwreg(padapter, HW_VAR_INITIAL_GAIN, (u8 *)(&initialgain));
			//turn on dynamic functions
			Restore_DM_Func_Flag(padapter);
			//Switch_DM_Func(padapter, DYNAMIC_FUNC_DIG|DYNAMIC_FUNC_HP|DYNAMIC_FUNC_SS, _TRUE);

			_set_timer( &pwdinfo->find_phase_timer, ( u32 ) ( ( u32 ) ( pwdinfo->listen_dwell ) * 100 ) );
		}
		else
#endif //CONFIG_P2P
		{

#ifdef CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
			pmlmeinfo->scan_cnt = 0;
#endif //CONFIG_DMP_STA_NODE_SCAN_UNDER_AP_MODE

#ifdef CONFIG_ANTENNA_DIVERSITY
			// 20100721:Interrupt scan operation here.
			// For SW antenna diversity before link, it needs to switch to another antenna and scan again.
			// It compares the scan result and select beter one to do connection.
			if(rtw_hal_antdiv_before_linked(padapter))
			{
				pmlmeext->sitesurvey_res.bss_cnt = 0;
				pmlmeext->sitesurvey_res.channel_idx = -1;
				pmlmeext->chan_scan_time = SURVEY_TO /2;
				set_survey_timer(pmlmeext, pmlmeext->chan_scan_time);
				return;
			}
#endif

#ifdef CONFIG_P2P
			if(rtw_p2p_chk_state(pwdinfo, P2P_STATE_SCAN) || rtw_p2p_chk_state(pwdinfo, P2P_STATE_FIND_PHASE_SEARCH))
			{
			#ifdef CONFIG_CONCURRENT_MODE
				#ifndef CONFIG_IOCTL_CFG80211
				if ( check_buddy_fwstate(padapter, _FW_LINKED ) )
				{
					_set_timer( &pwdinfo->ap_p2p_switch_timer, 500 );
				}
				#endif //CONFIG_IOCTL_CFG80211
				rtw_p2p_set_state(pwdinfo, rtw_p2p_pre_state(pwdinfo));
			#else
				rtw_p2p_set_state(pwdinfo, rtw_p2p_pre_state(pwdinfo));
			#endif
			}
			rtw_p2p_findphase_ex_set(pwdinfo, P2P_FINDPHASE_EX_NONE);
#endif //CONFIG_P2P

			pmlmeext->sitesurvey_res.state = SCAN_COMPLETE;

			//switch back to the original channel
			//SelectChannel(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset);

			{
#ifdef CONFIG_CONCURRENT_MODE
				set_channel_bwmode(padapter, cur_channel, cur_ch_offset, cur_bwmode);
#else
#ifdef CONFIG_DUALMAC_CONCURRENT
				dc_set_channel_bwmode_survey_done(padapter);
#else
#ifndef CONFIG_IOCTL_CFG80211
			if( rtw_p2p_chk_state(pwdinfo, P2P_STATE_LISTEN) )
			{
				set_channel_bwmode(padapter, pwdinfo->listen_channel, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);
			}
			else
#endif //CONFIG_IOCTL_CFG80211
				set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
#endif //CONFIG_DUALMAC_CONCURRENT
#endif //CONFIG_CONCURRENT_MODE
			}

			//flush 4-AC Queue after site_survey
			//val8 = 0;
			//rtw_hal_set_hwreg(padapter, HW_VAR_TXPAUSE, (u8 *)(&val8));

			//config MSR
			Set_MSR(padapter, (pmlmeinfo->state & 0x3));

			initialgain = 0xff; //restore RX GAIN
			rtw_hal_set_hwreg(padapter, HW_VAR_INITIAL_GAIN, (u8 *)(&initialgain));
			//turn on dynamic functions
			Restore_DM_Func_Flag(padapter);
			//Switch_DM_Func(padapter, DYNAMIC_ALL_FUNC_ENABLE, _TRUE);

			if (is_client_associated_to_ap(padapter) == _TRUE)
			{
				//issue null data
				issue_nulldata(padapter, 0);

#ifdef CONFIG_CONCURRENT_MODE
				if(is_client_associated_to_ap(padapter->pbuddy_adapter) == _TRUE)
				{
					DBG_871X("adapter is surveydone(buddy_adapter is linked), issue nulldata(pwrbit=0)\n");
					issue_nulldata(padapter->pbuddy_adapter, 0);
				}
#endif
			}
#ifdef CONFIG_CONCURRENT_MODE
			else if(is_client_associated_to_ap(padapter->pbuddy_adapter) == _TRUE)
			{
				issue_nulldata(padapter->pbuddy_adapter, 0);
			}
#endif

			val8 = 0; //survey done
			rtw_hal_set_hwreg(padapter, HW_VAR_MLME_SITESURVEY, (u8 *)(&val8));

			report_surveydone_event(padapter);

			pmlmeext->chan_scan_time = SURVEY_TO;
			pmlmeext->sitesurvey_res.state = SCAN_DISABLE;

			issue_action_BSSCoexistPacket(padapter);
			issue_action_BSSCoexistPacket(padapter);
			issue_action_BSSCoexistPacket(padapter);

		}

#ifdef CONFIG_CONCURRENT_MODE
		if(check_buddy_mlmeinfo_state(padapter, WIFI_FW_AP_STATE) &&
			check_buddy_fwstate(padapter, _FW_LINKED))
		{

			DBG_871X("survey done, current CH=%d, BW=%d, offset=%d\n", cur_channel, cur_bwmode, cur_ch_offset);

			DBG_871X("restart pbuddy_adapter's beacon\n");

			update_beacon(padapter->pbuddy_adapter, 0, NULL, _TRUE);
		}
#endif

	}

	return;

}

//collect bss info from Beacon and Probe response frames.
u8 collect_bss_info(_adapter *padapter, union recv_frame *precv_frame, WLAN_BSSID_EX *bssid)
{
	int	i;
	u32	len;
	u8	*p;
	u16	val16, subtype;
	u8	*pframe = precv_frame->u.hdr.rx_data;
	u32	packet_len = precv_frame->u.hdr.len;
	struct registry_priv 	*pregistrypriv = &padapter->registrypriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	len = packet_len - sizeof(struct rtw_ieee80211_hdr_3addr);

	if (len > MAX_IE_SZ)
	{
		//DBG_871X("IE too long for survey event\n");
		return _FAIL;
	}

	_rtw_memset(bssid, 0, sizeof(WLAN_BSSID_EX));

	subtype = GetFrameSubType(pframe);

	if(subtype==WIFI_BEACON)
		bssid->Reserved[0] = 1;
	else
		bssid->Reserved[0] = 0;

	bssid->Length = sizeof(WLAN_BSSID_EX) - MAX_IE_SZ + len;
	bssid->BcnLength = packet_len;

	//below is to copy the information element
	bssid->IELength = len;
	_rtw_memcpy(bssid->IEs, (pframe + sizeof(struct rtw_ieee80211_hdr_3addr)), bssid->IELength);

	//get the signal strength
	//bssid->Rssi = precv_frame->u.hdr.attrib.SignalStrength; // 0-100 index.
	bssid->Rssi = precv_frame->u.hdr.attrib.phy_info.RecvSignalPower; // in dBM.raw data
	bssid->PhyInfo.SignalQuality = precv_frame->u.hdr.attrib.phy_info.SignalQuality;//in percentage
	bssid->PhyInfo.SignalStrength = precv_frame->u.hdr.attrib.phy_info.SignalStrength;//in percentage
#ifdef CONFIG_ANTENNA_DIVERSITY
	//rtw_hal_get_hwreg(padapter, HW_VAR_CURRENT_ANTENNA, (u8 *)(&bssid->PhyInfo.Optimum_antenna));
	rtw_hal_get_def_var(padapter, HAL_DEF_CURRENT_ANTENNA,  &bssid->PhyInfo.Optimum_antenna);
#endif

	// checking SSID
	if ((p = rtw_get_ie(bssid->IEs + _FIXED_IE_LENGTH_, _SSID_IE_, &len, bssid->IELength - _FIXED_IE_LENGTH_)) == NULL)
	{
		DBG_871X("marc: cannot find SSID for survey event\n");
		return _FAIL;
	}

	if (*(p + 1))
	{
		if (len > NDIS_802_11_LENGTH_SSID)
		{
			DBG_871X("%s()-%d: IE too long (%d) for survey event\n", __FUNCTION__, __LINE__, len);
			return _FAIL;
		}
		_rtw_memcpy(bssid->Ssid.Ssid, (p + 2), *(p + 1));
		bssid->Ssid.SsidLength = *(p + 1);
	}
	else
	{
		bssid->Ssid.SsidLength = 0;
	}

	_rtw_memset(bssid->SupportedRates, 0, NDIS_802_11_LENGTH_RATES_EX);

	//checking rate info...
	i = 0;
	p = rtw_get_ie(bssid->IEs + _FIXED_IE_LENGTH_, _SUPPORTEDRATES_IE_, &len, bssid->IELength - _FIXED_IE_LENGTH_);
	if (p != NULL)
	{
		if (len > NDIS_802_11_LENGTH_RATES)
		{
			DBG_871X("%s()-%d: IE too long (%d) for survey event\n", __FUNCTION__, __LINE__, len);
			return _FAIL;
		}
		_rtw_memcpy(bssid->SupportedRates, (p + 2), len);
		i = len;
	}

	p = rtw_get_ie(bssid->IEs + _FIXED_IE_LENGTH_, _EXT_SUPPORTEDRATES_IE_, &len, bssid->IELength - _FIXED_IE_LENGTH_);
	if (p != NULL)
	{
		if (len > (NDIS_802_11_LENGTH_RATES_EX-i))
		{
			DBG_871X("%s()-%d: IE too long (%d) for survey event\n", __FUNCTION__, __LINE__, len);
			return _FAIL;
		}
		_rtw_memcpy(bssid->SupportedRates + i, (p + 2), len);
	}

	//todo:
#if 0
	if (judge_network_type(bssid->SupportedRates, (len + i)) == WIRELESS_11B)
	{
		bssid->NetworkTypeInUse = Ndis802_11DS;
	}
	else
#endif
	{
		bssid->NetworkTypeInUse = Ndis802_11OFDM24;
	}

	// Checking for DSConfig
	p = rtw_get_ie(bssid->IEs + _FIXED_IE_LENGTH_, _DSSET_IE_, &len, bssid->IELength - _FIXED_IE_LENGTH_);

	bssid->Configuration.DSConfig = 0;
	bssid->Configuration.Length = 0;

	if (p)
	{
		bssid->Configuration.DSConfig = *(p + 2);
	}
	else
	{// In 5G, some ap do not have DSSET IE
		// checking HT info for channel
		p = rtw_get_ie(bssid->IEs + _FIXED_IE_LENGTH_, _HT_ADD_INFO_IE_, &len, bssid->IELength - _FIXED_IE_LENGTH_);
		if(p)
		{
			struct HT_info_element *HT_info = (struct HT_info_element *)(p + 2);
			bssid->Configuration.DSConfig = HT_info->primary_channel;
		}
		else
		{ // use current channel
			bssid->Configuration.DSConfig = rtw_get_oper_ch(padapter);
		}
	}

	_rtw_memcpy(&bssid->Configuration.BeaconPeriod, rtw_get_beacon_interval_from_ie(bssid->IEs), 2);
	bssid->Configuration.BeaconPeriod = le32_to_cpu(bssid->Configuration.BeaconPeriod);

	val16 = rtw_get_capability((WLAN_BSSID_EX *)bssid);

	if (val16 & BIT(0))
	{
		bssid->InfrastructureMode = Ndis802_11Infrastructure;
		_rtw_memcpy(bssid->MacAddress, GetAddr2Ptr(pframe), ETH_ALEN);
	}
	else
	{
		bssid->InfrastructureMode = Ndis802_11IBSS;
		_rtw_memcpy(bssid->MacAddress, GetAddr3Ptr(pframe), ETH_ALEN);
	}

	if (val16 & BIT(4))
		bssid->Privacy = 1;
	else
		bssid->Privacy = 0;

	bssid->Configuration.ATIMWindow = 0;

	//20/40 BSS Coexistence check
	if((pregistrypriv->wifi_spec==1) && (_FALSE == pmlmeinfo->bwmode_updated))
	{
		struct mlme_priv *pmlmepriv = &padapter->mlmepriv;

		p = rtw_get_ie(bssid->IEs + _FIXED_IE_LENGTH_, _HT_CAPABILITY_IE_, &len, bssid->IELength - _FIXED_IE_LENGTH_);
		if(p && len>0)
		{
			struct HT_caps_element	*pHT_caps;
			pHT_caps = (struct HT_caps_element	*)(p + 2);

			if(pHT_caps->u.HT_cap_element.HT_caps_info&BIT(14))
			{
				pmlmepriv->num_FortyMHzIntolerant++;
			}
		}
		else
		{
			pmlmepriv->num_sta_no_ht++;
		}

	}

#ifdef CONFIG_INTEL_WIDI
	//process_intel_widi_query_or_tigger(padapter, bssid);
	if(process_intel_widi_query_or_tigger(padapter, bssid))
	{
		return _FAIL;
	}
#endif // CONFIG_INTEL_WIDI

	#if defined(DBG_RX_SIGNAL_DISPLAY_PROCESSING) & 1
	if(strcmp(bssid->Ssid.Ssid, DBG_RX_SIGNAL_DISPLAY_SSID_MONITORED) == 0) {
		DBG_871X("Receiving %s("MAC_FMT", DSConfig:%u) from ch%u with ss:%3u, sq:%3u, RawRSSI:%3ld\n"
			, bssid->Ssid.Ssid, MAC_ARG(bssid->MacAddress), bssid->Configuration.DSConfig
			, rtw_get_oper_ch(padapter)
			, bssid->PhyInfo.SignalStrength, bssid->PhyInfo.SignalQuality, bssid->Rssi
		);
	}
	#endif

	// mark bss info receving from nearby channel as SignalQuality 101
	if(bssid->Configuration.DSConfig != rtw_get_oper_ch(padapter))
	{
		bssid->PhyInfo.SignalQuality= 101;
	}

	return _SUCCESS;
}

void start_create_ibss(_adapter* padapter)
{
	unsigned short	caps;
	u8	val8;
	u8	join_type;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
	pmlmeext->cur_channel = (u8)pnetwork->Configuration.DSConfig;
	pmlmeinfo->bcn_interval = get_beacon_interval(pnetwork);

	//update wireless mode
	update_wireless_mode(padapter);

	//udpate capability
	caps = rtw_get_capability((WLAN_BSSID_EX *)pnetwork);
	update_capinfo(padapter, caps);
	if(caps&cap_IBSS)//adhoc master
	{
		//set_opmode_cmd(padapter, adhoc);//removed

		val8 = 0xcf;
		rtw_hal_set_hwreg(padapter, HW_VAR_SEC_CFG, (u8 *)(&val8));

		//switch channel
		//SelectChannel(padapter, pmlmeext->cur_channel, HAL_PRIME_CHNL_OFFSET_DONT_CARE);
		set_channel_bwmode(padapter, pmlmeext->cur_channel, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);

		beacon_timing_control(padapter);

		//set msr to WIFI_FW_ADHOC_STATE
		pmlmeinfo->state = WIFI_FW_ADHOC_STATE;
		Set_MSR(padapter, (pmlmeinfo->state & 0x3));

		//issue beacon
		if(send_beacon(padapter)==_FAIL)
		{
			RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("issuing beacon frame fail....\n"));

			report_join_res(padapter, -1);
			pmlmeinfo->state = WIFI_FW_NULL_STATE;
		}
		else
		{
			rtw_hal_set_hwreg(padapter, HW_VAR_BSSID, padapter->registrypriv.dev_network.MacAddress);
			join_type = 0;
			rtw_hal_set_hwreg(padapter, HW_VAR_MLME_JOIN, (u8 *)(&join_type));

			report_join_res(padapter, 1);
			pmlmeinfo->state |= WIFI_FW_ASSOC_SUCCESS;
		}
	}
	else
	{
		DBG_871X("start_create_ibss, invalid cap:%x\n", caps);
		return;
	}

}

void start_clnt_join(_adapter* padapter)
{
	unsigned short	caps;
	u8	val8;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
#ifdef CONFIG_DUALMAC_CONCURRENT
	u8	dc_join_status;
#endif
	int beacon_timeout;

	pmlmeext->cur_channel = (u8)pnetwork->Configuration.DSConfig;
	pmlmeinfo->bcn_interval = get_beacon_interval(pnetwork);

	//update wireless mode
	update_wireless_mode(padapter);

	//udpate capability
	caps = rtw_get_capability((WLAN_BSSID_EX *)pnetwork);
	update_capinfo(padapter, caps);
	if (caps&cap_ESS)
	{
#ifdef CONFIG_DUALMAC_CONCURRENT
		dc_join_status = dc_handle_join_request(padapter);
		if(dc_join_status == _FAIL)
		{
			DBG_871X("dc_handle_join_request for STA fail !!!\n");
			return;
		}
#endif
#ifdef CONFIG_CONCURRENT_MODE
		if(concurrent_chk_start_clnt_join(padapter) == _FAIL)
			return;
#endif

		Set_MSR(padapter, WIFI_FW_STATION_STATE);

		val8 = (pmlmeinfo->auth_algo == dot11AuthAlgrthm_8021X)? 0xcc: 0xcf;

#ifdef CONFIG_WAPI_SUPPORT
		if (padapter->wapiInfo.bWapiEnable && pmlmeinfo->auth_algo == dot11AuthAlgrthm_WAPI)
		{
			//Disable TxUseDefaultKey, RxUseDefaultKey, RxBroadcastUseDefaultKey.
			val8 = 0x4c;
		}
#endif
		rtw_hal_set_hwreg(padapter, HW_VAR_SEC_CFG, (u8 *)(&val8));

		//switch channel
		set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);

#ifndef CONFIG_AUTH_DIRECT_WITHOUT_BCN
		//here wait for receiving the beacon to start auth
		//and enable a timer
		beacon_timeout = decide_wait_for_beacon_timeout(pmlmeinfo->bcn_interval);
		set_link_timer(pmlmeext, beacon_timeout);
#endif
		_set_timer( &padapter->mlmepriv.assoc_timer,
			(REAUTH_TO * REAUTH_LIMIT) + (REASSOC_TO*REASSOC_LIMIT) +beacon_timeout);

		pmlmeinfo->state = WIFI_FW_AUTH_NULL | WIFI_FW_STATION_STATE;

#ifdef CONFIG_AUTH_DIRECT_WITHOUT_BCN
		//start auth without waite a beacon
#ifdef CONFIG_BT_COEXIST
		//should delay at least 100ms to wait for tdma start work
		rtw_msleep_os(100);
#endif
		start_clnt_auth(padapter);
#endif
	}
	else if (caps&cap_IBSS) //adhoc client
	{
#ifdef CONFIG_DUALMAC_CONCURRENT
		dc_join_status = dc_handle_join_request(padapter);
		if(dc_join_status == _FAIL)
		{
			DBG_871X("dc_handle_join_request for Ad-hoc fail !!!\n");
			return;
		}
#endif

		Set_MSR(padapter, WIFI_FW_ADHOC_STATE);

		val8 = 0xcf;
		rtw_hal_set_hwreg(padapter, HW_VAR_SEC_CFG, (u8 *)(&val8));

		//switch channel
		set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);

		beacon_timing_control(padapter);

		pmlmeinfo->state = WIFI_FW_ADHOC_STATE;

		report_join_res(padapter, 1);
	}
	else
	{
		//DBG_871X("marc: invalid cap:%x\n", caps);
		return;
	}

}

void start_clnt_auth(_adapter* padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	_cancel_timer_ex(&pmlmeext->link_timer);

	pmlmeinfo->state &= (~WIFI_FW_AUTH_NULL);
	pmlmeinfo->state |= WIFI_FW_AUTH_STATE;

	pmlmeinfo->auth_seq = 1;
	pmlmeinfo->reauth_count = 0;
	pmlmeinfo->reassoc_count = 0;
	pmlmeinfo->link_count = 0;


	// Because of AP's not receiving deauth before
	// AP may: 1)not response auth or 2)deauth us after link is complete
	// issue deauth before issuing auth to deal with the situation
#ifndef CONFIG_PLATFORM_RTK_DMP
	//	Commented by Albert 2012/07/21
	//	For the Win8 P2P connection, it will be hard to have a successful connection if this Wi-Fi doesn't connect to it.
	issue_deauth(padapter, (&(pmlmeinfo->network))->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
#endif

	DBG_871X_LEVEL(_drv_always_, "start auth\n");
	issue_auth(padapter, NULL, 0);

	set_link_timer(pmlmeext, REAUTH_TO);

}


void start_clnt_assoc(_adapter* padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	_cancel_timer_ex(&pmlmeext->link_timer);

	pmlmeinfo->state &= (~(WIFI_FW_AUTH_NULL | WIFI_FW_AUTH_STATE));
	pmlmeinfo->state |= (WIFI_FW_AUTH_SUCCESS | WIFI_FW_ASSOC_STATE);

	issue_assocreq(padapter);

	set_link_timer(pmlmeext, REASSOC_TO);
}

unsigned int receive_disconnect(_adapter *padapter, unsigned char *MacAddr, unsigned short reason)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	//check A3
	if (!(_rtw_memcmp(MacAddr, get_my_bssid(&pmlmeinfo->network), ETH_ALEN)))
		return _SUCCESS;

	DBG_871X("%s\n", __FUNCTION__);

	if((pmlmeinfo->state&0x03) == WIFI_FW_STATION_STATE)
	{
		if (pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS)
		{
			pmlmeinfo->state = WIFI_FW_NULL_STATE;
			report_del_sta_event(padapter, MacAddr, reason);
#ifdef CONFIG_INTEL_WIDI
			process_intel_widi_disconnect(padapter, 1);
#endif // CONFIG_INTEL_WIDI
		}
		else if (pmlmeinfo->state & WIFI_FW_LINKING_STATE)
		{
			pmlmeinfo->state = WIFI_FW_NULL_STATE;
			report_join_res(padapter, -2);
		}
	}

	return _SUCCESS;
}

#ifdef CONFIG_80211D
static void process_80211d(PADAPTER padapter, WLAN_BSSID_EX *bssid)
{
	struct registry_priv *pregistrypriv;
	struct mlme_ext_priv *pmlmeext;
	RT_CHANNEL_INFO *chplan_new;
	u8 channel;
	u8 i;


	pregistrypriv = &padapter->registrypriv;
	pmlmeext = &padapter->mlmeextpriv;

	// Adjust channel plan by AP Country IE
	if (pregistrypriv->enable80211d &&
		(!pmlmeext->update_channel_plan_by_ap_done))
	{
		u8 *ie, *p;
		u32 len;
		RT_CHANNEL_PLAN chplan_ap;
		RT_CHANNEL_INFO chplan_sta[MAX_CHANNEL_NUM];
		u8 country[4];
		u8 fcn; // first channel number
		u8 noc; // number of channel
		u8 j, k;

		ie = rtw_get_ie(bssid->IEs + _FIXED_IE_LENGTH_, _COUNTRY_IE_, &len, bssid->IELength - _FIXED_IE_LENGTH_);
		if (!ie) return;
		if (len < 6) return;

		ie += 2;
		p = ie;
		ie += len;

		_rtw_memset(country, 0, 4);
		_rtw_memcpy(country, p, 3);
		p += 3;
		RT_TRACE(_module_rtl871x_mlme_c_, _drv_notice_,
				("%s: 802.11d country=%s\n", __FUNCTION__, country));

		i = 0;
		while ((ie - p) >= 3)
		{
			fcn = *(p++);
			noc = *(p++);
			p++;

			for (j = 0; j < noc; j++)
			{
				if (fcn <= 14) channel = fcn + j; // 2.4 GHz
				else channel = fcn + j*4; // 5 GHz

				chplan_ap.Channel[i++] = channel;
			}
		}
		chplan_ap.Len = i;

#ifdef CONFIG_DEBUG_RTL871X
#ifdef PLATFORM_LINUX
		i = 0;
		DBG_871X("%s: AP[%s] channel plan {", __func__, bssid->Ssid.Ssid);
		while ((i < chplan_ap.Len) && (chplan_ap.Channel[i] != 0))
		{
			DBG_8192C("%02d,", chplan_ap.Channel[i]);
			i++;
		}
		DBG_871X("}\n");
#endif
#endif

		_rtw_memcpy(chplan_sta, pmlmeext->channel_set, sizeof(chplan_sta));
#ifdef CONFIG_DEBUG_RTL871X
#ifdef PLATFORM_LINUX
		i = 0;
		DBG_871X("%s: STA channel plan {", __func__);
		while ((i < MAX_CHANNEL_NUM) && (chplan_sta[i].ChannelNum != 0))
		{
			DBG_871X("%02d(%c),", chplan_sta[i].ChannelNum, chplan_sta[i].ScanType==SCAN_PASSIVE?'p':'a');
			i++;
		}
		DBG_871X("}\n");
#endif
#endif

		_rtw_memset(pmlmeext->channel_set, 0, sizeof(pmlmeext->channel_set));
		chplan_new = pmlmeext->channel_set;

		i = j = k = 0;
		if (pregistrypriv->wireless_mode & WIRELESS_11G)
		{
			do {
				if ((i == MAX_CHANNEL_NUM) ||
					(chplan_sta[i].ChannelNum == 0) ||
					(chplan_sta[i].ChannelNum > 14))
					break;

				if ((j == chplan_ap.Len) || (chplan_ap.Channel[j] > 14))
					break;

				if (chplan_sta[i].ChannelNum == chplan_ap.Channel[j])
				{
					chplan_new[k].ChannelNum = chplan_ap.Channel[j];
					chplan_new[k].ScanType = SCAN_ACTIVE;
					i++;
					j++;
					k++;
				}
				else if (chplan_sta[i].ChannelNum < chplan_ap.Channel[j])
				{
					chplan_new[k].ChannelNum = chplan_sta[i].ChannelNum;
//					chplan_new[k].ScanType = chplan_sta[i].ScanType;
					chplan_new[k].ScanType = SCAN_PASSIVE;
					i++;
					k++;
				}
				else if (chplan_sta[i].ChannelNum > chplan_ap.Channel[j])
				{
					chplan_new[k].ChannelNum = chplan_ap.Channel[j];
					chplan_new[k].ScanType = SCAN_ACTIVE;
					j++;
					k++;
				}
			} while (1);

			// change AP not support channel to Passive scan
			while ((i < MAX_CHANNEL_NUM) &&
				(chplan_sta[i].ChannelNum != 0) &&
				(chplan_sta[i].ChannelNum <= 14))
			{
				chplan_new[k].ChannelNum = chplan_sta[i].ChannelNum;
//				chplan_new[k].ScanType = chplan_sta[i].ScanType;
				chplan_new[k].ScanType = SCAN_PASSIVE;
				i++;
				k++;
			}

			// add channel AP supported
			while ((j < chplan_ap.Len) && (chplan_ap.Channel[j] <= 14))
			{
				chplan_new[k].ChannelNum = chplan_ap.Channel[j];
				chplan_new[k].ScanType = SCAN_ACTIVE;
				j++;
				k++;
			}
		}
		else
		{
			// keep original STA 2.4G channel plan
			while ((i < MAX_CHANNEL_NUM) &&
				(chplan_sta[i].ChannelNum != 0) &&
				(chplan_sta[i].ChannelNum <= 14))
			{
				chplan_new[k].ChannelNum = chplan_sta[i].ChannelNum;
				chplan_new[k].ScanType = chplan_sta[i].ScanType;
				i++;
				k++;
			}

			// skip AP 2.4G channel plan
			while ((j < chplan_ap.Len) && (chplan_ap.Channel[j] <= 14))
			{
				j++;
			}
		}

		if (pregistrypriv->wireless_mode & WIRELESS_11A)
		{
			do {
				if ((i == MAX_CHANNEL_NUM) ||
					(chplan_sta[i].ChannelNum == 0))
					break;

				if ((j == chplan_ap.Len) || (chplan_ap.Channel[j] == 0))
					break;

				if (chplan_sta[i].ChannelNum == chplan_ap.Channel[j])
				{
					chplan_new[k].ChannelNum = chplan_ap.Channel[j];
					chplan_new[k].ScanType = SCAN_ACTIVE;
					i++;
					j++;
					k++;
				}
				else if (chplan_sta[i].ChannelNum < chplan_ap.Channel[j])
				{
					chplan_new[k].ChannelNum = chplan_sta[i].ChannelNum;
//					chplan_new[k].ScanType = chplan_sta[i].ScanType;
					chplan_new[k].ScanType = SCAN_PASSIVE;
					i++;
					k++;
				}
				else if (chplan_sta[i].ChannelNum > chplan_ap.Channel[j])
				{
					chplan_new[k].ChannelNum = chplan_ap.Channel[j];
					chplan_new[k].ScanType = SCAN_ACTIVE;
					j++;
					k++;
				}
			} while (1);

			// change AP not support channel to Passive scan
			while ((i < MAX_CHANNEL_NUM) && (chplan_sta[i].ChannelNum != 0))
			{
				chplan_new[k].ChannelNum = chplan_sta[i].ChannelNum;
//				chplan_new[k].ScanType = chplan_sta[i].ScanType;
				chplan_new[k].ScanType = SCAN_PASSIVE;
				i++;
				k++;
			}

			// add channel AP supported
			while ((j < chplan_ap.Len) && (chplan_ap.Channel[j] != 0))
			{
				chplan_new[k].ChannelNum = chplan_ap.Channel[j];
				chplan_new[k].ScanType = SCAN_ACTIVE;
				j++;
				k++;
			}
		}
		else
		{
			// keep original STA 5G channel plan
			while ((i < MAX_CHANNEL_NUM) && (chplan_sta[i].ChannelNum != 0))
			{
				chplan_new[k].ChannelNum = chplan_sta[i].ChannelNum;
				chplan_new[k].ScanType = chplan_sta[i].ScanType;
				i++;
				k++;
			}
		}

		pmlmeext->update_channel_plan_by_ap_done = 1;

#ifdef CONFIG_DEBUG_RTL871X
#ifdef PLATFORM_LINUX
		k = 0;
		DBG_871X("%s: new STA channel plan {", __func__);
		while ((k < MAX_CHANNEL_NUM) && (chplan_new[k].ChannelNum != 0))
		{
			DBG_871X("%02d(%c),", chplan_new[k].ChannelNum, chplan_new[k].ScanType==SCAN_PASSIVE?'p':'c');
			k++;
		}
		DBG_871X("}\n");
#endif
#endif

		#if 0
		// recover the right channel index
		channel = chplan_sta[pmlmeext->sitesurvey_res.channel_idx].ChannelNum;
		k = 0;
		while ((k < MAX_CHANNEL_NUM) && (chplan_new[k].ChannelNum != 0))
		{
			if (chplan_new[k].ChannelNum == channel) {
				RT_TRACE(_module_rtl871x_mlme_c_, _drv_notice_,
						 ("%s: change mlme_ext sitesurvey channel index from %d to %d\n",
						  __FUNCTION__, pmlmeext->sitesurvey_res.channel_idx, k));
				pmlmeext->sitesurvey_res.channel_idx = k;
				break;
			}
			k++;
		}
		#endif
	}

	// If channel is used by AP, set channel scan type to active
	channel = bssid->Configuration.DSConfig;
	chplan_new = pmlmeext->channel_set;
	i = 0;
	while ((i < MAX_CHANNEL_NUM) && (chplan_new[i].ChannelNum != 0))
	{
		if (chplan_new[i].ChannelNum == channel)
		{
			if (chplan_new[i].ScanType == SCAN_PASSIVE)
			{
				chplan_new[i].ScanType = SCAN_ACTIVE;
				RT_TRACE(_module_rtl871x_mlme_c_, _drv_notice_,
						 ("%s: change channel %d scan type from passive to active\n",
						  __FUNCTION__, channel));
			}
			break;
		}
		i++;
	}
}
#endif

/****************************************************************************

Following are the functions to report events

*****************************************************************************/

void report_survey_event(_adapter *padapter, union recv_frame *precv_frame)
{
	struct cmd_obj *pcmd_obj;
	u8	*pevtcmd;
	u32 cmdsz;
	struct survey_event	*psurvey_evt;
	struct C2HEvent_Header *pc2h_evt_hdr;
	struct mlme_ext_priv *pmlmeext;
	struct cmd_priv *pcmdpriv;
	//u8 *pframe = precv_frame->u.hdr.rx_data;
	//uint len = precv_frame->u.hdr.len;

	if(!padapter)
		return;

	pmlmeext = &padapter->mlmeextpriv;
	pcmdpriv = &padapter->cmdpriv;


	if ((pcmd_obj = (struct cmd_obj*)rtw_zmalloc(sizeof(struct cmd_obj))) == NULL)
	{
		return;
	}

	cmdsz = (sizeof(struct survey_event) + sizeof(struct C2HEvent_Header));
	if ((pevtcmd = (u8*)rtw_zmalloc(cmdsz)) == NULL)
	{
		rtw_mfree((u8 *)pcmd_obj, sizeof(struct cmd_obj));
		return;
	}

	_rtw_init_listhead(&pcmd_obj->list);

	pcmd_obj->cmdcode = GEN_CMD_CODE(_Set_MLME_EVT);
	pcmd_obj->cmdsz = cmdsz;
	pcmd_obj->parmbuf = pevtcmd;

	pcmd_obj->rsp = NULL;
	pcmd_obj->rspsz  = 0;

	pc2h_evt_hdr = (struct C2HEvent_Header*)(pevtcmd);
	pc2h_evt_hdr->len = sizeof(struct survey_event);
	pc2h_evt_hdr->ID = GEN_EVT_CODE(_Survey);
	pc2h_evt_hdr->seq = ATOMIC_INC_RETURN(&pmlmeext->event_seq);

	psurvey_evt = (struct survey_event*)(pevtcmd + sizeof(struct C2HEvent_Header));

	if (collect_bss_info(padapter, precv_frame, (WLAN_BSSID_EX *)&psurvey_evt->bss) == _FAIL)
	{
		rtw_mfree((u8 *)pcmd_obj, sizeof(struct cmd_obj));
		rtw_mfree((u8 *)pevtcmd, cmdsz);
		return;
	}

#ifdef CONFIG_80211D
	process_80211d(padapter, &psurvey_evt->bss);
#endif

	rtw_enqueue_cmd(pcmdpriv, pcmd_obj);

	pmlmeext->sitesurvey_res.bss_cnt++;

	return;

}

void report_surveydone_event(_adapter *padapter)
{
	struct cmd_obj *pcmd_obj;
	u8	*pevtcmd;
	u32 cmdsz;
	struct surveydone_event *psurveydone_evt;
	struct C2HEvent_Header	*pc2h_evt_hdr;
	struct mlme_ext_priv		*pmlmeext = &padapter->mlmeextpriv;
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;

	if ((pcmd_obj = (struct cmd_obj*)rtw_zmalloc(sizeof(struct cmd_obj))) == NULL)
	{
		return;
	}

	cmdsz = (sizeof(struct surveydone_event) + sizeof(struct C2HEvent_Header));
	if ((pevtcmd = (u8*)rtw_zmalloc(cmdsz)) == NULL)
	{
		rtw_mfree((u8 *)pcmd_obj, sizeof(struct cmd_obj));
		return;
	}

	_rtw_init_listhead(&pcmd_obj->list);

	pcmd_obj->cmdcode = GEN_CMD_CODE(_Set_MLME_EVT);
	pcmd_obj->cmdsz = cmdsz;
	pcmd_obj->parmbuf = pevtcmd;

	pcmd_obj->rsp = NULL;
	pcmd_obj->rspsz  = 0;

	pc2h_evt_hdr = (struct C2HEvent_Header*)(pevtcmd);
	pc2h_evt_hdr->len = sizeof(struct surveydone_event);
	pc2h_evt_hdr->ID = GEN_EVT_CODE(_SurveyDone);
	pc2h_evt_hdr->seq = ATOMIC_INC_RETURN(&pmlmeext->event_seq);

	psurveydone_evt = (struct surveydone_event*)(pevtcmd + sizeof(struct C2HEvent_Header));
	psurveydone_evt->bss_cnt = pmlmeext->sitesurvey_res.bss_cnt;

	DBG_871X("survey done event(%x)\n", psurveydone_evt->bss_cnt);

	rtw_enqueue_cmd(pcmdpriv, pcmd_obj);

	return;

}

void report_join_res(_adapter *padapter, int res)
{
	struct cmd_obj *pcmd_obj;
	u8	*pevtcmd;
	u32 cmdsz;
	struct joinbss_event		*pjoinbss_evt;
	struct C2HEvent_Header	*pc2h_evt_hdr;
	struct mlme_ext_priv		*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;

	if ((pcmd_obj = (struct cmd_obj*)rtw_zmalloc(sizeof(struct cmd_obj))) == NULL)
	{
		return;
	}

	cmdsz = (sizeof(struct joinbss_event) + sizeof(struct C2HEvent_Header));
	if ((pevtcmd = (u8*)rtw_zmalloc(cmdsz)) == NULL)
	{
		rtw_mfree((u8 *)pcmd_obj, sizeof(struct cmd_obj));
		return;
	}

	_rtw_init_listhead(&pcmd_obj->list);

	pcmd_obj->cmdcode = GEN_CMD_CODE(_Set_MLME_EVT);
	pcmd_obj->cmdsz = cmdsz;
	pcmd_obj->parmbuf = pevtcmd;

	pcmd_obj->rsp = NULL;
	pcmd_obj->rspsz  = 0;

	pc2h_evt_hdr = (struct C2HEvent_Header*)(pevtcmd);
	pc2h_evt_hdr->len = sizeof(struct joinbss_event);
	pc2h_evt_hdr->ID = GEN_EVT_CODE(_JoinBss);
	pc2h_evt_hdr->seq = ATOMIC_INC_RETURN(&pmlmeext->event_seq);

	pjoinbss_evt = (struct joinbss_event*)(pevtcmd + sizeof(struct C2HEvent_Header));
	_rtw_memcpy((unsigned char *)(&(pjoinbss_evt->network.network)), &(pmlmeinfo->network), sizeof(WLAN_BSSID_EX));
	pjoinbss_evt->network.join_res 	= pjoinbss_evt->network.aid = res;

	DBG_871X("report_join_res(%d)\n", res);


	rtw_joinbss_event_prehandle(padapter, (u8 *)&pjoinbss_evt->network);


	rtw_enqueue_cmd(pcmdpriv, pcmd_obj);

	return;

}

void report_del_sta_event(_adapter *padapter, unsigned char* MacAddr, unsigned short reason)
{
	struct cmd_obj *pcmd_obj;
	u8	*pevtcmd;
	u32 cmdsz;
	struct sta_info *psta;
	int	mac_id;
	struct stadel_event			*pdel_sta_evt;
	struct C2HEvent_Header	*pc2h_evt_hdr;
	struct mlme_ext_priv		*pmlmeext = &padapter->mlmeextpriv;
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;

	if ((pcmd_obj = (struct cmd_obj*)rtw_zmalloc(sizeof(struct cmd_obj))) == NULL)
	{
		return;
	}

	cmdsz = (sizeof(struct stadel_event) + sizeof(struct C2HEvent_Header));
	if ((pevtcmd = (u8*)rtw_zmalloc(cmdsz)) == NULL)
	{
		rtw_mfree((u8 *)pcmd_obj, sizeof(struct cmd_obj));
		return;
	}

	_rtw_init_listhead(&pcmd_obj->list);

	pcmd_obj->cmdcode = GEN_CMD_CODE(_Set_MLME_EVT);
	pcmd_obj->cmdsz = cmdsz;
	pcmd_obj->parmbuf = pevtcmd;

	pcmd_obj->rsp = NULL;
	pcmd_obj->rspsz  = 0;

	pc2h_evt_hdr = (struct C2HEvent_Header*)(pevtcmd);
	pc2h_evt_hdr->len = sizeof(struct stadel_event);
	pc2h_evt_hdr->ID = GEN_EVT_CODE(_DelSTA);
	pc2h_evt_hdr->seq = ATOMIC_INC_RETURN(&pmlmeext->event_seq);

	pdel_sta_evt = (struct stadel_event*)(pevtcmd + sizeof(struct C2HEvent_Header));
	_rtw_memcpy((unsigned char *)(&(pdel_sta_evt->macaddr)), MacAddr, ETH_ALEN);
	_rtw_memcpy((unsigned char *)(pdel_sta_evt->rsvd),(unsigned char *)(&reason),2);


	psta = rtw_get_stainfo(&padapter->stapriv, MacAddr);
	if(psta)
		mac_id = (int)psta->mac_id;
	else
		mac_id = (-1);

	pdel_sta_evt->mac_id = mac_id;

	DBG_871X("report_del_sta_event: delete STA, mac_id=%d\n", mac_id);

	rtw_enqueue_cmd(pcmdpriv, pcmd_obj);

	return;
}

void report_add_sta_event(_adapter *padapter, unsigned char* MacAddr, int cam_idx)
{
	struct cmd_obj *pcmd_obj;
	u8	*pevtcmd;
	u32 cmdsz;
	struct stassoc_event		*padd_sta_evt;
	struct C2HEvent_Header	*pc2h_evt_hdr;
	struct mlme_ext_priv		*pmlmeext = &padapter->mlmeextpriv;
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;

	if ((pcmd_obj = (struct cmd_obj*)rtw_zmalloc(sizeof(struct cmd_obj))) == NULL)
	{
		return;
	}

	cmdsz = (sizeof(struct stassoc_event) + sizeof(struct C2HEvent_Header));
	if ((pevtcmd = (u8*)rtw_zmalloc(cmdsz)) == NULL)
	{
		rtw_mfree((u8 *)pcmd_obj, sizeof(struct cmd_obj));
		return;
	}

	_rtw_init_listhead(&pcmd_obj->list);

	pcmd_obj->cmdcode = GEN_CMD_CODE(_Set_MLME_EVT);
	pcmd_obj->cmdsz = cmdsz;
	pcmd_obj->parmbuf = pevtcmd;

	pcmd_obj->rsp = NULL;
	pcmd_obj->rspsz  = 0;

	pc2h_evt_hdr = (struct C2HEvent_Header*)(pevtcmd);
	pc2h_evt_hdr->len = sizeof(struct stassoc_event);
	pc2h_evt_hdr->ID = GEN_EVT_CODE(_AddSTA);
	pc2h_evt_hdr->seq = ATOMIC_INC_RETURN(&pmlmeext->event_seq);

	padd_sta_evt = (struct stassoc_event*)(pevtcmd + sizeof(struct C2HEvent_Header));
	_rtw_memcpy((unsigned char *)(&(padd_sta_evt->macaddr)), MacAddr, ETH_ALEN);
	padd_sta_evt->cam_id = cam_idx;

	DBG_871X("report_add_sta_event: add STA\n");

	rtw_enqueue_cmd(pcmdpriv, pcmd_obj);

	return;
}


/****************************************************************************

Following are the event callback functions

*****************************************************************************/

//for sta/adhoc mode
void update_sta_info(_adapter *padapter, struct sta_info *psta)
{
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	//ERP
	VCS_update(padapter, psta);


	//HT
	if(pmlmepriv->htpriv.ht_option)
	{
		psta->htpriv.ht_option = _TRUE;

		psta->htpriv.ampdu_enable = pmlmepriv->htpriv.ampdu_enable;

		if (support_short_GI(padapter, &(pmlmeinfo->HT_caps)))
			psta->htpriv.sgi = _TRUE;

		psta->qos_option = _TRUE;

	}
	else
	{
		psta->htpriv.ht_option = _FALSE;

		psta->htpriv.ampdu_enable = _FALSE;

		psta->htpriv.sgi = _FALSE;

		psta->qos_option = _FALSE;

	}

	psta->htpriv.bwmode = pmlmeext->cur_bwmode;
	psta->htpriv.ch_offset = pmlmeext->cur_ch_offset;

	psta->htpriv.agg_enable_bitmap = 0x0;//reset
	psta->htpriv.candidate_tid_bitmap = 0x0;//reset


	//QoS
	if(pmlmepriv->qospriv.qos_option)
		psta->qos_option = _TRUE;


	psta->state = _FW_LINKED;

}

void mlmeext_joinbss_event_callback(_adapter *padapter, int join_res)
{
	struct sta_info		*psta, *psta_bmc;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	struct sta_priv		*pstapriv = &padapter->stapriv;
	u8	join_type;
	u16 media_status;

	if(join_res < 0)
	{
		join_type = 1;
		rtw_hal_set_hwreg(padapter, HW_VAR_MLME_JOIN, (u8 *)(&join_type));
		rtw_hal_set_hwreg(padapter, HW_VAR_BSSID, null_addr);

		//restore to initial setting.
		update_tx_basic_rate(padapter, padapter->registrypriv.wireless_mode);

		goto exit_mlmeext_joinbss_event_callback;
	}

	if((pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE)
	{
		//for bc/mc
		psta_bmc = rtw_get_bcmc_stainfo(padapter);
		if(psta_bmc)
		{
			pmlmeinfo->FW_sta_info[psta_bmc->mac_id].psta = psta_bmc;
			update_bmc_sta_support_rate(padapter, psta_bmc->mac_id);
			Update_RA_Entry(padapter, psta_bmc->mac_id);
		}
	}


	//turn on dynamic functions
	Switch_DM_Func(padapter, DYNAMIC_ALL_FUNC_ENABLE, _TRUE);

	// update IOT-releated issue
	update_IOT_info(padapter);

	rtw_hal_set_hwreg(padapter, HW_VAR_BASIC_RATE, cur_network->SupportedRates);

	//BCN interval
	rtw_hal_set_hwreg(padapter, HW_VAR_BEACON_INTERVAL, (u8 *)(&pmlmeinfo->bcn_interval));

	//udpate capability
	update_capinfo(padapter, pmlmeinfo->capability);

	//WMM, Update EDCA param
	WMMOnAssocRsp(padapter);

	//HT
	HTOnAssocRsp(padapter);


	//Set cur_channel&cur_bwmode&cur_ch_offset
#ifndef CONFIG_CONCURRENT_MODE
	set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
#endif

	psta = rtw_get_stainfo(pstapriv, cur_network->MacAddress);
	if (psta) //only for infra. mode
	{
		pmlmeinfo->FW_sta_info[psta->mac_id].psta = psta;

		//DBG_871X("set_sta_rate\n");

		//set per sta rate after updating HT cap.
		set_sta_rate(padapter, psta);

		#if (RATE_ADAPTIVE_SUPPORT==1)	//for 88E RA
		rtw_hal_set_hwreg(padapter,HW_VAR_TX_RPT_MAX_MACID, (u8*)&psta->mac_id);
		#endif
		media_status = (psta->mac_id<<8)|1; //  MACID|OPMODE: 1 means connect
		rtw_hal_set_hwreg(padapter,HW_VAR_H2C_MEDIA_STATUS_RPT,(u8 *)&media_status);
	}

	join_type = 2;
	rtw_hal_set_hwreg(padapter, HW_VAR_MLME_JOIN, (u8 *)(&join_type));

	if((pmlmeinfo->state&0x03) == WIFI_FW_STATION_STATE)
	{
		// correcting TSF
		correct_TSF(padapter, pmlmeext);

		//set_link_timer(pmlmeext, DISCONNECT_TO);
	}

#ifdef CONFIG_LPS
	if(get_iface_type(padapter) == IFACE_PORT0)
	{
		rtw_lps_ctrl_wk_cmd(padapter, LPS_CTRL_CONNECT, 0);
		if (padapter->securitypriv.dot11AuthAlgrthm != dot11AuthAlgrthm_8021X &&
			padapter->securitypriv.dot11AuthAlgrthm != dot11AuthAlgrthm_WAPI &&
			check_fwstate(&padapter->mlmepriv, WIFI_UNDER_WPS) != _TRUE) {
#ifdef CONFIG_BT_COEXIST
			rtw_lps_ctrl_wk_cmd(padapter, LPS_CTRL_BT_WIFI_MEDIA_STATUS_NOTIFY, 0);
#endif
		}
	}
#endif


exit_mlmeext_joinbss_event_callback:

#ifdef CONFIG_DUALMAC_CONCURRENT
	dc_handle_join_done(padapter, join_res);
#endif
#ifdef CONFIG_CONCURRENT_MODE
	concurrent_chk_joinbss_done(padapter, join_res);
#endif

	DBG_871X("=>%s\n", __FUNCTION__);

}

void mlmeext_sta_add_event_callback(_adapter *padapter, struct sta_info *psta)
{
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u8	join_type;

	DBG_871X("%s\n", __FUNCTION__);

	if((pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE)
	{
		if(pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS)//adhoc master or sta_count>1
		{
			//nothing to do
		}
		else//adhoc client
		{
			//update TSF Value
			//update_TSF(pmlmeext, pframe, len);

			// correcting TSF
			correct_TSF(padapter, pmlmeext);

			//start beacon
			if(send_beacon(padapter)==_FAIL)
			{
				pmlmeinfo->FW_sta_info[psta->mac_id].status = 0;

				pmlmeinfo->state ^= WIFI_FW_ADHOC_STATE;

				return;
			}

			pmlmeinfo->state |= WIFI_FW_ASSOC_SUCCESS;

		}

		join_type = 2;
		rtw_hal_set_hwreg(padapter, HW_VAR_MLME_JOIN, (u8 *)(&join_type));
	}

	pmlmeinfo->FW_sta_info[psta->mac_id].psta = psta;

	//rate radaptive
	Update_RA_Entry(padapter, psta->mac_id);

	//update adhoc sta_info
	update_sta_info(padapter, psta);

}

void mlmeext_sta_del_event_callback(_adapter *padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	if (is_client_associated_to_ap(padapter) || is_IBSS_empty(padapter))
	{
		//set_opmode_cmd(padapter, infra_client_with_mlme);

		rtw_hal_set_hwreg(padapter, HW_VAR_MLME_DISCONNECT, 0);
		rtw_hal_set_hwreg(padapter, HW_VAR_BSSID, null_addr);

		//restore to initial setting.
		update_tx_basic_rate(padapter, padapter->registrypriv.wireless_mode);

#ifdef CONFIG_DUALMAC_CONCURRENT
		dc_set_channel_bwmode_disconnect(padapter);
#else
#ifdef CONFIG_CONCURRENT_MODE
		if((check_buddy_fwstate(padapter, _FW_LINKED)) != _TRUE)
		{
#endif //CONFIG_CONCURRENT_MODE

		//switch to the 20M Hz mode after disconnect
		pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
		pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

		//SelectChannel(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset);
		set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);

#ifdef CONFIG_CONCURRENT_MODE
		}
#endif //CONFIG_CONCURRENT_MODE
#endif //CONFIG_DUALMAC_CONCURRENT

		flush_all_cam_entry(padapter);

		pmlmeinfo->state = WIFI_FW_NULL_STATE;

		//set MSR to no link state -> infra. mode
		Set_MSR(padapter, _HW_STATE_STATION_);

		_cancel_timer_ex(&pmlmeext->link_timer);

	}

}

/****************************************************************************

Following are the functions for the timer handlers

*****************************************************************************/
void _linked_rx_signal_strehgth_display(_adapter *padapter);
void _linked_rx_signal_strehgth_display(_adapter *padapter)
{
	struct mlme_ext_priv    *pmlmeext = &padapter->mlmeextpriv;
        struct mlme_ext_info    *pmlmeinfo = &(pmlmeext->mlmext_info);
	u8 mac_id;
	#if 0
	DBG_871X("============ linked status check ===================\n");
	DBG_871X("pathA Rx SNRdb:%d\n",padapter->recvpriv.RxSNRdB[0]);
	DBG_871X("pathA Rx PWDB:%d\n",padapter->recvpriv.rxpwdb);
	rtw_hal_get_def_var(padapter, HAL_DEF_UNDERCORATEDSMOOTHEDPWDB, &UndecoratedSmoothedPWDB);
	DBG_871X("UndecoratedSmoothedPWDB:%d\n",UndecoratedSmoothedPWDB);
	DBG_871X("Rx RSSI:%d\n",padapter->recvpriv.rssi);
	DBG_871X("Rx Signal_strength:%d\n",padapter->recvpriv.signal_strength);
	DBG_871X("Rx Signal_qual:%d \n",padapter->recvpriv.signal_qual);
	if ( check_fwstate( &padapter->mlmepriv,  _FW_LINKED ) )
	{
		DBG_871X("bw mode: %d, channel: %d\n", padapter->mlmeextpriv.cur_bwmode, padapter->mlmeextpriv.cur_channel );
	}
	DBG_871X("============ linked status check ===================\n");
	#endif
	if((pmlmeinfo->state&0x03) == WIFI_FW_STATION_STATE)
	{
		mac_id=0;
	}
	else if((pmlmeinfo->state&0x03) == _HW_STATE_AP_)
	{
		mac_id=2;
	}

	rtw_hal_get_def_var(padapter, HW_DEF_RA_INFO_DUMP,&mac_id);

	DBG_871X("============ RX GAIN / FALSE ALARM  ===================\n");
	DBG_871X(" DIG PATH-A(0x%02x), PATH-B(0x%02x)\n",rtw_read8(padapter,0xc50),rtw_read8(padapter,0xc58));
	DBG_871X(" OFDM -Alarm DA2(0x%04x),DA4(0x%04x),DA6(0x%04x),DA8(0x%04x)\n",
		rtw_read16(padapter,0xDA2),rtw_read16(padapter,0xDA4),rtw_read16(padapter,0xDA6),rtw_read16(padapter,0xDA8));

	DBG_871X(" CCK -Alarm A5B(0x%02x),A5C(0x%02x)\n",rtw_read8(padapter,0xA5B),rtw_read8(padapter,0xA5C));


}

void linked_status_chk(_adapter *padapter)
{
	u32	i;
	struct sta_info		*psta;
	struct xmit_priv		*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct sta_priv		*pstapriv = &padapter->stapriv;
	static u32 nobcn_times = 0;
	//
	if(padapter->bRxRSSIDisplay)
		 _linked_rx_signal_strehgth_display(padapter);

	if (is_client_associated_to_ap(padapter))
	{
		//linked infrastructure client mode
		if ((psta = rtw_get_stainfo(pstapriv, pmlmeinfo->network.MacAddress)) != NULL)
		{
			#ifdef DBG_EXPIRATION_CHK
			DBG_871X("%s rx:"STA_PKTS_FMT", BI:%u, retry:%u\n"
				, __FUNCTION__
				, STA_RX_PKTS_ARG(psta)
				, pmlmeinfo->bcn_interval
				, pmlmeext->retry
			);
			#endif
#ifndef CONFIG_DISCONNECT_H2CWAY
			/*to monitor whether the AP is alive or not*/
			if (sta_last_rx_pkts(psta) == sta_rx_pkts(psta))
			{
				//	Commented by Albert 2010/07/21
				//	In this case, there is no any rx packet received by driver.

				if(pmlmeext->retry < DISCONNECT_LIMIT)// Alter the retry limit to 8
				{
					if(pmlmeext->retry==0)
					{
						#ifdef DBG_CONFIG_ERROR_DETECT
						rtw_hal_sreset_linked_status_check(padapter);
						#endif

						//	In order to know the AP's current state, try to send the probe request
						//	to trigger the AP to send the probe response.
						#ifdef CONFIG_P2P
						if(!rtw_p2p_chk_state(&padapter->wdinfo, P2P_STATE_NONE)) {
							#ifdef DBG_EXPIRATION_CHK
							DBG_871X("issue_probereq_p2p to trigger probersp, retry=%d\n", pmlmeext->retry);
							#endif
							issue_probereq_p2p(padapter);
						} else
						#endif
						{
							DBG_871X("issue_probereq to trigger probersp, retry=%d\n", pmlmeext->retry);
							issue_probereq(padapter, &(pmlmeinfo->network.Ssid), 0);
							issue_probereq(padapter, &(pmlmeinfo->network.Ssid), 0);
							issue_probereq(padapter, &(pmlmeinfo->network.Ssid), 0);
						}
					}

					pmlmeext->retry++;
				}
				else
				{
					pmlmeext->retry = 0;
					DBG_871X_LEVEL(_drv_always_, "no beacon for a long time, disconnect \n");
					receive_disconnect(padapter, pmlmeinfo->network.MacAddress, 3
						//, 65535// there is no need to roaming when when nobcn long time
					);
					pmlmeinfo->link_count = 0;
					return;
				}
			}
			else
			{
				pmlmeext->retry = 0;
				sta_update_last_rx_pkts(psta);
				//set_link_timer(pmlmeext, DISCONNECT_TO);
			}
#else
			/*to monitor whether the AP is alive or not*/
			if (sta_last_rx_pkts(psta) == sta_rx_pkts(psta))
			{
				if(nobcn_times < (TRY_AP_TIMES-1))
				{
					nobcn_times++;
					DBG_871X("%s nobcn_times:%d\n", __FUNCTION__, nobcn_times);
				}
				else if((pmlmeext->sitesurvey_res.state == SCAN_DISABLE))
				{
					nobcn_times++;
					DBG_871X("%s nobcn_times:%d\n", __FUNCTION__, nobcn_times);
					if (nobcn_times == TRY_AP_TIMES)
					{
						pmlmeext->check_ap_processing = _TRUE;
						//becaue in lps mode, we even send probe req, resp may cannot be received,
						//and in no lps mode, this case will not happen, so we should ask fw is link is keep.

						DBG_871X("%s no beacon and unicast ask fw reset tx/rx stats\n",__FUNCTION__);
						if (padapter->HalFunc.fw_try_ap_cmd)
						{
							padapter->HalFunc.fw_try_ap_cmd(padapter, 0);
							pmlmeext->try_ap_c2h_wait = _FALSE;
						}

						DBG_871X("%s no beacon and unicast Driver try AP now\n",__FUNCTION__);
						if (adapter_to_pwrctl(padapter)->pwr_mode == PS_MODE_ACTIVE)
						{
							issue_nulldata(padapter, 0);
							issue_nulldata(padapter, 0);
						}
						else
						{
							issue_nulldata(padapter, 1);
							issue_nulldata(padapter, 1);
						}
					} else if (nobcn_times == TRY_AP_TIMES + 1) {
						//becaue in lps mode, we even send probe req, resp may cannot be received,
						//and in no lps mode, this case will not happen, so we should ask fw is link is keep.

						DBG_871X_LEVEL(_drv_warning_, "%s no beacon and unicast ask fw try AP now\n",__FUNCTION__);
						if (padapter->HalFunc.fw_try_ap_cmd)
						{
							padapter->HalFunc.fw_try_ap_cmd(padapter, 1);
							pmlmeext->try_ap_c2h_wait = _TRUE;
						}
						pmlmeext->check_ap_processing = _FALSE;
						nobcn_times = 0;
					}
				}
			}
			else
			{
				pmlmeext->check_ap_processing = _FALSE;
				nobcn_times = 0;
				sta_update_last_rx_pkts(psta);
			}
#endif

			#ifdef DBG_EXPIRATION_CHK
			DBG_871X("%s tx_pkts:%llu, link_count:%u\n", __FUNCTION__
				, pxmitpriv->tx_pkts
				, pmlmeinfo->link_count
			);
			#endif

			/*to send the AP a nulldata if no frame is xmitted in order to keep alive*/
			if(pxmitpriv->last_tx_pkts == pxmitpriv->tx_pkts)
			{
				if(pmlmeinfo->link_count++ == 0xf)
				{
					//DBG_871X("(Interface %d)issue nulldata to keep alive\n", adapter_to_dvobj(padapter)->InterfaceNumber);
					#ifdef DBG_EXPIRATION_CHK
					DBG_871X("%s issue_nulldata 0\n", __FUNCTION__);
					#endif
					issue_nulldata(padapter, 0);
					pmlmeinfo->link_count = 0;
				}
			}
			else
			{
				pxmitpriv->last_tx_pkts = pxmitpriv->tx_pkts;
				pmlmeinfo->link_count = 0;
			}

		} //end of if ((psta = rtw_get_stainfo(pstapriv, passoc_res->network.MacAddress)) != NULL)
	}
	else if (is_client_associated_to_ibss(padapter))
	{
		//linked IBSS mode
		//for each assoc list entry to check the rx pkt counter
		for (i = IBSS_START_MAC_ID; i < NUM_STA; i++)
		{
			if (pmlmeinfo->FW_sta_info[i].status == 1)
			{
				psta = pmlmeinfo->FW_sta_info[i].psta;

				if(NULL==psta) continue;

				if (pmlmeinfo->FW_sta_info[i].rx_pkt == sta_rx_pkts(psta))
				{

					if(pmlmeinfo->FW_sta_info[i].retry<3)
					{
						pmlmeinfo->FW_sta_info[i].retry++;
					}
					else
					{
						pmlmeinfo->FW_sta_info[i].retry = 0;
						pmlmeinfo->FW_sta_info[i].status = 0;
						report_del_sta_event(padapter, psta->hwaddr
							, 65535// indicate disconnect caused by no rx
						);
					}
				}
				else
				{
					pmlmeinfo->FW_sta_info[i].retry = 0;
					pmlmeinfo->FW_sta_info[i].rx_pkt = (u32)sta_rx_pkts(psta);
				}
			}
		}

		//set_link_timer(pmlmeext, DISCONNECT_TO);

	}

}

void survey_timer_hdl(_adapter *padapter)
{
	struct cmd_obj	*ph2c;
	struct sitesurvey_parm	*psurveyPara;
	struct cmd_priv					*pcmdpriv=&padapter->cmdpriv;
	struct mlme_ext_priv 		*pmlmeext = &padapter->mlmeextpriv;
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo= &(padapter->wdinfo);
#endif

	//DBG_871X("marc: survey timer\n");
#ifdef PLATFORM_FREEBSD
	rtw_mtx_lock(NULL);
	 if (callout_pending(&padapter->mlmeextpriv.survey_timer.callout)) {
		 /* callout was reset */
		 //mtx_unlock(&sc->sc_mtx);
		 rtw_mtx_unlock(NULL);
		 return;
	 }
	 if (!callout_active(&padapter->mlmeextpriv.survey_timer.callout)) {
		 /* callout was stopped */
		 //mtx_unlock(&sc->sc_mtx);
		 rtw_mtx_unlock(NULL);
		 return;
	 }
	 callout_deactivate(&padapter->mlmeextpriv.survey_timer.callout);


#endif

	//issue rtw_sitesurvey_cmd
	if (pmlmeext->sitesurvey_res.state > SCAN_START)
	{
		if(pmlmeext->sitesurvey_res.state ==  SCAN_PROCESS)
		{
#ifdef CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
			if( padapter->mlmeextpriv.mlmext_info.scan_cnt != RTW_SCAN_NUM_OF_CH )
#endif //CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
				pmlmeext->sitesurvey_res.channel_idx++;
		}

		if(pmlmeext->scan_abort == _TRUE)
		{
			#ifdef CONFIG_P2P
			if(!rtw_p2p_chk_state(&padapter->wdinfo, P2P_STATE_NONE))
			{
				rtw_p2p_findphase_ex_set(pwdinfo, P2P_FINDPHASE_EX_MAX);
				pmlmeext->sitesurvey_res.channel_idx = 3;
				DBG_871X("%s idx:%d, cnt:%u\n", __FUNCTION__
					, pmlmeext->sitesurvey_res.channel_idx
					, pwdinfo->find_phase_state_exchange_cnt
				);
			}
			else
			#endif
			{
				pmlmeext->sitesurvey_res.channel_idx = pmlmeext->sitesurvey_res.ch_num;
				DBG_871X("%s idx:%d\n", __FUNCTION__
					, pmlmeext->sitesurvey_res.channel_idx
				);
			}

			pmlmeext->scan_abort = _FALSE;//reset
		}

		if ((ph2c = (struct cmd_obj*)rtw_zmalloc(sizeof(struct cmd_obj))) == NULL)
		{
			goto exit_survey_timer_hdl;
		}

		if ((psurveyPara = (struct sitesurvey_parm*)rtw_zmalloc(sizeof(struct sitesurvey_parm))) == NULL)
		{
			rtw_mfree((unsigned char *)ph2c, sizeof(struct cmd_obj));
			goto exit_survey_timer_hdl;
		}

		init_h2fwcmd_w_parm_no_rsp(ph2c, psurveyPara, GEN_CMD_CODE(_SiteSurvey));
		rtw_enqueue_cmd(pcmdpriv, ph2c);
	}


exit_survey_timer_hdl:
#ifdef PLATFORM_FREEBSD
		rtw_mtx_unlock(NULL);
#endif

	return;
}

void link_timer_hdl(_adapter *padapter)
{
	//static unsigned int		rx_pkt = 0;
	//static u64				tx_cnt = 0;
	//struct xmit_priv		*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	//struct sta_priv		*pstapriv = &padapter->stapriv;

#ifdef PLATFORM_FREEBSD
	rtw_mtx_lock(NULL);
	 if (callout_pending(&padapter->mlmeextpriv.survey_timer.callout)) {
		 /* callout was reset */
		 //mtx_unlock(&sc->sc_mtx);
		 rtw_mtx_unlock(NULL);
		 return;
	 }
	 if (!callout_active(&padapter->mlmeextpriv.survey_timer.callout)) {
		 /* callout was stopped */
		 //mtx_unlock(&sc->sc_mtx);
		 rtw_mtx_unlock(NULL);
		 return;
	 }
	 callout_deactivate(&padapter->mlmeextpriv.survey_timer.callout);


#endif

	if (pmlmeinfo->state & WIFI_FW_AUTH_NULL)
	{
		DBG_871X("link_timer_hdl:no beacon while connecting\n");
		pmlmeinfo->state = WIFI_FW_NULL_STATE;
		report_join_res(padapter, -3);
	}
	else if (pmlmeinfo->state & WIFI_FW_AUTH_STATE)
	{
		//re-auth timer
		if (++pmlmeinfo->reauth_count > REAUTH_LIMIT)
		{
			//if (pmlmeinfo->auth_algo != dot11AuthAlgrthm_Auto)
			//{
				pmlmeinfo->state = 0;
				report_join_res(padapter, -1);
				return;
			//}
			//else
			//{
			//	pmlmeinfo->auth_algo = dot11AuthAlgrthm_Shared;
			//	pmlmeinfo->reauth_count = 0;
			//}
		}

		DBG_871X("link_timer_hdl: auth timeout and try again\n");
		pmlmeinfo->auth_seq = 1;
		issue_auth(padapter, NULL, 0);
		set_link_timer(pmlmeext, REAUTH_TO);
	}
	else if (pmlmeinfo->state & WIFI_FW_ASSOC_STATE)
	{
		//re-assoc timer
		if (++pmlmeinfo->reassoc_count > REASSOC_LIMIT)
		{
			pmlmeinfo->state = WIFI_FW_NULL_STATE;
			report_join_res(padapter, -2);
			return;
		}

		DBG_871X("link_timer_hdl: assoc timeout and try again\n");
		issue_assocreq(padapter);
		set_link_timer(pmlmeext, REASSOC_TO);
	}
#if 0
	else if (is_client_associated_to_ap(padapter))
	{
		//linked infrastructure client mode
		if ((psta = rtw_get_stainfo(pstapriv, pmlmeinfo->network.MacAddress)) != NULL)
		{
			/*to monitor whether the AP is alive or not*/
			if (rx_pkt == psta->sta_stats.rx_pkts)
			{
				receive_disconnect(padapter, pmlmeinfo->network.MacAddress);
				return;
			}
			else
			{
				rx_pkt = psta->sta_stats.rx_pkts;
				set_link_timer(pmlmeext, DISCONNECT_TO);
			}

			//update the EDCA paramter according to the Tx/RX mode
			update_EDCA_param(padapter);

			/*to send the AP a nulldata if no frame is xmitted in order to keep alive*/
			if (pmlmeinfo->link_count++ == 0)
			{
				tx_cnt = pxmitpriv->tx_pkts;
			}
			else if ((pmlmeinfo->link_count & 0xf) == 0)
			{
				if (tx_cnt == pxmitpriv->tx_pkts)
				{
					issue_nulldata(padapter, 0);
				}

				tx_cnt = pxmitpriv->tx_pkts;
			}
		} //end of if ((psta = rtw_get_stainfo(pstapriv, passoc_res->network.MacAddress)) != NULL)
	}
	else if (is_client_associated_to_ibss(padapter))
	{
		//linked IBSS mode
		//for each assoc list entry to check the rx pkt counter
		for (i = IBSS_START_MAC_ID; i < NUM_STA; i++)
		{
			if (pmlmeinfo->FW_sta_info[i].status == 1)
			{
				psta = pmlmeinfo->FW_sta_info[i].psta;

				if (pmlmeinfo->FW_sta_info[i].rx_pkt == psta->sta_stats.rx_pkts)
				{
					pmlmeinfo->FW_sta_info[i].status = 0;
					report_del_sta_event(padapter, psta->hwaddr);
				}
				else
				{
					pmlmeinfo->FW_sta_info[i].rx_pkt = psta->sta_stats.rx_pkts;
				}
			}
		}

		set_link_timer(pmlmeext, DISCONNECT_TO);
	}
#endif

#ifdef PLATFORM_FREEBSD
	rtw_mtx_unlock(NULL);
#endif

	return;
}

void addba_timer_hdl(struct sta_info *psta)
{
	struct ht_priv	*phtpriv;

	if(!psta)
		return;

	phtpriv = &psta->htpriv;

	if((phtpriv->ht_option==_TRUE) && (phtpriv->ampdu_enable==_TRUE))
	{
		if(phtpriv->candidate_tid_bitmap)
			phtpriv->candidate_tid_bitmap=0x0;

	}

}

u8 NULL_hdl(_adapter *padapter, u8 *pbuf)
{
	return H2C_SUCCESS;
}

u8 setopmode_hdl(_adapter *padapter, u8 *pbuf)
{
	u8	type;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct setopmode_parm *psetop = (struct setopmode_parm *)pbuf;

	if(psetop->mode == Ndis802_11APMode)
	{
		pmlmeinfo->state = WIFI_FW_AP_STATE;
		type = _HW_STATE_AP_;
#ifdef CONFIG_NATIVEAP_MLME
		//start_ap_mode(padapter);
#endif
	}
	else if(psetop->mode == Ndis802_11Infrastructure)
	{
		pmlmeinfo->state &= ~(BIT(0)|BIT(1));// clear state
		pmlmeinfo->state |= WIFI_FW_STATION_STATE;//set to 	STATION_STATE
		type = _HW_STATE_STATION_;
	}
	else if(psetop->mode == Ndis802_11IBSS)
	{
		type = _HW_STATE_ADHOC_;
	}
	else
	{
		type = _HW_STATE_NOLINK_;
	}

	rtw_hal_set_hwreg(padapter, HW_VAR_SET_OPMODE, (u8 *)(&type));
	//Set_NETYPE0_MSR(padapter, type);

	return H2C_SUCCESS;

}

u8 createbss_hdl(_adapter *padapter, u8 *pbuf)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX	*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
	struct joinbss_parm *pparm = (struct joinbss_parm *)pbuf;
	//u32	initialgain;


	if(pparm->network.InfrastructureMode == Ndis802_11APMode)
	{
#ifdef CONFIG_AP_MODE

		if(pmlmeinfo->state == WIFI_FW_AP_STATE)
		{
			//todo:
			return H2C_SUCCESS;
		}
#endif
	}

	//below is for ad-hoc master
	if(pparm->network.InfrastructureMode == Ndis802_11IBSS)
	{
		rtw_joinbss_reset(padapter);

		pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
		pmlmeext->cur_ch_offset= HAL_PRIME_CHNL_OFFSET_DONT_CARE;
		pmlmeinfo->ERP_enable = 0;
		pmlmeinfo->WMM_enable = 0;
		pmlmeinfo->HT_enable = 0;
		pmlmeinfo->HT_caps_enable = 0;
		pmlmeinfo->HT_info_enable = 0;
		pmlmeinfo->agg_enable_bitmap = 0;
		pmlmeinfo->candidate_tid_bitmap = 0;

		//disable dynamic functions, such as high power, DIG
		Save_DM_Func_Flag(padapter);
		Switch_DM_Func(padapter, DYNAMIC_FUNC_DISABLE, _FALSE);

		//config the initial gain under linking, need to write the BB registers
		//initialgain = 0x1E;
		//rtw_hal_set_hwreg(padapter, HW_VAR_INITIAL_GAIN, (u8 *)(&initialgain));

		//cancel link timer
		_cancel_timer_ex(&pmlmeext->link_timer);

		//clear CAM
		flush_all_cam_entry(padapter);

		_rtw_memcpy(pnetwork, pbuf, FIELD_OFFSET(WLAN_BSSID_EX, IELength));
		pnetwork->IELength = ((WLAN_BSSID_EX *)pbuf)->IELength;

		if(pnetwork->IELength>MAX_IE_SZ)//Check pbuf->IELength
			return H2C_PARAMETERS_ERROR;

		_rtw_memcpy(pnetwork->IEs, ((WLAN_BSSID_EX *)pbuf)->IEs, pnetwork->IELength);

		start_create_ibss(padapter);

	}

	return H2C_SUCCESS;

}

u8 join_cmd_hdl(_adapter *padapter, u8 *pbuf)
{
	u8	join_type;
	PNDIS_802_11_VARIABLE_IEs	pIE;
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
#ifdef CONFIG_ANTENNA_DIVERSITY
	struct joinbss_parm	*pparm = (struct joinbss_parm *)pbuf;
#endif //CONFIG_ANTENNA_DIVERSITY
	u32 i;
        //u32	initialgain;
	//u32	acparm;

	//check already connecting to AP or not
	if (pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS)
	{
		if (pmlmeinfo->state & WIFI_FW_STATION_STATE)
		{
			issue_deauth(padapter, pnetwork->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
		}

		pmlmeinfo->state = WIFI_FW_NULL_STATE;

		//clear CAM
		flush_all_cam_entry(padapter);

		_cancel_timer_ex(&pmlmeext->link_timer);

		//set MSR to nolink -> infra. mode
		//Set_MSR(padapter, _HW_STATE_NOLINK_);
		Set_MSR(padapter, _HW_STATE_STATION_);


		rtw_hal_set_hwreg(padapter, HW_VAR_MLME_DISCONNECT, 0);
	}

#ifdef CONFIG_ANTENNA_DIVERSITY
	rtw_antenna_select_cmd(padapter, pparm->network.PhyInfo.Optimum_antenna, _FALSE);
#endif

#ifdef CONFIG_WAPI_SUPPORT
	rtw_wapi_clear_all_cam_entry(padapter);
#endif

	rtw_joinbss_reset(padapter);

	pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
	pmlmeext->cur_ch_offset= HAL_PRIME_CHNL_OFFSET_DONT_CARE;
	pmlmeinfo->ERP_enable = 0;
	pmlmeinfo->WMM_enable = 0;
	pmlmeinfo->HT_enable = 0;
	pmlmeinfo->HT_caps_enable = 0;
	pmlmeinfo->HT_info_enable = 0;
	pmlmeinfo->agg_enable_bitmap = 0;
	pmlmeinfo->candidate_tid_bitmap = 0;
	pmlmeinfo->bwmode_updated = _FALSE;
	//pmlmeinfo->assoc_AP_vendor = HT_IOT_PEER_MAX;

	//NOTICE: pbuf is copy from scan list network, but not all IEs are inclued
	_rtw_memcpy(pnetwork, pbuf, FIELD_OFFSET(WLAN_BSSID_EX, IELength));
	pnetwork->IELength = ((WLAN_BSSID_EX *)pbuf)->IELength;

	if(pnetwork->IELength>MAX_IE_SZ)//Check pbuf->IELength
		return H2C_PARAMETERS_ERROR;

	_rtw_memcpy(pnetwork->IEs, ((WLAN_BSSID_EX *)pbuf)->IEs, pnetwork->IELength);

	//Check AP vendor to move rtw_joinbss_cmd()
	//pmlmeinfo->assoc_AP_vendor = check_assoc_AP(pnetwork->IEs, pnetwork->IELength);

	for (i = sizeof(NDIS_802_11_FIXED_IEs); i < pnetwork->IELength;)
	{
		pIE = (PNDIS_802_11_VARIABLE_IEs)(pnetwork->IEs + i);

		switch (pIE->ElementID)
		{
			case _VENDOR_SPECIFIC_IE_://Get WMM IE.
				if ( _rtw_memcmp(pIE->data, WMM_OUI, 4) )
				{
					pmlmeinfo->WMM_enable = 1;
				}
				break;

			case _HT_CAPABILITY_IE_:	//Get HT Cap IE.
				pmlmeinfo->HT_caps_enable = 1;
				break;

			case _HT_EXTRA_INFO_IE_:	//Get HT Info IE.
				pmlmeinfo->HT_info_enable = 1;

				//spec case only for cisco's ap because cisco's ap issue assoc rsp using mcs rate @40MHz or @20MHz
//#if !defined(CONFIG_CONCURRENT_MODE) && !defined(CONFIG_DUALMAC_CONCURRENT)
//				if(pmlmeinfo->assoc_AP_vendor == ciscoAP)
//#endif
				{
					struct HT_info_element *pht_info = (struct HT_info_element *)(pIE->data);

					if ((pregpriv->cbw40_enable) &&	 (pht_info->infos[0] & BIT(2)))
					{
						//switch to the 40M Hz mode according to the AP
						pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_40;
						switch (pht_info->infos[0] & 0x3)
						{
							case 1:
								pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_LOWER;
								break;

							case 3:
								pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_UPPER;
								break;

							default:
								pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
								break;
						}

						DBG_871X("set ch/bw before connected\n");
					}
				}
				break;

			default:
				break;
		}

		i += (pIE->Length + 2);
	}
#if 0
	if (padapter->registrypriv.wifi_spec) {
		// for WiFi test, follow WMM test plan spec
		acparm = 0x002F431C; // VO
		rtw_hal_set_hwreg(padapter, HW_VAR_AC_PARAM_VO, (u8 *)(&acparm));
		acparm = 0x005E541C; // VI
		rtw_hal_set_hwreg(padapter, HW_VAR_AC_PARAM_VI, (u8 *)(&acparm));
		acparm = 0x0000A525; // BE
		rtw_hal_set_hwreg(padapter, HW_VAR_AC_PARAM_BE, (u8 *)(&acparm));
		acparm = 0x0000A549; // BK
		rtw_hal_set_hwreg(padapter, HW_VAR_AC_PARAM_BK, (u8 *)(&acparm));

		// for WiFi test, mixed mode with intel STA under bg mode throughput issue
		if (padapter->mlmepriv.htpriv.ht_option == _FALSE){
			acparm = 0x00004320;
			rtw_hal_set_hwreg(padapter, HW_VAR_AC_PARAM_BE, (u8 *)(&acparm));
		}
	}
	else {
		acparm = 0x002F3217; // VO
		rtw_hal_set_hwreg(padapter, HW_VAR_AC_PARAM_VO, (u8 *)(&acparm));
		acparm = 0x005E4317; // VI
		rtw_hal_set_hwreg(padapter, HW_VAR_AC_PARAM_VI, (u8 *)(&acparm));
		acparm = 0x00105320; // BE
		rtw_hal_set_hwreg(padapter, HW_VAR_AC_PARAM_BE, (u8 *)(&acparm));
		acparm = 0x0000A444; // BK
		rtw_hal_set_hwreg(padapter, HW_VAR_AC_PARAM_BK, (u8 *)(&acparm));
	}
#endif
	//disable dynamic functions, such as high power, DIG
	//Switch_DM_Func(padapter, DYNAMIC_FUNC_DISABLE, _FALSE);

	//config the initial gain under linking, need to write the BB registers
	//initialgain = 0x1E;
	//rtw_hal_set_hwreg(padapter, HW_VAR_INITIAL_GAIN, (u8 *)(&initialgain));

	rtw_hal_set_hwreg(padapter, HW_VAR_BSSID, pmlmeinfo->network.MacAddress);
	join_type = 0;
	rtw_hal_set_hwreg(padapter, HW_VAR_MLME_JOIN, (u8 *)(&join_type));

	//cancel link timer
	_cancel_timer_ex(&pmlmeext->link_timer);

	start_clnt_join(padapter);

	return H2C_SUCCESS;

}

u8 disconnect_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
	u8	val8;

	if (is_client_associated_to_ap(padapter))
	{
		issue_deauth(padapter, pnetwork->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
	}

	//set_opmode_cmd(padapter, infra_client_with_mlme);

	//pmlmeinfo->state = WIFI_FW_NULL_STATE;


	rtw_hal_set_hwreg(padapter, HW_VAR_MLME_DISCONNECT, 0);
	rtw_hal_set_hwreg(padapter, HW_VAR_BSSID, null_addr);

	//restore to initial setting.
	update_tx_basic_rate(padapter, padapter->registrypriv.wireless_mode);

	if(((pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE) || ((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE))
	{
		//Stop BCN
		val8 = 0;
		rtw_hal_set_hwreg(padapter, HW_VAR_BCN_FUNC, (u8 *)(&val8));
	}


	//set MSR to no link state -> infra. mode
	Set_MSR(padapter, _HW_STATE_STATION_);

	pmlmeinfo->state = WIFI_FW_NULL_STATE;

#ifdef CONFIG_DUALMAC_CONCURRENT
	dc_set_channel_bwmode_disconnect(padapter);
#else
#ifdef CONFIG_CONCURRENT_MODE
	if((check_buddy_fwstate(padapter, _FW_LINKED)) != _TRUE)
	{
#endif //CONFIG_CONCURRENT_MODE
		//switch to the 20M Hz mode after disconnect
		pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
		pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

	set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
#ifdef CONFIG_CONCURRENT_MODE
	}
#endif //CONFIG_CONCURRENT_MODE
#endif //CONFIG_DUALMAC_CONCURRENT

	flush_all_cam_entry(padapter);

	_cancel_timer_ex(&pmlmeext->link_timer);

	rtw_free_uc_swdec_pending_queue(padapter);

	return 	H2C_SUCCESS;
}

int rtw_scan_ch_decision(_adapter *padapter, struct rtw_ieee80211_channel *out,
	u32 out_num, struct rtw_ieee80211_channel *in, u32 in_num)
{
	int i, j;
	int set_idx;
	int k,idx=-1,set_idx_cur=-1;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;

	/* clear out first */
	_rtw_memset(out, 0, sizeof(struct rtw_ieee80211_channel)*out_num);

	/* acquire channels from in */
	j = 0;
#if  defined(ANDROID_2X)
	for(k=0;k<in_num;k++)
     	{
     		if(in[k].hw_value && (in[k].hw_value ==pmlmeext->cur_channel)
			&&!(in[k].flags & RTW_IEEE80211_CHAN_DISABLED))
     		{
     			if(set_idx_cur =rtw_ch_set_search_ch(pmlmeext->channel_set, pmlmeext->cur_channel)>0)
     			{
     				_rtw_memcpy(&out[j], &in[k], sizeof(struct rtw_ieee80211_channel));
				if(pmlmeext->channel_set[set_idx_cur].ScanType == SCAN_PASSIVE)
					out[j].flags &= RTW_IEEE80211_CHAN_PASSIVE_SCAN;
				j++;
				break;

     			}
     		}

     	}
#endif
	for (i=0;i<in_num;i++) {
		DBG_871X(FUNC_ADPT_FMT" "CHAN_FMT"\n", FUNC_ADPT_ARG(padapter), CHAN_ARG(&in[i]));
		if(in[i].hw_value && !(in[i].flags & RTW_IEEE80211_CHAN_DISABLED)
			&& (set_idx=rtw_ch_set_search_ch(pmlmeext->channel_set, in[i].hw_value)) >=0
		)
		{
#if  defined(ANDROID_2X)
		if((set_idx_cur>=0)&&(k==i))
				continue;
#endif
			_rtw_memcpy(&out[j], &in[i], sizeof(struct rtw_ieee80211_channel));

			if(pmlmeext->channel_set[set_idx].ScanType == SCAN_PASSIVE)
				out[j].flags &= RTW_IEEE80211_CHAN_PASSIVE_SCAN;

			j++;
		}
		if(j>=out_num)
			break;
	}

	/* if out is empty, use channel_set as default */
	if(j == 0) {
		for (i=0;i<pmlmeext->max_chan_nums;i++) {
			out[i].hw_value = pmlmeext->channel_set[i].ChannelNum;
#if  defined(ANDROID_2X)
			if(pmlmeext->cur_channel>0&&out[i].hw_value ==pmlmeext->cur_channel)
				idx =i;
#endif
			if(pmlmeext->channel_set[i].ScanType == SCAN_PASSIVE)
				out[i].flags &= RTW_IEEE80211_CHAN_PASSIVE_SCAN;

			j++;
		}
#if  defined(ANDROID_2X)
		if(idx>0)
		{
			u16 temp_hw_val;
			u32 temp_flag;
			temp_hw_val = out[0].hw_value;
			temp_flag = out[0].flags;
			out[0].hw_value = out[idx].hw_value;
			out[0].flags =out[idx].flags;
			out[idx].hw_value = temp_hw_val;
			out[idx].flags =temp_flag;

		}
#endif

	}

	return j;
}

u8 sitesurvey_cmd_hdl(_adapter *padapter, u8 *pbuf)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct sitesurvey_parm	*pparm = (struct sitesurvey_parm *)pbuf;
	u8	bdelayscan = _FALSE;
	u8	val8;
	u32	initialgain;
	u32	i;

#ifdef CONFIG_P2P
	struct wifidirect_info*	pwdinfo = &padapter->wdinfo;
#endif

	if (pmlmeext->sitesurvey_res.state == SCAN_DISABLE)
	{
		//for first time sitesurvey_cmd
		rtw_hal_set_hwreg(padapter, HW_VAR_CHECK_TXBUF, 0);

		pmlmeext->sitesurvey_res.state = SCAN_START;
		pmlmeext->sitesurvey_res.bss_cnt = 0;
		pmlmeext->sitesurvey_res.channel_idx = 0;

		for(i=0;i<RTW_SSID_SCAN_AMOUNT;i++){
			if(pparm->ssid[i].SsidLength) {
				_rtw_memcpy(pmlmeext->sitesurvey_res.ssid[i].Ssid, pparm->ssid[i].Ssid, IW_ESSID_MAX_SIZE);
				pmlmeext->sitesurvey_res.ssid[i].SsidLength= pparm->ssid[i].SsidLength;
			} else {
				pmlmeext->sitesurvey_res.ssid[i].SsidLength= 0;
			}
		}

		pmlmeext->sitesurvey_res.ch_num = rtw_scan_ch_decision(padapter
			, pmlmeext->sitesurvey_res.ch, RTW_CHANNEL_SCAN_AMOUNT
			, pparm->ch, pparm->ch_num
		);

		pmlmeext->sitesurvey_res.scan_mode = pparm->scan_mode;
#ifdef CONFIG_DUALMAC_CONCURRENT
		bdelayscan = dc_handle_site_survey(padapter);
#endif

		//issue null data if associating to the AP
		if (is_client_associated_to_ap(padapter) == _TRUE)
		{
			pmlmeext->sitesurvey_res.state = SCAN_TXNULL;

			/* switch to correct channel of current network  before issue keep-alive frames */
			if (rtw_get_oper_ch(padapter) != pmlmeext->cur_channel) {
				SelectChannel(padapter, pmlmeext->cur_channel);
			}

			issue_nulldata(padapter, 1);
			issue_nulldata(padapter, 1);

#ifdef CONFIG_CONCURRENT_MODE
			if(is_client_associated_to_ap(padapter->pbuddy_adapter) == _TRUE)
			{
				DBG_871X("adapter is scanning(buddy_adapter is linked), issue nulldata(pwrbit=1)\n");
				issue_nulldata(padapter->pbuddy_adapter, 1);
				issue_nulldata(padapter->pbuddy_adapter, 1);
			}
#endif
			bdelayscan = _TRUE;
		}
#ifdef CONFIG_CONCURRENT_MODE
		else if(is_client_associated_to_ap(padapter->pbuddy_adapter) == _TRUE)
		{
			#ifdef CONFIG_TDLS
			if(padapter->pbuddy_adapter->wdinfo.wfd_tdls_enable == 1)
			{
				issue_tunneled_probe_req(padapter->pbuddy_adapter);
			}
			#endif //CONFIG_TDLS

			pmlmeext->sitesurvey_res.state = SCAN_TXNULL;

			issue_nulldata(padapter->pbuddy_adapter, 1);
			issue_nulldata(padapter->pbuddy_adapter, 1);

			bdelayscan = _TRUE;
		}
#endif
		if(bdelayscan)
		{
			//delay 50ms to protect nulldata(1).
			set_survey_timer(pmlmeext, 50);
			return H2C_SUCCESS;
		}
	}

	if ((pmlmeext->sitesurvey_res.state == SCAN_START) || (pmlmeext->sitesurvey_res.state == SCAN_TXNULL))
	{
#ifdef CONFIG_FIND_BEST_CHANNEL
#if 0
		for (i=0; pmlmeext->channel_set[i].ChannelNum !=0; i++) {
			pmlmeext->channel_set[i].rx_count = 0;
		}
#endif
#endif /* CONFIG_FIND_BEST_CHANNEL */

		//disable dynamic functions, such as high power, DIG
		Save_DM_Func_Flag(padapter);
		Switch_DM_Func(padapter, DYNAMIC_FUNC_DISABLE, _FALSE);

		//config the initial gain under scaning, need to write the BB registers
#ifdef CONFIG_IOCTL_CFG80211
		if((wdev_to_priv(padapter->rtw_wdev))->p2p_enabled == _TRUE)
		{
			initialgain = 0x30;
		}
		else
#endif
		{
			initialgain = 0x1E;
		}
		rtw_hal_set_hwreg(padapter, HW_VAR_INITIAL_GAIN, (u8 *)(&initialgain));

		//set MSR to no link state
		Set_MSR(padapter, _HW_STATE_NOLINK_);

		val8 = 1; //under site survey
		rtw_hal_set_hwreg(padapter, HW_VAR_MLME_SITESURVEY, (u8 *)(&val8));

		pmlmeext->sitesurvey_res.state = SCAN_PROCESS;
	}

	site_survey(padapter);

	return H2C_SUCCESS;

}

u8 setauth_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct setauth_parm		*pparm = (struct setauth_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	if (pparm->mode < 4)
	{
		pmlmeinfo->auth_algo = pparm->mode;
	}

	return 	H2C_SUCCESS;
}

u8 setkey_hdl(_adapter *padapter, u8 *pbuf)
{
	unsigned short				ctrl;
	struct setkey_parm		*pparm = (struct setkey_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	unsigned char					null_sta[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	//main tx key for wep.
	if(pparm->set_tx)
		pmlmeinfo->key_index = pparm->keyid;

	//write cam
	ctrl = BIT(15) | ((pparm->algorithm) << 2) | pparm->keyid;

	DBG_871X_LEVEL(_drv_always_, "set group key to hw: alg:%d(WEP40-1 WEP104-5 TKIP-2 AES-4) "
			"keyid:%d\n", pparm->algorithm, pparm->keyid);
	write_cam(padapter, pparm->keyid, ctrl, null_sta, pparm->key);

	return H2C_SUCCESS;
}

u8 set_stakey_hdl(_adapter *padapter, u8 *pbuf)
{
	u16 ctrl=0;
	u8 cam_id;//cam_entry
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct set_stakey_parm	*pparm = (struct set_stakey_parm *)pbuf;
#ifdef CONFIG_TDLS
	struct tdls_info	*ptdlsinfo = &padapter->tdlsinfo;
	struct sta_priv	*pstapriv = &padapter->stapriv;
	struct sta_info	*psta;
#endif //CONFIG_TDLS

	//cam_entry:
	//0~3 for default key

	//for concurrent mode (ap+sta):
	//default key is disable, using sw encrypt/decrypt
	//cam_entry = 4 //for sta mode (macid=0)
	//cam_entry(macid+3) = 5 ~ N//for ap mode (aid=1~N, macid=2 ~N)

	//for concurrent mode (sta+sta):
	//default key is disable, using sw encrypt/decrypt
	//cam_entry = 4 //mapping to macid=0
	//cam_entry = 5 //mapping to macid=2

#ifdef CONFIG_CONCURRENT_MODE
	if((pmlmeinfo->state&0x03) == WIFI_FW_STATION_STATE)
	{
		struct sta_priv	*pstapriv = &padapter->stapriv;
		struct sta_info	*psta;

		psta = rtw_get_stainfo(pstapriv, pmlmeinfo->network.MacAddress);

		if(psta && psta->mac_id==2)
		{
			cam_id = 5;
		}
		else
		{
			cam_id = 4;
		}
/*
		if(padapter->iface_type > PRIMARY_IFACE)
		{
			cam_id = 5;
		}
		else
		{
			cam_id = 4;
		}
*/
	}
#else
	cam_id = 4;
#endif

	DBG_871X_LEVEL(_drv_always_, "set pairwise key to hw: alg:%d(WEP40-1 WEP104-5 TKIP-2 AES-4) camid:%d\n",
		       	pparm->algorithm, cam_id);
	if((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
	{

		struct sta_info *psta;
		struct sta_priv *pstapriv = &padapter->stapriv;

		if(pparm->algorithm == _NO_PRIVACY_)	// clear cam entry
		{
			clear_cam_entry(padapter, pparm->id);
			return H2C_SUCCESS_RSP;
		}

		psta = rtw_get_stainfo(pstapriv, pparm->addr);
		if(psta)
		{
			ctrl = (BIT(15) | ((pparm->algorithm) << 2));

			DBG_871X("r871x_set_stakey_hdl(): enc_algorithm=%d\n", pparm->algorithm);

			if((psta->mac_id<1) || (psta->mac_id>(NUM_STA-4)))
			{
				DBG_871X("r871x_set_stakey_hdl():set_stakey failed, mac_id(aid)=%d\n", psta->mac_id);
				return H2C_REJECTED;
			}

			cam_id = (psta->mac_id + 3);//0~3 for default key, cmd_id=macid + 3, macid=aid+1;

			DBG_871X("Write CAM, mac_addr=%x:%x:%x:%x:%x:%x, cam_entry=%d\n", pparm->addr[0],
						pparm->addr[1], pparm->addr[2], pparm->addr[3], pparm->addr[4],
						pparm->addr[5], cam_id);

			write_cam(padapter, cam_id, ctrl, pparm->addr, pparm->key);

			return H2C_SUCCESS_RSP;

		}
		else
		{
			DBG_871X("r871x_set_stakey_hdl(): sta has been free\n");
			return H2C_REJECTED;
		}

	}

	//below for sta mode

	if(pparm->algorithm == _NO_PRIVACY_)	// clear cam entry
	{
		clear_cam_entry(padapter, pparm->id);
		return H2C_SUCCESS;
	}

	ctrl = BIT(15) | ((pparm->algorithm) << 2);

#ifdef CONFIG_TDLS
	if(ptdlsinfo->clear_cam!=0){
		clear_cam_entry(padapter, ptdlsinfo->clear_cam);
		ptdlsinfo->clear_cam=0;

		return H2C_SUCCESS;
	}

	psta = rtw_get_stainfo(pstapriv, pparm->addr);//Get TDLS Peer STA
	if( psta->tdls_sta_state&TDLS_LINKED_STATE ){
		write_cam(padapter, psta->mac_id, ctrl, pparm->addr, pparm->key);
	}
	else
#endif //CONFIG_TDLS
	write_cam(padapter, cam_id, ctrl, pparm->addr, pparm->key);

	pmlmeinfo->enc_algo = pparm->algorithm;

	return H2C_SUCCESS;
}

u8 add_ba_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct addBaReq_parm 	*pparm = (struct addBaReq_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	struct sta_info *psta = rtw_get_stainfo(&padapter->stapriv, pparm->addr);

	if(!psta)
		return 	H2C_SUCCESS;


	if (((pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS) && (pmlmeinfo->HT_enable)) ||
		((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE))
	{
		//pmlmeinfo->ADDBA_retry_count = 0;
		//pmlmeinfo->candidate_tid_bitmap |= (0x1 << pparm->tid);
		//psta->htpriv.candidate_tid_bitmap |= BIT(pparm->tid);
		issue_action_BA(padapter, pparm->addr, RTW_WLAN_ACTION_ADDBA_REQ, (u16)pparm->tid);
		//_set_timer(&pmlmeext->ADDBA_timer, ADDBA_TO);
		_set_timer(&psta->addba_retry_timer, ADDBA_TO);
	}
#ifdef CONFIG_TDLS
	else if((psta->tdls_sta_state & TDLS_LINKED_STATE)&&
		(psta->htpriv.ht_option==_TRUE) &&
		(psta->htpriv.ampdu_enable==_TRUE) )
	{
		issue_action_BA(padapter, pparm->addr, RTW_WLAN_ACTION_ADDBA_REQ, (u16)pparm->tid);
		//_set_timer(&pmlmeext->ADDBA_timer, ADDBA_TO);
		_set_timer(&psta->addba_retry_timer, ADDBA_TO);
	}
#endif //CONFIG
	else
	{
		psta->htpriv.candidate_tid_bitmap &= ~BIT(pparm->tid);
	}

	return 	H2C_SUCCESS;
}

u8 set_tx_beacon_cmd(_adapter* padapter)
{
	struct cmd_obj	*ph2c;
	struct Tx_Beacon_param 	*ptxBeacon_parm;
	struct cmd_priv	*pcmdpriv = &(padapter->cmdpriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u8	res = _SUCCESS;
	int len_diff = 0;

_func_enter_;

	if ((ph2c = (struct cmd_obj*)rtw_zmalloc(sizeof(struct cmd_obj))) == NULL)
	{
		res= _FAIL;
		goto exit;
	}

	if ((ptxBeacon_parm = (struct Tx_Beacon_param *)rtw_zmalloc(sizeof(struct Tx_Beacon_param))) == NULL)
	{
		rtw_mfree((unsigned char *)ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	_rtw_memcpy(&(ptxBeacon_parm->network), &(pmlmeinfo->network), sizeof(WLAN_BSSID_EX));

	len_diff = update_hidden_ssid(
		ptxBeacon_parm->network.IEs+_BEACON_IE_OFFSET_
		, ptxBeacon_parm->network.IELength-_BEACON_IE_OFFSET_
		, pmlmeinfo->hidden_ssid_mode
	);
	ptxBeacon_parm->network.IELength += len_diff;

	init_h2fwcmd_w_parm_no_rsp(ph2c, ptxBeacon_parm, GEN_CMD_CODE(_TX_Beacon));

	res = rtw_enqueue_cmd(pcmdpriv, ph2c);


exit:

_func_exit_;

	return res;
}


u8 mlme_evt_hdl(_adapter *padapter, unsigned char *pbuf)
{
	u8 evt_code, evt_seq;
	u16 evt_sz;
	uint 	*peventbuf;
	void (*event_callback)(_adapter *dev, u8 *pbuf);
	struct evt_priv *pevt_priv = &(padapter->evtpriv);

	peventbuf = (uint*)pbuf;
	evt_sz = (u16)(*peventbuf&0xffff);
	evt_seq = (u8)((*peventbuf>>24)&0x7f);
	evt_code = (u8)((*peventbuf>>16)&0xff);


	#ifdef CHECK_EVENT_SEQ
	// checking event sequence...
	if (evt_seq != (ATOMIC_READ(&pevt_priv->event_seq) & 0x7f) )
	{
		RT_TRACE(_module_rtl871x_cmd_c_,_drv_info_,("Evetn Seq Error! %d vs %d\n", (evt_seq & 0x7f), (ATOMIC_READ(&pevt_priv->event_seq) & 0x7f)));

		pevt_priv->event_seq = (evt_seq+1)&0x7f;

		goto _abort_event_;
	}
	#endif

	// checking if event code is valid
	if (evt_code >= MAX_C2HEVT)
	{
		RT_TRACE(_module_rtl871x_cmd_c_,_drv_err_,("\nEvent Code(%d) mismatch!\n", evt_code));
		goto _abort_event_;
	}

	// checking if event size match the event parm size
	if ((wlanevents[evt_code].parmsize != 0) &&
			(wlanevents[evt_code].parmsize != evt_sz))
	{

		RT_TRACE(_module_rtl871x_cmd_c_,_drv_err_,("\nEvent(%d) Parm Size mismatch (%d vs %d)!\n",
			evt_code, wlanevents[evt_code].parmsize, evt_sz));
		goto _abort_event_;

	}

	ATOMIC_INC(&pevt_priv->event_seq);

	peventbuf += 2;

	if(peventbuf)
	{
		event_callback = wlanevents[evt_code].event_callback;
		event_callback(padapter, (u8*)peventbuf);

		pevt_priv->evt_done_cnt++;
	}


_abort_event_:


	return H2C_SUCCESS;

}

u8 h2c_msg_hdl(_adapter *padapter, unsigned char *pbuf)
{
	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	return H2C_SUCCESS;
}

u8 tx_beacon_hdl(_adapter *padapter, unsigned char *pbuf)
{
	if(send_beacon(padapter)==_FAIL)
	{
		DBG_871X("issue_beacon, fail!\n");
		return H2C_PARAMETERS_ERROR;
	}
#ifdef CONFIG_AP_MODE
	else //tx bc/mc frames after update TIM
	{
		_irqL irqL;
		struct sta_info *psta_bmc;
		_list	*xmitframe_plist, *xmitframe_phead;
		struct xmit_frame *pxmitframe=NULL;
		struct sta_priv  *pstapriv = &padapter->stapriv;

		//for BC/MC Frames
		psta_bmc = rtw_get_bcmc_stainfo(padapter);
		if(!psta_bmc)
			return H2C_SUCCESS;

		if((pstapriv->tim_bitmap&BIT(0)) && (psta_bmc->sleepq_len>0))
		{
#ifndef CONFIG_PCI_HCI
			rtw_msleep_os(10);// 10ms, ATIM(HIQ) Windows
#endif
			_enter_critical_bh(&psta_bmc->sleep_q.lock, &irqL);

			xmitframe_phead = get_list_head(&psta_bmc->sleep_q);
			xmitframe_plist = get_next(xmitframe_phead);

			while ((rtw_end_of_queue_search(xmitframe_phead, xmitframe_plist)) == _FALSE)
			{
				pxmitframe = LIST_CONTAINOR(xmitframe_plist, struct xmit_frame, list);

				xmitframe_plist = get_next(xmitframe_plist);

				rtw_list_delete(&pxmitframe->list);

				psta_bmc->sleepq_len--;
				if(psta_bmc->sleepq_len>0)
					pxmitframe->attrib.mdata = 1;
				else
					pxmitframe->attrib.mdata = 0;

				pxmitframe->attrib.triggered=1;

				pxmitframe->attrib.qsel = 0x11;//HIQ

                                _exit_critical_bh(&psta_bmc->sleep_q.lock, &irqL);
				if(rtw_hal_xmit(padapter, pxmitframe) == _TRUE)
				{
					rtw_os_xmit_complete(padapter, pxmitframe);
				}
                                _enter_critical_bh(&psta_bmc->sleep_q.lock, &irqL);

				//pstapriv->tim_bitmap &= ~BIT(0);

			}

			_exit_critical_bh(&psta_bmc->sleep_q.lock, &irqL);

//#if defined(CONFIG_PCI_HCI) || defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
#if defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
			rtw_chk_hi_queue_cmd(padapter);
#endif

		}

	}
#endif

	return H2C_SUCCESS;

}

#ifdef CONFIG_DUALMAC_CONCURRENT
void dc_SelectChannel(_adapter *padapter, unsigned char channel)
{
	PADAPTER ptarget_adapter;

	if( (padapter->pbuddy_adapter != NULL) &&
		(padapter->DualMacConcurrent == _TRUE) &&
		(padapter->adapter_type == SECONDARY_ADAPTER))
	{
		// only mac0 could control BB&RF
		ptarget_adapter = padapter->pbuddy_adapter;
	}
	else
	{
		ptarget_adapter = padapter;
	}

	_enter_critical_mutex(&(adapter_to_dvobj(ptarget_adapter)->setch_mutex), NULL);

	rtw_hal_set_chan(ptarget_adapter, channel);

	_exit_critical_mutex(&(adapter_to_dvobj(ptarget_adapter)->setch_mutex), NULL);
}

void dc_SetBWMode(_adapter *padapter, unsigned short bwmode, unsigned char channel_offset)
{
	PADAPTER ptarget_adapter;

	if( (padapter->pbuddy_adapter != NULL) &&
		(padapter->DualMacConcurrent == _TRUE) &&
		(padapter->adapter_type == SECONDARY_ADAPTER))
	{
		// only mac0 could control BB&RF
		ptarget_adapter = padapter->pbuddy_adapter;
	}
	else
	{
		ptarget_adapter = padapter;
	}

	_enter_critical_mutex(&(adapter_to_dvobj(ptarget_adapter)->setbw_mutex), NULL);

	rtw_hal_set_bwmode(ptarget_adapter, (HT_CHANNEL_WIDTH)bwmode, channel_offset);

	_exit_critical_mutex(&(adapter_to_dvobj(ptarget_adapter)->setbw_mutex), NULL);
}

static void dc_change_band(_adapter *padapter, WLAN_BSSID_EX *pnetwork)
{
	u8	network_type,rate_len, total_rate_len,remainder_rate_len;
	struct mlme_ext_priv *pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info *pmlmeinfo = &(pmlmeext->mlmext_info);
	u8	erpinfo=0x4;

	//DBG_871X("%s\n", __FUNCTION__);

	if(pmlmeext->cur_channel >= 36)
	{
		network_type = WIRELESS_11A;
		total_rate_len = IEEE80211_NUM_OFDM_RATESLEN;
		DBG_871X("%s(): change to 5G Band\n",__FUNCTION__);
		rtw_remove_bcn_ie(padapter, pnetwork, _ERPINFO_IE_);
	}
	else
	{
		network_type = WIRELESS_11BG;
		total_rate_len = IEEE80211_CCK_RATE_LEN+IEEE80211_NUM_OFDM_RATESLEN;
		DBG_871X("%s(): change to 2.4G Band\n",__FUNCTION__);
		rtw_add_bcn_ie(padapter, pnetwork, _ERPINFO_IE_, &erpinfo, 1);
	}

	rtw_set_supported_rate(pnetwork->SupportedRates, network_type);

	UpdateBrateTbl(padapter, pnetwork->SupportedRates);
	rtw_hal_set_hwreg(padapter, HW_VAR_BASIC_RATE, pnetwork->SupportedRates);

	if(total_rate_len > 8)
	{
		rate_len = 8;
		remainder_rate_len = total_rate_len - 8;
	}
	else
	{
		rate_len = total_rate_len;
		remainder_rate_len = 0;
	}

	rtw_add_bcn_ie(padapter, pnetwork, _SUPPORTEDRATES_IE_, pnetwork->SupportedRates, rate_len);

	if(remainder_rate_len)
	{
		rtw_add_bcn_ie(padapter, pnetwork, _EXT_SUPPORTEDRATES_IE_, (pnetwork->SupportedRates+8), remainder_rate_len);
	}
	else
	{
		rtw_remove_bcn_ie(padapter, pnetwork, _EXT_SUPPORTEDRATES_IE_);
	}
}

void dc_set_channel_bwmode_disconnect(_adapter *padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;
	struct mlme_priv *pbuddy_mlmepriv = NULL;

	if(pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)
	{
		pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);
		if((check_fwstate(pbuddy_mlmepriv, _FW_LINKED)) != _TRUE)
		{
			//switch to the 20M Hz mode after disconnect
			pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
			pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

			set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
		}
	}
	else
	{
		//switch to the 20M Hz mode after disconnect
		pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
		pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

		set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
	}
}

u8 dc_handle_join_request(_adapter *padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
	PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;
	struct mlme_ext_priv *pbuddy_mlmeext = NULL;
	struct mlme_priv	*pbuddy_mlmepriv = NULL;
	u8	ret = _SUCCESS;

	if(pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)
	{
		pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;
		pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);

		if(pmlmeext->cur_channel != pbuddy_mlmeext->cur_channel ||
			pmlmeext->cur_bwmode != pbuddy_mlmeext->cur_bwmode ||
			pmlmeext->cur_ch_offset != pbuddy_mlmeext->cur_ch_offset)
		{
			if((check_fwstate(pbuddy_mlmepriv, WIFI_AP_STATE)) == _TRUE)
			{
				//issue deauth to all stas if if2 is at ap mode
				rtw_sta_flush(pbuddy_adapter);

				//rtw_hal_set_hwreg(padapter, HW_VAR_CHECK_TXBUF, 0);
				rtw_hal_set_hwreg(pbuddy_adapter, HW_VAR_CHECK_TXBUF, 0);
			}
			else if(check_fwstate(pbuddy_mlmepriv, _FW_LINKED) == _TRUE)
			{
				if(pmlmeext->cur_channel == pbuddy_mlmeext->cur_channel)
				{
					// HT_CHANNEL_WIDTH_40 or HT_CHANNEL_WIDTH_20 but channel offset is different
					if((pmlmeext->cur_bwmode == pbuddy_mlmeext->cur_bwmode) &&
						(pmlmeext->cur_ch_offset != pbuddy_mlmeext->cur_ch_offset) )
					{
						report_join_res(padapter, -4);
						ret = _FAIL;
					}
				}
				else
				{
					report_join_res(padapter, -4);
					ret = _FAIL;
				}
			}
		}
		else	 if (is_client_associated_to_ap(pbuddy_adapter) == _TRUE)
		{
			issue_nulldata(pbuddy_adapter, 1);
		}
	}

	return ret;
}

void dc_handle_join_done(_adapter *padapter, u8 join_res)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;
	struct mlme_priv *pbuddy_mlmepriv = NULL;
	struct mlme_ext_priv *pbuddy_mlmeext = NULL;
	struct mlme_ext_info *pbuddy_mlmeinfo = NULL;
	WLAN_BSSID_EX *pbuddy_network_mlmeext = NULL;
	u8	change_band = _FALSE;


	if(pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)
	{
		pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);
		pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;
		pbuddy_mlmeinfo = &(pbuddy_mlmeext->mlmext_info);
		pbuddy_network_mlmeext = &(pbuddy_mlmeinfo->network);

		if(((pbuddy_mlmeinfo->state&0x03) == WIFI_FW_AP_STATE) &&
				check_fwstate(pbuddy_mlmepriv, _FW_LINKED))
		{
			//restart and update beacon
			DBG_871X("after join, current adapter, CH=%d, BW=%d, offset=%d\n", pmlmeext->cur_channel, pmlmeext->cur_bwmode, pmlmeext->cur_ch_offset);

			if(join_res >= 0)
			{
				u8 *p;
				int	ie_len;
				struct HT_info_element *pht_info=NULL;

				if((pbuddy_mlmeext->cur_channel <= 14 && pmlmeext->cur_channel >= 36) ||
					(pbuddy_mlmeext->cur_channel >= 36 && pmlmeext->cur_channel <= 14))
				{
					change_band = _TRUE;
				}

				//sync channel/bwmode/ch_offset with another adapter
				pbuddy_mlmeext->cur_channel = pmlmeext->cur_channel;

				if(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40)
				{
					p = rtw_get_ie((pbuddy_network_mlmeext->IEs + sizeof(NDIS_802_11_FIXED_IEs)), _HT_ADD_INFO_IE_, &ie_len, (pbuddy_network_mlmeext->IELength - sizeof(NDIS_802_11_FIXED_IEs)));
					if( p && ie_len)
					{
						pht_info = (struct HT_info_element *)(p+2);
						pht_info->infos[0] &= ~(BIT(0)|BIT(1)); //no secondary channel is present
					}

					if(pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40)
					{
						pbuddy_mlmeext->cur_ch_offset = pmlmeext->cur_ch_offset;

						//to update cur_ch_offset value in beacon
						if( pht_info )
						{
							switch(pmlmeext->cur_ch_offset)
							{
								case HAL_PRIME_CHNL_OFFSET_LOWER:
									pht_info->infos[0] |= 0x1;
									break;
								case HAL_PRIME_CHNL_OFFSET_UPPER:
									pht_info->infos[0] |= 0x3;
									break;
								case HAL_PRIME_CHNL_OFFSET_DONT_CARE:
								default:
									break;
							}
						}
					}
					else if(pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_20)
					{
						pbuddy_mlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
						pbuddy_mlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

						if(pmlmeext->cur_channel>0 && pmlmeext->cur_channel<5)
						{
							if(pht_info)
								pht_info->infos[0] |= 0x1;

							pbuddy_mlmeext->cur_bwmode = HT_CHANNEL_WIDTH_40;
							pbuddy_mlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_LOWER;
						}

						if(pmlmeext->cur_channel>7 && pmlmeext->cur_channel<(14+1))
						{
							if(pht_info)
								pht_info->infos[0] |= 0x3;

							pbuddy_mlmeext->cur_bwmode = HT_CHANNEL_WIDTH_40;
							pbuddy_mlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_UPPER;
						}

						set_channel_bwmode(padapter, pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_ch_offset, pbuddy_mlmeext->cur_bwmode);
					}
				}

				// to update channel value in beacon
				pbuddy_network_mlmeext->Configuration.DSConfig = pmlmeext->cur_channel;
				p = rtw_get_ie((pbuddy_network_mlmeext->IEs + sizeof(NDIS_802_11_FIXED_IEs)), _DSSET_IE_, &ie_len, (pbuddy_network_mlmeext->IELength - sizeof(NDIS_802_11_FIXED_IEs)));
				if(p && ie_len>0)
					*(p + 2) = pmlmeext->cur_channel;

				p = rtw_get_ie((pbuddy_network_mlmeext->IEs + sizeof(NDIS_802_11_FIXED_IEs)), _HT_ADD_INFO_IE_, &ie_len, (pbuddy_network_mlmeext->IELength - sizeof(NDIS_802_11_FIXED_IEs)));
				if( p && ie_len)
				{
					pht_info = (struct HT_info_element *)(p+2);
					pht_info->primary_channel = pmlmeext->cur_channel;
				}

				// update mlmepriv's cur_network
				_rtw_memcpy(&pbuddy_mlmepriv->cur_network.network, pbuddy_network_mlmeext, pbuddy_network_mlmeext->Length);
			}
			else
			{
				// switch back to original channel/bwmode/ch_offset;
				set_channel_bwmode(padapter, pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_ch_offset, pbuddy_mlmeext->cur_bwmode);
			}

			DBG_871X("after join, another adapter, CH=%d, BW=%d, offset=%d\n", pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_bwmode, pbuddy_mlmeext->cur_ch_offset);

			if(change_band == _TRUE)
				dc_change_band(pbuddy_adapter, pbuddy_network_mlmeext);

			DBG_871X("update pbuddy_adapter's beacon\n");

			update_beacon(pbuddy_adapter, 0, NULL, _TRUE);
		}
		else	 if (is_client_associated_to_ap(pbuddy_adapter) == _TRUE)
		{
			if((pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40) &&
				(pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_20))
			{
				set_channel_bwmode(padapter, pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_ch_offset, pbuddy_mlmeext->cur_bwmode);
			}

			issue_nulldata(pbuddy_adapter, 0);
		}
	}
}

sint	dc_check_fwstate(_adapter *padapter, sint fw_state)
{
	PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;
	struct mlme_priv *pbuddy_mlmepriv = NULL;

	if(padapter->pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)

	{
		pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);

		return check_fwstate(pbuddy_mlmepriv, fw_state);
	}

	return _FALSE;
}

u8 dc_handle_site_survey(_adapter *padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;

	// only mac0 can do scan request, help issue nulldata(1) for mac1
	if(pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)
	{
		if (is_client_associated_to_ap(pbuddy_adapter) == _TRUE)
		{
			pmlmeext->sitesurvey_res.state = SCAN_TXNULL;

			issue_nulldata(pbuddy_adapter, 1);
			issue_nulldata(pbuddy_adapter, 1);

			return _TRUE;
		}
	}

	return _FALSE;
}

void	dc_report_survey_event(_adapter *padapter, union recv_frame *precv_frame)
{
	if(padapter->pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)
	{
		report_survey_event(padapter->pbuddy_adapter, precv_frame);
	}
}

void dc_set_channel_bwmode_survey_done(_adapter *padapter)
{
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;
	struct mlme_priv *pbuddy_mlmepriv = NULL;
	struct mlme_ext_priv *pbuddy_mlmeext = NULL;
	struct mlme_ext_info *pbuddy_mlmeinfo = NULL;
	u8 cur_channel;
	u8 cur_bwmode;
	u8 cur_ch_offset;

	if(pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)
	{
		pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);
		pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;
		pbuddy_mlmeinfo = &(pbuddy_mlmeext->mlmext_info);

		if(check_fwstate(pbuddy_mlmepriv, _FW_LINKED))
		{
			if(check_fwstate(pmlmepriv, _FW_LINKED) &&
				(pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40))
			{
				cur_channel = pmlmeext->cur_channel;
				cur_bwmode = pmlmeext->cur_bwmode;
				cur_ch_offset = pmlmeext->cur_ch_offset;
			}
			else
			{
				cur_channel = pbuddy_mlmeext->cur_channel;
				cur_bwmode = pbuddy_mlmeext->cur_bwmode;
				cur_ch_offset = pbuddy_mlmeext->cur_ch_offset;
			}
		}
		else
		{
			cur_channel = pmlmeext->cur_channel;
			cur_bwmode = pmlmeext->cur_bwmode;
			cur_ch_offset = pmlmeext->cur_ch_offset;
		}

		set_channel_bwmode(padapter, cur_channel, cur_ch_offset, cur_bwmode);

		if (is_client_associated_to_ap(pbuddy_adapter) == _TRUE)
		{
			//issue null data
			issue_nulldata(pbuddy_adapter, 0);
		}

		if(((pbuddy_mlmeinfo->state&0x03) == WIFI_FW_AP_STATE) &&
			check_fwstate(pbuddy_mlmepriv, _FW_LINKED))
		{

			DBG_871X("survey done, current CH=%d, BW=%d, offset=%d\n", cur_channel, cur_bwmode, cur_ch_offset);

			DBG_871X("restart pbuddy_adapter's beacon\n");

			update_beacon(pbuddy_adapter, 0, NULL, _TRUE);
		}
	}
	else
	{
		set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
	}
}

void dc_set_ap_channel_bandwidth(_adapter *padapter, u8 channel, u8 channel_offset, u8 bwmode)
{
	u8	*p;
	u8	val8, cur_channel, cur_bwmode, cur_ch_offset, change_band;
	int	ie_len;
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	struct mlme_priv		*pmlmepriv = &(padapter->mlmepriv);
	WLAN_BSSID_EX		*pnetwork = (WLAN_BSSID_EX *)&pmlmepriv->cur_network.network;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct HT_info_element	*pht_info=NULL;
	_adapter	*pbuddy_adapter = padapter->pbuddy_adapter;
	struct mlme_priv	*pbuddy_mlmepriv = NULL;
	struct mlme_ext_priv	*pbuddy_mlmeext = NULL;

	DBG_871X("dualmac_concurrent_ap_set_channel_bwmode ==>\n");

	cur_channel = channel;
	cur_bwmode = bwmode;
	cur_ch_offset = channel_offset;
	change_band = _FALSE;

	p = rtw_get_ie((pnetwork->IEs + sizeof(NDIS_802_11_FIXED_IEs)), _HT_ADD_INFO_IE_, &ie_len, (pnetwork->IELength - sizeof(NDIS_802_11_FIXED_IEs)));
	if( p && ie_len)
	{
		pht_info = (struct HT_info_element *)(p+2);
	}

	if(pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)
	{
		pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);
		pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;

		if(!check_fwstate(pbuddy_mlmepriv, _FW_LINKED|_FW_UNDER_LINKING|_FW_UNDER_SURVEY))
		{
			set_channel_bwmode(padapter, cur_channel, cur_ch_offset, cur_bwmode);
		}
		else if(check_fwstate(pbuddy_mlmepriv, _FW_LINKED)==_TRUE)
		{
			//To sync cur_channel/cur_bwmode/cur_ch_offset with another adapter
			DBG_871X("Another iface is at linked state, sync cur_channel/cur_bwmode/cur_ch_offset\n");
			DBG_871X("Another adapter, CH=%d, BW=%d, offset=%d\n", pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_bwmode, pbuddy_mlmeext->cur_ch_offset);
			DBG_871X("Current adapter, CH=%d, BW=%d, offset=%d\n", cur_channel, cur_bwmode, cur_ch_offset);

			cur_channel = pbuddy_mlmeext->cur_channel;
			if(cur_bwmode == HT_CHANNEL_WIDTH_40)
			{
				if(pht_info)
					pht_info->infos[0] &= ~(BIT(0)|BIT(1));

				if(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40)
				{
					cur_ch_offset = pbuddy_mlmeext->cur_ch_offset;

					//to update cur_ch_offset value in beacon
					if(pht_info)
					{
						switch(cur_ch_offset)
						{
							case HAL_PRIME_CHNL_OFFSET_LOWER:
								pht_info->infos[0] |= 0x1;
								break;
							case HAL_PRIME_CHNL_OFFSET_UPPER:
								pht_info->infos[0] |= 0x3;
								break;
							case HAL_PRIME_CHNL_OFFSET_DONT_CARE:
							default:
								break;
						}
					}
				}
				else if(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_20)
				{
					cur_bwmode = HT_CHANNEL_WIDTH_20;
					cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

					if(cur_channel>0 && cur_channel<5)
					{
						if(pht_info)
							pht_info->infos[0] |= 0x1;

						cur_bwmode = HT_CHANNEL_WIDTH_40;
						cur_ch_offset = HAL_PRIME_CHNL_OFFSET_LOWER;
					}

					if(cur_channel>7 && cur_channel<(14+1))
					{
						if(pht_info)
							pht_info->infos[0] |= 0x3;

						cur_bwmode = HT_CHANNEL_WIDTH_40;
						cur_ch_offset = HAL_PRIME_CHNL_OFFSET_UPPER;
					}

					set_channel_bwmode(padapter, cur_channel, cur_ch_offset, cur_bwmode);
				}
			}

			// to update channel value in beacon
			pnetwork->Configuration.DSConfig = cur_channel;
			p = rtw_get_ie((pnetwork->IEs + sizeof(NDIS_802_11_FIXED_IEs)), _DSSET_IE_, &ie_len, (pnetwork->IELength - sizeof(NDIS_802_11_FIXED_IEs)));
			if(p && ie_len>0)
				*(p + 2) = cur_channel;

			if(pht_info)
				pht_info->primary_channel = cur_channel;
		}
	}
	else
	{
		set_channel_bwmode(padapter, cur_channel, cur_ch_offset, cur_bwmode);
	}

	DBG_871X("CH=%d, BW=%d, offset=%d\n", cur_channel, cur_bwmode, cur_ch_offset);

	if((channel <= 14 && cur_channel >= 36) ||
		(channel >= 36 && cur_channel <= 14))
	{
		change_band = _TRUE;
	}

	pmlmeext->cur_channel = cur_channel;
	pmlmeext->cur_bwmode = cur_bwmode;
	pmlmeext->cur_ch_offset = cur_ch_offset;

	if(change_band == _TRUE)
		dc_change_band(padapter, pnetwork);

	DBG_871X("dualmac_concurrent_ap_set_channel_bwmode <==\n");
}

void dc_resume_xmit(_adapter *padapter)
{
	PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;

	if(pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)
	{
		DBG_871X("dc_resume_xmit,  resume pbuddy_adapter Tx\n");
		rtw_os_xmit_schedule(pbuddy_adapter);
	}
}

u8	dc_check_xmit(_adapter *padapter)
{
	PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;
	struct mlme_priv *pbuddy_mlmepriv = NULL;

	if(pbuddy_adapter != NULL &&
		padapter->DualMacConcurrent == _TRUE)
	{
		pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);
		if (check_fwstate(pbuddy_mlmepriv, _FW_UNDER_SURVEY|_FW_UNDER_LINKING) == _TRUE)
		{
			DBG_871X("dc_check_xmit  pbuddy_adapter is under survey or under linking\n");
			return _FALSE;
		}
	}

	return _TRUE;
}
#endif

#ifdef CONFIG_CONCURRENT_MODE
sint check_buddy_mlmeinfo_state(_adapter *padapter, u32 state)
{
	PADAPTER pbuddy_adapter;
	struct mlme_ext_priv *pbuddy_mlmeext;
	struct mlme_ext_info *pbuddy_mlmeinfo;

	if(padapter == NULL)
		return _FALSE;

	pbuddy_adapter = padapter->pbuddy_adapter;

	if(pbuddy_adapter == NULL)
		return _FALSE;


	pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;
	pbuddy_mlmeinfo = &(pbuddy_mlmeext->mlmext_info);

	if((pbuddy_mlmeinfo->state&0x03) == state)
		return _TRUE;

	return _FALSE;

}

int concurrent_chk_start_clnt_join(_adapter *padapter)
{
	int ret = _FAIL;
	PADAPTER pbuddy_adapter;
	struct mlme_ext_priv *pbuddy_mlmeext;
	struct mlme_ext_info	*pbuddy_pmlmeinfo;
	struct mlme_priv *pbuddy_mlmepriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	unsigned char	cur_ch = pmlmeext->cur_channel;
	unsigned char	cur_bw = pmlmeext->cur_bwmode;
	unsigned char	cur_ch_offset = pmlmeext->cur_ch_offset;

	if(!rtw_buddy_adapter_up(padapter))
	{
		DBG_871X("concurrent_chk_start_clnt_join: buddy adapter is not up \n");
		goto start_join_set_ch_bw;
	}

	pbuddy_adapter = padapter->pbuddy_adapter;
	pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;
	pbuddy_pmlmeinfo = &(pbuddy_mlmeext->mlmext_info);
	pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);

	if((pbuddy_pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)//for AP MODE
	{
		bool inform_ch_switch = _FALSE;
		DBG_871X("concurrent_chk_start_clnt_join: buddy adapter is AP Case \n");
		if(pmlmeext->cur_channel != pbuddy_mlmeext->cur_channel)
		{
			inform_ch_switch = _TRUE;
		}
		else if((pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40) &&
			(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40) &&
			(pmlmeext->cur_ch_offset != pbuddy_mlmeext->cur_ch_offset))
		{
			inform_ch_switch = _TRUE;
		}
		else if((pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_20) &&
			(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40))
		{
			inform_ch_switch = _FALSE;
			cur_ch = pmlmeext->cur_channel;
			cur_bw = pbuddy_mlmeext->cur_bwmode;
			cur_ch_offset = pbuddy_mlmeext->cur_ch_offset;
		}


		if (inform_ch_switch) {
			//issue deauth to all stas if if2 is at ap mode
			rtw_sta_flush(pbuddy_adapter);
			rtw_hal_set_hwreg(padapter, HW_VAR_CHECK_TXBUF, 0);
		}
	}
	else if(check_fwstate(pbuddy_mlmepriv, _FW_LINKED) == _TRUE &&
		check_fwstate(pbuddy_mlmepriv, WIFI_STATION_STATE) == _TRUE) //for Client Mode/p2p client
	{
		DBG_871X("concurrent_chk_start_clnt_join: buddy adapter is STA Case \n");
#if defined(CONFIG_P2P) && defined(CONFIG_IOCTL_CFG80211)
		{
			struct wifidirect_info *pbuddy_wdinfo = &(pbuddy_adapter->wdinfo);
			{
				if(rtw_p2p_chk_role(pbuddy_wdinfo, P2P_ROLE_CLIENT))
				{
					bool inform_ch_switch_client = _FALSE;
					if(pmlmeext->cur_channel != pbuddy_mlmeext->cur_channel)
					{
						inform_ch_switch_client = _TRUE;
					}
					else if((pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40) &&
						(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40) &&
						(pmlmeext->cur_ch_offset != pbuddy_mlmeext->cur_ch_offset))
					{
						inform_ch_switch_client = _TRUE;
					}
					if(inform_ch_switch_client)
					{
						DBG_871X("%s  buddy adapter is Client and linked, and channel is not the same!\n",__func__);
						pbuddy_pmlmeinfo->state = WIFI_FW_NULL_STATE;
						issue_deauth(pbuddy_adapter, (&(pbuddy_pmlmeinfo->network))->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
						report_del_sta_event(pbuddy_adapter,  (&(pbuddy_pmlmeinfo->network))->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
					}
				}
				goto start_join_set_ch_bw;//wlan0-sta mode has higher priority than p2p0-p2p client
			}
		}
#endif //CONFIG_P2P && CONFIG_IOCTL_CFG80211

		if(pmlmeext->cur_channel != pbuddy_mlmeext->cur_channel)
		{
			DBG_871X("start_clnt_join(ch=%d), but channel mismatch with buddy(ch=%d) interface\n",
				pmlmeext->cur_channel, pbuddy_mlmeext->cur_channel);

				goto start_join_report_fail;
		}

		DBG_871X("start_clnt_join: (bwmode=%d, ch_offset=%d), and buddy(bwmode=%d, ch_offset=%d)\n",
				pmlmeext->cur_bwmode, pmlmeext->cur_ch_offset, pbuddy_mlmeext->cur_bwmode, pbuddy_mlmeext->cur_ch_offset);

		if((pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_20) &&
			(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40))
		{
			cur_bw = HT_CHANNEL_WIDTH_40;
			cur_ch_offset = pbuddy_mlmeext->cur_ch_offset;
		}
		else if((pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40) &&
			(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40) &&
			(pmlmeext->cur_ch_offset != pbuddy_mlmeext->cur_ch_offset))
		{
			goto start_join_report_fail;
		}
	}


start_join_set_ch_bw:

	DBG_871X("start_join_set_ch_bw: ch=%d, bwmode=%d, ch_offset=%d\n", cur_ch, cur_bw, cur_ch_offset);

	set_channel_bwmode(padapter, cur_ch, cur_ch_offset, cur_bw);

	return _SUCCESS;

start_join_report_fail:

	DBG_871X("concurrent_chk_start_clnt_join: start join report fail \n");

	report_join_res(padapter, (-4));

	return ret;

}


void concurrent_chk_joinbss_done(_adapter *padapter, int join_res)
{
	struct mlme_ext_priv	*pmlmeext;
	struct mlme_ext_info	*pmlmeinfo;
	PADAPTER pbuddy_adapter;
	struct mlme_priv *pbuddy_mlmepriv;
	struct mlme_ext_priv *pbuddy_mlmeext;
	struct mlme_ext_info *pbuddy_mlmeinfo;
	WLAN_BSSID_EX *pbuddy_network_mlmeext;

	pmlmeext = &padapter->mlmeextpriv;
	pmlmeinfo = &(pmlmeext->mlmext_info);


	if(!rtw_buddy_adapter_up(padapter))
	{
		DBG_871X("concurrent_chk_joinbss_done: buddy adapter is not up \n");
		set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
		return;
	}

	pbuddy_adapter = padapter->pbuddy_adapter;
	pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);
	pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;
	pbuddy_mlmeinfo = &(pbuddy_mlmeext->mlmext_info);
	pbuddy_network_mlmeext = &(pbuddy_mlmeinfo->network);

	if(((pbuddy_mlmeinfo->state&0x03) == WIFI_FW_AP_STATE) &&
			check_fwstate(pbuddy_mlmepriv, _FW_LINKED))
	{
		//restart and update beacon

		DBG_871X("after join,primary adapter, CH=%d, BW=%d, offset=%d\n"
			, pmlmeext->cur_channel, pmlmeext->cur_bwmode, pmlmeext->cur_ch_offset);


		if(join_res >= 0)
		{
			u8 *p;
			int ie_len;
			u8	change_band = _FALSE;
			struct HT_info_element *pht_info=NULL;

			p = rtw_get_ie((pbuddy_network_mlmeext->IEs + sizeof(NDIS_802_11_FIXED_IEs)), _HT_ADD_INFO_IE_, &ie_len, (pbuddy_network_mlmeext->IELength - sizeof(NDIS_802_11_FIXED_IEs)));
			if( p && ie_len)
			{
				pht_info = (struct HT_info_element *)(p+2);
				pht_info->infos[0] &= ~HT_INFO_HT_PARAM_SECONDARY_CHNL_OFF_MASK; //no secondary channel is present
			}

			//sync channel/bwmode/ch_offset with primary adapter
			pbuddy_mlmeext->cur_channel = pmlmeext->cur_channel;

			if(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40)
			{
				p = rtw_get_ie((pbuddy_network_mlmeext->IEs + sizeof(NDIS_802_11_FIXED_IEs)), _HT_ADD_INFO_IE_, &ie_len, (pbuddy_network_mlmeext->IELength - sizeof(NDIS_802_11_FIXED_IEs)));
				if( p && ie_len)
				{
					pht_info = (struct HT_info_element *)(p+2);
					pht_info->infos[0] &= ~(BIT(0)|BIT(1)); //no secondary channel is present
				}

				if(pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40)
				{
					pbuddy_mlmeext->cur_ch_offset = pmlmeext->cur_ch_offset;

					//to update cur_ch_offset value in beacon
					if( pht_info )
					{
						switch(pmlmeext->cur_ch_offset)
						{
							case HAL_PRIME_CHNL_OFFSET_LOWER:
								pht_info->infos[0] |= 0x1;
								break;
							case HAL_PRIME_CHNL_OFFSET_UPPER:
								pht_info->infos[0] |= 0x3;
								break;
							case HAL_PRIME_CHNL_OFFSET_DONT_CARE:
							default:
								break;
						}

					}

				}
				else if(pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_20)
				{
					if(pmlmeext->cur_channel>=1 && pmlmeext->cur_channel<=4)
					{
						if(pht_info)
							pht_info->infos[0] |= HT_INFO_HT_PARAM_SECONDARY_CHNL_ABOVE;

						pbuddy_mlmeext->cur_bwmode = HT_CHANNEL_WIDTH_40;
						pbuddy_mlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_LOWER;
					}
					else if(pmlmeext->cur_channel>=5 && pmlmeext->cur_channel<=14)
					{
						if(pht_info)
							pht_info->infos[0] |= HT_INFO_HT_PARAM_SECONDARY_CHNL_BELOW;

						pbuddy_mlmeext->cur_bwmode = HT_CHANNEL_WIDTH_40;
						pbuddy_mlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_UPPER;
					}
				}

				set_channel_bwmode(padapter, pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_ch_offset, pbuddy_mlmeext->cur_bwmode);

			}
			else
			{
				set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
			}


			// to update channel value in beacon
			pbuddy_network_mlmeext->Configuration.DSConfig = pmlmeext->cur_channel;
			p = rtw_get_ie((pbuddy_network_mlmeext->IEs + sizeof(NDIS_802_11_FIXED_IEs)), _DSSET_IE_, &ie_len, (pbuddy_network_mlmeext->IELength - sizeof(NDIS_802_11_FIXED_IEs)));
			if(p && ie_len>0)
				*(p + 2) = pmlmeext->cur_channel;

			p = rtw_get_ie((pbuddy_network_mlmeext->IEs + sizeof(NDIS_802_11_FIXED_IEs)), _HT_ADD_INFO_IE_, &ie_len, (pbuddy_network_mlmeext->IELength - sizeof(NDIS_802_11_FIXED_IEs)));
			if( p && ie_len)
			{
				pht_info = (struct HT_info_element *)(p+2);
				pht_info->primary_channel = pmlmeext->cur_channel;
			}
		}
		else
		{
			// switch back to original channel/bwmode/ch_offset;
			set_channel_bwmode(padapter, pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_ch_offset, pbuddy_mlmeext->cur_bwmode);
		}

		DBG_871X("after join, second adapter, CH=%d, BW=%d, offset=%d\n", pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_bwmode, pbuddy_mlmeext->cur_ch_offset);

		DBG_871X("update pbuddy_adapter's beacon\n");

		update_beacon(pbuddy_adapter, 0, NULL, _TRUE);

	}
	else if(((pbuddy_mlmeinfo->state&0x03) == WIFI_FW_STATION_STATE) &&
			check_fwstate(pbuddy_mlmepriv, _FW_LINKED))
	{
		DBG_871X("concurrent_chk_joinbss_done: buddy adapter is station and link \n");
		if(join_res >= 0)
		{
			pbuddy_mlmeext->cur_channel = pmlmeext->cur_channel;
			if(pbuddy_mlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40)
				set_channel_bwmode(padapter, pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_ch_offset, pbuddy_mlmeext->cur_bwmode);
			else if(pmlmeext->cur_bwmode == HT_CHANNEL_WIDTH_40)
				set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
			else
				set_channel_bwmode(padapter, pmlmeext->cur_channel, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);
		}
		else
		{
			// switch back to original channel/bwmode/ch_offset;
			set_channel_bwmode(padapter, pbuddy_mlmeext->cur_channel, pbuddy_mlmeext->cur_ch_offset, pbuddy_mlmeext->cur_bwmode);
		}
	}
	else
	{
		DBG_871X("concurrent_chk_joinbss_done: buddy adapter is up but not connect \n");
		set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
	}

}

#endif //CONFIG_CONCURRENT_MODE

u8 set_chplan_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct SetChannelPlan_param *setChannelPlan_param;
	struct mlme_priv		*pmlmepriv = &padapter->mlmepriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;

	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	setChannelPlan_param = (struct SetChannelPlan_param *)pbuf;

	pmlmeext->max_chan_nums = init_channel_set(padapter, setChannelPlan_param->channel_plan, pmlmeext->channel_set);

	return 	H2C_SUCCESS;
}

u8 led_blink_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct LedBlink_param *ledBlink_param;

	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	ledBlink_param = (struct LedBlink_param *)pbuf;

	#ifdef CONFIG_LED_HANDLED_BY_CMD_THREAD
	BlinkHandler(ledBlink_param->pLed);
	#endif

	return 	H2C_SUCCESS;
}

u8 set_csa_hdl(_adapter *padapter, unsigned char *pbuf)
{
#ifdef CONFIG_DFS
	struct SetChannelSwitch_param *setChannelSwitch_param;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &pmlmeext->mlmext_info;
	u8 new_ch_no;
	u8 gval8 = 0x00, sval8 = 0xff;

	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	setChannelSwitch_param = (struct SetChannelSwitch_param *)pbuf;
	new_ch_no = setChannelSwitch_param->new_ch_no;

	rtw_hal_get_hwreg(padapter, HW_VAR_TXPAUSE, &gval8);

	rtw_hal_set_hwreg(padapter, HW_VAR_TXPAUSE, &sval8);

	DBG_871X("DFS detected! Swiching channel to %d!\n", new_ch_no);
	SelectChannel(padapter, new_ch_no);

	rtw_hal_set_hwreg(padapter, HW_VAR_TXPAUSE, &gval8);

	rtw_free_network_queue(padapter, _TRUE);
	rtw_indicate_disconnect(padapter, 0);

	if ( ((new_ch_no >= 52) && (new_ch_no <= 64)) ||((new_ch_no >= 100) && (new_ch_no <= 140)) ) {
		DBG_871X("Switched to DFS band (ch %02x) again!!\n", new_ch_no);
	}

	return 	H2C_SUCCESS;
#else
	return	H2C_REJECTED;
#endif //CONFIG_DFS

}

// TDLS_WRCR		: write RCR DATA BIT
// TDLS_SD_PTI		: issue peer traffic indication
// TDLS_CS_OFF		: go back to the channel linked with AP, terminating channel switch procedure
// TDLS_INIT_CH_SEN	: init channel sensing, receive all data and mgnt frame
// TDLS_DONE_CH_SEN: channel sensing and report candidate channel
// TDLS_OFF_CH		: first time set channel to off channel
// TDLS_BASE_CH		: go back tp the channel linked with AP when set base channel as target channel
// TDLS_P_OFF_CH	: periodically go to off channel
// TDLS_P_BASE_CH	: periodically go back to base channel
// TDLS_RS_RCR		: restore RCR
// TDLS_CKALV_PH1	: check alive timer phase1
// TDLS_CKALV_PH2	: check alive timer phase2
// TDLS_FREE_STA	: free tdls sta
u8 tdls_hdl(_adapter *padapter, unsigned char *pbuf)
{
#ifdef CONFIG_TDLS
	_irqL irqL;
	struct tdls_info *ptdlsinfo = &padapter->tdlsinfo;
	struct TDLSoption_param *TDLSoption;
	struct sta_info *ptdls_sta;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &pmlmeext->mlmext_info;
	u8 survey_channel, i, min, option;

	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	TDLSoption = (struct TDLSoption_param *)pbuf;

	ptdls_sta = rtw_get_stainfo( &(padapter->stapriv), TDLSoption->addr );
	option = TDLSoption->option;

	if( ptdls_sta == NULL )
	{
		if( option != TDLS_RS_RCR )
			return H2C_REJECTED;
	}

	//_enter_critical_bh(&(ptdlsinfo->hdl_lock), &irqL);
	DBG_871X("[%s] option:%d\n", __FUNCTION__, option);

	switch(option){
		case TDLS_WRCR:
			//As long as TDLS handshake success, we should set RCR_CBSSID_DATA bit to 0
			//such we can receive all kinds of data frames.
			rtw_hal_set_hwreg(padapter, HW_VAR_TDLS_WRCR, 0);
			DBG_871X("TDLS with "MAC_FMT"\n", MAC_ARG(ptdls_sta->hwaddr));

			pmlmeinfo->FW_sta_info[ptdls_sta->mac_id].psta = ptdls_sta;
			//set TDLS sta rate.
			set_sta_rate(padapter, ptdls_sta);
			break;
		case TDLS_SD_PTI:
			issue_tdls_peer_traffic_indication(padapter, ptdls_sta);
			break;
		case TDLS_CS_OFF:
			_cancel_timer_ex(&ptdls_sta->base_ch_timer);
			_cancel_timer_ex(&ptdls_sta->off_ch_timer);
			SelectChannel(padapter, pmlmeext->cur_channel);
			ptdls_sta->tdls_sta_state &= ~(TDLS_CH_SWITCH_ON_STATE |
								TDLS_PEER_AT_OFF_STATE |
								TDLS_AT_OFF_CH_STATE);
			DBG_871X("go back to base channel\n ");
			issue_nulldata(padapter, 0);
			break;
		case TDLS_INIT_CH_SEN:
			rtw_hal_set_hwreg(padapter, HW_VAR_TDLS_INIT_CH_SEN, 0);
			pmlmeext->sitesurvey_res.channel_idx = 0;
			ptdls_sta->option = TDLS_DONE_CH_SEN;
			rtw_tdls_cmd(padapter, ptdls_sta->hwaddr, TDLS_DONE_CH_SEN);
			break;
		case TDLS_DONE_CH_SEN:
			survey_channel = pmlmeext->channel_set[pmlmeext->sitesurvey_res.channel_idx].ChannelNum;
			if(survey_channel){
				SelectChannel(padapter, survey_channel);
				ptdlsinfo->cur_channel = survey_channel;
				pmlmeext->sitesurvey_res.channel_idx++;
				_set_timer(&ptdls_sta->option_timer, SURVEY_TO);
			}else{
				SelectChannel(padapter, pmlmeext->cur_channel);

				rtw_hal_set_hwreg(padapter, HW_VAR_TDLS_DONE_CH_SEN, 0);

				if(ptdlsinfo->ch_sensing==1){
					ptdlsinfo->ch_sensing=0;
					ptdlsinfo->cur_channel=1;
					min=ptdlsinfo->collect_pkt_num[0];
					for(i=1; i<MAX_CHANNEL_NUM-1; i++){
						if(min > ptdlsinfo->collect_pkt_num[i]){
							ptdlsinfo->cur_channel=i+1;
							min=ptdlsinfo->collect_pkt_num[i];
						}
						ptdlsinfo->collect_pkt_num[i]=0;
					}
					ptdlsinfo->collect_pkt_num[0]=0;
					ptdlsinfo->candidate_ch=ptdlsinfo->cur_channel;
					DBG_871X("TDLS channel sensing done, candidate channel: %02x\n", ptdlsinfo->candidate_ch);
					ptdlsinfo->cur_channel=0;

				}

				if(ptdls_sta->tdls_sta_state & TDLS_PEER_SLEEP_STATE){
					ptdls_sta->tdls_sta_state |= TDLS_APSD_CHSW_STATE;
				}else{
					//send null data with pwrbit==1 before send ch_switching_req to peer STA.
					issue_nulldata(padapter, 1);

					ptdls_sta->tdls_sta_state |= TDLS_CH_SW_INITIATOR_STATE;

					issue_tdls_ch_switch_req(padapter, ptdls_sta->hwaddr);
					DBG_871X("issue tdls ch switch req\n");
				}
			}
			break;
		case TDLS_OFF_CH:
			issue_nulldata(padapter, 1);
			SelectChannel(padapter, ptdls_sta->off_ch);

			DBG_871X("change channel to tar ch:%02x\n", ptdls_sta->off_ch);
			ptdls_sta->tdls_sta_state |= TDLS_AT_OFF_CH_STATE;
			ptdls_sta->tdls_sta_state &= ~(TDLS_PEER_AT_OFF_STATE);
			_set_timer(&ptdls_sta->option_timer, (u32)ptdls_sta->ch_switch_time);
			break;
		case TDLS_BASE_CH:
			_cancel_timer_ex(&ptdls_sta->base_ch_timer);
			_cancel_timer_ex(&ptdls_sta->off_ch_timer);
			SelectChannel(padapter, pmlmeext->cur_channel);
			ptdls_sta->tdls_sta_state &= ~(TDLS_CH_SWITCH_ON_STATE |
								TDLS_PEER_AT_OFF_STATE |
								TDLS_AT_OFF_CH_STATE);
			DBG_871X("go back to base channel\n ");
			issue_nulldata(padapter, 0);
			_set_timer(&ptdls_sta->option_timer, (u32)ptdls_sta->ch_switch_time);
			break;
		case TDLS_P_OFF_CH:
			SelectChannel(padapter, pmlmeext->cur_channel);
			issue_nulldata(padapter, 0);
			DBG_871X("change channel to base ch:%02x\n", pmlmeext->cur_channel);
			ptdls_sta->tdls_sta_state &= ~(TDLS_PEER_AT_OFF_STATE| TDLS_AT_OFF_CH_STATE);
			_set_timer(&ptdls_sta->off_ch_timer, TDLS_STAY_TIME);
			break;
		case TDLS_P_BASE_CH:
			issue_nulldata(ptdls_sta->padapter, 1);
			SelectChannel(padapter, ptdls_sta->off_ch);
			DBG_871X("change channel to off ch:%02x\n", ptdls_sta->off_ch);
			ptdls_sta->tdls_sta_state |= TDLS_AT_OFF_CH_STATE;
			if((ptdls_sta->tdls_sta_state & TDLS_PEER_AT_OFF_STATE) != TDLS_PEER_AT_OFF_STATE){
				issue_nulldata_to_TDLS_peer_STA(padapter, ptdls_sta, 0);
			}
			_set_timer(&ptdls_sta->base_ch_timer, TDLS_STAY_TIME);
			break;
		case TDLS_RS_RCR:
			rtw_hal_set_hwreg(padapter, HW_VAR_TDLS_RS_RCR, 0);
			DBG_871X("wirte REG_RCR, set bit6 on\n");
			break;
		case TDLS_CKALV_PH1:
			_set_timer(&ptdls_sta->alive_timer2, TDLS_ALIVE_TIMER_PH2);
			break;
		case TDLS_CKALV_PH2:
			_set_timer(&ptdls_sta->alive_timer1, TDLS_ALIVE_TIMER_PH1);
			break;
		case TDLS_FREE_STA:
			free_tdls_sta(padapter, ptdls_sta);
			break;

	}

	//_exit_critical_bh(&(ptdlsinfo->hdl_lock), &irqL);

	return H2C_SUCCESS;
#else
	return H2C_REJECTED;
#endif //CONFIG_TDLS

}

int rtw_get_ch_setting_union(_adapter *adapter, u8 *ch, u8 *bw, u8 *offset)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	_adapter *iface;
	struct mlme_ext_priv *mlmeext;
	int i;
	u8 ch_ret = 0;
	u8 bw_ret = HT_CHANNEL_WIDTH_20;
	u8 offset_ret = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
	int num = 0;

	if (ch) *ch = 0;
	if (bw) *bw = HT_CHANNEL_WIDTH_20;
	if (offset) *offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

	for (i = 0; i<dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		mlmeext = &iface->mlmeextpriv;

		if (!check_fwstate(&iface->mlmepriv, _FW_LINKED))
			continue;

		if (num == 0) {
			ch_ret = mlmeext->cur_channel;
			bw_ret = mlmeext->cur_bwmode;
			offset_ret = mlmeext->cur_ch_offset;
			num++;
			continue;
		}

		if (ch_ret != mlmeext->cur_channel) {
			num = 0;
			break;
		}

		if (bw_ret < mlmeext->cur_bwmode) {
			bw_ret = mlmeext->cur_bwmode;
			offset_ret = mlmeext->cur_ch_offset;
		} else if (bw_ret == mlmeext->cur_bwmode && offset_ret != mlmeext->cur_ch_offset) {
			num = 0;
			break;
		}

		num++;
	}

	if (num) {
		if (ch) *ch = ch_ret;
		if (bw) *bw = bw_ret;
		if (offset) *offset = offset_ret;
	}

	return num;
}



