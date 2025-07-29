#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "mgos.h"

void start_bms(void);

bool bms_get_data(uint8_t bms, struct inv_deye_data *data);
bool bms_get_data_by_addr(char *addr, struct inv_deye_data *data);
int bms_get_id_by_addr(char *addr);

// bool bms_get_connected(uint8_t bms);
// bool bms_get_connected_by_addr(char *addr);
// void bms_set_connected(uint8_t bms, bool value);
// void bms_set_connected_by_addr(char *addr, bool value);
// char *bms_get_manufacturer(uint8_t bms);

struct mgos_config_bms *get_bms_config(uint8_t bms);

void bms_handle_bt_data(uint8_t id, struct mbuf *data);

#ifdef __cplusplus
}
#endif