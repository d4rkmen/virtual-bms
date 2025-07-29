/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/****************************************************************************
 *
 * This demo showcases BLE GATT client. It can scan BLE devices and bt_connect
 *to one device. Run the gatt_server demo, the client demo will automatically
 *bt_connect to the gatt_server demo. Client demo will enable gatt_server's
 *notify after connection. The two devices will then exchange data.
 *
 ****************************************************************************/

#include "ble.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bms.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mgos.h"
#include "mgos_config.h"

#define PROFILE_NUM BMS_MAX_COUNT
#define PROFILE_JKBMS_APP_ID 0
#define INVALID_HANDLE 0

static xSemaphoreHandle bleMutex;
static struct ble_scan_results scan_results;
static EventGroupHandle_t ble_event_group;
static const int SCAN_COMPLETE_BIT = BIT0;
static const int SCAN_FREE_BIT = BIT1;
static const int SCAN_PENDING_BIT = BIT2;

static int ble_refresh_timer_id = -1;
static int ble_scan_timer_id = -1;
static int ble_check_timer_id = -1;
static bool scan_params_set = false;

static esp_gattc_char_elem_t *char_elem_result = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;

/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event,
                       esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                         esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event,
                                        esp_gatt_if_t gattc_if,
                                        esp_ble_gattc_cb_param_t *param,
                                        int id);

static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid =
        {
            .uuid16 = JK_BMS_SERVICE_UUID,
        },
};

static esp_bt_uuid_t remote_filter_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid =
        {
            .uuid16 = JK_BMS_CHR_NOTIFY_UUID,
        },
};

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid =
        {
            .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
        },
};

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

typedef void (*gattc_cb_t)(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param, int id);

struct gattc_profile_inst {
  gattc_cb_t gattc_cb;
  uint16_t gattc_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_start_handle;
  uint16_t service_end_handle;
  uint16_t char_handle;
  esp_bd_addr_t remote_bda;
  uint16_t mtu;
  int timer_id;
  bool get_server;
  bool connected;
  double last_update;
  uint8_t bms;
  struct mbuf mbuf;
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the
 * gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [0] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,

        },
    [1] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [2] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [3] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [4] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [5] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [6] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [7] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [8] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [9] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [10] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [11] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [12] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [13] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
    [14] =
        {
            .gattc_cb = gattc_profile_event_handler,
            .gattc_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
            .get_server = false,
            .connected = false,
        },
};

static void ble_scan_start_cb(void *arg) {
  ble_scan_start((int) arg);
}

static void ble_scan_start_delayed(int delay) {
  if (ble_scan_timer_id != -1) {
    mgos_clear_timer(ble_scan_timer_id);
  }
  ble_scan_timer_id = mgos_set_timer(delay, 0, ble_scan_start_cb, (void *) 0);
}

int ble_scan_start(int try) {
  if (try == 0) {
    LOG(LL_INFO, ("BLE> Start scanning..."));
  } else {
    LOG(LL_INFO, ("BLE> Start scanning (try %d) ...", try));
  }
  // clear scan results only for try = -1
  if (try == -1) {
    if (xSemaphoreTake(bleMutex, portMAX_DELAY)) {
      scan_results.count = 0;
      memset(scan_results.devices, 0, sizeof(scan_results.devices));
      xEventGroupClearBits(ble_event_group, SCAN_COMPLETE_BIT);
      xSemaphoreGive(bleMutex);
    }
  }
  if (xEventGroupGetBits(ble_event_group) & SCAN_PENDING_BIT) {
    LOG(LL_INFO, ("BLE> Already scanning, exit"));
    return 2;
  } else {
    // wait for SCAN_BUSY_BIT for 5 sec to start scanning
    LOG(LL_INFO, ("BLE> BITS: %02X", xEventGroupGetBits(ble_event_group)));
    if ((xEventGroupWaitBits(ble_event_group, SCAN_FREE_BIT, pdFALSE, pdTRUE,
                             5000 / portTICK_PERIOD_MS) &
         SCAN_FREE_BIT) == 0) {
      LOG(LL_ERROR, ("BLE> Scan is busy, try again later"));
      return 3;
    }
    // check if all BMS are connected
    uint8_t not_connected = 0;
    for (int i = 0; i < PROFILE_NUM; i++) {
      struct mgos_config_bms *cfg = get_bms_config(i);
      if ((cfg != NULL) && (cfg->type == BMS_TYPE_JK_BT) &&
          (gl_profile_tab[i].connected == false)) {
        not_connected++;
      }
    }
    LOG(LL_INFO, ("BLE> Scan is free, not connected: %d", not_connected));
    // check also BLE scan for device list enabled
    if (not_connected == 0 && try != -1) {
      LOG(LL_INFO, ("BLE> All devices are connected, no need to scan"));
      return 2;
    } else {
      if (try == -1) {
        LOG(LL_INFO, ("BLE> Start scanning from RPC"));
      } else if (try == 0) {
        LOG(LL_INFO, ("BLE> Start scanning for %d devices", not_connected));
      } else {
        LOG(LL_INFO, ("BLE> Retry start scanning for %d devices (%d try)",
                      not_connected, try));
      }
      if (ble_scan_timer_id != -1) {
        mgos_clear_timer(ble_scan_timer_id);
      }
      esp_err_t ret = esp_ble_gap_start_scanning(BLE_SCAN_TIME_S);
      if (ret == ESP_OK) {
        // * need this? or callback will do the thing?
        xEventGroupSetBits(ble_event_group, SCAN_PENDING_BIT);
        ble_scan_timer_id = -1;
        LOG(LL_INFO, ("BLE> Start scanning success"));
        return 0;
      } else {
        LOG(LL_ERROR, ("BLE> Start scanning failed, status: %d", ret));
        ble_scan_timer_id = mgos_set_timer(
            BLE_SCAN_RETRY_DELAY_MS, 0, ble_scan_start_cb, (void *) (try + 1));
        return 1;
      }
    }
  }
}

