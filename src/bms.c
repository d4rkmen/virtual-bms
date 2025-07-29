#include "bms.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "mgos.h"
#include "mgos_config.h"

static struct mgos_config_bms *bms_cfg[BMS_MAX_COUNT];
static struct inv_deye_data bms_deye[BMS_MAX_COUNT];

static int refresh_timer_id;
static int current_bms = 0;
static int wait_bms = -1;
static int bms_uart_no = -1;
static uint8_t bms_buf[UART_MAX_BUF];
static size_t bms_buf_len = 0;
static xSemaphoreHandle bmsMutex;

static void bin_to_hex(char *to, const unsigned char *p, size_t len) {
  static const char *hex = "0123456789abcdef";

  for (; len--; p++) {
    *to++ = hex[p[0] >> 4];
    *to++ = hex[p[0] & 0x0f];
    if (len) *to++ = ' ';
  }
  *to = '\0';
}

bool bms_get_data(uint8_t bms, struct inv_deye_data *data) {
  if (bms >= BMS_MAX_COUNT) return false;
  if (bms_cfg[bms] == NULL) return false;
  if (bms_cfg[bms]->type != -1) {
    if (data != NULL) {
      if (xSemaphoreTake(bmsMutex, portMAX_DELAY)) {
        memcpy(data, &bms_deye[bms], sizeof(struct inv_deye_data));
        xSemaphoreGive(bmsMutex);
      }
    }
    // check if info is ready to avoid blank data
    return (bms_deye[bms].flags == INV_DEYE_GOT_ALL);
  }
  return false;
}

bool bms_get_data_by_addr(char *addr, struct inv_deye_data *data) {
  // iterate over all BMS and find the one with the address
  for (int i = 0; i < BMS_MAX_COUNT; i++) {
    if (bms_cfg[i] != NULL && bms_cfg[i]->type != -1 &&
        strcmp(bms_cfg[i]->addr, addr) == 0) {
      if (data != NULL) {
        if (xSemaphoreTake(bmsMutex, portMAX_DELAY)) {
          memcpy(data, &bms_deye[i], sizeof(struct inv_deye_data));
          xSemaphoreGive(bmsMutex);
        }
      }
      return true;
    }
  }
  return false;
}

int bms_get_id_by_addr(char *addr) {
  // iterate over all BMS and find the one with the address
  for (int i = 0; i < BMS_MAX_COUNT; i++) {
    if (bms_cfg[i] != NULL && bms_cfg[i]->type != -1 &&
        strcmp(bms_cfg[i]->addr, addr) == 0) {
      return i;
    }
  }
  return -1;
}

struct mgos_config_bms *get_bms_config(uint8_t bms) {
  if (bms >= BMS_MAX_COUNT) return NULL;
  return bms_cfg[bms];
}

static uint8_t checksum8(uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum;
}

void load_bms_config() {
  bms_cfg[0] = (struct mgos_config_bms *) mgos_sys_config_get_bms0();
  bms_cfg[1] = (struct mgos_config_bms *) mgos_sys_config_get_bms1();
  bms_cfg[2] = (struct mgos_config_bms *) mgos_sys_config_get_bms2();
  bms_cfg[3] = (struct mgos_config_bms *) mgos_sys_config_get_bms3();
  bms_cfg[4] = (struct mgos_config_bms *) mgos_sys_config_get_bms4();
  bms_cfg[5] = (struct mgos_config_bms *) mgos_sys_config_get_bms5();
  bms_cfg[6] = (struct mgos_config_bms *) mgos_sys_config_get_bms6();
  bms_cfg[7] = (struct mgos_config_bms *) mgos_sys_config_get_bms7();
}

static int next_bms(int current) {
  return (current < BMS_LAST) ? current + 1 : 0;
}

static void byd_pro_rs485_read() {
  // read from BMS
  uint8_t bms_packet[0x10];
  bms_packet[0] = atoi(bms_cfg[current_bms]->addr);
  // size big endian
  bms_packet[1] = 0;
  bms_packet[2] = 4;  // size
  bms_packet[3] = BMS_BYD_PROTOVOL_VER;
  bms_packet[4] = BMS_BYD_CMD_READ;
  bms_packet[5] = BMS_BYD_EOF_55;
  bms_packet[6] = BMS_BYD_EOF_AA;
  bms_packet[7] = checksum8(bms_packet + 1, 6);
  // store for rx event
  wait_bms = current_bms;
  mgos_uart_write(bms_uart_no, (uint8_t *) &bms_packet, 8);
  mgos_uart_flush(bms_uart_no);
}

