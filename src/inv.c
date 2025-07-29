#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// #include "driver/gpio.h"
// #include "driver/twai.h"  // Update from V4.2
// #include "driver/uart.h"
// #include "esp_err.h"
// #include "esp_event.h"
#include "mgos.h"
#include "mgos_config.h"
// #include "esp_system.h"

#include "bms.h"
#include "inv.h"
static int inv_uart_no = -1;
static uint8_t inv_buf[UART_MAX_BUF];
static size_t inv_buf_len = 0;
static int uart_timer_id = -1;

static uint8_t crc_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40};
static uint8_t crc_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
    0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
    0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
    0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
    0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
    0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
    0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
    0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
    0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
    0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
    0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
    0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
    0x41, 0x81, 0x80, 0x40};

static void bin_to_hex(char *to, const unsigned char *p, size_t len) {
  static const char *hex = "0123456789abcdef";

  for (; len--; p++) {
    *to++ = hex[p[0] >> 4];
    *to++ = hex[p[0] & 0x0f];
    if (len) *to++ = ' ';
  }
  *to = '\0';
}

static uint16_t crc16(uint8_t *data, size_t size) {
  uint8_t hi = 0xFF;
  uint8_t lo = 0xFF;
  uint16_t index;
  while (size--) {
    index = lo ^ *data++;
    lo = hi ^ crc_hi[index];
    hi = crc_lo[index];
  }
  return (hi << 8 | lo);
}

// void deye_rs485_read() {
//   // read from BMS
//   uint8_t bms_packet[0x10];
//   bms_packet[0] = current_bms;
//   // size big endian
//   bms_packet[1] = 0;
//   bms_packet[2] = 4;
//   bms_packet[3] = BMS_BYD_PROTOVOL_VER;
//   bms_packet[4] = BMS_BYD_CMD_READ;
//   bms_packet[5] = BMS_BYD_EOF_55;
//   bms_packet[6] = BMS_BYD_EOF_AA;
//   bms_packet[7] = checksum8(bms_packet + 1, 6);
//   // store for rx event
//   wait_bms = current_bms;
//   mgos_uart_write(bms_uart_no, (uint8_t *) &bms_packet, 8);
//   mgos_uart_flush(bms_uart_no);
// }
static uint16_t inv_reg_read(uint8_t addr, struct inv_deye_data *data,
                             uint16_t reg) {
  // double now = mg_time();
  // uint16_t fault = 0;
  // if ((now - data->last_update) > BMS_MAX_COUNT * 10) {
  //   fault = INV_DEYE_FAULT_COMMUNICATION;
  // }
  switch (reg) {
    case INV_DEYE_REG_PACK_VOLTAGE:
      return data->pack_voltage;
    case INV_DEYE_REG_PACK_CURRENT:
      return data->pack_current;
    case INV_DEYE_REG_REMAINING_CAPACITY:
      return data->remain_capacity;
    case INV_DEYE_REG_CELL_TEMP_AVG:
      return data->cell_temp_avg;
    case INV_DEYE_REG_ENV_TEMP:
      return data->env_temp;
    case INV_DEYE_REG_WARNING:
      return data->warning;
    case INV_DEYE_REG_PROTECTION:
      return data->protection;
    case INV_DEYE_REG_FAULT_STATUS:
      return data->fault_status;
    case INV_DEYE_REG_SOC:
      return data->SOC;
    case INV_DEYE_REG_SOH:
      return data->SOH;
    case INV_DEYE_REG_FULL_CHARGE_CAPACITY:
      return data->full_charge_capacity;
    case INV_DEYE_REG_CYCLE_COUNT:
      return data->cycle_count;
    case INV_DEYE_REG_MAX_CHARGE_CURRENT:
      return data->max_charge_current;
    case INV_DEYE_REG_MAX_CELL_VOLTAGE:
      return data->max_cell_voltage;
    case INV_DEYE_REG_MIN_CELL_VOLTAGE:
      return data->min_cell_voltage;
    case INV_DEYE_REG_MAX_CELL_TEMP:
      return data->max_cell_temp;
    case INV_DEYE_REG_MIN_CELL_TEMP:
      return data->min_cell_temp;
    case INV_DEYE_REG_MOSFET_TEMP:
      return data->mosfet_temp;
    case INV_DEYE_REG_NOMINAL_FLOAT_VOLTAGE:
      return data->nominal_float_voltage;
    case INV_DEYE_REG_DESIGN_CAPACITY:
      return data->design_capacity;
    case 0x100F:
    case 0x1013:
    case 0x1016:
      return 0;  // reserved
    default:
      return 0xFFFF;
  }
}

