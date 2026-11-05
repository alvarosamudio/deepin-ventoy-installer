#include "mainwindow.h"

#include <QCloseEvent>
#include <QAction>
#include <QMenu>
#include <DMainWindow>
#include <DDialog>
#include <DTitlebar>

DWIDGET_USE_NAMESPACE

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
{
    w = new Widget;

    setCentralWidget(w);
    // Tamano fijo: la ventana no se puede maximizar ni redimensionar
    setFixedSize(w->size());
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    initWindow();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
  if(w->hasOngoingOperation())
  {
    DDialog askExit;
    askExit.setMessage(tr("There is still ongoing tasks running!\n"
                          "Terminating the program right now could cause problems.\n\n"
                          "Continue?"));
    askExit.setTitle(tr("Exit"));
    askExit.setIcon(QIcon::fromTheme("dialog-warning"));
    askExit.addButton(tr("Yes"), true, DDialog::ButtonRecommend);
    askExit.addButton(tr("No"), false, DDialog::ButtonNormal);
    if(askExit.exec() == 0)
    {
      w->forceStopAll();
      e->accept();
    }
    else
    {
      e->ignore();
    }
  }
  else
  {
    e->accept();
  }
}

void MainWindow::initWindow()
{
  // El titulo de la ventana se mantiene para la barra de tareas, pero la
  // barra superior no muestra ni el nombre ni el icono.
  this->setWindowTitle(tr("Deepin Ventoy Installer"));
  this->titlebar()->setIcon(QIcon());
  this->titlebar()->setTitle(QString());
}
