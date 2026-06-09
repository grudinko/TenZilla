# Как выложить TenZilla на GitHub

## 1. Установить Git (если ещё нет)

- Скачать: https://git-scm.com/download/win  
- Установить с настройками по умолчанию.

## 2. Открыть терминал в папке проекта

В Cursor / VS Code: **Terminal → New Terminal** (корень проекта — там, где `Tenzilla.ino`).

Или в **PowerShell** / **cmd**:
```bash
cd "C:\Users\grudi\OneDrive\Документы\Arduino\Tenzilla"
```

## 3. Инициализировать репозиторий и первый коммит

```bash
git init
git add .
git status
```

Проверьте, что в `git status` нет лишнего (например, `backups/` и `build/` не должны попасть — они в `.gitignore`).

```bash
git commit -m "Initial commit: TenZilla ESP32-S3 control system"
```

## 4. Создать репозиторий на GitHub

1. Зайти на https://github.com и войти в аккаунт.  
2. **Create repository** (зелёная кнопка или **+** → **New repository**).  
3. Название, например: `Tenzilla`.  
4. **Public**, без **README**, **.gitignore**, **License** — у вас всё уже есть локально.  
5. Нажать **Create repository**.

## 5. Привязать локальный репозиторий и отправить код

На странице нового репо GitHub будут команды. Используйте такие (подставьте **ваш логин** и **имя репо**):

```bash
git remote add origin https://github.com/VASH_LOGIN/Tenzilla.git
git branch -M main
git push -u origin main
```

При `git push` спросят логин и пароль. **Пароль** — это не пароль от GitHub, а **Personal Access Token (PAT)**:

1. GitHub → **Settings** → **Developer settings** → **Personal access tokens** → **Tokens (classic)**.  
2. **Generate new token**.  
3. Выдать права **repo**, скопировать токен.  
4. Ввести его вместо пароля при `git push`.

(Если используете **GitHub Desktop** или **Git Credential Manager**, логин/токен могут сохраниться.)

## 6. Дальнейшие изменения

После правок в коде:

```bash
git add .
git commit -m "Описание изменений"
git push
```

---

**Кратко:**  
`git init` → `git add .` → `git commit` → создать репо на GitHub → `git remote add origin ...` → `git push -u origin main`.
