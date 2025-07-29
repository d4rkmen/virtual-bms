/*
 * Copyright 2020 d4rkmen <darkmen@i.ua>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi.h"
#endif
#include "ble.h"
#include "bms.h"
#include "frozen.h"
#include "inv.h"
#include "mgos.h"
#include "mgos_config.h"
#include "mgos_rpc.h"
#include "reset_btn.h"

static void timer_cb(void *arg) {
  static bool s_tick_tock = false;
  LOG(LL_INFO,
      ("%s uptime: %.2lf, RAM: %lu, %lu free", (s_tick_tock ? "Tick" : "Tock"),
       mgos_uptime(), (unsigned long) mgos_get_heap_size(),
       (unsigned long) mgos_get_free_heap_size()));
  s_tick_tock = !s_tick_tock;
  (void) arg;
}

/* WiFi last event*/
static int wifi_state = MGOS_WIFI_EV_STA_DISCONNECTED;

static void wifi_timer_cb(void *arg) {
  int on_ms = 0, off_ms = 0;
  switch (wifi_state) {
    case MGOS_WIFI_EV_STA_DISCONNECTED: {
      on_ms = 500, off_ms = 500;
      break;
    }
    case MGOS_WIFI_EV_STA_CONNECTING: {
      on_ms = 50, off_ms = 950;
      break;
    }
    case MGOS_WIFI_EV_STA_CONNECTED: {
      on_ms = 0, off_ms = 0;
      break;
    }
    case MGOS_WIFI_EV_STA_IP_ACQUIRED: {
      on_ms = 0, off_ms = 0;
      break;
    }
    case MGOS_WIFI_EV_AP_STA_CONNECTED: {
      on_ms = 100, off_ms = 100;
      break;
    }
    case MGOS_WIFI_EV_AP_STA_DISCONNECTED: {
      on_ms = 500, off_ms = 500;
      break;
    }
  }
  mgos_gpio_blink(mgos_sys_config_get_pins_led(), on_ms, off_ms);
  (void) arg;
}

static void net_cb(int ev, void *evd, void *arg) {
  switch (ev) {
    case MGOS_NET_EV_DISCONNECTED:
      LOG(LL_INFO, ("%s", "Net disconnected"));
      break;
    case MGOS_NET_EV_CONNECTING:
      LOG(LL_INFO, ("%s", "Net connecting..."));
      break;
    case MGOS_NET_EV_CONNECTED:
      LOG(LL_INFO, ("%s", "Net connected"));
      break;
    case MGOS_NET_EV_IP_ACQUIRED:
      LOG(LL_INFO, ("%s", "Net got IP address"));
      break;
  }

  (void) evd;
  (void) arg;
}

#ifdef MGOS_HAVE_WIFI
static void wifi_cb(int ev, void *evd, void *arg) {
  wifi_state = ev;
  switch (ev) {
    case MGOS_WIFI_EV_STA_DISCONNECTED: {
      struct mgos_wifi_sta_disconnected_arg *da =
          (struct mgos_wifi_sta_disconnected_arg *) evd;
      LOG(LL_INFO, ("WiFi STA disconnected, reason %d", da->reason));
      break;
    }
    case MGOS_WIFI_EV_STA_CONNECTING:
      LOG(LL_INFO, ("WiFi STA connecting %p", arg));
      break;
    case MGOS_WIFI_EV_STA_CONNECTED:
      LOG(LL_INFO, ("WiFi STA connected %p", arg));
      break;
    case MGOS_WIFI_EV_STA_IP_ACQUIRED:
      LOG(LL_INFO,
          ("WiFi STA IP acquired: %s", mgos_sys_config_get_wifi_ap_ip()));
      break;
    case MGOS_WIFI_EV_AP_STA_CONNECTED: {
      struct mgos_wifi_ap_sta_connected_arg *aa =
          (struct mgos_wifi_ap_sta_connected_arg *) evd;
      LOG(LL_INFO, ("WiFi AP STA connected MAC %02x:%02x:%02x:%02x:%02x:%02x",
                    aa->mac[0], aa->mac[1], aa->mac[2], aa->mac[3], aa->mac[4],
                    aa->mac[5]));
      break;
    }
    case MGOS_WIFI_EV_AP_STA_DISCONNECTED: {
      struct mgos_wifi_ap_sta_disconnected_arg *aa =
          (struct mgos_wifi_ap_sta_disconnected_arg *) evd;
      LOG(LL_INFO,
          ("WiFi AP STA disconnected MAC %02x:%02x:%02x:%02x:%02x:%02x",
           aa->mac[0], aa->mac[1], aa->mac[2], aa->mac[3], aa->mac[4],
           aa->mac[5]));
      break;
    }
  }
  (void) arg;
}
#endif /* MGOS_HAVE_WIFI */

