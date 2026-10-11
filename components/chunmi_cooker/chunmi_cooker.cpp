#include "chunmi_cooker.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/defines.h"

#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif

#ifdef USE_CAPTIVE_PORTAL
#include "esphome/components/captive_portal/captive_portal.h"
#endif

#include <ArduinoJson.h>

namespace esphome {
namespace chunmi_cooker {

static const char *const TAG = "chunmi_cooker";

void ChunmiCooker::set_network_state(const std::string &state) {
  ESP_LOGI(TAG, "Queueing network state change: %s", state.c_str());
  this->pending_command_ = "down MIIO_net_change " + state + "\r";
}

void ChunmiCooker::cancel() {
  ESP_LOGI(TAG, "Executing Cancel / Stop: down set_func \"end02\"");
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("Cancel");
  }
  this->pending_command_ = "down set_func \"end02\"\r";
}

void ChunmiCooker::acknowledge() {
  ESP_LOGI(TAG, "Executing Acknowledge Warning: down set_func \"ack\"");
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("Acknowledge");
  }
  this->pending_command_ = "down set_func \"ack\"\r";
}

void ChunmiCooker::mute() {
  ESP_LOGI(TAG, "Executing Mute / Disable Warnings: down set_func \"nowarn\"");
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("Mute");
  }
  this->pending_command_ = "down set_func \"nowarn\"\r";
}

void ChunmiCooker::refresh() {
  ESP_LOGI(TAG, "Manual Status Refresh: down get_prop \"all\"");
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("Refresh");
  }
  this->pending_command_ = "down get_prop \"all\"\r";
}

void ChunmiCooker::start_fallback_ap() {
  ESP_LOGW(TAG, "Triggering Fallback AP and Captive Portal...");
  this->set_network_state("unprov");
#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr) {
    wifi::global_wifi_component->clear_sta();
  }
#endif
#ifdef USE_CAPTIVE_PORTAL
  if (captive_portal::global_captive_portal != nullptr) {
    captive_portal::global_captive_portal->start();
  }
#endif
}

void ChunmiCooker::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Chunmi Cooker UART interface...");
  this->rx_buffer_.reserve(256);

  if (this->pot_present_binary_sensor_ != nullptr) {
    this->pot_present_binary_sensor_->publish_state(true);
  }
  if (this->error_binary_sensor_ != nullptr) {
    this->error_binary_sensor_->publish_state(false);
  }
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("None");
  }
  if (this->last_event_text_sensor_ != nullptr) {
    this->last_event_text_sensor_->publish_state("None");
  }

  // Queue initial network status announcement to host MCU
  this->set_network_state("local");
}