static void inv_send_error(uint8_t addr, uint8_t cmd, uint8_t error) {
  uint8_t resp[5];
  resp[0] = addr;
  resp[1] = cmd + 0x80;
  resp[2] = error;
  uint16_t crc = crc16(resp, 3);
  resp[3] = crc & 0xFF;
  resp[4] = crc >> 8;
  mgos_uart_write(inv_uart_no, resp, 5);
  mgos_uart_flush(inv_uart_no);
}

static void inv_send_read_response(uint8_t addr, uint16_t read_from,
                                   uint16_t read_len) {
  uint8_t resp[0x100];
  struct inv_deye_data data;
  if (!bms_get_data(addr, &data)) {
    // inv_send_error(addr, INV_DEYE_CMD_READ, INV_DEYE_ERROR_ADDRESS);
    return;
  }
  if ((mg_time() - data.last_update) > 10 * BMS_MAX_COUNT) {
    // inv_send_error(addr, INV_DEYE_CMD_INFO, INV_DEYE_ERROR_ADDRESS);
    return;
  }
  uint8_t len = read_len * 2;
  resp[0] = addr;
  resp[1] = INV_DEYE_CMD_READ;
  resp[2] = len;
  for (int i = 0; i < read_len; i++) {
    uint16_t value = inv_reg_read(addr, &data, read_from + i);
    resp[3 + i * 2] = value >> 8;
    resp[4 + i * 2] = value & 0xFF;
  }
  uint16_t crc = crc16(resp, 3 + len);
  resp[3 + len] = crc & 0xFF;
  resp[4 + len] = crc >> 8;
  mgos_uart_write(inv_uart_no, resp, 5 + len);
  mgos_uart_flush(inv_uart_no);
  char str[0x80 * 2] = {
      0,
  };
  // dump data to log
  LOG(LL_INFO, ("INV << BMS %d: SOC: %d, V: %d, I: %d, C: %d / %d, W: %04X, "
                "P: %04X, F: %04X",
                addr, data.SOC, data.pack_voltage, data.pack_current,
                data.remain_capacity, data.design_capacity, data.warning,
                data.protection, data.fault_status));
  bin_to_hex(str, (unsigned char *) resp, 5 + len);
  LOG(LL_DEBUG, ("INV << %s", str));
}

static void inv_send_info_response(uint8_t addr) {
  uint16_t mlen;
  uint8_t resp[0x100];
  char serial[INV_DEYE_SERIAL_LEN + 1] = {'0'};
  struct inv_deye_data data;
  // if (!bms_get_connected(addr))
  if (!bms_get_data(addr, &data)) {
    // inv_send_error(addr, INV_DEYE_CMD_INFO, INV_DEYE_ERROR_ADDRESS);
    return;
  }
  if ((mg_time() - data.last_update) > 10 * BMS_MAX_COUNT) {
    // inv_send_error(addr, INV_DEYE_CMD_INFO, INV_DEYE_ERROR_ADDRESS);
    return;
  }

  resp[0] = addr;
  resp[1] = INV_DEYE_CMD_INFO;

  mlen = strlen(data.model);
  if (mlen > INV_DEYE_MODEL_LEN) mlen = INV_DEYE_MODEL_LEN;
  strncpy((char *) resp + 3, data.model, mlen);
  mlen += 3;
  resp[mlen++] = '*';
  resp[mlen++] = 0x10;  // SW
  resp[mlen++] = 0x11;  // SW
  resp[mlen++] = '*';
  resp[mlen++] = 0x20;  // HW
  resp[mlen++] = 0x21;  // HW
  resp[mlen++] = '*';
  struct mgos_config_bms *cfg = get_bms_config(addr);
  if (cfg == NULL) {
    return;
  }
  strncpy(serial, data.serial_no, INV_DEYE_SERIAL_LEN);
  strncpy((char *) resp + mlen, serial, INV_DEYE_SERIAL_LEN);
  mlen += INV_DEYE_SERIAL_LEN;
  resp[mlen++] = '*';
  resp[2] = mlen - 3;
  uint16_t crc = crc16(resp, mlen);
  resp[mlen++] = crc & 0xFF;
  resp[mlen++] = crc >> 8;
  mgos_uart_write(inv_uart_no, resp, mlen);
  mgos_uart_flush(inv_uart_no);
  char str[0x40 * 2] = {
      0,
  };
  bin_to_hex(str, (unsigned char *) resp, mlen);
  LOG(LL_DEBUG, ("INV << %s", str));
}

static void uart_timer_cb(void *arg) {
  inv_buf_len = 0;
  (void) arg;
}

