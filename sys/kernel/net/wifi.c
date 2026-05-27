#include "kernel_util.h"
#include "wifi.h"
#include "net.h"
#include "../drivers/serial.h"
#include "printf.h"
#include <stdint.h>

/* ===== WiFi interfaces ===== */
static brights_wifi_if_t wifi_ifs[BRIGHTS_WIFI_MAX_IF];
static int wifi_if_count = 0;

/* ===== Sequence number counter ===== */
static uint16_t wifi_seq_counter = 0;

/* ===== Helper functions ===== */

static uint16_t wifi_next_seq(void)
{
  return (wifi_seq_counter++) & 0xFFF;
}

/* ===== State name ===== */

const char *brights_wifi_state_name(int state)
{
  switch (state) {
    case WIFI_STATE_DOWN:      return "down";
    case WIFI_STATE_SCANNING:  return "scanning";
    case WIFI_STATE_AUTH:      return "authenticating";
    case WIFI_STATE_ASSOC:     return "associating";
    case WIFI_STATE_CONNECTED: return "connected";
    default:                   return "unknown";
  }
}

/* ===== Interface management ===== */

void brights_wifi_init(void)
{
  wifi_if_count = 0;
  kutil_memset(wifi_ifs, 0, sizeof(wifi_ifs));
}

int brights_wifi_if_add(const char *name, const uint8_t *mac)
{
  if (wifi_if_count >= BRIGHTS_WIFI_MAX_IF) return -1;

  brights_wifi_if_t *iface = &wifi_ifs[wifi_if_count];
  uint32_t i = 0;
  while (name[i] && i < 15) { iface->name[i] = name[i]; ++i; }
  iface->name[i] = 0;

  kutil_memcpy(iface->mac, mac, 6);
  kutil_memset(iface->bssid, 0, 6);
  kutil_memset(iface->ssid, 0, sizeof(iface->ssid));
  iface->channel = 0;
  iface->state = WIFI_STATE_DOWN;
  iface->up = 0;
  iface->rssi = 0;
  iface->ip_addr = 0;
  iface->netmask = 0;
  iface->gateway = 0;
  iface->bss_count = 0;
  kutil_memset(iface->wpa_pmk, 0, sizeof(iface->wpa_pmk));
  iface->wpa_version = 0;

  ++wifi_if_count;
  return 0;
}

brights_wifi_if_t *brights_wifi_if_get(const char *name)
{
  for (int i = 0; i < wifi_if_count; ++i) {
    if (kutil_strcmp(wifi_ifs[i].name, name) == 0) {
      return &wifi_ifs[i];
    }
  }
  return 0;
}

int brights_wifi_if_up(const char *name)
{
  brights_wifi_if_t *iface = brights_wifi_if_get(name);
  if (!iface) return -1;
  iface->up = 1;
  return 0;
}

int brights_wifi_if_down(const char *name)
{
  brights_wifi_if_t *iface = brights_wifi_if_get(name);
  if (!iface) return -1;
  iface->up = 0;
  iface->state = WIFI_STATE_DOWN;
  return 0;
}

int brights_wifi_if_set_ip(const char *name, uint32_t ip, uint32_t netmask, uint32_t gateway)
{
  brights_wifi_if_t *iface = brights_wifi_if_get(name);
  if (!iface) return -1;
  iface->ip_addr = ip;
  iface->netmask = netmask;
  iface->gateway = gateway;
  return 0;
}

/* ===== Send management frame ===== */

