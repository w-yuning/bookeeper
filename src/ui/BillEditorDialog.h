// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#pragma once

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>

#include "core/Entities.h"

namespace ui {

// BillEditorDialog 提供账单新增/编辑界面。
class BillEditorDialog : public QDialog {
  Q_OBJECT

 public:
  explicit BillEditorDialog(QWidget *parent = nullptr);

  void setCategories(const QVector<core::Category> &categories,
                     const QString &selectedId = QString());
  void setBill(const core::Bill &bill);
  core::Bill bill() const;

 private:
  void buildUi();

  QDoubleSpinBox *amountSpin_ = nullptr;
  QComboBox *typeCombo_ = nullptr;
  QComboBox *categoryCombo_ = nullptr;
  QDateTimeEdit *timeEdit_ = nullptr;
  QPlainTextEdit *noteEdit_ = nullptr;

  core::Bill bill_;
};

}  // namespace ui
