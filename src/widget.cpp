#include <QProcessEnvironment>
#include <QSysInfo>
#include <QRegularExpression>
#include <QLabel>
#include <QHBoxLayout>
#include <QHash>
#include <QSet>
#include <QStandardItemModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <DDialog>
#include <DGuiApplicationHelper>
#include <DSpinner>
#include <QNetworkRequest>
#include <QProcess>
#include <QScrollBar>
#include <QSettings>
#include <QTemporaryFile>
#include <QUrl>

#include "utilities.h"
#include "widget.h"

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

#include "ui_widget.h"

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
  ui->setupUi(this);
  initWidget();
}

Widget::~Widget() { delete ui; }

bool Widget::hasOngoingOperation() {
  return m_unarchiveProcess.state() == QProcess::ProcessState::Running ||
         m_installProcess.state() == QProcess::ProcessState::Running;
}

void Widget::processVentoyArchive(QString aPath, bool aNoTest) {
  m_unarchiveProcess.kill();
  m_unarchiveProcess.waitForFinished();
  if (!tmpDir.removeRecursively()) {
    qWarning() << "Failed to remove the temporary directory for extraction";
  }
  tmpDir.mkpath(tmpDir.path());
  m_noTestVentoy = aNoTest;
  m_unarchiveProcess.start(
      "tar", {"-xavf", QUrl(aPath).path(), "-C", tmpDir.path()});
}

void Widget::finishedUnarchive() {
  switch (m_unarchiveProcess.exitCode()) {
  case 0:
    break;
  case 1:
    qInfo() << "Extracted files differ; they have been updated.";
    break;
  default: {
    DDialog failExtractMsg;
    failExtractMsg.setTitle(tr("Extraction Error"));
    failExtractMsg.setMessage(tr("Extraction of the selected archive was not "
                              "successful; exit code: %1.")
                               .arg(m_unarchiveProcess.exitCode()));
    failExtractMsg.setIcon(QIcon::fromTheme("dialog-error"));
    failExtractMsg.addButton(tr("OK"), true, DDialog::ButtonNormal);
    failExtractMsg.exec();
    return;
  }
  }
  tmpDir.cd(tmpDir.path());

  // Successfully unarchived, let's roll!
  QDir::setCurrent(tmpDir.path());

  QString dviWorkDir = findDVIWorkFile(tmpDir, m_noTestVentoy);
  if (dviWorkDir == "") {
    DDialog failFindScriptMsg;
    failFindScriptMsg.setTitle(tr("Error"));
    failFindScriptMsg.setMessage(
        tr("No working Ventoy build can be found in the extracted archive."));
    failFindScriptMsg.setIcon(QIcon::fromTheme("dialog-error"));
    failFindScriptMsg.addButton(tr("OK"), true, DDialog::ButtonNormal);
    failFindScriptMsg.exec();
    return;
  }

  QDir::setCurrent(dviWorkDir);

  switchToInstaller();
}

// Modos del selector (ver initWidget)
enum ModoVentoy { ModoAuto = 0, ModoInstalar, ModoNoDestructivo, ModoActualizar, ModoForzar };