// static void service_button_handler(int pin, void *arg) {
//   struct mgos_m_net *mel = (struct mgos_m_net *) arg;
//   if (!mel) return;
// }

static void bms_cb(int ev, void *ev_data, void *arg) {
  // switch (ev) {
  //   case MGOS_M_NET_EV_INITIALIZED:
  //     LOG(LL_INFO, ("M-NET init done"));
  //     break;
  //   case MGOS_M_NET_EV_CONNECTED:
  //     LOG(LL_INFO, ("connected: %s", *(bool *) ev_data ? "true" : "false"));
  //     break;
  //   case MGOS_M_NET_EV_CONNECT_ERROR:
  //     LOG(LL_INFO, ("connect_error: %d", *(uint8_t *) ev_data));
  //     break;
  //   case MGOS_M_NET_EV_PACKET_WRITE:
  //     LOG(LL_INFO, ("tx: %s", (char *) ev_data));
  //     break;
  //   case MGOS_M_NET_EV_PACKET_READ:
  //     LOG(LL_INFO, ("rx: %s", (char *) ev_data));
  //     break;
  //   case MGOS_M_NET_EV_PARAMS_CHANGED: {
  //     struct mgos_m_net_params *params = (struct mgos_m_net_params *)
  //     ev_data; LOG(LL_INFO, ("power: %d, mode: %d, setpoint: %.1f, fan: %d,
  //     vane_vert: "
  //                   "%d, vane_horiz: %d, isee: %d",
  //                   params->power, params->mode, params->setpoint,
  //                   params->fan, params->vane_vert, params->vane_horiz,
  //                   params->isee));
  //     break;
  //   }
  //   case MGOS_M_NET_EV_ROOMTEMP_CHANGED:
  //     LOG(LL_INFO, ("room_temp: %.1f", *(float *) ev_data));
  //     break;
  //   case MGOS_M_NET_EV_PARAMS_SET:
  //     LOG(LL_INFO, ("new params applied by HVAC"));
  //     break;
  //   case MGOS_M_NET_EV_PACKET_READ_ERROR:
  //     LOG(LL_ERROR, ("error: packet crc"));
  //     break;
  //   case MGOS_M_NET_EV_TIMER:
  //     // mgos_wdt_feed();
  //     // LOG(LL_INFO, ("MEL timer"));
  //     break;
  //   default:
  //     LOG(LL_WARN, ("event: %d", ev));
  // }
  LOG(LL_WARN, ("event: %d", ev));
  (void) ev_data;
  (void) arg;
}

#define BMU_JSON_SIZE 512
// RPC handlers
static void bmu_get_summary_rpc_handler(struct mg_rpc_request_info *ri,
                                        void *cb_arg,
                                        struct mg_rpc_frame_info *fi,
                                        struct mg_str args) {
  struct mbuf fb;
  struct json_out out = JSON_OUT_MBUF(&fb);

  mbuf_init(&fb, BMU_JSON_SIZE);

  struct inv_deye_data data;
  json_printf(&out, "{timestamp: %ld, bms:[", (unsigned long) mg_time());
  int count = 0;
  for (int i = 0; i < BMS_MAX_COUNT; i++) {
    if (bms_get_data(i, &data)) {
      if ((mg_time() - data.last_update) > 10 * BMS_MAX_COUNT) continue;
      if (count++ > 0) json_printf(&out, ",");
      json_printf(
          &out,
          "{addr: %d, serial_no: %Q, model: %Q, last_update: "
          "%ld, pack_voltage: %d, pack_current: %d, remain_capacity: %d, "
          "cell_temp_avg: %d, env_temp: %d, warning: %d, protection: %d, "
          "fault_status: %d, SOC: %d, SOH: %d, full_charge_capacity: %d, "
          "cycle_count: %d, max_charge_current: %d, nominal_float_voltage: %d, "
          "design_capacity: %d}",
          data.addr, data.serial_no, data.model,
          (unsigned long) data.last_update, data.pack_voltage,
          data.pack_current, data.remain_capacity, data.cell_temp_avg,
          data.env_temp, data.warning, data.protection, data.fault_status,
          data.SOC, data.SOH, data.full_charge_capacity, data.cycle_count,
          data.max_charge_current, data.nominal_float_voltage,
          data.design_capacity);
    }
  }
  json_printf(&out, "]}");
  mg_rpc_send_responsef(ri, "%.*s", fb.len, fb.buf);
  ri = NULL;

  mbuf_free(&fb);

  (void) cb_arg;
  (void) fi;
}

