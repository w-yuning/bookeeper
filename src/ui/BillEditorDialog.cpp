// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#include "BillEditorDialog.h"

#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace ui {

// 对话框构造时即搭建 UI。
BillEditorDialog::BillEditorDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle("账单编辑");
  buildUi();
}

// 构建账单编辑表单与按钮区。
void BillEditorDialog::buildUi() {
  auto *layout = new QVBoxLayout(this);
  auto *form = new QFormLayout();

  amountSpin_ = new QDoubleSpinBox(this);
  amountSpin_->setRange(0.0, 100000000.0);
  amountSpin_->setPrefix("￥");
  amountSpin_->setDecimals(2);
  form->addRow("金额", amountSpin_);

  typeCombo_ = new QComboBox(this);
  typeCombo_->addItem(
      "支出", QVariant::fromValue(static_cast<int>(core::BillType::Expense)));
  typeCombo_->addItem(
      "收入", QVariant::fromValue(static_cast<int>(core::BillType::Income)));
  form->addRow("类型", typeCombo_);

  categoryCombo_ = new QComboBox(this);
  form->addRow("分类", categoryCombo_);

  timeEdit_ = new QDateTimeEdit(QDateTime::currentDateTime(), this);
  timeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
  form->addRow("时间", timeEdit_);

  noteEdit_ = new QPlainTextEdit(this);
  form->addRow("备注", noteEdit_);

  layout->addLayout(form);

  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this,
          &BillEditorDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this,
          &BillEditorDialog::reject);
  layout->addWidget(buttons);
}

// 刷新分类下拉框，必要时定位默认选项。
void BillEditorDialog::setCategories(const QVector<core::Category> &categories,
                                     const QString &selectedId) {
  categoryCombo_->clear();
  int selectedIndex = -1;
  for (int i = 0; i < categories.size(); ++i) {
    categoryCombo_->addItem(categories[i].name, categories[i].id);
    if (!selectedId.isEmpty() && categories[i].id == selectedId) {
      selectedIndex = i;
    }
  }
  if (selectedIndex >= 0) {
    categoryCombo_->setCurrentIndex(selectedIndex);
  }
}

// 将已有账单内容加载到界面上，便于修改。
void BillEditorDialog::setBill(const core::Bill &bill) {
  bill_ = bill;
  amountSpin_->setValue(bill.amount);
  typeCombo_->setCurrentIndex(bill.type == core::BillType::Expense ? 0 : 1);
  categoryCombo_->setCurrentIndex(categoryCombo_->findData(bill.categoryId));
  timeEdit_->setDateTime(
      bill.timestamp.isValid() ? bill.timestamp : QDateTime::currentDateTime());
  noteEdit_->setPlainText(bill.note);
}

// 汇总用户输入，返回账单对象。
core::Bill BillEditorDialog::bill() const {
  core::Bill result = bill_;
  result.amount = amountSpin_->value();
  result.type = static_cast<core::BillType>(typeCombo_->currentData().toInt());
  result.categoryId = categoryCombo_->currentData().toString();
  result.timestamp = timeEdit_->dateTime();
  result.note = noteEdit_->toPlainText();
  return result;
}

}  // namespace ui
