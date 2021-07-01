import spareduck from "./spareduck.js"
import spareduckModule from "./spareduck.wasm"
const module = spareduck()

console.log(module.then((m) => { console.log(m._add_two(1,2))}))
