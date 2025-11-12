// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#pragma once

#include <QComboBox>
#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>

#include <QtCharts/QChartView>

#include "core/LedgerService.h"
#include "ui/BillEditorDialog.h"
#include "ui/ReminderDialog.h"

namespace ui {

// MainWindow 集成仪表盘、账单、分类、提醒与社交等多个模块界面。
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(core::LedgerService *service, const core::UserProfile &profile,
             QWidget *parent = nullptr);

 private slots:
  void handleNavigation(int row);
  void handleAddBill();
  void handleEditBill();
  void handleDeleteBill();
  void handleAddCategory();
  void handleDeleteCategory();
  void handleAddReminder();
  void handleEditReminder();
  void handleDeleteReminder();
  void handlePublishPost();
  void handleRefreshTimeline();
  void handleAddFriend();
  void handleComment();

 private:
  void buildUi();
  void setupDashboardPage();
  void setupBillsPage();
  void setupCategoryPage();
  void setupReminderPage();
  void setupSocialPage();

  void refreshDashboard();
  void refreshBills();
  void refreshCategories();
  void refreshReminders();
  void refreshTimeline();
  void refreshReminderNotifications();

  core::LedgerService *service_ = nullptr;
  core::UserProfile profile_;

  QListWidget *navList_ = nullptr;
  QStackedWidget *stacked_ = nullptr;

  QWidget *dashboardPage_ = nullptr;
  QWidget *billsPage_ = nullptr;
  QWidget *categoryPage_ = nullptr;
  QWidget *reminderPage_ = nullptr;
  QWidget *socialPage_ = nullptr;

  QLabel *totalIncomeLabel_ = nullptr;
  QLabel *totalExpenseLabel_ = nullptr;
  QtCharts::QChartView *pieChartView_ = nullptr;
  QtCharts::QChartView *barChartView_ = nullptr;

  QTableWidget *billTable_ = nullptr;

  QListWidget *categoryList_ = nullptr;
  QLineEdit *categoryNameEdit_ = nullptr;
  QComboBox *categoryTypeCombo_ = nullptr;

  QListWidget *reminderList_ = nullptr;

  QPlainTextEdit *postEdit_ = nullptr;
  QComboBox *visibilityCombo_ = nullptr;
  QListWidget *timelineList_ = nullptr;
  QLineEdit *friendEdit_ = nullptr;

  QTimer *reminderTimer_ = nullptr;
  QDateTime lastReminderCheck_;
};

}  // namespace ui