void Widget::proceedInstallation() {
  const QString devicePath = ui->selectDevice_combo->currentData().toString();
  if (devicePath.isEmpty())
    return;

  // Se vuelve a comprobar al instalar: nunca el disco del sistema (Ventoy
  // intentaria desmontar /, /home, /boot...).
  const DeviceInfo info = deviceInfo(devicePath);
  if (info.isSystem) {
    DDialog refuse;
    refuse.setTitle(tr("Error"));
    refuse.setMessage(tr("%1 contains the running system and cannot be used.").arg(devicePath));
    refuse.setIcon(QIcon::fromTheme("dialog-error"));
    refuse.addButton(tr("OK"), true, DDialog::ButtonNormal);
    refuse.exec();
    return;
  }

  // Sin Ventoy en el dispositivo su version es 0.0.0 ("invalida"). Antes se
  // consideraba actualizacion cualquier version menor, asi que un USB sin
  // Ventoy se "actualizaba" (-u): fallaba ("/dev/sdX2 not exist") y ademas se
  // saltaba la segunda confirmacion de borrado.
  const bool tieneVentoy = !m_DeviceVer.invalid();
  int modo = m_comboModo ? m_comboModo->currentIndex() : ModoAuto;
  if (modo == ModoAuto)
    modo = !tieneVentoy ? ModoInstalar
         : (m_DeviceVer < m_archiveVer ? ModoActualizar : ModoForzar);

  auto aviso = [](const QString &titulo, const QString &texto) {
    DDialog d;
    d.setTitle(titulo);
    d.setMessage(texto);
    d.setIcon(QIcon::fromTheme("dialog-warning"));
    d.addButton(tr("OK"), true, DDialog::ButtonNormal);
    d.exec();
  };
  if (modo == ModoActualizar && !tieneVentoy) {
    aviso(tr("Confirm Installation"),
          tr("%1 does not have Ventoy installed, so it cannot be updated.\n"
             "Choose \"Install\" instead.").arg(devicePath));
    return;
  }
  if (modo == ModoNoDestructivo && tieneVentoy) {
    aviso(tr("Confirm Installation"),
          tr("%1 already has Ventoy. Use \"Update\" to keep your files.").arg(devicePath));
    return;
  }

  // Etiqueta: exFAT admite como maximo 15 caracteres y no permite algunos
  // simbolos. Ventoy no lo comprueba y fallaria despues de reparticionar el
  // disco, dejandolo a medio formatear.
  if (modo != ModoActualizar && ui->chkUseCustomVolLabel->isChecked()) {
    const QString etiqueta = ui->lineCustomVolLabel->text();
    static const QRegularExpression prohibidos(QStringLiteral("[\"*/:<>?\\\\|\\x00-\\x1f]"));
    if (etiqueta.trimmed().isEmpty() || etiqueta.size() > 15 || etiqueta.contains(prohibidos)) {
      aviso(tr("Custom volume label"),
            tr("The volume label must have between 1 and 15 characters and cannot "
               "contain any of these: \" * / : < > ? \\ |"));
      return;
    }
  }
  // Espacio reservado: debe quedar sitio para Ventoy y para tus ISO
  if (modo != ModoActualizar && m_chkReservar->isChecked()) {
    const qint64 discoMiB = info.sizeBytes / (1024 * 1024);
    if (m_spinReservar->value() >= discoMiB - 1024) {
      aviso(tr("Reserve space at the end of the disk"),
            tr("Cannot reserve %1 MiB on %2: the disk has %3 MiB and Ventoy needs "
               "at least 1 GiB.").arg(m_spinReservar->value()).arg(devicePath).arg(discoMiB));
      return;
    }
  }

  // Disco interno (no extraible): aviso explicito
  if (!info.isRemovable) {
    DDialog internal;
    internal.setTitle(tr("Internal disk"));
    internal.setMessage(tr("%1 is an internal disk, not a USB drive.\n"
                           "Installing Ventoy will erase ALL its data.\n\n"
                           "Are you sure this is the disk you want?").arg(devicePath));
    internal.setIcon(QIcon::fromTheme("dialog-warning"));
    internal.addButton(tr("Cancel"), true);
    internal.addButton(tr("Use internal disk"), false, DDialog::ButtonWarning);
    if (internal.exec() != 1)
      return;
  }

  // Confirmaciones: dos si se borran los datos, una si se conservan
  auto confirmar = [](const QString &titulo, const QString &texto) {
    DDialog d;
    d.setTitle(titulo);
    d.setMessage(texto);
    d.setIcon(QIcon::fromTheme("dialog-warning"));
    d.addButton(tr("Confirm"), false, DDialog::ButtonWarning);
    d.addButton(tr("Cancel"), true);
    return d.exec() == 0;
  };
  if (modo == ModoActualizar) {
    if (!confirmar(tr("Confirm Installation"),
                   tr("You're about to update your Ventoy inside %1. Confirm?\n"
                      "Updating from %2 to %3.")
                       .arg(devicePath).arg(m_DeviceVer).arg(m_archiveVer)))
      return;
  } else if (modo == ModoNoDestructivo) {
    if (!confirmar(tr("Confirm Installation"),
                   tr("Ventoy will try a non-destructive installation on %1, keeping "
                      "your files.\nMake a backup first: if the disk layout does not "
                      "allow it, the installation will be cancelled.").arg(devicePath)))
      return;
  } else {
    if (!confirmar(tr("Confirm Installation"),
                   tr("You're about to install Ventoy into %1. Confirm?\n"
                      "ALL your data on the target will be erased!").arg(devicePath)) ||
        !confirmar(tr("Confirm Installation Again"),
                   tr("Double-check! The target device is %1.\n"
                      "Installing Ventoy will destroy ALL DATA present on the device!")
                       .arg(devicePath)))
      return;
  }

  // Parametros de Ventoy2Disk.sh
  QStringList params;
  switch (modo) {
  case ModoActualizar:    params << "-u"; break;
  case ModoForzar:        params << "-I"; break;
  case ModoNoDestructivo: params << "-i" << "-n"; break;
  default:                params << (tieneVentoy ? "-I" : "-i"); break;
  }
  if (modo != ModoActualizar) {
    if (ui->chkUseGptPartTable->isEnabled() && ui->chkUseGptPartTable->isChecked())
      params << "-g";
    if (m_chkReservar && m_chkReservar->isChecked() && m_spinReservar->value() > 0)
      params << "-r" << QString::number(m_spinReservar->value());
  }
  // Al instalar: -s / -S segun la casilla (Ventoy lo activa por defecto, asi
  // que sin -S no se desactivaba). Al actualizar no se pasa nada y Ventoy
  // conserva la configuracion que ya tenia el USB.
  if (modo != ModoActualizar && ui->chkEnableSecureBoot->isEnabled())
    params << (ui->chkEnableSecureBoot->isChecked() ? "-s" : "-S");
  if (modo != ModoActualizar && ui->chkUseCustomVolLabel->isChecked())
    params << "-L" << ui->lineCustomVolLabel->text();

  installVentoy(devicePath, params);
}