// ble_scan_wait_task
static void ble_scan_wait_task(void *pvParameters) {
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *) pvParameters;
  struct mbuf fb;
  struct json_out out = JSON_OUT_MBUF(&fb);

  mbuf_init(&fb, BMU_JSON_SIZE);

  // if BLE is busy, useless to wait, reporting error
  if (ble_scan_start(-1) == 3) {
    json_printf(&out, "{timestamp:%ld, error:%Q}", (unsigned long) mg_time(),
                "BLE is busy");
  } else if (ble_scan_wait(BLE_SCAN_TIMEOUT_MS)) {
    json_printf(&out, "{timestamp:%ld, ble:[", (unsigned long) mg_time());
    ble_scan_results_print(&out);
    json_printf(&out, "]}");
  } else {
    json_printf(&out, "{timestamp:%ld, error:%Q}", (unsigned long) mg_time(),
                "BLE scan timeout");
  }
  mg_rpc_send_responsef(ri, "%.*s", fb.len, fb.buf);
  ri = NULL;

  mbuf_free(&fb);
  vTaskDelete(NULL);
}

static void bmu_scan_ble_rpc_handler(struct mg_rpc_request_info *ri,
                                     void *cb_arg, struct mg_rpc_frame_info *fi,
                                     struct mg_str args) {
  // start new task to wait for scan results and pass ri to it
  xTaskCreate(ble_scan_wait_task, "ble_scan_wait_task", 4096, (void *) ri,
              tskIDLE_PRIORITY, NULL);
}

#define MGOS_BMS_EV_BASE MGOS_EVENT_BASE('B', 'M', 'S')
#define MGOS_EVENT_GRP_BMS MGOS_BMS_EV_BASE

enum mgos_app_init_result mgos_app_init(void) {
  /* Simple repeating timer */
  mgos_set_timer(1000, MGOS_TIMER_REPEAT, timer_cb, NULL);

  // Register service button, just in case
  //   mgos_gpio_set_mode(mgos_sys_config_get_app_button_gpio(),
  //                      MGOS_GPIO_MODE_INPUT);
  //   mgos_gpio_set_button_handler(mgos_sys_config_get_app_button_gpio(),
  //                                MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_POS,
  //                                50, service_button_handler, mel);
  // * setup LED
  mgos_gpio_set_mode(mgos_sys_config_get_pins_led(), MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_write(mgos_sys_config_get_pins_led(), false);
  // * setup reset button
  reset_button_init();
  // * setup WiFi status LED timer
  mgos_set_timer(1000, MGOS_TIMER_REPEAT, wifi_timer_cb, NULL);
  if (mgos_sys_config_get_wifi_ap_enable()) {
    LOG(LL_WARN, ("Runing captive portal to setup WiFi"));
    return MGOS_APP_INIT_SUCCESS;
  };

  // mgos_wdt_set_timeout(40);
  // mgos_wdt_enable();

  /* BMS events */
  mgos_event_add_group_handler(MGOS_EVENT_GRP_BMS, bms_cb, NULL);

  /* Network connectivity events */
  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, net_cb, NULL);

#ifdef MGOS_HAVE_WIFI
  mgos_event_add_group_handler(MGOS_EVENT_GRP_WIFI, wifi_cb, NULL);
#endif
  start_bms();
  start_inv();
  start_ble();
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "VBMS.GetSummary", "{}", bmu_get_summary_rpc_handler,
                     NULL);
  mg_rpc_add_handler(c, "VBMS.ScanBLE", "{}", bmu_scan_ble_rpc_handler, NULL);

  return MGOS_APP_INIT_SUCCESS;
}
