// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#include "LoginWindow.h"

#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace ui {

// 构造函数注入服务实例，供注册与登录使用。
LoginWindow::LoginWindow(core::LedgerService *service, QWidget *parent)
    : QDialog(parent), service_(service) {
  setWindowTitle("Bookeeper 登录");
  setModal(true);
  buildUi();
  connectSignals();
}

// 构建登录与注册两个标签页。
void LoginWindow::buildUi() {
  tabWidget_ = new QTabWidget(this);

  loginPage_ = new QWidget(tabWidget_);
  registerPage_ = new QWidget(tabWidget_);

  // 登录页
  {
    auto *layout = new QVBoxLayout(loginPage_);
    auto *form = new QFormLayout();
    loginUserEdit_ = new QLineEdit(loginPage_);
    loginPasswordEdit_ = new QLineEdit(loginPage_);
    loginPasswordEdit_->setEchoMode(QLineEdit::Password);
    form->addRow("用户名/邮箱", loginUserEdit_);
    form->addRow("密码", loginPasswordEdit_);
    layout->addLayout(form);

    auto *button = new QPushButton("登录", loginPage_);
    layout->addWidget(button);
    loginStatusLabel_ = new QLabel(loginPage_);
    layout->addWidget(loginStatusLabel_);
    layout->addStretch();
    button->setDefault(true);
    connect(button, &QPushButton::clicked, this, &LoginWindow::handleLogin);
  }

  // 注册页
  {
    auto *layout = new QVBoxLayout(registerPage_);
    auto *form = new QFormLayout();
    registerNameEdit_ = new QLineEdit(registerPage_);
    registerEmailEdit_ = new QLineEdit(registerPage_);
    registerPasswordEdit_ = new QLineEdit(registerPage_);
    registerConfirmEdit_ = new QLineEdit(registerPage_);
    registerPasswordEdit_->setEchoMode(QLineEdit::Password);
    registerConfirmEdit_->setEchoMode(QLineEdit::Password);
    form->addRow("用户名", registerNameEdit_);
    form->addRow("邮箱", registerEmailEdit_);
    form->addRow("密码", registerPasswordEdit_);
    form->addRow("确认密码", registerConfirmEdit_);
    layout->addLayout(form);

    auto *button = new QPushButton("创建账户", registerPage_);
    layout->addWidget(button);
    registerStatusLabel_ = new QLabel(registerPage_);
    layout->addWidget(registerStatusLabel_);
    layout->addStretch();
    connect(button, &QPushButton::clicked, this, &LoginWindow::handleRegister);
  }

  tabWidget_->addTab(loginPage_, "登录");
  tabWidget_->addTab(registerPage_, "注册");

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(tabWidget_);
  setLayout(mainLayout);
}

// 绑定回车快捷操作，提高可用性。
void LoginWindow::connectSignals() {
  connect(loginUserEdit_, &QLineEdit::returnPressed, this,
          &LoginWindow::handleLogin);
  connect(loginPasswordEdit_, &QLineEdit::returnPressed, this,
          &LoginWindow::handleLogin);
}

// 登录流程：调用服务验证并发送成功信号。
void LoginWindow::handleLogin() {
  if (!service_) {
    return;
  }
  QString message;
  auto profile = service_->authenticate(loginUserEdit_->text().trimmed(),
                                        loginPasswordEdit_->text(), message);
  if (!profile.has_value()) {
    loginStatusLabel_->setText(message);
    return;
  }
  loginStatusLabel_->setText("登录成功");
  emit authenticated(profile.value());
  accept();
}

// 注册流程：校验必填项后调用服务创建账户。
void LoginWindow::handleRegister() {
  if (!service_) {
    return;
  }
  const auto username = registerNameEdit_->text().trimmed();
  const auto email = registerEmailEdit_->text().trimmed();
  const auto password = registerPasswordEdit_->text();
  const auto confirm = registerConfirmEdit_->text();

  if (username.isEmpty() || email.isEmpty() || password.isEmpty()) {
    registerStatusLabel_->setText("请完整填写信息");
    return;
  }
  if (password != confirm) {
    registerStatusLabel_->setText("两次密码不一致");
    return;
  }

  QString message;
  QString userId;
  if (!service_->registerUser(username, email, password, userId, message)) {
    registerStatusLabel_->setText(message);
    return;
  }

  registerStatusLabel_->setText("注册成功，请登录");
  tabWidget_->setCurrentWidget(loginPage_);
  loginUserEdit_->setText(username);
  loginPasswordEdit_->setFocus();
}

}  // namespace ui
