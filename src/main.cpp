#include "mainwindow.h"
#include "utilities.h"

#include <cstdlib>
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DWidgetUtil>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[]) {
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORMTHEME")) {
    setenv("QT_QPA_PLATFORMTHEME", "deepin", 1);
  }

  DApplication a(argc, argv);

  a.loadTranslator(QList<QLocale>() << QLocale::Spanish << QLocale::Chinese
                                    << QLocale::Portuguese << QLocale::Italian
                                    << QLocale::French);
  a.setOrganizationName("0x4C57 Technology Group");
  a.setApplicationVersion(DApplication::buildVersion("1.1.0"));
  a.setProductIcon(QIcon(":/images/res/dvtoyinst.png"));
  a.setProductName(QObject::tr("Deepin Ventoy Installer"));
  a.setApplicationDescription(
      QObject::tr("A Ventoy installer front-end written with DTK."));
  a.setApplicationName("deepin-ventoy-installer");

  Dtk::Gui::DGuiApplicationHelper::instance()->setAttribute(
      Dtk::Gui::DGuiApplicationHelper::UseInactiveColorGroup, false);

  rerunOnNonRoot();

  MainWindow w;
  w.show();

  Dtk::Widget::moveToCenter(&w);

  return a.exec();
}