static void bms_refresh_cb(void *arg) {
  // int64_t now = mgos_uptime_micros();
  int last_bms = current_bms;
  // get next BMS from the list, if all empty then return
  while ((bms_cfg[current_bms] == NULL) || (bms_cfg[current_bms]->type == -1)) {
    current_bms = next_bms(current_bms);
    // all BMS are empty
    if (current_bms == last_bms) return;
  }
  LOG(LL_VERBOSE_DEBUG, ("BMS %d> type: %d, addr: %s, name: %s...", current_bms,
                         bms_cfg[current_bms]->type, bms_cfg[current_bms]->addr,
                         bms_cfg[current_bms]->name));
  // call the BMS
  switch (bms_cfg[current_bms]->type) {
    case BMS_TYPE_BYD_PRO_RS485:
      byd_pro_rs485_read();
      break;
    case BMS_TYPE_JK_BT:
      break;
    case BMS_TYPE_JK_RS485:
      break;
    default:
      LOG(LL_ERROR, ("BMS %d> Unknown BMS type %d", current_bms,
                     bms_cfg[current_bms]->type));
      break;
  }
  // next BMS for the next timer
  current_bms = next_bms(current_bms);
}

void init_bms(void) {
  load_bms_config();
  for (int i = 0; i < BMS_MAX_COUNT; i++) {
    if (bms_cfg[i] != NULL) {
      bms_deye[i].addr = i;
      bms_deye[i].last_update = 0;
      // init serial_no
      if (bms_cfg[i]->serial != NULL) {
        strncpy(bms_deye[i].serial_no, bms_cfg[i]->serial, INV_DEYE_SERIAL_LEN);
      }
      bms_deye[i].flags = 0;
      switch (bms_cfg[i]->type) {
        case BMS_TYPE_BYD_PRO_RS485:
          strncpy(bms_deye[i].model, BMS_MANUFACTURER_BYD, INV_DEYE_MODEL_LEN);
          break;
        case BMS_TYPE_JK_BT:
        case BMS_TYPE_JK_RS485:
          strncpy(bms_deye[i].model, BMS_MANUFACTURER_JK, INV_DEYE_MODEL_LEN);
          break;
        default:
          strncpy(bms_deye[i].model, BMS_MANUFACTURER_DEYE, INV_DEYE_MODEL_LEN);
          break;
      }
      LOG(LL_INFO, ("Init BMS %d: type: %d, name: %s, model: %s", i,
                    bms_cfg[i]->type, bms_cfg[i]->name, bms_deye[i].model));
    }
  }
  int period = mgos_sys_config_get_app_refresh_ms();
  refresh_timer_id = mgos_set_timer(period, true, bms_refresh_cb, NULL);
  LOG(LL_INFO, ("BMS refresh running, period: %ums", period));
}

static int16_t get_cell_max(int16_t cells[], int count) {
  int16_t max = cells[0];
  for (int i = 0; i < count; i++) {
    if (cells[i] > max) max = cells[i];
  }
  return max;
}

static int16_t get_cell_min(int16_t cells[], int count) {
  int16_t min = cells[0];
  for (int i = 0; i < count; i++) {
    if (cells[i] < min) min = cells[i];
  }
  return min;
}