void ble_scan_stop_cb(void *arg) {
  ble_scan_stop((int) arg);
}

int ble_scan_stop(int try) {
  if (xEventGroupGetBits(ble_event_group) & SCAN_PENDING_BIT) {
    if (try != 0) {
      LOG(LL_INFO, ("BLE> Retry stop scanning (%d try)", try));
    } else {
      LOG(LL_INFO, ("BLE> Stop scanning..."));
    }
    esp_err_t ret = esp_ble_gap_stop_scanning();
    if (ret == ESP_OK) {
      xEventGroupClearBits(ble_event_group, SCAN_PENDING_BIT);
      ble_scan_timer_id = -1;
      return 0;
    } else {
      LOG(LL_ERROR, ("BLE> Stop scanning failed, status: 0x%x", ret));
      if (ble_scan_timer_id != -1) {
        mgos_clear_timer(ble_scan_timer_id);
      }
      ble_scan_timer_id = mgos_set_timer(BLE_SCAN_RETRY_DELAY_MS, 0,
                                         ble_scan_stop_cb, (void *) (try + 1));
      return 1;
    }
  }
  return 2;
}

static void ble_check_cb(void *arg) {
  // ble_scan_start((int) arg);
  // check for stalled connections
  int connected = 0;
  for (int i = 0; i < PROFILE_NUM; i++) {
    struct mgos_config_bms *cfg = get_bms_config(i);
    if ((cfg != NULL) && (cfg->type == BMS_TYPE_JK_BT) &&
        (gl_profile_tab[i].connected == true)) {
      // check last update time
      if ((mg_time() - gl_profile_tab[i].last_update) > BLE_STALLED_TIME_S) {
        LOG(LL_ERROR, ("CHECK> [%d] Connection stalled, disconnect", i));
        esp_err_t ret = esp_ble_gattc_close(gl_profile_tab[i].gattc_if,
                                            gl_profile_tab[i].conn_id);
        if (ret != ESP_OK) {
          LOG(LL_ERROR, ("CHECK> [%d] Disconnect failed, status: %d", i, ret));
        } else {
          LOG(LL_INFO, ("CHECK> [%d] Disconnect success", i));
          // * need this? or callback will do the thing?
          gl_profile_tab[i].connected = false;
        }
      }
      if (gl_profile_tab[i].connected) connected++;
    }
  }
  if (connected >= BLE_MAX_COUNT) {
    LOG(LL_INFO, ("CHECK> All devices are connected"));
  } else {
    LOG(LL_INFO, ("CHECK> Connected devices: %d / %d, start scanning...",
                  connected, BLE_MAX_COUNT));
    ble_scan_start(0);
  }
}

