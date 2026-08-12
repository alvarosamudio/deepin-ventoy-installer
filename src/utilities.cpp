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

void rerunOnNonRoot()
{
  if(qgetenv("USER") != "root")
  {

    Dtk::Widget::DDialog input;
    Dtk::Widget::DLineEdit *edit = new Dtk::Widget::DLineEdit(&input);
    edit->setEchoMode(QLineEdit::Password);
    input.setTitle(QObject::tr("Run As Root"));
    input.setMessage(QObject::tr("This application must be run as root user.\n"
                                   "Provide password to continue.\n")); // An empty line to separate contents
    input.addContent(edit);
    input.addButton(QObject::tr("Authenticate"), true, DDialog::ButtonType::ButtonRecommend);
    input.setIcon(QIcon::fromTheme("dialog-information"));
    input.setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    edit->lineEdit()->setFocus(Qt::FocusReason::TabFocusReason); // FIXME: The input will not get focused despite my efforts.
    QObject::connect(edit, SIGNAL(returnPressed()), &input, SLOT(accept()));
    //QObject::connect(&input, SIGNAL())

    // The only way to explicitly close the window will return -1, not rejected (0) nor accepted (1)
    if(input.exec() != -1)
    {
      QString password = edit->text();
      QProcess rerun;
      // sudo resets the environment by default; preserve the variables the
      // GUI session depends on (platform plugin, display, D-Bus...) so the
      // relaunched instance keeps the Deepin/DTK look and feel.
      QString envArgs;
      QStringList envPreserve = {"QT_QPA_PLATFORM", "QT_QPA_PLATFORMTHEME",
                                 "DISPLAY", "WAYLAND_DISPLAY",
                                 "XDG_SESSION_TYPE", "XDG_CURRENT_DESKTOP",
                                 "XAUTHORITY", "DBUS_SESSION_BUS_ADDRESS"};
      foreach(const QString &var, envPreserve)
      {
        QByteArray value = qgetenv(var.toLatin1());
        if(!value.isEmpty())
        {
          // Single-quote the value so shell metacharacters (e.g. ';' inside
          // "dxcb;xcb") survive the sudo command line.
          QString quoted = QString::fromLocal8Bit(value);
          quoted.replace('\'', "'\\''");
          envArgs += QString(" %1='%2'").arg(var, quoted);
        }
      }
      rerun.startDetached("bash", QStringList() << "-c"
                  << QString("echo %1\"\n\n\n\n\n\n\n\n\" | sudo -S%2 %3") // If fails then ENTER ENTER ENTER til sudo ends
                  .arg(password)
                  .arg(envArgs)
                  .arg(QCoreApplication::applicationFilePath()));
    }
    exit(1);
  }
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
