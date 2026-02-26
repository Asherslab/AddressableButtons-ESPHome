#include "i2c_button_rgb.h"

namespace esphome {
namespace i2c_button_rgb {

static const char *const TAG = "i2c_button_rgb";

void I2CButtonRGB::add_button_sensor(uint16_t index, binary_sensor::BinarySensor *sensor) {
  bindings_.push_back(ButtonBinding{index, sensor, false});
}

void I2CButtonRGB::setup() {
  // WS2812 init
  if (ws2812_pin_ != nullptr) {
    ws2812_pin_->setup();
    ws2812_pin_->pin_mode(gpio::FLAG_OUTPUT);

    pixels_.updateLength(max_leds_);
    pixels_.setPin(ws2812_pin_->get_pin());
    pixels_.updateType(NEO_GRB + NEO_KHZ800);
    pixels_.begin();
    pixels_.clear();
    pixels_.show();
  }

  // dynamic addressing trigger pin (active low)
  if (dynamic_address_pin_ != nullptr) {
    dynamic_address_pin_->setup();
    dynamic_address_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  }

  discover_devices_();
  ESP_LOGI(TAG, "Discovered %u I2C button devices", (unsigned) button_addresses_.size());
}

void I2CButtonRGB::update() {
  // Dynamic addressing trigger (active low)
  if (dynamic_address_pin_ != nullptr) {
    bool pressed = (dynamic_address_pin_->digital_read() == false);
    if (pressed && !dynamic_addressing_) {
      initiate_dynamic_addressing_();
    }
  }

  if (dynamic_addressing_) {
    dynamic_addressing_step_();
    return;
  }

  normal_poll_();
}

// -------------------------
// I2C scan/discovery (uses ESPHome bus lock)
// -------------------------
void I2CButtonRGB::discover_devices_() {
  button_addresses_.clear();

  // Scan 1..119 like your original sketch.
  // Use write_readv(addr, nullptr, 0, nullptr, 0) as an ACK probe.
  for (uint8_t addr = 1; addr < 120; addr++) {
    auto err = this->bus_->write_readv(addr, nullptr, 0, nullptr, 0);
    if (err == i2c::ERROR_OK) {
      button_addresses_.push_back(addr);
      delay(2);
    }
    delay(2);
  }
}

// -------------------------
// Normal polling (reads 1 byte per device)
// -------------------------
void I2CButtonRGB::normal_poll_() {
  const uint16_t discovered = button_addresses_.size();

  for (uint16_t idx = 0; idx < discovered; idx++) {
    uint8_t addr = button_addresses_[idx];

    uint8_t val = 0;
    auto err = this->bus_->read(addr, &val, 1);
    if (err != i2c::ERROR_OK) {
      continue;
    }

    // Keep semantics consistent with typical pullup buttons:
    // device sends LOW when pressed
    bool pressed = (val == LOW);

    // Publish only to bindings that match this discovered index
    for (auto &b : bindings_) {
      if (b.index != idx) continue;

      if (b.last_state != pressed) {
        b.sensor->publish_state(pressed);
        b.last_state = pressed;
      }
    }
  }
}

// -------------------------
// Dynamic addressing
// -------------------------
void I2CButtonRGB::initiate_dynamic_addressing_() {
  ESP_LOGI(TAG, "Dynamic addressing initiated");
  dynamic_addressing_ = true;
  addresses_requested_ = 0;

  // Reset all currently discovered devices
  for (auto addr : button_addresses_) {
    uint8_t data = i2c_message_reset_address_;
    // Equivalent of:
    // Wire.beginTransmission(addr); Wire.write(reset_msg); Wire.endTransmission();
    (void) this->bus_->write(addr, &data, 1, true);
    delay(2);
  }

  // Allow devices time to reboot/reset their address
  next_dyn_action_ms_ = millis() + 100;
}

void I2CButtonRGB::dynamic_addressing_step_() {
  uint32_t now = millis();
  if (now < next_dyn_action_ms_) return;

  uint8_t new_address = addresses_requested_ + addresses_start_point_;

  // Send request + new address to requesting address
  // Equivalent of:
  // Wire.beginTransmission(I2C_REQUESTING_ADDRESS);
  // Wire.write(REQUEST_MSG); Wire.write(new_address);
  // if (Wire.endTransmission() == 0) { ... }
  uint8_t data[2] = {i2c_message_request_address_, new_address};
  auto err = this->bus_->write(i2c_requesting_address_, data, 2, true);

  if (err == i2c::ERROR_OK) {  // same meaning as endTransmission()==0
    ESP_LOGI(TAG, "Assigned address: %u", new_address);
    addresses_requested_++;
    next_dyn_action_ms_ = now + 100;  // give device time to apply address
  } else {
    // Retry quickly
    next_dyn_action_ms_ = now + 20;
  }

  // Check if any device still responds on reset address.
  // In original code, endTransmission()==0 means a device still exists at reset address.
  // Here, ERROR_OK means present; any error means not present -> finish.
  auto probe = this->bus_->write_readv(i2c_reset_address_, nullptr, 0, nullptr, 0);
  if (probe != i2c::ERROR_OK) {
    ESP_LOGI(TAG, "Dynamic addressing finished (reset address not responding, err=%d)", (int) probe);
    dynamic_addressing_ = false;
    addresses_requested_ = 0;
    discover_devices_();
    ESP_LOGI(TAG, "Re-discovered %u I2C button devices", (unsigned) button_addresses_.size());
  }
}

// -------------------------
// LED primitives
// -------------------------
void I2CButtonRGB::set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (ws2812_pin_ == nullptr) return;
  if (index >= max_leds_) return;
  pixels_.setPixelColor(index, pixels_.Color(r, g, b));
}

void I2CButtonRGB::fill(uint8_t r, uint8_t g, uint8_t b) {
  if (ws2812_pin_ == nullptr) return;
  pixels_.fill(pixels_.Color(r, g, b), 0, max_leds_);
}

void I2CButtonRGB::clear() {
  if (ws2812_pin_ == nullptr) return;
  pixels_.clear();
}

void I2CButtonRGB::show() {
  if (ws2812_pin_ == nullptr) return;
  pixels_.show();
}

}  // namespace i2c_button_rgb
}  // namespace esphome