int brights_wifi_send_mgmt(const char *name, uint8_t subtype, const uint8_t *dst,
                           const uint8_t *body, uint32_t body_len)
{
  brights_wifi_if_t *iface = brights_wifi_if_get(name);
  if (!iface || !iface->up) return -1;

  uint8_t frame[1024];
  kutil_memset(frame, 0, sizeof(frame));

  ieee80211_hdr_t *hdr = (ieee80211_hdr_t *)frame;
  hdr->frame_ctrl = IEEE80211_FC(IEEE80211_FTYPE_MGMT, subtype);
  hdr->duration = 0x013A;
  kutil_memcpy(hdr->addr1, dst, 6);
  kutil_memcpy(hdr->addr2, iface->mac, 6);
  kutil_memcpy(hdr->addr3, iface->bssid, 6);
  hdr->seq_ctrl = wifi_next_seq() << 4;

  if (body && body_len > 0) {
    uint32_t hdr_len = sizeof(ieee80211_hdr_t);
    if (hdr_len + body_len > sizeof(frame)) return -1;
    kutil_memcpy(frame + hdr_len, body, body_len);
  }

  uint32_t frame_len = sizeof(ieee80211_hdr_t) + body_len;

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: mgmt frame subtype=");
  char buf[8];
  int i = 0;
  uint32_t v = subtype;
  if (v == 0) { buf[i++] = '0'; }
  else { while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; } }
  buf[i] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " len=");
  i = 0;
  v = frame_len;
  if (v == 0) { buf[i++] = '0'; }
  else { while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; } }
  buf[i] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");

  return 0;
}

/* ===== Send data frame ===== */

int brights_wifi_send_data(const char *name, const uint8_t *dst, const void *data, uint32_t len)
{
  brights_wifi_if_t *iface = brights_wifi_if_get(name);
  if (!iface || !iface->up) return -1;
  if (iface->state != WIFI_STATE_CONNECTED) return -1;

  uint8_t frame[1600];
  kutil_memset(frame, 0, sizeof(frame));

  ieee80211_hdr_t *hdr = (ieee80211_hdr_t *)frame;
  hdr->frame_ctrl = IEEE80211_FC(IEEE80211_FTYPE_DATA, IEEE80211_STYPE_DATA);
  hdr->frame_ctrl |= 0x0200;
  hdr->duration = 0x013A;
  kutil_memcpy(hdr->addr1, iface->bssid, 6);
  kutil_memcpy(hdr->addr2, iface->mac, 6);
  kutil_memcpy(hdr->addr3, dst, 6);
  hdr->seq_ctrl = wifi_next_seq() << 4;

  uint32_t hdr_len = sizeof(ieee80211_hdr_t);
  if (hdr_len + len > sizeof(frame)) return -1;
  kutil_memcpy(frame + hdr_len, data, len);

  uint32_t frame_len = hdr_len + len;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: data frame len=");
  char buf[8];
  int i = 0;
  uint32_t v = frame_len;
  if (v == 0) { buf[i++] = '0'; }
  else { while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; } }
  buf[i] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");

  return 0;
}

/* ===== Forward declarations ===== */
static void wifi_handle_mgmt(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                             const uint8_t *frame, uint32_t len, uint8_t stype);
static void wifi_handle_beacon(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                               const uint8_t *body, uint32_t body_len);
static void wifi_handle_probe_resp(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                                   const uint8_t *body, uint32_t body_len);
static void wifi_handle_auth(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                             const uint8_t *body, uint32_t body_len);
static void wifi_handle_assoc_resp(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                                   const uint8_t *body, uint32_t body_len);
static void wifi_handle_disassoc(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                                 const uint8_t *body, uint32_t body_len);
static void wifi_handle_data(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                             const uint8_t *frame, uint32_t len);
static int wifi_send_probe_req(brights_wifi_if_t *iface, const char *ssid);
static int wifi_send_auth_req(brights_wifi_if_t *iface, const uint8_t *bssid);
static int wifi_send_assoc_req(brights_wifi_if_t *iface, const uint8_t *bssid);
static int wifi_send_disassoc(brights_wifi_if_t *iface, const uint8_t *bssid);

/* ===== Receive frame ===== */