void ChunmiCooker::dump_config() {
  ESP_LOGCONFIG(TAG, "Chunmi Cooker:");
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Remaining Time", this->remaining_time_sensor_);
  LOG_SENSOR("  ", "Duration", this->duration_sensor_);
  LOG_TEXT_SENSOR("  ", "Mode", this->mode_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Stage", this->stage_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Menu", this->menu_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Last Button", this->last_button_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Last Event", this->last_event_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Raw Status", this->raw_status_text_sensor_);
  LOG_BINARY_SENSOR("  ", "Pot Present", this->pot_present_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Error", this->error_binary_sensor_);
}

void ChunmiCooker::update() {
  this->pending_command_ = "down get_prop \"all\"\r";
}

void ChunmiCooker::loop() {
  while (this->available()) {
    char c = this->read();
    if (c == '\r' || c == '\n') {
      if (!this->rx_buffer_.empty()) {
        this->handle_line_(this->rx_buffer_);
        this->rx_buffer_.clear();
      }
    } else {
      if (this->rx_buffer_.length() < 512) {
        this->rx_buffer_.push_back(c);
      } else {
        ESP_LOGW(TAG, "UART RX buffer overflow, discarding: %s", this->rx_buffer_.c_str());
        this->rx_buffer_.clear();
      }
    }
  }
}

void ChunmiCooker::handle_line_(const std::string &line) {
  if (line == "get_down") {
    if (!this->pending_command_.empty()) {
      ESP_LOGD(TAG, "Dispatching pending command: %s", this->pending_command_.c_str());
      this->write_str(this->pending_command_.c_str());
      this->pending_command_.clear();
    } else {
      this->write_str("down none\r");
    }
    return;
  }

  ESP_LOGD(TAG, "MCU RX: %s", line.c_str());

  if (this->raw_status_text_sensor_ != nullptr) {
    this->raw_status_text_sensor_->publish_state(line);
  }

  if (line.rfind("result \"", 0) == 0) {
    if (line.find(',') != std::string::npos) {
      std::string payload = line.substr(7);
      std::string json_str = "{\"result\":[" + payload + "]}";
      this->parse_json_result_(json_str);
    } else {
      ESP_LOGI(TAG, "MCU Command Acknowledged: %s", line.c_str());
    }
  } else if (line.rfind("{\"result\":", 0) == 0 || line.rfind("{\"id\":", 0) == 0) {
    this->parse_json_result_(line);
  } else if (line.rfind("event ", 0) == 0) {
    this->handle_event_(line);
  } else if (line.rfind("props ", 0) == 0) {
    this->handle_props_(line);
  } else if (line == "restore" || line == "factory") {
    ESP_LOGW(TAG, "Host MCU requested Wi-Fi / Factory reset (%s)!", line.c_str());
    if (this->last_button_text_sensor_ != nullptr) {
      this->last_button_text_sensor_->publish_state("wifi_reset");
    }
    if (this->last_event_text_sensor_ != nullptr) {
      this->last_event_text_sensor_->publish_state("wifi_reset");
    }
    this->start_fallback_ap();
    this->wifi_reset_callback_.call();
  } else if (line.find("model chunmi.cooker.normal2") != std::string::npos) {
    ESP_LOGI(TAG, "Host MCU confirmed model: %s", line.c_str());
    this->handshake_done_ = true;
  } else if (line.rfind("result ", 0) == 0 || line == "ok") {
    ESP_LOGD(TAG, "Command acknowledged by MCU: %s", line.c_str());
  } else if (line == "error") {
    ESP_LOGW(TAG, "MCU rejected command with error");
  }
}

static std::string decode_stage_name(const std::string &stage_hex) {
  if (stage_hex == "null" || stage_hex.empty()) {
    return "Idle";
  }
  if (stage_hex.length() >= 2) {
    int stage_id = -1;
    char hex_byte[3] = {stage_hex[0], stage_hex[1], '\0'};
    stage_id = strtol(hex_byte, nullptr, 16);
    switch (stage_id) {
      case 0x00: return "Preheating";
      case 0x01: return "Water Absorption";
      case 0x02:
      case 0x05:
      case 0x07: return "Boiling";
      case 0x03: return "Gelatinization";
      case 0x04: return "Braising";
      case 0x08: return "Flavor Development";
      case 0x0A: return "High-Temp Gelatinization";
      case 0x10: return "Finished";
      default: return "Cooking (" + stage_hex.substr(0, 2) + ")";
    }
  }
  return stage_hex;
}

static std::string decode_menu_name(const std::string &menu_id) {
  if (menu_id == "0001") return "Standard Rice";
  if (menu_id == "0002") return "Quick Rice";
  if (menu_id == "0003") return "Porridge";
  if (menu_id == "0004") return "Keep Warm";
  if (menu_id == "0103") return "Favorite / Custom";
  return "Preset (" + menu_id + ")";
}

void ChunmiCooker::parse_json_result_(const std::string &json_str) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json_str);
  if (error) {
    ESP_LOGW(TAG, "Failed to parse JSON status: %s", error.c_str());
    return;
  }

  JsonArray result = doc["result"];
  if (result.isNull() || result.size() < 7) {
    ESP_LOGD(TAG, "JSON does not contain expected status array");
    return;
  }

  // Index 0: Operation Mode (func)
  const char *func_str = result[0];
  if (func_str != nullptr) {
    std::string mode = func_str;
    if (this->mode_text_sensor_ != nullptr) {
      this->mode_text_sensor_->publish_state(mode);
    }
    if (this->error_binary_sensor_ != nullptr) {
      this->error_binary_sensor_->publish_state(mode == "error");
    }
    if (mode != "error" && this->pot_present_binary_sensor_ != nullptr) {
      this->pot_present_binary_sensor_->publish_state(true);
    }
  }

  // Index 1: Menu profile ID
  const char *menu_str = result[1];
  if (menu_str != nullptr && this->menu_text_sensor_ != nullptr) {
    this->menu_text_sensor_->publish_state(decode_menu_name(menu_str));
  }

  // Index 2: Cooking Stage
  const char *stage_str = result[2];
  if (stage_str != nullptr && this->stage_text_sensor_ != nullptr) {
    this->stage_text_sensor_->publish_state(decode_stage_name(stage_str));
  }

  // Index 3: Temperature (ambient in °C when idle e.g. "22")
  const char *temp_str = result[3];
  if (temp_str != nullptr && this->temperature_sensor_ != nullptr) {
    std::string t_str = temp_str;
    if (t_str.length() <= 3 && isdigit(t_str[0])) {
      float temp_val = strtof(t_str.c_str(), nullptr);
      this->temperature_sensor_->publish_state(temp_val);
    }
  }

  // Index 4: Remaining Time in minutes
  const char *t_func_str = result[4];
  if (t_func_str != nullptr && this->remaining_time_sensor_ != nullptr) {
    float rem_val = strtof(t_func_str, nullptr);
    this->remaining_time_sensor_->publish_state(rem_val);
  }

  // Index 6: Duration in minutes
  const char *t_cook_str = result[6];
  if (t_cook_str != nullptr && this->duration_sensor_ != nullptr) {
    float dur_val = strtof(t_cook_str, nullptr);
    this->duration_sensor_->publish_state(dur_val);
  }

  ESP_LOGI(TAG, "Cooker Status: Mode=%s, Temp=%s°C, Remaining=%s min",
           func_str ? func_str : "null",
           temp_str ? temp_str : "null",
           t_func_str ? t_func_str : "null");
}

