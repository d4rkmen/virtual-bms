<template>
  <div>
    <section class="section">
      <h1 class="title is-spaced">
        <span class="icon-text">
          <span class="icon">
            <i class="icon-about has-text-primary"></i>
          </span>
          <span>{{ $t("Menu_About") }}</span>
        </span>
      </h1>
      <h2 class="subtitle">{{ $t("Menu_About_Sub") }}</h2>
      <p>
        <strong>{{ $t("About_WebUI") }}:</strong> {{ package_version }}
      </p>
      <template v-if="info && info.ID">
        <b-skeleton width="20%" animated></b-skeleton>
        <b-skeleton width="40%" animated></b-skeleton>
        <b-skeleton width="30%" animated></b-skeleton>
        <b-skeleton animated></b-skeleton>
      </template>
      <template v-else>
        <p v-for="(value, key) in info" :key="key">
          <strong>{{ $t(key) }}:</strong> {{ value }}
        </p>
      </template>
    </section>
  </div>
</template>

<script>
export default {
  data() {
    return {
      tid: null,
    };
  },
  computed: {
    package_version() {
      return this.$store.getters["package_version"];
    },
    info() {
      return this.$store.getters["info"];
    },
  },
  mounted() {
    console.log("About mounted");
    this.load_info();
  },
  beforeDestroy() {
    console.log("About destroyed");
    clearTimeout(this.tid);
  },
  methods: {
    load_info() {
      this.$store.dispatch("get_info").then(
        (resp) => {},
        (err) => {
          this.$buefy.toast.open({
            duration: 5000,
            message: this.$t("Error_Info", { err: err.statusText }),
            position: "is-bottom",
            type: "is-danger",
          });
          clearTimeout(this.tid);
          this.tid = setTimeout(() => {
            this.load_info();
          }, 5000);
        }
      );
    },
    error(message) {},
  },
};
</script>
