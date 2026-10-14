#include "chunmi_cooker.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/defines.h"
#include "esphome/core/application.h"

#include <ArduinoJson.h>

namespace esphome {
namespace chunmi_cooker {

static const char *const TAG = "chunmi_cooker";

void ChunmiCooker::queue_command(const std::string &cmd) {
  if (this->command_queue_.size() >= 10) {
    ESP_LOGW(TAG, "Command queue full (10 items), discarding oldest command");
    this->command_queue_.pop();
  }
  this->command_queue_.push(cmd);
}

void ChunmiCooker::set_network_state(const std::string &state) {
  ESP_LOGI(TAG, "Queueing network state change: %s", state.c_str());
  this->queue_command("down MIIO_net_change " + state + "\r");
}

void ChunmiCooker::cancel() {
  ESP_LOGI(TAG, "Executing Cancel / Stop: down set_func \"end02\"");
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("Cancel");
  }
  this->queue_command("down set_func \"end02\"\r");
  this->queue_command("down get_prop \"all\"\r");
}

void ChunmiCooker::acknowledge() {
  ESP_LOGI(TAG, "Executing Acknowledge Warning: down set_func \"ack\"");
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("Acknowledge");
  }
  this->queue_command("down set_func \"ack\"\r");
}

void ChunmiCooker::mute() {
  ESP_LOGI(TAG, "Executing Mute / Disable Warnings: down set_func \"nowarn\"");
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("Mute");
  }
  this->queue_command("down set_func \"nowarn\"\r");
}

void ChunmiCooker::refresh() {
  ESP_LOGI(TAG, "Manual Status Refresh: down get_prop \"all\"");
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("Refresh");
  }
  this->queue_command("down get_prop \"all\"\r");
}

void ChunmiCooker::start_profile(const std::string &profile) {
  ESP_LOGI(TAG, "Starting cooking profile: %.16s...", profile.c_str());
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("Start");
  }
  this->queue_command("down set_start \"" + profile + "\"\r");
  this->queue_command("down get_prop \"all\"\r");
}

void ChunmiCooker::set_menu_profile(const std::string &profile) {
  ESP_LOGI(TAG, "Selecting menu profile: %.16s...", profile.c_str());
  this->queue_command("down set_menu \"" + profile + "\"\r");
  this->queue_command("down get_prop \"all\"\r");
}

void ChunmiCooker::select_menu_by_name(const std::string &name) {
  ESP_LOGI(TAG, "Selecting menu by name: %s", name.c_str());
  if (name == "Standard Rice") {
    this->set_menu_profile(profiles::JINGZHU);
  } else if (name == "Quick Rice") {
    this->set_menu_profile(profiles::KUAIZHU);
  } else if (name == "Porridge") {
    this->set_menu_profile(profiles::ZHUZHOU);
  } else if (name == "Keep Warm") {
    this->set_menu_profile(profiles::BAOWEN);
  } else if (name == "Cooking / Steam") {
    this->set_menu_profile(profiles::COOKING);
  } else if (name == "Cake") {
    this->set_menu_profile(profiles::CAKE);
  } else if (name == "Sushi Rice" || name == "Sushi Rice (Experimental)") {
    this->set_menu_profile(profiles::SUSHI_RICE);
  } else if (name == "Express Rice" || name == "Express Rice (Experimental)") {
    this->set_menu_profile(profiles::EXPRESS_RICE);
  } else {
    ESP_LOGW(TAG, "Unknown menu name: %s", name.c_str());
  }
}