void brights_wifi_recv(const uint8_t *frame, uint32_t len)
{
  if (len < sizeof(ieee80211_hdr_t)) return;

  ieee80211_hdr_t *hdr = (ieee80211_hdr_t *)frame;
  uint16_t fc = hdr->frame_ctrl;
  uint8_t ftype = (fc >> 2) & 0x3;
  uint8_t stype = (fc >> 4) & 0xF;

  brights_wifi_if_t *iface = 0;
  for (int i = 0; i < wifi_if_count; ++i) {
    if (wifi_ifs[i].up &&
        kutil_memcmp(wifi_ifs[i].mac, hdr->addr1, 6) == 0) {
      iface = &wifi_ifs[i];
      break;
    }
    if (wifi_ifs[i].up &&
        kutil_memcmp(hdr->addr1, "\xFF\xFF\xFF\xFF\xFF\xFF", 6) == 0) {
      iface = &wifi_ifs[i];
      break;
    }
  }
  if (!iface) return;

  switch (ftype) {
    case IEEE80211_FTYPE_MGMT:
      wifi_handle_mgmt(iface, hdr, frame, len, stype);
      break;
    case IEEE80211_FTYPE_DATA:
      wifi_handle_data(iface, hdr, frame, len);
      break;
  }
}

/* ===== Management frame handlers ===== */

static void wifi_handle_mgmt(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                             const uint8_t *frame, uint32_t len, uint8_t stype)
{
  const uint8_t *body = frame + sizeof(ieee80211_hdr_t);
  uint32_t body_len = len - sizeof(ieee80211_hdr_t);

  switch (stype) {
    case IEEE80211_STYPE_BEACON:
      wifi_handle_beacon(iface, hdr, body, body_len);
      break;
    case IEEE80211_STYPE_PROBE_RESP:
      wifi_handle_probe_resp(iface, hdr, body, body_len);
      break;
    case IEEE80211_STYPE_AUTH:
      wifi_handle_auth(iface, hdr, body, body_len);
      break;
    case IEEE80211_STYPE_ASSOC_RESP:
      wifi_handle_assoc_resp(iface, hdr, body, body_len);
      break;
    case IEEE80211_STYPE_DISASSOC:
    case IEEE80211_STYPE_DEAUTH:
      wifi_handle_disassoc(iface, hdr, body, body_len);
      break;
  }
}

static void wifi_handle_beacon(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                               const uint8_t *body, uint32_t body_len)
{
  if (iface->state == WIFI_STATE_SCANNING) {
    const uint8_t *p = body;
    uint32_t remaining = body_len;

    if (remaining < 12) return;
    p += 12;
    remaining -= 12;

    char ssid[BRIGHTS_WIFI_MAX_SSID_LEN + 1];
    uint8_t channel = 0;
    int wpa = 0, rsn = 0;

    kutil_memset(ssid, 0, sizeof(ssid));

    while (remaining >= 2) {
      uint8_t id = p[0];
      uint8_t elen = p[1];
      if (remaining < 2 + elen) break;

      const uint8_t *edata = p + 2;

      if (id == IEEE80211_ELEMID_SSID && elen <= BRIGHTS_WIFI_MAX_SSID_LEN) {
        kutil_memcpy(ssid, edata, elen);
        ssid[elen] = 0;
      } else if (id == IEEE80211_ELEMID_DS && elen >= 1) {
        channel = edata[0];
      } else if (id == IEEE80211_ELEMID_RSN) {
        rsn = 1;
      } else if (id == IEEE80211_ELEMID_VENDOR && elen >= 4) {
        if (edata[0] == 0x00 && edata[1] == 0x50 &&
            edata[2] == 0xF2 && edata[3] == 0x01) {
          wpa = 1;
        }
      }

      p += 2 + elen;
      remaining -= 2 + elen;
    }

    if (iface->bss_count < BRIGHTS_WIFI_MAX_BSSIDS) {
      brights_wifi_bss_t *bss = &iface->bss_list[iface->bss_count];
      kutil_memcpy(bss->bssid, hdr->addr3, 6);
      kutil_memcpy(bss->ssid, ssid, sizeof(bss->ssid));
      bss->channel = channel;
      bss->rssi = -70;
      bss->wpa = wpa;
      bss->rsn = rsn;
      bss->rate_count = 0;
      ++iface->bss_count;
    }
  }
}

