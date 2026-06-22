#include "登录窗口.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "工作台窗口.h"

namespace {
std::string stdstr(const QString& text) {
    return text.toStdString();
}

struct PersonalRegistrationFormData {
    QString username;
    QString password;
    QString name;
    QString gender;
    QString phone;
    QString email;
    QString education;
    QString experience;
    QString skills;
    QString selfIntroduction;
};

struct EnterpriseRegistrationFormData {
    QString username;
    QString password;
    QString companyName;
    QString contactPerson;
    QString phone;
    QString email;
    QString address;
    QString introduction;
};

bool promptPersonalRegistrationDialog(QWidget* parent, PersonalRegistrationFormData& data) {
    QDialog dialog(parent);
    dialog.setWindowTitle("个人用户注册");
    dialog.resize(560, 520);
    QFormLayout form(&dialog);

    QLineEdit usernameEdit(&dialog);
    QLineEdit passwordEdit(&dialog);
    QLineEdit confirmPasswordEdit(&dialog);
    QLineEdit nameEdit(&dialog);
    QLineEdit genderEdit(&dialog);
    QLineEdit phoneEdit(&dialog);
    QLineEdit emailEdit(&dialog);
    QLineEdit educationEdit(&dialog);
    QLineEdit skillsEdit(&dialog);
    QPlainTextEdit experienceEdit(&dialog);
    QPlainTextEdit selfIntroductionEdit(&dialog);

    passwordEdit.setEchoMode(QLineEdit::Password);
    confirmPasswordEdit.setEchoMode(QLineEdit::Password);
    experienceEdit.setMinimumHeight(90);
    selfIntroductionEdit.setMinimumHeight(100);

    form.addRow("用户名：", &usernameEdit);
    form.addRow("密码：", &passwordEdit);
    form.addRow("确认密码：", &confirmPasswordEdit);
    form.addRow("姓名：", &nameEdit);
    form.addRow("性别：", &genderEdit);
    form.addRow("电话：", &phoneEdit);
    form.addRow("邮箱：", &emailEdit);
    form.addRow("学历：", &educationEdit);
    form.addRow("工作经验：", &experienceEdit);
    form.addRow("技能特长：", &skillsEdit);
    form.addRow("自我介绍：", &selfIntroductionEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(&buttons);

    QObject::connect(&buttons, &QDialogButtonBox::accepted, [&]() {
        if (usernameEdit.text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "用户名不能为空。");
            return;
        }
        if (passwordEdit.text().isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "密码不能为空。");
            return;
        }
        if (passwordEdit.text() != confirmPasswordEdit.text()) {
            QMessageBox::warning(&dialog, "输入错误", "两次输入的密码不一致。");
            return;
        }
        if (nameEdit.text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "姓名不能为空。");
            return;
        }
        dialog.accept();
    });
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    data.username = usernameEdit.text().trimmed();
    data.password = passwordEdit.text();
    data.name = nameEdit.text().trimmed();
    data.gender = genderEdit.text().trimmed();
    data.phone = phoneEdit.text().trimmed();
    data.email = emailEdit.text().trimmed();
    data.education = educationEdit.text().trimmed();
    data.experience = experienceEdit.toPlainText().trimmed();
    data.skills = skillsEdit.text().trimmed();
    data.selfIntroduction = selfIntroductionEdit.toPlainText().trimmed();
    return true;
}

bool promptEnterpriseRegistrationDialog(QWidget* parent, EnterpriseRegistrationFormData& data) {
    QDialog dialog(parent);
    dialog.setWindowTitle("企业用户注册");
    dialog.resize(560, 460);
    QFormLayout form(&dialog);

    QLineEdit usernameEdit(&dialog);
    QLineEdit passwordEdit(&dialog);
    QLineEdit confirmPasswordEdit(&dialog);
    QLineEdit companyNameEdit(&dialog);
    QLineEdit contactPersonEdit(&dialog);
    QLineEdit phoneEdit(&dialog);
    QLineEdit emailEdit(&dialog);
    QLineEdit addressEdit(&dialog);
    QPlainTextEdit introductionEdit(&dialog);

    passwordEdit.setEchoMode(QLineEdit::Password);
    confirmPasswordEdit.setEchoMode(QLineEdit::Password);
    introductionEdit.setMinimumHeight(120);

    form.addRow("用户名：", &usernameEdit);
    form.addRow("密码：", &passwordEdit);
    form.addRow("确认密码：", &confirmPasswordEdit);
    form.addRow("企业名称：", &companyNameEdit);
    form.addRow("联系人：", &contactPersonEdit);
    form.addRow("电话：", &phoneEdit);
    form.addRow("邮箱：", &emailEdit);
    form.addRow("地址：", &addressEdit);
    form.addRow("企业简介：", &introductionEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(&buttons);

    QObject::connect(&buttons, &QDialogButtonBox::accepted, [&]() {
        if (usernameEdit.text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "用户名不能为空。");
            return;
        }
        if (passwordEdit.text().isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "密码不能为空。");
            return;
        }
        if (passwordEdit.text() != confirmPasswordEdit.text()) {
            QMessageBox::warning(&dialog, "输入错误", "两次输入的密码不一致。");
            return;
        }
        if (companyNameEdit.text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "企业名称不能为空。");
            return;
        }
        dialog.accept();
    });
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    data.username = usernameEdit.text().trimmed();
    data.password = passwordEdit.text();
    data.companyName = companyNameEdit.text().trimmed();
    data.contactPerson = contactPersonEdit.text().trimmed();
    data.phone = phoneEdit.text().trimmed();
    data.email = emailEdit.text().trimmed();
    data.address = addressEdit.text().trimmed();
    data.introduction = introductionEdit.toPlainText().trimmed();
    return true;
}
}  // namespace

