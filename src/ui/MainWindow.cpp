// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#include "MainWindow.h"

#include <QAbstractItemView>
#include <QDate>
#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QInputDialog>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariantMap>
#include <QtCharts>

#include <algorithm>

namespace ui {

// 构造器初始化主界面并立即加载业务数据。
MainWindow::MainWindow(core::LedgerService *service,
                       const core::UserProfile &profile, QWidget *parent)
    : QMainWindow(parent), service_(service), profile_(profile) {
  setWindowTitle(QString("Bookeeper - %1").arg(profile.username));
  resize(1080, 720);
  buildUi();
  refreshDashboard();
  refreshBills();
  refreshCategories();
  refreshReminders();
  refreshTimeline();

  reminderTimer_ = new QTimer(this);
  reminderTimer_->setInterval(60 * 1000);
  lastReminderCheck_ = QDateTime::currentDateTimeUtc().addSecs(-300);
  connect(reminderTimer_, &QTimer::timeout, this,
          &MainWindow::refreshReminderNotifications);
  reminderTimer_->start();

  refreshReminderNotifications();
}

// 构建左侧导航与右侧堆叠页面。
void MainWindow::buildUi() {
  auto *central = new QWidget(this);
  auto *layout = new QHBoxLayout(central);

  navList_ = new QListWidget(central);
  navList_->addItems({"概览", "账单", "分类", "提醒", "社交"});
  navList_->setFixedWidth(150);
  navList_->setCurrentRow(0);

  stacked_ = new QStackedWidget(central);

  setupDashboardPage();
  setupBillsPage();
  setupCategoryPage();
  setupReminderPage();
  setupSocialPage();

  stacked_->addWidget(dashboardPage_);
  stacked_->addWidget(billsPage_);
  stacked_->addWidget(categoryPage_);
  stacked_->addWidget(reminderPage_);
  stacked_->addWidget(socialPage_);

  layout->addWidget(navList_);
  layout->addWidget(stacked_, 1);

  setCentralWidget(central);

  connect(navList_, &QListWidget::currentRowChanged, this,
          &MainWindow::handleNavigation);
}

// 仪表盘页面展示汇总数据与图表。
void MainWindow::setupDashboardPage() {
  dashboardPage_ = new QWidget(stacked_);
  auto *layout = new QVBoxLayout(dashboardPage_);

  totalIncomeLabel_ = new QLabel(dashboardPage_);
  totalExpenseLabel_ = new QLabel(dashboardPage_);
  totalIncomeLabel_->setObjectName("totalIncomeLabel");
  totalExpenseLabel_->setObjectName("totalExpenseLabel");

  layout->addWidget(totalIncomeLabel_);
  layout->addWidget(totalExpenseLabel_);

  pieChartView_ = new QtCharts::QChartView(dashboardPage_);
  pieChartView_->setRenderHint(QPainter::Antialiasing);
  layout->addWidget(pieChartView_, 1);

  barChartView_ = new QtCharts::QChartView(dashboardPage_);
  barChartView_->setRenderHint(QPainter::Antialiasing);
  layout->addWidget(barChartView_, 1);
}

// 账单页面用于列表展示与增删改。
void MainWindow::setupBillsPage() {
  billsPage_ = new QWidget(stacked_);
  auto *layout = new QVBoxLayout(billsPage_);

  billTable_ = new QTableWidget(billsPage_);
  billTable_->setColumnCount(5);
  billTable_->setHorizontalHeaderLabels(
      {"时间", "分类", "类型", "金额", "备注"});
  billTable_->horizontalHeader()->setStretchLastSection(true);
  billTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  billTable_->setSelectionMode(QAbstractItemView::SingleSelection);
  billTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  layout->addWidget(billTable_, 1);

  auto *buttonRow = new QHBoxLayout();
  auto *addBtn = new QPushButton("新增", billsPage_);
  auto *editBtn = new QPushButton("编辑", billsPage_);
  auto *deleteBtn = new QPushButton("删除", billsPage_);
  buttonRow->addWidget(addBtn);
  buttonRow->addWidget(editBtn);
  buttonRow->addWidget(deleteBtn);
  buttonRow->addStretch();
  layout->addLayout(buttonRow);

  connect(addBtn, &QPushButton::clicked, this, &MainWindow::handleAddBill);
  connect(editBtn, &QPushButton::clicked, this, &MainWindow::handleEditBill);
  connect(deleteBtn, &QPushButton::clicked, this,
          &MainWindow::handleDeleteBill);
}

// 分类页面支持新增与删除自定义分类。
void MainWindow::setupCategoryPage() {
  categoryPage_ = new QWidget(stacked_);
  auto *layout = new QVBoxLayout(categoryPage_);

  categoryList_ = new QListWidget(categoryPage_);
  layout->addWidget(categoryList_, 1);

  auto *form = new QFormLayout();
  categoryNameEdit_ = new QLineEdit(categoryPage_);
  categoryTypeCombo_ = new QComboBox(categoryPage_);
  categoryTypeCombo_->addItem("支出", "expense");
  categoryTypeCombo_->addItem("收入", "income");
  form->addRow("名称", categoryNameEdit_);
  form->addRow("类型", categoryTypeCombo_);
  layout->addLayout(form);

  auto *buttonRow = new QHBoxLayout();
  auto *addBtn = new QPushButton("新增分类", categoryPage_);
  auto *deleteBtn = new QPushButton("删除分类", categoryPage_);
  buttonRow->addWidget(addBtn);
  buttonRow->addWidget(deleteBtn);
  buttonRow->addStretch();
  layout->addLayout(buttonRow);

  connect(addBtn, &QPushButton::clicked, this, &MainWindow::handleAddCategory);
  connect(deleteBtn, &QPushButton::clicked, this,
          &MainWindow::handleDeleteCategory);
}

// 提醒页面管理提醒列表与编辑。
void MainWindow::setupReminderPage() {
  reminderPage_ = new QWidget(stacked_);
  auto *layout = new QVBoxLayout(reminderPage_);

  reminderList_ = new QListWidget(reminderPage_);
  layout->addWidget(reminderList_, 1);

  auto *buttonRow = new QHBoxLayout();
  auto *addBtn = new QPushButton("新增提醒", reminderPage_);
  auto *editBtn = new QPushButton("编辑提醒", reminderPage_);
  auto *deleteBtn = new QPushButton("删除提醒", reminderPage_);
  buttonRow->addWidget(addBtn);
  buttonRow->addWidget(editBtn);
  buttonRow->addWidget(deleteBtn);
  buttonRow->addStretch();
  layout->addLayout(buttonRow);

  connect(addBtn, &QPushButton::clicked, this, &MainWindow::handleAddReminder);
  connect(editBtn, &QPushButton::clicked, this,
          &MainWindow::handleEditReminder);
  connect(deleteBtn, &QPushButton::clicked, this,
          &MainWindow::handleDeleteReminder);
}

// 社交页面提供好友、动态与评论功能。
void MainWindow::setupSocialPage() {
  socialPage_ = new QWidget(stacked_);
  auto *layout = new QVBoxLayout(socialPage_);

  auto *friendRow = new QHBoxLayout();
  friendEdit_ = new QLineEdit(socialPage_);
  friendEdit_->setPlaceholderText("输入好友用户名或邮箱");
  auto *friendBtn = new QPushButton("添加好友", socialPage_);
  friendRow->addWidget(friendEdit_);
  friendRow->addWidget(friendBtn);
  layout->addLayout(friendRow);

  postEdit_ = new QPlainTextEdit(socialPage_);
  postEdit_->setPlaceholderText("分享你的记账心得...");
  layout->addWidget(postEdit_, 1);

  auto *postRow = new QHBoxLayout();
  visibilityCombo_ = new QComboBox(socialPage_);
  visibilityCombo_->addItem("公开", "public");
  visibilityCombo_->addItem("仅好友", "friends");
  auto *publishBtn = new QPushButton("发布动态", socialPage_);
  auto *refreshBtn = new QPushButton("刷新", socialPage_);
  auto *commentBtn = new QPushButton("评论选中动态", socialPage_);
  postRow->addWidget(visibilityCombo_);
  postRow->addWidget(publishBtn);
  postRow->addWidget(refreshBtn);
  postRow->addWidget(commentBtn);
  postRow->addStretch();
  layout->addLayout(postRow);

  timelineList_ = new QListWidget(socialPage_);
  layout->addWidget(timelineList_, 2);

  connect(publishBtn, &QPushButton::clicked, this,
          &MainWindow::handlePublishPost);
  connect(refreshBtn, &QPushButton::clicked, this,
          &MainWindow::handleRefreshTimeline);
  connect(friendBtn, &QPushButton::clicked, this, &MainWindow::handleAddFriend);
  connect(commentBtn, &QPushButton::clicked, this, &MainWindow::handleComment);
}

// 切换堆叠页面索引。
void MainWindow::handleNavigation(int row) {
  if (row >= 0 && row < stacked_->count()) {
    stacked_->setCurrentIndex(row);
  }
}

// 新增账单，需先确认已有分类。
void MainWindow::handleAddBill() {
  auto categoriesData = service_->categories(profile_.id);
  if (categoriesData.isEmpty()) {
    QMessageBox::warning(this, "提示", "请先创建分类");
    return;
  }

  BillEditorDialog dialog(this);
  dialog.setCategories(categoriesData);
  core::Bill bill;
  bill.type = core::BillType::Expense;
  dialog.setBill(bill);
  if (dialog.exec() == QDialog::Accepted) {
    QString message;
    if (!service_->upsertBill(profile_.id, dialog.bill(), message)) {
      QMessageBox::warning(this, "失败", message);
      return;
    }
    refreshBills();
    refreshDashboard();
  }
}

// 编辑当前选中账单。
void MainWindow::handleEditBill() {
  const auto selected = billTable_->currentRow();
  if (selected < 0) {
    return;
  }
  const auto billId =
      billTable_->item(selected, 0)->data(Qt::UserRole).toString();
  const auto billsData = service_->bills(profile_.id);
  auto it =
      std::find_if(billsData.begin(), billsData.end(),
                   [&](const core::Bill &bill) { return bill.id == billId; });
  if (it == billsData.end()) {
    return;
  }

  BillEditorDialog dialog(this);
  dialog.setCategories(service_->categories(profile_.id), it->categoryId);
  dialog.setBill(*it);
  if (dialog.exec() == QDialog::Accepted) {
    QString message;
    if (!service_->upsertBill(profile_.id, dialog.bill(), message)) {
      QMessageBox::warning(this, "失败", message);
      return;
    }
    refreshBills();
    refreshDashboard();
  }
}

// 删除选中账单，二次确认后执行。
void MainWindow::handleDeleteBill() {
  const auto selected = billTable_->currentRow();
  if (selected < 0) {
    return;
  }
  if (QMessageBox::question(this, "确认", "确认删除选中账单？") !=
      QMessageBox::Yes) {
    return;
  }
  const auto billId =
      billTable_->item(selected, 0)->data(Qt::UserRole).toString();
  QString message;
  if (!service_->removeBill(profile_.id, billId, message)) {
    QMessageBox::warning(this, "失败", message);
    return;
  }
  refreshBills();
  refreshDashboard();
}

// 新增分类时校验名称非空。
void MainWindow::handleAddCategory() {
  core::Category category;
  category.name = categoryNameEdit_->text().trimmed();
  category.type = categoryTypeCombo_->currentData().toString();
  if (category.name.isEmpty()) {
    QMessageBox::warning(this, "提示", "分类名称不能为空");
    return;
  }
  QString message;
  if (!service_->upsertCategory(profile_.id, category, message)) {
    QMessageBox::warning(this, "失败", message);
    return;
  }
  categoryNameEdit_->clear();
  refreshCategories();
}

// 删除分类前提示存在账单关联的限制。
void MainWindow::handleDeleteCategory() {
  const auto *item = categoryList_->currentItem();
  if (!item) {
    return;
  }
  if (QMessageBox::question(this, "确认", "确认删除选中分类？") !=
      QMessageBox::Yes) {
    return;
  }
  QString message;
  if (!service_->removeCategory(profile_.id,
                                item->data(Qt::UserRole).toString(), message)) {
    QMessageBox::warning(this, "失败", message);
    return;
  }
  refreshCategories();
}

// 新建提醒并刷新列表。
void MainWindow::handleAddReminder() {
  ReminderDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    QString message;
    if (!service_->upsertReminder(profile_.id, dialog.reminder(), message)) {
      QMessageBox::warning(this, "失败", message);
      return;
    }
    refreshReminders();
  }
}

