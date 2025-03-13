// Copyright 2019 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/QtUtils/ModalMessageBox.h"

#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QString>

#include "DolphinQt/QtUtils/SetWindowDecorations.h"

ModalMessageBox::ModalMessageBox(QWidget* parent, Qt::WindowModality modality)
    : QMessageBox(parent != nullptr ? parent->window() : nullptr)
{
  setWindowModality(modality);
  setWindowFlags(Qt::Sheet | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

  // No parent is still preferable to showing a hidden parent here.
  if (parent != nullptr && !parent->window()->isVisible())
    setParent(nullptr);
}

static inline int ExecMessageBox(ModalMessageBox::Icon icon, QWidget* parent, const QString& title,
                                 const QString& text, ModalMessageBox::StandardButtons buttons,
                                 ModalMessageBox::StandardButton default_button,
                                 Qt::WindowModality modality)
{
  ModalMessageBox msg(parent, modality);
  msg.setIcon(icon);
  msg.setWindowTitle(title);
  msg.setText(text);
  msg.setStandardButtons(buttons);
  msg.setDefaultButton(default_button);

  SetQWidgetWindowDecorations(&msg);
  return msg.exec();
}

static inline int ExecPrimeHackMessage(QWidget* parent)
{
  ModalMessageBox msg(parent, Qt::WindowModal);
  msg.setIcon(QMessageBox::Information);
  msg.setWindowTitle(QString::fromStdString("PrimeHack"));
  msg.setTextFormat(Qt::RichText);
  msg.setText(QString::fromStdString(
    "<p>PrimeHack has detected this is your initial run. "
    "If you are new to Primehack, we highly recommend you press the <b>Default</b> button inside the controller profile window to ensure you are using the default PrimeHack controls."
    "</p><p>"
    "If you have any further questions, please see our wiki:<br>"
    "<a href='https://github.com/shiiion/dolphin/wiki'>https://github.com/shiiion/dolphin/wiki</a></p>"));
  msg.setStandardButtons(QMessageBox::Ok);
  msg.addButton(QMessageBox::Help);
  msg.setDefaultButton(QMessageBox::NoButton);

  return msg.exec();
}

static inline int ExecPrimeHackWiiTabMessage(QWidget* parent)
{
  ModalMessageBox msg(parent, Qt::WindowModal);
  msg.setIcon(QMessageBox::Information);
  msg.setWindowTitle(QString::fromStdString("PrimeHack"));
  msg.setTextFormat(Qt::RichText);
  msg.setText(QString::fromStdString(
    "<p>Unless you have a specific reason to use the 'Emulated Wii Remote' window, "
    "such as alternate control schemes that deal with motion inputs, we highly recommend you use the Metroid controller window instead. "
    "Both windows will work, but the Metroid window is specialised to make configuring your controls far easier."
    "</p><p>"
    "If you would like to go to the Metroid controller window instead, press <b>Open</b>.<br>"));
  msg.setStandardButtons(QMessageBox::StandardButton::Ignore);
  msg.addButton(QMessageBox::StandardButton::Open);
  msg.setDefaultButton(QMessageBox::NoButton);

  return msg.exec();
}

static inline int ExecPrimeHackGCTabMessage(QWidget* parent)
{
  ModalMessageBox msg(parent, Qt::WindowModal);
  msg.setIcon(QMessageBox::Information);
  msg.setWindowTitle(QString::fromStdString("PrimeHack"));
  msg.setTextFormat(Qt::RichText);
  msg.setText(QString::fromStdString(
    "<p>Unless you have a specific reason to use the 'Standard Controller' window, "
    " we highly recommend you use the Metroid Controller window instead. "
    "Both windows will work, but the Metroid window is specialised to make configuring your controls far easier."
    "</p><p>"
    "If you would like to go to the Metroid controller window instead, press <b>Open</b>.<br>"));
  msg.setStandardButtons(QMessageBox::StandardButton::Ignore);
  msg.addButton(QMessageBox::StandardButton::Open);
  msg.setDefaultButton(QMessageBox::NoButton);

  return msg.exec();
}

void ModalMessageBox::primehack_initialrun(QWidget* parent)
{
  if (ExecPrimeHackMessage(parent) == QMessageBox::Help) {
    QDesktopServices::openUrl(QUrl(QString::fromStdString("https://github.com/shiiion/dolphin/wiki/Installation")));
  }
}

bool ModalMessageBox::primehack_wiitab(QWidget* parent)
{
  if (ExecPrimeHackWiiTabMessage(parent) == QMessageBox::Open) {
    return true;
  }

  return false;
}

bool ModalMessageBox::primehack_gctab(QWidget* parent)
{
  if (ExecPrimeHackGCTabMessage(parent) == QMessageBox::Open) {
    return true;
  }

  return false;
}

int ModalMessageBox::critical(QWidget* parent, const QString& title, const QString& text,
                              StandardButtons buttons, StandardButton default_button,
                              Qt::WindowModality modality)
{
  return ExecMessageBox(QMessageBox::Critical, parent, title, text, buttons, default_button,
                        modality);
}

int ModalMessageBox::information(QWidget* parent, const QString& title, const QString& text,
                                 StandardButtons buttons, StandardButton default_button,
                                 Qt::WindowModality modality)
{
  return ExecMessageBox(QMessageBox::Information, parent, title, text, buttons, default_button,
                        modality);
}

int ModalMessageBox::primehackInitial()
{
	return 0;
}

int ModalMessageBox::question(QWidget* parent, const QString& title, const QString& text,
                              StandardButtons buttons, StandardButton default_button,
                              Qt::WindowModality modality)
{
  return ExecMessageBox(QMessageBox::Warning, parent, title, text, buttons, default_button,
                        modality);
}

int ModalMessageBox::warning(QWidget* parent, const QString& title, const QString& text,
                             StandardButtons buttons, StandardButton default_button,
                             Qt::WindowModality modality)
{
  return ExecMessageBox(QMessageBox::Warning, parent, title, text, buttons, default_button,
                        modality);
}
