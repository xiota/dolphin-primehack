#pragma once

#include <memory>
#include <map>

#include "Core/Core.h"
#include "Core/PrimeHack/PrimeMod.h"

namespace prime {

// Determines current running game, activates enabled modifications for said game
class HackManager {
public:
  HackManager();
  void run_active_mods(const Core::CPUThreadGuard& cpu_guard);
  void update_mod_states();
  void add_mod(std::string const &name, std::unique_ptr<PrimeMod> mod);
  Game get_active_game() const { return active_game; }
  Region get_active_region() const { return active_region; }
  void disable_mod(std::string const &name);
  void enable_mod(std::string const &name);
  void disable_mod_without_notify(std::string const &name);
  void enable_mod_without_notify(std::string const &name);
  void set_mod_enabled(std::string const &name, bool enabled);
  bool is_mod_active(std::string const &name);
  void reset_mod(std::string const &name);

  void shutdown();

  PrimeMod *get_mod(std::string const& name);

private:
  Game active_game;
  Region active_region;
  Game last_game;
  Region last_region;

  std::map<std::string, std::unique_ptr<PrimeMod>> mods;
  std::map<std::string, ModState> mod_state_backup;
};

}
