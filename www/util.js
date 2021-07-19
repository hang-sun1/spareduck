/*
    Chess board interface. Move and utility functions
    https://github.com/ornicar/chessground-examples/blob/master/src/util.ts
*/

// Returns a map of moves square -> square
export function toDests(chess) {
  const dests = new Map();
  chess.generate_moves().forEach((s) => {
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
  return chess.get_side_to_move() ? 'black' : 'white';
}

// Plays a move and then switches players.
export function playOtherSide(ground, chess) {
  return (orig, dest) => {
    //chess.make_move({ from: orig, to: dest });
    ground.set({
      turnColor: toColor(chess),
      //check:chess.in_check(),
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
    chess.make_move({ from: orig, to: dest });
    setTimeout(() => {
      const moves = chess.moves({ verbose: true });
      const move = firstMove
        ? moves[0]
        : moves[Math.floor(Math.random() * moves.length)];
      chess.make_move(move.san);
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
