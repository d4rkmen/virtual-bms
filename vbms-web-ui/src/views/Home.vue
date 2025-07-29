<template>
  <div class="">
    <section class="section">
      <h1 class="title is-spaced">
        <span class="icon-text">
          <span class="icon">
            <i class="icon-home has-text-primary"></i>
          </span>
          <span>{{ $t("Menu_Home") }}</span>
        </span>
      </h1>
      <h2 class="subtitle">{{ $t("Menu_Home_Sub") }}</h2>
      <div class="columns has-background-dark">
        <div class="column is-three-fifths">
          <p class="is-size-6">{{ message }}</p>
        </div>
        <div class="column">
          <div class="block has-text-right">
            <b-radio v-model="refresh_sec" name="name" :native-value="0">
              {{ $t("Refresh_Off") }}
            </b-radio>
            <b-radio v-model="refresh_sec" name="name" :native-value="1">
              {{ $t("Refresh_NSec", { sec: 1 }) }}
            </b-radio>
            <b-radio v-model="refresh_sec" name="name" :native-value="5">
              {{ $t("Refresh_NSec", { sec: 5 }) }}
            </b-radio>
            <b-radio v-model="refresh_sec" name="name" :native-value="10">
              {{ $t("Refresh_NSec", { sec: 10 }) }}
            </b-radio>
          </div>
        </div>
      </div>
      <div class="block has-text-right">
        <p class="is-size-6 has-text-primary" v-if="refresh_sec === 0">
          {{ $t("Refresh", { sec: $t("Refresh_Off") }) }}
        </p>
        <p class="is-size-6 has-text-primary" v-else>
          {{ $t("Refresh", { sec: $t("Refresh_NSec", { sec: refresh_sec }) }) }}
        </p>
      </div>
      <div class="table-container">
        <template v-if="isMobile">
          <table class="table is-fullwidth iws-narrow is-striped is-hoverable">
            <thead>
              <tr>
                <th>
                  <abbr :title="$t('Col_NameT')">{{ $t("Col_Name") }}</abbr>
                </th>
                <th class="has-text-centered">
                  <abbr :title="$t('Col_SOCT')">{{ $t("Col_SOC") }}</abbr>
                </th>
                <th class="has-text-centered">
                  <abbr :title="$t('Col_VoltT')">{{ $t("Col_Volt") }}</abbr>
                </th>
                <th class="has-text-centered">
                  <abbr :title="$t('Col_CurT')">{{ $t("Col_Cur") }}</abbr>
                </th>
              </tr>
            </thead>
            <tfoot>
              <tr>
                <th>
                  <abbr :title="$t('Foot_TotalT')">{{ $t("Foot_Total") }}</abbr>
                </th>
                <template
                  v-if="
                    bms === null ||
                    bms.length === 0 ||
                    bms.filter((a) => a.data === null).length === bms.length
                  "
                >
                  <th colspan="3">
                    <abbr :title="$t('Foot_NoDataT')">{{
                      $t("Foot_NoData")
                    }}</abbr>
                  </th>
                </template>
                <template v-else>
                  <th class="is-vcentered">
                    <b-progress
                      :value="soc"
                      show-value
                      format="percent"
                      type="is-success"
                      size="is-medium"
                    ></b-progress>
                  </th>
                  <th class="has-text-centered">
                    {{
                      $t("Foot_Voltage", {
                        volt: (avg_voltage / 100).toFixed(1),
                      })
                    }}
                  </th>
                  <th class="has-text-centered">
                    {{
                      $t("Foot_Current", {
                        cur: (tot_current / 100).toFixed(1),
                      })
                    }}
                  </th>
                </template>
              </tr>
            </tfoot>
            <tbody>
              <tr
                v-for="(a, index) in bms"
                :key="index"
                :class="{ 'is-selected': false }"
              >
                <template v-if="a.config.type !== -1">
                  <td>
                    <a :title="$t('Open_Details')">{{ a.config.name }}</a>
                    <div v-if="a.data !== null" class="content is-small">
                      <p
                        v-if="
                          (a.data !== null && a.data.warning !== 0) ||
                          a.data.protection !== 0 ||
                          (a.data.fault_status & 0xff) !== 0
                        "
                        class="content is-small has-text-danger"
                        :title="$t('Faults')"
                      >
                        {{ toHex(a.data.warning, 8) }} /
                        {{ toHex(a.data.protection, 8) }} /
                        {{ toHex(a.data.fault_status & 0xff, 2) }}
                      </p>
                    </div>
                  </td>
                  <template v-if="a.data !== null">
                    <td class="is-vcentered">
                      <b-progress
                        :value="a.data.SOC / 10"
                        show-value
                        format="percent"
                        type="is-success"
                        size="is-medium"
                      ></b-progress>
                    </td>
                    <td class="is-vcentered has-text-centered">
                      {{ (a.data.pack_voltage / 100).toFixed(1) }}
                    </td>
                    <td class="is-vcentered has-text-centered">
                      {{ (a.data.pack_current / 100).toFixed(1) }}
                    </td>
                  </template>
                  <template v-else>
                    <td colspan="3" class="has-text-centered">
                      {{ $t("Not_Connected") }}
                      <!-- <b-skeleton animated></b-skeleton> -->
                    </td>
                  </template>
                </template>
              </tr>
            </tbody>
          </table>
        </template>
        <template v-else>
          <table class="table is-fullwidth iws-narrow is-striped is-hoverable">
            <thead>
              <tr>
                <th class="has-text-centered">{{ $t("Col_N") }}</th>
                <th>
                  <abbr :title="$t('Col_NameT')">{{ $t("Col_Name") }}</abbr>
                </th>
                <th class="has-text-centered">
                  <abbr :title="$t('Col_CapacityT')">{{
                    $t("Col_Capacity")
                  }}</abbr>
                </th>
                <th class="has-text-centered">
                  <abbr :title="$t('Col_SOCT')">{{ $t("Col_SOC") }}</abbr>
                </th>
                <th class="has-text-centered">
                  <abbr :title="$t('Col_SOHT')">{{ $t("Col_SOH") }}</abbr>
                </th>
                <th class="has-text-centered">
                  <abbr :title="$t('Col_VoltT')">{{ $t("Col_Volt") }}</abbr>
                </th>
                <th class="has-text-centered">
                  <abbr :title="$t('Col_CurT')">{{ $t("Col_Cur") }}</abbr>
                </th>
                <th class="has-text-centered">
                  <abbr :title="$t('Col_TempT')">{{ $t("Col_Temp") }}</abbr>
                </th>
                <th width="45" class="has-text-centered">
                  <abbr :title="$t('Col_ChargingT')">{{
                    $t("Col_Charging")
                  }}</abbr>
                </th>
                <th width="45" class="has-text-centered">
                  <abbr :title="$t('Col_DischargingT')">{{
                    $t("Col_Discharging")
                  }}</abbr>
                </th>
              </tr>
            </thead>
            <tfoot>
              <tr>
                <th colspan="2">
                  <abbr :title="$t('Foot_TotalT')">{{ $t("Foot_Total") }}</abbr>
                </th>
                <template
                  v-if="
                    bms === null ||
                    bms.length === 0 ||
                    bms.filter((a) => a.data === null).length === bms.length
                  "
                >
                  <th colspan="7">
                    <abbr :title="$t('Foot_NoDataT')">{{
                      $t("Foot_NoData")
                    }}</abbr>
                  </th>
                </template>
                <template v-else>
                  <th class="has-text-centered">
                    {{
                      $t("Foot_Capacity", {
                        cap: (calcSum(bms, "remain_capacity") / 100).toFixed(0),
                        tot: (
                          calcSum(bms, "full_charge_capacity") / 100
                        ).toFixed(0),
                      })
                    }}
                  </th>
                  <th class="is-vcentered">
                    <b-progress
                      :value="soc"
                      show-value
                      format="percent"
                      type="is-success"
                      size="is-medium"
                    ></b-progress>
                  </th>
                  <th class="is-vcentered">
                    <b-progress
                      :value="soh"
                      show-value
                      format="percent"
                      type="is-info"
                      size="is-medium"
                    ></b-progress>
                  </th>
                  <th class="has-text-centered">
                    {{
                      $t("Foot_Voltage", {
                        volt: (avg_voltage / 100).toFixed(1),
                      })
                    }}
                  </th>
                  <th class="has-text-centered">
                    {{
                      $t("Foot_Current", {
                        cur: (tot_current / 100).toFixed(1),
                      })
                    }}
                  </th>
                </template>
              </tr>
            </tfoot>
            <tbody>
              <tr
                v-for="(a, index) in bms"
                :key="index"
                :class="{ 'is-selected': false }"
              >
                <template v-if="a.config.type !== -1">
                  <th class="is-vcentered has-text-centered">{{ index }}</th>
                  <td>
                    <a :title="$t('Open_Details')">{{ a.config.name }}</a>
                    <span
                      :title="bms_type_str(a.config.type)"
                      class="icon-text"
                    >
                      <span class="icon">
                        <i class="icon-ble" v-if="a.config.type === 2"></i>
                        <i class="icon-cable" v-else></i>
                      </span>
                    </span>
                    {{ a.config.addr }}
                    <div v-if="a.data !== null" class="content is-small">
                      {{ a.data.model }}
                      <p
                        v-if="
                          (a.data !== null && a.data.warning !== 0) ||
                          a.data.protection !== 0 ||
                          (a.data.fault_status & 0xff) !== 0
                        "
                        class="content is-small has-text-danger"
                        :title="$t('Faults')"
                      >
                        {{ toHex(a.data.warning, 8) }} /
                        {{ toHex(a.data.protection, 8) }} /
                        {{ toHex(a.data.fault_status & 0xff, 2) }}
                      </p>
                    </div>
                  </td>
                  <template v-if="a.data !== null">
                    <td class="is-vcentered has-text-centered">
                      <strong>{{
                        (a.data.remain_capacity / 100).toFixed(2)
                      }}</strong>
                      / {{ (a.data.full_charge_capacity / 100).toFixed(2) }}
                    </td>
                    <td class="is-vcentered">
                      <b-progress class="mb-0"
                        :value="a.data.SOC / 10"
                        show-value
                        format="percent"
                        type="is-success"
                        size="is-medium"
                      ></b-progress>
                    </td>
                    <td class="is-vcentered">
                      <b-progress
                        :value="a.data.SOH / 10"
                        show-value
                        format="percent"
                        type="is-info"
                        size="is-medium"
                      ></b-progress>
                    </td>
                    <td class="is-vcentered has-text-centered">
                      {{ (a.data.pack_voltage / 100).toFixed(1) }}
                    </td>
                    <td class="is-vcentered has-text-centered">
                      {{ (a.data.pack_current / 100).toFixed(1) }}
                    </td>
                    <td
                      class="is-vcentered has-text-centered"
                      :class="{
                        'has-text-danger': a.data.cell_temp_avg / 10 > 35,
                      }"
                    >
                      <b>{{ (a.data.cell_temp_avg / 10).toFixed(1) }}</b>
                    </td>
                    <!-- <td class="has-text-centered" :class="{ 'is-success': (a.data.fault_status && 256) !== 0 }">
                  {{ (a.data.fault_status && 256) !== 0 }}
                </td> -->
                    <td class="is-vcentered">
                      <b-progress
                        :value="(a.data.fault_status && 256) === 0 ? 0 : 100"
                        type="is-success"
                        size="is-medium"
                      ></b-progress>
                    </td>
                    <td class="is-vcentered">
                      <b-progress
                        :value="(a.data.fault_status && 512) === 0 ? 0 : 100"
                        type="is-info"
                        size="is-medium"
                      ></b-progress>
                    </td>
                  </template>
                  <template v-else>
                    <td colspan="2" class="has-text-centered">
                      {{ $t("Not_Connected") }}
                    </td>
                    <td colspan="8" class="has-text-centered">
                      <b-skeleton animated></b-skeleton>
                    </td>
                  </template>
                </template>
              </tr>
            </tbody>
          </table>
        </template>
      </div>
    </section>
  </div>
