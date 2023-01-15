/*
 * video.cpp
 */

#include <Arduino.h>
#include "video.h"
#include "config.h"

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
  0x36, 1, 0x88,                    // Memory Access Control, upside down
#else
  0x36, 1, 0x48,                    // Memory Access Control, upside up
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

  // 40Mhz is max possible rate with esp32
  // 40Mhz = 2.5MPix/s. A frame has 64512 pixels
  // -> max 38 frames/s = 25.8ms/frame
  _settings = SPISettings(TFT_SPICLK, MSBFIRST, SPI_MODE0);
  SPI.begin();

  // trigger hardware reset
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(200);
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

  // the SPI is not shared, so we do everything in one transaction
  SPI.beginTransaction(_settings);

  // write 320x240 16 bit words to zero (black)
  digitalWrite(TFT_CS, LOW);
  setAddrWindow(0, 0, 240, 320);
  for(int i=0;i<320*240;i++) SPI.write16(0);    

  // set active screen area to centered 224x288 pixels
  setAddrWindow(TFT_X_OFFSET, TFT_Y_OFFSET, 224, 288);
}

void Video::write(uint16_t *colors, uint32_t len) {
  SPI.writeBytes((uint8_t *)colors, len * 2);
}

void Video::sendCommand(uint8_t commandByte, uint8_t *dataBytes, uint8_t numDataBytes) {
  digitalWrite(TFT_CS, LOW);
  writeCommand(commandByte);
  if(numDataBytes)
    SPI.writeBytes(dataBytes, numDataBytes);
  digitalWrite(TFT_CS, HIGH);
}

void Video::writeCommand(uint8_t cmd) {
  digitalWrite(TFT_DC, LOW);
  SPI.write(cmd);
  digitalWrite(TFT_DC, HIGH);
}

void Video::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  writeCommand(0x2A); // Column address set, same command for ili9341 and st7789
  SPI.write16(x);
  SPI.write16(x + w - 1);
  
  writeCommand(0x2B); // Row address set, same command for ili9341 and st7789
  SPI.write16(y);
  SPI.write16(y + h - 1);
  
  writeCommand(0x2C); // Write to RAM, same command for ili9341 and st7789
}
