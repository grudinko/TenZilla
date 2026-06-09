#!/usr/bin/env node
/**
 * Генерация FontAwesome шрифта для LVGL с иконками:
 * - WiFi (U+F1EB)
 * - Circle-up (U+F0AA)
 * - Circle-down (U+F0AB)
 * - Circle-stop (U+F28D)
 * 
 * Нужен Node.js 14+. Запуск: npm install && npm run gen:fa
 */

import { createWriteStream, existsSync, unlinkSync } from "fs";
import { get } from "https";
import { execSync } from "child_process";
import { dirname, join } from "path";
import { fileURLToPath } from "url";

const __dir = dirname(fileURLToPath(import.meta.url));

// FontAwesome TTF файл (пользователь должен скачать его в эту папку)
const FONT_FILE = join(__dir, "fa-solid-900.ttf");

// Unicode коды нужных иконок из FontAwesome
// WiFi: U+F1EB, Circle-up: U+F0AA, Circle-down: U+F0AB, Circle-stop: U+F28D
const RANGES = "-r 0xF0AA -r 0xF0AB -r 0xF1EB -r 0xF28D";

// Размеры шрифта для разных элементов
// 14px - для индикаторов WiFi/CPU/FPS
// 48px - для иконок двигателя
const SIZES = [14, 48];

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
  console.log("FontAwesome Font Generator for LVGL");
  console.log("Icons: WiFi (F1EB), Circle-up (F0AA), Circle-down (F0AB), Circle-stop (F28D)");
  console.log("");

  if (!existsSync(FONT_FILE)) {
    console.error("❌ Файл не найден:", FONT_FILE);
    console.error("Пожалуйста, скачайте fa-solid-900.ttf и поместите его в папку:", __dir);
    console.error("Или конвертируйте fa-solid-900.woff2 в TTF через онлайн-конвертер");
    process.exit(1);
  }
  
  console.log("✅ Найден TTF файл:", FONT_FILE);

  const lvConv = join(__dir, "node_modules", ".bin", "lv_font_conv");
  if (!existsSync(lvConv)) {
    console.log("Error: lv_font_conv not found. Run: npm install");
    process.exit(1);
  }

  for (const size of SIZES) {
    const out = join(__dir, `lv_font_fontawesome_${size}.c`);
    // Используем WOFF2, но lv_font_conv может не поддерживать его напрямую
    // Попробуем сначала, если не получится - нужно будет конвертировать в TTF
    const cmd = `"${lvConv}" --font "${FONT_FILE}" ${RANGES} --size ${size} --bpp 4 --format lvgl --no-compress -o "${out}"`;
    console.log(`Generating FontAwesome ${size}px...`);
    try {
      execSync(cmd, { stdio: "inherit", shell: true });
      console.log(`✅ Generated: lv_font_fontawesome_${size}.c`);
    } catch (e) {
      console.error(`❌ Failed to generate ${size}px font.`);
      console.error("Note: lv_font_conv may not support WOFF2 directly.");
      console.error("Alternative: Use online converter at https://lvgl.io/tools/fontconverter");
      console.error("  - Upload fa-solid-900.woff2 or convert to TTF first");
      console.error(`  - Range: ${RANGES}`);
      console.error(`  - Size: ${size}, Bpp: 4`);
    }
  }

  console.log("");
  console.log("Done! Add the generated .c files to the project.");
  console.log("Then include them in your code and use the font for labels with FontAwesome icons.");
}

main().catch((e) => {
  console.error(e);
  process.exit(1);
});