static void ble_refresh_timer_cb(void *arg) {
  int id = (int) arg;
  uint8_t write_char_data[20] = {0xaa, 0x55, 0x90, 0xeb, 0x96, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x10};

  int rc = esp_ble_gattc_write_char(
      gl_profile_tab[id].gattc_if, gl_profile_tab[id].conn_id,
      gl_profile_tab[id].char_handle, sizeof(write_char_data), write_char_data,
      ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
  if (rc) {
    LOG(LL_ERROR, ("GATTC> Cell info failed, status: %d", rc));
  } else {
    LOG(LL_INFO, ("GATTC> [%d] Cell info success", id));
  }
  (void) arg;
}

static void gattc_profile_event_handler(esp_gattc_cb_event_t event,
                                        esp_gatt_if_t gattc_if,
                                        esp_ble_gattc_cb_param_t *param,
                                        int id) {
  char addr[BLE_ADDR_LEN];
  esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *) param;
  struct mbuf fb;
  struct json_out out = JSON_OUT_MBUF(&fb);

  switch (event) {
    case ESP_GATTC_REG_EVT:
      LOG(LL_INFO, ("GATTC> [%d] Client register, status: 0x%x, app_id: %d, "
                    "gattc_if: %d",
                    id, param->reg.status, param->reg.app_id, gattc_if));
      // set params only for the first time!
      if (!scan_params_set) {
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret) {
          LOG(LL_ERROR, ("GATTC> Set scan params error, code: %x", scan_ret));
        } else {
          LOG(LL_INFO, ("GATTC> Set scan params success"));
          scan_params_set = true;
        }
      }
      break;
    case ESP_GATTC_CONNECT_EVT: {
      // can do custom handle for connect profile

      // LOG(LL_INFO,
      //     ("GATTC> [%d] Connected, conn_id: %d, remote: " ESP_BD_ADDR_STR,
      //     id,
      //      p_data->connect.conn_id,
      //      ESP_BD_ADDR_HEX(p_data->connect.remote_bda)));
      // gl_profile_tab[id].conn_id = p_data->connect.conn_id;
      // memcpy(gl_profile_tab[id].remote_bda, p_data->connect.remote_bda,
      //        sizeof(esp_bd_addr_t));
      // esp_err_t mtu_ret =
      //     esp_ble_gattc_send_mtu_req(gattc_if, p_data->connect.conn_id);
      // if (mtu_ret) {
      //   LOG(LL_ERROR, ("GATTC> Config MTU error, code: %x", mtu_ret));
      // }
      break;
    }
    case ESP_GATTC_OPEN_EVT:
      if (param->open.status != ESP_GATT_OK) {
        LOG(LL_ERROR, ("GATTC> Open failed, status: %d", p_data->open.status));
        // can start scanning again
        xEventGroupSetBits(ble_event_group, SCAN_FREE_BIT);
        ble_scan_start_delayed(BLE_SCAN_RETRY_DELAY_MS);
        break;
      }
      gl_profile_tab[id].last_update = mg_time();
      LOG(LL_INFO,
          ("GATTC> [%d] Open successfully, MTU: %u", id, p_data->open.mtu));
      gl_profile_tab[id].mtu = p_data->open.mtu;
      break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
      if (param->dis_srvc_cmpl.status != ESP_GATT_OK) {
        LOG(LL_ERROR, ("GATTC> Service discover failed, status: %d",
                       param->dis_srvc_cmpl.status));
        break;
      }
      LOG(LL_INFO, ("GATTC> [%d] Service discover complete, conn_id: %d", id,
                    param->dis_srvc_cmpl.conn_id));
      esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id,
                                   &remote_filter_service_uuid);
      break;
    case ESP_GATTC_CFG_MTU_EVT:
      LOG(LL_INFO, ("GATTC> [%d] MTU exchange, status: %d, MTU: %d", id,
                    param->cfg_mtu.status, param->cfg_mtu.mtu));
      break;
    case ESP_GATTC_SEARCH_RES_EVT: {
      LOG(LL_INFO, ("GATTC> [%d] Service search result, addr: " ESP_BD_ADDR_STR
                    ", conn_id: 0x%x, is primary service: "
                    "%s",
                    id, ESP_BD_ADDR_HEX(gl_profile_tab[id].remote_bda),
                    p_data->search_res.conn_id,
                    p_data->search_res.is_primary ? "true" : "false"));
      LOG(LL_INFO,
          ("GATTC> [%d] start handle: 0x%x, end handle: 0x%x, handle value: "
           "0x%x",
           id, p_data->search_res.start_handle, p_data->search_res.end_handle,
           p_data->search_res.srvc_id.inst_id));
      if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 &&
          p_data->search_res.srvc_id.uuid.uuid.uuid16 == JK_BMS_SERVICE_UUID) {
        LOG(LL_INFO,
            ("GATTC> [%d] Service %04X found", id, JK_BMS_SERVICE_UUID));
        gl_profile_tab[id].get_server = true;
        gl_profile_tab[id].service_start_handle =
            p_data->search_res.start_handle;
        gl_profile_tab[id].service_end_handle = p_data->search_res.end_handle;
        LOG(LL_VERBOSE_DEBUG, ("GATTC> [%d]   UUID16: %04X", id,
                               p_data->search_res.srvc_id.uuid.uuid.uuid16));
      }
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
      if (p_data->search_cmpl.status != ESP_GATT_OK) {
        LOG(LL_ERROR, ("GATTC> Service search failed, status: %x",
                       p_data->search_cmpl.status));
        break;
      }
      if (p_data->search_cmpl.searched_service_source ==
          ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
        LOG(LL_INFO, ("GATTC> [%d] Get service information from remote "
                      "device: complete",
                      id));
      } else if (p_data->search_cmpl.searched_service_source ==
                 ESP_GATT_SERVICE_FROM_NVS_FLASH) {
        LOG(LL_INFO,
            ("GATTC> [%d] Get service information from flash: complete", id));
      } else {
        LOG(LL_INFO, ("GATTC> [%d] Unknown service source: complete", id));
      }
      if (gl_profile_tab[id].get_server) {
        uint16_t count = 0;
        esp_gatt_status_t status = esp_ble_gattc_get_attr_count(
            gattc_if, p_data->search_cmpl.conn_id, ESP_GATT_DB_CHARACTERISTIC,
            gl_profile_tab[id].service_start_handle,
            gl_profile_tab[id].service_end_handle, INVALID_HANDLE, &count);
        if (status != ESP_GATT_OK) {
          LOG(LL_ERROR,
              ("GATTC> Failed to get char count, status: %d", status));
          break;
        }

        if (count > 0) {
          char_elem_result = (esp_gattc_char_elem_t *) malloc(
              sizeof(esp_gattc_char_elem_t) * count);
          if (!char_elem_result) {
            LOG(LL_ERROR, ("GATTC> Failed to allocate memory for %d "
                           "characteristics: %d bytes",
                           count, sizeof(esp_gattc_char_elem_t) * count));
            break;
          } else {
            status = esp_ble_gattc_get_char_by_uuid(
                gattc_if, p_data->search_cmpl.conn_id,
                gl_profile_tab[id].service_start_handle,
                gl_profile_tab[id].service_end_handle, remote_filter_char_uuid,
                char_elem_result, &count);
            if (status != ESP_GATT_OK) {
              LOG(LL_ERROR,
                  ("GATTC> Failed to get char by UUID: %04X, status: %d",
                   remote_filter_char_uuid.uuid.uuid16, status));
              free(char_elem_result);
              char_elem_result = NULL;
              break;
            }

            for (int i = 0; i < count; i++) {
              LOG(LL_INFO, ("GATTC> [%d] Char: %04X, handle: %x, props: %02X",
                            id, char_elem_result[i].uuid.uuid.uuid16,
                            char_elem_result[i].char_handle,
                            char_elem_result[i].properties));
              if (char_elem_result[i].properties &
                  ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                gl_profile_tab[id].char_handle =
                    char_elem_result[i].char_handle;
                esp_ble_gattc_register_for_notify(
                    gattc_if, gl_profile_tab[id].remote_bda,
                    char_elem_result[i].char_handle);
              }
            }
          }
          /* free char_elem_result */
          free(char_elem_result);
        } else {
          LOG(LL_ERROR, ("GATTC> [%d] No characteristics found", id));
        }
      }
      break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      if (p_data->reg_for_notify.status != ESP_GATT_OK) {
        LOG(LL_ERROR, ("GATTC> Notification register failed, status: %d",
                       p_data->reg_for_notify.status));
      } else {
        LOG(LL_INFO, ("GATTC> [%d] Notification register successfully", id));
        uint16_t count = 0;
        uint16_t notify_en = 1;
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(
            gattc_if, gl_profile_tab[id].conn_id, ESP_GATT_DB_DESCRIPTOR,
            gl_profile_tab[id].service_start_handle,
            gl_profile_tab[id].service_end_handle,
            gl_profile_tab[id].char_handle, &count);
        if (ret_status != ESP_GATT_OK) {
          LOG(LL_ERROR, ("GATTC> Failed to get descriptor count, status: %d",
                         ret_status));
          break;
        }
        if (count > 0) {
          descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
          if (!descr_elem_result) {
            LOG(LL_ERROR, ("GATTC> Failed to allocate memory for %d "
                           "descriptors: %d bytes",
                           count, sizeof(esp_gattc_descr_elem_t) * count));
            break;
          } else {
            ret_status = esp_ble_gattc_get_descr_by_char_handle(
                gattc_if, gl_profile_tab[id].conn_id,
                p_data->reg_for_notify.handle, notify_descr_uuid,
                descr_elem_result, &count);
            if (ret_status != ESP_GATT_OK) {
              LOG(LL_ERROR,
                  ("GATTC> Failed to get descriptor %04X by char handle %x, "
                   "status: %d",
                   notify_descr_uuid.uuid.uuid16, p_data->reg_for_notify.handle,
                   ret_status));
              free(descr_elem_result);
              descr_elem_result = NULL;
              break;
            }
            // iterating through the descriptors
            for (int i = 0; i < count; i++) {
              LOG(LL_INFO, ("GATTC> [%d] Descr: %04X, handle: %x", id,
                            descr_elem_result[i].uuid.uuid.uuid16,
                            descr_elem_result[i].handle));

              if (descr_elem_result[i].uuid.len == ESP_UUID_LEN_16 &&
                  descr_elem_result[i].uuid.uuid.uuid16 ==
                      ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
                ret_status = esp_ble_gattc_write_char_descr(
                    gattc_if, gl_profile_tab[id].conn_id,
                    descr_elem_result[i].handle, sizeof(notify_en),
                    (uint8_t *) &notify_en, ESP_GATT_WRITE_TYPE_RSP,
                    ESP_GATT_AUTH_REQ_NONE);
              }

              if (ret_status != ESP_GATT_OK) {
                LOG(LL_ERROR, ("GATTC> Failed to write descriptor %04X, handle "
                               "%x, status: %d",
                               descr_elem_result[i].uuid.uuid.uuid16,
                               descr_elem_result[i].handle, ret_status));
              }
              /* free descr_elem_result */
            }
            free(descr_elem_result);
          }
        } else {
          LOG(LL_ERROR, ("GATTC> [%d] Decsriptors not found", id));
        }
      }
      break;
    }
    case ESP_GATTC_NOTIFY_EVT:
      snprintf(addr, BLE_ADDR_LEN, ESP_BD_ADDR_STR,
               ESP_BD_ADDR_HEX(p_data->notify.remote_bda));
      uint8_t *data = p_data->notify.value;
      uint16_t len = p_data->notify.value_len;

      mbuf_init(&fb, 0x100);
      json_printf(&out, "%H", len, data);
      if (p_data->notify.is_notify) {
        LOG(LL_VERBOSE_DEBUG, ("GATTC> [%d] Notification from %s: [%02X] %.*s",
                               id, addr, len, fb.len, fb.buf));
      } else {
        LOG(LL_VERBOSE_DEBUG, ("GATTC> [%d] Indication from %s: [%02X] %.*s",
                               id, addr, len, fb.len, fb.buf));
      }
      mbuf_free(&fb);
      // assemble packet and send to BMS
      // dropping packets larger than MAX_RESPONSE_SIZE
      if (gl_profile_tab[id].mbuf.len > JK_BMS_BLE_MAX_FRAME) {
        mbuf_clear(&gl_profile_tab[id].mbuf);
      }
      if (len >= 4 && data[0] == 0x55 && data[1] == 0xAA && data[2] == 0xEB &&
          data[3] == 0x90) {
        mbuf_clear(&gl_profile_tab[id].mbuf);
      }
      mbuf_append(&gl_profile_tab[id].mbuf, data, len);
      // trimming large buffer. not happened yet
      if (gl_profile_tab[id].mbuf.len > JK_BMS_BLE_MIN_FRAME) {
        mbuf_resize(&gl_profile_tab[id].mbuf, JK_BMS_BLE_MIN_FRAME);
      }
      if (gl_profile_tab[id].mbuf.len == JK_BMS_BLE_MIN_FRAME) {
        // last updated time
        gl_profile_tab[id].last_update = mg_time();
        // handling the data
        bms_handle_bt_data(id, &gl_profile_tab[id].mbuf);
        // clearing the buffer
        mbuf_clear(&gl_profile_tab[id].mbuf);
      }
      break;
    case ESP_GATTC_WRITE_DESCR_EVT:
      if (p_data->write.status != ESP_GATT_OK) {
        LOG(LL_ERROR, ("GATTC> Descriptor write failed, status: %x",
                       p_data->write.status));
        break;
      }
      LOG(LL_INFO, ("GATTC> [%d] Descriptor write successfully", id));
      // aa 55 90 eb 97 00 00 00 00 00 00 00 00 00 00 00 00 00 00 11
      uint8_t write_char_data[20] = {0xaa, 0x55, 0x90, 0xeb, 0x97, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x11};
      int rc = esp_ble_gattc_write_char(
          gattc_if, gl_profile_tab[id].conn_id, gl_profile_tab[id].char_handle,
          sizeof(write_char_data), write_char_data, ESP_GATT_WRITE_TYPE_NO_RSP,
          ESP_GATT_AUTH_REQ_NONE);
      if (rc) {
        LOG(LL_ERROR, ("GATTC> Device Info failed, status: %d", rc));
      } else {
        LOG(LL_INFO, ("GATTC> [%d] Device Info success", id));
      }
      if (ble_refresh_timer_id != -1) {
        mgos_clear_timer(ble_refresh_timer_id);
      }
      ble_refresh_timer_id =
          mgos_set_timer(1000, 0, ble_refresh_timer_cb, (void *) id);
      // enable scanning
      xEventGroupSetBits(ble_event_group, SCAN_FREE_BIT);
      // start the scanning again
      ble_scan_start_delayed(BLE_SCAN_RETRY_DELAY_MS);
      break;
    case ESP_GATTC_SRVC_CHG_EVT: {
      esp_bd_addr_t bda;
      memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
      LOG(LL_INFO, ("GATTC> [%d] Service change from: " ESP_BD_ADDR_STR, id,
                    ESP_BD_ADDR_HEX(bda)));
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
      if (p_data->write.status != ESP_GATT_OK) {
        LOG(LL_ERROR, ("GATTC> Characteristic write failed, status: %x",
                       p_data->write.status));
        break;
      }
      LOG(LL_INFO, ("GATTC> [%d] Characteristic write successfully", id));
      break;
    case ESP_GATTC_DISCONNECT_EVT:
      // moved to GAP cb, same for every app profile
      break;
    default:
      break;
  }
}

