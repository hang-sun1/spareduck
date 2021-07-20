/*
    Chess board interface. Move and utility functions
    https://github.com/ornicar/chessground-examples/blob/master/src/util.ts
*/

// Returns a map of moves square -> square
export function toDests(chess) {
  console.log('getting dests for', toColor(chess));
  const dests = new Map();
  let moves = []
  let moves_vect = chess.get_moves();
  for (let i = 0; i < moves_vect.size(); i++) {
    let move = moves_vect.get(i);
    let from = move >> 6;
    let to = move & 63;
    from = indexToAlgebraic(from);
    to = indexToAlgebraic(to);
    moves.push(from);
    moves.push(to);
    // console.log(from, to);

  }
  for (let i = 0; i < moves.length; i += 2) {
    // console.log(moves[i]);
    if (dests.has(moves[i])) {
      dests.set(moves[i], dests.get(moves[i]).concat(moves[i + 1]));
    } else {
      dests.set(moves[i], [moves[i + 1]]);
    }
  }

  return dests;
}

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

// Maps engine side representation to strings.
export function toColor(chess) {
  return chess._get_side_to_move() ? 'black' : 'white';
}

// Plays a move and then switches players.
export function playOtherSide(ground, chess) {
  return (from, to) => {
    from = algebraicToIndex(from);
    to = algebraicToIndex(to)
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
