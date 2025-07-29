<template>
  <div id="app">
    <b-navbar type="is-dark">
      <template #brand>
        <b-navbar-item tag="router-link" :to="{ path: '/' }">
          <img src="@/assets/logo.png" alt="Virtual BMS" />
        </b-navbar-item>
      </template>
      <template #start>
        <b-navbar-item tag="router-link" :to="{ path: '/' }">
          <span class="icon mr-1 has-text-primary">
            <i class="icon-home"></i>
          </span>
          {{ $t("Menu_Home") }}
        </b-navbar-item>
        <b-navbar-item tag="router-link" :to="{ path: '/settings' }">
          <span class="icon mr-1 has-text-primary">
            <i class="icon-settings"></i>
          </span>
          {{ $t("Menu_Settings") }}
        </b-navbar-item>
        <b-navbar-dropdown>
          <template #label>
            <span class="icon mr-1 has-text-primary">
              <i class="icon-info"></i>
            </span>
            {{ $t("Menu_Info") }}
          </template>
          <b-navbar-item tag="router-link" :to="{ path: '/about' }">
            <span class="icon mr-1 has-text-primary">
              <i class="icon-about"></i>
            </span>
            {{ $t("Menu_About") }}
          </b-navbar-item>
          <b-navbar-item tag="router-link" :to="{ path: '/contact' }">
            <span class="icon mr-1 has-text-primary">
              <i class="icon-contact"></i>
            </span>
            {{ $t("Menu_Contact") }}
          </b-navbar-item>
        </b-navbar-dropdown>
      </template>

      <template #end>
        <b-navbar-item tag="div">
          <div class="buttons">
            <a
              class="button"
              :class="{
                'is-primary': locale === 'en',
                'is-dark': locale !== 'en',
              }"
              @click="setLocale('en')"
            >
              English
            </a>
            <a
              class="button"
              :class="{
                'is-primary': locale === 'ua',
                'is-dark': locale !== 'ua',
              }"
              @click="setLocale('ua')"
            >
              Українська
            </a>
          </div>
        </b-navbar-item>
      </template>
    </b-navbar>
    <router-view></router-view>
    <footer class="footer">
      <div class="content has-text-centered">
        <p>
          <strong>Virtual BMS</strong> by
          <a href="https://github.com/d4rkmen">d4rkmen</a> Copyright © 2024
        </p>
        <p>
          Powered by <strong>Mongoose OS</strong> ©
          <a href="https://github.com/cesanta"> Cesanta Software</a>
        </p>
      </div>
    </footer>
  </div>
</template>
<script>
export default {
  props: {},
  data() {
    return {
      locale: "en",
    };
  },
  mounted() {
    console.log("App mounted");
    this.locale = this.$i18n.locale;
    // set window title
    document.title = this.$config.TITLE;
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
