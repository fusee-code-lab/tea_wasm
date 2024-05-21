const { teaEncrypt } = require("tea-napi");

const key = Buffer.from("Pr4nOjvzNc2BMHpMK/e4Aw==", "base64");
const value = Buffer.from("He");

console.log("Encrypt:", teaEncrypt(value, key, 16).toString("base64"));