static int8_t find_index_by_addr(uint8_t addr) {
  for (int i = 0; i < BMS_MAX_COUNT; i++) {
    if (bms_cfg[i] != NULL && bms_cfg[i]->type != -1 &&
        atoi(bms_cfg[i]->addr) == addr) {
      return i;
    }
  }
  return -1;
}
static void bms_uart_dispatcher(int uart_no, void *arg) {
  assert(uart_no == bms_uart_no);
  size_t len = mgos_uart_read_avail(uart_no);
  if (len == 0) return;
  size_t room = UART_MAX_BUF - bms_buf_len;
  if (len > room) len = room;
  // log data in hex
  len = mgos_uart_read(uart_no, bms_buf + bms_buf_len, len);
  char str[0x80 * 2] = {
      0,
  };
  bin_to_hex(str, (unsigned char *) bms_buf + bms_buf_len, len);
  LOG(LL_DEBUG, ("BMS >%02X> %s", len, str));
  bms_buf_len += len;
  // validating the packet
  int8_t cur = find_index_by_addr(bms_buf[0]);
  if (cur == -1) {
    goto reset;
  }
  if (wait_bms != cur) {
    LOG(LL_ERROR,
        ("BMS: invalid address, expecting: %d, got: %d", wait_bms, cur));
    goto reset;
  }
  // got full packet?
  if (bms_buf_len < 8) return;
  // compare size
  size_t size = (bms_buf[1] << 8) | bms_buf[2];
  if (size != bms_buf_len - 4) {
    // LOG(LL_ERROR, ("BMS%d: invalid size, expecting: %02X, got: %02X",
    // wait_bms,
    //                size + 4, bms_buf_len));
    return;
  }
  // check version
  if (bms_buf[3] != BMS_BYD_PROTOVOL_VER) {
    LOG(LL_ERROR,
        ("BMS%d: invalid protocol version, expecting: %02X, got: %02X",
         wait_bms, BMS_BYD_PROTOVOL_VER, bms_buf[3]));
    goto reset;
  }
  // check command
  if (bms_buf[4] != (BMS_BYD_CMD_READ | 0x80)) {
    LOG(LL_ERROR, ("BMS%d: invalid command, expecting: %02X, got: %02X",
                   wait_bms, BMS_BYD_CMD_READ | 0x80, bms_buf[4]));
    goto reset;
  }
  // check EOF
  if ((bms_buf[5] != BMS_BYD_EOF_55) || (bms_buf[6] != BMS_BYD_EOF_AA)) {
    LOG(LL_ERROR,
        ("BMS%d: invalid EOF, expecting: %02X%02X, got: %02X%02X", wait_bms,
         BMS_BYD_EOF_55, BMS_BYD_EOF_AA, bms_buf[5], bms_buf[6]));
    goto reset;
  }
  // check checksum
  uint8_t sum = checksum8(bms_buf + 1, bms_buf_len - 2);
  uint8_t sum_actual = bms_buf[bms_buf_len - 1];
  if (sum != sum_actual) {
    LOG(LL_ERROR, ("BMS%d: invalid checksum, expecting: %02X, got: %02X",
                   wait_bms, sum, sum_actual));
    goto reset;
  }
  // result code
  if (bms_buf[7] != 0) {
    LOG(LL_ERROR, ("BMS%d: error code: %02X", wait_bms, bms_buf[7]));
    //! todo handle error
    goto reset;
  }
  // process the packet
  struct bms_byd_data *bms_data = (struct bms_byd_data *) (bms_buf + 8);
  // print all fields
  LOG(LL_INFO,
      ("BMS %d: SOC: %d, SOH: %d, V %d, I: %d, T: %d %d %d %d %d, W: %08X, "
       "P: %08X, F: %08X",
       wait_bms, bms_data->SOC, bms_data->SOH, bms_data->pack_voltage,
       bms_data->current, bms_data->cell_temp[0], bms_data->cell_temp[1],
       bms_data->cell_temp[2], bms_data->cell_temp[3], bms_data->cell_temp[4],
       bms_data->alarm_state, bms_data->protect_state, bms_data->fail_state));
  // for (int i = 0; i < 16; i++) {
  //   LOG(LL_INFO,
  //       ("BMS%d: cell_volt[%d]: %d", wait_bms, i, bms_data->cell_volt[i]));
  // }
  // LOG(LL_INFO, ("BMS%d: current: %d", wait_bms, bms_data->current));
  // for (int i = 0; i < 8; i++) {
  //   LOG(LL_INFO,
  //       ("BMS%d: cell_temp[%d]: %d", wait_bms, i, bms_data->cell_temp[i]));
  // }
  // LOG(LL_INFO, ("BMS%d: others: %d", wait_bms, bms_data->others));
  // LOG(LL_INFO, ("BMS%d: alarm_state: %d", wait_bms,
  // bms_data->alarm_state)); LOG(LL_INFO, ("BMS%d: protect_state: %d",
  // wait_bms, bms_data->protect_state)); LOG(LL_INFO, ("BMS%d: fail_state:
  // %d", wait_bms, bms_data->fail_state)); LOG(LL_INFO, ("BMS%d:
  // balance_state: %d", wait_bms, bms_data->balance_state)); LOG(LL_INFO,
  // ("BMS%d: battery_state: %d", wait_bms, bms_data->battery_state));
  // LOG(LL_INFO, ("BMS%d: SOC: %d", wait_bms, bms_data->SOC)); LOG(LL_INFO,
  // ("BMS%d: SOH: %d", wait_bms, bms_data->SOH)); LOG(LL_INFO,
  //     ("BMS%d: discharge_times: %d", wait_bms, bms_data->discharge_times));
  // LOG(LL_INFO, ("BMS%d: total_discharge_soc: %d", wait_bms,
  //               bms_data->total_discharge_soc));

  // fill bms_deye
  if (xSemaphoreTake(bmsMutex, portMAX_DELAY)) {
    bms_deye[wait_bms].last_update = mg_time();
    bms_deye[wait_bms].pack_voltage = bms_data->pack_voltage / 10;
    bms_deye[wait_bms].pack_current = bms_data->current / 10;
    bms_deye[wait_bms].remain_capacity =
        (bms_data->SOC * 5000 * bms_data->SOH / 100) / 100;
    bms_deye[wait_bms].cell_temp_avg =
        (10 * (bms_data->cell_temp[0] + bms_data->cell_temp[1] +
               bms_data->cell_temp[2] + bms_data->cell_temp[3])) /
        4;
    bms_deye[wait_bms].env_temp = bms_data->cell_temp[4] * 10;
    uint16_t warning = 0;
    uint16_t protection = 0;
    uint16_t fault_status = 0;
    // warnings
    if (bms_data->alarm_state & BMS_BYD_STATE_CELL_VOLT_HI)
      warning |= INV_DEYE_WARNING_CELL_VOLT_HI;
    if (bms_data->alarm_state & BMS_BYD_STATE_CELL_VOLT_LO)
      warning |= INV_DEYE_WARNING_CELL_VOLT_LO;
    if (bms_data->alarm_state & BMS_BYD_STATE_PACK_VOLT_HI)
      warning |= INV_DEYE_WARNING_PACK_VOLT_HI;
    if (bms_data->alarm_state & BMS_BYD_STATE_PACK_VOLT_LO)
      warning |= INV_DEYE_WARNING_PACK_VOLT_LO;
    if (bms_data->alarm_state & BMS_BYD_STATE_CHARGE_CUR_HI)
      warning |= INV_DEYE_WARNING_CHARGE_CUR_HI;
    if (bms_data->alarm_state & BMS_BYD_STATE_DISCHARGE_CUR_HI)
      warning |= INV_DEYE_WARNING_DISCHARGE_CUR_HI;
    if (bms_data->alarm_state &
        (BMS_BYD_STATE_CHARGE_TEMP_HI | BMS_BYD_STATE_DISCHARGE_TEMP_HI))
      warning |= INV_DEYE_WARNING_BATT_TEMP_HI;
    if (bms_data->alarm_state &
        (BMS_BYD_STATE_CHARGE_TEMP_LO | BMS_BYD_STATE_DISCHARGE_TEMP_LO))
      warning |= INV_DEYE_WARNING_BATT_TEMP_LO;
    if (bms_deye[wait_bms].env_temp > 500)
      warning |= INV_DEYE_WARNING_ENV_TEMP_HI;
    if (bms_deye[wait_bms].env_temp < -100)
      warning |= INV_DEYE_WARNING_ENV_TEMP_LO;
    if (bms_data->alarm_state & (BMS_BYD_STATE_LEW_TEMP_OVER_CHARGE_2 |
                                 BMS_BYD_STATE_LEW_TEMP_OVER_CHARGE_3))
      warning |= INV_DEYE_WARNING_MOSFET_TEMP_HI;
    if (bms_data->alarm_state & BMS_BYD_STATE_SOC_LO)
      warning |= INV_DEYE_WARNING_CAPACITY_LO;
    // protection
    if (bms_data->protect_state & BMS_BYD_STATE_CELL_VOLT_HI)
      protection |= INV_DEYE_PROTECTION_CELL_VOLT_HI;
    if (bms_data->protect_state & BMS_BYD_STATE_CELL_VOLT_LO)
      protection |= INV_DEYE_PROTECTION_CELL_VOLT_LO;
    if (bms_data->protect_state & BMS_BYD_STATE_PACK_VOLT_HI)
      protection |= INV_DEYE_PROTECTION_PACK_VOLT_HI;
    if (bms_data->protect_state & BMS_BYD_STATE_PACK_VOLT_LO)
      protection |= INV_DEYE_PROTECTION_PACK_VOLT_LO;
    if (bms_data->protect_state & (BMS_BYD_STATE_CHARGE_SHOR_CIRCUIT |
                                   BMS_BYD_STATE_DISCHARGE_SHOR_CIRCUIT))
      protection |= INV_DEYE_PROTECTION_SHORT_CIRCUIT;
    if (bms_data->protect_state &
        (BMS_BYD_STATE_CHARGE_CUR_HI | BMS_BYD_STATE_DISCHARGE_CUR_HI))
      protection |= INV_DEYE_PROTECTION_CUR_HI;
    if (bms_data->protect_state & BMS_BYD_STATE_CHARGE_TEMP_HI)
      protection |= INV_DEYE_PROTECTION_CHARGE_TEMP_HI;
    if (bms_data->protect_state & BMS_BYD_STATE_CHARGE_TEMP_LO)
      protection |= INV_DEYE_PROTECTION_CHARGE_TEMP_LO;
    if (bms_data->protect_state & BMS_BYD_STATE_DISCHARGE_TEMP_HI)
      protection |= INV_DEYE_PROTECTION_DISCHARGE_TEMP_HI;
    if (bms_data->protect_state & BMS_BYD_STATE_DISCHARGE_TEMP_LO)
      protection |= INV_DEYE_PROTECTION_DISCHARGE_TEMP_LO;
    // faults
    if (bms_data->fail_state & BMS_BYD_FAIL_TEMP_SENSOR_FAIL)
      fault_status |= INV_DEYE_FAULT_TEMP_SENSOR_FAIL;
    if (bms_data->fail_state &
        (BMS_BYD_FAIL_VOLT_SENSOR_FAIL | BMS_BYD_FAIL_CHARGE_TUBE_FAIL |
         BMS_BYD_FAIL_DISCHARGE_TUBE_FAIL | BMS_BYD_FAIL_CELL_BROKEN |
         BMS_BYD_FAIL_HEATER_FAIL | BMS_BYD_FAIL_CHARGE_CIRCUIT_FAIL))
      fault_status |= INV_DEYE_FAULT_COMMUNICATION;
    // status
    if (bms_data->battery_state & BMS_BYD_BAT_STATE_CHARGE)
      fault_status |= INV_DEYE_STATUS_CHARGE | INV_DEYE_STATUS_CHARGE_MOSFET;
    if (bms_data->battery_state & BMS_BYD_BAT_STATE_DISCHARGE)
      fault_status |=
          INV_DEYE_STATUS_DISCHARGE | INV_DEYE_STATUS_DISCHARGE_MOSFET;
    // update
    bms_deye[wait_bms].warning = warning;
    bms_deye[wait_bms].protection = protection;
    bms_deye[wait_bms].fault_status = fault_status;
    // SOC
    bms_deye[wait_bms].SOC = bms_data->SOC * 10;
    bms_deye[wait_bms].SOH = bms_data->SOH * 10;
    bms_deye[wait_bms].design_capacity = 50 * 100;  // 50Ah
    bms_deye[wait_bms].full_charge_capacity =
        bms_deye[wait_bms].design_capacity * bms_data->SOH / 100;
    bms_deye[wait_bms].cycle_count = bms_data->discharge_times;
    bms_deye[wait_bms].max_charge_current = 50 * 100;  // 50A
    bms_deye[wait_bms].max_cell_voltage =
        get_cell_max(bms_data->cell_volt, BMS_MAX_CELL_COUNT);
    bms_deye[wait_bms].min_cell_voltage =
        get_cell_min(bms_data->cell_volt, BMS_MAX_CELL_COUNT);
    bms_deye[wait_bms].max_cell_temp =
        get_cell_max(bms_data->cell_temp, 4) * 10;
    bms_deye[wait_bms].min_cell_temp =
        get_cell_min(bms_data->cell_temp, 4) * 10;
    bms_deye[wait_bms].mosfet_temp = bms_data->cell_temp[0] * 10;
    bms_deye[wait_bms].flags = INV_DEYE_GOT_ALL;
    bms_deye[wait_bms].nominal_float_voltage =
        (uint16_t) (bms_cfg[wait_bms]->voltage * 100);  // 5520;  // 55.2V
    //
    xSemaphoreGive(bmsMutex);
  }
  // reset buffer
reset:
#ifdef SELF_TEST
  if (bms_buf[0] == 0xBE) {
    LOG(LL_WARN,
        ("BMS: Got self-test ACK. Both (BMS and INV) interfaces are working"));
  }
#endif
  bms_buf_len = 0;
  memset(bms_buf, 0, UART_MAX_BUF);
}

