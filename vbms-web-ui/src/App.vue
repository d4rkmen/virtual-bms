<template>
  <div id="app" class="battery-hub">
    <header class="hub-header">
      <router-link class="hub-logo" to="/">
        <span class="hub-symbol">ϟ</span>
        <span><strong>BATTERY HUB</strong><small>ENERGY CONTROL</small></span>
      </router-link>
      <nav class="hub-nav" aria-label="Навігація">
        <router-link exact to="/">Моніторинг</router-link>
        <router-link to="/settings">{{ $t("Menu_Settings") }}</router-link>
        <router-link to="/about">{{ $t("Menu_About") }}</router-link>
      </nav>
      <div class="hub-locale">
        <button type="button" :class="{ selected: locale === 'ua' }" @click="setLocale('ua')">UA</button>
        <button type="button" :class="{ selected: locale === 'en' }" @click="setLocale('en')">EN</button>
      </div>
    </header>
    <main class="hub-content"><router-view /></main>
    <footer class="hub-footer">
      <span>BATTERY HUB · ENERGY CONTROL</span>
      <span>Сделано bond_ua · Днепр</span>
    </footer>
  </div>
</template>

<script>
export default {
  data() {
    return { locale: "ua" };
  },
  mounted() {
    this.locale = this.$i18n.locale;
    document.title = "BATTERY HUB | Energy Control";
  },
  methods: {
    setLocale(locale) {
      this.locale = locale;
      this.$i18n.locale = locale;
      localStorage.setItem("locale", JSON.stringify(locale));
    },
  },
};
</script>