LoginWindow::LoginWindow(std::shared_ptr<RecruitmentService> service, QWidget* parent)
    : QWidget(parent), service_(std::move(service)) {
    buildUi();
}

void LoginWindow::buildUi() {
    setWindowTitle("招聘系统 - 登录");
    resize(460, 320);

    auto* layout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel("人才招聘系统", this);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold;");

    auto* subtitleLabel =
        new QLabel("", this);
    subtitleLabel->setWordWrap(true);

    roleComboBox_ = new QComboBox(this);
    roleComboBox_->addItem("个人用户");
    roleComboBox_->addItem("企业用户");
    roleComboBox_->addItem("管理员");

    usernameEdit_ = new QLineEdit(this);
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);

    auto* formLayout = new QFormLayout();
    formLayout->addRow("登录角色：", roleComboBox_);
    formLayout->addRow("用户名：", usernameEdit_);
    formLayout->addRow("密码：", passwordEdit_);

    auto* loginButton = new QPushButton("登录", this);
    auto* clearButton = new QPushButton("清空", this);
    auto* personalRegisterButton = new QPushButton("个人注册", this);
    auto* enterpriseRegisterButton = new QPushButton("企业注册", this);
    auto* actionLayout = new QHBoxLayout();
    actionLayout->addWidget(loginButton);
    actionLayout->addWidget(clearButton);
    actionLayout->addWidget(personalRegisterButton);
    actionLayout->addWidget(enterpriseRegisterButton);

    tipLabel_ = new QLabel("示例账号：alice / futurehr / admin。新用户可先注册，注册后等待管理员审核。", this);
    tipLabel_->setWordWrap(true);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        usernameEdit_->clear();
        passwordEdit_->clear();
        usernameEdit_->setFocus();
    });
    connect(personalRegisterButton, &QPushButton::clicked, this,
            &LoginWindow::handlePersonalRegister);
    connect(enterpriseRegisterButton, &QPushButton::clicked, this,
            &LoginWindow::handleEnterpriseRegister);

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addLayout(formLayout);
    layout->addLayout(actionLayout);
    layout->addWidget(tipLabel_);
}

void LoginWindow::handleLogin() {
    const QString username = usernameEdit_->text().trimmed();
    const QString password = passwordEdit_->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "输入不完整", "请输入用户名和密码。");
        return;
    }

    const int roleIndex = roleComboBox_->currentIndex();
    UserRole role = UserRole::Personal;
    int currentUserId = -1;

    if (roleIndex == 0) {
        PersonalUser* user = service_->loginPersonal(username.toStdString(), password.toStdString());
        if (user == nullptr) {
            QMessageBox::warning(this, "登录失败", "个人用户账号或密码错误。");
            return;
        }
        role = UserRole::Personal;
        currentUserId = user->id;
    } else if (roleIndex == 1) {
        EnterpriseUser* user =
            service_->loginEnterprise(username.toStdString(), password.toStdString());
        if (user == nullptr) {
            QMessageBox::warning(this, "登录失败", "企业用户账号或密码错误。");
            return;
        }
        role = UserRole::Enterprise;
        currentUserId = user->id;
    } else {
        if (!service_->loginAdmin(username.toStdString(), password.toStdString())) {
            QMessageBox::warning(this, "登录失败", "管理员账号或密码错误。");
            return;
        }
        role = UserRole::Admin;
    }

    auto* dashboard = new DashboardWindow(service_, role, currentUserId);
    dashboard->setAttribute(Qt::WA_DeleteOnClose);
    dashboard->show();
    hide();

    connect(dashboard, &QObject::destroyed, this, [this]() {
        passwordEdit_->clear();
        show();
    });
}

void LoginWindow::handlePersonalRegister() {
    PersonalRegistrationFormData data;
    if (!promptPersonalRegistrationDialog(this, data)) {
        return;
    }

    Resume resume;
    resume.education = stdstr(data.education);
    resume.experience = stdstr(data.experience);
    resume.skills = stdstr(data.skills);
    resume.selfIntroduction = stdstr(data.selfIntroduction);

    std::string error;
    if (!service_->registerPersonalUser(stdstr(data.username), stdstr(data.password),
                                        stdstr(data.name), stdstr(data.gender),
                                        stdstr(data.phone), stdstr(data.email), resume, error)) {
        QMessageBox::warning(this, "注册失败", QString::fromStdString(error));
        return;
    }

    roleComboBox_->setCurrentIndex(0);
    usernameEdit_->setText(data.username);
    passwordEdit_->clear();
    QMessageBox::information(this, "注册成功",
                             "个人用户注册成功，当前账号已进入待审核状态。");
}

void LoginWindow::handleEnterpriseRegister() {
    EnterpriseRegistrationFormData data;
    if (!promptEnterpriseRegistrationDialog(this, data)) {
        return;
    }

    std::string error;
    if (!service_->registerEnterpriseUser(
            stdstr(data.username), stdstr(data.password), stdstr(data.companyName),
            stdstr(data.contactPerson), stdstr(data.phone), stdstr(data.email),
            stdstr(data.address), stdstr(data.introduction), error)) {
        QMessageBox::warning(this, "注册失败", QString::fromStdString(error));
        return;
    }

    roleComboBox_->setCurrentIndex(1);
    usernameEdit_->setText(data.username);
    passwordEdit_->clear();
    QMessageBox::information(this, "注册成功",
                             "企业用户注册成功，当前账号已进入待审核状态。");
}