static void init_uart_bms() {
  struct mgos_uart_config bms_ucfg;
  bms_uart_no = mgos_sys_config_get_app_bms_uart_no();

  mgos_uart_config_set_defaults(bms_uart_no, &bms_ucfg);
  bms_ucfg.baud_rate = 9600;
  bms_ucfg.num_data_bits = 8;
  bms_ucfg.parity = MGOS_UART_PARITY_NONE;
  bms_ucfg.stop_bits = MGOS_UART_STOP_BITS_1;
  bms_ucfg.rx_buf_size = 256;
  bms_ucfg.tx_buf_size = 256;
  if (!mgos_uart_configure(bms_uart_no, &bms_ucfg)) goto err;
  mgos_uart_set_dispatcher(bms_uart_no, bms_uart_dispatcher, (void *) NULL);
  mgos_uart_set_rx_enabled(bms_uart_no, true);

  LOG(LL_INFO,
      ("BMS UART%d initialized %u,%d%c%d", bms_uart_no, bms_ucfg.baud_rate,
       bms_ucfg.num_data_bits,
       bms_ucfg.parity == MGOS_UART_PARITY_NONE ? 'N' : bms_ucfg.parity + '0',
       bms_ucfg.stop_bits));
  return;
err:
  LOG(LL_ERROR, ("Failed to configure UART%d", bms_uart_no));
}

