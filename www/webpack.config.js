// webpack configuration taken from https://gist.github.com/surma/b2705b6cca29357ebea1c9e6e15684cc

const path = require('path');

module.exports = {
  mode: 'development',
  context: path.resolve(__dirname, '.'),
  entry: [
    'webpack/hot/dev-server',
    'webpack-dev-server/client?http://localhost:8080/',
    './index.js',
  ],
  output: {
    path: path.resolve(__dirname, 'dist'),
    filename: 'bundle.js',
  },
  devServer: {
    hot: true,
    port: 8080,
    host: '0.0.0.0',
    contentBase: path.join(__dirname, 'dist'),
    compress: false,
    watchOptions: {
      aggregateTimeout: 500,
      poll: 1000,
    },
  },
  // This is necessary due to the fact that emscripten puts both Node and web
  // code into one file. The node part uses Node’s `fs` module to load the wasm
  // file.
  // Issue: https://github.com/kripken/emscripten/issues/6542.
  node: {
    fs: 'empty',
  },
  module: {
    rules: [
      {
        test: /\.css$/i,
        use: ['style-loader', 'css-loader'],
      },
      // Emscripten JS files define a global. With `exports-loader` we can
      // load these files correctly (provided the global’s name is the same
      // as the file name).
      {
        test: /spareduck\.js$/,
        loader: 'exports-loader',
      },
      // wasm files should not be processed but just be emitted and we want
      // to have their public URL.
      {
        test: /spareduck\.wasm$/,
        type: 'javascript/auto',
        loader: 'file-loader',
        options: {
          publicPath: 'dist/',
        },
      },
    ],
  },
};
