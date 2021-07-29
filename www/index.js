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
    init_buttons(m);
  }),
);

/*
    Chess board interface. Build using Chessground.
    https://github.com/ornicar/chessground
*/
const init = (chess, fen) => {
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
  
  if (fen) {
    config.fen = fen;
    chess.start_from_position(fen);
    config.movable.color = fen.includes('w') ? 'white' : 'black';
  }

  const ground = Chessground(
    document.getElementById('chessground-board'),
    config,
  );

  ground.set({
    movable: {
      events: {
        // after: playOtherSide(ground, chess),
        after: aiPlay(ground, chess),
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
        pv.get(i).destination_square_algebraic();
    }
    pv_elem.innerHTML = 'Principal Variation: ' + pv_list.join(', ');
  }, 2000);
};

const init_buttons = (chess) => {
  document.getElementById('test-button').addEventListener('click', function () {
    run_tests(chess);
  });
};

const run_tests = (chess) => {
  let fails_vect = chess.test_positions();
  let fails = new Array(fails_vect.size());
  for (let i = 0; i < fails_vect.size(); i++) {
    fails[i] = fails_vect.get(i);
  }
  let extra_data = document.getElementById('extra-data');
  let fail_positions = '<div>Failed positions:</div>';
  fails.forEach(
    (fail, ind) =>
      (fail_positions += `<div style="display:flex;flex-direction:row;text-align: center;">
      <p>${fail}</p>
      <button id="position-${ind}">Start from FEN</button>
     </div>`),
  );
  extra_data.innerHTML = fail_positions;
  fails.forEach((fen, ind) =>
    document
      .getElementById(`position-${ind}`)
      .addEventListener('click', function () {
        init(chess, fen);
      }),
  );

  return;
};