void ChunmiCooker::start_selected_menu() {
  std::string current_menu = "Quick Rice";
  if (this->menu_text_sensor_ != nullptr && !this->menu_text_sensor_->state.empty()) {
    current_menu = this->menu_text_sensor_->state;
  }
  ESP_LOGI(TAG, "Starting currently selected menu: %s", current_menu.c_str());
  if (current_menu == "Standard Rice") {
    this->start_profile(profiles::JINGZHU);
  } else if (current_menu == "Quick Rice") {
    this->start_profile(profiles::KUAIZHU);
  } else if (current_menu == "Porridge") {
    this->start_profile(profiles::ZHUZHOU);
  } else if (current_menu == "Keep Warm") {
    this->start_profile(profiles::BAOWEN);
  } else if (current_menu == "Cooking / Steam") {
    this->start_profile(profiles::COOKING);
  } else if (current_menu == "Cake") {
    this->start_profile(profiles::CAKE);
  } else if (current_menu == "Sushi Rice" || current_menu == "Sushi Rice (Experimental)") {
    this->start_profile(profiles::SUSHI_RICE);
  } else if (current_menu == "Express Rice" || current_menu == "Express Rice (Experimental)") {
    this->start_profile(profiles::EXPRESS_RICE);
  } else {
    this->start_profile(profiles::KUAIZHU);
  }
}

void ChunmiCooker::set_interaction(const std::string &settings, uint8_t led_sec, uint8_t lid_sec, uint8_t alarm_sec) {
  char cmd[128];
  snprintf(cmd, sizeof(cmd), "down set_interaction \"%s\" \"%x\" \"%x\" \"%x\"\r",
           settings.c_str(), led_sec, lid_sec, alarm_sec);
  ESP_LOGI(TAG, "Setting interaction: %s", cmd);
  this->queue_command(cmd);
}

void ChunmiCooker::set_led_timeout(uint8_t sec) {
  this->current_led_sec_ = sec;
  this->set_interaction(this->current_settings_, sec, this->current_lid_sec_, this->current_alarm_sec_);
}

void ChunmiCooker::set_lid_warning_timeout(uint8_t sec) {
  this->current_lid_sec_ = sec;
  this->set_interaction(this->current_settings_, this->current_led_sec_, sec, this->current_alarm_sec_);
}

void ChunmiCooker::set_alarm_delay(uint8_t sec) {
  this->current_alarm_sec_ = sec;
  this->set_interaction(this->current_settings_, this->current_led_sec_, this->current_lid_sec_, sec);
}

void ChunmiCooker::start_favorite() {
  if (this->favorite_raw_.empty()) {
    ESP_LOGW(TAG, "Cannot start favorite: No favorite profile received from MCU yet");
    return;
  }
  ESP_LOGI(TAG, "Starting stored favorite recipe: %s", this->favorite_raw_.c_str());
  if (this->favorite_raw_ == "0001") {
    this->start_profile(profiles::JINGZHU);
  } else if (this->favorite_raw_ == "0002") {
    this->start_profile(profiles::KUAIZHU);
  } else if (this->favorite_raw_ == "0003") {
    this->start_profile(profiles::ZHUZHOU);
  } else if (this->favorite_raw_ == "0004") {
    this->start_profile(profiles::BAOWEN);
  } else if (this->favorite_raw_ == "0100" || this->favorite_raw_ == "0101" || this->favorite_raw_ == "0103") {
    this->start_profile(profiles::COOKING);
  } else if (this->favorite_raw_ == "0102") {
    this->start_profile(profiles::CAKE);
  } else if (this->favorite_raw_ == "0105") {
    this->start_profile(profiles::SUSHI_RICE);
  } else if (this->favorite_raw_ == "0106") {
    this->start_profile(profiles::EXPRESS_RICE);
  } else {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "down set_start \"%s\"\r", this->favorite_raw_.c_str());
    this->queue_command(cmd);
  }
}

void ChunmiCooker::send_raw_command(const std::string &cmd) {
  std::string full_cmd = cmd;
  if (full_cmd.rfind("down ", 0) != 0) {
    full_cmd = "down " + full_cmd;
  }
  if (full_cmd.empty() || full_cmd.back() != '\r') {
    full_cmd += '\r';
  }
  ESP_LOGI(TAG, "Queueing raw command: %s", full_cmd.c_str());
  this->queue_command(full_cmd);
}

