// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#include <QApplication>

#include "core/LedgerService.h"
#include "ui/LoginWindow.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  // 初始化应用基本信息，确保 Qt 能正确定位配置与数据目录
  QApplication::setOrganizationName("BookeeperLab");
  QApplication::setApplicationName("Bookeeper");

  core::LedgerService service;

  // 登录窗口负责注册/鉴权，成功后通过信号返回用户信息
  ui::LoginWindow login(&service);
  core::UserProfile profile;
  QObject::connect(
      &login, &ui::LoginWindow::authenticated,
      [&](const core::UserProfile &current) { profile = current; });

  if (login.exec() != QDialog::Accepted || profile.id.isEmpty()) {
    return 0;
  }

  // 登录成功后启动主界面，主界面托管全部业务模块
  ui::MainWindow mainWindow(&service, profile);
  mainWindow.show();
  return app.exec();
}
