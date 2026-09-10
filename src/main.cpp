#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMainWindow>
#include <QMessageBox>
#include <QTimer>
#include <QTextStream>

#include "MyPlugin.h"
#include "core/ProjectModel.h"
#include "netinterface.h"
#include "tessinterface.h"

namespace {

QString findProjectRoot(const QDir& executableDir)
{
    QDir candidate(executableDir);
    for (int depth = 0; depth < 5; ++depth) {
        const bool hasScenario = QFileInfo::exists(
            candidate.filePath(QStringLiteral("config/scenario.json")));
        const bool hasNetwork = QFileInfo::exists(
            candidate.filePath(QStringLiteral(
                "WorkSpace/Data/雄安区路网_C++本地仿真版.tess")));
        if (hasScenario && hasNetwork) {
            return candidate.absolutePath();
        }
        if (!candidate.cdUp()) {
            break;
        }
    }
    return QString();
}

}  // namespace

int main(int argc, char* argv[])
{
    const QFileInfo executable(QString::fromLocal8Bit(argv[0]));
    const QDir runtimeDir = executable.absoluteDir();

    const QString projectRoot = findProjectRoot(runtimeDir);
    if (projectRoot.isEmpty()) {
        return 2;
    }
    const QDir projectDir(projectRoot);

    // TESSNG 4.1.3 ships with Qt 5.15.16, while the build SDK is Qt 5.15.2.
    // Visual Studio may inherit QT_PLUGIN_PATH from the build SDK.  Mixing its
    // qwindows.dll with the deployed TESSNG runtime makes the platform plugin
    // visible but impossible to initialize.  Pin all runtime plugin discovery
    // to the coherent bundle beside this executable before QApplication starts.
    const QString pluginDir = runtimeDir.filePath(QStringLiteral("plugins"));
    const QString platformDir = QDir(pluginDir).filePath(QStringLiteral("platforms"));
    if (!QFileInfo::exists(QDir(platformDir).filePath(QStringLiteral("qwindows.dll")))) {
        return 7;
    }
    qputenv("QT_PLUGIN_PATH", QDir::toNativeSeparators(pluginDir).toLocal8Bit());
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
            QDir::toNativeSeparators(platformDir).toLocal8Bit());
    qputenv("QML2_IMPORT_PATH",
            QDir::toNativeSeparators(runtimeDir.absolutePath()).toLocal8Bit());
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication application(argc, argv);
    QCoreApplication::setLibraryPaths({pluginDir});
    application.setApplicationName(QStringLiteral("雄安路网原生C++仿真系统"));
    const bool startupSmokeTest =
        application.arguments().contains(QStringLiteral("--startup-smoke-test"));

    const QString networkPath = projectDir.filePath(
        QStringLiteral("WorkSpace/Data/雄安区路网_C++本地仿真版.tess"));
    if (!QFileInfo::exists(networkPath)) {
        QMessageBox::critical(nullptr, QStringLiteral("启动失败"),
            QStringLiteral("找不到路网：\n%1").arg(QDir::toNativeSeparators(networkPath)));
        return 3;
    }

    auto* model = new ProjectModel();
    QString error;
    if (!model->load(projectRoot, &error)) {
        QMessageBox::critical(nullptr, QStringLiteral("配置失败"), error);
        delete model;
        return 4;
    }

    // Follow the official C++ lifecycle: create TESSNG first, register the
    // in-memory plugin, then open the network after the event loop starts.
    // Loading the .tess file in tessng(networkPath) consumes the network-load
    // boundary before CustomerNet/CustomerSimulator have been registered.
    QMainWindow* window = tessng();
    if (!window) {
        QMessageBox::critical(nullptr, QStringLiteral("启动失败"),
                              QStringLiteral("无法创建 TESSNG 4.1.3 主窗口"));
        delete model;
        return 5;
    }
    auto* plugin = new MyPlugin(model, projectRoot);
    plugin->init();
    const QString pluginDiagnosticPath =
        runtimeDir.filePath(QStringLiteral("plugin-load-diagnostic.txt"));
    QFile::remove(pluginDiagnosticPath);
    if (!gpTessInterface->loadPluginFromMem(plugin)) {
        const QString keyAccount = gpTessInterface->activeKeyAccount();
        const QString keyName = gpTessInterface->activeKeyName();
        QFile diagnostic(pluginDiagnosticPath);
        if (diagnostic.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream stream(&diagnostic);
            stream.setCodec("UTF-8");
            stream << "loadPluginFromMem=false\n"
                   << "activeKeyAccount=" << keyAccount << '\n'
                   << "activeKeyName=" << keyName << '\n';
        }
        if (!startupSmokeTest) {
            QMessageBox::critical(window, QStringLiteral("加载失败"),
                QStringLiteral("原生 C++ 插件加载失败。\n"
                               "TESSNG 授权账号：%1\n授权名称：%2")
                    .arg(keyAccount.isEmpty() ? QStringLiteral("未检测到") : keyAccount,
                         keyName.isEmpty() ? QStringLiteral("未检测到") : keyName));
        }
        delete plugin;
        window->deleteLater();
        return 6;
    }
    if (startupSmokeTest) {
        return 0;
    }
    QTimer::singleShot(0, window, [networkPath]() {
        gpTessInterface->netInterface()->openNetFle(networkPath);
    });
    window->showMaximized();
    const int result = application.exec();
    window->deleteLater();
    return result;
}
