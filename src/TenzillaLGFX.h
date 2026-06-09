#ifndef TENZILLA_LGFX_H
#define TENZILLA_LGFX_H

#include "TenZillaLvglShim.h"
#include <LovyanGFX.hpp>
#include "TenZillaPins.h"

// Класс для настройки LovyanGFX под дисплей 320x480
// Для 320x480 обычно используется ILI9488 или ST7796
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9488 _panel_instance;  // Изменено с ILI9341 на ILI9488 для 320x480
  lgfx::Bus_SPI       _bus_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      
      cfg.spi_host = SPI2_HOST;  // Используем SPI2 для ESP32-S3
      cfg.spi_mode = 0;
      cfg.freq_write = 50000000;  // 50 MHz для высокой производительности (40-60 МГц для FPS)
      cfg.freq_read  = 20000000;  // 20 MHz для чтения
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;  // DMA включен для повышения производительности
      cfg.pin_sclk = DISPLAY_SCK_PIN;
      cfg.pin_mosi = DISPLAY_MOSI_PIN;
      cfg.pin_miso = DISPLAY_MISO_PIN;
      cfg.pin_dc   = DISPLAY_DC_PIN;
      
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    
    {
      auto cfg = _panel_instance.config();
      
      cfg.pin_cs   = DISPLAY_CS_PIN;
      cfg.pin_rst  = DISPLAY_RST_PIN;
      cfg.pin_busy = -1;
      
      cfg.panel_width      = 320;
      cfg.panel_height     = 480;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = false;
      cfg.invert           = false;
      cfg.rgb_order        = true;   // Для ILI9488 часто требуется true
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;
      
      _panel_instance.config(cfg);
    }
    
    setPanel(&_panel_instance);
  }
};

#endif
