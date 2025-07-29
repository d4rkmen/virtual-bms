#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mgos.h"

#define BLE_SCAN_TIME_S 5
#define BLE_STALLED_TIME_S 60
#define BLE_SCAN_TIMEOUT_MS (BLE_SCAN_TIME_S + 1) * 1000
// maximim connected devices same time
#define BLE_MAX_COUNT 9
// esp_ble_gap_disconnect
#define BLE_CHECK_TIME_MS 15000
#define BLE_SCAN_RETRY_DELAY_MS 2000
/* 16 Bit Service UUID */
#define JK_BMS_SERVICE_UUID 0xFFE0
#define JK_BMS_BLE_MIN_FRAME 300
#define JK_BMS_BLE_MAX_FRAME 320
/* 16 Bit Service Characteristic UUID */
// #define JK_BMS_CHARACTERISTIC_UUID 0xFEE7
#define JK_BMS_CHR_NOTIFY_UUID 0xFFE1
#define JK_BMS_CHR_WRITE_UUID 0xFFE2

#define BLE_MAX_NAME_LEN 32
#define BLE_MAX_ADDRESS_LEN 18
#define BLE_MAX_DEVICES 16

struct ble_device {
  char addr[BLE_MAX_ADDRESS_LEN];
  char name[BLE_MAX_NAME_LEN];
};

struct ble_scan_results {
  struct ble_device devices[BLE_MAX_DEVICES];
  int count;
};

void start_ble(void);
int ble_scan_start(int try);
int ble_scan_stop(int try);
bool ble_scan_wait(int timeout);
void ble_scan_results_print(struct json_out *out);

#ifdef __cplusplus
}
#endif