void Widget::actualizarOpcionesSegunModo() {
  const int modo = m_comboModo->currentIndex();
  // GPT y reservar espacio solo tienen efecto al instalar
  const bool instala = (modo != ModoActualizar);
  ui->chkUseGptPartTable->setEnabled(instala && m_archiveVer >= SemanticVersion(1, 0, 15));
  ui->chkEnableSecureBoot->setEnabled(instala && m_archiveVer >= SemanticVersion(1, 0, 7));
  ui->chkUseCustomVolLabel->setEnabled(instala && m_archiveVer >= SemanticVersion(1, 0, 28));
  m_chkReservar->setEnabled(instala);
  m_spinReservar->setEnabled(instala && m_chkReservar->isChecked());
}

void Widget::finishedInstallation() {
  ui->spinnerInstall->stop();
  ui->spinnerInstall->setVisible(false);
  ui->btnBack->setVisible(true);
  if (m_installProcess.exitStatus() != QProcess::NormalExit ||
      m_installProcess.error() == QProcess::FailedToStart ||
      m_installProcess.exitCode()) {
    ui->lblSuccessFailImage->setPixmap(
        QIcon::fromTheme("dialog-error").pixmap({48, 48}));
    ui->lblInstallPrompt->setText(tr("Installation failed!"));
  } else {
    ui->lblSuccessFailImage->setPixmap(
        QIcon::fromTheme("dialog-ok").pixmap({48, 48}));
    ui->lblInstallPrompt->setText(tr("Installation succeeded!"));
  }
  ui->lblSuccessFailImage->setVisible(true);
}

void Widget::readInstallLog() {
  ui->textInstallLogs->textCursor().movePosition(
      QTextCursor::MoveOperation::End);
  ui->textInstallLogs->textCursor().insertText(m_installProcess.readAll());
  auto scroll = ui->textInstallLogs->verticalScrollBar();
  scroll->setValue(scroll->maximum());
}

void Widget::goBackDropArchive() {
  const QString &dviWorkDir = findDVIWorkFile(tmpDir, true);
  if (dviWorkDir == "") {
    ui->btnUsePreviousExtraction->setVisible(false);
  } else {
    QDir::setCurrent(dviWorkDir);
  }
  ui->mainPager->setCurrentIndex(0);
}

