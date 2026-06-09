#!/usr/bin/env python3
"""Скачивание TTF файла FontAwesome"""

import urllib.request
import sys
from pathlib import Path

def download_ttf():
    url = "https://raw.githubusercontent.com/components/font-awesome/master/webfonts/fa-solid-900.ttf"
    output_file = Path(__file__).parent / "fa-solid-900.ttf"
    
    print(f"Скачивание TTF файла FontAwesome...")
    print(f"URL: {url}")
    print(f"Сохранить в: {output_file}")
    
    try:
        urllib.request.urlretrieve(url, output_file)
        print(f"✅ Файл успешно скачан: {output_file}")
        print(f"Размер файла: {output_file.stat().st_size / 1024:.1f} KB")
        return True
    except Exception as e:
        print(f"❌ Ошибка при скачивании: {e}")
        return False

if __name__ == "__main__":
    if download_ttf():
        sys.exit(0)
    else:
        sys.exit(1)
