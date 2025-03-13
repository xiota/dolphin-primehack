#include "Core/PrimeHack/Mods/FpsControls.h"

#include "Core/PowerPC/MMU.h"
#include "Core/PowerPC/PowerPC.h"
#include "Core/PrimeHack/Mods/ContextSensitiveControls.h"
#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/System.h"

#include "Common/Timer.h"

#include <cmath>

namespace prime {
namespace {
const std::array<int, 4> prime_one_beams = {0, 2, 1, 3};
const std::array<int, 4> prime_two_beams = {0, 1, 2, 3};

const std::array<std::tuple<int, int>, 4> prime_one_visors = {
  std::make_tuple<int, int>(0, 0x11), std::make_tuple<int, int>(2, 0x05),
  std::make_tuple<int, int>(3, 0x09), std::make_tuple<int, int>(1, 0x0d)};
const std::array<std::tuple<int, int>, 4> prime_two_visors = {
  std::make_tuple<int, int>(0, 0x08), std::make_tuple<int, int>(2, 0x09),
  std::make_tuple<int, int>(3, 0x0a), std::make_tuple<int, int>(1, 0x0b)};
const std::array<std::tuple<int, int>, 4> prime_three_visors = {
  std::make_tuple<int, int>(0, 0x0b), std::make_tuple<int, int>(1, 0x0c),
  std::make_tuple<int, int>(2, 0x0d), std::make_tuple<int, int>(3, 0x0e)};

constexpr u32 ORBIT_STATE_GRAPPLE = 5;
#define RIDLEY_STR(r) (r == Region::NTSC_J ? L"メタリドリー" : L"Meta Ridley")
#define RIDLEY_STR_LEN(r) (r == Region::NTSC_J ? 6 : 11)
}

bool FpsControls::is_string_ridley(Region active_region, u32 string_base) {
  if (string_base == 0) {
    return false;
  }

  const wchar_t * ridley_str = RIDLEY_STR(active_region);
  const auto str_len = RIDLEY_STR_LEN(active_region);
  int str_idx = 0;

  while (read16(string_base) != 0 && str_idx < str_len) {
    if (static_cast<wchar_t>(read16(string_base)) != ridley_str[str_idx]) {
      return false;
    }
    str_idx++;
    string_base += 2;
  }
  return str_idx == str_len && read16(string_base) == 0;
}

void FpsControls::run_mod(Game game, Region region) {
  switch (game) {
  case Game::MENU:
  case Game::MENU_PRIME_1:
  case Game::MENU_PRIME_2:
    run_mod_menu(game, region);
    break;
  case Game::PRIME_1:
    run_mod_mp1(region);
    break;
  case Game::PRIME_2:
    run_mod_mp2(region);
    break;
  case Game::PRIME_3:
  case Game::PRIME_3_STANDALONE:
    run_mod_mp3(game, region);
    break;
  case Game::PRIME_1_GCN:
  case Game::PRIME_1_GCN_R1:
  case Game::PRIME_1_GCN_R2:
    run_mod_mp1_gc(region);
    break;
  case Game::PRIME_2_GCN:
    run_mod_mp2_gc(region);
    break;
  default:
    break;
  }
}

bool FpsControls::input_disabled() const {
  LOOKUP_DYN(menu_state);
  return read32(menu_state) != 0;
}

void FpsControls::calculate_pitch_delta() {
  if (input_disabled()) {
    return;
  }
  const float compensated_sens = GetSensitivity() * kTurnrateRatio / 60.f;

  if (CheckPitchRecentre()) {
    calculate_pitch_to_target(0.f);
    return;
  } else {
    // Cancel any interpolation that was taking place.
    interpolating = false;
  }

  pitch += static_cast<float>(GetVerticalAxis()) * compensated_sens *
    (InvertedY() ? 1.f : -1.f);
  pitch = std::clamp(pitch, -1.52f, 1.52f);
}

void FpsControls::calculate_pitchyaw_delta() {
  if (input_disabled()) {
    return;
  }
  constexpr auto yaw_clamp = [](float t) -> float {
    constexpr float PI = 3.141592654f;
    constexpr float TWO_PI = PI * 2.f;
    return (t > PI) ? (t - TWO_PI) : ((t < -PI) ? (t + TWO_PI) : (t));
  };

  const float compensated_sens = GetSensitivity() * kTurnrateRatio / 60.f;

  pitch += static_cast<float>(GetVerticalAxis()) * compensated_sens *
    (InvertedY() ? 1.f : -1.f);
  pitch = std::clamp(pitch, -1.52f, 1.52f);

  yaw += static_cast<float>(GetHorizontalAxis()) * compensated_sens *
    (InvertedX() ? 1.f : -1.f);
  yaw = yaw_clamp(yaw);
}

void FpsControls::calculate_pitch_locked(Game game, Region region) {
  // Calculate the pitch based on the XF matrix to allow us to write out the pitch
  // even while locked onto a target, the pitch will be written to match the lock
  // angle throughout the entire lock-on. The very first frame when the lock is
  // released (and before this mod has run again) the game will still render the
  // correct angle. If we stop writing the angle during lock-on then a 1-frame snap
  // occurs immediately after releasing the lock, due to the mod running after the
  // frame has already been rendered.
  LOOKUP_DYN(object_list);
  LOOKUP_DYN(camera_manager);
  const u16 camera_uid = read16(camera_manager);
  if (camera_uid == 0xffff) {
    return;
  }
  const u32 camera = read32(object_list + ((camera_uid & 0x3ff) << 3) + 4);
  u32 camera_xf_offset = 0;

  switch (game) {
    case Game::PRIME_1:
      camera_xf_offset = 0x2c;
      break;
    case Game::PRIME_1_GCN:
      camera_xf_offset = 0x34;
      break;
    case Game::PRIME_2:
      camera_xf_offset = 0x20;
      break;
    case Game::PRIME_2_GCN:
      camera_xf_offset = 0x24;
      break;
    case Game::PRIME_3:
    case Game::PRIME_3_STANDALONE:
      camera_xf_offset = 0x3c;
      break;
  }

  Transform camera_tf;
  camera_tf.read_from(*active_guard, camera + camera_xf_offset);
  pitch = asin(camera_tf.fwd().z);
  pitch = std::clamp(pitch, -1.52f, 1.52f);
}

void FpsControls::calculate_pitch_to_target(float target_pitch)
{
  // Smoothly transitions pitch to target through interpolation

  const float margin = 0.05f;
  if (pitch >= (target_pitch - margin) && pitch <= (target_pitch + margin)) {
    pitch = target_pitch;
    interpolating = false;

    return;
  }

  if (!interpolating) {
    delta = 0;
    start_pitch = pitch;
    interpolating = true;
  }

  pitch = Lerp(start_pitch, target_pitch, delta / 15.f);
  pitch = std::clamp(pitch, -1.52f, 1.52f);

  delta++;

  return;
}

float FpsControls::calculate_yaw_vel() {
  return GetHorizontalAxis() * GetSensitivity() * (InvertedX() ? 1.f : -1.f);;
}

void FpsControls::handle_beam_visor_switch(std::array<int, 4> const &beams,
  std::array<std::tuple<int, int>, 4> const &visors) {
  // Global array of all powerups (measured in "ammunition"
  // even for things like visors/beams)
  LOOKUP_DYN(powerups_array);
  LOOKUP(powerups_size);
  LOOKUP(powerups_offset);

  // We copy out the ownership status of beams and visors to our own array for
  // get_beam_switch and get_visor_switch
  for (int i = 0; i < 4; i++) {
    const bool visor_owned =
      read32(powerups_array + std::get<1>(visors[i]) *
        powerups_size + powerups_offset) ? true : false;
    set_visor_owned(i, visor_owned);
    if (has_beams) {
      const bool beam_owned =
        read32(powerups_array + beams[i] *
          powerups_size + powerups_offset) ? true : false;
      set_beam_owned(i, beam_owned);
    }
  }

  if (has_beams) {
    const int beam_id = get_beam_switch(beams);
    if (beam_id != -1) {
      // Prevent triggering more than one beam swap while one is currently in progress.
      std::chrono::duration<double, std::milli> elapsed = hr_clock::now() - beam_scroll_timeout;
      if (elapsed.count() > 800) {
        prime::GetVariableManager()->set_variable(*active_guard, "new_beam", static_cast<u32>(beam_id));
        prime::GetVariableManager()->set_variable(*active_guard, "beamchange_flag", u32{1});
        beam_scroll_timeout = hr_clock::now();
      }
    }
  }

  LOOKUP_DYN(active_visor);
  int visor_id, visor_off;
  std::tie(visor_id, visor_off) = get_visor_switch(visors,
    read32(active_visor) == 0);

  if (visor_id != -1) {
    if (read32(powerups_array + (visor_off * powerups_size) + powerups_offset)) {
      write32(visor_id, active_visor);

      // Trigger holster animation.
      // Prime 3 already animates holstering. Gamecube does not need to use the visor controls.
      if (visor_id == 2) {
        auto active_game = GetHackManager()->get_active_game();
        if (active_game == Game::PRIME_1 || active_game == Game::PRIME_2) {
          LOOKUP_DYN(gun_holster_state);
          LOOKUP(holster_timer_offset);

          write32(3, gun_holster_state);
          writef32(0.2f, gun_holster_state + holster_timer_offset); // Holster timer
        }
      }
    }
  }

  DevInfo("powerups_array", "%08X", powerups_array);
}

void FpsControls::run_mod_menu(Game game, Region region) {
  if (region == Region::NTSC_U) {
    u32 p0 = read32(Core::System::GetInstance().GetPPCState().gpr[13] - 0x2870);
    if (!mem_check(p0)) {
      return;
    }
    p0 = read32(p0 + 0xc54) + 0x9c;
    if (!mem_check(p0)) {
      return;
    }

    handle_cursor(*active_guard, p0, p0 + 0xc0, region);
  } else if (region == Region::NTSC_J) {
    if (game == Game::MENU_PRIME_1) {
      handle_cursor(*active_guard, 0x805a7da8, 0x805a7dac, region);
    }
    if (game == Game::MENU_PRIME_2) {
      handle_cursor(*active_guard, 0x805a7ba8, 0x805a7bac, region);
    }
  } else if (region == Region::PAL) {
    u32 cursor_address = read32(0x80621ffc);
    handle_cursor(*active_guard, cursor_address + 0xdc, cursor_address + 0x19c, region);
  }
}

void FpsControls::run_mod_mp1(Region region) {
  LOOKUP_DYN(player);
  if (player == 0) {
    return;
  }
  DevInfo("Player", "%08x", player);

  handle_beam_visor_switch(prime_one_beams, prime_one_visors);
  CheckBeamVisorSetting(Game::PRIME_1);

  // Is beam/visor menu showing on screen
  LOOKUP_DYN(beamvisor_menu_state);
  bool beamvisor_menu_enabled = read32(beamvisor_menu_state) == 1;

  LOOKUP_DYN(orbit_state);
  LOOKUP_DYN(lockon_state);

  // Allows freelook in grapple, otherwise we are orbiting (locked on) to something
  bool locked = (read32(orbit_state) != ORBIT_STATE_GRAPPLE &&
    read8(lockon_state) || beamvisor_menu_enabled);


  LOOKUP_DYN(cursor);
  LOOKUP_DYN(angular_vel);
  LOOKUP_DYN(angular_momentum);
  LOOKUP_DYN(firstperson_pitch);
  LOOKUP(arm_cannon_matrix);
  if (locked) {
    write32(0, angular_momentum);
    write32(0, angular_vel);
    calculate_pitch_locked(Game::PRIME_1, region);
    writef32(FpsControls::pitch, firstperson_pitch);
    writef32(FpsControls::pitch, arm_cannon_matrix);

    if (beamvisor_menu_enabled) {
      LOOKUP_DYN(beamvisor_menu_mode);
      // if the menu id is not null
      if (read32(beamvisor_menu_mode) != 0xFFFFFFFF) {
        if (menu_open == false) {
          set_code_group_state("beam_change", ModState::DISABLED);
        }

        handle_reticle(*active_guard, cursor + 0x9c, cursor + 0x15c, region, GetFov());
        menu_open = true;
      }
    } else if (HandleReticleLockOn()) {  // If we handle menus, this doesn't need to be ran
      handle_reticle(*active_guard, cursor + 0x9c, cursor + 0x15c, region, GetFov());
    }
  } else {
    if (menu_open) {
      set_code_group_state("beam_change", ModState::ENABLED);
      menu_open = false;
    }

    set_cursor_pos(0, 0);
    write32(0, cursor + 0x9c);
    write32(0, cursor + 0x15c);

    calculate_pitch_delta();
    writef32(FpsControls::pitch, firstperson_pitch);
    writef32(FpsControls::pitch, arm_cannon_matrix);

    LOOKUP(tweakplayer);
    // Max pitch angle, as abs val (any higher = gimbal lock)
    writef32(1.52f, tweakplayer + 0x134);

    write32(0, angular_vel);
    LOOKUP_DYN(ball_state);
    if (read32(ball_state) == 0) {
      writef32(calculate_yaw_vel(), angular_momentum);
    }

    LOOKUP_DYN(menu_state);
    swap_alt_profiles(read32(ball_state), read32(menu_state));
  }
}

void FpsControls::run_mod_mp1_gc(Region region) {
  const bool show_crosshair = GetShowGCCrosshair();
  const u32 crosshair_color_rgba = show_crosshair ? GetGCCrosshairColor() : 0x4b7ea331;
  set_code_group_state("show_crosshair", show_crosshair ? ModState::ENABLED : ModState::DISABLED);
  LOOKUP(crosshair_color);
  if (show_crosshair) {
    write32(crosshair_color_rgba, crosshair_color);
  }

  LOOKUP_DYN(player);
  if (player == 0) {
    return;
  }
  DevInfo("Player", "%08x", player);

  LOOKUP_DYN(player_xf);
  Transform cplayer_xf;
  cplayer_xf.read_from(*active_guard, player_xf);
  LOOKUP_DYN(orbit_state);
  const u32 orbit_state_val = read32(orbit_state);
  if (orbit_state_val != ORBIT_STATE_GRAPPLE &&
    orbit_state_val != 0) {
    calculate_pitch_locked(Game::PRIME_1_GCN, region);
    LOOKUP_DYN(firstperson_pitch);
    writef32(FpsControls::pitch, firstperson_pitch);

    vec3 fwd = cplayer_xf.fwd();
    yaw = atan2f(fwd.y, fwd.x);

    return;
  }

  LOOKUP_DYN(camera_state);
  if (read32(camera_state) != 0) {
    vec3 fwd = cplayer_xf.fwd();
    yaw = atan2f(fwd.y, fwd.x);
    return;
  }

  calculate_pitchyaw_delta();
  LOOKUP(tweak_player);
  LOOKUP(grapple_swing_speed_offset);
  LOOKUP_DYN(firstperson_pitch);
  LOOKUP_DYN(angular_vel);
  writef32(FpsControls::pitch, firstperson_pitch);
  writef32(1.52f, tweak_player + 0x134);
  writef32(0, angular_vel);
  cplayer_xf.build_rotation(yaw);
  cplayer_xf.write_to(*active_guard, player_xf);

  for (int i = 0; i < 8; i++) {
    writef32(0, (tweak_player + 0x84) + i * 4);
    writef32(0, (tweak_player + 0x84) + i * 4 - 32);
  }
  writef32(1000.f, tweak_player + grapple_swing_speed_offset);

  LOOKUP_DYN(freelook_rotation_speed);
  LOOKUP_DYN(air_transitional_friction);

  // Freelook rotation speed tweak
  write32(0x4f800000, freelook_rotation_speed);
  // Air translational friction changes to make diagonal strafe match normal speed
  writef32(0.25f, air_transitional_friction);
}

void FpsControls::run_mod_mp2(Region region) {
  CheckBeamVisorSetting(Game::PRIME_2);

  // VERY similar to mp1, this time CPlayer isn't TOneStatic (presumably because
  // of multiplayer mode in the GCN version?)
  LOOKUP_DYN(player);
  if (player == 0) {
    return;
  }
  DevInfo("Player", "%08x", player);

  LOOKUP_DYN(load_state);
  if (read32(load_state) != 1) {
    return;
  }

  handle_beam_visor_switch(prime_two_beams, prime_two_visors);

  // Is beam/visor menu showing on screen
  LOOKUP_DYN(beamvisor_menu_state);
  bool beamvisor_menu = read32(beamvisor_menu_state) == 1;

  // Allows freelook in grapple, otherwise we are orbiting (locked on) to something
  LOOKUP_DYN(orbit_state);
  LOOKUP_DYN(lockon_state);
  bool locked = (read32(orbit_state) != ORBIT_STATE_GRAPPLE &&
    read8(lockon_state) || beamvisor_menu);

  LOOKUP_DYN(cursor);
  LOOKUP_DYN(angular_momentum);
  if (locked) {
    // Angular velocity (not really, but momentum) is being messed with like mp1
    // just being accessed relative to cplayer
    LOOKUP_DYN(firstperson_pitch);
    write32(0, angular_momentum);
    calculate_pitch_locked(Game::PRIME_2, region);
    writef32(FpsControls::pitch, firstperson_pitch);

    if (beamvisor_menu) {
      LOOKUP_DYN(beamvisor_menu_mode);
      u32 mode = read32(beamvisor_menu_mode);

      // if the menu id is not null
      if (mode != 0xFFFFFFFF) {
        if (menu_open == false) {
          set_code_group_state("beam_change", ModState::DISABLED);
        }

        handle_reticle(*active_guard, cursor + 0x9c, cursor + 0x15c, region, GetFov());
        menu_open = true;
      }
    } else if (HandleReticleLockOn()) {
      handle_reticle(*active_guard, cursor + 0x9c, cursor + 0x15c, region, GetFov());
    }
  } else {
    if (menu_open) {
      set_code_group_state("beam_change", ModState::ENABLED);

      menu_open = false;
    }

    set_cursor_pos(0, 0);
    write32(0, cursor + 0x9c);
    write32(0, cursor + 0x15c);

    calculate_pitch_delta();
    // Grab the arm cannon address, go to its transform field (NOT the
    // Actor's xf @ 0x30!!)
    LOOKUP_DYN(firstperson_pitch);
    writef32(FpsControls::pitch, firstperson_pitch);

    // For whatever god forsaken reason, writing pitch to the z component of the
    // right vector for this xf makes the gun not lag. Won't fix what ain't broken
    LOOKUP_DYN(armcannon_matrix);
    writef32(FpsControls::pitch, armcannon_matrix + 0x24);

    LOOKUP(tweak_player_offset);
    u32 tweak_player_address = read32(read32(Core::System::GetInstance().GetPPCState().gpr[13] + tweak_player_offset));
    if (mem_check(tweak_player_address)) {
      // This one's stored as degrees instead of radians
      writef32(87.0896f, tweak_player_address + 0x180);
    }

    LOOKUP_DYN(ball_state);
    if (read32(ball_state) == 0) {
      writef32(calculate_yaw_vel(), angular_momentum);
    }

    // Nothing new here
    write32(0, angular_momentum + 0x18);

    LOOKUP_DYN(menu_state);
    swap_alt_profiles(read32(ball_state), read32(menu_state));
  }
}

void null_players_on_destruct_mp2_gc(PowerPC::PowerPCState& ppc_state, PowerPC::MMU& mmu, u32) {
  // r27 is an iterator variable pointing to start of statemgr
  mmu.Write_U32(0, ppc_state.gpr[27] + 0x14fc);

  // Original instruction: addi r27, r27, 4
  ppc_state.gpr[27] += 4;
}

void FpsControls::run_mod_mp2_gc(Region region) {
  LOOKUP_DYN(world);
  if (world == 0) {
    return;
  }
  // World loading phase == 4 -> complete
  if (read32(world + 0x4) != 4) {
    return;
  }

  LOOKUP_DYN(player);
  if (player == 0) {
    return;
  }
  DevInfo("Player", "%08x", player);

  const bool show_crosshair = GetShowGCCrosshair();
  const u32 crosshair_color_rgba = show_crosshair ? GetGCCrosshairColor() : 0x4b7ea331;
  set_code_group_state("show_crosshair", show_crosshair ? ModState::ENABLED : ModState::DISABLED);
  LOOKUP(tweakgui_offset);
  u32 crosshair_color_addr = read32(read32(Core::System::GetInstance().GetPPCState().gpr[13] + tweakgui_offset)) + 0x268;
  if (show_crosshair) {
    write32(crosshair_color_rgba, crosshair_color_addr);
  }

  LOOKUP_DYN(player_xf);
  Transform cplayer_xf;
  cplayer_xf.read_from(*active_guard, player_xf);
  LOOKUP_DYN(orbit_state);
  LOOKUP_DYN(firstperson_pitch);
  if (read32(orbit_state) != ORBIT_STATE_GRAPPLE &&
      read32(orbit_state) != 0) {
    calculate_pitch_locked(Game::PRIME_2_GCN, region);
    writef32(FpsControls::pitch, firstperson_pitch);

    vec3 fwd = cplayer_xf.fwd();
    yaw = atan2f(fwd.y, fwd.x);

    return;
  }

  LOOKUP(tweak_player_offset);
  const u32 tweak_player_address = read32(read32(Core::System::GetInstance().GetPPCState().gpr[13] + tweak_player_offset));
  if (mem_check(tweak_player_address)) {
    // Freelook rotation speed tweak
    write32(0x4f800000, tweak_player_address + 0x188);
    // Freelook pitch half-angle range tweak
    writef32(87.0896f, tweak_player_address + 0x184);
    // Air translational friction changes to make diagonal strafe match normal speed
    writef32(0.25f, tweak_player_address + 0x88);
    for (int i = 0; i < 8; i++) {
      writef32(100000000.f, tweak_player_address + 0xc4 + i * 4);
      writef32(1.f, tweak_player_address + 0xa4 + i * 4);
    }
  }

  LOOKUP_DYN(ball_state);
  if (read32(ball_state) == 0) {
    calculate_pitchyaw_delta();
    writef32(FpsControls::pitch, firstperson_pitch);
    cplayer_xf.build_rotation(yaw);
    cplayer_xf.write_to(*active_guard, player_xf);
  }
}

void FpsControls::mp3_handle_lasso(u32 grapple_state_addr) {
  if (GrappleCtlBound()) {
    set_code_group_state("grapple_lasso", ModState::ENABLED);

    // Disable animation code changes if trying to use grapple voltage.
    set_code_group_state("grapple_lasso_animation",
      CheckForward() || CheckBack() ? ModState::DISABLED : ModState::ENABLED);
  }
  else {
    set_code_group_state("grapple_lasso", ModState::DISABLED);
    set_code_group_state("grapple_lasso_animation", ModState::DISABLED);
  }

  // This is outside of the grapple detection so that people can quickly tap the bind to pull rather than wait.
  bool holding_grapple = prime::CheckGrappleCtl();

  // If currently locked onto a grapple point. This must be seperate from lock-on for grapple swing.
  // 1 => Locked On
  // 2 => Grapple Lasso(/Voltage)
  // 3 => Grapple Swing
  if (read32(grapple_state_addr) == 2) {
    if (holding_grapple && !grapple_button_state) {
      grapple_button_state = true;

      grapple_swap_axis = !grapple_swap_axis;

      // 0.45 for repeated taps. 1 will instantly complete the grapple.
      if (GrappleTappingMode())
        grapple_force += 0.40f;
    }
    else if (!holding_grapple) {
      grapple_button_state = false;

      if (!GrappleTappingMode())
        grapple_force += 1.f;
    }

    constexpr float force_delta = 0.045f;

    // Use tapping/force method
    if (grapple_force > 0) {
      grapple_hand_pos += force_delta;
      grapple_force -= force_delta;
    }
    else {
      grapple_hand_pos -= 0.045f;
      grapple_force = 0;
    }

    grapple_hand_pos = std::clamp(grapple_hand_pos, 0.f, 1.0f);

    prime::GetVariableManager()->set_variable(*active_guard, "grapple_hand_x", grapple_hand_pos);
    prime::GetVariableManager()->set_variable(*active_guard, "grapple_hand_y", grapple_hand_pos / 4);

    if (grapple_hand_pos >= 1.0f) {
      // State 4 completes the grapple (e.g pull door from frame)
      prime::GetVariableManager()->set_variable(*active_guard, "grapple_lasso_state", (u32) 4);
    } else {
      // State 2 "holds" the grapple for lasso/voltage.
      prime::GetVariableManager()->set_variable(*active_guard, "grapple_lasso_state", (u32) 2);
    }
  } else {
    prime::GetVariableManager()->set_variable(*active_guard, "grapple_hand_x", 0.f);
    prime::GetVariableManager()->set_variable(*active_guard, "grapple_hand_y", 0.f);
    prime::GetVariableManager()->set_variable(*active_guard, "grapple_lasso_state", (u32) 0);

    grapple_hand_pos = 0;
    grapple_force = 0;
    grapple_button_state = false;
  }
}

// this game is
void FpsControls::run_mod_mp3(Game active_game, Region active_region) {
  CheckBeamVisorSetting(active_game);

  if (GrappleCtlBound()) {
    set_code_group_state("grapple_lasso", ModState::ENABLED);

    // Disable animation code changes if trying to use grapple voltage.
    set_code_group_state("grapple_lasso_animation",
      CheckForward() || CheckBack() ? ModState::DISABLED : ModState::ENABLED);
  }
  else {
    set_code_group_state("grapple_lasso", ModState::DISABLED);
    set_code_group_state("grapple_lasso_animation", ModState::DISABLED);
  }

  LOOKUP_DYN(cursor);
  const auto mp3_handle_cursor = [this, cursor, active_region] (bool locked, bool for_reticle) {
    if (locked) {
      write32(0, cursor + 0x9c);
      write32(0, cursor + 0x15c);
    } else if (for_reticle) {
      handle_reticle(*active_guard, cursor + 0x9c, cursor + 0x15c, active_region, GetFov());
    } else {
      handle_cursor(*active_guard, cursor + 0x9c, cursor + 0x15c, active_region);
    }

  };

  LOOKUP(state_manager);
  LOOKUP_DYN(ball_state);
  LOOKUP_DYN(menu_state);

  swap_alt_profiles(read32(ball_state), read32(menu_state));

  // Handles menu screen cursor
  LOOKUP(cursor_dlg_enabled);
  if (read8(cursor_dlg_enabled)) {
    mp3_handle_cursor(false, false);
    return;
  }

  // In NTSC-J version there is a quiz to select the difficulty
  // This checks if we are ingame
  // I won't add (state_manager + 0x29C) to the address db, not sure what it is
  if (active_region == Region::NTSC_J && read32(state_manager + 0x29C) == 0xffffffff) {
    mp3_handle_cursor(false, false);
    return;
  }

  LOOKUP_DYN(player);
  if (player == 0) {
    return;
  }
  DevInfo("Player", "%08x", player);

  handle_beam_visor_switch({}, prime_three_visors);

  LOOKUP_DYN(boss_name);
  LOOKUP_DYN(boss_status);
  bool is_boss_metaridley = is_string_ridley(active_region, boss_name);

  // Compare based on boss name string, Meta Ridley only appears once
  if (is_boss_metaridley) {
    // If boss is dead
    if (read8(boss_status) == 8) {
      set_state(ModState::ENABLED);
      mp3_handle_cursor(true, true);
    } else {
      set_state(ModState::CODE_DISABLED);
      mp3_handle_cursor(false, true);
      return;
    }
  }

  prime::GetVariableManager()->set_variable(*active_guard, "trigger_grapple", prime::CheckGrappleCtl() ? u32{1} : u32{0});

  LOOKUP_DYN(firstperson_pitch);
  LOOKUP_DYN(angular_momentum);
  LOOKUP_DYN(beamvisor_menu_state);
  LOOKUP_DYN(lockon_type);
  LOOKUP(lockon_state);
  bool beamvisor_menu = read32(beamvisor_menu_state) == 3;
  if ((read32(lockon_type) == 0 && read8(lockon_state)) || read32(lockon_type) == 1 || beamvisor_menu) {
    write32(0, angular_momentum);
    calculate_pitch_locked(active_game, active_region);

    if (HandleReticleLockOn() || beamvisor_menu) {
      mp3_handle_cursor(false, true);
    }

    writef32(FpsControls::pitch, firstperson_pitch);

    return;
  }

  // Handle grapple lasso bind
  mp3_handle_lasso(lockon_type);

  // Lock Camera according to ContextSensitiveControls and interpolate to pitch 0
  if (prime::GetLockCamera() != Unlocked) {
    const float target_pitch = Centre == prime::GetLockCamera() ? 0.f : 0.23f;

    if (FpsControls::pitch == target_pitch) {
      writef32(target_pitch, pitch);
      mp3_handle_cursor(false, false);

      return;
    }

    calculate_pitch_to_target(target_pitch);
    writef32(FpsControls::pitch, firstperson_pitch);

    // Known edge-case, if a user were to exit a camera lock without reaching
    // the target (e.g loadstate), the next time they enter a camera lock,
    // it will attempt to centre to the last target pitch.
    // We cannot reset here due to `Reset Camera Pitch`

    return;
  }


  mp3_handle_cursor(true, true);
  set_cursor_pos(0, 0);

  calculate_pitch_delta();
  // Gun damping uses its own TOC value, so screw it (I checked the binary)
  // Byte pattern to find the offset : c0?2???? ec23082a fc60f850
  LOOKUP(gun_lag_toc_offset);
  u32 rtoc_gun_damp = Core::System::GetInstance().GetPPCState().gpr[2] + gun_lag_toc_offset;
  write32(0, rtoc_gun_damp);
  writef32(FpsControls::pitch, firstperson_pitch);

  if (read32(ball_state) == 0) {
    writef32(calculate_yaw_vel(), angular_momentum);
  }

  // Nothing new here
  write32(0, angular_momentum + 0x18);
}

void FpsControls::CheckBeamVisorSetting(Game game)
{
  bool beam,visor;
  std::tie<bool, bool>(beam, visor) = GetMenuOptions();

  switch (game) {
  case Game::PRIME_1:
  case Game::PRIME_2:
    set_code_group_state("beam_menu", beam ? ModState::DISABLED : ModState::ENABLED);
  case Game::PRIME_3:
  case Game::PRIME_3_STANDALONE:
    set_code_group_state("visor_menu", visor ? ModState::DISABLED : ModState::ENABLED);
    break;
  default:
    break;
  }
}

bool FpsControls::init_mod(Game game, Region region) {
  swap_alt_profiles(0, 0);

  switch (game) {
  case Game::MENU_PRIME_1:
  case Game::MENU_PRIME_2:
    init_mod_menu(game, region);
    break;
  case Game::PRIME_1:
    init_mod_mp1(region);
    break;
  case Game::PRIME_1_GCN:
    init_mod_mp1_gc(region);
    break;
  case Game::PRIME_1_GCN_R1:
    init_mod_mp1_gc_r1();
    break;
  case Game::PRIME_1_GCN_R2:
    init_mod_mp1_gc_r2();
    break;
  case Game::PRIME_2:
    init_mod_mp2(region);
    break;
  case Game::PRIME_2_GCN:
    init_mod_mp2_gc(region);
    break;
  case Game::PRIME_3:
    init_mod_mp3(region);
    break;
  case Game::PRIME_3_STANDALONE:
    init_mod_mp3_standalone(region);
    break;
  default:
    break;
  }
  return true;
}

void FpsControls::add_beam_change_code_mp1(u32 start_point) {
  u32 bcf_lis, bcf_ori;
  std::tie(bcf_lis, bcf_ori) = prime::GetVariableManager()->make_lis_ori(4, "beamchange_flag");
  u32 nb_lis, nb_ori;
  std::tie(nb_lis, nb_ori) = prime::GetVariableManager()->make_lis_ori(5, "new_beam");
  add_code_change(start_point + 0x00, bcf_lis, "beam_change");     //                        ; set r4 to beam change base address
  add_code_change(start_point + 0x04, bcf_ori, "beam_change");     //                        ;
  add_code_change(start_point + 0x08, 0x80640000, "beam_change");  // lwz    r3, 0(r4)       ; grab flag
  add_code_change(start_point + 0x0c, 0x2c030000, "beam_change");  // cmpwi  r3, 0           ; check if beam should change
  add_code_change(start_point + 0x10, 0x41820058, "beam_change");  // beq    0x58            ; don't attempt beam change if 0
  add_code_change(start_point + 0x14, nb_lis, "beam_change");      //                        ; set r5 to new beam base address
  add_code_change(start_point + 0x18, nb_ori, "beam_change");      //                        ;
  add_code_change(start_point + 0x1c, 0x83450000, "beam_change");  // lwz    r26, 0(r5)      ; get expected beam (r25, r26 used to assign beam)
  add_code_change(start_point + 0x20, 0x7f59d378, "beam_change");  // mr     r25, r26        ; copy expected beam to other reg
  add_code_change(start_point + 0x24, 0x38600000, "beam_change");  // li     r3, 0           ; reset flag
  add_code_change(start_point + 0x28, 0x90640000, "beam_change");  // stw    r3, 0(r4)       ;
  add_code_change(start_point + 0x2c, 0x4800003c, "beam_change");  // b      0x3c            ; jump forward to beam assign
}

void FpsControls::add_beam_change_code_mp2(u32 start_point) {
  u32 bcf_lis, bcf_ori;
  std::tie(bcf_lis, bcf_ori) = prime::GetVariableManager()->make_lis_ori(4, "beamchange_flag");
  u32 nb_lis, nb_ori;
  std::tie(nb_lis, nb_ori) = prime::GetVariableManager()->make_lis_ori(5, "new_beam");
  add_code_change(start_point + 0x00, bcf_lis, "beam_change");     //                        ; set r4 to beam change base address
  add_code_change(start_point + 0x04, bcf_ori, "beam_change");     //                        ;
  add_code_change(start_point + 0x08, 0x80640000, "beam_change");  // lwz    r3, 0(r4)       ; grab flag
  add_code_change(start_point + 0x0c, 0x2c030000, "beam_change");  // cmpwi  r3, 0           ; check if beam should change
  add_code_change(start_point + 0x10, 0x4182005c, "beam_change");  // beq    0x5c            ; don't attempt beam change if 0
  add_code_change(start_point + 0x14, nb_lis, "beam_change");      //                        ; set r5 to new beam base address
  add_code_change(start_point + 0x18, nb_ori, "beam_change");      //                        ;
  add_code_change(start_point + 0x1c, 0x83e50000, "beam_change");  // lwz    r31, 0(r5)      ; get expected beam (r31, r30 used to assign beam)
  add_code_change(start_point + 0x20, 0x7ffefb78, "beam_change");  // mr     r30, r31        ; copy expected beam to other reg
  add_code_change(start_point + 0x24, 0x38600000, "beam_change");  // li     r3, 0           ; reset flag
  add_code_change(start_point + 0x28, 0x90640000, "beam_change");  // stw    r3, 0(r4)       ;
  add_code_change(start_point + 0x2c, 0x48000040, "beam_change");  // b      0x40            ; jump forward to beam assign
}

void FpsControls::add_grapple_slide_code_mp3(u32 start_point) {
  add_code_change(start_point + 0x00, 0x60000000);  // nop                  ; trashed because useless
  add_code_change(start_point + 0x04, 0x60000000);  // nop                  ; trashed because useless
  add_code_change(start_point + 0x08, 0x60000000);  // nop                  ; trashed because useless
  add_code_change(start_point + 0x0c, 0xc0010240);  // lfs  f0, 0x240(r1)   ; grab the x component of new origin
  add_code_change(start_point + 0x10, 0xd01f0048);  // stfs f0, 0x48(r31)   ; store it into player's xform x origin (CTransform + 0x0c)
  add_code_change(start_point + 0x14, 0xc0010244);  // lfs  f0, 0x244(r1)   ; grab the y component of new origin
  add_code_change(start_point + 0x18, 0xd01f0058);  // stfs f0, 0x58(r31)   ; store it into player's xform y origin (CTransform + 0x1c)
  add_code_change(start_point + 0x1c, 0xc0010248);  // lfs  f0, 0x248(r1)   ; grab the z component of new origin
  add_code_change(start_point + 0x20, 0xd01f0068);  // stfs f0, 0x68(r31)   ; store it into player's xform z origin (CTransform + 0xcc)
  add_code_change(start_point + 0x28, 0x389f003c);  // addi r4, r31, 0x3c   ; next sub call is SetTransform, so set player's transform
                                                    //                      ; to their own transform (safety no-op, does other updating too)
}

void FpsControls::add_grapple_lasso_code_mp3(u32 func1, u32 func2, u32 func3) {
  u32 lis_x, ori_x, lis_y, ori_y;

  std::tie<u32, u32>(lis_x, ori_x) = prime::GetVariableManager()->make_lis_ori(11, "grapple_hand_x");
  std::tie<u32, u32>(lis_y, ori_y) = prime::GetVariableManager()->make_lis_ori(11, "grapple_hand_y");

  // Controls how tight the lasso is and if to turn yellow.
  add_code_change(func1, lis_x, "grapple_lasso");
  add_code_change(func1 + 0x4, ori_x, "grapple_lasso");
  add_code_change(func1 + 0xC, 0xC04B0000, "grapple_lasso");  // lfs f2, 0(r11)

                                                              // Skips the game's checks and lets us control grapple lasso ourselves.
  add_code_change(func1 + 0x8, 0x40800158, "grapple_lasso"); // first conditional branch changed to jmp to end
  add_code_change(func1 + 0x18, 0x40810148, "grapple_lasso"); // second conditional branch changed to jmp to end
  add_code_change(func1 + 0x54, 0x4800010C, "grapple_lasso"); // end of 'yellow' segment jmp to end

                                                              // Controls the pulling animation.
  add_code_change(func2, lis_x, "grapple_lasso_animation");
  add_code_change(func2 + 0x4, ori_x, "grapple_lasso_animation");
  add_code_change(func2 + 0x8, 0xC00B0000, "grapple_lasso_animation");  // lfs f0, 0(r11)
  add_code_change(func2 + 0xC, lis_y, "grapple_lasso_animation");
  add_code_change(func2 + 0x10, ori_y, "grapple_lasso_animation");
  add_code_change(func2 + 0x14, 0xC04B0000, "grapple_lasso_animation");  // lfs f2, 0(r11)
  add_code_change(func2 + 0x18, 0xD05701C8, "grapple_lasso_animation");  // stfs f2, 0x1C8(r23)

  u32 lis, ori;
  // Controls the return value of the "ProcessGrappleLasso" function.
  std::tie<u32, u32>(lis, ori) = prime::GetVariableManager()->make_lis_ori(30, "grapple_lasso_state");
  add_code_change(func1 + 0x160, lis, "grapple_lasso");
  add_code_change(func1 + 0x164, ori, "grapple_lasso");
  add_code_change(func1 + 0x168, 0x83DE0000, "grapple_lasso"); // lwz r30, 0(r30)

                                                               // Triggers grapple.
  std::tie<u32, u32>(lis, ori) = prime::GetVariableManager()->make_lis_ori(3, "trigger_grapple");
  add_code_change(func3 + 0x0, lis, "grapple_lasso");
  add_code_change(func3 + 0x4, ori, "grapple_lasso");
  add_code_change(func3 + 0x8, 0x80630000, "grapple_lasso"); // lwz r3, 0(r3)
  add_code_change(func3 + 0xC, 0x4E800020, "grapple_lasso"); // blr
}

void FpsControls::add_control_state_hook_mp3(u32 start_point, Region region) {
  Game active_game = GetHackManager()->get_active_game();
  if (region == Region::NTSC_U) {
    if (active_game == Game::PRIME_3) {
      add_code_change(start_point + 0x00, 0x3c60805c);  // lis  r3, 0x805c
      add_code_change(start_point + 0x04, 0x38636c40);  // addi r3, r3, 0x6c40
    } else {
      add_code_change(start_point + 0x00, 0x3c60805c);  // lis  r3, 0x805c
      add_code_change(start_point + 0x04, 0x38634f6c);  // addi r3, r3, 0x4f6c
    }
  } else if (region == Region::NTSC_J) {
    if (active_game == Game::PRIME_3_STANDALONE)
    {
      add_code_change(start_point + 0x00, 0x3c60805d);  // lis  r3, 0x805d
      add_code_change(start_point + 0x04, 0x3863aa30);  // subi r3, r3, 0x55d0
    }
  } else if (region == Region::PAL) {
    if (active_game == Game::PRIME_3) {
      add_code_change(start_point + 0x00, 0x3c60805d);  // lis  r3, 0x805d
      add_code_change(start_point + 0x04, 0x3863a0c0);  // subi r3, r3, 0x5f40
    }
    else {
      add_code_change(start_point + 0x00, 0x3c60805c);  // lis  r3, 0x805c
      add_code_change(start_point + 0x04, 0x38637570);  // addi r3, r3, 0x7570
    }
  }
  add_code_change(start_point + 0x08, 0x8063002c);  // lwz  r3, 0x2c(r3)
  if (active_game == Game::PRIME_3_STANDALONE && region == Region::NTSC_U) {
    add_code_change(start_point + 0x0c, 0x60000000);  // nop
  } else {
    add_code_change(start_point + 0x0c, 0x80630004);  // lwz  r3, 0x04(r3)
  }
  add_code_change(start_point + 0x10, 0x80632184);  // lwz  r3, 0x2184(r3)
  add_code_change(start_point + 0x14, 0x7c03f800);  // cmpw r3, r31
  add_code_change(start_point + 0x18, 0x4d820020);  // beqlr
  add_code_change(start_point + 0x1c, 0x7fe3fb78);  // mr   r3, r31
  add_code_change(start_point + 0x20, 0x90c30078);  // stw  r6, 0x78(r3)
  add_code_change(start_point + 0x24, 0x4e800020);  // blr
}

// Truly cursed
void FpsControls::add_strafe_code_mp1_100(Game revision) {
  const bool is_v100 = revision == Game::PRIME_1_GCN;
  // calculate side movement @ 805afc00
  // stwu r1, 0x18(r1)
  // mfspr r0, LR
  // stw r0, 0x1c(r1)
  // lwz r5, -0x5ee8(r13)
  // lwz r4, 0x2b0(r29)
  // cmpwi r4, 2
  // li r4, 4
  // bne 0x8
  // lwz r4, 0x2ac(r29)
  // slwi r4, r4, 2
  // add r3, r4, r5
  // lfs f1, 0x44(r3)
  // lfs f2, 0x4(r3)
  // fmuls f3, f2, f27
  // lfs f0, 0xe8(r29)
  // fmuls f1, f1, f0
  // fdivs f1, f1, f3
  // lfs f0, 0xa4(r3)
  // stfs f0, 0x10(r1)
  // fmuls f1, f1, f0
  // lfs f0, -0x4260(r2)
  // fcmpo cr0, f30, f0
  // lfs f0, -0x4238(r2)
  // ble 0x8
  // lfs f0, -0x4280(r2)
  // fmuls f0, f0, f1
  // lfs f3, 0x10(r1)
  // fsubs f3, f3, f1
  // fmuls f3, f3, f30
  // fadds f0, f0, f3
  // stfs f0, 0x18(r1)
  // stfs f2, 0x14(r1)
  // addi r3, r1, 0x4
  // addi r4, r29, 0x34
  // addi r5, r29, 0x138
  // bl 0xFFD62D98
  // lfs f0, 0x18(r1)
  // lfs f1, 0x4(r1)
  // fsubs f0, f0, f1
  // lfs f1, 0x10(r1)
  // fdivs f0, f0, f1
  // lfs f1, -0x4238(r2)
  // fcmpo cr0, f0, f1
  // bge 0xc
  // fmr f0, f1
  // b 0x14
  // lfs f1, -0x4280(r2)
  // fcmpo cr0, f0, f1
  // ble 0x8
  // fmr f0, f1
  // lfs f1, 0x14(r1)
  // fmuls f1, f0, f1
  // lwz r0, 0x1c(r1)
  // mtspr LR, r0
  // addi r1, r1, -0x18
  // blr
  add_code_change(0x805afc00, 0x94210018);
  add_code_change(0x805afc04, 0x7c0802a6);
  add_code_change(0x805afc08, 0x9001001c);
  add_code_change(0x805afc0c, 0x80ada118);
  add_code_change(0x805afc10, 0x809d02b0);
  add_code_change(0x805afc14, 0x2c040002);
  add_code_change(0x805afc18, 0x38800004);
  add_code_change(0x805afc1c, 0x40820008);
  add_code_change(0x805afc20, 0x809d02ac);
  add_code_change(0x805afc24, 0x5484103a);
  add_code_change(0x805afc28, 0x7c642a14);
  add_code_change(0x805afc2c, 0xc0230044);
  add_code_change(0x805afc30, 0xc0430004);
  add_code_change(0x805afc34, 0xec6206f2);
  add_code_change(0x805afc38, 0xc01d00e8);
  add_code_change(0x805afc3c, 0xec210032);
  add_code_change(0x805afc40, 0xec211824);
  add_code_change(0x805afc44, 0xc00300a4);
  add_code_change(0x805afc48, 0xd0010010);
  add_code_change(0x805afc4c, 0xec210032);
  add_code_change(0x805afc50, 0xc002bda0);
  add_code_change(0x805afc54, 0xfc1e0040);
  add_code_change(0x805afc58, 0xc002bdc8);
  add_code_change(0x805afc5c, 0x40810008);
  add_code_change(0x805afc60, 0xc002bd80);
  add_code_change(0x805afc64, 0xec000072);
  add_code_change(0x805afc68, 0xc0610010);
  add_code_change(0x805afc6c, 0xec630828);
  add_code_change(0x805afc70, 0xec6307b2);
  add_code_change(0x805afc74, 0xec00182a);
  add_code_change(0x805afc78, 0xd0010018);
  add_code_change(0x805afc7c, 0xd0410014);
  add_code_change(0x805afc80, 0x38610004);
  add_code_change(0x805afc84, 0x389d0034);
  add_code_change(0x805afc88, 0x38bd0138);
  add_code_change(0x805afc8c, is_v100 ? 0x4bd62d99 : 0x4bd62e79);
  add_code_change(0x805afc90, 0xc0010018);
  add_code_change(0x805afc94, 0xc0210004);
  add_code_change(0x805afc98, 0xec000828);
  add_code_change(0x805afc9c, 0xc0210010);
  add_code_change(0x805afca0, 0xec000824);
  add_code_change(0x805afca4, 0xc022bdc8);
  add_code_change(0x805afca8, 0xfc000840);
  add_code_change(0x805afcac, 0x4080000c);
  add_code_change(0x805afcb0, 0xfc000890);
  add_code_change(0x805afcb4, 0x48000014);
  add_code_change(0x805afcb8, 0xc022bd80);
  add_code_change(0x805afcbc, 0xfc000840);
  add_code_change(0x805afcc0, 0x40810008);
  add_code_change(0x805afcc4, 0xfc000890);
  add_code_change(0x805afcc8, 0xc0210014);
  add_code_change(0x805afccc, 0xec200072);
  add_code_change(0x805afcd0, 0x8001001c);
  add_code_change(0x805afcd4, 0x7c0803a6);
  add_code_change(0x805afcd8, 0x3821ffe8);
  add_code_change(0x805afcdc, 0x4e800020);

  u32 inject_base = is_v100 ? 0x802875c4 : 0x80287640;
  // Apply strafe force instead of torque v1.00 @ 802875c4 | v1.01 @ 80287640
  // lfs f1, -0x4260(r2)
  // lfs f0, -0x41bc(r2)
  // fsubs f1, f30, f1
  // fabs f1, f1
  // fcmpo cr0, f1, f0
  // ble 0x2c
  // bl 0x328624
  // bl 0xFFD93F54
  // mr r5, r3
  // mr r3, r29
  // lfs f0, -0x4260(r2)
  // stfs f1, 0x10(r1)
  // stfs f0, 0x14(r1)
  // stfs f0, 0x18(r1)
  // addi r4, r1, 0x10
  add_code_change(inject_base + 0x0, 0xc022bda0);
  add_code_change(inject_base + 0x4, 0xc002be44);
  add_code_change(inject_base + 0x8, 0xec3e0828);
  add_code_change(inject_base + 0xc, 0xfc200a10);
  add_code_change(inject_base + 0x10, 0xfc010040);
  add_code_change(inject_base + 0x14, 0x4081002c);
  add_code_change(inject_base + 0x18, is_v100 ? 0x48328625 : 0x483285a9);
  add_code_change(inject_base + 0x1c, is_v100 ? 0x4bd93f55 : 0x4bd93f55);
  add_code_change(inject_base + 0x20, 0x7c651b78);
  add_code_change(inject_base + 0x24, 0x7fa3eb78);
  add_code_change(inject_base + 0x28, 0xc002bda0);
  add_code_change(inject_base + 0x2c, 0xd0210010);
  add_code_change(inject_base + 0x30, 0xd0010014);
  add_code_change(inject_base + 0x34, 0xd0010018);
  add_code_change(inject_base + 0x38, 0x38810010);

  // disable rotation on LR analog
  if (is_v100) {
    add_code_change(0x80286fe0, 0x4bfffc71);
    add_code_change(0x80286c88, 0x4800000c);
    add_code_change(0x8028739c, 0x60000000);
    add_code_change(0x802873e0, 0x60000000);
    add_code_change(0x8028707c, 0x60000000);
    add_code_change(0x802871bc, 0x60000000);
    add_code_change(0x80287288, 0x60000000);
  } else {
    add_code_change(0x8028705c, 0x4bfffc71);
    add_code_change(0x80286d04, 0x4800000c);
    add_code_change(0x80287418, 0x60000000);
    add_code_change(0x8028745c, 0x60000000);
    add_code_change(0x802870f8, 0x60000000);
    add_code_change(0x80287238, 0x60000000);
    add_code_change(0x80287288, 0x60000000);
  }

  // Clamp current xy velocity v1.00 @ 802872a4 | v1.01 @ 80287320
  // lfs f1, -0x7ec0(r2)
  // fmuls f0, f30, f30
  // fcmpo cr0, f0, f1
  // ble 0x134
  // fmuls f0, f31, f31m
  // fcmpo cr0, f0, f1
  // ble 0x128
  // lfs f0, 0x138(r29)
  // lfs f1, 0x13c(r29)
  // fmuls f0, f0, f0
  // fmadds f1, f1, f1, f0
  // frsqrte f1, f1
  // fres f1, f1
  // addi r3, r2, -0x2040
  // slwi r0, r0, 2
  // add r3, r0, r3
  // lfs f0, 0(r3)
  // fcmpo cr0, f1, f0
  // ble 0xf8
  // lfs f3, 0xe8(r29)
  // lfs f2, 0x138(r29)
  // fdivs f2, f2, f1
  // fmuls f2, f0, f2
  // stfs f2, 0x138(r29)
  // fmuls f2, f3, f2
  // stfs f2, 0xfc(r29)
  // lfs f2, 0x13c(r29)
  // fdivs f2, f2, f1
  // fmuls f2, f0, f2
  // stfs f2, 0x13c(r29)
  // fmuls f2, f3, f2
  // stfs f2, 0x100(r29)
  // b 0xc0
  inject_base = is_v100 ? 0x802872a4 : 0x80287320;

  add_code_change(inject_base + 0x00, 0xc0228140);
  add_code_change(inject_base + 0x04, 0xec1e07b2);
  add_code_change(inject_base + 0x08, 0xfc000840);
  add_code_change(inject_base + 0x0c, 0x40810134);
  add_code_change(inject_base + 0x10, 0xec1f07f2);
  add_code_change(inject_base + 0x14, 0xfc000840);
  add_code_change(inject_base + 0x18, 0x40810128);
  add_code_change(inject_base + 0x1c, 0xc01d0138);
  add_code_change(inject_base + 0x20, 0xc03d013c);
  add_code_change(inject_base + 0x24, 0xec000032);
  add_code_change(inject_base + 0x28, 0xec21007a);
  add_code_change(inject_base + 0x2c, 0xfc200834);
  add_code_change(inject_base + 0x30, 0xec200830);
  add_code_change(inject_base + 0x34, 0x3862dfc0);
  add_code_change(inject_base + 0x38, 0x5400103a);
  add_code_change(inject_base + 0x3c, 0x7c601a14);
  add_code_change(inject_base + 0x40, 0xc0030000);
  add_code_change(inject_base + 0x44, 0xfc010040);
  add_code_change(inject_base + 0x48, 0x408100f8);
  add_code_change(inject_base + 0x4c, 0xc07d00e8);
  add_code_change(inject_base + 0x50, 0xc05d0138);
  add_code_change(inject_base + 0x54, 0xec420824);
  add_code_change(inject_base + 0x58, 0xec4000b2);
  add_code_change(inject_base + 0x5c, 0xd05d0138);
  add_code_change(inject_base + 0x60, 0xec4300b2);
  add_code_change(inject_base + 0x64, 0xd05d00fc);
  add_code_change(inject_base + 0x68, 0xc05d013c);
  add_code_change(inject_base + 0x6c, 0xec420824);
  add_code_change(inject_base + 0x70, 0xec4000b2);
  add_code_change(inject_base + 0x74, 0xd05d013c);
  add_code_change(inject_base + 0x78, 0xec4300b2);
  add_code_change(inject_base + 0x7c, 0xd05d0100);
  add_code_change(inject_base + 0x80, 0x480000c0);

  // max speed values table @ 805afce0
  add_code_change(0x805afce0, 0x41480000);
  add_code_change(0x805afce4, 0x41480000);
  add_code_change(0x805afce8, 0x41480000);
  add_code_change(0x805afcec, 0x41480000);
  add_code_change(0x805afcf0, 0x41480000);
  add_code_change(0x805afcf4, 0x41480000);
  add_code_change(0x805afcf8, 0x41480000);
  add_code_change(0x805afcfc, 0x41480000);
}

void FpsControls::add_strafe_code_mp1_102(Region region) {
  const bool is_ntsc = region == Region::NTSC_U;
  // calculate side movement @ 80471c00
  // stwu r1, 0x18(r1)
  // mfspr r0, LR
  // stw r0, 0x1c(r1)
  // lwz r5, -0x5e70(r13) -> -0x5ec8
  // lwz r4, 0x2c0(r29)
  // cmpwi r4, 2
  // li r4, 4
  // bne 0x8
  // lwz r4, 0x2bc(r29)
  // slwi r4, r4, 2
  // add r3, r4, r5
  // lfs f1, 0x44(r3)
  // lfs f2, 0x4(r3)
  // fmuls f3, f2, f27
  // lfs f0, 0xf8(r29)
  // fmuls f1, f1, f0
  // fdivs f1, f1, f3
  // lfs f0, 0xa4(r3)
  // stfs f0, 0x10(r1)
  // fmuls f1, f1, f0
  // lfs f0, -0x4180(r2) = 0.0f
  // fcmpo cr0, f30, f0
  // lfs f0, -0x4158(r2) = -1
  // ble 0x8
  // lfs f0, -0x41A0(r2) = 1
  // fmuls f0, f0, f1
  // lfs f3, 0x10(r1)
  // fsubs f3, f3, f1
  // fmuls f3, f3, f30
  // fadds f0, f0, f3
  // stfs f0, 0x18(r1)
  // stfs f2, 0x14(r1)
  // addi r3, r1, 0x4
  // addi r4, r29, 0x34
  // addi r5, r29, 0x148
  // bl 0xffe89b68
  // lfs f0, 0x18(r1)
  // lfs f1, 0x4(r1)
  // fsubs f0, f0, f1
  // lfs f1, 0x10(r1)
  // fdivs f0, f0, f1
  // lfs f1, -0x4158(r2) = -1
  // fcmpo cr0, f0, f1
  // bge 0xc
  // fmr f0, f1
  // b 0x14
  // lfs f1, -0x41A0(r2) = 1
  // fcmpo cr0, f0, f1
  // ble 0x8
  // fmr f0, f1
  // lfs f1, 0x14(r1)
  // fmuls f1, f0, f1
  // lwz r0, 0x1c(r1)
  // mtspr LR, r0
  // addi r1, r1, -0x18
  // blr
  u32 inject_base = is_ntsc ? 0x805b0c00 : 0x80471c00;

  add_code_change(inject_base + 0x00, 0x94210018);
  add_code_change(inject_base + 0x04, 0x7c0802a6);
  add_code_change(inject_base + 0x08, 0x9001001c);
  add_code_change(inject_base + 0x0c, is_ntsc ? 0x80ada138 : 0x80ada190);
  add_code_change(inject_base + 0x10, 0x809d02c0);
  add_code_change(inject_base + 0x14, 0x2c040002);
  add_code_change(inject_base + 0x18, 0x38800004);
  add_code_change(inject_base + 0x1c, 0x40820008);
  add_code_change(inject_base + 0x20, 0x809d02bc);
  add_code_change(inject_base + 0x24, 0x5484103a);
  add_code_change(inject_base + 0x28, 0x7c642a14);
  add_code_change(inject_base + 0x2c, 0xc0230044);
  add_code_change(inject_base + 0x30, 0xc0430004);
  add_code_change(inject_base + 0x34, 0xec6206f2);
  add_code_change(inject_base + 0x38, 0xc01d00f8);
  add_code_change(inject_base + 0x3c, 0xec210032);
  add_code_change(inject_base + 0x40, 0xec211824);
  add_code_change(inject_base + 0x44, 0xc00300a4);
  add_code_change(inject_base + 0x48, 0xd0010010);
  add_code_change(inject_base + 0x4c, 0xec210032);
  add_code_change(inject_base + 0x50, is_ntsc ? 0xc002be70 : 0xc002be80);
  add_code_change(inject_base + 0x54, 0xfc1e0040);
  add_code_change(inject_base + 0x58, is_ntsc ? 0xc002bdd0 : 0xc002bea8);
  add_code_change(inject_base + 0x5c, 0x40810008);
  add_code_change(inject_base + 0x60, is_ntsc ? 0xc002be80 : 0xc002be60);
  add_code_change(inject_base + 0x64, 0xec000072);
  add_code_change(inject_base + 0x68, 0xc0610010);
  add_code_change(inject_base + 0x6c, 0xec630828);
  add_code_change(inject_base + 0x70, 0xec6307b2);
  add_code_change(inject_base + 0x74, 0xec00182a);
  add_code_change(inject_base + 0x78, 0xd0010018);
  add_code_change(inject_base + 0x7c, 0xd0410014);
  add_code_change(inject_base + 0x80, 0x38610004);
  add_code_change(inject_base + 0x84, 0x389d0034);
  add_code_change(inject_base + 0x88, 0x38bd0148);
  add_code_change(inject_base + 0x8c, is_ntsc ? 0x4bd627e9 : 0x4be89b69);
  add_code_change(inject_base + 0x90, 0xc0010018);
  add_code_change(inject_base + 0x94, 0xc0210004);
  add_code_change(inject_base + 0x98, 0xec000828);
  add_code_change(inject_base + 0x9c, 0xc0210010);
  add_code_change(inject_base + 0xa0, 0xec000824);
  add_code_change(inject_base + 0xa4, is_ntsc ? 0xc022bdd0 : 0xc022bea8);
  add_code_change(inject_base + 0xa8, 0xfc000840);
  add_code_change(inject_base + 0xac, 0x4080000c);
  add_code_change(inject_base + 0xb0, 0xfc000890);
  add_code_change(inject_base + 0xb4, 0x48000014);
  add_code_change(inject_base + 0xb8, is_ntsc ? 0xc022be80 : 0xc022be60);
  add_code_change(inject_base + 0xbc, 0xfc000840);
  add_code_change(inject_base + 0xc0, 0x40810008);
  add_code_change(inject_base + 0xc4, 0xfc000890);
  add_code_change(inject_base + 0xc8, 0xc0210014);
  add_code_change(inject_base + 0xcc, 0xec200072);
  add_code_change(inject_base + 0xd0, 0x8001001c);
  add_code_change(inject_base + 0xd4, 0x7c0803a6);
  add_code_change(inject_base + 0xd8, 0x3821ffe8);
  add_code_change(inject_base + 0xdc, 0x4e800020);

  // Apply strafe force instead of torque @ 802749a8
  // lfs f1, -0x4180(r2) = 0.0f
  // lfs f0, -0x40DC(r2) = 1e-5
  // fsubs f1, f30, f1
  // fabs f1, f1
  // fcmpo cr0, f1, f0
  // ble 0x2c
  // bl 0x1fd240
  // bl 0xffda74e0
  // mr r5, r3
  // mr r3, r29
  // lfs f0, -0x4180(r2) = 0.0f
  // stfs f1, 0x10(r1)
  // stfs f0, 0x14(r1)
  // stfs f0, 0x18(r1)
  // addi r4, r1, 0x10
  inject_base = is_ntsc ? 0x80287f50 : 0x802749a8;

  add_code_change(inject_base + 0x00, is_ntsc ? 0xc022be70 : 0xc022be80);
  add_code_change(inject_base + 0x04, is_ntsc ? 0xc002bc94 : 0xc002bf24);
  add_code_change(inject_base + 0x08, 0xec3e0828);
  add_code_change(inject_base + 0x0c, 0xfc200a10);
  add_code_change(inject_base + 0x10, 0xfc010040);
  add_code_change(inject_base + 0x14, 0x4081002c);
  add_code_change(inject_base + 0x18, is_ntsc ? 0x48328c99 : 0x481fd241);
  add_code_change(inject_base + 0x1c, is_ntsc ? 0x4bd938a9 : 0x4bda74e1);
  add_code_change(inject_base + 0x20, 0x7c651b78);
  add_code_change(inject_base + 0x24, 0x7fa3eb78);
  add_code_change(inject_base + 0x28, is_ntsc ? 0xc002be70 : 0xc002be80);
  add_code_change(inject_base + 0x2c, 0xd0210010);
  add_code_change(inject_base + 0x30, 0xd0010014);
  add_code_change(inject_base + 0x34, 0xd0010018);
  add_code_change(inject_base + 0x38, 0x38810010);

  // disable rotation on LR analog
  if (is_ntsc) {
    add_code_change(0x8028796c, 0x4bfffc71); // jump/address updated
    add_code_change(0x80287614, 0x4800000c); // updated following addresses
    add_code_change(0x80287d28, 0x60000000);
    add_code_change(0x80287d6c, 0x60000000);
    add_code_change(0x80287a08, 0x60000000);
    add_code_change(0x80287b48, 0x60000000);
    add_code_change(0x80287c14, 0x60000000);
  } else {
    add_code_change(0x802743c4, 0x4bfffc71); // jump/address updated
    add_code_change(0x8027406c, 0x4800000c); // updated following addresses
    add_code_change(0x80274780, 0x60000000);
    add_code_change(0x802747c4, 0x60000000);
    add_code_change(0x80274460, 0x60000000);
    add_code_change(0x802745a0, 0x60000000);
    add_code_change(0x8027466c, 0x60000000);
  }

  // Clamp current xy velocity NTSC @ 80287c30 | PAL @ 80274688
  // lfs f1, -0x7ec0(r2) = 0.1
  // fmuls f0, f30, f30
  // fcmpo cr0, f0, f1
  // ble 0x134
  // fmuls f0, f31, f31
  // fcmpo cr0, f0, f1
  // ble 0x128
  // lfs f0, 0x148(r29)
  // lfs f1, 0x14c(r29)
  // fmuls f0, f0, f0
  // fmadds f1, f1, f1, f0
  // frsqrte f1, f1
  // fres f1, f1
  // addi r3, r2, -0x2180 or -0x2100
  // slwi r0, r0, 2
  // add r3, r0, r3
  // lfs f0, 0(r3)
  // fcmpo cr0, f1, f0
  // ble 0xf8
  // lfs f3, 0xf8(r29)
  // lfs f2, 0x148(r29)
  // fdivs f2, f2, f1
  // fmuls f2, f0, f2
  // stfs f2, 0x148(r29)
  // fmuls f2, f3, f2
  // stfs f2, 0x10c(r29)
  // lfs f2, 0x14c(r29)
  // fdivs f2, f2, f1
  // fmuls f2, f0, f2
  // stfs f2, 0x14c(r29)
  // fmuls f2, f3, f2
  // stfs f2, 0x110(r29)
  // b 0xc0
  inject_base = is_ntsc ? 0x80287c30 : 0x80274688;

  add_code_change(inject_base + 0x00, 0xc0228140);
  add_code_change(inject_base + 0x04, 0xec1e07b2);
  add_code_change(inject_base + 0x08, 0xfc000840);
  add_code_change(inject_base + 0x0c, 0x40810134);
  add_code_change(inject_base + 0x10, 0xec1f07f2);
  add_code_change(inject_base + 0x14, 0xfc000840);
  add_code_change(inject_base + 0x18, 0x40810128);
  add_code_change(inject_base + 0x1c, 0xc01d0148);
  add_code_change(inject_base + 0x20, 0xc03d014c);
  add_code_change(inject_base + 0x24, 0xec000032);
  add_code_change(inject_base + 0x28, 0xec21007a);
  add_code_change(inject_base + 0x2c, 0xfc200834);
  add_code_change(inject_base + 0x30, 0xec200830);
  add_code_change(inject_base + 0x34, is_ntsc ? 0x3862df00 : 0x3862de80);
  add_code_change(inject_base + 0x38, 0x5400103a);
  add_code_change(inject_base + 0x3c, 0x7c601a14);
  add_code_change(inject_base + 0x40, 0xc0030000);
  add_code_change(inject_base + 0x44, 0xfc010040);
  add_code_change(inject_base + 0x48, 0x408100f8);
  add_code_change(inject_base + 0x4c, 0xc07d00f8);
  add_code_change(inject_base + 0x50, 0xc05d0148);
  add_code_change(inject_base + 0x54, 0xec420824);
  add_code_change(inject_base + 0x58, 0xec4000b2);
  add_code_change(inject_base + 0x5c, 0xd05d0148);
  add_code_change(inject_base + 0x60, 0xec4300b2);
  add_code_change(inject_base + 0x64, 0xd05d010c);
  add_code_change(inject_base + 0x68, 0xc05d014c);
  add_code_change(inject_base + 0x6c, 0xec420824);
  add_code_change(inject_base + 0x70, 0xec4000b2);
  add_code_change(inject_base + 0x74, 0xd05d014c);
  add_code_change(inject_base + 0x78, 0xec4300b2);
  add_code_change(inject_base + 0x7c, 0xd05d0110);
  add_code_change(inject_base + 0x80, 0x480000c0);

  // No change needed
  // max speed values table NTSC @ 805b2de0 | PAL @ 80471ce0
  inject_base = is_ntsc ? 0x805b2de0 : 0x80471ce0;
  add_code_change(inject_base + 0x00, 0x41480000);
  add_code_change(inject_base + 0x04, 0x41480000);
  add_code_change(inject_base + 0x08, 0x41480000);
  add_code_change(inject_base + 0x0c, 0x41480000);
  add_code_change(inject_base + 0x10, 0x41480000);
  add_code_change(inject_base + 0x14, 0x41480000);
  add_code_change(inject_base + 0x18, 0x41480000);
  add_code_change(inject_base + 0x1c, 0x41480000);
}

void FpsControls::init_mod_menu(Game game, Region region)
{
  if (region == Region::NTSC_J) {
    if (game == Game::MENU_PRIME_1) {
      // prevent wiimote pointer feedback to move the cursor
      add_code_change(0x80487160, 0x60000000);
      add_code_change(0x80487164, 0x60000000);
      // Prevent recentering the cursor on X axis
      add_code_change(0x80487090, 0x60000000);
      // Prevent recentering the cursor on Y axis
      add_code_change(0x80487098, 0x60000000);
    }
    if (game == Game::MENU_PRIME_2) {
      // prevent wiimote pointer feedback to move the cursor
      add_code_change(0x80486fe8, 0x60000000);
      add_code_change(0x80486fec, 0x60000000);
      // Prevent recentering the cursor on X axis
      add_code_change(0x80486f18, 0x60000000);
      // Prevent recentering the cursor on Y axis
      add_code_change(0x80486f20, 0x60000000);
    }
  }
}

void FpsControls::init_mod_mp1(Region region) {
  prime::GetVariableManager()->register_variable("new_beam");
  prime::GetVariableManager()->register_variable("beamchange_flag");
  if (region == Region::NTSC_U) {
    // This instruction change is used in all 3 games, all 3 regions. It's an update to what I believe
    // to be interpolation for player camera pitch The change is from fmuls f0, f0, f1 (0xec000072) to
    // fmuls f0, f1, f1 (0xec010072), where f0 is the output. The higher f0 is, the faster the pitch
    // *can* change in-game, and squaring f1 seems to do the job.
    add_code_change(0x80098ee4, 0xec010072);

    // NOPs for changing First Person Camera's pitch based on floor (normal? angle?)
    add_code_change(0x80099138, 0x60000000);
    add_code_change(0x80183a8c, 0x60000000);
    add_code_change(0x80183a64, 0x60000000);
    add_code_change(0x8017661c, 0x60000000);
    // Cursor location, sets to f17 (always 0 due to little use)
    add_code_change(0x802fb5b4, 0xd23f009c);
    add_code_change(0x8019fbcc, 0x60000000);

    add_code_change(0x80075f24, 0x60000000, "beam_menu");
    add_code_change(0x80075f0c, 0x60000000, "visor_menu");

    add_beam_change_code_mp1(0x8018e544);

    // Steps over bounds checking on the reticle
    add_code_change(0x80015164, 0x4800010c);
  } else if (region == Region::PAL) {
    // Same as NTSC but slightly offset
    add_code_change(0x80099068, 0xec010072);
    add_code_change(0x800992c4, 0x60000000);
    add_code_change(0x80183cfc, 0x60000000);
    add_code_change(0x80183d24, 0x60000000);
    add_code_change(0x801768b4, 0x60000000);
    add_code_change(0x802fb84c, 0xd23f009c);
    add_code_change(0x8019fe64, 0x60000000);

    add_code_change(0x80075f74, 0x60000000, "beam_menu");
    add_code_change(0x80075f8c, 0x60000000, "visor_menu");

    add_beam_change_code_mp1(0x8018e7dc);

    // Steps over bounds checking on the reticle
    add_code_change(0x80015164, 0x4800010c);
  } else { // region == Region::NTSC-J
    // Same as NTSC but slightly offset
    add_code_change(0x80099060, 0xec010072);
    add_code_change(0x800992b4, 0x60000000);
    add_code_change(0x8018460c, 0x60000000);
    add_code_change(0x801835e4, 0x60000000);
    add_code_change(0x80176ff0, 0x60000000);
    add_code_change(0x802fb234, 0xd23f009c);
    add_code_change(0x801a074c, 0x60000000);

    add_code_change(0x800760a4, 0x60000000, "beam_menu");
    add_code_change(0x8007608c, 0x60000000, "visor_menu");

    add_beam_change_code_mp1(0x8018f0c4);
  }
  has_beams = true;
}

void FpsControls::init_mod_mp1_gc(Region region) {
  if (region == Region::NTSC_U) {
    //add_code_change(0x8000f63c, 0x48000048);
    add_code_change(0x800ea15c, 0x38810044); // output cannon bob only for viewbob
    add_code_change(0x8000e538, 0x60000000);
    //add_code_change(0x80016ee4, 0x4e800020);
    add_code_change(0x80014820, 0x4e800020);
    add_code_change(0x8000e73c, 0x60000000);
    add_code_change(0x8000f810, 0x48000244);
    // When attached to a grapple point and spinning around it
    // the player's yaw is adjusted, this ensures only position is updated
    // Grapple point yaw fix
    add_code_change(0x8017a18c, 0x7fa3eb78);
    add_code_change(0x8017a190, 0x3881006c);
    add_code_change(0x8017a194, 0x4bed8cf9);

    // Show crosshair but don't consider pressing R button
    add_code_change(0x80016ee4, 0x3b000001, "show_crosshair"); // li r24, 1
    add_code_change(0x80016ee8, 0x8afd09c4, "show_crosshair"); // lbz r23, 0x9c4(r29)
    add_code_change(0x80016eec, 0x53173672, "show_crosshair"); // rlwimi r23, r24, 6, 25, 25 (00000001)
    add_code_change(0x80016ef0, 0x9afd09c4, "show_crosshair"); // stb r23, 0x9c4(r29)
    add_code_change(0x80016ef4, 0x4e800020, "show_crosshair"); // blr

    add_strafe_code_mp1_100(Game::PRIME_1_GCN);
  } else if (region == Region::PAL) {
    //add_code_change(0x8000fb4c, 0x48000048);
    add_code_change(0x800e2190, 0x38810044); // output cannon bob only for viewbob
    add_code_change(0x8000ea60, 0x60000000);
    //add_code_change(0x80017878, 0x4e800020);
    add_code_change(0x80015258, 0x4e800020);
    add_code_change(0x8000ec64, 0x60000000);
    add_code_change(0x8000fd20, 0x4800022c);
    // Grapple point yaw fix
    add_code_change(0x8016fc54, 0x7fa3eb78);
    add_code_change(0x8016fc58, 0x38810064); // 6c-8 = 64
    add_code_change(0x8016fc5c, 0x4bee4345); // bl 80053fa0

    // Show crosshair but don't consider pressing R button
    add_code_change(0x80017878, 0x3b000001, "show_crosshair"); // li r24, 1
    add_code_change(0x8001787c, 0x8afd09d4, "show_crosshair"); // lbz r23, 0x9d4(r29)
    add_code_change(0x80017880, 0x53173672, "show_crosshair"); // rlwimi r23, r24, 6, 25, 25 (00000001)
    add_code_change(0x80017884, 0x9afd09d4, "show_crosshair"); // stb r23, 0x9d4(r29)
    add_code_change(0x80017888, 0x4e800020, "show_crosshair"); // blr

    add_strafe_code_mp1_102(Region::PAL);
  } else {}
}

void FpsControls::init_mod_mp1_gc_r1() {
  //add_code_change(0x8000f6b8, 0x48000048);
  add_code_change(0x800ea1d8, 0x38810044); // output cannon bob only for viewbob

  add_code_change(0x8000e5b4, 0x60000000);

  add_code_change(0x8001489c, 0x4e800020);
  add_code_change(0x8000e7b8, 0x60000000);
  add_code_change(0x8000f88c, 0x48000244);
  // When attached to a grapple point and spinning around it
  // the player's yaw is adjusted, this ensures only position is updated
  // Grapple point yaw fix
  add_code_change(0x8017a208, 0x7fa3eb78);
  add_code_change(0x8017a20c, 0x3881006c);
  add_code_change(0x8017a210, 0x4bed8cf9);

  // Show crosshair but don't consider pressing R button
  add_code_change(0x80016f60, 0x3b000001, "show_crosshair");
  add_code_change(0x80016f64, 0x8afd09c4, "show_crosshair");
  add_code_change(0x80016f68, 0x53173672, "show_crosshair");
  add_code_change(0x80016f6c, 0x9afd09c4, "show_crosshair");
  add_code_change(0x80016f70, 0x4e800020, "show_crosshair");
  add_strafe_code_mp1_100(Game::PRIME_1_GCN_R1);
}

void FpsControls::init_mod_mp1_gc_r2() {
  //add_code_change(0x8000f8f8, 0x48000048);
  add_code_change(0x800ea6e0, 0x38810044); // output cannon bob only for viewbob

  add_code_change(0x8000e7f4, 0x60000000);

  add_code_change(0x80014aec, 0x4e800020);
  add_code_change(0x8000e9f8, 0x60000000);
  add_code_change(0x8000facc, 0x4800022c);
  // Grapple point yaw fix
  add_code_change(0x8017a970, 0x7fa3eb78);
  add_code_change(0x8017a974, 0x3881006c);
  add_code_change(0x8017a978, 0x4bed885d);

  // Show crosshair but don't consider pressing R button
  add_code_change(0x800171c4, 0x3b000001, "show_crosshair"); // li r24, 1
  add_code_change(0x800171c8, 0x8afd09d4, "show_crosshair"); // lbz r23, 0x9d4(r29)
  add_code_change(0x800171cc, 0x53173672, "show_crosshair"); // rlwimi r23, r24, 6, 25, 25 (00000001)
  add_code_change(0x800171d0, 0x9afd09d4, "show_crosshair"); // stb r23, 0x9d4(r29)
  add_code_change(0x800171d4, 0x4e800020, "show_crosshair"); // blr

  add_strafe_code_mp1_102(Region::NTSC_U);
}

void FpsControls::init_mod_mp2(Region region) {
  prime::GetVariableManager()->register_variable("new_beam");
  prime::GetVariableManager()->register_variable("beamchange_flag");
  if (region == Region::NTSC_U) {
    add_code_change(0x8008ccc8, 0xc0430184);
    add_code_change(0x8008cd1c, 0x60000000);
    add_code_change(0x80147f70, 0x60000000);
    add_code_change(0x80147f98, 0x60000000);
    add_code_change(0x80135b20, 0x60000000);
    add_code_change(0x8008bb48, 0x60000000);
    add_code_change(0x8008bb18, 0x60000000);
    add_code_change(0x803054a0, 0xd23f009c);
    add_code_change(0x80169dbc, 0x60000000);
    add_code_change(0x80143d00, 0x48000050);

    add_code_change(0x8006fde0, 0x60000000, "beam_menu");
    add_code_change(0x8006fdc4, 0x60000000, "visor_menu");

    add_beam_change_code_mp2(0x8018cc88);

    // Steps over bounds checking on the reticle
    add_code_change(0x80018528, 0x48000144);
  } else if (region == Region::PAL) {
    add_code_change(0x8008e30c, 0xc0430184);
    add_code_change(0x8008e360, 0x60000000);
    add_code_change(0x801496e4, 0x60000000);
    add_code_change(0x8014970c, 0x60000000);
    add_code_change(0x80137240, 0x60000000);
    add_code_change(0x8008d18c, 0x60000000);
    add_code_change(0x8008d15c, 0x60000000);
    add_code_change(0x80307d2c, 0xd23f009c);
    add_code_change(0x8016b534, 0x60000000);
    add_code_change(0x80145474, 0x48000050);

    add_code_change(0x80071358, 0x60000000, "beam_menu");
    add_code_change(0x8007133c, 0x60000000, "visor_menu");

    add_beam_change_code_mp2(0x8018e41c);

    // Steps over bounds checking on the reticle
    add_code_change(0x80018528, 0x48000144);
  } else if (region == Region::NTSC_J) {
    add_code_change(0x8008c944, 0xc0430184);
    add_code_change(0x8008c998, 0x60000000);
    add_code_change(0x80147578, 0x60000000);
    add_code_change(0x801475a0, 0x60000000);
    add_code_change(0x8013511c, 0x60000000);
    add_code_change(0x8008b7c4, 0x60000000);
    add_code_change(0x8008b794, 0x60000000);
    add_code_change(0x80303ec8, 0xd23f009c);
    add_code_change(0x80169388, 0x60000000);
    add_code_change(0x8014331c, 0x48000050);

    add_code_change(0x8006fbb0, 0x60000000, "beam_menu");
    add_code_change(0x8006fb94, 0x60000000, "visor_menu");

    add_beam_change_code_mp2(0x8018c0d4);
  } else {}
  has_beams = true;
}

void FpsControls::init_mod_mp2_gc(Region region) {
  if (region == Region::NTSC_U) {
    //add_code_change(0x801b00b4, 0x48000050);
    add_code_change(0x800bcd44, 0x38810044); // output cannon bob only for viewbob

    add_code_change(0x801aef58, 0x60000000);
    add_code_change(0x800129c8, 0x4e800020);
    add_code_change(0x801af160, 0x60000000);
    add_code_change(0x801b0248, 0x48000078);
    add_code_change(0x801af450, 0x48000a34);
    // Enable strafing with left/right on L-Stick
    add_code_change(0x8018846c, 0xc022a5b0);
    add_code_change(0x80188104, 0x4800000c);
    // Grapple point yaw fix
    add_code_change(0x8011d9c4, 0x389d0054);
    add_code_change(0x8011d9c8, 0x4bf2d1fd);

    add_code_change(0x80015ed8, 0x3aa00001, "show_crosshair"); // li r21, 1
    add_code_change(0x80015edc, 0x8add1268, "show_crosshair"); // lbz r22, 0x1268(r29)
    add_code_change(0x80015ee0, 0x52b63672, "show_crosshair"); // rlwimi r22, r21, 6, 25, 25 (00000001)
    add_code_change(0x80015ee4, 0x9add1268, "show_crosshair"); // stb r22, 0x1268(r29)
    add_code_change(0x80015ee8, 0x4e800020, "show_crosshair"); // blr

    add_code_change(0x800614f8, 0xc022d3e8);
    const int null_players_vmc_idx = Core::System::GetInstance().GetPowerPC().RegisterVmcall(null_players_on_destruct_mp2_gc);
    u32 null_players_vmc = gen_vmcall(null_players_vmc_idx, 0);
    add_code_change(0x80042994, null_players_vmc);
  } else if (region == Region::NTSC_J) {
    // TODO: Enable arm cannon bobbing for JP
    add_code_change(0x801b1e6c, 0x48000050);
    add_code_change(0x801b0d10, 0x60000000);
    add_code_change(0x80013414, 0x4e800020);
    add_code_change(0x801b0f18, 0x60000000);
    add_code_change(0x801b2000, 0x48000078);
    add_code_change(0x801b1208, 0x48000a34);
    // Enable strafing with left/right on L-Stick
    add_code_change(0x80189f70, 0xc022a5c8);
    add_code_change(0x80189c08, 0x4800000c);
    // Grapple point yaw fix
    add_code_change(0x8011e918, 0x389d0054);
    add_code_change(0x8011e91c, 0x4bf2cd91);

    add_code_change(0x8001695c, 0x3aa00001, "show_crosshair"); // li r21, 1
    add_code_change(0x80016960, 0x8add1268, "show_crosshair"); // lbz r22, 0x1268(r29)
    add_code_change(0x80016964, 0x52b63672, "show_crosshair"); // rlwimi r22, r21, 6, 25, 25 (00000001)
    add_code_change(0x80016968, 0x9add1268, "show_crosshair"); // stb r22, 0x1268(r29)
    add_code_change(0x8001696c, 0x4e800020, "show_crosshair"); // blr

    add_code_change(0x80061fc0, 0xc022d400);
  } else if (region == Region::PAL) {
    //add_code_change(0x801b03c0, 0x48000050);
    add_code_change(0x800bcdd0, 0x38810044); // output cannon bob only for viewbob

    add_code_change(0x801af264, 0x60000000);
    add_code_change(0x80012a2c, 0x4e800020);
    add_code_change(0x801af46c, 0x60000000);
    add_code_change(0x801b0554, 0x48000078);
    add_code_change(0x801af75c, 0x48000a34);
    // Enable strafing with left/right on L-Stick
    add_code_change(0x80188754, 0xc022a5a0);
    add_code_change(0x801883ec, 0x4800000c);
    // Grapple point yaw fix
    add_code_change(0x8011dbf8, 0x389d0054);
    add_code_change(0x8011dbfc, 0x4bf2d145);  // bl 8004ad40

    add_code_change(0x80015f74, 0x3aa00001, "show_crosshair"); // li r21, 1
    add_code_change(0x80015f78, 0x8add1268, "show_crosshair"); // lbz r22, 0x1268(r29)
    add_code_change(0x80015f7c, 0x52b63672, "show_crosshair"); // rlwimi r22, r21, 6, 25, 25 (00000001)
    add_code_change(0x80015f80, 0x9add1268, "show_crosshair"); // stb r22, 0x1268(r29)
    add_code_change(0x80015f84, 0x4e800020, "show_crosshair"); // blr

    add_code_change(0x800615f8, 0xc022d3c0); // not c022d3d8 ???
    const int null_players_vmc_idx = Core::System::GetInstance().GetPowerPC().RegisterVmcall(null_players_on_destruct_mp2_gc);
    u32 null_players_vmc = gen_vmcall(null_players_vmc_idx, 0);
    add_code_change(0x80042b04, null_players_vmc);
  } else {}
}

void wiimote_shake_override(PowerPC::PowerPCState& ppc_state, PowerPC::MMU&, u32) {
  ppc_state.gpr[26] = CheckJump() ? 1 : 0;
}

void FpsControls::init_mod_mp3(Region region) {
  prime::GetVariableManager()->register_variable("grapple_lasso_state");
  prime::GetVariableManager()->register_variable("grapple_hand_x");
  prime::GetVariableManager()->register_variable("grapple_hand_y");
  prime::GetVariableManager()->register_variable("trigger_grapple");
  prime::GetVariableManager()->register_variable("new_beam");
  prime::GetVariableManager()->register_variable("beamchange_flag");

  if (region == Region::NTSC_U) {
    add_code_change(0x80080ac0, 0xec010072);
    add_code_change(0x8014e094, 0x60000000);
    add_code_change(0x8014e06c, 0x60000000);
    add_code_change(0x80134328, 0x60000000);
    add_code_change(0x80133970, 0x60000000);
    add_code_change(0x8000ab58, 0x4bffad29);
    add_code_change(0x80080d44, 0x60000000);
    add_code_change(0x8007fdc8, 0x480000e4);
    add_code_change(0x8017f88c, 0x60000000);

    // Grapple Lasso
    add_grapple_lasso_code_mp3(0x800DDE64, 0x80170CF0, 0x80171AD8);

    add_control_state_hook_mp3(0x80005880, Region::NTSC_U);
    add_grapple_slide_code_mp3(0x8017f2a0);

    // Steps over bounds checking on the reticle
    add_code_change(0x80016f48, 0x48000120);
  } else if (region == Region::PAL) {
    add_code_change(0x80080ab8, 0xec010072);
    add_code_change(0x8014d9e0, 0x60000000);
    add_code_change(0x8014d9b8, 0x60000000);
    add_code_change(0x80133c74, 0x60000000);
    add_code_change(0x801332bc, 0x60000000);
    add_code_change(0x8000ab58, 0x4bffad29);
    add_code_change(0x80080d44, 0x60000000);
    add_code_change(0x8007fdc8, 0x480000e4);
    add_code_change(0x8017f1d8, 0x60000000);

    // Grapple Lasso
    add_grapple_lasso_code_mp3(0x800DDE44, 0x8017063C, 0x80171424);

    add_control_state_hook_mp3(0x80005880, Region::PAL);
    add_grapple_slide_code_mp3(0x8017ebec);

    // Steps over bounds checking on the reticle
    add_code_change(0x80016f48, 0x48000120);
  } else {}

  // Same for both.
  add_code_change(0x800614d0, 0x60000000, "visor_menu");
  const int wiimote_shake_override_idx = Core::System::GetInstance().GetPowerPC().RegisterVmcall(wiimote_shake_override);
  add_code_change(0x800a8fc0, gen_vmcall(wiimote_shake_override_idx, 0));
  has_beams = false;
}

void FpsControls::init_mod_mp3_standalone(Region region) {
  prime::GetVariableManager()->register_variable("grapple_lasso_state");
  prime::GetVariableManager()->register_variable("grapple_hand_x");
  prime::GetVariableManager()->register_variable("grapple_hand_y");
  prime::GetVariableManager()->register_variable("trigger_grapple");
  prime::GetVariableManager()->register_variable("new_beam");
  prime::GetVariableManager()->register_variable("beamchange_flag");
  if (region == Region::NTSC_U) {
    add_code_change(0x80080be8, 0xec010072);
    add_code_change(0x801521f0, 0x60000000);
    add_code_change(0x801521c8, 0x60000000);
    add_code_change(0x80139108, 0x60000000);
    add_code_change(0x80138750, 0x60000000);
    add_code_change(0x8000ae44, 0x4bffaa3d);
    add_code_change(0x80080e6c, 0x60000000);
    add_code_change(0x8007fef0, 0x480000e4);
    add_code_change(0x80183288, 0x60000000);

    add_code_change(0x800617c8, 0x60000000, "visor_menu");

    // Grapple Lasso
    add_grapple_lasso_code_mp3(0x800DF790, 0x80174D70, 0x80175B54);

    add_control_state_hook_mp3(0x80005880, Region::NTSC_U);
    add_grapple_slide_code_mp3(0x80182c9c);

    // Steps over bounds checking on the reticle
    add_code_change(0x80017290, 0x48000120);
    const int wiimote_shake_override_idx = Core::System::GetInstance().GetPowerPC().RegisterVmcall(wiimote_shake_override);
    add_code_change(0x800a8f40, gen_vmcall(wiimote_shake_override_idx, 0));
  } else if (region == Region::NTSC_J) {
    add_code_change(0x80081018, 0xec010072);
    add_code_change(0x80153ed4, 0x60000000);
    add_code_change(0x80153eac, 0x60000000);
    add_code_change(0x8013a054, 0x60000000);
    add_code_change(0x8013969c, 0x60000000);
    add_code_change(0x8000ae44, 0x4bffaa3d);
    add_code_change(0x8008129c, 0x60000000);
    add_code_change(0x80080320, 0x480000e4);
    add_code_change(0x80184fd4, 0x60000000);

    add_code_change(0x80075f0c, 0x80061958, "visor_menu");

    // Grapple Lasso
    add_grapple_lasso_code_mp3(0x800E003C, 0x80176B20, 0x80177908);

    add_control_state_hook_mp3(0x80005880, Region::NTSC_J);
    add_grapple_slide_code_mp3(0x801849e8);

    // Steps over bounds checking on the reticle
    add_code_change(0x80017258, 0x48000120);
  } else if (region == Region::PAL) {
    add_code_change(0x80080e84, 0xec010072);
    add_code_change(0x80152d50, 0x60000000);
    add_code_change(0x80152d28, 0x60000000);
    add_code_change(0x80139860, 0x60000000);
    add_code_change(0x80138ea8, 0x60000000);
    add_code_change(0x8000ae44, 0x4bffaa3d);
    add_code_change(0x80081108, 0x60000000);
    add_code_change(0x8008018c, 0x480000e4);
    add_code_change(0x80183dc8, 0x60000000);

    add_code_change(0x80061a88, 0x60000000, "visor_menu");

    // Grapple Lasso
    add_grapple_lasso_code_mp3(0x800DFC4C, 0x80175914, 0x801766FC);

    add_control_state_hook_mp3(0x80005880, Region::PAL);
    add_grapple_slide_code_mp3(0x801837dc);

    // Steps over bounds checking on the reticle
    add_code_change(0x80017258, 0x48000120);
  } else {}
  has_beams = false;
}
}
