<template>
  <div>
    <section class="section">
      <h1 class="title is-spaced">
        <span class="icon-text">
          <span class="icon">
            <i class="icon-contact has-text-primary"></i>
          </span>
          <span>{{ $t("Menu_Contact") }}</span>
        </span>
      </h1>
      <h2 class="subtitle">{{ $t("Menu_Contact_Sub") }}</h2>
      <p>
        <a href="http://github.com/d4rkmen" target="_blank">
          <span class="icon">
            <i class="icon-github"></i>
          </span>
          <span>d4rkmen</span>
        </a>        
      </p>
      <p>
        <a href="mailto:darkmen@i.ua" target="_blank">
          <span class="icon">
            <i class="icon-mail"></i>
          </span>
          <span>darkmen@i.ua</span>
        </a>
        </p>  
    </section>
  </div>
</template>

<script>
export default {
  data() {
    return {};
  },

  mounted() {
    console.log("Contact mounted");
  },
};
</script>
