#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace chunmi_cooker {

class ChunmiCooker : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  void set_temperature_sensor(sensor::Sensor *s) { this->temperature_sensor_ = s; }
  void set_remaining_time_sensor(sensor::Sensor *s) { this->remaining_time_sensor_ = s; }
  void set_duration_sensor(sensor::Sensor *s) { this->duration_sensor_ = s; }

  void set_mode_text_sensor(text_sensor::TextSensor *s) { this->mode_text_sensor_ = s; }
  void set_stage_text_sensor(text_sensor::TextSensor *s) { this->stage_text_sensor_ = s; }
  void set_menu_text_sensor(text_sensor::TextSensor *s) { this->menu_text_sensor_ = s; }
  void set_last_button_text_sensor(text_sensor::TextSensor *s) { this->last_button_text_sensor_ = s; }
  void set_last_event_text_sensor(text_sensor::TextSensor *s) { this->last_event_text_sensor_ = s; }
  void set_raw_status_text_sensor(text_sensor::TextSensor *s) { this->raw_status_text_sensor_ = s; }

  void set_pot_present_binary_sensor(binary_sensor::BinarySensor *s) { this->pot_present_binary_sensor_ = s; }
  void set_error_binary_sensor(binary_sensor::BinarySensor *s) { this->error_binary_sensor_ = s; }

  void set_network_state(const std::string &state);

  void add_on_wifi_reset_callback(std::function<void()> callback) {
    this->wifi_reset_callback_.add(std::move(callback));
  }

  // Command dispatch actions
  void cancel();
  void acknowledge();
  void mute();
  void refresh();
  void start_fallback_ap();

 protected:
  void handle_line_(const std::string &line);
  void parse_json_result_(const std::string &json_str);
  void handle_event_(const std::string &event_str);
  void handle_props_(const std::string &props_str);

  std::string rx_buffer_;
  std::string pending_command_;
  bool handshake_done_{false};

  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *remaining_time_sensor_{nullptr};
  sensor::Sensor *duration_sensor_{nullptr};

  text_sensor::TextSensor *mode_text_sensor_{nullptr};
  text_sensor::TextSensor *stage_text_sensor_{nullptr};
  text_sensor::TextSensor *menu_text_sensor_{nullptr};
  text_sensor::TextSensor *last_button_text_sensor_{nullptr};
  text_sensor::TextSensor *last_event_text_sensor_{nullptr};
  text_sensor::TextSensor *raw_status_text_sensor_{nullptr};

  binary_sensor::BinarySensor *pot_present_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *error_binary_sensor_{nullptr};

  CallbackManager<void()> wifi_reset_callback_;
};

class WiFiResetTrigger : public Trigger<> {
 public:
  explicit WiFiResetTrigger(ChunmiCooker *parent) {
    parent->add_on_wifi_reset_callback([this]() { this->trigger(); });
  }
};

class CancelButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) {
      this->parent_->cancel();
    }
  }
  ChunmiCooker *parent_{nullptr};
};

class AcknowledgeButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) {
      this->parent_->acknowledge();
    }
  }
  ChunmiCooker *parent_{nullptr};
};

class MuteButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) {
      this->parent_->mute();
    }
  }
  ChunmiCooker *parent_{nullptr};
};

class RefreshButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) {
      this->parent_->refresh();
    }
  }
  ChunmiCooker *parent_{nullptr};
};

}  // namespace chunmi_cooker
}  // namespace esphome
