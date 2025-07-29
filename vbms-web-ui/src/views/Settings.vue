<template>
  <div>
    <section class="section">
      <h1 class="title is-spaced">
        <span class="icon-text">
          <span class="icon">
            <i class="icon-settings has-text-primary"></i>
          </span>
          <span>{{ $t("Menu_Settings") }}</span>
        </span>
      </h1>
      <h2 class="subtitle">{{ $t("Menu_Settings_Sub") }}</h2>
      <template v-if="list">
        <b-notification>
          <i class="icon-info"></i>
          <span>{{ $t("Settings_Info") }}</span>
        </b-notification>
        <div class="table-container">
          <table class="table is-narrow is-hoverable">
            <thead>
              <tr>
                <th>{{ $t("Col_N") }}</th>
                <th width="200">
                  <abbr :title="$t('Col_NameT')">{{ $t("Col_Name") }}</abbr>
                </th>
                <th width="200">
                  <abbr :title="$t('Col_TypeT')">{{ $t("Col_Type") }}</abbr>
                </th>
                <th width="200">
                  <abbr :title="$t('Col_AddrT')">{{ $t("Col_Addr") }}</abbr>
                </th>
                <th>{{ $t("Col_ChargeVolt") }}</th>
              </tr>
            </thead>
            <tfoot>
              <tr>
                <template v-if="list === null || list.length === 0">
                  <th colspan="5">
                    <abbr :title="$t('Foot_NoDataT')">{{
                      $t("Foot_NoData")
                    }}</abbr>
                  </th>
                </template>
                <template v-else>
                  <th class="is-vcentered" colspan="4">
                    {{
                      $t("Foot_Assigned", {
                        count: list.filter((a) => a.type !== -1).length,
                      })
                    }}
                  </th>
                  <th>
                    <b-button
                      :label="$t('Settings_Save')"
                      :type="
                        list_changed ? 'is-primary' : 'is-dark'
                      "
                      size="is-medium"
                      expanded
                      @click="confirmSave"
                    />
                  </th>
                </template>
              </tr>
            </tfoot>
            <tbody>
              <tr
                v-for="(a, index) in list"
                :key="index"
                :class="{
                  'is-selected': false,
                  'has-background-black-ter': a.type !== -1,
                }"
              >
                <th class="is-vcentered has-text-centered">{{ index }}</th>
                <td style="min-width: 160px">
                  <b-input v-model="a.name" expanded maxlength="30"></b-input>
                </td>
                <td style="min-width: 160px" class="has-text-centered">
                  <b-select
                    v-model="a.type"
                    icon-pack="bms"
                    :icon="icon_by_type(a.type)"
                    @input="fix_addr(a)"
                  >
                    <option :value="-1">{{ $t("Type_Unused") }}</option>
                    <optgroup label="BLE">
                      <option :value="2">JK BMS</option>
                    </optgroup>
                    <optgroup label="RS-485">
                      <option :value="1">BYD Pro 2.5</option>
                    </optgroup>
                  </b-select>
                </td>
                <td style="min-width: 180px" class="has-text-centered">
                  <template v-if="a.type === 2">
                    <b-select v-model="a.addr" icon-pack="bms" icon="icon-ble">
                      <optgroup :label="$t('BLE_OldAddr')">
                        <option :value="a.addr">{{ a.addr }}</option>
                      </optgroup>
                      <optgroup
                        v-if="ble_status !== ''"
                        :label="$t(ble_status)"
                      >
                        <option
                          v-for="option in ble"
                          :value="option.addr"
                          :key="option.id"
                        >
                          {{ option.name }} / {{ option.addr }}
                        </option>
                      </optgroup>
                    </b-select>
                    <div class="block has-text-right">
                      <b-button
                        rounded
                        :disabled="ble_scanning"
                        class="is-small"
                        @click="load_ble"
                      >
                        Scan
                      </b-button>
                    </div>
                  </template>
                  <b-numberinput
                    icon="icon-cable"
                    icon-pack="bms"
                    v-else-if="a.type === 1"
                    v-model.number="a.addr"
                    type="is-primary"
                    :controls="false"
                    :min="0"
                    :max="63"
                  ></b-numberinput>
                </td>
                <td style="min-width: 160px">
                  <b-numberinput
                    v-if="a.type !== -1"
                    icon="icon-voltage"
                    icon-pack="bms"
                    v-model.number="a.voltage"
                    type="is-primary"
                    :controls="false"
                    :min="48"
                    :max="60"
                  ></b-numberinput>
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </template>
      <template v-else>
        <b-skeleton width="20%" animated></b-skeleton>
        <b-skeleton width="40%" animated></b-skeleton>
        <b-skeleton width="30%" animated></b-skeleton>
        <b-skeleton animated></b-skeleton>
      </template>
    </section>
  </div>
