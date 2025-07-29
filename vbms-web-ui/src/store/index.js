import Vue from "vue";
import Vuex from "vuex";
import * as conf from "@/config";

Vue.use(Vuex);

export default new Vuex.Store({
  state: {
    bms: [],
    config_loaded: false,
    package_version: conf.PACKAGE_VERSION,
    info: {},
    ble: [],
  },
  getters: {
    bms: (state) => {
      return state.bms;
    },
    config_loaded: (state) => {
      return state.config_loaded;
    },
    package_version: (state) => {
      return state.package_version;
    },
    info: (state) => {
      return state.info;
    },
    ble: (state) => {
      return state.ble;
    },
  },
  mutations: {
    SET_DATA(state, resp) {
      //resp is array of bms data
      if (
        !state.config_loaded ||
        resp === undefined ||
        !Array.isArray(resp.bms)
      )
        return;
      state.bms.forEach((bms) => {
        bms.data = null;
      });
      resp.bms.forEach((data) => {
        if (state.bms[data.addr] !== undefined) {
          state.bms[data.addr].data = data;
        }
      });
    },
    SET_CONFIG(state, resp) {
      state.bms.length = 0;
      for (let i = 0; i < conf.MAX_BMS_COUNT; i++) {
        let bms_config = resp[`bms${i}`];
        if (bms_config == undefined) bms_config = null;
        else {
          bms_config.voltage = parseFloat(bms_config.voltage).toFixed(1);
        }
        state.bms.push({ id: i, config: bms_config, data: null });
      }
      state.config_loaded = true;
      console.log("SET_CONFIG", state.bms);
    },
    SET_INFO(state, resp) {
      state.info = {};
      if (!resp) return;
      Object.keys(resp).forEach((key) => {
        let v = resp[key];
        if (typeof v === "object") v = JSON.stringify(v);
        state.info[key] = v;
      });

      console.log("SET_INFO", state.info);
    },
    SET_BLE(state, resp) {
      state.ble = resp.ble.map((b, index) => {
        return { id: index, addr: b.addr, name: b.name };
      });
      console.log("SET_BLE", state.ble);
    },
  },
  actions: {
    get_config({ commit }) {
      return new Promise((resolve, reject) => {
        Vue.http
          .get(conf.url + "/rpc/Config.Get")
          .then(
            (response) => {
              if (response.ok) {
                console.log(response.data);
                // preparing bms data
                commit("SET_CONFIG", response.data);
                resolve(response.data);
              } else {
                reject(response);
              }
            },
            (error) => {
              reject(error);
            }
          )
          .catch((error) => {
            console.error(error);
            reject(error);
          });
      });
    },
    set_config({ commit }, data) {
      return new Promise((resolve, reject) => {
        Vue.http
          .post(conf.url + "/rpc/Config.Set", data)
          .then(
            (response) => {
              if (response.ok) {
                resolve(response.data);
              } else {
                reject(response);
              }
            },
            (error) => {
              reject(error);
            }
          )
          .catch((error) => {
            console.error(error);
            reject(error);
          });
      }
      );
    },
    get_summary({ commit }) {
      return new Promise((resolve, reject) => {
        Vue.http
          .get(conf.url + "/rpc/VBMS.GetSummary")
          .then(
            (response) => {
              if (response.ok) {
                console.log(response.data);
                // preparing bms data
                commit("SET_DATA", response.data);
                resolve(response.data);
              } else {
                reject(response);
              }
            },
            (error) => {
              reject(error);
            }
          )
          .catch((error) => {
            console.error(error);
            reject(error);
          });
      });
    },
    get_info({ commit }) {
      return new Promise((resolve, reject) => {
        commit("SET_INFO", null);
        Vue.http
          .get(conf.url + "/rpc/Sys.GetInfo")
          .then(
            (response) => {
              if (response.ok) {
                console.log(response.data);
                // preparing bms data
                commit("SET_INFO", response.data);
                resolve(response.data);
              } else {
                reject(response);
              }
            },
            (error) => {
              reject(error);
            }
          )
          .catch((error) => {
            console.error(error);
            reject(error);
          });
      });
    },
    get_ble({ commit }) {
      return new Promise((resolve, reject) => {
        commit("SET_BLE", { ble: [] });
        Vue.http
          .get(conf.url + "/rpc/VBMS.ScanBLE")
          .then(
            (response) => {
              if (response.ok) {
                commit("SET_BLE", response.data);
                resolve(response);
              } else {
                reject(response);
              }
            },
            (error) => {
              // const data = {
              //   ble: [
              //     { addr: "00:00:00:00:00:00", name: "BLE1" },
              //     { addr: "00:00:00:00:00:01", name: "BLE2" },
              //   ],
              // };
              // setTimeout(
              //   () => {
              //     commit("SET_BLE", data);
              //     resolve(data);
              //   }, 3000);
              reject(error);
            }
          )
          .catch((error) => {
            console.error(error);
            reject(error);
          });
      });
    },
  },
  modules: {},
});