void Widget::changeTheme(bool aDark) {
  const QString sufijo = aDark ? QStringLiteral("-dark") : QString();
  const QIcon refresh(QStringLiteral(":/images/res/refresh%1.svg").arg(sufijo));
  ui->btnRefreshDeviceList->setIcon(refresh);
  ui->btnQueryDeviceVentoyVer->setIcon(refresh);
  // Via QIcon el SVG se dibuja a la resolucion de la pantalla (nitido en HiDPI)
  ui->lblPictureDropArchive->setPixmap(
      QIcon(QStringLiteral(":/images/res/drop%1.svg").arg(sufijo)).pixmap(QSize(104, 104)));
}

// Puntos de montaje que indican que el disco contiene el sistema en uso
static bool esMontajeDelSistema(const QString &m)
{
  static const QStringList sistema = {"/", "/boot", "/boot/efi", "/home", "/var",
                                      "/usr", "/root", "/opt", "/srv", "/sysroot",
                                      "/persistent", "/ostree", "[SWAP]"};
  return sistema.contains(m);
}

// Todas las entradas de lsblk (discos, particiones, LVM, cifrados...) en una
// lista plana: segun la version, lsblk anida las particiones en "children" o
// las devuelve al mismo nivel que el disco.
static void aplanar(const QJsonArray &arr, QList<QJsonObject> &out)
{
  for (const auto &v : arr) {
    const QJsonObject o = v.toObject();
    out.append(o);
    aplanar(o["children"].toArray(), out);
  }
}

static QList<QJsonObject> consultarDiscos()
{
  QProcess proc;
  proc.start("lsblk", {"-J", "-b", "-o", "NAME,PKNAME,PATH,SIZE,TYPE,RM,TRAN,MODEL,MOUNTPOINTS"});
  proc.waitForFinished();
  QList<QJsonObject> todos;
  if (proc.exitCode() == 0)
    aplanar(QJsonDocument::fromJson(proc.readAllStandardOutput()).object()["blockdevices"].toArray(), todos);
  return todos;
}

// Discos que contienen el sistema en uso: desde cada entrada con un montaje
// del sistema se sube por PKNAME hasta el disco (particiones, LVM, cifrados).
static QSet<QString> discosDelSistema(const QList<QJsonObject> &todos)
{
  QHash<QString, QString> padre;
  for (const auto &o : todos)
    padre[o["name"].toString()] = o["pkname"].toString();
  QSet<QString> sistema;
  for (const auto &o : todos) {
    bool montado = false;
    for (const auto &m : o["mountpoints"].toArray())
      montado = montado || esMontajeDelSistema(m.toString());
    if (!montado) continue;
    QString n = o["name"].toString();
    for (int guarda = 0; guarda < 16 && !padre.value(n).isEmpty(); ++guarda)
      n = padre.value(n);
    sistema.insert(n);
  }
  return sistema;
}

static Widget::DeviceInfo infoDeDisco(const QJsonObject &dev, const QSet<QString> &sistema)
{
  Widget::DeviceInfo info;
  info.path = dev["path"].toString();
  info.isRemovable = dev["rm"].toVariant().toBool() || dev["tran"].toString() == "usb";
  info.isSystem = sistema.contains(dev["name"].toString());
  info.sizeBytes = dev["size"].toVariant().toLongLong();
  const double gb = dev["size"].toVariant().toDouble() / 1e9;
  info.label = QString("%1  %2 GB  %3").arg(info.path).arg(gb, 0, 'f', 1)
                   .arg(dev["model"].toString().trimmed());
  return info;
}

Widget::DeviceInfo Widget::deviceInfo(const QString &path)
{
  const auto todos = consultarDiscos();
  const auto sistema = discosDelSistema(todos);
  for (const auto &dev : todos)
    if (dev["type"].toString() == "disk" && dev["path"].toString() == path)
      return infoDeDisco(dev, sistema);
  DeviceInfo desconocido;
  desconocido.path = path;
  desconocido.isSystem = true;  // si no se puede comprobar, no se permite
  return desconocido;
}

