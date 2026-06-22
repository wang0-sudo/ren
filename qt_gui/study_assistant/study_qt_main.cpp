#include <memory>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSslSocket>
#include <QTimer>

#include "study_service.h"
#include "study_window.h"

namespace {
QString resolveStudyDataPath() {
    const QDir appDir(QCoreApplication::applicationDirPath());

    const QString localDbPath = appDir.filePath("study_assistant_data.db");
    if (QFileInfo::exists(localDbPath)) {
        return localDbPath;
    }

    const QString parentDbPath = QDir(appDir.filePath("..")).filePath("study_assistant_data.db");
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
