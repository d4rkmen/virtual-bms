const purgecss = require("@fullhuman/postcss-purgecss");

const plugins = [
  require("autoprefixer")({
    browsers: ["> 1%", "last 2 versions"],
  }),
];

if (process.env.NODE_ENV === "production4") {
  plugins.push(
    purgecss({
      content: [],
      css: [],
      keyframes: true,
      variables: true,
      rejected: true,
    })
  );
}

module.exports = { plugins };
