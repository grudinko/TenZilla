/**
 * @file lv_conf.h
 * Configuration file for LVGL v8.x
 * 
 * Этот файл должен находиться в корне проекта для удобства версионирования.
 * 
 * Для Arduino IDE: если файл не подхватывается автоматически, скопируйте его
 * в папку libraries/ рядом с папкой lvgl/ или добавьте флаг компилятора:
 * -DLV_CONF_INCLUDE_SIMPLE или -DLV_CONF_PATH="lv_conf.h"
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/*Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888).
 * 16 = native RGB565 for ILI9488. (8 saved RAM but was linked to boot loop after OTA — reverted.)*/
#define LV_COLOR_DEPTH     16

/*Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI)*/
#define LV_COLOR_16_SWAP   0

/*Enable more complex drawing routines to manage screens transparency.
 *Can be used if the UI is above another layer, e.g. an OSD menu or video player.*/
#define LV_COLOR_SCREEN_TRANSP    0

/* Adjust color mix functions rounding. GPUs might calculate color mix (blending) differently.
 * 0: round down, 64: round up, 128: round nearest*/
#define LV_COLOR_MIX_ROUND_OFS    0

/*====================
   MEMORY SETTINGS
 *====================*/

/*1: use custom malloc/free, 0: use the built-in `lv_mem_alloc()` and `lv_mem_free()`*/
#define LV_MEM_CUSTOM      0
#if LV_MEM_CUSTOM == 0
    /*Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB)*/
    #define LV_MEM_SIZE    (64U * 1024U)          /*[bytes] 64KB для ESP32-S3*/

    /*Set an address for the memory pool instead of allocating it as a normal array. Can be in external SRAM too.*/
    #define LV_MEM_ADR          0     /*0: unused*/
    /*Instead of an address give a memory allocator that will be called to get a memory pool for LVGL. E.g. my_malloc*/
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif

#else       /*LV_MEM_CUSTOM*/
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /*Header for the dynamic memory function*/
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif     /*LV_MEM_CUSTOM*/

/*Number of the intermediate memory buffer used during rendering and other internal processing.
 *You will see an error log message if there wasn't enough buffers. */
#define LV_MEM_BUF_MAX_NUM     16

/*Use the standard `memcpy` and `memset` instead of LVGL's own functions. (Might or might not be faster).*/
#define LV_MEMCPY_MEMSET_STD    1

/*====================
   HAL SETTINGS
 *====================*/

/*Default display refresh, input device read and animation step period.*/
#define LV_DEF_REFR_PERIOD     33      /*[ms] 30 FPS по умолчанию*/

/*Input device read period in milliseconds*/
#define LV_INDEV_DEF_READ_PERIOD    30      /*[ms]*/

/*Use a custom tick source that tells the elapsed time in milliseconds.
 *It removes the need to manually update the tick with `lv_tick_inc()`)*/
#define LV_TICK_CUSTOM     1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE  "Arduino.h"         /*Header for the system time function*/
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())     /*Expression evaluating to current system time in ms*/
    /*If using lvgl as ESP32 component*/
    // #define LV_TICK_CUSTOM_INCLUDE  "esp_timer.h"
    // #define LV_TICK_CUSTOM_SYS_TIME_EXPR (esp_timer_get_time() / 1000)
#endif   /*LV_TICK_CUSTOM*/

/*Default Dot Per Inch. Used to initialize default sizes such as widgets sized, style paddings.
 *(Not so important, you can adjust it to modify default sizes and spaces)*/
#define LV_DPI_DEF      130     /*[px/inch]*/

/*====================
 * FEATURE CONFIGURATION
 *====================*/

/*-------------
 * Drawing
 *-----------*/

/*Enable complex draw engine.
 *Required to draw shadow, gradient, rounded corners, circles, arc, skew, image transformations or any masks*/
#define LV_DRAW_COMPLEX 1
#if LV_DRAW_COMPLEX != 0
    /*Allow buffering some shadow calculation.
    *LV_DRAW_COMPLEX should be 1 as well.*/
    #define LV_DRAW_SW_SHADOW_CACHE_SIZE    0

    /* Set number of maximally cached circle data.
    * The circumference of 1/4 circle are saved for anti-aliasing
    * radius * 4 bytes are used per circle (the most often used radiuses are saved)
    * 0: to disable caching */
    #define LV_DRAW_SW_CIRCLE_CACHE_SIZE    4
#endif /*LV_DRAW_COMPLEX*/

/*Default image cache size. Image caching keeps the images opened.
 *If only the built-in image formats are used there is no real advantage of caching. (I.e. if no new image decoder is added)
 *With complex image decoders (e.g. PNG or JPG) caching can save the continuous open/decode of images.
 *However, the opened images might consume additional RAM.
 *0: to disable caching*/
