#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BMS_MAX_COUNT 15  // limit by DEYE inverter protocol addr: 00..0E
#define BMS_LAST BMS_MAX_COUNT - 1
#define BMS_MAX_CELL_COUNT 20
#define BLE_ADDR_LEN 6 * 2 + 5 + 1

#pragma pack(push, 1)

// enum BMS types
enum bms_type {
  BMS_TYPE_NONE = -1,
  BMS_TYPE_UNKNOWN = 0,
  BMS_TYPE_BYD_PRO_RS485,
  BMS_TYPE_JK_BT,
  BMS_TYPE_JK_RS485,
} bms_type_t;

#define UART_MAX_BUF 0x100

#define BMS_BYD_PROTOVOL_VER 0x01
#define BMS_BYD_MAX_DATA_SIZE 0x70
#define BMS_BYD_CMD_READ 0x03
#define BMS_BYD_EOF_55 0x55
#define BMS_BYD_EOF_AA 0xAA

// BYD BMS packet
struct bms_byd_packet {
  uint8_t addr;
  uint16_t size;
  uint8_t data[BMS_BYD_MAX_DATA_SIZE];
};

struct bms_byd_data {
  int32_t pack_voltage;
  int32_t battery_voltage;
  int16_t cell_volt[16];
  int32_t current;
  int16_t cell_temp[8];
  uint32_t others;
  uint32_t alarm_state;
  uint32_t protect_state;
  uint32_t fail_state;
  uint32_t balance_state;
  uint16_t battery_state;
  uint16_t SOC;
  uint32_t SOH;
  uint32_t discharge_times;
  uint32_t total_discharge_soc;
};

enum bms_byd_others {
  BMS_BYD_OTHER_HEATER = 0x1,
  BMS_BYD_OTHER_FULL_COUNT = 0x2,
  BMS_BYD_OTHER_VOLT_CAL = 0x4,
  BMS_BYD_OTHER_CURR_CAL = 0x8,
  BMS_BYD_OTHER_CHARGE_CURR = 0x10,
};

#define BMS_MANUFACTURER_BYD "BYD PRO 2.5"
#define BMS_MANUFACTURER_JK "JK-BMS"
#define BMS_MANUFACTURER_DEYE "DEYE"

enum bms_byd_state {
  BMS_BYD_STATE_PACK_VOLT_HI = 0x1,
  BMS_BYD_STATE_MODULE_VOLT_HI = 0x2,
  BMS_BYD_STATE_CELL_VOLT_HI = 0x4,
  BMS_BYD_STATE_PACK_VOLT_LO = 0x8,
  BMS_BYD_STATE_MODULE_VOLT_LO = 0x10,
  BMS_BYD_STATE_CELL_VOLT_LO = 0x20,
  BMS_BYD_STATE_CHARGE_CUR_HI = 0x40,
  BMS_BYD_STATE_DISCHARGE_CUR_HI = 0x80,
  BMS_BYD_STATE_CHARGE_TEMP_HI = 0x100,
  BMS_BYD_STATE_DISCHARGE_TEMP_HI = 0x200,
  BMS_BYD_STATE_CHARGE_TEMP_LO = 0x400,
  BMS_BYD_STATE_DISCHARGE_TEMP_LO = 0x800,
  BMS_BYD_STATE_SOC_LO = 0x1000,
  BMS_BYD_STATE_DISCHARGE_SHOR_CIRCUIT = 0x4000,
  BMS_BYD_STATE_REVERSE_CONNECTION = 0x8000,
  BMS_BYD_STATE_CYCLES_HI = 0x10000,
  BMS_BYD_STATE_LEW_TEMP_OVER_CHARGE_2 = 0x20000,
  BMS_BYD_STATE_LEW_TEMP_OVER_CHARGE_3 = 0x40000,
  BMS_BYD_STATE_CHARGE_SHOR_CIRCUIT = 0x80000,
};