void ble_scan_results_print(struct json_out *out) {
  struct ble_device *dev;
  // wait for 5 seconds
  LOG(LL_INFO, ("BLE> ble_scan_results_print"));
  if (xSemaphoreTake(bleMutex, 5000 / portTICK_PERIOD_MS)) {
    for (int i = 0; i < scan_results.count; i++) {
      dev = &scan_results.devices[i];
      if (i > 0) json_printf(out, ",");
      json_printf(out, "{addr: %Q, name: %Q}", dev->addr, dev->name);
      LOG(LL_INFO, ("BLE> %s = %s", dev->addr, dev->name));
    }
    xSemaphoreGive(bleMutex);
  } else {
    LOG(LL_ERROR, ("BLE> Failed to take bleMutex"));
  }
}

// true if added
static bool ble_device_add(char *addr, uint8_t *name, uint8_t name_len) {
  struct ble_device *dev;
  bool found = false;
  if (xSemaphoreTake(bleMutex, portMAX_DELAY)) {
    for (int i = 0; i < scan_results.count; i++) {
      dev = &scan_results.devices[i];
      if (strcmp(dev->addr, addr) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      if (scan_results.count >= BLE_MAX_DEVICES) {
        LOG(LL_ERROR, ("BLE> Device list full, cannot add %s", addr));
        found = true;
      } else {
        // copy addr and name to the scan results
        dev = &scan_results.devices[scan_results.count];
        strncpy(dev->addr, addr, BLE_MAX_ADDRESS_LEN);
        strncpy(dev->name, (char *) name, name_len);
        scan_results.count++;
        LOG(LL_DEBUG, ("BLE> Device added: %s, %.*s", addr, name_len, name));
      }
    }
    xSemaphoreGive(bleMutex);
  }
  return !found;
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event,
                       esp_ble_gap_cb_param_t *param) {
  uint8_t *adv_name = NULL;
  uint8_t adv_name_len = 0;
  struct mbuf fb;
  struct json_out out = JSON_OUT_MBUF(&fb);
  char addr[BLE_ADDR_LEN];
  int id;

  mbuf_init(&fb, 0x100);

  switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
      LOG(LL_DEBUG, ("GAP> Scan parameters set, start scanning"));
      xEventGroupClearBits(ble_event_group, SCAN_COMPLETE_BIT);
      xEventGroupSetBits(ble_event_group, SCAN_FREE_BIT);
      xEventGroupClearBits(ble_event_group, SCAN_PENDING_BIT);
      ble_scan_start_delayed(BLE_SCAN_RETRY_DELAY_MS);
      break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
      // scan start complete event to indicate scan start successfully or
      // failed
      if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        LOG(LL_ERROR, ("GAP> Scanning start failed, status: 0x%x",
                       param->scan_start_cmpl.status));
        break;
      }
      LOG(LL_DEBUG, ("GAP> Scanning start successfully"));
      break;
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
      if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        LOG(LL_ERROR, ("GAP> Scanning stop failed, status: 0x%x",
                       param->scan_stop_cmpl.status));
        break;
      }
      LOG(LL_DEBUG, ("GAP> Scanning stop successfully"));
      // * need this? or scan_stop will do the thing?
      xEventGroupClearBits(ble_event_group, SCAN_PENDING_BIT);
      break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
      esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *) param;
      switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
          adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                              ESP_BLE_AD_TYPE_NAME_CMPL,
                                              &adv_name_len);
          json_printf(&out, "name: %.*s %H", adv_name_len, adv_name,
                      adv_name_len, adv_name);
          LOG(LL_VERBOSE_DEBUG, ("GAP> Scan result, device " ESP_BD_ADDR_STR
                                 ", name len %u, %.*s",
                                 ESP_BD_ADDR_HEX(scan_result->scan_rst.bda),
                                 adv_name_len, fb.len, fb.buf));
          if (adv_name != NULL) {
            snprintf(addr, BLE_ADDR_LEN, ESP_BD_ADDR_STR,
                     ESP_BD_ADDR_HEX(scan_result->scan_rst.bda));
            // fill scan result
            ble_device_add(addr, adv_name, adv_name_len);
            // look for the device in the list
            id = bms_get_id_by_addr(addr);
            if (id == -1) {
              LOG(LL_DEBUG, ("GAP> %s \"%.*s \" not in config, skipping", addr,
                            adv_name_len, adv_name));
              break;
            }
            LOG(LL_INFO,
                ("GAP> [%d] Device found: %s, %.*s, %s", id, addr, adv_name_len,
                 adv_name,
                 gl_profile_tab[id].connected ? "connected" : "not connected"));
            if (gl_profile_tab[id].connected == false) {
              // disabling scanning for connecting time
              xEventGroupClearBits(ble_event_group, SCAN_FREE_BIT);
              ble_scan_stop(0);
              LOG(LL_DEBUG, ("GAP> [%d] Connecting to %s, if: %d...", id, addr,
                             gl_profile_tab[id].gattc_if));
              int try = 1;
              esp_err_t ret;
              do {
                ret = esp_ble_gattc_open(
                    gl_profile_tab[id].gattc_if, scan_result->scan_rst.bda,
                    scan_result->scan_rst.ble_addr_type, true);
                if (ret != ESP_OK) {
                  LOG(LL_ERROR,
                      ("GAP> [%d] Connection try %d failed, status: %d", id,
                       try, ret));
                  if (try == 10) {
                    LOG(LL_ERROR,
                        ("GAP> [%d] Connection failed, giving up", id));
                    break;
                  } else {
                    try++;
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                  }
                }
              } while (ret != ESP_OK);
            }
          }

          break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
          LOG(LL_INFO, ("GAP> Search complete"));
          xEventGroupSetBits(ble_event_group, SCAN_COMPLETE_BIT);
          xEventGroupClearBits(ble_event_group, SCAN_PENDING_BIT);
          // start scanning again, if not all devices are connected
          ble_scan_start_delayed(30000);
          break;
        default:
          break;
      }
      break;
    }

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
      if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        LOG(LL_ERROR, ("GAP> Advertising stop failed, status %x",
                       param->adv_stop_cmpl.status));
        break;
      }
      LOG(LL_DEBUG, ("GAP> Advertising stop successfully"));
      break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
      LOG(LL_INFO,
          ("GAP> Connection params update, status %d, conn_int %d, "
           "latency %d, "
           "timeout %d",
           param->update_conn_params.status, param->update_conn_params.conn_int,
           param->update_conn_params.latency,
           param->update_conn_params.timeout));
      break;
    case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
      LOG(LL_DEBUG, ("GAP> Packet length update, status %d, rx %d, tx %d",
                    param->pkt_data_lenth_cmpl.status,
                    param->pkt_data_lenth_cmpl.params.rx_len,
                    param->pkt_data_lenth_cmpl.params.tx_len));
      break;
    default:
      break;
  }
  mbuf_free(&fb);
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                         esp_ble_gattc_cb_param_t *param) {
  char addr[BLE_ADDR_LEN];
  /* If event is register event, store the gattc_if for each profile */
  if (event == ESP_GATTC_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      LOG(LL_INFO,
          ("GATTC> Register app %d, gattc_if %d", param->reg.app_id, gattc_if));
      gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
      gl_profile_tab[param->reg.app_id].app_id = param->reg.app_id;
      mbuf_init(&gl_profile_tab[param->reg.app_id].mbuf, 0);
    } else {
      LOG(LL_ERROR, ("reg app failed, app_id %04x, status %d",
                     param->reg.app_id, param->reg.status));
      return;
    }
  }
  if (event == ESP_GATTC_CONNECT_EVT) {
    // call setup MTU only once, not for every profile
    snprintf(addr, BLE_ADDR_LEN, ESP_BD_ADDR_STR,
             ESP_BD_ADDR_HEX(param->connect.remote_bda));
    int id = bms_get_id_by_addr(addr);
    if (id != -1) {
      // set connected
      if (gl_profile_tab[id].connected == false) {
        gl_profile_tab[id].connected = true;
        LOG(LL_INFO, ("GATTC> [%d] Connected, conn_id: %d, addr: %s", id,
                      param->connect.conn_id, addr));
        gl_profile_tab[id].conn_id = param->connect.conn_id;
        memcpy(gl_profile_tab[id].remote_bda, param->connect.remote_bda,
               sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret =
            esp_ble_gattc_send_mtu_req(gattc_if, param->connect.conn_id);
        if (mtu_ret) {
          LOG(LL_ERROR, ("GATTC> Config MTU error, code: 0x%x", mtu_ret));
        }
      }
    }
  }
  if (event == ESP_GATTC_DISCONNECT_EVT) {
    snprintf(addr, BLE_ADDR_LEN, ESP_BD_ADDR_STR,
             ESP_BD_ADDR_HEX(param->disconnect.remote_bda));
    int id = bms_get_id_by_addr(addr);
    if (id != -1) {
      if (gl_profile_tab[id].connected == true) {
        gl_profile_tab[id].connected = false;
        gl_profile_tab[id].get_server = false;
        LOG(LL_INFO, ("GATTC> [%d] Disconnected: %s, reason 0x%x", id, addr,
                      param->disconnect.reason));
      }
    }
    // enable scanning
    xEventGroupSetBits(ble_event_group, SCAN_FREE_BIT);
    ble_scan_start_delayed(BLE_SCAN_RETRY_DELAY_MS);
  }
  /* If the gattc_if equal to profile A, call profile A cb handler,
   * so here call each profile's callback */
  do {
    int idx;
    for (idx = 0; idx < PROFILE_NUM; idx++) {
      if (gattc_if == ESP_GATT_IF_NONE ||
          gattc_if == gl_profile_tab[idx].gattc_if) {
        if (gl_profile_tab[idx].gattc_cb) {
          gl_profile_tab[idx].gattc_cb(event, gattc_if, param, idx);
        }
      }
    }
  } while (0);
}