void bms_handle_jk_settings(uint8_t id, struct mbuf *data) {
  struct bms_jk_settings *settings = (struct bms_jk_settings *) data->buf;
  // print all the structure members
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_volt_smart_sleep: %d", id,
                         settings->cell_volt_smart_sleep));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_volt_lo_protection: %d", id,
                         settings->cell_volt_lo_protection));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_volt_lo_recovery: %d", id,
                         settings->cell_volt_lo_recovery));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_volt_hi_protection: %d", id,
                         settings->cell_volt_hi_protection));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_volt_hi_recovery: %d", id,
                         settings->cell_volt_hi_recovery));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_balance_trigger_volt: %d", id,
                         settings->cell_balance_trigger_volt));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: cell_volt_soc_100: %d", id, settings->cell_volt_soc_100));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: cell_volt_soc_0: %d", id, settings->cell_volt_soc_0));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: cell_req_charge_volt: %d", id, settings->cell_req_charge_volt));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: cell_req_float_volt: %d", id, settings->cell_req_float_volt));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: cell_volt_power_off: %d", id, settings->cell_volt_power_off));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: max_charge_current: %d", id, settings->max_charge_current));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: charge_ocp_delay: %d", id, settings->charge_ocp_delay));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: charge_ocp_recovery_time: %d", id,
                         settings->charge_ocp_recovery_time));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: max_discharge_current: %d", id,
                         settings->max_discharge_current));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: discharge_ocp_delay: %d", id, settings->discharge_ocp_delay));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: discharge_ocp_recovery_time: %d", id,
                         settings->discharge_ocp_recovery_time));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: scp_recovery_time: %d", id, settings->scp_recovery_time));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: max_balance_current: %d", id, settings->max_balance_current));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: charge_otp: %d", id, settings->charge_otp));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: charge_otp_recovery: %d", id, settings->charge_otp_recovery));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: discharge_otp: %d", id, settings->discharge_otp));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: discharge_otp_recovery: %d", id,
                         settings->discharge_otp_recovery));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: charge_utp: %d", id, settings->charge_utp));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: charge_utp_recovery: %d", id, settings->charge_utp_recovery));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: mosfet_otp: %d", id, settings->mosfet_otp));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: mosfet_otp_recovery: %d", id, settings->mosfet_otp_recovery));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_count: %d", id, settings->cell_count));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: charge_on: %d", id, settings->charge_on));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: discharge_on: %d", id, settings->discharge_on));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: balance_on: %d", id, settings->balance_on));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: design_capacity: %d", id, settings->design_capacity));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: scp_delay: %d", id, settings->scp_delay));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_volt_start_balance: %d", id,
                         settings->cell_volt_start_balance));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_volt_stop_balance: %d", id,
                         settings->cell_volt_stop_balance));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: device_addr: %d", id, settings->device_addr));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: precharge_time: %d", id, settings->precharge_time));

  if (xSemaphoreTake(bmsMutex, portMAX_DELAY)) {
    bms_deye[id].flags |= INV_DEYE_GOT_SETTINGS;
    // if (bms_deye[id].flags == INV_DEYE_GOT_ALL)
    bms_deye[id].last_update = mg_time();
    bms_deye[id].design_capacity = settings->design_capacity / 10;
    // ! workaround for lack of precision in total current for 655.35Ah
    bms_deye[id].max_charge_current =
        (settings->max_charge_current < 100000 ? settings->max_charge_current
                                               : 100000) /
        10;
    // bms_deye[id].max_charge_current = settings->max_charge_current / 10;
    bms_deye[id].nominal_float_voltage =
        (uint16_t) (bms_cfg[id]->voltage * 100);
    // 5520;  // 55.2V
    xSemaphoreGive(bmsMutex);
  }
  (void) id;
  (void) data;
}

