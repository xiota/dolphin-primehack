// Copyright 2017 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <QWidget>

class GraphicsWindow;
class QCheckBox;
class QLabel;
class QRadioButton;
class QSlider;
class QSpinBox;
class ConfigBool;
class ConfigSlider;
class ConfigInteger;
class ConfigRadioInt;
class QPushButton;
class QColorDialog;

class PrimeWidget final : public QWidget {
  Q_OBJECT
public:
  explicit PrimeWidget(GraphicsWindow* parent);

private:
  void ToggleShowCrosshair(bool mode);
  void ArmPositionModeChanged(bool mode);

  // Misc
  ConfigBool* m_disable_bloom;
  ConfigBool* m_reduce_bloom;
  ConfigSlider* m_bloom_intensity;
  ConfigInteger* bloom_intensity_val;
  ConfigBool* m_motions_lock;
  ConfigBool* m_toggle_culling;
  ConfigBool* m_toggle_secondaryFX;

  ConfigInteger* fov_counter;
  ConfigSlider* m_fov_axis;

  ConfigBool* m_toggle_gc_show_crosshair;
  QPushButton* m_select_colour;
  QPushButton* m_reset_colour;
  QColorDialog* colorpicker;

  ConfigInteger* x_counter;
  ConfigInteger* y_counter;
  ConfigInteger* z_counter;

  ConfigSlider* m_x_axis;
  ConfigSlider* m_y_axis;
  ConfigSlider* m_z_axis;

  ConfigBool* m_toggle_arm_position;
  ConfigRadioInt* m_auto_arm_position;
  ConfigRadioInt* m_manual_arm_position;

  void CreateWidgets();
  void ConnectWidgets();
  void AddDescriptions();
};
