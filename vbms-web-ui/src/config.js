// ================== For develop uncomment ==============
let ip = window.location.host;
let api_url;
export const BRAND = process.env.BRAND;
export const TITLE = process.env.TITLE;
export const ICON = process.env.ICON;
export const COMPANY_NAME = process.env.COMPANY_NAME;
export const COMPANY_ADDRESS = process.env.COMPANY_ADDRESS;
export const SOFTWARE_NAME = process.env.SOFTWARE_NAME;
export const PACKAGE_VERSION = process.env.PACKAGE_VERSION.replace(/["]/g, "") || "N/a";
export const PORTAL_URL = process.env.PORTAL_URL;
export const MAX_BMS_COUNT = 15;
export const MAX_BLE_COUNT = 9;

if (process.env.NODE_ENV === "development") {
  // This hack for development and testing. URL of real Butler controller
  // api_url = "http://192.168.88.127";
  api_url = "http://192.168.88.112";
} else {
  api_url = "http://" + ip;
}
export const url = api_url;
console.log(ICON + "Welcome to " + TITLE + " v" + PACKAGE_VERSION);
