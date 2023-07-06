/*
 * video.cpp
 */

#include <Arduino.h>
#include "video.h"
#include "config.h"

static spi_device_interface_config_t if_cfg {
  .command_bits = 0,
  .address_bits = 0,
  .dummy_bits = 0,
  .mode = SPI_MODE0,
  .duty_cycle_pos = 128,
  .cs_ena_pretrans = 0,
  .cs_ena_posttrans = 0,
  .clock_speed_hz = TFT_SPICLK,
  .input_delay_ns = 0,
  .spics_io_num = -1,  // 5 = VSPI
  .flags = SPI_DEVICE_HALFDUPLEX,
  .queue_size = 3,
  .pre_cb = NULL,
  .post_cb = NULL,
};

#if !defined(TFT_MISO) || !defined(TFT_MOSI) || !defined(TFT_SCLK)
// At least one of the SPI pins is not defined, so we'll set that to default ESP32 SPI pins
#ifndef TFT_MISO
#define TFT_MISO MISO
#endif

#ifndef TFT_MOSI
#define TFT_MOSI MOSI
#endif

#ifndef TFT_SCLK
#define TFT_SCLK SCK
#endif

#endif

#ifndef TFT_MAC
#define TFT_MAC 0x48
#endif

spi_bus_config_t bus_cfg{
    .mosi_io_num = TFT_MOSI,
    .miso_io_num = TFT_MISO,
    .sclk_io_num = TFT_SCLK,
    .max_transfer_sz = 224 * 8 * 2, // one complete 8x8 tile row at 16 bpp
    .flags = SPICOMMON_BUSFLAG_MASTER,
};

#define W16(a)    (a>>8), (a&0xff)    // store 16 bit parameter

static const uint8_t init_cmd[] = {
#ifdef TFT_ILI9341
  0xEF, 3, 0x03, 0x80, 0x02,
  0xCF, 3, 0x00, 0xC1, 0x30,
  0xED, 4, 0x64, 0x03, 0x12, 0x81,
  0xE8, 3, 0x85, 0x00, 0x78,
  0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
  0xF7, 1, 0x20,
  0xEA, 2, 0x00, 0x00,
  0xC0, 1, 0x23,                    // Power control VRH[5:0]
  0xC1, 1, 0x10,                    // Power control SAP[2:0];BT[3:0]
  0xC5, 2, 0x3e, 0x28,              // VCM control
  0xC7, 1, 0x86,                    // VCM control2
#ifdef TFT_VFLIP
  0x36, 1, TFT_MAC^0xc0,            // Memory Access Control
#else
  0x36, 1, TFT_MAC,                 // Memory Access Control, with x/y order reversed
#endif
  0x37, 1, 0x00,                    // Vertical scroll zero
  0x3A, 1, 0x55,
  0xB1, 2, 0x00, 0x18,              // Framerate control
  0xB6, 3, 0x08, 0x82, 0x27,        // Display Function Control
  0xF2, 1, 0x00,                    // 3Gamma Function Disable
  0x26, 1, 0x01,                    // Gamma curve selected
  0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
    0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
  0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
    0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
  0x11, 0,                          // Exit Sleep
  0xff, 150,                        // 150ms delay
  0x29, 0,                          // Display on
  0xff, 150,                        // 150ms delay
  0x00                              // End of list
#else
  // Init commands for 7789 screens
  0x01, 0,                          // Software reset
  0xff, 150,                        // 150 ms delay
  0x11, 0,                          // Out of sleep mode
  0xff, 10,                         // 10 ms delay
  0x3a, 1, 0x55,                    // Set color mode 16-bit color
  0xff, 10,                         // 10 ms delay
#ifdef TFT_VFLIP
  0x36, 1, 0x00,                    // Mem access ctrl, upside down, RGB
#else
  0x36, 1, 0xc0,                    // Mem access ctrl, upside up, RGB
#endif
  0x2a, 4, W16(0), W16(240),        // Column addr set, XSTART = 0, XEND = 240     
  0x2b, 4, W16(0), W16(320),        // Row addr set, YSTART = 0, YEND = 320
  0x21, 0,                          // INV ON
  0xff, 10,                         // 10 ms delay
  0x13, 0,                          // Normal display on
  0xff, 10,                         // 10 ms delay
  0x29, 0,                          // Main screen turn on
  0xff, 10,                         // 10 ms delay
  0x00                              // End of list
#endif
};