enum bms_byd_fail {
  BMS_BYD_FAIL_VOLT_SENSOR_FAIL = 0x1,
  BMS_BYD_FAIL_TEMP_SENSOR_FAIL = 0x2,
  BMS_BYD_FAIL_CHARGE_TUBE_FAIL = 0x4,
  BMS_BYD_FAIL_DISCHARGE_TUBE_FAIL = 0x8,
  BMS_BYD_FAIL_CELL_BROKEN = 0x10,
  BMS_BYD_FAIL_HEATER_FAIL = 0x20,
  BMS_BYD_FAIL_SWITCH_OF_PANEL_OFF = 0x80,
  BMS_BYD_FAIL_CHARGE_CIRCUIT_FAIL = 0x100,
};

enum bms_byd_battery_state {
  BMS_BYD_BAT_STATE_CHARGE = 0x1,
  BMS_BYD_BAT_STATE_DISCHARGE = 0x2,
  BMS_BYD_BAT_STATE_FULL = 0x4,
  BMS_BYD_BAT_STATE_IDLE = 0x8,
};

// inverter
enum inverter_type {
  INVERTER_TYPE_NONE = -1,
  INVERTER_TYPE_UNKNOWN = 0,
  INVERTER_TYPE_DEYE_RS485,
  INVERTER_TYPE_DEYE_CAN,
} inverter_type_t;

enum inv_deye_cmd {
  INV_DEYE_CMD_READ = 0x04,
  INV_DEYE_CMD_INFO = 0x11,
};

enum inv_deye_error {
  INV_DEYE_ERROR_NONE = 0,
  INV_DEYE_ERROR_FUNCTION = 1,
  INV_DEYE_ERROR_ADDRESS = 2,
  INV_DEYE_ERROR_DATA = 3,
};

#define INV_DEYE_REG_BASE 0x1000
enum inv_deye_regs {
  INV_DEYE_REG_PACK_VOLTAGE = 0x1000,
  INV_DEYE_REG_PACK_CURRENT = 0x1001,
  INV_DEYE_REG_REMAINING_CAPACITY = 0x1002,
  INV_DEYE_REG_CELL_TEMP_AVG = 0x1003,
  INV_DEYE_REG_ENV_TEMP = 0x1004,
  INV_DEYE_REG_WARNING = 0x1005,
  INV_DEYE_REG_PROTECTION = 0x1006,
  INV_DEYE_REG_FAULT_STATUS = 0x1007,
  INV_DEYE_REG_SOC = 0x1008,
  INV_DEYE_REG_SOH = 0x1009,
  INV_DEYE_REG_FULL_CHARGE_CAPACITY = 0x100A,
  INV_DEYE_REG_CYCLE_COUNT = 0x100B,
  INV_DEYE_REG_MAX_CHARGE_CURRENT = 0x100C,
  INV_DEYE_REG_MAX_CELL_VOLTAGE = 0x100D,
  INV_DEYE_REG_MIN_CELL_VOLTAGE = 0x100E,
  INV_DEYE_REG_MAX_CELL_TEMP = 0x1010,
  INV_DEYE_REG_MIN_CELL_TEMP = 0x1011,
  INV_DEYE_REG_MOSFET_TEMP = 0x1012,
  INV_DEYE_REG_NOMINAL_FLOAT_VOLTAGE = 0x1014,
  INV_DEYE_REG_DESIGN_CAPACITY = 0x1015,
};
enum inv_deye_warnings {
  INV_DEYE_WARNING_CELL_VOLT_HI = 0x01,
  INV_DEYE_WARNING_CELL_VOLT_LO = 0x02,
  INV_DEYE_WARNING_PACK_VOLT_HI = 0x04,
  INV_DEYE_WARNING_PACK_VOLT_LO = 0x08,
  INV_DEYE_WARNING_CHARGE_CUR_HI = 0x10,
  INV_DEYE_WARNING_DISCHARGE_CUR_HI = 0x20,
  INV_DEYE_WARNING_BATT_TEMP_HI = 0x40,
  INV_DEYE_WARNING_BATT_TEMP_LO = 0x80,
  INV_DEYE_WARNING_ENV_TEMP_HI = 0x100,
  INV_DEYE_WARNING_ENV_TEMP_LO = 0x200,
  INV_DEYE_WARNING_MOSFET_TEMP_HI = 0x400,
  INV_DEYE_WARNING_CAPACITY_LO = 0x800,
};
enum inv_deye_protections {
  INV_DEYE_PROTECTION_CELL_VOLT_HI = 0x01,
  INV_DEYE_PROTECTION_CELL_VOLT_LO = 0x02,
  INV_DEYE_PROTECTION_PACK_VOLT_HI = 0x04,
  INV_DEYE_PROTECTION_PACK_VOLT_LO = 0x08,
  INV_DEYE_PROTECTION_SHORT_CIRCUIT = 0x10,
  INV_DEYE_PROTECTION_CUR_HI = 0x20,
  INV_DEYE_PROTECTION_CHARGE_TEMP_HI = 0x40,
  INV_DEYE_PROTECTION_CHARGE_TEMP_LO = 0x80,
  INV_DEYE_PROTECTION_DISCHARGE_TEMP_HI = 0x100,
  INV_DEYE_PROTECTION_DISCHARGE_TEMP_LO = 0x200,
};

