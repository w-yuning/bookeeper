// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>

#include "core/LedgerService.h"

namespace ui {

// LoginWindow 负责用户登录与注册入口流程。
class LoginWindow : public QDialog {
  Q_OBJECT

 public:
  explicit LoginWindow(core::LedgerService *service, QWidget *parent = nullptr);

 signals:
  void authenticated(const core::UserProfile &profile);

 private:
  void buildUi();
  void connectSignals();
  void handleLogin();
  void handleRegister();

  core::LedgerService *service_ = nullptr;

  QTabWidget *tabWidget_ = nullptr;
  QWidget *loginPage_ = nullptr;
  QWidget *registerPage_ = nullptr;

  QLineEdit *loginUserEdit_ = nullptr;
  QLineEdit *loginPasswordEdit_ = nullptr;
  QLabel *loginStatusLabel_ = nullptr;

  QLineEdit *registerNameEdit_ = nullptr;
  QLineEdit *registerEmailEdit_ = nullptr;
  QLineEdit *registerPasswordEdit_ = nullptr;
  QLineEdit *registerConfirmEdit_ = nullptr;
  QLabel *registerStatusLabel_ = nullptr;
};

}  // namespace ui
