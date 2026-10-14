#pragma once

#include <queue>
#include <string>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/select/select.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace chunmi_cooker {

namespace profiles {
// Stock 242-hex character cooking profiles for IHFB01CM (chunmi.cooker.normal2)
static const char *const JINGZHU =
    "0001E101000000000000800200A00069030103730000085A020000EB006B040103740000095A0400012D006E0501037400000A5A0401FFFF00700601047600000C5A0401052D0A0F3C0A1E91FF820E01FF05FF78826EFF10FF786E02690F0DFF826EFF691000FF826EFF69100069FF5AFF00000000000081E9";

static const char *const KUAIZHU =
    "0002E100280000000000800200D20069030103730000075A0200012D006B040103740000075A02000182006E050003740000095A0401FFFF0070060004760000095A040100280A063C0D1E91FF820E01FF05FF78826EFF10FF786E02690F0FFF826EFF69100082826EFF69100069FF5AFF00000000000015B6";

static const char *const ZHUZHOU =
    "0003E2011E04000028008000019055140601001600000000000001B855140601001900000000000001E0561406000020000000000000FFFF57140600002200000000000000280A0082001E914E730E01001E82FF736E0610FF756E02690A1E75826E0269100F75826E0269100069005A000000000000001A2B";

static const char *const BAOWEN =
    "00040C180018000001000045000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000280A0082001E914E730801001E82FF736E0610FF756E02690A0F75826E0169101E75826E0169100069005A000000000000001BA2";

static const char *const CAKE =
    "010088010001000028000012000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000280A003C00089178320600001E8278666E041000826E00690FFF96666E0469100082826E0069100069005A00000000000000B267";

static const char *const COOKING =
    "010366001E0100000A010540000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000280A003C001E9155730E0000008278736E001000756E00690A0075826E006910FF756E6E0869100069005A00000000000000F462";

static const char *const SUSHI_RICE =
    "0105E100320000000002800200780069030103730000085A020000B4006B040103740000095A04000140006E0501037400000A5A0401FFFF00700601047600000C5A040103840A0F3C0A1E91FF820E01FF05FF78826EFF10FF786E02690F0DFF826EFF691000FF826EFF69100069FF5AFF000000000000FB8B";

static const char *const EXPRESS_RICE =
    "0106E1001E00000000018002005A0069030103730000075A02000078006B040103740000075A0200012C006E050103740000095A0401FFFF0070060104760000095A040101680A0F3C0A1E91FF820E01FF05FF78826EFF10FF786E02690F0DFF826EFF691000FF826EFF69100069FF5AFF00000000000021A8";
}  // namespace profiles

class ChunmiCooker : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  // Sensor Setters
  void set_temperature_sensor(sensor::Sensor *s) { this->temperature_sensor_ = s; }
  void set_remaining_time_sensor(sensor::Sensor *s) { this->remaining_time_sensor_ = s; }
  void set_duration_sensor(sensor::Sensor *s) { this->duration_sensor_ = s; }
  void set_delay_time_sensor(sensor::Sensor *s) { this->delay_time_sensor_ = s; }

  // Text Sensor Setters
  void set_mode_text_sensor(text_sensor::TextSensor *s) { this->mode_text_sensor_ = s; }
  void set_stage_text_sensor(text_sensor::TextSensor *s) { this->stage_text_sensor_ = s; }
  void set_menu_text_sensor(text_sensor::TextSensor *s) { this->menu_text_sensor_ = s; }
  void set_last_button_text_sensor(text_sensor::TextSensor *s) { this->last_button_text_sensor_ = s; }
  void set_last_event_text_sensor(text_sensor::TextSensor *s) { this->last_event_text_sensor_ = s; }
  void set_raw_status_text_sensor(text_sensor::TextSensor *s) { this->raw_status_text_sensor_ = s; }
  void set_hardware_version_text_sensor(text_sensor::TextSensor *s) { this->hardware_version_text_sensor_ = s; }
  void set_mcu_version_text_sensor(text_sensor::TextSensor *s) { this->mcu_version_text_sensor_ = s; }

  // Binary Sensor Setters
  void set_pot_present_binary_sensor(binary_sensor::BinarySensor *s) { this->pot_present_binary_sensor_ = s; }
  void set_error_binary_sensor(binary_sensor::BinarySensor *s) { this->error_binary_sensor_ = s; }
  void set_auto_keep_warm_binary_sensor(binary_sensor::BinarySensor *s) { this->auto_keep_warm_binary_sensor_ = s; }
  void set_lid_open_warning_binary_sensor(binary_sensor::BinarySensor *s) { this->lid_open_warning_binary_sensor_ = s; }

  // Select Setter
  void set_menu_select(select::Select *s) { this->menu_select_ = s; }

  // Command Execution & Dispatch
  void cancel();
  void acknowledge();
  void mute();
  void refresh();
  void start_profile(const std::string &profile);
  void set_menu_profile(const std::string &profile);
  void select_menu_by_name(const std::string &name);
  void start_selected_menu();
  void set_interaction(const std::string &settings, uint8_t led_sec, uint8_t lid_sec, uint8_t alarm_sec);
  void send_raw_command(const std::string &cmd);
  void request_temp_history();
  void set_network_state(const std::string &state);
  void queue_command(const std::string &cmd);

 protected:
  void handle_line_(const std::string &line);
  void parse_json_result_(const std::string &json_str);
  void handle_event_(const std::string &event_str);
  void handle_props_(const std::string &props_str);

  std::string rx_buffer_;
  std::queue<std::string> command_queue_;
  bool handshake_done_{false};
  bool is_running_{false};
  std::string last_mode_;
  std::string last_stage_;
  float last_remaining_{-1.0f};

  // Sensors
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *remaining_time_sensor_{nullptr};
  sensor::Sensor *duration_sensor_{nullptr};
  sensor::Sensor *delay_time_sensor_{nullptr};

  // Text Sensors
  text_sensor::TextSensor *mode_text_sensor_{nullptr};
  text_sensor::TextSensor *stage_text_sensor_{nullptr};
  text_sensor::TextSensor *menu_text_sensor_{nullptr};
  text_sensor::TextSensor *last_button_text_sensor_{nullptr};
  text_sensor::TextSensor *last_event_text_sensor_{nullptr};
  text_sensor::TextSensor *raw_status_text_sensor_{nullptr};
  text_sensor::TextSensor *hardware_version_text_sensor_{nullptr};
  text_sensor::TextSensor *mcu_version_text_sensor_{nullptr};

  // Binary Sensors
  binary_sensor::BinarySensor *pot_present_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *error_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *auto_keep_warm_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *lid_open_warning_binary_sensor_{nullptr};

  // Select
  select::Select *menu_select_{nullptr};
};

