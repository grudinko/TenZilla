#ifndef TENZILLA_VERSION_H
#define TENZILLA_VERSION_H

/**
 * TenZilla Version Information
 * Обновляется при каждом релизе с изменениями
 */

#define TENZILLA_VERSION_MAJOR    2
#define TENZILLA_VERSION_MINOR    12
#define TENZILLA_VERSION_PATCH    1
#define TENZILLA_VERSION_BUILD    169

// Номер релиза (увеличивается при каждом запросе на изменение)
// Формат: R{MAJOR}.{MINOR}.{PATCH}.{BUILD}
// MAJOR - несовместимые изменения, MINOR - новая функциональность, PATCH - исправления багов, BUILD - любое изменение
#define TENZILLA_RELEASE_NUMBER    "R2.12.1.169"

// Дата изменений (формат: DD.MM.YYYY)
#define TENZILLA_RELEASE_DATE      "10.06.2026"

// Макрос для преобразования числа в строку
#define STRINGIFY(x) #x
#define STRINGIFY_VAL(x) STRINGIFY(x)

// Полная строка версии
#define TENZILLA_VERSION_STRING   "v" STRINGIFY_VAL(TENZILLA_VERSION_MAJOR) "." \
                                   STRINGIFY_VAL(TENZILLA_VERSION_MINOR) "." \
                                   STRINGIFY_VAL(TENZILLA_VERSION_PATCH)

#endif // TENZILLA_VERSION_H