void bms_handle_jk_cell_info(uint8_t id, struct mbuf *data) {
  struct bms_jk_cell_info *info = (struct bms_jk_cell_info *) data->buf;
  uint8_t cell_count = 0;
  for (int i = 0; i < BMS_JK_MAX_CELL_COUNT; i++) {
    if (info->cell_enable_bitmask & (1 << i)) {
      cell_count++;
      LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell %d: %dV, %dOhm", id, i,
                             info->cell_volt[i], info->cell_res[i]));
    }
  }
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_count: %d", id, cell_count));
  // dump all data in log
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: cell_enable_bitmask: %08X", id, info->cell_enable_bitmask));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: avg_cell_voltage: %d", id, info->avg_cell_voltage));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: delta_cell_voltage: %d", id, info->delta_cell_voltage));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: max_voltage_cell: %d", id, info->max_voltage_cell));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: min_voltage_cell: %d", id, info->min_voltage_cell));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: mosfet_temp: %d", id, info->mosfet_temp));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cell_res_warning_bitmask: %08X", id,
                         info->cell_res_warning_bitmask));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: pack_voltage: %d", id, info->pack_voltage));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: pack_power: %d", id, info->pack_power));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: pack_current: %d", id, info->pack_current));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: temp1: %d", id, info->temp1));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: temp2: %d", id, info->temp2));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: errors: %04X", id, info->errors));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: errors2: %04X", id, info->errors2));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: balance_current: %d", id, info->balance_current));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: balancing_state: %d", id, info->balancing_state));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: SOC: %d", id, info->SOC));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: remain_capacity: %d", id, info->remain_capacity));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: full_charge_capacity: %d", id, info->full_charge_capacity));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: cycle_count: %d", id, info->cycle_count));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: cycle_capacity: %d", id, info->cycle_capacity));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: SOH: %d", id, info->SOH));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: precharge: %d", id, info->precharge));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: user_alarm: %d", id, info->user_alarm));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: uptime_sec: %d", id, info->uptime_sec));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: charging_on: %d", id, info->charging_on));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: discharging_on: %d", id, info->discharging_on));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: precharging_on: %d", id, info->precharging_on));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: balancing_on: %d", id, info->balancing_on));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: release_timers: %d %d %d %d %d %d", id,
                         info->release_timers[0], info->release_timers[1],
                         info->release_timers[2], info->release_timers[3],
                         info->release_timers[4], info->release_timers[5]));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: temp_sensors_bitmask: %04X", id, info->temp_sensors_bitmask));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: heating_on: %d", id, info->heating_on));
  // LOG(LL_VERBOSE_DEBUG, ("BMS %d: dummy: %04X", id, info->dummy0000));
  // LOG(LL_VERBOSE_DEBUG,
  //     ("BMS %d: time_emergency_sec: %d", id, info->time_emergency_sec));
  // LOG(LL_VERBOSE_DEBUG, ("BMS %d: discharge_cur_correction_factor: %d", id,
  //                info->discharge_cur_correction_factor));
  // LOG(LL_VERBOSE_DEBUG, ("BMS %d: charge_volt: %d", id, info->charge_volt));
  // LOG(LL_VERBOSE_DEBUG, ("BMS %d: discharge_volt: %d", id,
  // info->discharge_volt)); LOG(LL_VERBOSE_DEBUG, ("BMS %d:
  // battery_voltage_correction_factor: %d", id,
  //                info->battery_volt_correction_factor));
  // LOG(LL_VERBOSE_DEBUG, ("BMS %d: battery_voltage_corrected: %d", id,
  //                info->battery_volt_corrected));
  // LOG(LL_VERBOSE_DEBUG, ("BMS %d: heating_current: %d", id,
  // info->heating_current));

  // LOG(LL_VERBOSE_DEBUG, ("BMS %d: charger_pulled_on: %d", id,
  // info->charger_pulled_on)); LOG(LL_VERBOSE_DEBUG, ("BMS %d: temp3: %d", id,
  // info->temp3)); LOG(LL_VERBOSE_DEBUG, ("BMS %d: temp4: %d", id,
  // info->temp4)); LOG(LL_VERBOSE_DEBUG, ("BMS %d: temp5: %d", id,
  // info->temp5)); LOG(LL_VERBOSE_DEBUG, ("BMS %d: time_enter_sleep: %d", id,
  // info->time_enter_sleep)); LOG(LL_VERBOSE_DEBUG, ("BMS %d: plc_state: %d",
  // id, info->plc_state));
  LOG(LL_INFO,
      ("BMS %d: SOC: %d, SOH: %d, V: %d, I: %d, T: %d %d,  E: %04X-%04X", id,
       info->SOC, info->SOH, info->pack_voltage, info->pack_current,
       info->temp1, info->temp2, info->errors, info->errors2));

  if (xSemaphoreTake(bmsMutex, portMAX_DELAY)) {
    bms_deye[id].flags |= INV_DEYE_GOT_CELL_INFO;
    bms_deye[id].last_update = mg_time();
    bms_deye[id].pack_voltage = info->pack_voltage / 10;
    bms_deye[id].pack_current = info->pack_current / 10;
    bms_deye[id].remain_capacity = info->remain_capacity / 10;
    bms_deye[id].cell_temp_avg = info->temp1;
    bms_deye[id].env_temp = info->temp2;
    // warnings / protection
    uint16_t warning = 0;
    uint16_t protection = 0;
    uint16_t fault_status = 0;

    if (info->errors &
        (BMS_JK_ERROR_WIRE_RESISTANCE | BMS_JK_ERROR_CELL_COUNT_MISMATCH |
         BMS_JK_ERROR_CURRENT_SENSOR_ANOMALY | BMS_JK_ERROR_COPROCESSOR_COMM)) {
      fault_status |= INV_DEYE_FAULT_COMMUNICATION;
    }
    if (info->errors & BMS_JK_ERROR_MOSFET_TEMP_HI)
      warning |= INV_DEYE_WARNING_MOSFET_TEMP_HI;
    if (info->errors & BMS_JK_ERROR_CELL_VOLT_HI) {
      warning |= INV_DEYE_WARNING_CELL_VOLT_HI;
      protection |= INV_DEYE_PROTECTION_CELL_VOLT_HI;
    }
    if (info->errors & BMS_JK_ERROR_CELL_VOLT_LO) {
      warning |= INV_DEYE_WARNING_CELL_VOLT_LO;
      protection |= INV_DEYE_PROTECTION_CELL_VOLT_LO;
    }
    if (info->errors & BMS_JK_ERROR_PACK_VOLT_HI) {
      warning |= INV_DEYE_WARNING_PACK_VOLT_HI;
      protection |= INV_DEYE_PROTECTION_PACK_VOLT_HI;
    }
    if (info->errors & BMS_JK_ERROR_PACK_VOLT_LO) {
      warning |= INV_DEYE_WARNING_PACK_VOLT_LO;
      protection |= INV_DEYE_PROTECTION_PACK_VOLT_LO;
    }
    if (info->errors & BMS_JK_ERROR_CHARGE_CUR_HI) {
      warning |= INV_DEYE_WARNING_CHARGE_CUR_HI;
      protection |= INV_DEYE_PROTECTION_CUR_HI;
    }
    if (info->errors & BMS_JK_ERROR_DISCHARGE_CUR_HI) {
      warning |= INV_DEYE_WARNING_DISCHARGE_CUR_HI;
      protection |= INV_DEYE_PROTECTION_CUR_HI;
    }
    if (info->errors & BMS_JK_ERROR_CHARGE_TEMP_HI) {
      warning |= INV_DEYE_WARNING_BATT_TEMP_HI;
      protection |= INV_DEYE_PROTECTION_CHARGE_TEMP_HI;
    }
    if (info->errors & BMS_JK_ERROR_CHARGE_TEMP_LO) {
      warning |= INV_DEYE_WARNING_BATT_TEMP_LO;
      protection |= INV_DEYE_PROTECTION_CHARGE_TEMP_LO;
    }
    if (info->errors & BMS_JK_ERROR_DISCHARGE_TEMP_HI) {
      warning |= INV_DEYE_WARNING_BATT_TEMP_HI;
      protection |= INV_DEYE_PROTECTION_DISCHARGE_TEMP_HI;
    }
    if (info->errors & (BMS_JK_ERROR_CHARGE_SHORT_CIRCUIT |
                        BMS_JK_ERROR_DISCHARGE_SHORT_CIRCUIT)) {
      protection |= INV_DEYE_PROTECTION_SHORT_CIRCUIT;
    }
    if (info->cell_res_warning_bitmask)
      fault_status |= INV_DEYE_FAULT_TEMP_SENSOR_FAIL;
    if (info->charging_on)
      fault_status |= INV_DEYE_STATUS_CHARGE | INV_DEYE_STATUS_CHARGE_MOSFET;
    if (info->discharging_on)
      fault_status |=
          INV_DEYE_STATUS_DISCHARGE | INV_DEYE_STATUS_DISCHARGE_MOSFET;
    bms_deye[id].warning = warning;
    bms_deye[id].protection = protection;
    bms_deye[id].fault_status = fault_status;
    bms_deye[id].SOC = info->SOC * 10;
    bms_deye[id].SOH = info->SOH * 10;
    bms_deye[id].full_charge_capacity = info->full_charge_capacity / 10;
    bms_deye[id].cycle_count = info->cycle_count;
    bms_deye[id].max_cell_voltage = get_cell_max(info->cell_volt, cell_count);
    bms_deye[id].min_cell_voltage = get_cell_min(info->cell_volt, cell_count);
    bms_deye[id].max_cell_temp = info->temp1;
    bms_deye[id].min_cell_temp = info->temp1;
    bms_deye[id].mosfet_temp = info->mosfet_temp;
    xSemaphoreGive(bmsMutex);
  }
  (void) id;
  (void) data;
}

