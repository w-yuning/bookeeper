// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#include "ReminderDialog.h"

#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace ui {

// 构造对话框并立即创建控件。
ReminderDialog::ReminderDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle("提醒编辑");
  buildUi();
}

// 构建提醒表单及按钮。
void ReminderDialog::buildUi() {
  auto *layout = new QVBoxLayout(this);
  auto *form = new QFormLayout();

  messageEdit_ = new QLineEdit(this);
  form->addRow("内容", messageEdit_);

  timeEdit_ = new QDateTimeEdit(QDateTime::currentDateTime(), this);
  timeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
  form->addRow("提醒时间", timeEdit_);

  enabledCheck_ = new QCheckBox("启用", this);
  enabledCheck_->setChecked(true);
  form->addRow("状态", enabledCheck_);

  layout->addLayout(form);

  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &ReminderDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &ReminderDialog::reject);
  layout->addWidget(buttons);
}

// 将已有提醒信息加载到控件。
void ReminderDialog::setReminder(const core::Reminder &reminder) {
  reminder_ = reminder;
  messageEdit_->setText(reminder.message);
  timeEdit_->setDateTime(reminder.remindAt.isValid()
                             ? reminder.remindAt
                             : QDateTime::currentDateTime());
  enabledCheck_->setChecked(reminder.enabled);
}

// 汇总当前表单数据，返回提醒实体。
core::Reminder ReminderDialog::reminder() const {
  core::Reminder result = reminder_;
  result.message = messageEdit_->text().trimmed();
  result.remindAt = timeEdit_->dateTime();
  result.enabled = enabledCheck_->isChecked();
  return result;
}

}  // namespace ui