// Select Component
class MenuSelect : public select::Select, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void control(const std::string &value) override {
    if (this->parent_ != nullptr) {
      this->parent_->select_menu_by_name(value);
    }
  }
  ChunmiCooker *parent_{nullptr};
};

// Button Components
class CancelButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->cancel();
  }
  ChunmiCooker *parent_{nullptr};
};

class AcknowledgeButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->acknowledge();
  }
  ChunmiCooker *parent_{nullptr};
};

class MuteButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->mute();
  }
  ChunmiCooker *parent_{nullptr};
};

class RefreshButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->refresh();
  }
  ChunmiCooker *parent_{nullptr};
};

class StartStandardRiceButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->start_profile(profiles::JINGZHU);
  }
  ChunmiCooker *parent_{nullptr};
};

class StartQuickRiceButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->start_profile(profiles::KUAIZHU);
  }
  ChunmiCooker *parent_{nullptr};
};

class StartPorridgeButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->start_profile(profiles::ZHUZHOU);
  }
  ChunmiCooker *parent_{nullptr};
};

class StartKeepWarmButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->start_profile(profiles::BAOWEN);
  }
  ChunmiCooker *parent_{nullptr};
};

class StartCookingButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->start_profile(profiles::COOKING);
  }
  ChunmiCooker *parent_{nullptr};
};

class StartCakeButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->start_profile(profiles::CAKE);
  }
  ChunmiCooker *parent_{nullptr};
};

class StartSushiRiceButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->start_profile(profiles::SUSHI_RICE);
  }
  ChunmiCooker *parent_{nullptr};
};

class StartExpressRiceButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->start_profile(profiles::EXPRESS_RICE);
  }
  ChunmiCooker *parent_{nullptr};
};

class StartSelectedMenuButton : public button::Button, public Component {
 public:
  void set_parent(ChunmiCooker *parent) { this->parent_ = parent; }
 protected:
  void press_action() override {
    if (this->parent_ != nullptr) this->parent_->start_selected_menu();
  }
  ChunmiCooker *parent_{nullptr};
};

// Automations & Actions
template<typename... Ts> class StartAction : public Action<Ts...> {
 public:
  StartAction(ChunmiCooker *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, profile)
  void play(Ts... x) override {
    this->parent_->start_profile(this->profile_.value(x...));
  }
 protected:
  ChunmiCooker *parent_;
};

template<typename... Ts> class SetMenuAction : public Action<Ts...> {
 public:
  SetMenuAction(ChunmiCooker *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, profile)
  void play(Ts... x) override {
    this->parent_->set_menu_profile(this->profile_.value(x...));
  }
 protected:
  ChunmiCooker *parent_;
};

template<typename... Ts> class CancelAction : public Action<Ts...> {
 public:
  CancelAction(ChunmiCooker *parent) : parent_(parent) {}
  void play(Ts... x) override {
    this->parent_->cancel();
  }
 protected:
  ChunmiCooker *parent_;
};

template<typename... Ts> class SendRawAction : public Action<Ts...> {
 public:
  SendRawAction(ChunmiCooker *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, command)
  void play(Ts... x) override {
    this->parent_->send_raw_command(this->command_.value(x...));
  }
 protected:
  ChunmiCooker *parent_;
};

}  // namespace chunmi_cooker
}  // namespace esphome
