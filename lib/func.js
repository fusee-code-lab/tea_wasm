import initModule from "./tea.js";

let Module = {};
const ITER = 16;

function to_str_c(str) {
  let strBuffer = new TextEncoder().encode(str);
  let strPointer = Module._malloc(strBuffer.length + 1);
  Module.HEAPU8.set(strBuffer, strPointer);
  Module.HEAPU8[strPointer + strBuffer.length] = 0;
  return strPointer;
}

// 初始化
export async function teaInit() {
  Module = await initModule();
}
/**
 * Encode
 * @param {string} str
 * @param {string} key
 * @param {number} iter
 */
export function teaEncode(str, key, iter = ITER) {
  const str_c = to_str_c(str);
  const key_c = to_str_c(key);
  const size = Module._teaEncode(0, str_c, key_c, iter);
  let str_pointer = Module._malloc(size);
  Module._teaEncode(str_pointer, to_str_c(str), key_c, iter);
  const str_buffer = new Uint8Array(Module.HEAPU8.buffer, str_pointer, size);
  const str_out = new TextDecoder().decode(str_buffer);
  Module._free(str_pointer);
  return str_out;
}

/**
 * Decrypt
 * @param {string} str
 * @param {string} key
 * @param {number} iter
 */
export function teaDecrypt(str, key, iter = ITER) {
  const str_c = to_str_c(str);
  const key_c = to_str_c(key);
  const size = Module._teaDecrypt(0, str_c, key_c, iter);
  let str_pointer = Module._malloc(size);
  Module._teaDecrypt(str_pointer, to_str_c(str), key_c, iter);
  const str_buffer = new Uint8Array(Module.HEAPU8.buffer, str_pointer, size);
  const str_out = new TextDecoder().decode(str_buffer);
  Module._free(str_pointer);
  return str_out;
}
