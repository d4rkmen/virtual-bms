"use strict";

const webpack = require("webpack");
const { VueLoaderPlugin } = require("vue-loader");
const HtmlPlugin = require("html-webpack-plugin");
const MiniCSSExtractPlugin = require("mini-css-extract-plugin");
const DotenvPlugin = require("dotenv-webpack");
const helpers = require("./helpers");

const isDev = process.env.NODE_ENV === "development";
const version = require("../package.json").version;
const environment = {
  REMOTE: process.env.REMOTE,
  CONFIG: process.env.CONFIG,
  PACKAGE_VERSION:
    version +
    " (" +
    new Date().toISOString().replace(/T/, " ").replace(/\..+/, "") +
    ")",
};
// * loading configuration data from .env.* file
console.log(
  "[CONFIG]",
  process.env.CONFIG,
  "@",
  process.env.REMOTE === "true" ? "REMOTE" : "LOCAL"
);
const dotenvPlugin = new DotenvPlugin({
  path: "./.env." + process.env.CONFIG,
  safe: false,
});
const config_env = dotenvPlugin.gatherVariables();
console.log("[CONFIG_ENV]", config_env);

const webpackConfig = {
  entry: {
    main: helpers.root("src", "main"),
  },
  resolve: {
    extensions: [".js", ".vue", ".json"],
    alias: {
      // vue$: "vue/dist/vue.runtime.common.prod.js",
      // vue$: "vue/dist/vue.esm.js",
      vue$: "vue/dist/vue.min.js",
      // branding: helpers.resolve("src/less"),
      "@": helpers.resolve("src"),
      "styles.less": helpers.resolve(config_env.STYLES),
      // assets: helpers.resolve("src/assets")
      // 'static': "static",
    },
  },
  module: {
    rules: [
      {
        test: /\.vue$/,
        loader: "vue-loader",
      },
      {
        test: /\.js$/,
        include: [helpers.root("src")],
        exclude: /node_modules/,
        loader: "babel-loader",
      },
      {
        test: /\.css$/,
        use: [
          isDev ? "vue-style-loader" : MiniCSSExtractPlugin.loader,
          { loader: "css-loader", options: { sourceMap: isDev } },
          { loader: "postcss-loader", options: { sourceMap: isDev } },
        ],
      },
      {
        test: /\.less$/,
        include: [helpers.root("src")],
        use: [
          isDev ? "vue-style-loader" : MiniCSSExtractPlugin.loader,
          { loader: "css-loader", options: { sourceMap: isDev } },
          { loader: "postcss-loader", options: { sourceMap: isDev } },
          { loader: "less-loader", options: { sourceMap: isDev } },
        ],
      },
      {
        test: /\.s[ac]ss$/i,
        use: [
          isDev ? "vue-style-loader" : MiniCSSExtractPlugin.loader,
          { loader: "css-loader", options: { sourceMap: isDev } },
          { loader: "postcss-loader", options: { sourceMap: isDev } },
          { loader: "sass-loader", options: { sourceMap: isDev } },
        ],
      },
      {
        test: /\.(png|jpe?g|gif|svg)(\?.*)?$/,
        loader: "url-loader",
        options: {
          publicPath: "./",
          limit: 1024,
          esModule: false,
          name: "[name].[ext]",
        },
      },
      {
        test: /\.(woff2?|eot|ttf|otf)(\?.*)?$/,
        loader: "url-loader",
        options: {
          publicPath: "./",
          limit: 1024 * 10,
          esModule: false,
          name: "[name].[ext]",
        },
      },
    ],
  },
  plugins: [
    new webpack.EnvironmentPlugin(environment),
    dotenvPlugin,
    new VueLoaderPlugin(),
    new HtmlPlugin({
      filename: "index.html",
      inject: true,
      template: "index.html",
      minify: {
        removeComments: true,
        collapseWhitespace: true,
        removeAttributeQuotes: true,
      },
    }),
  ],
};

module.exports = webpackConfig;
