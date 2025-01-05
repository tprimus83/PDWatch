#include "display.h"
#include <TFT_eSPI.h> 
#include "pd.h"

TFT_eSPI tft = TFT_eSPI();  //135x240


/* Function to decode and draw the image on the TFT display */
void drawImage(char *data) {
  unsigned int x = 0;
  unsigned int y = 0;
  unsigned char pixel[3];  // RGB components of a pixel

  // Loop over the header data
  while (*data != 0 && y < height) {
    // Extract pixel data using the HEADER_PIXEL macro
    HEADER_PIXEL(data, pixel);

    // Convert the RGB values to 16-bit RGB565 color format
    uint16_t color = (pixel[0] >> 3) << 11 | (pixel[1] >> 2) << 5 | (pixel[2] >> 3);

    // Draw the pixel on the TFT screen
    tft.drawPixel(x, y, color);

    // Move to the next pixel
    x++;
    if (x >= width) {
      x = 0;
      y++;
    }
  }
}

void beginDisplay() {
  tft.init();
  
  tft.setRotation(1);
  tft.setSwapBytes(true);

  tft.fillScreen(TFT_BLACK); 
}

void drawBG() {
  drawImage(header_data);
}

TFT_eSPI* getDisplay() {
    return &tft;
}

void alert(String text) {
  TFT_eSPI *display = getDisplay();
  display->fillScreen(TFT_BLACK);
  tft.setTextColor(((252 >> 3) << 11) | ((3 >> 2) << 5) | (3 >> 3), TFT_BLACK);
  display->drawString(text.c_str(), 0, 65);
  delay(1000);
  display->fillScreen(TFT_BLACK);
}