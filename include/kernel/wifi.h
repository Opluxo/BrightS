#ifndef BRIGHTS_WIFI_H
#define BRIGHTS_WIFI_H

#include <stdint.h>

/* ===== 802.11 Constants ===== */
#define BRIGHTS_WIFI_MAX_SSID_LEN 32
#define BRIGHTS_WIFI_MAX_BSSIDS   32
#define BRIGHTS_WIFI_MAX_IF       4
#define BRIGHTS_WIFI_SCAN_TIMEOUT 5000  /* ms */

/* ===== 802.11 Frame Types ===== */
#define IEEE80211_FTYPE_MGMT    0x0
#define IEEE80211_FTYPE_CTL     0x1
#define IEEE80211_FTYPE_DATA    0x2

/* Management subtypes */
#define IEEE80211_STYPE_ASSOC_REQ   0x0
#define IEEE80211_STYPE_ASSOC_RESP  0x1
#define IEEE80211_STYPE_REASSOC_REQ 0x2
#define IEEE80211_STYPE_REASSOC_RESP 0x3
#define IEEE80211_STYPE_PROBE_REQ   0x4
#define IEEE80211_STYPE_PROBE_RESP  0x5
#define IEEE80211_STYPE_BEACON      0x8
#define IEEE80211_STYPE_ATIM        0x9
#define IEEE80211_STYPE_DISASSOC    0xA
#define IEEE80211_STYPE_AUTH        0xB
#define IEEE80211_STYPE_DEAUTH      0xC
#define IEEE80211_STYPE_ACTION      0xD

/* Control subtypes */
#define IEEE80211_STYPE_RTS         0xB
#define IEEE80211_STYPE_CTS         0xC
#define IEEE80211_STYPE_ACK         0xD

/* Data subtypes */
#define IEEE80211_STYPE_DATA        0x8
#define IEEE80211_STYPE_QOS_DATA    0x8

/* ===== 802.11 Frame Header ===== */
typedef struct {
  uint16_t frame_ctrl;
  uint16_t duration;
  uint8_t addr1[6];  /* DA/RA */
  uint8_t addr2[6];  /* SA/TA */
  uint8_t addr3[6];  /* BSSID */
  uint16_t seq_ctrl;
} __attribute__((packed)) ieee80211_hdr_t;

/* Frame control fields */
#define IEEE80211_FC( type, stype ) \
  ( (type) << 2 | (stype) << 4 )

/* Capability info */
#define IEEE80211_CAP_ESS   0x0001
#define IEEE80211_CAP_IBSS  0x0002
#define IEEE80211_CAP_PRIV  0x0010
#define IEEE80211_CAP_SHORT 0x0020
#define IEEE80211_CAP_QOS   0x0200

/* ===== Information Elements ===== */
#define IEEE80211_ELEMID_SSID       0
#define IEEE80211_ELEMID_RATES      1
#define IEEE80211_ELEMID_FH         2
#define IEEE80211_ELEMID_DS         3
#define IEEE80211_ELEMID_CF         4
#define IEEE80211_ELEMID_TIM        5
#define IEEE80211_ELEMID_IBSS       6
#define IEEE80211_ELEMID_COUNTRY    7
#define IEEE80211_ELEMID_CHALLENGE  16
#define IEEE80211_ELEMID_PWR_CONSTRAINT 32
#define IEEE80211_ELEMID_PWR_CAP      33
#define IEEE80211_ELEMID_TPC_REQUEST  34
#define IEEE80211_ELEMID_TPC_REPORT   35
#define IEEE80211_ELEMID_SUPP_CHANNELS 36
#define IEEE80211_ELEMID_RSN        48  /* WPA2/RSN */
#define IEEE80211_ELEMID_EXT_RATES  50
#define IEEE80211_ELEMID_VENDOR     221

/* ===== Authentication types ===== */
#define IEEE80211_AUTH_OPEN     0
#define IEEE80211_AUTH_SHARED   1
#define IEEE80211_AUTH_FAST_BSS 2

/* ===== Reason codes ===== */
#define IEEE80211_REASON_UNSPECIFIED  1
#define IEEE80211_REASON_AUTH_EXPIRE  2
#define IEEE80211_REASON_AUTH_LEAVE   3
#define IEEE80211_REASON_ASSOC_EXPIRE 4
#define IEEE80211_REASON_ASSOC_TOOMANY 5
#define IEEE80211_REASON_NOT_AUTHED   6

/* ===== WiFi interface state ===== */
#define WIFI_STATE_DOWN       0
#define WIFI_STATE_SCANNING   1
#define WIFI_STATE_AUTH       2
#define WIFI_STATE_ASSOC      3
#define WIFI_STATE_CONNECTED  4

/* ===== BSS entry (scan result) ===== */
typedef struct {
  uint8_t bssid[6];
  char ssid[BRIGHTS_WIFI_MAX_SSID_LEN + 1];
  uint8_t channel;
  int16_t rssi;       /* Signal strength (dBm) */
  uint16_t capab;     /* Capability info */
  uint8_t wpa;        /* WPA supported */
  uint8_t rsn;        /* RSN/WPA2 supported */
  uint8_t rates[8];   /* Supported rates */
  uint8_t rate_count;
} brights_wifi_bss_t;

/* ===== WiFi interface ===== */
typedef struct {
  char name[16];
  uint8_t mac[6];
  uint8_t bssid[6];
  char ssid[BRIGHTS_WIFI_MAX_SSID_LEN + 1];
  uint8_t channel;
  int state;
  int up;
  int16_t rssi;
  uint32_t ip_addr;
  uint32_t netmask;
  uint32_t gateway;
  /* Scan results */
  brights_wifi_bss_t bss_list[BRIGHTS_WIFI_MAX_BSSIDS];
  int bss_count;
  /* WPA/WPA2 */
  uint8_t wpa_pmk[32];  /* Pairwise Master Key */
  int wpa_version;      /* 0=none, 1=WPA, 2=WPA2 */
} brights_wifi_if_t;

/* ===== WiFi API ===== */

/* Initialize WiFi subsystem */
void brights_wifi_init(void);

/* Add a WiFi interface */
int brights_wifi_if_add(const char *name, const uint8_t *mac);

/* Get interface pointer */
brights_wifi_if_t *brights_wifi_if_get(const char *name);

/* Bring interface up/down */
int brights_wifi_if_up(const char *name);
int brights_wifi_if_down(const char *name);

/* Set IP configuration */
int brights_wifi_if_set_ip(const char *name, uint32_t ip, uint32_t netmask, uint32_t gateway);

/* Scan for networks (populates bss_list) */
int brights_wifi_scan(const char *name);

/* Connect to a network */
int brights_wifi_connect(const char *name, const char *ssid, const char *password);

/* Disconnect from current network */
int brights_wifi_disconnect(const char *name);

/* Get current state */
const char *brights_wifi_state_name(int state);

/* Send 802.11 management frame */
int brights_wifi_send_mgmt(const char *name, uint8_t subtype, const uint8_t *dst,
                           const uint8_t *body, uint32_t body_len);

/* Send 802.11 data frame (to net stack) */
int brights_wifi_send_data(const char *name, const uint8_t *dst, const void *data, uint32_t len);

/* Receive 802.11 frame (called by driver) */
void brights_wifi_recv(const uint8_t *frame, uint32_t len);

/* WPA/WPA2 helpers */
int brights_wpa_psk(const char *passphrase, const char *ssid, uint8_t *pmk);
int brights_wpa_4way_handshake(brights_wifi_if_t *iface, const uint8_t *anonce);

#endif
