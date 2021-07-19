/*
    Chess board interface. Move and utility functions
    https://github.com/ornicar/chessground-examples/blob/master/src/util.ts
*/

// Returns a map of moves square -> square
export function toDests(chess) {
  const dests = new Map();
  let moves = chess._generate_moves();
  if (!moves) {
    moves = ['a4'];
  }
  moves.forEach((s) => {
    /*const ms = chess.moves({ square: s, verbose: true });
      if (ms.length)
        dests.set(
          s,
          ms.map((m) => m.to),
        );*/
    console.log(s);
  });
  return dests;
}

// Maps engine side representation to strings.
export function toColor(chess) {
  return chess._get_side_to_move() ? 'black' : 'white';
}

// Plays a move and then switches players.
export function playOtherSide(ground, chess) {
  return (orig, dest) => {
    //chess._make_move({ from: orig, to: dest });
    ground.set({
      turnColor: toColor(chess),
      //check:chess._in_check(),
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