</template>

<script>
import * as conf from "../config";

export default {
  data() {
    return {
      ble_status: "",
      list: null,
      tid: null,
      ble_scanning: false,
    };
  },
  computed: {
    bms() {
      return this.$store.getters["bms"];
    },
    ble() {
      return this.$store.getters["ble"];
    },
    list_changed() {
      return this.list.some((a, index) => {
        if (this.bms[index].config) {
          return (
            a.name !== this.bms[index].config.name ||
            a.type !== this.bms[index].config.type ||
            a.addr != this.bms[index].config.addr ||
            a.voltage != this.bms[index].config.voltage
          );
        } else {
          return a.type !== -1;
        }
      });
    },
  },
  methods: {
    load_ble() {
      this.ble_status = "BLE_Scanning";
      this.ble_scanning = true;
      this.$store.dispatch("get_ble").then(
        (resp) => {
          this.ble_scanning = false;
          if (resp.data.ble) {
            this.ble_status =
              resp.data.ble.length === 0 ? "BLE_ScanFail" : "BLE_ScanOK";
          } else {
            this.ble_status = "BLE_ScanError";
            this.$buefy.toast.open({
              duration: 5000,
              message: this.$t("Error_BLE", { err: resp.data.error }),
              position: "is-bottom",
              type: "is-danger",
            });
          }
        },
        (err) => {
          this.ble_scanning = false;
          this.ble_status = "BLE_ScanError";
          this.$buefy.toast.open({
            duration: 5000,
            message: this.$t("Error_BLE", { err: err.statusText }),
            position: "is-bottom",
            type: "is-danger",
          });
        }
      );
    },
    icon_by_type(type) {
      switch (type) {
        case 1:
          return "icon-cable";
        case 2:
          return "icon-ble";
        default:
          return "icon-about";
      }
    },
    bms_type_str(type) {
      switch (type) {
        case 0:
          return "Unknown";
        case 1:
          return "RS-485";
        case 2:
          return "BLE";
        case -1:
          return "N/A";
      }
    },

    load_list() {
      this.$store.dispatch("get_config").then(
        (resp) => {
          this.list = this.bms.map((bms, index) => {
            if (bms.config) {
              return {
                id: index,
                type: bms.config.type,
                addr: bms.config.addr,
                name: bms.config.name,
                voltage: bms.config.voltage,
              };
            } else {
              return {
                id: index,
                type: -1,
                addr: "",
                name: "",
                voltage: 56.4,
              };
            }
          });
        },
        (err) => {
          this.$buefy.toast.open({
            duration: 5000,
            message: this.$t("Error_Config", { err: err.statusText }),
            position: "is-bottom",
            type: "is-danger",
          });
          clearTimeout(this.tid);
          this.tid = setTimeout(() => {
            this.load_ble();
          }, 5000);
        }
      );
    },

    confirmSave() {
      // check max BLE count
      let count = this.list.filter((a) => a.type === 2).length;
      if (count > conf.MAX_BLE_COUNT) {
        this.$buefy.toast.open({
          duration: 5000,
          message: this.$t("Error_TooManyBLE", {
            count: count,
            max: conf.MAX_BLE_COUNT,
          }),
          position: "is-bottom",
          type: "is-danger",
        });
        return;
      }
      this.$buefy.dialog.confirm({
        title: this.$t("Save_Title"),
        message: this.$t("Save_Message"),
        cancelText: this.$t("Cancel"),
        confirmText: this.$t("Save"),
        type: "is-primary",
        onConfirm: () => {
          this.save();
        },
      });
    },
    save() {
      let config = {};
      this.list.forEach((a, index) => {
        config[`bms${index}`] = {
          type: a.type,
          name: a.name,
          addr: a.addr.toString(),
          voltage: parseFloat(a.voltage),
        };
      });
      let data = {
        config: config,
        save: true,
        reboot: true,
      };
      console.log(data);
      this.$store.dispatch("set_config", data).then(
        (resp) => {
          this.$buefy.toast.open({
            duration: 5000,
            message: this.$t("Save_OK"),
            position: "is-top",
            type: "is-success",
          });
          this.list = null;
          setTimeout(() => {
            this.load_list();
          }, 5000);
        },
        (err) => {
          this.$buefy.toast.open({
            duration: 5000,
            message: this.$t("Error_Save", { err: err.statusText }),
            position: "is-bottom",
            type: "is-danger",
          });
        }
      );
      // prepare json data
    },
    fix_addr(a) {
      if (a.type === 2 && a.addr == 0) {
        a.addr = "00:00:00:00:00:00";
      } else if (a.type !== 2 && a.addr === "00:00:00:00:00:00") {
        a.addr = "0";
      }
    },
  },
  mounted() {
    console.log("Settings mounted");
    this.load_list();
  },
};
</script>
