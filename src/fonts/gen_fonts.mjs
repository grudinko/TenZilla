#!/usr/bin/env node
/**
 * Генерация шрифтов Montserrat 14/18/24 с кириллицей для LVGL.
 * Нужен Node.js 14+. Запуск: npm install && npm run gen
 */

import { createWriteStream, existsSync, unlinkSync } from "fs";
import { get } from "https";
import { execSync } from "child_process";
import { dirname, join } from "path";
import { fileURLToPath } from "url";

const __dir = dirname(fileURLToPath(import.meta.url));
const FONT_URL = "https://raw.githubusercontent.com/JulietaUla/Montserrat/master/fonts/ttf/Montserrat-Medium.ttf";
const TTF = join(__dir, "Montserrat-Medium.ttf");
const RANGES = "-r 0x20-0x7F -r 0x400-0x4FF";
const SIZES = [14, 18, 24];

function download(url, dest) {
  return new Promise((resolve, reject) => {
    const file = createWriteStream(dest);
    get(url, (res) => {
      if (res.statusCode !== 200) {
        reject(new Error(`HTTP ${res.statusCode}`));
        return;
      }
      res.pipe(file);
      file.on("finish", () => {
        file.close();
        resolve();
      });
    }).on("error", (e) => {
      try { unlinkSync(dest); } catch (_) {}
      reject(e);
    });
  });
}

async function main() {
  if (!existsSync(TTF)) {
    console.log("Downloading Montserrat-Medium.ttf...");
    await download(FONT_URL, TTF);
  }

  const lvConv = join(__dir, "node_modules", ".bin", "lv_font_conv");
  if (!existsSync(lvConv)) {
    console.log("Run: npm install");
    process.exit(1);
  }

  for (const size of SIZES) {
    const out = join(__dir, `lv_font_montserrat_${size}_cyr.c`);
    const cmd = `"${lvConv}" --font "${TTF}" ${RANGES} --size ${size} --bpp 4 --format lvgl --no-compress -o "${out}"`;
    console.log(`Generating ${size}px...`);
    execSync(cmd, { stdio: "inherit", shell: true });
  }

  console.log("Done. Add the generated .c files to the project (e.g. Sketch → Add File).");
}

main().catch((e) => {
  console.error(e);
  process.exit(1);
});