void ChunmiCooker::request_temp_history() {
  this->queue_command("down get_temp_history\r");
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
  if (this->auto_keep_warm_binary_sensor_ != nullptr) {
    this->auto_keep_warm_binary_sensor_->publish_state(true);
  }
  if (this->lid_open_warning_binary_sensor_ != nullptr) {
    this->lid_open_warning_binary_sensor_->publish_state(false);
  }
  if (this->last_button_text_sensor_ != nullptr) {
    this->last_button_text_sensor_->publish_state("None");
  }
  if (this->last_event_text_sensor_ != nullptr) {
    this->last_event_text_sensor_->publish_state("None");
  }
  if (this->error_text_sensor_ != nullptr) {
    this->error_text_sensor_->publish_state("None");
  }

  // Queue initial network status announcement to host MCU
  this->set_network_state("local");
}

void ChunmiCooker::dump_config() {
  ESP_LOGCONFIG(TAG, "Chunmi Cooker Component:");
  LOG_SENSOR("  ", "Temperature Sensor:", this->temperature_sensor_);
  LOG_SENSOR("  ", "Remaining Time Sensor:", this->remaining_time_sensor_);
  LOG_SENSOR("  ", "Duration Sensor:", this->duration_sensor_);
  LOG_SENSOR("  ", "Delay Time Sensor:", this->delay_time_sensor_);
  LOG_TEXT_SENSOR("  ", "Mode Text Sensor:", this->mode_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Stage Text Sensor:", this->stage_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Menu Text Sensor:", this->menu_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Last Button Text Sensor:", this->last_button_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Last Event Text Sensor:", this->last_event_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Error Text Sensor:", this->error_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Favorite Text Sensor:", this->favorite_text_sensor_);
  LOG_TEXT_SENSOR("  ", "MCU Version Text Sensor:", this->mcu_version_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Hardware Version Text Sensor:", this->hardware_version_text_sensor_);
  LOG_BINARY_SENSOR("  ", "Pot Present Binary Sensor:", this->pot_present_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Error Binary Sensor:", this->error_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Auto Keep Warm Binary Sensor:", this->auto_keep_warm_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Lid Open Warning Binary Sensor:", this->lid_open_warning_binary_sensor_);
}

void ChunmiCooker::update() {
  this->queue_command("down get_prop \"all\"\r");
  if (this->is_running_) {
    this->queue_command("down get_temp_history\r");
  }
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
    if (!this->command_queue_.empty()) {
      std::string cmd = this->command_queue_.front();
      this->command_queue_.pop();
      if (cmd.find("get_prop") != std::string::npos || cmd.find("get_temp_history") != std::string::npos) {
        ESP_LOGV(TAG, "Dispatching poll command: %s", cmd.c_str());
      } else {
        ESP_LOGI(TAG, "Dispatching command: %s", cmd.c_str());
      }
      this->write_str(cmd.c_str());
    } else {
      this->write_str("down none\r");
    }
    return;
  }

  ESP_LOGV(TAG, "MCU RX: %s", line.c_str());

  if (this->raw_status_text_sensor_ != nullptr &&
      (!this->raw_status_text_sensor_->has_state() || this->raw_status_text_sensor_->state != line)) {
    this->raw_status_text_sensor_->publish_state(line);
  }

  if (line.rfind("result \"", 0) == 0) {
    if (line.find(',') != std::string::npos) {
      std::string payload = line.substr(7);
      std::string json_str = "{\"result\":[" + payload + "]}";
      this->parse_json_result_(json_str);
    } else {
      // Single scalar result, e.g. result "0" or result "161515..." (temperature history)
      size_t first = line.find('"');
      size_t last = line.rfind('"');
      if (first != std::string::npos && last != std::string::npos && last > first) {
        std::string val = line.substr(first + 1, last - first - 1);
        if (val.length() >= 2 && val.length() % 2 == 0) {
          bool is_hex = true;
          for (char c : val) {
            if (!isxdigit(c)) { is_hex = false; break; }
          }
          if (is_hex && val.length() > 2) {
            // Temperature history hex stream: last 2 hex chars is current temp in °C
            std::string last_byte = val.substr(val.length() - 2);
            int temp_c = strtol(last_byte.c_str(), nullptr, 16);
            if (temp_c > 0 && temp_c <= 150 && this->temperature_sensor_ != nullptr) {
              if (!this->temperature_sensor_->has_state() || this->temperature_sensor_->state != temp_c) {
                ESP_LOGV(TAG, "Cooking temperature history update: %d°C", temp_c);
                this->temperature_sensor_->publish_state(temp_c);
              }
            }
          }
        }
      }
      ESP_LOGV(TAG, "MCU Command Acknowledged: %s", line.c_str());
    }
  } else if (line.rfind("{\"result\":", 0) == 0 || line.rfind("{\"id\":", 0) == 0) {
    this->parse_json_result_(line);
  } else if (line.rfind("event ", 0) == 0) {
    this->handle_event_(line);
  } else if (line.rfind("props ", 0) == 0) {
    this->handle_props_(line);
  } else if (line == "restore" || line == "factory") {
    ESP_LOGW(TAG, "Host MCU requested Wi-Fi reset (%s) -> Rebooting...", line.c_str());
    if (this->last_button_text_sensor_ != nullptr) {
      this->last_button_text_sensor_->publish_state("wifi_reset");
    }
    if (this->last_event_text_sensor_ != nullptr) {
      this->last_event_text_sensor_->publish_state("wifi_reset");
    }
    this->set_network_state("unprov");
    App.safe_reboot();
  } else if (line == "menuchange") {
    ESP_LOGI(TAG, "Cooker menu changed on hardware lid");
    if (this->last_event_text_sensor_ != nullptr) {
      this->last_event_text_sensor_->publish_state("menuchange");
    }
    this->queue_command("down get_prop \"all\"\r");
  } else if (line == "reboot") {
    ESP_LOGW(TAG, "Host MCU rebooted, re-sending local network sync");
    if (this->last_event_text_sensor_ != nullptr) {
      this->last_event_text_sensor_->publish_state("mcu_reboot");
    }
    this->set_network_state("local");
    this->queue_command("down get_prop \"all\"\r");
  } else if (line.find("model chunmi.cooker.normal2") != std::string::npos) {
    ESP_LOGI(TAG, "Host MCU confirmed model: %s", line.c_str());
    this->handshake_done_ = true;
  } else if (line.rfind("mcu_version ", 0) == 0) {
    std::string ver = line.substr(12);
    ESP_LOGI(TAG, "Host MCU version: %s", ver.c_str());
    if (this->mcu_version_text_sensor_ != nullptr) {
      this->mcu_version_text_sensor_->publish_state(ver);
    }
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
    char hex_byte[3] = {stage_hex[0], stage_hex[1], '\0'};
    int stage_id = strtol(hex_byte, nullptr, 16);
    switch (stage_id) {
      case 0x00: return "Preheating";
      case 0x01: return "Water Absorption";
      case 0x02:
      case 0x05:
      case 0x07: return "Boiling";
      case 0x03: return "Gelatinization";
      case 0x04: return "Braising";
      case 0x08: return "Flavor Development";
      case 0x0a: return "High-Temp Gelatinization";
      case 0x10:
      case 0x11:
      case 0x12: return "Finished";
      default: return "Cooking";
    }
  }
  return "Cooking";
}

static std::string decode_menu_name(const std::string &menu_hex) {
  if (menu_hex == "0001") return "Standard Rice";
  if (menu_hex == "0002") return "Quick Rice";
  if (menu_hex == "0003") return "Porridge";
  if (menu_hex == "0004") return "Keep Warm";
  if (menu_hex == "0100") return "Cake";
  if (menu_hex == "0101") return "Yoghurt";
  if (menu_hex == "0102") return "Reheat Rice";
  if (menu_hex == "0103") return "Cooking / Steam";
  if (menu_hex == "0104") return "Sweet Rice";
  if (menu_hex == "0105") return "Sushi Rice (Experimental)";
  if (menu_hex == "0106") return "Express Rice (Experimental)";
  return "Custom (" + menu_hex + ")";
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
    this->is_running_ = (mode == "running");
    if (this->mode_text_sensor_ != nullptr &&
        (!this->mode_text_sensor_->has_state() || this->mode_text_sensor_->state != mode)) {
      this->mode_text_sensor_->publish_state(mode);
    }
    bool is_err = (mode == "error");
    if (this->error_binary_sensor_ != nullptr &&
        (!this->error_binary_sensor_->has_state() || this->error_binary_sensor_->state != is_err)) {
      this->error_binary_sensor_->publish_state(is_err);
    }
    if (this->error_text_sensor_ != nullptr) {
      if (is_err) {
        if (!this->error_text_sensor_->has_state() || this->error_text_sensor_->state == "None") {
          this->error_text_sensor_->publish_state("MCU Error");
        }
      } else {
        if (this->error_text_sensor_->has_state() && this->error_text_sensor_->state != "None") {
          this->error_text_sensor_->publish_state("None");
        }
      }
    }
    if (mode != "error" && this->pot_present_binary_sensor_ != nullptr &&
        (!this->pot_present_binary_sensor_->has_state() || !this->pot_present_binary_sensor_->state)) {
      this->pot_present_binary_sensor_->publish_state(true);
    }
  }

  // Index 1: Menu profile ID
  const char *menu_str = result[1];
  if (menu_str != nullptr) {
    std::string menu_name = decode_menu_name(menu_str);
    if (this->menu_text_sensor_ != nullptr &&
        (!this->menu_text_sensor_->has_state() || this->menu_text_sensor_->state != menu_name)) {
      this->menu_text_sensor_->publish_state(menu_name);
    }
    if (this->menu_select_ != nullptr &&
        (!this->menu_select_->has_state() || this->menu_select_->current_option() != menu_name)) {
      this->menu_select_->publish_state(menu_name);
    }
  }

  // Index 2: Cooking Stage
  const char *stage_str = result[2];
  if (stage_str != nullptr && this->stage_text_sensor_ != nullptr) {
    std::string stage_name = decode_stage_name(stage_str);
    if (!this->stage_text_sensor_->has_state() || this->stage_text_sensor_->state != stage_name) {
      this->stage_text_sensor_->publish_state(stage_name);
    }
  }

  // Index 3: Temperature (ambient in °C when idle e.g. "22")
  const char *temp_str = result[3];
  if (temp_str != nullptr && this->temperature_sensor_ != nullptr) {
    std::string t_str = temp_str;
    if (t_str.length() <= 3 && isdigit(t_str[0])) {
      float temp_val = strtof(t_str.c_str(), nullptr);
      if (!this->temperature_sensor_->has_state() || this->temperature_sensor_->state != temp_val) {
        this->temperature_sensor_->publish_state(temp_val);
      }
    }
  }

  // Index 4: Remaining Time in minutes
  const char *t_func_str = result[4];
  if (t_func_str != nullptr && this->remaining_time_sensor_ != nullptr) {
    float rem_val = strtof(t_func_str, nullptr);
    if (!this->remaining_time_sensor_->has_state() || this->remaining_time_sensor_->state != rem_val) {
      this->remaining_time_sensor_->publish_state(rem_val);
    }
  }

  // Index 5: Delay / Scheduled Start (t_precook)
  if (result.size() > 5) {
    const char *t_precook_str = result[5];
    if (t_precook_str != nullptr && this->delay_time_sensor_ != nullptr) {
      float delay_val = strtof(t_precook_str, nullptr);
      float final_delay = delay_val >= 0 ? delay_val : 0.0f;
      if (!this->delay_time_sensor_->has_state() || this->delay_time_sensor_->state != final_delay) {
        this->delay_time_sensor_->publish_state(final_delay);
      }
    }
  }

  // Index 6: Duration in minutes
  const char *t_cook_str = result[6];
  if (t_cook_str != nullptr && this->duration_sensor_ != nullptr) {
    float dur_val = strtof(t_cook_str, nullptr);
    if (!this->duration_sensor_->has_state() || this->duration_sensor_->state != dur_val) {
      this->duration_sensor_->publish_state(dur_val);
    }
  }

  // Index 7: Settings bitmask (e.g. "0607")
  if (result.size() > 7) {
    const char *setting_str = result[7];
    if (setting_str != nullptr && strlen(setting_str) >= 2) {
      this->current_settings_ = setting_str;
      char b0_hex[3] = {setting_str[0], setting_str[1], '\0'};
      int b0 = strtol(b0_hex, nullptr, 16);
      bool akw = (b0 & 0x04) != 0;
      bool low = (b0 & 0x08) != 0;
      if (this->auto_keep_warm_binary_sensor_ != nullptr &&
          (!this->auto_keep_warm_binary_sensor_->has_state() || this->auto_keep_warm_binary_sensor_->state != akw)) {
        this->auto_keep_warm_binary_sensor_->publish_state(akw);
      }
      if (this->lid_open_warning_binary_sensor_ != nullptr &&
          (!this->lid_open_warning_binary_sensor_->has_state() || this->lid_open_warning_binary_sensor_->state != low)) {
        this->lid_open_warning_binary_sensor_->publish_state(low);
      }
    }
  }

  // Index 8: Delay / Interaction Timeouts (e.g. "050415")
  if (result.size() > 8) {
    const char *delay_str = result[8];
    if (delay_str != nullptr && strlen(delay_str) >= 6) {
      char led_hex[3] = {delay_str[0], delay_str[1], '\0'};
      char lid_hex[3] = {delay_str[2], delay_str[3], '\0'};
      char alarm_hex[3] = {delay_str[4], delay_str[5], '\0'};
      this->current_led_sec_ = strtol(led_hex, nullptr, 16);
      this->current_lid_sec_ = strtol(lid_hex, nullptr, 16);
      this->current_alarm_sec_ = strtol(alarm_hex, nullptr, 16);
    }
  }

  // Index 9: Hardware and Firmware Version (e.g. "0003001a")
  if (result.size() > 9) {
    const char *ver_str = result[9];
    if (ver_str != nullptr && strlen(ver_str) == 8) {
      char hw_hex[5] = {ver_str[0], ver_str[1], ver_str[2], ver_str[3], '\0'};
      char fw_hex[5] = {ver_str[4], ver_str[5], ver_str[6], ver_str[7], '\0'};
      int hw_ver = strtol(hw_hex, nullptr, 16);
      int fw_ver = strtol(fw_hex, nullptr, 16);
      if (this->hardware_version_text_sensor_ != nullptr) {
        char buf[16];
        snprintf(buf, sizeof(buf), "v%d", hw_ver);
        if (!this->hardware_version_text_sensor_->has_state() || this->hardware_version_text_sensor_->state != buf) {
          this->hardware_version_text_sensor_->publish_state(buf);
        }
      }
      if (this->mcu_version_text_sensor_ != nullptr) {
        char buf[16];
        snprintf(buf, sizeof(buf), "v%d", fw_ver);
        if (!this->mcu_version_text_sensor_->has_state() || this->mcu_version_text_sensor_->state != buf) {
          this->mcu_version_text_sensor_->publish_state(buf);
        }
      }
    }
  }

  // Index 10: Stored Favorite Profile ID (e.g. "0103")
  if (result.size() > 10) {
    const char *fav_str = result[10];
    if (fav_str != nullptr) {
      this->favorite_raw_ = fav_str;
      std::string fav_name = decode_menu_name(fav_str);
      if (this->favorite_text_sensor_ != nullptr &&
          (!this->favorite_text_sensor_->has_state() || this->favorite_text_sensor_->state != fav_name)) {
        this->favorite_text_sensor_->publish_state(fav_name);
      }
    }
  }

  // Log only when core state transitions occur:
  std::string cur_mode = func_str ? func_str : "null";
  std::string cur_stage = stage_str ? decode_stage_name(stage_str) : "null";
  float cur_remaining = t_func_str ? strtof(t_func_str, nullptr) : -1.0f;

  if (cur_mode != this->last_mode_ || cur_stage != this->last_stage_ || cur_remaining != this->last_remaining_) {
    ESP_LOGI(TAG, "Cooker Status: Mode=%s, Stage=%s, Remaining=%.0f min",
             cur_mode.c_str(), cur_stage.c_str(), cur_remaining);
    this->last_mode_ = cur_mode;
    this->last_stage_ = cur_stage;
    this->last_remaining_ = cur_remaining;
  } else {
    ESP_LOGV(TAG, "Cooker Status (polling): Mode=%s, Temp=%s, Remaining=%.0f min",
             cur_mode.c_str(), temp_str ? temp_str : "null", cur_remaining);
  }
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
  } else if (event_str.find("event riceready") != std::string::npos) {
    ESP_LOGI(TAG, "Rice ready / Cooking finished!");
    if (this->stage_text_sensor_ != nullptr) {
      this->stage_text_sensor_->publish_state("Finished");
    }
    this->queue_command("down get_prop \"all\"\r");
  } else if (event_str.find("sensorbad") != std::string::npos ||
             event_str.find("sensorhigh") != std::string::npos ||
             event_str.find("protection") != std::string::npos ||
             event_str.find("wirebroken") != std::string::npos) {
    ESP_LOGE(TAG, "Hardware fault event: %s", event_str.c_str());
    if (this->error_binary_sensor_ != nullptr) {
      this->error_binary_sensor_->publish_state(true);
    }
    if (this->error_text_sensor_ != nullptr) {
      std::string err_desc;
      if (event_str.find("bottomsensorbad") != std::string::npos) {
        err_desc = "Bottom Sensor Fault";
      } else if (event_str.find("topsensorbad") != std::string::npos) {
        err_desc = "Top Lid Sensor Fault";
      } else if (event_str.find("igbtsensorbad") != std::string::npos) {
        err_desc = "IGBT Sensor Fault";
      } else if (event_str.find("botsensorhigh") != std::string::npos) {
        err_desc = "Bottom Overheat";
      } else if (event_str.find("topsensorhigh") != std::string::npos) {
        err_desc = "Top Lid Overheat";
      } else if (event_str.find("igbtsensorhigh") != std::string::npos) {
        err_desc = "IGBT Overheat";
      } else if (event_str.find("hvprotection") != std::string::npos) {
        err_desc = "High Voltage Protection";
      } else if (event_str.find("lvprotection") != std::string::npos) {
        err_desc = "Low Voltage Protection";
      } else if (event_str.find("wirebroken") != std::string::npos) {
        err_desc = "Sensor/Coil Wire Broken";
      } else {
        err_desc = event_str;
      }
      this->error_text_sensor_->publish_state(err_desc);
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
      if (this->last_event_text_sensor_ != nullptr) {
        this->last_event_text_sensor_->publish_state(btn);
      }
      this->queue_command("down get_prop \"all\"\r");
    }
  } else if (props_str.rfind("props runstatus \"", 0) == 0) {
    size_t first = props_str.find('"');
    size_t last = props_str.rfind('"');
    if (first != std::string::npos && last != std::string::npos && last > first) {
      std::string status = props_str.substr(first + 1, last - first - 1);
      ESP_LOGI(TAG, "Cooker Runstatus Update: %s", status.c_str());
      if (this->last_event_text_sensor_ != nullptr) {
        this->last_event_text_sensor_->publish_state(status);
      }
      this->queue_command("down get_prop \"all\"\r");
    }
  } else if (props_str.rfind("props work_status \"", 0) == 0) {
    size_t first = props_str.find('"');
    size_t last = props_str.rfind('"');
    if (first != std::string::npos && last != std::string::npos && last > first) {
      std::string ws = props_str.substr(first + 1, last - first - 1);
      ESP_LOGI(TAG, "Cooker Work Status Update: %s", ws.c_str());
      if (this->last_event_text_sensor_ != nullptr) {
        this->last_event_text_sensor_->publish_state(ws);
      }
      this->queue_command("down get_prop \"all\"\r");
    }
  }
}

}  // namespace chunmi_cooker
}  // namespace esphome
