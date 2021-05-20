#pragma once

#include <string>
#include <vector>

#include "esphome/core/defines.h"

using std::string;
using std::vector;

namespace esphome {
namespace esp32_ble_controller {

class BLECommand {
public:
  BLECommand(const string& name, const string& description) : name(name), description(description) {}
  virtual ~BLECommand() {}

  const string get_name() const { return name; }

  const string get_description() const { return description; }

  virtual void execute(const std::vector<string>& arguments) const = 0;

private:
  string name;
  string description;
};

class BLECommandHelp : public BLECommand {
public:
  BLECommandHelp();
  virtual ~BLECommandHelp() {}

  virtual void execute(const vector<string>& arguments) const override;
};

class BLECommandSwitchServicesOnOrOff : public BLECommand {
public:
  BLECommandSwitchServicesOnOrOff();
  virtual ~BLECommandSwitchServicesOnOrOff() {}

  virtual void execute(const vector<string>& arguments) const override;
};

class BLEControllerCommandExecutionTrigger;

class BLECommandCustom : public BLECommand {
public:
  BLECommandCustom(const string& name, const string& description, BLEControllerCommandExecutionTrigger* trigger);
  virtual ~BLECommandCustom() {}

  virtual void execute(const vector<string>& arguments) const override;

private:
  BLEControllerCommandExecutionTrigger* trigger;
};

#ifdef USE_LOGGER
class BLECommandLogLevel : public BLECommand {
public:
  BLECommandLogLevel();
  virtual ~BLECommandLogLevel() {}

  virtual void execute(const vector<string>& arguments) const override;
};
#endif

} // namespace esp32_ble_controller
} // namespace esphome