void bms_handle_jk_device_info(uint8_t id, struct mbuf *data) {
  struct bms_jk_device_info *info = (struct bms_jk_device_info *) data->buf;
  LOG(LL_DEBUG, ("BMS %d: vendor_id: %s, name: %s, serial_no: %s", id,
                 info->vendor_id, info->name, info->serial_no));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: hw_ver: %s", id, info->hw_ver));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: sw_ver: %s", id, info->sw_ver));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: uptime: %d", id, info->uptime));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: power_on_count: %d", id, info->power_on_count));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: device_passcode: %s", id, info->device_passcode));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: mfg_date: %s", id, info->mfg_date));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: passcode: %s", id, info->passcode));
  LOG(LL_VERBOSE_DEBUG, ("BMS %d: user_data: %s", id, info->user_data));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: setup_passcode: %s", id, info->setup_passcode));
  LOG(LL_VERBOSE_DEBUG,
      ("BMS %d: reset_passcode: %s", id, info->reset_passcode));
  if (xSemaphoreTake(bmsMutex, portMAX_DELAY)) {
    bms_deye[id].flags |= INV_DEYE_GOT_DEVICE_INFO;
    bms_deye[id].last_update = mg_time();
    // set serial number
    if (strlen(info->serial_no) > 0) {
      strncpy(bms_deye[id].serial_no, info->serial_no,
              sizeof(bms_deye[id].serial_no));
    }
    xSemaphoreGive(bmsMutex);
  }
  (void) id;
  (void) data;
}

