// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#pragma once

#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QLineEdit>

#include "core/Entities.h"

namespace ui {

// ReminderDialog 负责提醒的新增与编辑。
class ReminderDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ReminderDialog(QWidget *parent = nullptr);

  void setReminder(const core::Reminder &reminder);
  core::Reminder reminder() const;

 private:
  void buildUi();

  QLineEdit *messageEdit_ = nullptr;
  QDateTimeEdit *timeEdit_ = nullptr;
  QCheckBox *enabledCheck_ = nullptr;
  core::Reminder reminder_;
};

}  // namespace ui
