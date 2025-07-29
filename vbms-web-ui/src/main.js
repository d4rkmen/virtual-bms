import Vue from "vue";
import App from "./App.vue";
import router from "./router";
import store from "./store";
import VueI18n from "vue-i18n";
import {
  Navbar,
  Progress,
  Table,
  Radio,
  Toast,
  Skeleton,
  Dialog,
  Button,
  Field,
  Input,
  Numberinput,
  Select,
  Notification,
  Dropdown,
  Icon,
} from "buefy";
import * as conf from "./config";
import "buefy/dist/buefy.min.css";
import "bulma/css/bulma.min.css"  ;
import "@/assets/font.css";
import VueResource from "vue-resource";
import langEn from "@/assets/i18n/en.json";
import langUa from "@/assets/i18n/ua.json";
import "@/assets/style.scss";

Vue.use(Navbar);
Vue.use(Progress);
Vue.use(Table);
Vue.use(Radio);
Vue.use(Toast);
Vue.use(Skeleton);
Vue.use(Dialog);
Vue.use(Button);
Vue.use(Field);
Vue.use(Input);
Vue.use(Numberinput);
Vue.use(Select);
Vue.use(Notification);
Vue.use(Dropdown);
Vue.use(Icon);
Vue.use(VueI18n);
Vue.use(VueResource);
// Vue.use(VueMq, {
//   breakpoints: {
//     mobile: 450,
//     tablet: 900,
//     laptop: 1250,
//     desktop: Infinity,
//   },
// });

Vue.config.productionTip = false;

Vue.prototype.$config = Object.freeze(conf);

let locale = "en";
try {
  locale = JSON.parse(localStorage.getItem("locale")) || "en";
  //
} catch (e) {}
if (["en", "ua"].indexOf(locale) === -1) locale = "en";
console.log("LOCALE", locale);

/**
 * @param choice {number} a choice index given by the input to $tc: `$tc('path.to.rule', choiceIndex)`
 * @param choicesLength {number} an overall amount of available choices
 * @returns a final choice index to select plural word by
 */
const customRule = function (choice, choicesLength) {
  if (choice === 0) {
    return 0;
  }

  const teen = choice > 10 && choice < 20;
  const endsWithOne = choice % 10 === 1;

  if (choicesLength < 4) {
    return !teen && endsWithOne ? 1 : 2;
  }
  if (!teen && endsWithOne) {
    return 1;
  }
  if (!teen && choice % 10 >= 2 && choice % 10 <= 4) {
    return 2;
  }

  return choicesLength < 4 ? 2 : 3;
};

new Vue({
  router,
  store,
  render: (h) => h(App),
  i18n: new VueI18n({
    locale: locale,
    silentTranslationWarn: true,
    pluralizationRules: {
      ru: customRule,
      ua: customRule,
    },
    messages: {
      en: { ...langEn },
      ua: { ...langUa },
    },
  }),
}).$mount("#app");