static void wifi_handle_probe_resp(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                                   const uint8_t *body, uint32_t body_len)
{
  wifi_handle_beacon(iface, hdr, body, body_len);
}

static void wifi_handle_auth(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                             const uint8_t *body, uint32_t body_len)
{
  if (body_len < 6) return;

  uint16_t auth_algo = body[0] | (body[1] << 8);
  uint16_t auth_seq = body[2] | (body[3] << 8);
  uint16_t status = body[4] | (body[5] << 8);

  (void)auth_algo;

  if (status == 0 && auth_seq == 2) {
    iface->state = WIFI_STATE_ASSOC;
    wifi_send_assoc_req(iface, hdr->addr3);
  }
}

static void wifi_handle_assoc_resp(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                                   const uint8_t *body, uint32_t body_len)
{
  if (body_len < 4) return;

  uint16_t status = body[2] | (body[3] << 8);

  if (status == 0) {
    kutil_memcpy(iface->bssid, hdr->addr3, 6);
    iface->state = WIFI_STATE_CONNECTED;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: connected to ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, iface->ssid);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  }
}

static void wifi_handle_disassoc(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                                 const uint8_t *body, uint32_t body_len)
{
  (void)hdr; (void)body; (void)body_len;
  iface->state = WIFI_STATE_DOWN;
  kutil_memset(iface->bssid, 0, 6);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: disconnected\n");
}

static void wifi_handle_data(brights_wifi_if_t *iface, ieee80211_hdr_t *hdr,
                             const uint8_t *frame, uint32_t len)
{
  (void)iface; (void)hdr;
  uint32_t hdr_len = sizeof(ieee80211_hdr_t);
  if (len <= hdr_len) return;

  brights_net_recv(frame + hdr_len, len - hdr_len);
}

/* ===== Send management frames ===== */

static int wifi_send_probe_req(brights_wifi_if_t *iface, const char *ssid)
{
  uint8_t body[64];
  uint32_t body_len = 0;

  uint32_t ssid_len = ssid ? kutil_strlen(ssid) : 0;
  if (ssid_len > BRIGHTS_WIFI_MAX_SSID_LEN) ssid_len = BRIGHTS_WIFI_MAX_SSID_LEN;

  body[body_len++] = IEEE80211_ELEMID_SSID;
  body[body_len++] = (uint8_t)ssid_len;
  if (ssid_len > 0) {
    kutil_memcpy(body + body_len, ssid, ssid_len);
    body_len += ssid_len;
  }

  body[body_len++] = IEEE80211_ELEMID_RATES;
  body[body_len++] = 4;
  body[body_len++] = 0x82;
  body[body_len++] = 0x84;
  body[body_len++] = 0x8B;
  body[body_len++] = 0x96;

  uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  return brights_wifi_send_mgmt(iface->name, IEEE80211_STYPE_PROBE_REQ,
                                broadcast, body, body_len);
}

static int wifi_send_auth_req(brights_wifi_if_t *iface, const uint8_t *bssid)
{
  uint8_t body[6];
  body[0] = 0x00; body[1] = 0x00;
  body[2] = 0x01; body[3] = 0x00;
  body[4] = 0x00; body[5] = 0x00;

  iface->state = WIFI_STATE_AUTH;
  return brights_wifi_send_mgmt(iface->name, IEEE80211_STYPE_AUTH,
                                bssid, body, sizeof(body));
}

