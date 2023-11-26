// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/ConfigLoaders/IsSettingSaveable.h"

#include <algorithm>
#include <vector>

#include "Common/Config/Config.h"
#include "Core/Config/AchievementSettings.h"
#include "Core/Config/GraphicsSettings.h"
#include "Core/Config/MainSettings.h"
#include "Core/Config/UISettings.h"
#include "Core/Config/WiimoteSettings.h"

namespace ConfigLoaders
{
bool IsSettingSaveable(const Config::Location& config_location)
{
  for (Config::System system :
       {Config::System::SYSCONF, Config::System::GFX, Config::System::DualShockUDPClient,
        Config::System::Logger, Config::System::FreeLook, Config::System::Main,
        Config::System::GameSettingsOnly})
  {
    if (config_location.system == system)
      return true;
  }

  static const auto s_setting_saveable = {
      // UI.General

      &Config::MAIN_USE_DISCORD_PRESENCE.GetLocation(),

      // PrimeHack

      &Config::PRIMEHACK_INITIAL_RUN.GetLocation(),
      &Config::PRIMEHACK_ENABLE.GetLocation(),
      &Config::PRIMEHACK_PROMPT_TAB.GetLocation(),
      &Config::PRIMEHACK_NOCLIP.GetLocation(),
      &Config::PRIMEHACK_INVULNERABILITY.GetLocation(),
      &Config::PRIMEHACK_SKIPMP2_PORTAL.GetLocation(),
      &Config::PRIMEHACK_SKIPPABLE_CUTSCENES.GetLocation(),
      &Config::PRIMEHACK_RESTORE_SCANDASH.GetLocation(),
      &Config::PRIMEHACK_FRIENDVOUCHERS.GetLocation(),
      &Config::PRIMEHACK_DISABLE_HUDMEMO.GetLocation(),
      &Config::PRIMEHACK_UNLOCK_HYPERMODE.GetLocation(),

      &Config::TOGGLE_ARM_REPOSITION.GetLocation(),
      &Config::AUTO_EFB.GetLocation(),
      &Config::ARMPOSITION_MODE.GetLocation(),
      &Config::ARMPOSITION_LEFTRIGHT.GetLocation(),
      &Config::ARMPOSITION_UPDOWN.GetLocation(),
      &Config::ARMPOSITION_FORWARDBACK.GetLocation(),
      &Config::TOGGLE_CULLING.GetLocation(),
      &Config::DISABLE_BLOOM.GetLocation(),
      &Config::REDUCE_BLOOM.GetLocation(),
      &Config::FOV.GetLocation(),
      &Config::ENABLE_SECONDARY_GUNFX.GetLocation(),
      &Config::GC_SHOW_CROSSHAIR.GetLocation(),
      &Config::GC_CROSSHAIR_COLOR_RGBA.GetLocation(),

      // Wiimote

      &Config::WIIMOTE_1_SOURCE.GetLocation(),
      &Config::WIIMOTE_2_SOURCE.GetLocation(),
      &Config::WIIMOTE_3_SOURCE.GetLocation(),
      &Config::WIIMOTE_4_SOURCE.GetLocation(),
      &Config::WIIMOTE_BB_SOURCE.GetLocation(),

      // Achievements

      &Config::RA_ENABLED.GetLocation(),
      &Config::RA_USERNAME.GetLocation(),
      &Config::RA_API_TOKEN.GetLocation(),
      &Config::RA_ACHIEVEMENTS_ENABLED.GetLocation(),
      &Config::RA_LEADERBOARDS_ENABLED.GetLocation(),
      &Config::RA_RICH_PRESENCE_ENABLED.GetLocation(),
      &Config::RA_PROGRESS_ENABLED.GetLocation(),
      &Config::RA_BADGES_ENABLED.GetLocation(),
      &Config::RA_UNOFFICIAL_ENABLED.GetLocation(),
      &Config::RA_ENCORE_ENABLED.GetLocation(),
  };

  return std::any_of(begin(s_setting_saveable), end(s_setting_saveable),
                     [&config_location](const Config::Location* location) {
                       return *location == config_location;
                     });
}
}  // namespace ConfigLoaders