// Lista de discos: el del sistema aparece desactivado (no se puede elegir),
// los internos se marcan como tales y se seleccionan primero los USB.
void Widget::queryDeviceList() {
  const auto todos = consultarDiscos();
  const auto sistema = discosDelSistema(todos);
  ui->selectDevice_combo->clear();
  auto *modelo = qobject_cast<QStandardItemModel *>(ui->selectDevice_combo->model());
  int primeroUsb = -1, primeroValido = -1;

  for (const auto &dev : todos) {
    if (dev["type"].toString() != "disk")
      continue;  // sin loop, rom, zram, particiones...
    const DeviceInfo info = infoDeDisco(dev, sistema);
    QString texto = info.label;
    if (info.isSystem)
      texto += "  " + tr("(system disk, not available)");
    else if (!info.isRemovable)
      texto += "  " + tr("(internal disk)");
    ui->selectDevice_combo->addItem(texto, info.isSystem ? QString() : info.path);
    const int idx = ui->selectDevice_combo->count() - 1;
    if (info.isSystem) {
      if (modelo && modelo->item(idx))
        modelo->item(idx)->setEnabled(false);
    } else {
      if (primeroValido < 0) primeroValido = idx;
      if (info.isRemovable && primeroUsb < 0) primeroUsb = idx;
    }
  }

  const int sel = primeroUsb >= 0 ? primeroUsb : primeroValido;
  ui->selectDevice_combo->setCurrentIndex(sel);
  ui->btnInstall->setEnabled(sel >= 0);
}

void Widget::switchToInstaller() {
  emit getArchiveVentoyVersion();
  emit queryDeviceList();
  ui->mainPager->setCurrentIndex(1);
}

void Widget::verifyVentoyFeatures() {
  enableCheckBox(m_archiveVer >= SemanticVersion(1, 0, 28),
                 ui->chkUseCustomVolLabel);
  enableCheckBox(m_archiveVer >= SemanticVersion(1, 0, 15),
                 ui->chkUseGptPartTable);
  enableCheckBox(m_archiveVer >= SemanticVersion(1, 0, 7),
                 ui->chkEnableSecureBoot);
  if (m_comboModo)
    actualizarOpcionesSegunModo();
}

void Widget::getArchiveVentoyVersion() {
  QFile ventoyVersion("ventoy/version");
  if (ventoyVersion.open(QFile::ReadWrite)) {
    QString version = ventoyVersion.readAll();
    version.chop(1);
    ui->lblSelectedVentoyVerVal->setText(version);
    m_archiveVer = version;
    QSettings settings;
    settings.setValue("lastVentoyVersion", version);
    verifyVentoyFeatures();
    ventoyVersion.close();
  }
}

void Widget::installVentoy(const QString &aDevice, const QStringList &aParams) {
  const QStringList params = aParams + QStringList{aDevice};
  ui->mainPager->setCurrentIndex(2);
  ui->btnBack->setVisible(false);
  ui->textInstallLogs->clear();
  ui->lblSuccessFailImage->setVisible(false);
  ui->spinnerInstall->setVisible(true);
  ui->spinnerInstall->start();

  // Antes se lanzaba "Ventoy2Disk.sh" sin ruta: QProcess lo buscaba en el
  // PATH, no en la carpeta actual, el proceso no llegaba a arrancar y la
  // pantalla se quedaba para siempre en "instalacion en curso".
  const QString ventoyDir = findDVIWorkFile(tmpDir, true);
  if (ventoyDir.isEmpty()) {
    ui->textInstallLogs->setPlainText(
        tr("Could not find Linux archive in latest release."));
    finishedInstallation();
    return;
  }
  // Se llama directamente a tool/VentoyWorker.sh con el mismo entorno que
  // prepara Ventoy2Disk.sh: este le pasa los argumentos con $* sin comillas,
  // asi que una etiqueta con espacios ("AS lite") se partia y Ventoy fallaba
  // con "lite is NOT a valid device". Las herramientas ya estan
  // descomprimidas (findDVIWorkFile ejecuta Ventoy2Disk.sh una vez).
  const QString arch = QSysInfo::currentCpuArchitecture();
  const QString toolDir = arch.contains("arm64") || arch.contains("aarch64") ? "aarch64"
                        : arch.contains("x86_64") ? "x86_64"
                        : arch.contains("mips64") ? "mips64el" : "i386";
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("TOOLDIR", toolDir);
  env.insert("PATH", ventoyDir + "/tool/" + toolDir + ":" + env.value("PATH"));
  m_installProcess.setProcessEnvironment(env);
  m_installProcess.setWorkingDirectory(ventoyDir);
  m_installProcess.setProcessChannelMode(QProcess::MergedChannels);
  m_installProcess.start(QStringLiteral("bash"),
                         QStringList{QStringLiteral("./tool/VentoyWorker.sh")} + params);
  // Respuestas a las confirmaciones "Continue? (y/n)" del script; al cerrar
  // la entrada, cualquier pregunta adicional recibe fin de fichero en vez
  // de quedarse esperando.
  m_installProcess.write("y\ny\ny\ny\n");
  m_installProcess.closeWriteChannel();
}

