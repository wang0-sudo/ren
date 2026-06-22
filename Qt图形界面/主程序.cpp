#include <memory>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "登录窗口.h"
#include "招聘服务.h"

namespace {
QString resolveDataFilePath() {
    const QDir appDir(QCoreApplication::applicationDirPath());

    const QString localPath = appDir.filePath("招聘数据.txt");
    if (QFileInfo::exists(localPath)) {
        return localPath;
    }

    const QString parentPath = QDir(appDir.filePath("..")).filePath("招聘数据.txt");
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
