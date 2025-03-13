// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>

#include "Common/Common.h"
#include "Common/CommonTypes.h"

#include <tuple>

class InputConfig;
class PointerWrap;

namespace ControllerEmu
{
class ControlGroup;
}

namespace WiimoteEmu
{
enum class WiimoteGroup;
enum class NunchukGroup;
enum class ClassicGroup;
enum class GuitarGroup;
enum class DrumsGroup;
enum class TurntableGroup;
enum class UDrawTabletGroup;
enum class DrawsomeTabletGroup;
enum class TaTaConGroup;
enum class ShinkansenGroup;
}  // namespace WiimoteEmu

enum
{
  WIIMOTE_CHAN_0 = 0,
  WIIMOTE_CHAN_1,
  WIIMOTE_CHAN_2,
  WIIMOTE_CHAN_3,
  WIIMOTE_BALANCE_BOARD,
  MAX_WIIMOTES = WIIMOTE_BALANCE_BOARD,
  MAX_BBMOTES = 5,
};

#define WIIMOTE_INI_NAME "WiimoteNew"

enum class WiimoteSource
{
  None = 0,
  Emulated = 1,
  Real = 2,
  Metroid = 3,
};

namespace WiimoteCommon
{
class HIDWiimote;

// Used to reconnect WiimoteDevice instance to HID source.
// Must be run from CPU thread.
void UpdateSource(unsigned int index);

HIDWiimote* GetHIDWiimoteSource(unsigned int index);

}  // namespace WiimoteCommon

namespace Wiimote
{
enum class InitializeMode
{
  DO_WAIT_FOR_WIIMOTES,
  DO_NOT_WAIT_FOR_WIIMOTES,
};

// The Real Wii Remote sends report every ~5ms (200 Hz).
constexpr int UPDATE_FREQ = 200;

void Shutdown();
void Initialize(InitializeMode init_mode);
void ResetAllWiimotes();
void LoadConfig();
void Resume();
void Pause();

void DoState(PointerWrap& p);
InputConfig* GetConfig();
ControllerEmu::ControlGroup* GetWiimoteGroup(int number, WiimoteEmu::WiimoteGroup group);
ControllerEmu::ControlGroup* GetNunchukGroup(int number, WiimoteEmu::NunchukGroup group);
ControllerEmu::ControlGroup* GetClassicGroup(int number, WiimoteEmu::ClassicGroup group);
ControllerEmu::ControlGroup* GetGuitarGroup(int number, WiimoteEmu::GuitarGroup group);
ControllerEmu::ControlGroup* GetDrumsGroup(int number, WiimoteEmu::DrumsGroup group);
ControllerEmu::ControlGroup* GetTurntableGroup(int number, WiimoteEmu::TurntableGroup group);
ControllerEmu::ControlGroup* GetUDrawTabletGroup(int number, WiimoteEmu::UDrawTabletGroup group);
ControllerEmu::ControlGroup* GetDrawsomeTabletGroup(int number,
                                                    WiimoteEmu::DrawsomeTabletGroup group);
ControllerEmu::ControlGroup* GetTaTaConGroup(int number, WiimoteEmu::TaTaConGroup group);
ControllerEmu::ControlGroup* GetShinkansenGroup(int number, WiimoteEmu::ShinkansenGroup group);

void ChangeUIPrimeHack(int number, bool useMetroidUI);

bool CheckVisor(int visor_count);
bool CheckBeam(int beam_count);
bool CheckBeamScroll(bool direction);
bool CheckVisorScroll(bool direction);
bool CheckSpringBall();
bool CheckImprovedMotions();
bool CheckForward();
bool CheckBack();
bool CheckLeft();
bool CheckRight();
bool CheckJump();

bool CheckGrapple();
bool UseGrappleTapping();
bool GrappleCtlBound();
bool PrimeUseController();

bool PrimeUseController();
bool CheckPitchRecentre();

std::tuple<double, double> GetPrimeStickXY();
std::tuple<bool, bool> GetBVMenuOptions();

std::tuple<double, double, bool, bool, bool, bool, bool> PrimeSettings();
WiimoteSource GetSource(unsigned int index);
}  // namespace Wiimote

namespace WiimoteReal
{
void Initialize(::Wiimote::InitializeMode init_mode);
void Stop();
void Shutdown();
void Resume();
void Pause();
void Refresh();

}  // namespace WiimoteReal