// 编辑提醒时定位原记录。
void MainWindow::handleEditReminder() {
  const auto *item = reminderList_->currentItem();
  if (!item) {
    return;
  }
  const auto reminderId = item->data(Qt::UserRole).toString();
  const auto remindersData = service_->reminders(profile_.id);
  auto it = std::find_if(remindersData.begin(), remindersData.end(),
                         [&](const core::Reminder &reminder) {
                           return reminder.id == reminderId;
                         });
  if (it == remindersData.end()) {
    return;
  }

  ReminderDialog dialog(this);
  dialog.setReminder(*it);
  if (dialog.exec() == QDialog::Accepted) {
    QString message;
    if (!service_->upsertReminder(profile_.id, dialog.reminder(), message)) {
      QMessageBox::warning(this, "失败", message);
      return;
    }
    refreshReminders();
  }
}

// 删除提醒需确认。
void MainWindow::handleDeleteReminder() {
  const auto *item = reminderList_->currentItem();
  if (!item) {
    return;
  }
  if (QMessageBox::question(this, "确认", "确认删除选中提醒？") !=
      QMessageBox::Yes) {
    return;
  }
  QString message;
  if (!service_->removeReminder(profile_.id,
                                item->data(Qt::UserRole).toString(), message)) {
    QMessageBox::warning(this, "失败", message);
    return;
  }
  refreshReminders();
}