static int wifi_send_assoc_req(brights_wifi_if_t *iface, const uint8_t *bssid)
{
  uint8_t body[128];
  uint32_t body_len = 0;

  body[body_len++] = 0x01; body[body_len++] = 0x00;
  body[body_len++] = 0x0A; body[body_len++] = 0x00;

  uint32_t ssid_len = kutil_strlen(iface->ssid);
  body[body_len++] = IEEE80211_ELEMID_SSID;
  body[body_len++] = (uint8_t)ssid_len;
  kutil_memcpy(body + body_len, iface->ssid, ssid_len);
  body_len += ssid_len;

  body[body_len++] = IEEE80211_ELEMID_RATES;
  body[body_len++] = 4;
  body[body_len++] = 0x82;
  body[body_len++] = 0x84;
  body[body_len++] = 0x8B;
  body[body_len++] = 0x96;

  return brights_wifi_send_mgmt(iface->name, IEEE80211_STYPE_ASSOC_REQ,
                                bssid, body, body_len);
}

static int wifi_send_disassoc(brights_wifi_if_t *iface, const uint8_t *bssid)
{
  uint8_t body[2];
  body[0] = 0x00; body[1] = 0x00;

  return brights_wifi_send_mgmt(iface->name, IEEE80211_STYPE_DISASSOC,
                                bssid, body, sizeof(body));
}

/* ===== Scan ===== */

int brights_wifi_scan(const char *name)
{
  brights_wifi_if_t *iface = brights_wifi_if_get(name);
  if (!iface || !iface->up) return -1;

  iface->state = WIFI_STATE_SCANNING;
  iface->bss_count = 0;

  wifi_send_probe_req(iface, 0);

  return 0;
}

/* ===== Connect ===== */

int brights_wifi_connect(const char *name, const char *ssid, const char *password)
{
  brights_wifi_if_t *iface = brights_wifi_if_get(name);
  if (!iface || !iface->up) return -1;

  const brights_wifi_bss_t *target = 0;
  for (int i = 0; i < iface->bss_count; ++i) {
    if (kutil_strcmp(iface->bss_list[i].ssid, ssid) == 0) {
      target = &iface->bss_list[i];
      break;
    }
  }

  if (!target) {
    brights_wifi_scan(name);
    for (int i = 0; i < iface->bss_count; ++i) {
      if (kutil_strcmp(iface->bss_list[i].ssid, ssid) == 0) {
        target = &iface->bss_list[i];
        break;
      }
    }
  }

  if (!target) return -1;

  kutil_memcpy(iface->ssid, target->ssid, sizeof(iface->ssid));
  kutil_memcpy(iface->bssid, target->bssid, 6);
  iface->channel = target->channel;
  iface->rssi = target->rssi;

  if (target->rsn) {
    iface->wpa_version = 2;
    if (password) brights_wpa_psk(password, ssid, iface->wpa_pmk);
  } else if (target->wpa) {
    iface->wpa_version = 1;
    if (password) brights_wpa_psk(password, ssid, iface->wpa_pmk);
  } else {
    iface->wpa_version = 0;
  }

  wifi_send_auth_req(iface, target->bssid);

  return 0;
}

/* ===== Disconnect ===== */

int brights_wifi_disconnect(const char *name)
{
  brights_wifi_if_t *iface = brights_wifi_if_get(name);
  if (!iface) return -1;

  if (iface->state >= WIFI_STATE_ASSOC) {
    wifi_send_disassoc(iface, iface->bssid);
  }

  iface->state = WIFI_STATE_DOWN;
  kutil_memset(iface->bssid, 0, 6);
  kutil_memset(iface->ssid, 0, sizeof(iface->ssid));
  return 0;
}

/* ===== WPA/WPA2 helpers ===== */

int brights_wpa_psk(const char *passphrase, const char *ssid, uint8_t *pmk)
{
  uint32_t pass_len = kutil_strlen(passphrase);
  uint32_t ssid_len = kutil_strlen(ssid);

  kutil_memset(pmk, 0, 32);
  for (uint32_t i = 0; i < pass_len && i < 32; ++i) {
    pmk[i] = (uint8_t)(passphrase[i] ^ ssid[i % ssid_len]);
  }

  return 0;
}

int brights_wpa_4way_handshake(brights_wifi_if_t *iface, const uint8_t *anonce)
{
  (void)anonce;
  (void)iface;
  return 0;
}
