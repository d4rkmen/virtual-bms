"use strict";

const path = require("path");
// const webpack = require("webpack");
const merge = require("webpack-merge");
const OptimizeCSSAssetsPlugin = require("optimize-css-assets-webpack-plugin");
const { CleanWebpackPlugin } = require("clean-webpack-plugin");
const MiniCSSExtractPlugin = require("mini-css-extract-plugin");
const UglifyJSPlugin = require("uglifyjs-webpack-plugin");
const CompressionPlugin = require("compression-webpack-plugin");
const CopyWebpackPlugin = require("copy-webpack-plugin");
const commonConfig = require("./webpack.config.common");
const helpers = require("./helpers");

const webpackConfig = merge(commonConfig, {
  mode: "production",
  output: {
    path: helpers.root("dist"),
    publicPath: "./",
    filename: "[name].js",
    chunkFilename: "[name].js",
  },
  optimization: {
    minimize: true,
    runtimeChunk: "single",
    minimizer: [
      new OptimizeCSSAssetsPlugin({
        cssProcessorPluginOptions: {
          preset: ["default", { discardComments: { removeAll: true } }],
        },
      }),
      new UglifyJSPlugin({
        cache: true,
        parallel: true,
        sourceMap: false,
      }),
    ],
    splitChunks: {
      // chunks: "all",
      // name: true,
      // maxInitialRequests: Infinity,
      // minSize: 0,
      cacheGroups: {
        app: {
          reuseExistingChunk: true,
          test: /[\\/]src[\\/]/,
          name: "app",
          chunks: "initial",
        },
        vendor: {
          reuseExistingChunk: true,
          test: /[\\/]node_modules[\\/]/,
          name: "vendor",
          chunks: "initial",
        },
      },
    },
  },
  plugins: [
    new CleanWebpackPlugin(),
    new MiniCSSExtractPlugin({
      filename: "[name].css",
      chunkFilename: "[name].css",
    }),
    new CompressionPlugin({
      test: /\.(js|css|html)$/,
      filename: "[path].gz",
      algorithm: "gzip",
      threshold: 2048,
      minRatio: 0.8,
      deleteOriginalAssets: true,
    }),
    // copy custom static assets
    new CopyWebpackPlugin([
      {
        from: path.resolve(__dirname, "../static"),
        to: "./",
        ignore: [".*"],
      },
    ]),
    new CopyWebpackPlugin([
      {
        from: path.resolve(__dirname, "../gz_index.html"),
        to: "./index.html",
      },
    ]),
  ],
});

module.exports = webpackConfig;