static void inv_uart_dispatcher(int uart_no, void *arg) {
  assert(uart_no == inv_uart_no);
  uint16_t crc, calc_crc;
  size_t len = mgos_uart_read_avail(uart_no);
  if (len == 0) return;
  size_t room = UART_MAX_BUF - inv_buf_len;
  if (len > room) len = room;
  len = mgos_uart_read(uart_no, inv_buf + inv_buf_len, len);
  // log data in hex
  char str[0x80 * 2] = {
      0,
  };
  bin_to_hex(str, (unsigned char *) inv_buf + inv_buf_len, len);
  LOG(LL_DEBUG, ("INV >%02X> %s", len, str));
  inv_buf_len += len;
  // reset packet on timeout
  if (uart_timer_id != -1) mgos_clear_timer(uart_timer_id);
  uart_timer_id = mgos_set_timer(200, true, uart_timer_cb, NULL);
  // validating the packet
  uint8_t addr = inv_buf[0];
  // got full packet?
  if (inv_buf_len < 4) return;
  // compare size
  uint8_t cmd = inv_buf[1];
  uint16_t cmd_size = 0;
  switch (cmd) {
    case INV_DEYE_CMD_READ:
      if (inv_buf_len < 8) return;
      bin_to_hex(str, (unsigned char *) inv_buf, inv_buf_len);
      LOG(LL_DEBUG, ("INV >> %s", str));
      cmd_size = 8;
      crc = inv_buf[6] | (inv_buf[7] << 8);
      calc_crc = crc16(inv_buf, cmd_size - 2);
      if (crc != calc_crc) {
        LOG(LL_ERROR,
            ("INV: invalid CRC, expecting: %04X, got: %04X", calc_crc, crc));
        goto reset;
      }
      uint16_t read_from = (inv_buf[2] << 8) | inv_buf[3];
      uint16_t read_len = (inv_buf[4] << 8) | inv_buf[5];
      LOG(LL_DEBUG,
          ("INV >> BMS %d: read %04X++%04X", addr, read_from, read_len));
      inv_send_read_response(addr, read_from, read_len);
      break;
    case INV_DEYE_CMD_INFO:
      if (inv_buf_len < 4) return;
      bin_to_hex(str, (unsigned char *) inv_buf, inv_buf_len);
      LOG(LL_DEBUG, ("INV >> %s", str));
      cmd_size = 4;
      crc = inv_buf[2] | (inv_buf[3] << 8);
      calc_crc = crc16(inv_buf, cmd_size - 2);
      if (crc != calc_crc) {
        LOG(LL_ERROR,
            ("INV: invalid CRC, expecting: %04X, got: %04X", calc_crc, crc));
        goto reset;
      }
      LOG(LL_DEBUG, ("INV >> BMS %d: info", addr));
      inv_send_info_response(addr);
      break;
#ifdef SELF_TEST      
    case 0x00:
      LOG(LL_INFO, ("INV >> got read BMS packet, self-test ACK"));
      unsigned char resp[11] = {0xBE, 0xC0, 0x01, 0x11, 0x22, 0x33,
                                0x44, 0x55, 0x66, 0x77, 0x88};
      mgos_uart_write(inv_uart_no, resp, sizeof(resp));
      mgos_uart_flush(inv_uart_no);
      break;
#endif
    default:
      // LOG(LL_ERROR, ("INV: invalid command %d: %02X", addr, cmd));
      // inv_send_error(addr, cmd, INV_DEYE_ERROR_FUNCTION);
      break;
  }
reset:
  // remove the packet
  inv_buf_len = 0;
  if (uart_timer_id != -1) mgos_clear_timer(uart_timer_id);
}

static void init_uart_inv() {
  struct mgos_uart_config inv_ucfg;
  inv_uart_no = mgos_sys_config_get_app_inv_uart_no();

  mgos_uart_config_set_defaults(inv_uart_no, &inv_ucfg);
  inv_ucfg.baud_rate = 9600;
  inv_ucfg.num_data_bits = 8;
  inv_ucfg.parity = MGOS_UART_PARITY_NONE;
  inv_ucfg.stop_bits = MGOS_UART_STOP_BITS_1;
  inv_ucfg.rx_buf_size = 256;
  inv_ucfg.tx_buf_size = 256;
  if (!mgos_uart_configure(inv_uart_no, &inv_ucfg)) goto err;
  mgos_uart_set_dispatcher(inv_uart_no, inv_uart_dispatcher, (void *) NULL);
  mgos_uart_set_rx_enabled(inv_uart_no, true);
  LOG(LL_INFO,
      ("Inverter UART%d initialized %u,%d%c%d", inv_uart_no, inv_ucfg.baud_rate,
       inv_ucfg.num_data_bits,
       inv_ucfg.parity == MGOS_UART_PARITY_NONE ? 'N' : inv_ucfg.parity + '0',
       inv_ucfg.stop_bits));
  return;
err:
  LOG(LL_ERROR, ("Failed to configure UART%d", inv_uart_no));
}

void start_inv(void) {
  init_uart_inv();
}