// 发布动态前检查内容非空。
void MainWindow::handlePublishPost() {
  const auto content = postEdit_->toPlainText().trimmed();
  if (content.isEmpty()) {
    QMessageBox::warning(this, "提示", "内容不能为空");
    return;
  }
  QString message;
  if (!service_->publishPost(profile_.id, content,
                             visibilityCombo_->currentData().toString(),
                             message)) {
    QMessageBox::warning(this, "失败", message);
    return;
  }
  postEdit_->clear();
  refreshTimeline();
}

// 手动刷新时间线。
void MainWindow::handleRefreshTimeline() { refreshTimeline(); }

// 通过用户名或邮箱添加好友。
void MainWindow::handleAddFriend() {
  const auto handle = friendEdit_->text().trimmed();
  if (handle.isEmpty()) {
    return;
  }
  QString message;
  if (!service_->addFriend(profile_.id, handle, message)) {
    QMessageBox::warning(this, "失败", message);
    return;
  }
  friendEdit_->clear();
  QMessageBox::information(this, "提示", "好友添加成功");
  refreshTimeline();
}

// 对选中动态添加评论。
void MainWindow::handleComment() {
  const auto *item = timelineList_->currentItem();
  if (!item) {
    return;
  }
  const auto payload = item->data(Qt::UserRole).toMap();
  const auto ownerId = payload.value("ownerId").toString();
  const auto postId = payload.value("postId").toString();
  const auto text =
      QInputDialog::getMultiLineText(this, "发表评论", "评论内容");
  if (text.trimmed().isEmpty()) {
    return;
  }
  QString message;
  if (!service_->addComment(profile_.id, ownerId, postId, text, message)) {
    QMessageBox::warning(this, "失败", message);
    return;
  }
  refreshTimeline();
}