void ChunmiCooker::handle_event_(const std::string &event_str) {
  ESP_LOGW(TAG, "MCU Alarm/Event: %s", event_str.c_str());

  if (this->last_event_text_sensor_ != nullptr) {
    this->last_event_text_sensor_->publish_state(event_str);
  }

  if (event_str.find("event nopan") != std::string::npos) {
    ESP_LOGW(TAG, "Inner cooking pot removed!");
    if (this->pot_present_binary_sensor_ != nullptr) {
      this->pot_present_binary_sensor_->publish_state(false);
    }
  } else if (event_str.find("sensorbad") != std::string::npos ||
             event_str.find("sensorhigh") != std::string::npos ||
             event_str.find("protection") != std::string::npos ||
             event_str.find("wirebroken") != std::string::npos) {
    ESP_LOGE(TAG, "Hardware fault event: %s", event_str.c_str());
    if (this->error_binary_sensor_ != nullptr) {
      this->error_binary_sensor_->publish_state(true);
    }
  }
}

void ChunmiCooker::handle_props_(const std::string &props_str) {
  ESP_LOGI(TAG, "MCU Property update: %s", props_str.c_str());

  if (props_str.rfind("props button_pressed \"", 0) == 0) {
    size_t first = props_str.find('"');
    size_t last = props_str.rfind('"');
    if (first != std::string::npos && last != std::string::npos && last > first) {
      std::string btn = props_str.substr(first + 1, last - first - 1);
      ESP_LOGI(TAG, "Cooker Button Pressed: %s", btn.c_str());
      if (this->last_button_text_sensor_ != nullptr) {
        this->last_button_text_sensor_->publish_state(btn);
      }
      // Fast refresh on button press
      this->pending_command_ = "down get_prop \"all\"\r";
    }
  }
}

}  // namespace chunmi_cooker
}  // namespace esphome
