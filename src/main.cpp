#include <QTranslator>
#include <QLocale>
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
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
    setenv("QT_QPA_PLATFORM", "dxcb;xcb", 1);
  }

  DApplication a(argc, argv);

  // Antes se pasaba una lista fija (espanol primero) y se cargaba siempre el
  // espanol, fuese cual fuese el idioma del sistema. Las traducciones van
  // embebidas en :/i18n (qt_add_translations) y se elige la del sistema.
  a.loadTranslator();
  QTranslator translator;
  if (translator.load(QLocale(), QStringLiteral("dvtoyinst"), QStringLiteral("_"),
                      QStringLiteral(":/i18n")))
    a.installTranslator(&translator);
  a.setOrganizationName("0x4C57 Technology Group");
  a.setApplicationVersion(DApplication::buildVersion("1.2.0"));
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
