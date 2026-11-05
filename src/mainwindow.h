#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DMainWindow>
#include <DMessageBox>
#include "widget.h"

DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

private:
  Widget *w;
  void closeEvent(QCloseEvent *e) Q_DECL_OVERRIDE;
  void initWindow();
};

#endif // MAINWINDOW_H