#define LV_IMG_CACHE_DEF_SIZE       0

/*Number of stops allowed per gradient. Increase this to allow more stops.
 *This adds (sizeof(lv_color_t) + 1) bytes per additional stop*/
#define LV_GRADIENT_MAX_STOPS       2

/* Adjust color mix functions rounding. GPUs might calculate color mix (blending) differently.
 * 0: round down, 64: round up, 128: round nearest*/
#define LV_COLOR_MIX_ROUND_OFS       (LV_COLOR_DEPTH == 32 ? 0: 128)

/*-------------
 * GPU
 *-----------*/

/*Use Arm 2D library to offload the rendering*/
#define LV_USE_DRAW_ARM2D           0

/*-------------
 * Logging
 *-----------*/

/*Enable the log module*/
#define LV_USE_LOG      1
#if LV_USE_LOG

    /*How important log should be added:
    *LV_LOG_LEVEL_TRACE       A lot of logs to give detailed information
    *LV_LOG_LEVEL_INFO        Log important events
    *LV_LOG_LEVEL_WARN        Log if something unwanted happened but didn't cause a problem
    *LV_LOG_LEVEL_ERROR       Only critical issue, when the system may fail
    *LV_LOG_LEVEL_USER        Only logs added by the user
    *LV_LOG_LEVEL_NONE        Do not log anything*/
    #define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN

    /*1: Print the log with 'printf';
    *0: User need to register a callback with `lv_log_register_print_cb()`*/
    #define LV_LOG_PRINTF   1

    /*1: Enable print timestamp;
     *0: Disable print timestamp*/
    #define LV_LOG_USE_TIMESTAMP   1

    /*Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs*/
    #define LV_LOG_TRACE_MEM        1
    #define LV_LOG_TRACE_TIMER       1
    #define LV_LOG_TRACE_INDEV       1
    #define LV_LOG_TRACE_DISP_REFR   1
    #define LV_LOG_TRACE_EVENT       1
    #define LV_LOG_TRACE_OBJ_CREATE  1
    #define LV_LOG_TRACE_LAYOUT      1
    #define LV_LOG_TRACE_ANIM        1

#endif  /*LV_USE_LOG*/

/*-------------
 * Asserts
 *-----------*/

/*Enable asserts if an operation is failed or an invalid data is found.
 *If LV_USE_LOG is enabled an error message will be printed on failure*/
#define LV_USE_ASSERT_NULL          1   /*Check if the parameter is NULL. (Very fast, recommended)*/
#define LV_USE_ASSERT_MALLOC        1   /*Checks is the memory is successfully allocated or no. (Very fast, recommended)*/
#define LV_USE_ASSERT_STYLE         0   /*Check if the styles are properly initialized. (Very fast, recommended)*/
#define LV_USE_ASSERT_MEM_INTEGRITY 0   /*Check the integrity of `lv_mem` after critical operations. (Slow)*/
#define LV_USE_ASSERT_OBJ           0   /*Check the object's type and existence (e.g. not deleted). (Slow)*/

/*Add a custom handler when assert happens e.g. to restart the MCU*/
#define LV_ASSERT_HANDLER_INCLUDE   <stdint.h>
#define LV_ASSERT_HANDLER   while(1);   /*Halt by default*/

/*-------------
 * Others
 *-----------*/

/*1: Enable CPU usage and FPS monitoring*/
#define LV_USE_PERF_MONITOR     0
#if LV_USE_PERF_MONITOR
    #define LV_USE_PERF_MONITOR_POS      LV_ALIGN_BOTTOM_RIGHT
#endif

/*1: Enable the runtime performance profiler*/
#define LV_USE_PROFILER     0
#if LV_USE_PROFILER
    /*1: Enable the built-in profiler*/
    #define LV_USE_PROFILER_BUILTIN   1
    #if LV_USE_PROFILER_BUILTIN
        /*Default profiler trace buffer size*/
        #define LV_USE_PROFILER_BUILTIN_BUFFER_SIZE    (16 * 1024)     /*[bytes]*/
    #endif
#endif

/*1: Enable system monitor component*/
#define LV_USE_MONITOR      0

/*1: Enable the snapshot feature*/
#define LV_USE_SNAPSHOT     0

/*1: Enable system info component*/
#define LV_USE_SYSMON       0

/*1: Enable the file system related functions*/
#define LV_USE_FS_STDIO     0
#if LV_USE_FS_STDIO
    #define LV_FS_STDIO_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
    #define LV_FS_STDIO_PATH ""         /*Set the working directory. File/directory paths will be appended to it.*/
    #define LV_FS_STDIO_CACHE_SIZE      0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif

/*1: Enable PNG decoder (libpng library)*/
#define LV_USE_PNG      0