enum inv_deye_faults {
  INV_DEYE_FAULT_COMMUNICATION = 0x01,
  INV_DEYE_FAULT_TEMP_SENSOR_FAIL = 0x02,
  // INV_DEYE_FAULT_PACK_HI_VOLTAGE = 0x04,
  // INV_DEYE_FAULT_PACK_LO_VOLTAGE = 0x08,
  INV_DEYE_STATUS_CHARGE = 0x100,
  INV_DEYE_STATUS_DISCHARGE = 0x200,
  INV_DEYE_STATUS_CHARGE_MOSFET = 0x400,
  INV_DEYE_STATUS_DISCHARGE_MOSFET = 0x800,
  INV_DEYE_STATUS_CHARGE_CURRENT_LIMIT = 0x1000,
};

enum inv_deye_flags {
  INV_DEYE_GOT_SETTINGS = 0x01,
  INV_DEYE_GOT_CELL_INFO = 0x02,
  INV_DEYE_GOT_DEVICE_INFO = 0x04,
  INV_DEYE_GOT_ALL = 0x07,
};

#define INV_DEYE_SERIAL_LEN 20
#define INV_DEYE_MODEL_LEN 15

struct inv_deye_data {
  uint8_t addr;  // index
  char serial_no[INV_DEYE_SERIAL_LEN + 1];
  char model[INV_DEYE_MODEL_LEN + 1];
  enum inv_deye_flags flags;
  double last_update;
  uint16_t pack_voltage;
  int16_t pack_current;
  uint16_t remain_capacity;
  int16_t cell_temp_avg;
  int16_t env_temp;
  uint16_t warning;
  uint16_t protection;
  uint16_t fault_status;
  uint16_t SOC;
  uint16_t SOH;
  uint16_t full_charge_capacity;
  uint16_t cycle_count;
  uint16_t max_charge_current;
  uint16_t max_cell_voltage;
  uint16_t min_cell_voltage;
  //   uint16_t reserved_100F;
  int16_t max_cell_temp;
  int16_t min_cell_temp;
  int16_t mosfet_temp;
  //   uint16_t reserved_1013;
  uint16_t nominal_float_voltage;
  uint16_t design_capacity;
  //   uint16_t reserved_1018;
};

enum bms_jk_protocol {
  BMS_JK_PROTOCOL_VERSION_JK04,
  BMS_JK_PROTOCOL_VERSION_JK02_24S,
  BMS_JK_PROTOCOL_VERSION_JK02_32S,
};

enum bms_jk_frame_type {
  BMS_JK_FRAME_TYPE_SETTINGS = 0x01,
  BMS_JK_FRAME_TYPE_CELL_INFO = 0x02,
  BMS_JK_FRAME_TYPE_DEVICE_INFO = 0x03,
  BMS_JK_FRAME_TYPE_DETAIL_LOG = 0x06,
  BMS_JK_FRAME_TYPE_C8 = 0xC8,
};

