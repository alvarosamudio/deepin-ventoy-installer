#include <unistd.h>
#include <QProcess>
#include <QDebug>
#include <QApplication>
#include <DDialog>
#include <DLineEdit>
#include "utilities.h"

DWIDGET_USE_NAMESPACE

QString findDVIWorkFile(const QDir &dviTmpDir, bool aNoTest)
{
  auto tmpDirContent = dviTmpDir.entryList();
  if(tmpDirContent.size() == 0)
  {
    return "";
  }
  tmpDirContent.removeFirst(); // "."
  tmpDirContent.removeFirst(); // and ".."
  QString concatePath;
  QFileInfo verifyIsDir;
  QDir ventoyDir;
  foreach(auto &&i, tmpDirContent)
  {
    concatePath = dviTmpDir.path() + "/" + i;
    verifyIsDir.setFile(concatePath);
    if(verifyIsDir.isDir())
    {
      ventoyDir.setPath(verifyIsDir.absoluteFilePath());
    }
  }

  QFileInfo mainScriptInfo(ventoyDir, "tool/ventoy_lib.sh"),
            helpScriptInfo(ventoyDir, "Ventoy2Disk.sh");
  if(mainScriptInfo.exists() && helpScriptInfo.exists())
  {
    if(!helpScriptInfo.isExecutable())
    {
      QFile::setPermissions(helpScriptInfo.filePath(),
                            QFile::ExeUser | QFile::ReadUser);
    }
    QProcess runTest;
    runTest.setWorkingDirectory(ventoyDir.absolutePath());
    // Ventoy2Disk.sh decompresses the bundled tool binaries (tool/<arch>/*.xz)
    // before doing anything else, even in its usage path.
    runTest.start("sh", {"Ventoy2Disk.sh"});
    runTest.waitForFinished();
    runTest.start("sh");
    runTest.waitForStarted();
    runTest.write("export PATH=$PWD/tool/x86_64:$PATH \n");
    runTest.write(". tool/ventoy_lib.sh \n");
    runTest.write("check_tool_work_ok \n");
    runTest.write("exit\n");
    runTest.waitForFinished(3000);
    if(!runTest.exitCode() || aNoTest)
    {
      return ventoyDir.absolutePath();
    }
    qInfo() << runTest.exitCode() << runTest.readAllStandardError();
  }

  return "";
}

SemanticVersion::SemanticVersion(QString aVer)
{
  auto l = aVer.split('.');
  if(l.size() < 3)
  {
    major = minor = patch = 0;
    return;
  }
  major = l[0].toUInt();
  minor = l[1].toUInt();
  patch = l[2].toUInt();
}

SemanticVersion::SemanticVersion(uint aMajor, uint aMinor, uint aPatch)
{
  major = aMajor;
  minor = aMinor;
  patch = aPatch;
}

SemanticVersion::SemanticVersion()
{
  major = minor = patch = 0;
}

// Se relanza como root con pkexec (polkit), que muestra el dialogo de
// autenticacion del sistema. Antes la aplicacion pedia la contrasena en un
// dialogo propio y la insertaba en una orden de bash ("echo <contrasena> |
// sudo -S ..."): quedaba visible para cualquier usuario en la lista de
// procesos y una contrasena con ; $( ) o comillas ejecutaba ordenes.
void rerunOnNonRoot()
{
  if(geteuid() == 0)
    return;

  // pkexec limpia el entorno; se pasan con env (sin shell, como argumentos
  // independientes) las variables que necesita la sesion grafica.
  QStringList args{QStringLiteral("env")};
  const QStringList envPreserve = {"QT_QPA_PLATFORM", "QT_QPA_PLATFORMTHEME",
                                   "DISPLAY", "WAYLAND_DISPLAY",
                                   "XDG_SESSION_TYPE", "XDG_CURRENT_DESKTOP",
                                   "XDG_RUNTIME_DIR", "XAUTHORITY",
                                   "DBUS_SESSION_BUS_ADDRESS", "LANG", "LANGUAGE"};
  for(const QString &var : envPreserve)
  {
    const QByteArray value = qgetenv(var.toLatin1());
    if(!value.isEmpty())
      args << QStringLiteral("%1=%2").arg(var, QString::fromLocal8Bit(value));
  }
  args << QCoreApplication::applicationFilePath();

  // No se usa startDetached: pkexec autoriza segun su proceso padre y, si
  // este ya ha terminado, el padre pasa a ser systemd. Al abrir la app desde
  // el menu de Deepin polkit podia entonces no reconocer la sesion del
  // usuario y negar el permiso. Esta instancia (sin ventana) espera a que
  // termine la de root.
  QProcess elevada;
  elevada.setProcessChannelMode(QProcess::ForwardedChannels);
  elevada.start(QStringLiteral("pkexec"), args);
  if(elevada.waitForStarted())
  {
    elevada.waitForFinished(-1);
    // 126: dialogo cancelado; 127: sin autorizacion. En ambos casos se sale
    // sin mas, igual que si el usuario hubiese cerrado la app.
    exit(0);
  }
  else
  {
    Dtk::Widget::DDialog error;
    error.setTitle(QObject::tr("Run As Root"));
    error.setMessage(QObject::tr("This application must be run as root user.\n"
                                 "Provide password to continue.\n"));
    error.setIcon(QIcon::fromTheme("dialog-error"));
    error.addButton(QObject::tr("OK"), true);
    error.exec();
  }
  exit(0);
}

void enableCheckBox(bool criteria, QCheckBox *w)
{
  //qInfo() << criteria;
  w->setEnabled(criteria);
  if(!criteria)
  {
    w->setChecked(criteria);
  }
}