</template>
<script>
import VueBreakpointMixin from "vue-breakpoint-mixin";

export default {
  name: "Home",
  mixins: [VueBreakpointMixin],
  data() {
    return {
      refresh_sec: 5,
      refresh_timer: null,
      tid: null,
      message: this.$t("Message_Loading"),
    };
  },

  watch: {
    refresh_sec: function (val) {
      clearTimeout(this.refresh_timer);
      this.refresh_timer = null;
      if (val !== 0) {
        this.load_data();
      }
    },
  },

  computed: {
    bms() {
      return this.$store.getters["bms"];
    },
    soc() {
      return (
        (this.calcSum(this.$store.getters["bms"], "remain_capacity") * 100) /
        this.calcSum(this.$store.getters["bms"], "full_charge_capacity")
      ).toFixed(0);
    },
    soh() {
      return (this.calcAvg(this.$store.getters["bms"], "SOH") / 10).toFixed(0);
    },
    avg_voltage() {
      return this.calcAvg(this.$store.getters["bms"], "pack_voltage");
    },
    tot_current() {
      return this.calcSum(this.$store.getters["bms"], "pack_current");
    },
  },

  mounted() {
    console.log("Home mounted");
    this.load_config();
  },

  beforeDestroy() {
    console.log("Home destroyed");
    clearTimeout(this.tid);
    clearTimeout(this.refresh_timer);
  },

  methods: {
    toHex(d, padding) {
      var hex = Number(d).toString(16);
      padding =
        typeof padding === "undefined" || padding === null
          ? (padding = 2)
          : padding;

      while (hex.length < padding) {
        hex = "0" + hex;
      }
      return hex;
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
    calcSum(arr, key) {
      if (arr === null) return 0;
      return arr.reduce(
        (acc, item) => acc + (item.data !== null ? item.data[key] : 0),
        0
      );
    },
    calcAvg(arr, key) {
      if (arr === null) return 0;
      return (
        this.calcSum(arr, key) / arr.filter((item) => item.data !== null).length
      );
    },
    calcSOC(arr) {
      return (
        (this.calcSum(arr, "remain_capacity") * 100) /
        this.calcSum(arr, "full_charge_capacity")
      );
    },
    load_config() {
      this.$store
        .dispatch("get_config")
        .then(
          (response) => {
            this.load_data();
          },
          (error) => {
            // console.error("get_config", error);
            this.message = this.$t("Error_Config", { err: error.statusText });
            this.$buefy.toast.open({
              duration: 5000,
              message: this.$t("Error_Config", { err: error.statusText }),
              position: "is-bottom",
              type: "is-danger",
            });
            cancelTimeout(this.tid);
            this.tid = setTimeout(() => {
              this.load_config();
            }, 5000);
          }
        )
        .catch((error) => {
          console.error("get_config", error);
        });
    },
    load_data() {
      clearTimeout(this.refresh_timer);
      if (!this.$store.getters["config_loaded"]) {
        this.load_config();
        return;
      }
      this.message = this.$t("Message_Refreshing");
      this.$store
        .dispatch("get_summary")
        .then(
          (response) => {
            // show timestamp
            this.message = this.$t("Message_LasUpdated", {
              time: new Date().toLocaleTimeString(),
            });
            if (this.refresh_sec !== 0) {
              this.refresh_timer = setTimeout(() => {
                this.load_data();
              }, this.refresh_sec * 1000);
            }
          },
          (error) => {
            this.message = this.$t("Error_ReadData", { err: error.statusText });
            this.$buefy.toast.open({
              duration: 5000,
              message: this.$t("Error_ReadData", { err: error.statusText }),
              position: "is-bottom",
              type: "is-danger",
            });
            if (this.refresh_sec !== 0) {
              this.refresh_timer = setTimeout(() => {
                this.load_data();
              }, 5000);
            }
          }
        )
        .catch((error) => {
          console.error("get_summary", error);
          this.message = this.$t("Error_HttpRequest");
          this.$buefy.toast.open({
            duration: 5000,
            message: this.$t("Error_HttpRequest"),
            position: "is-bottom",
            type: "is-danger",
          });
          if (this.refresh_sec !== 0) {
            this.refresh_timer = setTimeout(() => {
              this.load_data();
            }, this.refresh_sec * 1000);
          }
        });
    },
  },
};
</script>
<style scoped></style>
