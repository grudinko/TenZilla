#ifndef TENZILLA_LGFX_ALT_H
#define TENZILLA_LGFX_ALT_H

#include <LovyanGFX.hpp>
#include "TenZillaPins.h"

// Альтернативная версия с ST7796 (если ILI9488 не работает)
// Для использования замените #include "TenZillaLGFX.h" на #include "TenZillaLGFX_ALT.h"
// в TenZillaDisplay.cpp

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7796 _panel_instance;  // ST7796 для 320x480
  lgfx::Bus_SPI       _bus_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
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
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;
      
      _panel_instance.config(cfg);
    }
    
    setPanel(&_panel_instance);
  }
};

#endif