// 更新仪表盘的汇总文本与图表。
void MainWindow::refreshDashboard() {
  const auto income = service_->totalIncome(profile_.id);
  const auto expense = service_->totalExpense(profile_.id);
  totalIncomeLabel_->setText(QString("总收入：￥%1").arg(income, 0, 'f', 2));
  totalExpenseLabel_->setText(QString("总支出：￥%1").arg(expense, 0, 'f', 2));

  const auto summaries = service_->summarizeByCategory(profile_.id);

  auto *pieSeries = new QtCharts::QPieSeries();
  for (const auto &summary : summaries) {
    if (summary.expense > 0.0) {
      pieSeries->append(summary.name, summary.expense);
    }
  }
  if (pieSeries->isEmpty()) {
    pieSeries->append("暂无线下支出", 1.0);
  }
  auto *pieChart = new QtCharts::QChart();
  pieChart->addSeries(pieSeries);
  pieChart->setTitle("分类支出占比");
  pieChart->legend()->setAlignment(Qt::AlignRight);
  pieChartView_->setChart(pieChart);

  const auto billsData = service_->bills(profile_.id);
  const QDate today = QDate::currentDate();
  const QDate startDate = today.addDays(-6);
  QVector<double> dailyExpense(7, 0.0);
  for (const auto &bill : billsData) {
    if (bill.type != core::BillType::Expense || !bill.timestamp.isValid()) {
      continue;
    }
    const QDate billDate = bill.timestamp.date();
    if (billDate < startDate || billDate > today) {
      continue;
    }
    const int index = startDate.daysTo(billDate);
    if (index >= 0 && index < dailyExpense.size()) {
      dailyExpense[index] += bill.amount;
    }
  }

  QStringList dayLabels;
  auto *dailySet = new QtCharts::QBarSet("支出");
  for (int i = 0; i < dailyExpense.size(); ++i) {
    const QDate day = startDate.addDays(i);
    dayLabels << day.toString("MM-dd");
    dailySet->append(dailyExpense[i]);
  }

  auto *barSeries = new QtCharts::QBarSeries();
  barSeries->append(dailySet);

  auto *barChart = new QtCharts::QChart();
  barChart->addSeries(barSeries);
  barChart->setTitle("近7天支出");
  auto *axisX = new QtCharts::QBarCategoryAxis();
  axisX->append(dayLabels);
  barChart->addAxis(axisX, Qt::AlignBottom);
  barSeries->attachAxis(axisX);
  auto *axisY = new QtCharts::QValueAxis();
  axisY->setLabelFormat("%.2f");
  double maxValue = 0.0;
  for (const auto value : dailyExpense) {
    if (value > maxValue) {
      maxValue = value;
    }
  }
  axisY->setRange(0.0, maxValue > 0.0 ? maxValue * 1.2 : 1.0);
  barChart->addAxis(axisY, Qt::AlignLeft);
  barSeries->attachAxis(axisY);
  barChartView_->setChart(barChart);
}

