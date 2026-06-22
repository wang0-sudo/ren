#include <memory>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSslSocket>
#include <QTimer>

#include "学习服务.h"
#include "学习窗口.h"

namespace {
QString resolveStudyDataPath() {
    const QDir appDir(QCoreApplication::applicationDirPath());

    const QString localDbPath = appDir.filePath("学习助手数据.db");
    if (QFileInfo::exists(localDbPath)) {
        return localDbPath;
    }

    const QString parentDbPath = QDir(appDir.filePath("..")).filePath("学习助手数据.db");
    if (QFileInfo::exists(parentDbPath)) {
        return parentDbPath;
    }

    return parentDbPath;
}
}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    if (QSslSocket::availableBackends().contains("schannel")) {
        QSslSocket::setActiveBackend("schannel");
    }

    auto service = std::make_shared<StudyService>(resolveStudyDataPath().toStdString());
    service->loadOrSeed();

    StudyAssistantWindow window(service);
    window.show();

    if (app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(200, &app, &QApplication::quit);
    }

    return app.exec();
}
