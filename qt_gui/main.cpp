#include <memory>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "login_window.h"
#include "recruitment_service.h"

namespace {
QString resolveDataFilePath() {
    const QDir appDir(QCoreApplication::applicationDirPath());

    const QString localPath = appDir.filePath("recruitment_data.txt");
    if (QFileInfo::exists(localPath)) {
        return localPath;
    }

    const QString parentPath = QDir(appDir.filePath("..")).filePath("recruitment_data.txt");
    if (QFileInfo::exists(parentPath)) {
        return parentPath;
    }

    return parentPath;
}
}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    const QString dataFilePath = resolveDataFilePath();
    auto service = std::make_shared<RecruitmentService>(dataFilePath.toStdString());
    service->loadOrSeed();

    LoginWindow window(service);
    window.show();

    return app.exec();
}