void Widget::forceStopAll() {
  m_installProcess.kill();
  m_unarchiveProcess.kill();
  m_installProcess.waitForFinished();
  m_unarchiveProcess.waitForFinished();
}

void Widget::getDeviceVentoyVersion() {
  QProcess getDeviceVer;
  bool forceExit = false;
  QString vtoyfatPath;
  if (m_archiveVer >= SemanticVersion(1, 0, 30)) {
    // Since Ventoy 1.0.x the bundled tool is vtoycli (vtoyfat was dropped)
    vtoyfatPath = "tool/x86_64/vtoycli fat";
  } else {
    vtoyfatPath = "tool/vtoyfat_32";
  }

  getDeviceVer.start("sh");
  getDeviceVer.waitForStarted();
  getDeviceVer.write(". tool/ventoy_lib.sh\n");
  getDeviceVer.write(QString("%1 `get_disk_part_name %2 2`\n")
                         .arg(vtoyfatPath)
                         .arg(ui->selectDevice_combo->currentData().toString())
                         .toStdString()
                         .data());
  getDeviceVer.write("exit\n");
  if (!getDeviceVer.waitForFinished(100)) {
    forceExit = true;
    getDeviceVer.kill();
    getDeviceVer.waitForFinished();
  }
  qInfo() << getDeviceVer.exitCode() << forceExit
          << QString(getDeviceVer.readAllStandardError());
  if (!getDeviceVer.exitCode() && !forceExit) {
    QString deviceVer = getDeviceVer.readAll();
    deviceVer.chop(1);
    ui->lblDeviceVentoyVerVal->setText(deviceVer);
    m_DeviceVer = deviceVer;
  } else {
    ui->lblDeviceVentoyVerVal->setText("N/A");
    m_DeviceVer = SemanticVersion(0, 0, 0);
  }

  if (!m_DeviceVer.invalid()) {
    if (m_DeviceVer > m_archiveVer) {
      ui->lblTips->setText(tr("Will force install to the drive!"));
    } else if (m_DeviceVer < m_archiveVer) {
      ui->lblTips->setText(
          tr("Will upgrade Ventoy. Volume label will remain untouched."));
    } else {
      ui->lblTips->clear();
    }
  } else {
    ui->lblTips->clear();
  }
}