#define BMS_JK_MAX_CELL_COUNT 32

//! BIG ENDIAN
// "Charge Overtemperature",               // 0000 0000 0000 0001
// "Charge Undertemperature",              // 0000 0000 0000 0010
// "Coprocessor communication error",      // 0000 0000 0000 0100
// "Cell Undervoltage",                    // 0000 0000 0000 1000
// "Battery pack undervoltage",            // 0000 0000 0001 0000
// "Discharge overcurrent",                // 0000 0000 0010 0000
// "Discharge short circuit",              // 0000 0000 0100 0000
// "Discharge overtemperature",            // 0000 0000 1000 0000
// "Wire resistance",                      // 0000 0001 0000 0000
// "Mosfet overtemperature",               // 0000 0010 0000 0000
// "Cell count is not equal to settings",  // 0000 0100 0000 0000
// "Current sensor anomaly",               // 0000 1000 0000 0000
// "Cell Overvoltage",                     // 0001 0000 0000 0000
// "Battery pack overvoltage",             // 0010 0000 0000 0000
// "Charge overcurrent protection",        // 0100 0000 0000 0000
// "Charge short circuit",                 // 1000 0000 0000 0000

enum bms_jk_errors {
  BMS_JK_ERROR_WIRE_RESISTANCE = 0x0001,         //
  BMS_JK_ERROR_MOSFET_TEMP_HI = 0x0002,          //
  BMS_JK_ERROR_CELL_COUNT_MISMATCH = 0x0004,     //
  BMS_JK_ERROR_CURRENT_SENSOR_ANOMALY = 0x0008,  //
  BMS_JK_ERROR_CELL_VOLT_HI = 0x0010,            //
  BMS_JK_ERROR_PACK_VOLT_HI = 0x0020,            //
  BMS_JK_ERROR_CHARGE_CUR_HI = 0x0040,           //
  BMS_JK_ERROR_CHARGE_SHORT_CIRCUIT = 0x0080,
  BMS_JK_ERROR_CHARGE_TEMP_HI = 0x0100,    //
  BMS_JK_ERROR_CHARGE_TEMP_LO = 0x0200,    //
  BMS_JK_ERROR_COPROCESSOR_COMM = 0x0400,  //
  BMS_JK_ERROR_CELL_VOLT_LO = 0x0800,      //
  BMS_JK_ERROR_PACK_VOLT_LO = 0x1000,      //
  BMS_JK_ERROR_DISCHARGE_CUR_HI = 0x2000,  //
  BMS_JK_ERROR_DISCHARGE_SHORT_CIRCUIT = 0x4000,
  BMS_JK_ERROR_DISCHARGE_TEMP_HI = 0x8000,  //
};

struct bms_jk_device_info {
  uint32_t magic;  // 0x55AAEB90
  uint8_t type;
  uint8_t counter;
  char vendor_id[16];
  char hw_ver[8];
  char sw_ver[8];
  uint32_t uptime;
  uint32_t power_on_count;
  char name[16];
  char device_passcode[16];
  char mfg_date[8];
  char serial_no[12];
  char passcode[4];
  char user_data[16];
  char setup_passcode[16];
  char reset_passcode[16];
};

