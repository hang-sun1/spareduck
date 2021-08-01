/*
    Chess board interface. Move and utility functions
    https://github.com/ornicar/chessground-examples/blob/master/src/util.ts
*/

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

// Returns a map of moves square -> squares
export function toDests(chess) {
  console.log('getting dests for', toColor(chess));
  const dests = new Map();
  let moves = [];
  let moves_vect = chess.get_moves();
  for (let i = 0; i < moves_vect.size(); i += 2) {
    let from = moves_vect.get(i);
    let to = moves_vect.get(i + 1);
    /*if (to === "O-O" || to === "O-O-O") {
      dests.set(to, undefined);
      continue;
    }*/
    moves.push(from);
    moves.push(to);
  }

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
export function toColor(chess) {
  return chess._get_side_to_move() ? 'black' : 'white';
}

// Plays a move and then switches players.
export function playOtherSide(ground, chess) {
  return (from, to) => {
    if (to[1] === '1') {
      let rows = ground.getFen().split('/');
      console.log('white on last rank', rows);
      let ind = rows[7].indexOf('p');
      if (ind > 0) {
        let piece = prompt('Promote to (q,r,n,b)', 'q');
        rows[7] =
          rows[7].substring(0, ind) +
          piece.toLowerCase() +
          rows[7].substring(ind + 1);
        const config = { fen: rows.join('/') };
        ground.set(config);
      }
    } else if (to[1] === '8') {
      let rows = ground.getFen().split('/');
      console.log('white on last rank', rows);
      let ind = rows[0].indexOf('P');
      if (ind > 0) {
        let piece = prompt('Promote to (Q,R,N,B)', 'Q');
        rows[0] =
          rows[0].substring(0, ind) +
          piece.toUpperCase() +
          rows[0].substring(ind + 1);
        const config = { fen: rows.join('/') };
        ground.set(config);
      }
    }
    from = algebraicToIndex(from);
    to = algebraicToIndex(to);
    chess._make_move(from, to);
    //ground.toggleOrientation();
    ground.set({
      turnColor: toColor(chess),
      check: chess._in_check(),
      movable: {
        color: toColor(chess),
        dests: toDests(chess),
      },
    });
  };
}

// play against ai
export function aiPlay(ground, chess) {
  return (from, to) => {
    chess._make_move(algebraicToIndex(from), algebraicToIndex(to));
    console.log('ai making move for', toColor(chess));
    setTimeout(() => {
      const ai_move = chess.get_engine_move();
      let ai_from = ai_move.substring(0, 2);
      let ai_to = ai_move.substring(2);
      console.log({ to, from, ai_to, ai_from });
      ground.move(ai_from, ai_to);
      ground.set({
        turnColor: toColor(chess),
        movable: {
          color: toColor(chess),
          dests: toDests(chess),
        },
      });
    }, 250);
    ground.playPremove();
  };
}
