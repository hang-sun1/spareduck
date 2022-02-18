import { Chessground } from 'chessground';
import { toColor, toDests, aiPlay, playOtherSide } from './util';
require('./chessground.styles.css');
import * as Comlink from 'comlink';

const data = require('./db/db');

/*
    Chess board interface. Build using Chessground.
    https://github.com/ornicar/chessground
*/
const init = async (init_config = {}) => {
  // combine fixed params
  const { fen, side, playAi, engines_only} = {
    fen: false,
    side: 'white',
    playAi: true,
    engines_only: false,
    //
    ...init_config,
  };

  // Add loading spinner
  let spinner = document.createElement('div');
  spinner.classList.add('spinner');
  document.body.appendChild(spinner);

  // initialize web worker
  const worker = new Worker('./worker.js', { type: 'module' });
  const EngineWorker = Comlink.wrap(worker);
  const engine_worker = await new EngineWorker(fen);
  await engine_worker.init();

  const config = {
    turnColor: await toColor(engine_worker),
    movable: {
      color: side, // sets starting color
      free: false,
      dests: await toDests(engine_worker),
    },
    orientation: side,
    draggable: {
      showGhost: true,
    },
  };

  if (fen) {
    config.fen = fen;
    //await engine_worker.start_from_position(fen);
    config.movable.color = fen.includes('w') ? 'white' : 'black';
    config.turnColor = config.movable.color;
  }

  const ground = Chessground(
    document.getElementById('chessground-board'),
    config,
  );

  let after = playAi ? aiPlay : playOtherSide;
  if (!engines_only) {
    ground.set({
      movable: {
        events: {
          after: after(ground, engine_worker, false),
        },
      },
    });
  }

  if (engines_only) {
    ground.set( {
      movable: {
        color: undefined,
      },
    })

    setInterval(async () => {
      console.time('get_engine_move');
      const ai_move = await engine_worker.play_engines();
      console.timeEnd('get_engine_move');

      let ai_from = ai_move.substring(0, 2);
      let ai_to = ai_move.substring(2);
      ground.move(ai_from, ai_to);
      ground.set({
        turnColor: await toColor(engine_worker),
      });
    }, 3500);
  }

  // if (side === 'black') {
  //   const open_book = [
  //     ['e2', 'e4'],
  //     ['d2', 'd4'],
  //     ['c2', 'c4'],
  //     ['g1', 'f3'],
  //     ['a2', 'a4'],
  //   ];
  //   const open_move = open_book[Math.floor(Math.random() * open_book.length)];
  //   await engine_worker.make_move(open_move[0], open_move[1], false, 1);
  //   ground.move(open_move[0], open_move[1]);
  //   ground.set({
  //     turnColor: await toColor(engine_worker),
  //     movable: {
  //       color: await toColor(engine_worker),
  //       dests: await toDests(engine_worker),
  //     },
  //   });
  // }

  // initialize buttons
  init_buttons(engine_worker, worker);

  // remove loading spinner
  document.body.removeChild(spinner);

  // ui update loop
  setInterval(async () => {
    // evaluation
    const evaluation = await engine_worker.get_engine_evaluation();
    let eval_bar = document.getElementById('eval-bar');
    const eval_percent = Math.floor(evaluation / 50 + 50);
    eval_bar.innerHTML = `<div style='margin-left:2px;'>${evaluation}</div>`;
    eval_bar.setAttribute(
      'style',
      `height: 18px;width: 476px;margin: 12px 0;background:linear-gradient(to right, white ${eval_percent}%, black ${eval_percent}%, black 100%);border: 2px solid black;`,
    );

    // Principal Variation
    let pv_elem = document.getElementById('p-var');
    let pv = await engine_worker.get_principal_variation();
    pv_elem.innerHTML = 'Principal Variation: ' + pv.join(', ');

    return;
  }, 500);
};

// initialize button bar
const init_buttons = (engine_worker, worker) => {
  // start a new game as white
  let handle_white = () => {
    worker.terminate();
    destroy_buttons();
    init();
  };
  document.getElementById('wht-button').addEventListener('click', handle_white);

  // start a new game as black
  let handle_black = () => {
    worker.terminate();
    destroy_buttons();
    init({ side: 'black' });
  };
  document.getElementById('blk-button').addEventListener('click', handle_black);

  //start a new game playing against self
  let handle_self = () => {
    worker.terminate();
    destroy_buttons({ playAi: false });
    init({ playAi: false });
  };
  document.getElementById('self-button').addEventListener('click', handle_self);

  // start from fen position
  let handle_fen = () => {
    worker.terminate();
    const default_fen =
      'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0';
    destroy_buttons();
    init({ fen: prompt('Input FEN', default_fen) });
  };
  document.getElementById('fen-button').addEventListener('click', handle_fen);

  // run tests from js file
  let handle_test = () => {
    destroy_buttons();
    run_tests(engine_worker);
  };
  document.getElementById('test-button').addEventListener('click', handle_test);
  
  let handle_engines_vs_engines = () => {
    destroy_buttons();
    worker.terminate();
    destroy_buttons();
    const default_fen =
      'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0';
    init({ engines_only: true , fen: prompt('Input FEN', default_fen)});
  };
  document.getElementById('engine-self-play-button').addEventListener('click', handle_engines_vs_engines);

  // remove button listeners
  const destroy_buttons = () => {
    document
      .getElementById('wht-button')
      .removeEventListener('click', handle_white);
    document
      .getElementById('blk-button')
      .removeEventListener('click', handle_black);
    document
      .getElementById('self-button')
      .removeEventListener('click', handle_self);
    document
      .getElementById('fen-button')
      .removeEventListener('click', handle_fen);
    document
      .getElementById('test-button')
      .removeEventListener('click', handle_test);
    document
      .getElementById('engine-self-play-button')
      .removeEventListener('click', handle_engines_vs_engines);
  };
};

// Run tests from puzzle DB and show failed FENS
const run_tests = (engine_worker) => {
  let fails = [];
  data.records.forEach(async (record, hang_is_a_troll) => {
    console.log(hang_is_a_troll, record.moves[0]);
    let moves = await engine_worker.test_position(
      record.fen,
      record.moves.shift(),
    );
    let i = Math.min(moves.length, record.moves.length);
    while (i--) {
      console.log(moves[i], record.moves[i]);
      if (moves[i] !== record.moves[i]) {
        fails.push(record.fen);
        break;
      }
    }
  });

  console.log(fails.length / 100);

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
      .addEventListener('click', async () => {
        await init(fen);
      }),
  );

  return;
};

// Start the program
console.log('hello');
init();