struct bms_jk_cell_info {
  uint32_t magic;  // 0x55AAEB90
  uint8_t type;
  uint8_t counter;
  int16_t cell_volt[BMS_JK_MAX_CELL_COUNT];
  uint32_t cell_enable_bitmask;
  int16_t avg_cell_voltage;
  int16_t delta_cell_voltage;
  uint8_t max_voltage_cell;
  uint8_t min_voltage_cell;
  uint16_t cell_res[BMS_JK_MAX_CELL_COUNT];
  int16_t mosfet_temp;
  uint32_t cell_res_warning_bitmask;
  int32_t pack_voltage;
  int32_t pack_power;
  int32_t pack_current;
  int16_t temp1;
  int16_t temp2;
  uint16_t errors;
  uint16_t errors2;
  int16_t balance_current;
  uint8_t balancing_state;  // 0 - off, 1 - charging, 2 - discharging
  uint8_t SOC;
  uint32_t remain_capacity;       // 0.001 Ah
  uint32_t full_charge_capacity;  // 0.001 Ah
  uint32_t cycle_count;           // 0.001 Ah
  uint32_t cycle_capacity;        // 0.001 Ah
  uint8_t SOH;
  uint8_t precharge;
  uint16_t user_alarm;
  uint32_t uptime_sec;
  uint8_t charging_on;
  uint8_t discharging_on;
  uint8_t precharging_on;
  uint8_t balancing_on;
  uint16_t release_timers[6];
  uint16_t temp_sensors_bitmask;  // 01 -mosfet, 02 - temp sensor1, 04 -
                                  // temp sensor2...
  uint8_t dummy;
  uint16_t heating_on;
  // uint16_t dummy0000;
  // uint16_t time_emergency_sec;
  // uint16_t discharge_cur_correction_factor;
  // uint16_t charge_volt;
  // uint16_t discharge_volt;
  // uint16_t battery_volt_correction_factor;  // V
  // uint16_t battery_volt_corrected;          // V
  // uint32_t heating_current;                 // 0.001 A
  // uint8_t charger_pulled_on;
  // uint16_t temp3;
  // uint16_t temp4;
  // uint16_t temp5;
  // uint32_t time_enter_sleep;
  // uint8_t plc_state;
};

struct bms_jk_settings {
  uint32_t magic;  // 0x55AAEB90
  uint8_t type;
  uint8_t counter;
  int32_t cell_volt_smart_sleep;
  int32_t cell_volt_lo_protection;
  int32_t cell_volt_lo_recovery;
  int32_t cell_volt_hi_protection;
  int32_t cell_volt_hi_recovery;
  int32_t cell_balance_trigger_volt;
  int32_t cell_volt_soc_100;
  int32_t cell_volt_soc_0;
  int32_t cell_req_charge_volt;
  int32_t cell_req_float_volt;
  int32_t cell_volt_power_off;
  int32_t max_charge_current;
  uint32_t charge_ocp_delay;          // over current protection
  uint32_t charge_ocp_recovery_time;  // recovery time
  int32_t max_discharge_current;
  uint32_t discharge_ocp_delay;          // over current protection
  uint32_t discharge_ocp_recovery_time;  // recovery time
  uint32_t scp_recovery_time;            // short circuit protection
  int32_t max_balance_current;
  int32_t charge_otp;           // over temperature protection
  int32_t charge_otp_recovery;  // recovery value
  int32_t discharge_otp;
  int32_t discharge_otp_recovery;
  int32_t charge_utp;           // under temperature protection
  int32_t charge_utp_recovery;  // recovery value
  int32_t mosfet_otp;           // over temperature protection
  int32_t mosfet_otp_recovery;  // recovery value
  uint32_t cell_count;          // 16, 24, 32
  uint32_t charge_on;           // 0 - off, 1 - on
  uint32_t discharge_on;        // 0 - off, 1 - on
  uint32_t balance_on;          // 0 - off, 1 - on
  uint32_t design_capacity;     // 0.001 Ah
  uint32_t scp_delay;           // short circuit protection
  int32_t cell_volt_start_balance;
  int32_t cell_volt_stop_balance;
  uint32_t cell_wire_res[BMS_JK_MAX_CELL_COUNT];
  uint32_t device_addr;
  uint32_t precharge_time;
  int32_t undef_278;  // 750 000
};

struct bms_jk_log_item {
  uint32_t d0;
  uint32_t d1;
  uint32_t d2;
  uint32_t d3;
  uint32_t d4;
  uint32_t d5;
};

struct bms_jk_log {
  uint32_t magic;  // 0x55AAEB90
  uint8_t type;    // 06
  uint8_t counter;
  uint16_t offset;
  uint8_t size;  // offset + size <= 1000
  struct bms_jk_log_item items[0];
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif