#pragma once

#include <memory>

#include <QWidget>

#include "招聘服务.h"

class QComboBox;
class QLabel;
class QLineEdit;

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(std::shared_ptr<RecruitmentService> service,
                         QWidget* parent = nullptr);

private:
    void buildUi();
    void handleLogin();
    void handlePersonalRegister();
    void handleEnterpriseRegister();

    std::shared_ptr<RecruitmentService> service_;

    QComboBox* roleComboBox_ = nullptr;
    QLineEdit* usernameEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QLabel* tipLabel_ = nullptr;
};
