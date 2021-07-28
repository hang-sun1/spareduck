import { Chessground } from 'chessground';
import { toColor, toDests, aiPlay, playOtherSide } from './util';
require('./chessground.styles.css');

import spareduck from './spareduck.js';
import spareduckModule from './spareduck.wasm';
const module = spareduck();

console.log(
  module.then((m) => {
    console.log(m._add_two(1, 2));
    console.log('init', toColor(m));
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
    fen: "1r3r1k/1p1Q2p1/p4q1p/2p1p1b1/2P1B3/P1BPP3/4R1PP/1R4K1 w - - 6 26",
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
        after: playOtherSide(ground, chess),
        // after: aiPlay(ground, chess, 500),
      },
    },
  });

  setInterval(() => {
    const evaluation = chess._get_engine_evaluation(); // To work with negamax this eval function swaps sign based on who's moving, this causes a bug for the frontend
    let eval_bar = document.getElementById('eval-bar');
    //console.log('eval: ', evaluation);
    const eval_percent = Math.floor(evaluation / 50 + 50);
    eval_bar.innerHTML = `<div style='margin-left:2px;'>${evaluation}</div>`;
    eval_bar.setAttribute(
      'style',
      `height: 16px;width: 476px;margin: 12px 0;background:linear-gradient(to right, white ${eval_percent}%, black ${eval_percent}%, black 100%);border: 2px solid black;`,
    );

    let pv_elem = document.getElementById('p-var');
    let pv = chess.get_principal_variation();
    let pv_list = new Array(pv.size());
    for (let i = 0; i < pv.size(); i++) {
      pv_list[i] =
        pv.get(i).origin_square_algebraic() +
        ' ' +
        pv.get(i).destination_square_algebraic();
    }
    console.log('js-pv: ', pv_list);
    pv_elem.innerHTML = 'Principal Variation: ' + pv_list.join(', ');
  }, 2000);
};
