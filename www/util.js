/*
    Chess board interface. Move and utility functions
    https://github.com/ornicar/chessground-examples/blob/master/src/util.ts
*/

// Returns a map of moves square -> squares
export async function toDests(chess) {
  console.log('getting dests for', await toColor(chess));
  const dests = new Map();
  let moves = await chess.get_moves();
  //console.log(moves);
  for (let i = 0; i < moves.length; i += 2) {
    if (dests.has(moves[i])) {
      dests.set(moves[i], dests.get(moves[i]).concat(moves[i + 1]));
    } else {
      dests.set(moves[i], [moves[i + 1]]);
    }
  }
  return dests;
}

// Maps engine side representation to strings.
export async function toColor(chess) {
  return (await chess.get_side_to_move()) ? 'black' : 'white';
}

// checks if
function checkPromotion(ground, to) {
  if (to[1] === '1') {
    let rows = ground.getFen().split('/');
    let ind = rows[7].indexOf('p');
    if (ind > -1) {
      let piece = prompt('Promote to (q,r,n,b)', 'q');
      rows[7] =
        rows[7].substring(0, ind) +
        piece.toLowerCase() +
        rows[7].substring(ind + 1);
      const config = { fen: rows.join('/') };
      ground.set(config);
      return piece;
    }
  } else if (to[1] === '8') {
    let rows = ground.getFen().split('/');
    console.log('checkPromotion: ', rows);
    let ind = rows[0].indexOf('P');
    if (ind > -1) {
      let piece = prompt('Promote to (Q,R,N,B)', 'Q');
      rows[0] =
        rows[0].substring(0, ind) +
        piece.toUpperCase() +
        rows[0].substring(ind + 1);
      const config = { fen: rows.join('/') };
      ground.set(config);
      return piece;
    }
  }
  return false;
}

// Plays a move and then switches players.
export function playOtherSide(ground, chess) {
  return async (from, to) => {
    let promotion = checkPromotion(ground, to); // returns false if no promote else q,r,n,b
    from = algebraicToIndex(from);
    to = algebraicToIndex(to);
    if (promotion) {
      console.log(promotion);
      await chess.make_move(
        from,
        to,
        true,
        promotion.toLowerCase().charCodeAt(0),
      );
    } else {
      await chess.make_move(from, to, false, 1);
    }
    //ground.toggleOrientation();
    ground.set({
      turnColor: await toColor(chess),
      check: await chess.in_check(),
      movable: {
        color: await toColor(chess),
        dests: await toDests(chess),
      },
    });
  };
}

// play against ai
export function aiPlay(ground, chess, self_play) {
  console.log('we are here')
  return async (from, to) => {
    let promotion = checkPromotion(ground, to); // returns false if no promote else q,r,n,b
    from = algebraicToIndex(from);
    to = algebraicToIndex(to);
    if (promotion) {
      console.log(promotion);
      promotion = promotion.toLowerCase().charCodeAt(0);
      await chess.make_move(from, to, true, promotion);
    } else {
      await chess.make_move(from, to, false, 1);
    }
    console.log('ai making move for', await toColor(chess));
    setTimeout(async () => {
      console.time('get_engine_move');
      const ai_move = self_play ? await chess.play_engines() : await chess.get_engine_move();
      console.timeEnd('get_engine_move');

      let ai_from = ai_move.substring(0, 2);
      let ai_to = ai_move.substring(2);
      ground.move(ai_from, ai_to);
      if (!self_play) {
        ground.set({ turnColor: await toColor(chess),
          movable: {
            color: await toColor(chess),
            dests: await toDests(chess),
          },
        });
        ground.playPremove();
      } else {
        ground.set({
          turnColor: await toColor(chess),
        })
      }
    }, 250);
  };
}

// Move conversion functions
function indexToAlgebraic(n) {
  let file = n & 7;
  let rank = n >> 3;
  rank += 1;
  let files = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'];
  file = files[file];
  return file + rank;
}

function algebraicToIndex(square) {
  let file = square.charCodeAt(0) - 97;
  let rank = parseInt(square[1]) - 1;
  return 8 * rank + file;
}