bool ble_scan_wait(int timeout) {
  if (ble_event_group == NULL) {
    return false;
  }
  LOG(LL_INFO, ("GAP> Waiting for scan to complete"));
  if (xEventGroupWaitBits(ble_event_group, SCAN_COMPLETE_BIT, pdFALSE, pdTRUE,
                          timeout / portTICK_PERIOD_MS) &
      SCAN_COMPLETE_BIT) {
    LOG(LL_INFO, ("GAP> SCAN completed"));
    return true;
  } else {
    LOG(LL_INFO, ("GAP> SCAN timed out"));
    return false;
  }
}

void start_ble(void) {
  int ret;
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
  bleMutex = xSemaphoreCreateMutex();
  assert(bleMutex != NULL);
  ble_event_group = xEventGroupCreate();
  assert(ble_event_group != NULL);
  scan_results.count = 0;
  memset(scan_results.devices, 0, sizeof(scan_results.devices));
  // init events
  xEventGroupClearBits(ble_event_group, SCAN_COMPLETE_BIT);
  xEventGroupSetBits(ble_event_group, SCAN_FREE_BIT);
  xEventGroupClearBits(ble_event_group, SCAN_PENDING_BIT);

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  bt_cfg.ble_max_conn = 8;
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    LOG(LL_ERROR, ("%s initialize controller failed: %s", __func__,
                   esp_err_to_name(ret)));
    return;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    LOG(LL_ERROR,
        ("%s enable controller failed: %s", __func__, esp_err_to_name(ret)));
    return;
  }

  ret = esp_bluedroid_init();
  if (ret) {
    LOG(LL_ERROR,
        ("%s init bluetooth failed: %s", __func__, esp_err_to_name(ret)));
    return;
  }

  ret = esp_bluedroid_enable();
  if (ret) {
    LOG(LL_ERROR,
        ("%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret)));
    return;
  }

  // register the  callback function to the gap module
  ret = esp_ble_gap_register_callback(esp_gap_cb);
  if (ret) {
    LOG(LL_ERROR, ("%s gap register failed, error code = %x", __func__, ret));
    return;
  }

  // register the callback function to the gattc module
  ret = esp_ble_gattc_register_callback(esp_gattc_cb);
  if (ret) {
    LOG(LL_ERROR, ("%s gattc register failed, error code = %x", __func__, ret));
    return;
  }
  // register the application profiles
  for (int id = 0; id < PROFILE_NUM; id++) {
    struct mgos_config_bms *cfg = get_bms_config(id);
    // registering clients only for device from the added list
    if ((cfg == NULL) || (cfg->type != BMS_TYPE_JK_BT)) {
      continue;
    }
    ret = esp_ble_gattc_app_register(id);
    if (ret) {
      LOG(LL_ERROR, ("MAIN> [%d] %s gattc app register failed, code: 0x%x", id,
                     __func__, ret));
    } else {
      LOG(LL_INFO, ("MAIN> [%d] client registered for %s / %s", id, cfg->name,
                    cfg->addr));
    }
  }
  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
  if (local_mtu_ret) {
    LOG(LL_ERROR, ("Set local MTU failed, code: %x", local_mtu_ret));
  }
  // setup timer check if need to start scanning
  ble_check_timer_id = mgos_set_timer(BLE_CHECK_TIME_MS, MGOS_TIMER_REPEAT,
                                      ble_check_cb, (void *) 0);
}