import { Chessground } from 'chessground';
import { toColor, toDests, aiPlay, playOtherSide } from './util';
require('./chessground.styles.css');

import spareduck from './spareduck.js';
import spareduckModule from './spareduck.wasm';
const module = spareduck();

console.log(
  module.then((m) => {
    console.log(m._add_two(1, 2));
    console.log("init", toColor(m));
    init(m);
  }),
);

/*
    Chess board interface. Build using Chessground.
    https://github.com/ornicar/chessground
*/
const init = (chess) => {
  const config = {
    turnColor: toColor(chess),
    movable: {
      color: 'white', // sets starting color
      free: false,
      dests: toDests(chess),
    },
    draggable: {
      showGhost: true,
    },
  };

  const ground = Chessground(
    document.getElementById('chessground-board'),
    config,
  );

  ground.set({
    movable: {
      events: {
        after: playOtherSide(ground, chess), //after: aiPlay(ground, chess...)
      },
    },
  });
};