// 刷新账单表格并按时间降序排列。
void MainWindow::refreshBills() {
  const auto categoriesData = service_->categories(profile_.id);
  QHash<QString, QString> categoryNames;
  for (const auto &category : categoriesData) {
    categoryNames.insert(category.id, category.name);
  }

  auto billsData = service_->bills(profile_.id);
  std::sort(billsData.begin(), billsData.end(),
            [](const core::Bill &a, const core::Bill &b) {
              return a.timestamp > b.timestamp;
            });

  billTable_->setRowCount(billsData.size());
  for (int row = 0; row < billsData.size(); ++row) {
    const auto &bill = billsData[row];
    auto *timeItem =
        new QTableWidgetItem(bill.timestamp.toString("yyyy-MM-dd HH:mm"));
    timeItem->setData(Qt::UserRole, bill.id);
    billTable_->setItem(row, 0, timeItem);

    billTable_->setItem(
        row, 1,
        new QTableWidgetItem(categoryNames.value(bill.categoryId, "未知")));
    billTable_->setItem(
        row, 2,
        new QTableWidgetItem(bill.type == core::BillType::Expense ? "支出"
                                                                  : "收入"));
    billTable_->setItem(
        row, 3, new QTableWidgetItem(QString::number(bill.amount, 'f', 2)));
    billTable_->setItem(row, 4, new QTableWidgetItem(bill.note));
  }
  billTable_->resizeColumnsToContents();
}