/*1: Enable BMP decoder*/
#define LV_USE_BMP      0

/*1: Enable JPG decoder (libjpeg library)*/
#define LV_USE_SJPG     0

/*1: Enable GIF decoder*/
#define LV_USE_GIF      0

/*1: Enable QR code library*/
#define LV_USE_QRCODE   0

/*1: Enable FreeType library*/
#define LV_USE_FREETYPE 0
#if LV_USE_FREETYPE
    /*Memory used by FreeType to cache characters [bytes] (-1: no caching)*/
    #define LV_FREETYPE_CACHE_SIZE (16 * 1024)
    #if LV_FREETYPE_CACHE_SIZE >= 0
        /* 1: bitmap cache use the sbit cache, 0:bitmap cache use the image cache. */
        /* sbit cache:it is much more memory efficient for small bitmaps(font size < 256) */
        /* if font size >= 256, must use image cache */
        #define LV_FREETYPE_SBIT_CACHE 0
        /* Maximum number of opened FT_Face/FT_Size objects managed by this cache instance. */
        /* (0:use system defaults) */
        #define LV_FREETYPE_CACHE_FT_FACES 0
        #define LV_FREETYPE_CACHE_FT_SIZES 0
    #endif
#endif

/*1: Enable built in Flash or RAM file system (LV_USE_FS_STDIO is required)*/
#define LV_USE_FS_FATFS     0

/*1: Enable API for taking screenshots*/
#define LV_USE_FFMPEG  0
#if LV_USE_FFMPEG != 0
    #define LV_FFMPEG_DUMP_PATH    ""
#endif

/*====================
 * FONTS
 *====================*/

/* Montserrat fonts with various sizes
 * These are built-in fonts in LVGL v8
 * Set to 1 to enable the font, 0 to disable */
#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_18    1
#define LV_FONT_MONTSERRAT_22    1
#define LV_FONT_MONTSERRAT_24    1
#define LV_FONT_MONTSERRAT_30    1
#define LV_FONT_MONTSERRAT_48    1

/*====================
 * OTHERS
 *====================*/

/*1: Enable the grid navigation*/
#define LV_USE_GRIDNAV     0

/*1: Enable lv_obj fragment*/
#define LV_USE_FRAGMENT     0

/*1: Support using images as font in label or span widgets */
#define LV_USE_IMGFONT     0

/*1: Enable a published subscriber based messaging system */
#define LV_USE_MSG     0

/*1: Enable Pinyin input method*/
/*Requires: lv_keyboard*/
#define LV_USE_IME_PINYIN     0
#if LV_USE_IME_PINYIN
    /*1: Use default thesaurus*/
    /*If you do not use the default thesaurus, be sure to use `lv_ime_pinyin` after setting the thesaurus*/
    #define LV_IME_PINYIN_USE_DEFAULT_DICT    1
    /*Set the maximum number of candidate panels that can be displayed*/
    /*This needs to be adjusted according to the size of the screen*/
    #define LV_IME_PINYIN_CAND_TEXT_NUM 6
#endif

/*1: Enable file explorer (requires: lv_table)*/
#define LV_USE_FILE_EXPLORER                     0
#if LV_USE_FILE_EXPLORER
    /*Maximum length of path*/
    #define LV_FILE_EXPLORER_PATH_MAX_LEN        (128)
    /*Quick access bar, 1:use, 0:not use*/
    /*Requires: lv_list*/
    #define LV_FILE_EXPLORER_QUICK_ACCESS        1
#endif

/*==================
* EXAMPLES
*==================*/

/*Enable the examples to be built with the library*/
#define LV_BUILD_EXAMPLES    0

/*===================
 * DEMO USAGE
 ====================*/

/*Show some widget. It might be required to increase `LV_MEM_SIZE` */
#define LV_USE_DEMO_WIDGETS        0
#if LV_USE_DEMO_WIDGETS
#define LV_DEMO_WIDGETS_SLIDESHOW  0
#endif

/*Demonstrate the usage of encoder and keyboard*/
#define LV_USE_DEMO_KEYPAD_AND_ENCODER     0

/*Benchmark your system*/
#define LV_USE_DEMO_BENCHMARK   0

/*Stress test for LVGL*/
#define LV_USE_DEMO_STRESS      0

/*Music player demo*/
#define LV_USE_DEMO_MUSIC       0
#if LV_USE_DEMO_MUSIC
    #define LV_DEMO_MUSIC_SQUARE       0
    #define LV_DEMO_MUSIC_LANDSCAPE    0
    #define LV_DEMO_MUSIC_ROUND        0
    #define LV_DEMO_MUSIC_LARGE        0
    #define LV_DEMO_MUSIC_AUTO_PLAY    0
#endif

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_H*/
