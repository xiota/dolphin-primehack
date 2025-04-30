// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/HW/WiimoteEmu/Extension/Nunchuk.h"

#include <algorithm>
#include <array>
#include <cstring>

#include "Common/Assert.h"
#include "Common/BitUtils.h"
#include "Common/Common.h"
#include "Common/CommonTypes.h"
#include "Common/MathUtil.h"

#include "Core/HW/Wiimote.h"
#include "Core/HW/WiimoteEmu/Extension/DesiredExtensionState.h"
#include "Core/HW/WiimoteEmu/WiimoteEmu.h"

#include "InputCommon/ControllerEmu/Control/Input.h"
#include "InputCommon/ControllerEmu/ControlGroup/AnalogStick.h"
#include "InputCommon/ControllerEmu/ControlGroup/Buttons.h"
#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerEmu/ControlGroup/Force.h"
#include "InputCommon/ControllerEmu/ControlGroup/IMUAccelerometer.h"
#include "InputCommon/ControllerEmu/ControlGroup/Tilt.h"

namespace WiimoteEmu
{
constexpr std::array<u8, 6> nunchuk_id{{0x00, 0x00, 0xa4, 0x20, 0x00, 0x00}};

constexpr std::array<u8, 2> nunchuk_button_bitmasks{{
    Nunchuk::BUTTON_C,
    Nunchuk::BUTTON_Z,
}};

Nunchuk::Nunchuk() : Extension1stParty(_trans("Nunchuk"))
{
  // buttons
  groups.emplace_back(m_buttons = new ControllerEmu::Buttons(BUTTONS_GROUP));
  using Translatability = ControllerEmu::Translatability;
  m_buttons->AddInput(Translatability::DoNotTranslate, C_BUTTON);
  m_buttons->AddInput(Translatability::DoNotTranslate, Z_BUTTON);

  // stick
  constexpr auto gate_radius = ControlState(STICK_GATE_RADIUS) / STICK_RADIUS;
  groups.emplace_back(m_stick = new ControllerEmu::OctagonAnalogStick(STICK_GROUP, gate_radius));

  // Shake
  // Inverse the default intensity so shake is opposite that of wiimote.
  // This is needed by Donkey Kong Country Returns for proper shake action detection.
  groups.emplace_back(m_shake = new ControllerEmu::Shake(_trans("Shake"), -1));

  // tilt
  groups.emplace_back(m_tilt = new ControllerEmu::Tilt(_trans("Tilt")));

  // swing
  groups.emplace_back(m_swing = new ControllerEmu::Force(_trans("Swing")));

  // accelerometer
  groups.emplace_back(m_imu_accelerometer = new ControllerEmu::IMUAccelerometer(
                          ACCELEROMETER_GROUP, _trans("Accelerometer")));
}

void Nunchuk::BuildDesiredExtensionState(DesiredExtensionState* target_state)
{
  DataFormat nc_data = {};

  // stick
  bool override_occurred = false;
  const ControllerEmu::AnalogStick::StateData stick_state =
      m_stick->GetState(m_input_override_function, &override_occurred);
  nc_data.jx = MapFloat<u8>(stick_state.x, STICK_CENTER, 0, STICK_RANGE);
  nc_data.jy = MapFloat<u8>(stick_state.y, STICK_CENTER, 0, STICK_RANGE);

  if (!override_occurred)
  {
    // Some terribly coded games check whether to move with a check like
    //
    //     if (x != 0 && y != 0)
    //         do_movement(x, y);
    //
    // With keyboard controls, these games break if you simply hit one
    // of the axes. Adjust this if you're hitting one of the axes so that
    // we slightly tweak the other axis.
    if (nc_data.jx != STICK_CENTER || nc_data.jy != STICK_CENTER)
    {
      if (nc_data.jx == STICK_CENTER)
        ++nc_data.jx;
      if (nc_data.jy == STICK_CENTER)
        ++nc_data.jy;
    }
  }

  // buttons
  u8 buttons = 0;
  m_buttons->GetState(&buttons, nunchuk_button_bitmasks.data(), m_input_override_function);
  nc_data.SetButtons(buttons);

  // Acceleration data:
  EmulateSwing(&m_swing_state, m_swing, 1.f / ::Wiimote::UPDATE_FREQ);
  EmulateTilt(&m_tilt_state, m_tilt, 1.f / ::Wiimote::UPDATE_FREQ);
  EmulateShake(&m_shake_state, m_shake, 1.f / ::Wiimote::UPDATE_FREQ);

  const auto transformation =
      GetRotationalMatrix(-m_tilt_state.angle) * GetRotationalMatrix(-m_swing_state.angle);

  Common::Vec3 accel =
      transformation *
      (m_swing_state.acceleration +
       m_imu_accelerometer->GetState().value_or(Common::Vec3(0, 0, float(GRAVITY_ACCELERATION))));

  // shake
  accel += m_shake_state.acceleration;

  accel = Wiimote::OverrideVec3(m_imu_accelerometer, accel, m_input_override_function);

  // Calibration values are 8-bit but we want 10-bit precision, so << 2.
  const auto acc = ConvertAccelData(accel, ACCEL_ZERO_G << 2, ACCEL_ONE_G << 2);
  nc_data.SetAccel(acc.value);

  target_state->data = nc_data;
}

void Nunchuk::Update(const DesiredExtensionState& target_state)
{
  DefaultExtensionUpdate<DataFormat>(&m_reg, target_state);
}

void Nunchuk::Reset()
{
  EncryptedExtension::Reset();

  m_reg.identifier = nunchuk_id;

  m_swing_state = {};
  m_tilt_state = {};

  // Build calibration data:
  m_reg.calibration = {{
      // Accel Zero X,Y,Z:
      ACCEL_ZERO_G,
      ACCEL_ZERO_G,
      ACCEL_ZERO_G,
      // Possibly LSBs of zero values:
      0x00,
      // Accel 1G X,Y,Z:
      ACCEL_ONE_G,
      ACCEL_ONE_G,
      ACCEL_ONE_G,
      // Possibly LSBs of 1G values:
      0x00,
      // Stick X max,min,center:
      STICK_CENTER + STICK_GATE_RADIUS,
      STICK_CENTER - STICK_GATE_RADIUS,
      STICK_CENTER,
      // Stick Y max,min,center:
      STICK_CENTER + STICK_GATE_RADIUS,
      STICK_CENTER - STICK_GATE_RADIUS,
      STICK_CENTER,
      // 2 checksum bytes calculated below:
      0x00,
      0x00,
  }};

  UpdateCalibrationDataChecksum(m_reg.calibration, CALIBRATION_CHECKSUM_BYTES);
}

ControllerEmu::ControlGroup* Nunchuk::GetGroup(NunchukGroup group)
{
  switch (group)
  {
  case NunchukGroup::Buttons:
    return m_buttons;
  case NunchukGroup::Stick:
    return m_stick;
  case NunchukGroup::Tilt:
    return m_tilt;
  case NunchukGroup::Swing:
    return m_swing;
  case NunchukGroup::Shake:
    return m_shake;
  case NunchukGroup::IMUAccelerometer:
    return m_imu_accelerometer;
  default:
    ASSERT(false);
    return nullptr;
  }
}

void Nunchuk::DoState(PointerWrap& p)
{
  EncryptedExtension::DoState(p);

  p.Do(m_swing_state);
  p.Do(m_tilt_state);
  p.Do(m_shake_state);
}

void Nunchuk::ChangeUIPrimeHack(bool useMetroidUI)
{
  if (using_metroid_ui == useMetroidUI)
    return;

  m_buttons->controls[0]->ui_name = useMetroidUI ? "Morph Ball" : "C";
  m_buttons->controls[0]->display_alt = useMetroidUI;
  m_buttons->controls[1]->ui_name = useMetroidUI ? "Lock-On" : "Z";
  m_buttons->controls[1]->display_alt = useMetroidUI;

  using_metroid_ui = useMetroidUI;
}

// Button Defaults
void Nunchuk::LoadDefaults(const ControllerInterface& ciface)
{
  // Movement
  m_stick->SetControlExpression(0, "W | UP");     // Push
  m_stick->SetControlExpression(1, "S | DOWN");   // Pull
  m_stick->SetControlExpression(2, "A | LEFT");   // Tilt left
  m_stick->SetControlExpression(3, "D | RIGHT");  // Tilt right

  // Because our defaults use keyboard input, set calibration shape to a square.
  m_stick->SetCalibrationFromGate(ControllerEmu::SquareStickGate(1.0));

#ifdef _WIN32
  m_buttons->SetControlExpression(1, "`Click 1`"); // Lock-On
#else
  m_buttons->SetControlExpression(1, "`Click 3`"); // Lock-On
#endif

  m_buttons->SetControlExpression(0, "Ctrl"); // Morph

  // Shake (Only used in Prime 3, may need revision
  m_shake->SetControlExpression(1, "LSHIFT & (`Axis Y-` | `Axis Y+` | `Axis X-` | `Axis X+`)");
}
}  // namespace WiimoteEmu