void Widget::initWidget() {
  tmpDir = QDir("/tmp/dvtoyinst");

  // Iconos claros u oscuros segun el tema. Antes solo se aplicaba al cambiar
  // de tema, asi que con el sistema ya en modo oscuro al abrir la app se
  // quedaban los iconos claros.
  connect(DGuiApplicationHelper::instance(),
          &DGuiApplicationHelper::themeTypeChanged, this,
          [this](DGuiApplicationHelper::ColorType themeType) {
            changeTheme(themeType == DGuiApplicationHelper::DarkType);
          });
  changeTheme(DGuiApplicationHelper::instance()->themeType() ==
              DGuiApplicationHelper::DarkType);

  connect(ui->lblPictureDropArchive, SIGNAL(droppedArchive(QString, bool)),
          this, SLOT(processVentoyArchive(QString, bool)));
  connect(&m_unarchiveProcess,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [&](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitCode);
            Q_UNUSED(exitStatus);
            finishedUnarchive();
          });
  connect(&m_installProcess, &QProcess::errorOccurred, this,
          [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
              ui->textInstallLogs->appendPlainText(m_installProcess.errorString());
              finishedInstallation();
            }
          });
  connect(&m_installProcess,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [&](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitCode);
            Q_UNUSED(exitStatus);
            finishedInstallation();
          });
  connect(ui->btnQueryDeviceVentoyVer, SIGNAL(clicked()), this,
          SLOT(getDeviceVentoyVersion()));
  connect(ui->btnRefreshDeviceList, SIGNAL(clicked()), this,
          SLOT(queryDeviceList()));
  connect(ui->btnUsePreviousExtraction, SIGNAL(clicked()), this,
          SLOT(switchToInstaller()));
  connect(ui->btnInstall, SIGNAL(clicked()), this, SLOT(proceedInstallation()));
  connect(ui->btnBack, &QPushButton::clicked,
          [&]() { ui->mainPager->setCurrentIndex(1); });

  connect(&m_installProcess, SIGNAL(readyReadStandardOutput()), this,
          SLOT(readInstallLog()));

  ui->lblSuccessFailImage->setVisible(false);
  ui->textInstallLogs->setReadOnly(true);

  ui->lineCustomVolLabel->lineEdit()->setMaxLength(15);  // limite de exFAT

  // Opciones adicionales: modo de instalacion y espacio reservado
  m_comboModo = new QComboBox(this);
  m_comboModo->addItem(tr("Automatic (install or update as needed)"));
  m_comboModo->addItem(tr("Install (erases ALL data)"));
  m_comboModo->addItem(tr("Non-destructive install (keeps your files)"));
  m_comboModo->addItem(tr("Update Ventoy (keeps your ISO files)"));
  m_comboModo->addItem(tr("Force reinstall (erases ALL data)"));
  auto *filaModo = new QHBoxLayout();
  filaModo->addWidget(new QLabel(tr("Mode:"), this));
  filaModo->addWidget(m_comboModo, 1);
  ui->verticalLayout_2->insertLayout(0, filaModo);

  m_chkReservar = new QCheckBox(tr("Reserve space at the end of the disk"), this);
  m_spinReservar = new QSpinBox(this);
  m_spinReservar->setRange(0, 1024 * 1024);
  m_spinReservar->setSingleStep(1024);
  m_spinReservar->setValue(4096);
  m_spinReservar->setSuffix(QStringLiteral(" MiB"));
  m_spinReservar->setEnabled(false);
  auto *filaReservar = new QHBoxLayout();
  filaReservar->addWidget(m_chkReservar);
  filaReservar->addWidget(m_spinReservar);
  filaReservar->addStretch();
  ui->verticalLayout_2->insertLayout(
      ui->verticalLayout_2->indexOf(ui->chkUseGptPartTable) + 1, filaReservar);

  connect(m_comboModo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { actualizarOpcionesSegunModo(); });
  connect(m_chkReservar, &QCheckBox::toggled, this,
          [this](bool) { actualizarOpcionesSegunModo(); });

  m_networkManager = new QNetworkAccessManager(this);
  m_currentDownload = nullptr;
  m_downloadRequested = false;

  goBackDropArchive();
  checkForUpdates();
}

void Widget::on_btnDownloadLatest_clicked() {
  m_downloadRequested = true;
  checkForUpdates();
}

void Widget::checkForUpdates() {
  ui->btnDownloadLatest->setEnabled(false);
  ui->lblHintDropArchive->setText(tr("Fetching latest release information..."));

  QNetworkRequest request(
      QUrl("https://api.github.com/repos/ventoy/Ventoy/releases/latest"));
  request.setHeader(QNetworkRequest::UserAgentHeader, "DeepinVentoyInstaller");

  QNetworkReply *reply = m_networkManager->get(request);
  connect(reply, &QNetworkReply::finished,
          [this, reply]() { onLatestReleaseInfoFetched(reply); });
}

