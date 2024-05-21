const { execSync } = require("child_process");
const {
  readFileSync,
  writeFileSync,
  renameSync,
  unlinkSync,
  accessSync,
  mkdirSync,
} = require("fs");
const { resolve } = require("path");
const { EOL } = require("os");

try {
  try {
    accessSync(resolve("dist"));
  } catch (error) {
    mkdirSync(resolve("dist"));
  }
  execSync(
    'emcc tea.c -s ASM_JS=1 -s EXPORTED_FUNCTIONS="["_malloc","_free"]" -o tea.js',
    {
      cwd: resolve("lib"),
    }
  );
  const func_str = readFileSync(resolve("lib/func.js"), { encoding: "utf8" });
  const tea_str = readFileSync(resolve("lib/tea.js"), { encoding: "utf8" });
  renameSync(resolve("lib/tea.wasm"), resolve("dist/tea.wasm"));
  writeFileSync(resolve("dist/index.js"), tea_str + EOL + func_str);
  unlinkSync(resolve("lib/tea.js"));
} catch (error) {}
