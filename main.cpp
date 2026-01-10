#include "mainwindow.h"
#include "utilities.h"
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DWidgetUtil> //Dtk::Widget::moveToCenter(&w); 要调用它，就得引用DWidgetUtil

DWIDGET_USE_NAMESPACE
int main(int argc, char *argv[]) {
  // DApplication::loadDXcbPlugin();  //让bar处在标题栏中
  DApplication a(argc, argv);

  a.setAttribute(Qt::AA_UseHighDpiPixmaps);
  a.loadTranslator(QList<QLocale>() << QLocale::Spanish << QLocale::Chinese << QLocale::Portuguese << QLocale::Italian << QLocale::French);
  a.setOrganizationName("0x4C57 Technology Group");
  a.setApplicationVersion(DApplication::buildVersion("1.0.1"));
  // a.setApplicationAcknowledgementPage("https://你的网站");
  a.setProductIcon(QIcon(":/images/res/dvtoyinst.png")); // 设置Logo
  a.setProductName(QObject::tr("Deepin Ventoy Installer"));
  a.setApplicationDescription(
      QObject::tr("A Ventoy installer front-end written with DTK."));
  a.setApplicationName(
      "Deepin Ventoy Installer"); // 只有在这儿修改窗口标题才有效

  rerunOnNonRoot();

  MainWindow w;
  w.show();

  // 让打开时界面显示在正中
  Dtk::Widget::moveToCenter(&w);

  return a.exec();
}
