#!/usr/bin/env python3
"""
Конвертация FontAwesome WOFF2 в TTF
Требуется: pip install fonttools[woff]
"""

import sys
import os
from pathlib import Path

try:
    from fontTools.ttLib import TTFont
except ImportError:
    print("Ошибка: fonttools не установлен!")
    print("Установите: pip install fonttools[woff]")
    sys.exit(1)

def convert_woff2_to_ttf(woff2_path, ttf_path):
    """Конвертирует WOFF2 файл в TTF"""
    try:
        print(f"Чтение WOFF2 файла: {woff2_path}")
        font = TTFont(woff2_path)
        
        # Убираем flavor для сохранения как TTF
        font.flavor = None
        
        print(f"Сохранение TTF файла: {ttf_path}")
        font.save(ttf_path)
        
        print("✅ Конвертация завершена успешно!")
        return True
    except Exception as e:
        print(f"❌ Ошибка при конвертации: {e}")
        return False

if __name__ == "__main__":
    script_dir = Path(__file__).parent
    woff2_file = script_dir / "fa-solid-900.woff2"
    ttf_file = script_dir / "fa-solid-900.ttf"
    
    if not woff2_file.exists():
        print(f"❌ Файл не найден: {woff2_file}")
        print("Убедитесь, что fa-solid-900.woff2 находится в папке src/fonts/")
        sys.exit(1)
    
    if ttf_file.exists():
        response = input(f"Файл {ttf_file} уже существует. Перезаписать? (y/n): ")
        if response.lower() != 'y':
            print("Отменено.")
            sys.exit(0)
    
    if convert_woff2_to_ttf(woff2_file, ttf_file):
        print(f"\n✅ Готово! TTF файл сохранен: {ttf_file}")
        print("\nТеперь используйте fa-solid-900.ttf в онлайн-конвертере LVGL:")
        print("https://lvgl.io/tools/fontconverter")
    else:
        sys.exit(1)
