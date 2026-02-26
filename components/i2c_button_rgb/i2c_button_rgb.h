#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <vector>
#include <cstdint>
#include <Arduino.h>
// #include <Wire.h>
#include <Adafruit_NeoPixel.h>

namespace esphome {
namespace i2c_button_rgb {

struct ButtonBinding {
  uint16_t index;
  binary_sensor::BinarySensor *sensor;
  bool last_state{false};
};

class I2CButtonRGB : public PollingComponent, public i2c::I2CDevice {
 public:
  // Config setters
  void set_ws2812_pin(InternalGPIOPin *pin) { ws2812_pin_ = pin; }
  void set_max_leds(uint16_t n) { max_leds_ = n; }

  void set_dynamic_address_pin(InternalGPIOPin *pin) { dynamic_address_pin_ = pin; }

  void set_addresses_start_point(uint8_t v) { addresses_start_point_ = v; }
  void set_i2c_reset_address(uint8_t v) { i2c_reset_address_ = v; }
  void set_i2c_requesting_address(uint8_t v) { i2c_requesting_address_ = v; }
  void set_i2c_message_reset_address(uint8_t v) { i2c_message_reset_address_ = v; }
  void set_i2c_message_request_address(uint8_t v) { i2c_message_request_address_ = v; }

  void add_button_sensor(uint16_t index, binary_sensor::BinarySensor *sensor);

  // Hardware / state
  void setup() override;
  void update() override;

  // LED primitives exposed to YAML actions
  void set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
  void fill(uint8_t r, uint8_t g, uint8_t b);
  void clear();
  void show();

 protected:
  void discover_devices_();
  void normal_poll_();

  void initiate_dynamic_addressing_();
  void dynamic_addressing_step_();

  // Pins / config
  InternalGPIOPin *ws2812_pin_{nullptr};
  uint16_t max_leds_{128};

  InternalGPIOPin *dynamic_address_pin_{nullptr};

  uint8_t addresses_start_point_{2};
  uint8_t i2c_reset_address_{0x7F};
  uint8_t i2c_requesting_address_{0x7E};
  uint8_t i2c_message_reset_address_{0x0A};
  uint8_t i2c_message_request_address_{0x0B};

  // Runtime state
  std::vector<uint8_t> button_addresses_;
  std::vector<ButtonBinding> bindings_;

  bool dynamic_addressing_{false};
  uint8_t addresses_requested_{0};
  uint32_t next_dyn_action_ms_{0};

  Adafruit_NeoPixel pixels_{0, 0, NEO_GRB + NEO_KHZ800};
};

// -------------------------
// Actions
// -------------------------

template<typename... Ts>
class SetPixelAction : public Action<Ts...> {
 public:
  explicit SetPixelAction(I2CButtonRGB *parent) : parent_(parent) {}
  void set_index(esphome::TemplatableValue<uint16_t, Ts...> v) { index_ = v; }
  void set_r(esphome::TemplatableValue<uint8_t, Ts...> v) { r_ = v; }
  void set_g(esphome::TemplatableValue<uint8_t, Ts...> v) { g_ = v; }
  void set_b(esphome::TemplatableValue<uint8_t, Ts...> v) { b_ = v; }

  void play(const Ts &...x) override {
    parent_->set_pixel(index_.value(x...), r_.value(x...), g_.value(x...), b_.value(x...));
  }

 protected:
  I2CButtonRGB *parent_;
  esphome::TemplatableValue<uint16_t, Ts...> index_;
  esphome::TemplatableValue<uint8_t, Ts...> r_;
  esphome::TemplatableValue<uint8_t, Ts...> g_;
  esphome::TemplatableValue<uint8_t, Ts...> b_;
};

template<typename... Ts>
class FillAction : public Action<Ts...> {
 public:
  explicit FillAction(I2CButtonRGB *parent) : parent_(parent) {}
  void set_r(esphome::TemplatableValue<uint8_t, Ts...> v) { r_ = v; }
  void set_g(esphome::TemplatableValue<uint8_t, Ts...> v) { g_ = v; }
  void set_b(esphome::TemplatableValue<uint8_t, Ts...> v) { b_ = v; }

  void play(const Ts &...x) override {
    parent_->fill(r_.value(x...), g_.value(x...), b_.value(x...));
  }

 protected:
  I2CButtonRGB *parent_;
  esphome::TemplatableValue<uint8_t, Ts...> r_;
  esphome::TemplatableValue<uint8_t, Ts...> g_;
  esphome::TemplatableValue<uint8_t, Ts...> b_;
};

template<typename... Ts>
class ClearAction : public Action<Ts...> {
 public:
  explicit ClearAction(I2CButtonRGB *parent) : parent_(parent) {}
  void play(const Ts &...x) override { parent_->clear(); }

 protected:
  I2CButtonRGB *parent_;
};

template<typename... Ts>
class ShowAction : public Action<Ts...> {
 public:
  explicit ShowAction(I2CButtonRGB *parent) : parent_(parent) {}
  void play(const Ts &...x) override { parent_->show(); }

 protected:
  I2CButtonRGB *parent_;
};

}  // namespace i2c_button_rgb
}  // namespace esphome