void Widget::onLatestReleaseInfoFetched(QNetworkReply *reply) {
  reply->deleteLater();
  if (reply->error() != QNetworkReply::NoError) {
    ui->lblHintDropArchive->setText(
        tr("Failed to fetch release info: %1").arg(reply->errorString()));
    ui->btnDownloadLatest->setEnabled(true);
    return;
  }

  QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
  QJsonObject root = json.object();
  QString tagName = root["tag_name"].toString();
  if (tagName.startsWith('v')) {
    tagName.remove(0, 1);
  }
  m_latestAvailableVer = SemanticVersion(tagName);
  if (m_latestAvailableVer.invalid()) {
    ui->lblHintDropArchive->setText(
        tr("Could not parse the latest release version."));
    ui->btnDownloadLatest->setEnabled(true);
    return;
  }

  if (m_downloadRequested) {
    m_downloadRequested = false;
    QJsonArray assets = root["assets"].toArray();

    QString downloadUrl;
    QString fileName;

    for (const QJsonValue &value : assets) {
      QJsonObject asset = value.toObject();
      QString name = asset["name"].toString();
      if (name.endsWith("-linux.tar.gz")) {
        downloadUrl = asset["browser_download_url"].toString();
        fileName = name;
        break;
      }
    }

    if (downloadUrl.isEmpty()) {
      ui->lblHintDropArchive->setText(
          tr("Could not find Linux archive in latest release."));
      ui->btnDownloadLatest->setEnabled(true);
      return;
    }

    ui->lblHintDropArchive->setText(tr("Downloading %1...").arg(fileName));

    QNetworkRequest request(downloadUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    m_currentDownload = m_networkManager->get(request);

    connect(m_currentDownload, &QNetworkReply::downloadProgress, this,
            &Widget::onDownloadProgress);
    connect(m_currentDownload, &QNetworkReply::finished,
            [this]() { onArchiveDownloaded(m_currentDownload); });
  } else {
    notifyUpdateAvailability();
  }
}

void Widget::notifyUpdateAvailability() {
  ui->btnDownloadLatest->setEnabled(true);

  QSettings settings;
  SemanticVersion lastVer(settings.value("lastVentoyVersion").toString());

  // La aplicacion necesita la ultima version de Ventoy para funcionar: se
  // descarga automaticamente si no hay ninguna descomprimida o si la que hay
  // es antigua. La descompresion vive en /tmp y se pierde al reiniciar, asi
  // que se comprueba que exista de verdad y no solo el ajuste guardado.
  const bool hayExtraccion = !findDVIWorkFile(tmpDir, true).isEmpty();
  if (!hayExtraccion || lastVer.invalid() || lastVer < m_latestAvailableVer) {
    m_downloadRequested = true;
    checkForUpdates();
    return;
  }

  ui->lblHintDropArchive->setText(
      tr("You have the latest Ventoy version (%1).")
          .arg(QString(lastVer)));
}

void Widget::onArchiveDownloaded(QNetworkReply *reply) {
  reply->deleteLater();
  m_currentDownload = nullptr;

  if (reply->error() != QNetworkReply::NoError) {
    ui->lblHintDropArchive->setText(
        tr("Download failed: %1").arg(reply->errorString()));
    ui->btnDownloadLatest->setEnabled(true);
    return;
  }

  QTemporaryFile *tempFile = new QTemporaryFile(this);
  tempFile->setFileTemplate(QDir::tempPath() + "/ventoy-XXXXXX.tar.gz");
  if (tempFile->open()) {
    tempFile->write(reply->readAll());
    QString path = tempFile->fileName();
    tempFile->close();

    ui->lblHintDropArchive->setText(tr("Download complete. Extracting..."));
    processVentoyArchive(path, false);
  } else {
    ui->lblHintDropArchive->setText(tr("Failed to save downloaded file."));
    ui->btnDownloadLatest->setEnabled(true);
  }
}

void Widget::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
  if (bytesTotal > 0) {
    int percentage = static_cast<int>(bytesReceived * 100 / bytesTotal);
    ui->lblHintDropArchive->setText(tr("Downloading... %1%").arg(percentage));
  }
}

void Widget::on_chkUseCustomVolLabel_stateChanged(int checkState) {
  bool checked = (checkState == Qt::CheckState::Checked);
  ui->lineCustomVolLabel->setEnabled(checked);
  ui->lineCustomVolLabel->lineEdit()->setEnabled(checked);
  if (!checked) {
    ui->lineCustomVolLabel->lineEdit()->clear();
  }
}

void Widget::on_selectDevice_combo_currentIndexChanged(int index) {
  Q_UNUSED(index);
  getDeviceVentoyVersion();
}