void bms_handle_bt_data(uint8_t id, struct mbuf *data) {
  if (data == NULL || data->len < 16) return;
  // check crc
  uint8_t *f = (uint8_t *) data->buf;
  uint8_t crc = f[data->len - 1];
  uint8_t calc_crc = checksum8(f, data->len - 1);
  if (crc != calc_crc) {
    LOG(LL_ERROR,
        ("BMS %d: invalid CRC, expecting: %02X, got: %02X", id, calc_crc, crc));
    return;
  }
  // process the packet
  uint8_t frame_type = data->buf[4];
  uint8_t counter = data->buf[5];
  switch (frame_type) {
    case BMS_JK_FRAME_TYPE_SETTINGS:
      LOG(LL_DEBUG, ("BMS %d: Settings 0x%02X", id, counter));
      bms_handle_jk_settings(id, data);
      break;
    case BMS_JK_FRAME_TYPE_CELL_INFO:
      LOG(LL_DEBUG, ("BMS %d: Cell Info 0x%02X", id, counter));
      bms_handle_jk_cell_info(id, data);
      break;
    case BMS_JK_FRAME_TYPE_DEVICE_INFO:
      LOG(LL_DEBUG, ("BMS %d: Device Info 0x%02X", id, counter));
      bms_handle_jk_device_info(id, data);
      break;
    case BMS_JK_FRAME_TYPE_DETAIL_LOG:
      LOG(LL_DEBUG, ("BMS %d: Detail Log 0x%02X", id, counter));
      break;
    default:
      LOG(LL_ERROR, ("BMS %d: Unsupported frame type: 0x%02X", id, frame_type));
  }
  (void) id;
}

void start_bms(void) {
  bmsMutex = xSemaphoreCreateMutex();
  assert(bmsMutex != NULL);
  init_bms();
  init_uart_bms();
}