Video::Video() {
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH); // Deselect
  pinMode(TFT_DC, OUTPUT);
  digitalWrite(TFT_DC, HIGH); // Data mode

  // allocate a background buffer which is kept untouched during DMA transfer
  dma_buffer = (unsigned char*)heap_caps_malloc(224*8*2, MALLOC_CAP_DMA);

  // 40Mhz is max possible rate with esp32
  // 40Mhz = 2.5MPix/s. A frame has 64512 pixels
  // -> max 38 frames/s = 25.8ms/frame
  spi_bus_initialize(VSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  spi_bus_add_device(VSPI_HOST, &if_cfg, &handle);

  // trigger hardware reset
  if (TFT_RST >= 0)
  {
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, LOW);
    delay(100);
    digitalWrite(TFT_RST, HIGH);
    delay(200);
  } else {
    sendCommand(0x01, NULL, 0);
    delay(150);
  }
}

void Video::begin(void) {
  uint8_t cmd;
  const uint8_t *addr = init_cmd;
  while(cmd = *addr++) {
    const uint8_t num = *addr++;
    if(cmd != 0xff) {
      sendCommand(cmd, (uint8_t*)addr, num);
      addr += num;
    } else
      delay(num);
  }

  // write 320x240 16 bit words to zero (black)
  digitalWrite(TFT_CS, LOW);
  setAddrWindow(0, 0, 240, 320);

  memset(dma_buffer, 0, 240*4*2);   // 4 lines per transfer, bytes must be less than 224*8*2
  for(int i=0;i<320/4;i++) {
    transaction.flags = 0;
    transaction.length = 240*4*16; // Length in bits
    transaction.tx_buffer = (const void *)dma_buffer;
    spi_device_transmit(handle, &transaction);
  }

  // set active screen area to centered 224x288 pixels
  setAddrWindow(TFT_X_OFFSET, TFT_Y_OFFSET, 224, 288);

  // enable backlight if pin is specified
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif
}

void Video::write(uint16_t *colors, uint32_t len) {
  static char dma_active = 0;

  transaction.flags = 0;
  transaction.length = 16*len; // Length in bits
  transaction.tx_buffer = dma_buffer;

  if(dma_active) {
    spi_transaction_t* r_trans;
    esp_err_t e = spi_device_get_trans_result(handle, &r_trans, portMAX_DELAY);
    if (e != ESP_OK) 
      printf("[ERROR] SPI device get trans result failed: %d\n", e);
  }

  memcpy(dma_buffer, colors, 2*len);
  esp_err_t e = spi_device_queue_trans(handle, &transaction, portMAX_DELAY);
  if (e != ESP_OK) 
    printf("[ERROR] SPI device queue trans failed: %d\n", e);

  dma_active = 1;
}

void Video::sendCommand(uint8_t commandByte, uint8_t *dataBytes, uint8_t numDataBytes) {
  digitalWrite(TFT_CS, LOW);
  writeCommand(commandByte);
  for(int i=0;i<numDataBytes;i++)
    write8(dataBytes[i]);
  digitalWrite(TFT_CS, HIGH);
}

void Video::writeCommand(uint8_t cmd) {
  digitalWrite(TFT_DC, LOW);
  write8(cmd);
  digitalWrite(TFT_DC, HIGH);
}

void Video::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  writeCommand(0x2A); // Column address set, same command for ili9341 and st7789
  write16(x);
  write16(x + w - 1);

  writeCommand(0x2B); // Row address set, same command for ili9341 and st7789
  write16(y);
  write16(y + h - 1);

  writeCommand(0x2C); // Write to RAM, same command for ili9341 and st7789
}

void Video::write16(uint16_t data) {
  transaction.flags = SPI_TRANS_USE_TXDATA;
  transaction.length = 16; // Length in bits
  transaction.rxlength = 0;
  transaction.tx_data[0] = ((data >> 8) & 0xFF);
  transaction.tx_data[1] = data & 0xFF;

  spi_device_transmit(handle, &transaction);
}

void Video::write8(uint8_t data) {
  transaction.flags = SPI_TRANS_USE_TXDATA;
  transaction.length = 8; // Length in bits
  transaction.rxlength = 0;
  transaction.tx_data[0] = data;

  spi_device_transmit(handle, &transaction);
}
