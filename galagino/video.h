/*
 */

#ifndef VIDEO_H
#define VIDEO_H

#include <driver/spi_master.h>

class Video {
public:
  Video();

  void begin(void);
  void write(uint16_t *colors, uint32_t len);

private:
  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void sendCommand(uint8_t commandByte, uint8_t *dataBytes, uint8_t numDataBytes);
  void writeCommand(uint8_t cmd);
  void write8(uint8_t u8);
  void write16(uint16_t u16);

protected:
  spi_device_handle_t handle;
  spi_transaction_t transaction;
  unsigned char *dma_buffer;  // use a second buffer for dma transfers
};

#endif // VIDEO_H
