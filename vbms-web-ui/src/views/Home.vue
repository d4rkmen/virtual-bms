<template>
  <section class="hub-dashboard">
    <div class="hub-page-heading">
      <div>
        <div class="hub-eyebrow">LIVE BATTERY MONITOR</div>
        <h1>Акумуляторна система</h1>
        <p class="hub-subtitle">Моніторинг п'яти BMS, з'єднаних послідовно · {{ message }}</p>
      </div>
      <div class="hub-refresh">
        <span class="hub-pill" :class="allOnline ? 'online' : (onlineCount ? 'warn' : 'offline')">
          {{ onlineCount }} / {{ units.length }} BMS онлайн
        </span>
        <select v-model.number="refreshSec" aria-label="Частота оновлення">
          <option :value="0">Без автооновлення</option>
          <option :value="1">Кожну секунду</option>
          <option :value="5">Кожні 5 секунд</option>
          <option :value="10">Кожні 10 секунд</option>
        </select>
        <button type="button" :disabled="loading" @click="refresh">↻ Оновити</button>
      </div>
    </div>

    <div class="hub-stats">
      <article class="hub-card">
        <div class="hub-stat-label">Напруга послідовної збірки</div>
        <strong class="hub-stat-value hub-accent">{{ numberOrDash(totalVoltage, 1) }}<small>В</small></strong>
        <div class="hub-stat-note">Сума напруг усіх підключених BMS</div>
      </article>
      <article class="hub-card">
        <div class="hub-stat-label">Струм збірки</div>
        <strong class="hub-stat-value">{{ numberOrDash(packCurrent, 1) }}<small>А</small></strong>
        <div class="hub-stat-note">Медіана вимірів · не сума струмів</div>
      </article>
      <article class="hub-card">
        <div class="hub-stat-label">Заряд збірки (SOC)</div>
        <strong class="hub-stat-value">{{ numberOrDash(packSoc, 0) }}<small>%</small></strong>
        <div class="hub-stat-note">Мінімальний SOC серед модулів</div>
        <div class="hub-progress"><span class="hub-progress-fill" :style="{width: ((packSoc === null ? 0 : packSoc) + '%')}"></span></div>
      </article>
      <article class="hub-card">
        <div class="hub-stat-label">Доступна ємність</div>
        <strong class="hub-stat-value">{{ numberOrDash(packCapacity, 1) }}<small>А·год</small></strong>
        <div class="hub-stat-note">Мінімальна залишкова ємність модуля</div>
      </article>
    </div>

    <div v-if="!allOnline && units.length" class="hub-alert">
      Немає актуальних даних від усіх BMS. Загальні показники приховано, щоб не показувати неповну збірку.
    </div>
    <div v-if="!units.length" class="hub-hint">
      BMS ще не налаштовані. Додайте їх у розділі «Налаштування».
    </div>

    <div class="hub-panel-heading">
      <h2>Окремі модулі</h2>
      <span class="hub-pill">ПОСЛІДОВНЕ З'ЄДНАННЯ · {{ units.length }} / 5</span>
    </div>
    <div class="hub-bms-grid">
      <article class="hub-card" v-for="(unit, index) in units" :key="unit.id">
        <div class="hub-bms-top">
          <div>
            <h3>{{ unit.config.name || ('BMS ' + (index + 1)) }}</h3>
            <div class="hub-bms-meta">Модуль {{ index + 1 }} · {{ unit.config.addr }} <template v-if="unit.data">· {{ unit.data.model }}</template></div>
          </div>
          <span class="hub-pill" :class="unit.data ? 'online' : 'offline'">{{ unit.data ? 'ОНЛАЙН' : 'НЕМАЄ ДАНИХ' }}</span>
        </div>
        <div class="hub-bms-values">
          <div><span>Напруга</span><strong>{{ value(unit.data, 'pack_voltage', 100, 1) }} В</strong></div>
          <div><span>Струм</span><strong>{{ value(unit.data, 'pack_current', 100, 1) }} А</strong></div>
          <div><span>SOC</span><strong>{{ value(unit.data, 'SOC', 10, 0) }} %</strong></div>
          <div><span>Температура</span><strong>{{ value(unit.data, 'cell_temp_avg', 10, 1) }} °C</strong></div>
          <div><span>Залишок</span><strong>{{ value(unit.data, 'remain_capacity', 100, 1) }} А·год</strong></div>
          <div><span>Повна ємність</span><strong>{{ value(unit.data, 'full_charge_capacity', 100, 1) }} А·год</strong></div>
        </div>
        <div class="hub-progress"><span class="hub-progress-fill" :style="{width: progress(unit.data) + '%'}"></span></div>
        <div v-if="hasFault(unit.data)" class="hub-alert">
          Попередження / захист / помилка: {{ hex(unit.data.warning) }} / {{ hex(unit.data.protection) }} / {{ hex(unit.data.fault_status) }}
        </div>
      </article>
    </div>
    <div class="hub-hint">
      Зведення призначене для моніторингу. Данні для інвертора Deye формуються прошивкою ESP32 окремо.
      Струм показано як медіану показників BMS; за суттєвої різниці датчиків потрібна перевірка.
    </div>
  </section>
