import { Chessground } from 'chessground';
import { toColor, toDests, aiPlay, playOtherSide } from './util';
require('./chessground.styles.css');
import * as Comlink from 'comlink';

const data = require('./db/db');

/*
    Chess board interface. Build using Chessground.
    https://github.com/ornicar/chessground
*/
const init = async (
  init_config = { fen: false, side: 'white', playAi: true },
) => {
  const { fen, side, playAi } = init_config;

  // Initialize web worker
  const worker = new Worker('./worker.js', { type: 'module' });
  const EngineWorker = Comlink.wrap(worker);
  const engine_worker = await new EngineWorker(fen);
  await engine_worker.init();

  console.log(engine_worker);
  console.log('------------------');

  init_buttons(engine_worker);

  const config = {
    turnColor: await toColor(engine_worker),
    movable: {
      color: side, // sets starting color
      free: false,
      dests: await toDests(engine_worker),
    },
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

  ground.set({
    movable: {
      events: {
        after: playAi
          ? await aiPlay(ground, engine_worker)
          : await playOtherSide(ground, engine_worker),
      },
    },
  });

  setInterval(async () => {
    const evaluation = await engine_worker.get_engine_evaluation();
    let eval_bar = document.getElementById('eval-bar');
    const eval_percent = Math.floor(evaluation / 50 + 50);
    eval_bar.innerHTML = `<div style='margin-left:2px;'>${evaluation}</div>`;
    eval_bar.setAttribute(
      'style',
      `height: 18px;width: 476px;margin: 12px 0;background:linear-gradient(to right, white ${eval_percent}%, black ${eval_percent}%, black 100%);border: 2px solid black;`,
    );

    let pv_elem = document.getElementById('p-var');
    let pv = await engine_worker.get_principal_variation();

    pv_elem.innerHTML = 'Principal Variation: ' + pv.join(', ');
    return;
  }, 2000);
};

const init_buttons = (engine_worker) => {
  document.getElementById('swap-button').addEventListener('click', function () {
    init({ side: 'black' });
  });
  document.getElementById('test-button').addEventListener('click', function () {
    run_tests(engine_worker);
  });
  document.getElementById('self-button').addEventListener('click', function () {
    init({ playAi: false });
  });
  document.getElementById('rst-button').addEventListener('click', function () {
    init();
  });
  // TODO add reset, switch sides, and play local buttons
};

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