// 更新分类列表展示。
void MainWindow::refreshCategories() {
  categoryList_->clear();
  const auto categoriesData = service_->categories(profile_.id);
  for (const auto &category : categoriesData) {
    auto *item = new QListWidgetItem(QString("%1 (%2)").arg(
        category.name, category.type == "income" ? "收入" : "支出"));
    item->setData(Qt::UserRole, category.id);
    categoryList_->addItem(item);
  }
}

// 以时间排序刷新提醒列表。
void MainWindow::refreshReminders() {
  reminderList_->clear();
  auto remindersData = service_->reminders(profile_.id);
  std::sort(remindersData.begin(), remindersData.end(),
            [](const core::Reminder &a, const core::Reminder &b) {
              return a.remindAt < b.remindAt;
            });
  for (const auto &reminder : remindersData) {
    auto text = QString("[%1] %2 %3")
                    .arg(reminder.remindAt.toString("yyyy-MM-dd HH:mm"))
                    .arg(reminder.enabled ? "启用" : "停用")
                    .arg(reminder.message);
    auto *item = new QListWidgetItem(text);
    item->setData(Qt::UserRole, reminder.id);
    reminderList_->addItem(item);
  }
}

// 重新生成时间线文本内容。
void MainWindow::refreshTimeline() {
  timelineList_->clear();
  const auto posts = service_->timeline(profile_.id);
  for (const auto &post : posts) {
    const auto profileOpt = service_->profile(post.authorId);
    const auto authorName = profileOpt ? profileOpt->username : post.authorId;
    QString text =
        QString("%1 (%2) [%3]\n%4\n评论:")
            .arg(authorName, post.visibility == "public" ? "公开" : "好友")
            .arg(post.createdAt.toLocalTime().toString("yyyy-MM-dd HH:mm"))
            .arg(post.content);
    for (const auto &comment : post.comments) {
      const auto commenter = service_->profile(comment.authorId);
      text.append(QString("\n - %1: %2")
                      .arg(commenter ? commenter->username : comment.authorId,
                           comment.content));
    }
    auto *item = new QListWidgetItem(text);
    QVariantMap payload;
    payload.insert("postId", post.id);
    payload.insert("ownerId", post.authorId);
    item->setData(Qt::UserRole, payload);
    timelineList_->addItem(item);
  }
}

// 轮询提醒并通过弹窗提示即将到期项目。
void MainWindow::refreshReminderNotifications() {
  const auto nowUtc = QDateTime::currentDateTimeUtc();
  const auto upcoming = service_->upcomingReminders(
      profile_.id, lastReminderCheck_, nowUtc.addSecs(60));
  lastReminderCheck_ = nowUtc;

  for (const auto &reminder : upcoming) {
    const auto localTime =
        reminder.remindAt.toLocalTime().toString("yyyy-MM-dd HH:mm");
    QMessageBox::information(
        this, "提醒", QString("%1\n时间：%2").arg(reminder.message, localTime));
  }
}

}  // namespace ui
