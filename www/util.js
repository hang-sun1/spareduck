/*
    Chess board interface. Move and utility functions
    https://github.com/ornicar/chessground-examples/blob/master/src/util.ts
*/

// Returns a map of moves square -> square
export function toDests(chess) {
  console.log('getting dests for', toColor(chess));
  const dests = new Map();
  let moves_vect = chess._get_moves();
  console.log('moves_vect', moves_vect);

  let moves = ['a2', 'a4'];
  if (moves_vect) {
    moves = new Array(moves_vect.size());
    for (let i = 0; i < moves_vect.size(); i++) moves[i] = moves_vect.get(i);
    moves_vect.delete();
  }
  console.log('moves', moves);

  for (let i = 0; i < moves.length; i += 2) {
    console.log(moves[i]);
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
    chess._make_move(from, to);
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
export function aiPlay(ground, chess, delay, firstMove) {
  return (orig, dest) => {
    chess._make_move({ from: orig, to: dest });
    setTimeout(() => {
      const moves = chess._generate_moves();
      const move = firstMove
        ? moves[0]
        : moves[Math.floor(Math.random() * moves.length)];
      chess._make_move(move.san);
      ground.move(move.from, move.to);
      ground.set({
        turnColor: toColor(chess),
        movable: {
          color: toColor(chess),
          dests: toDests(chess),
        },
      });
      ground.playPremove();
    }, delay);
  };
}
