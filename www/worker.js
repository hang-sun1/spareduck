import * as Comlink from 'comlink';
import spareduck from './spareduck.js';

class EngineWorker {
  constructor(fen) {
    this._fen = fen;
  }
  async init() {
    this._chess = await spareduck();
    if (this._fen) {
      this.start_from_position(fen);
    }
  }
  get_moves() {
    return this.vect_to_list(this._chess.get_moves());
  }
  get_side_to_move() {
    return this._chess._get_side_to_move();
  }
  make_move(from, to, promote, promote_piece) {
    this._chess._make_move(from, to, promote, promote_piece);
  }
  in_check() {
    return this._chess._in_check();
  }
  get_engine_move() {
    return this._chess.get_engine_move();
  }
  get_principal_variation() {
    let pv = this._chess.get_principal_variation();
    let pv_list = new Array(pv.size());
    for (let i = 0; i < pv.size(); i++) {
      pv_list[i] =
        pv.get(i).origin_square_algebraic() +
        pv.get(i).destination_square_algebraic();
    }
    return pv_list;
  }
  get_engine_evaluation() {
    return this._chess._get_engine_evaluation();
  }
  test_position(fen, moves) {
    return this.vect_to_list(this._chess.test_position(fen, moves));
  }
  start_from_position(fen) {
    this._chess.start_fr1om_position(fen);
  }
  vect_to_list(vect) {
    let list = [];
    for (let i = 0; i < vect.size(); i++) {
      list.push(vect.get(i));
    }
    return list;
  }
}

Comlink.expose(EngineWorker);