</template>

<script>
export default {
  name: "Home",
  data() {
    return {
      refreshSec: 5,
      refreshTimer: null,
      loading: false,
      message: "Завантаження...",
    };
  },
  computed: {
    units() {
      const all = this.$store.getters["bms"] || [];
      return all.filter(unit => unit && unit.config && Number(unit.config.type) !== -1).slice(0, 5);
    },
    onlineCount() {
      return this.units.filter(unit => unit.data !== null && unit.data !== undefined).length;
    },
    allOnline() {
      return this.units.length > 0 && this.onlineCount === this.units.length;
    },
    totalVoltage() {
      if (!this.allOnline) return null;
      const values = this.measurements("pack_voltage", 100);
      return values ? values.reduce((sum, v) => sum + v, 0) : null;
    },
    packCurrent() {
      if (!this.allOnline) return null;
      const values = this.measurements("pack_current", 100);
      if (!values) return null;
      const sorted = values.slice().sort((a, b) => a - b);
      const mid = Math.floor(sorted.length / 2);
      return sorted.length % 2 ? sorted[mid] : (sorted[mid - 1] + sorted[mid]) / 2;
    },
    packSoc() {
      if (!this.allOnline) return null;
      const values = this.measurements("SOC", 10);
      return values ? Math.min.apply(null, values) : null;
    },
    packCapacity() {
      if (!this.allOnline) return null;
      const values = this.measurements("remain_capacity", 100);
      return values ? Math.min.apply(null, values) : null;
    },
  },
  watch: {
    refreshSec() {
      this.schedule();
    },
  },
  mounted() {
    this.initialize();
  },
  beforeDestroy() {
    clearTimeout(this.refreshTimer);
  },
  methods: {
    numberOrDash(n, digits) {
      return n === null || !Number.isFinite(n) ? "—" : n.toFixed(digits);
    },
    value(data, key, scale, digits) {
      if (!data || data[key] === null || data[key] === undefined) return "—";
      const n = Number(data[key]) / scale;
      return Number.isFinite(n) ? n.toFixed(digits) : "—";
    },
    progress(data) {
      if (!data) return 0;
      const n = Number(data.SOC) / 10;
      return Number.isFinite(n) ? Math.max(0, Math.min(100, n)) : 0;
    },
    measurements(key, scale) {
      const values = this.units.map(unit => {
        const data = unit.data || {};
        if (data[key] === null || data[key] === undefined) return NaN;
        return Number(data[key]) / scale;
      });
      return values.length && values.every(Number.isFinite) ? values : null;
    },
    hasFault(data) {
      return !!data && [data.warning, data.protection, data.fault_status].some(v => Number(v) !== 0 && Number.isFinite(Number(v)));
    },
    hex(value) {
      return '0x' + (Number(value) >>> 0).toString(16).toUpperCase();
    },
    initialize() {
      this.$store.dispatch("get_config")
        .then(() => this.refresh())
        .catch(error => {
          this.message = "Помилка конфігурації: " + (error.statusText || error.message || "немає зв'язку");
          this.schedule();
        });
    },
    refresh() {
      if (this.loading) return;
      this.loading = true;
      clearTimeout(this.refreshTimer);
      this.$store.dispatch("get_summary")
        .then(() => {
          this.message = "Оновлено о " + new Date().toLocaleTimeString();
        })
        .catch(error => {
          this.message = "Помилка оновлення: " + (error.statusText || error.message || "немає зв'язку");
          // Clear stale readings when the API is unreachable.
          this.$store.commit("CLEAR_DATA");
        })
        .then(() => {
          this.loading = false;
          this.schedule();
        });
    },
    schedule() {
      clearTimeout(this.refreshTimer);
      if (this.refreshSec > 0) {
        this.refreshTimer = setTimeout(() => {
          if (this.$store.getters["config_loaded"]) this.refresh();
          else this.initialize();
        }, this.refreshSec * 1000);
      }
    },
  },
};
</script>
