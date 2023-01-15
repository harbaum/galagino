/*
 */

#ifndef VIDEO_H
#define VIDEO_H

#include <SPI.h>

class Video {
public:
  Video();

  void begin(void);
  void write(uint16_t *colors, uint32_t len);

private:
  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void sendCommand(uint8_t commandByte, uint8_t *dataBytes, uint8_t numDataBytes);
  void writeCommand(uint8_t cmd);

protected:
  SPISettings _settings;
};

#endif // VIDEO_H
