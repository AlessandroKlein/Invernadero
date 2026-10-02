#include "hardware/SpiManager.hpp"

namespace gh {

bool SpiManager::begin(int sck, int miso, int mosi, uint32_t freq) {
  if (ready_) return true;
  freq_ = freq;
  spi_.begin(sck, miso, mosi, -1);  // -1: sin SS gestionado por el bus
  ready_ = true;
  return true;
}

void SpiManager::end() {
  if (!ready_) return;
  spi_.end();
  ready_ = false;
}

} // namespace gh
