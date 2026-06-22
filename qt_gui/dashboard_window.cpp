#include "dashboard_window.h"

#include <algorithm>
#include <initializer_list>
#include <string>

#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString qstr(const std::string& text) {
    return QString::fromStdString(text);
}

std::string stdstr(const QString& text) {
    return text.toStdString();
}

void prepareTable(QTableWidget* table, const QStringList& headers) {
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

bool matchesFilter(const QString& keyword, std::initializer_list<QString> fields) {
    if (keyword.trimmed().isEmpty()) {
        return true;
    }

    for (const auto& field : fields) {
        if (field.contains(keyword, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool promptPersonalInfoDialog(QWidget* parent,
                              const PersonalUser& user,
                              QString& name,
                              QString& gender,
                              QString& phone,
                              QString& email) {
    QDialog dialog(parent);
    dialog.setWindowTitle("修改个人信息");
    QFormLayout form(&dialog);

    QLineEdit nameEdit(qstr(user.name), &dialog);
    QLineEdit genderEdit(qstr(user.gender), &dialog);
    QLineEdit phoneEdit(qstr(user.phone), &dialog);
    QLineEdit emailEdit(qstr(user.email), &dialog);

    form.addRow("姓名：", &nameEdit);
    form.addRow("性别：", &genderEdit);
    form.addRow("电话：", &phoneEdit);
    form.addRow("邮箱：", &emailEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(&buttons);

    QObject::connect(&buttons, &QDialogButtonBox::accepted, [&]() {
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

    name = nameEdit.text().trimmed();
    gender = genderEdit.text().trimmed();
    phone = phoneEdit.text().trimmed();
    email = emailEdit.text().trimmed();
    return true;
}

bool promptResumeDialog(QWidget* parent, const Resume& resume, Resume& result) {
    QDialog dialog(parent);
    dialog.setWindowTitle("修改简历");
    dialog.resize(520, 420);
    QFormLayout form(&dialog);

    QLineEdit educationEdit(qstr(resume.education), &dialog);
    QPlainTextEdit experienceEdit(&dialog);
    QLineEdit skillsEdit(qstr(resume.skills), &dialog);
    QPlainTextEdit introEdit(&dialog);

    experienceEdit.setPlainText(qstr(resume.experience));
    introEdit.setPlainText(qstr(resume.selfIntroduction));
    experienceEdit.setMinimumHeight(90);
    introEdit.setMinimumHeight(120);

    form.addRow("学历：", &educationEdit);
    form.addRow("工作经验：", &experienceEdit);
    form.addRow("技能特长：", &skillsEdit);
    form.addRow("自我介绍：", &introEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(&buttons);

    QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    result.education = stdstr(educationEdit.text().trimmed());
    result.experience = stdstr(experienceEdit.toPlainText().trimmed());
    result.skills = stdstr(skillsEdit.text().trimmed());
    result.selfIntroduction = stdstr(introEdit.toPlainText().trimmed());
    return true;
}

bool promptPasswordDialog(QWidget* parent,
                          const QString& title,
                          QString& oldPassword,
                          QString& newPassword) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    QFormLayout form(&dialog);

    QLineEdit oldEdit(&dialog);
    QLineEdit newEdit(&dialog);
    QLineEdit confirmEdit(&dialog);
    oldEdit.setEchoMode(QLineEdit::Password);
    newEdit.setEchoMode(QLineEdit::Password);
    confirmEdit.setEchoMode(QLineEdit::Password);

    form.addRow("当前密码：", &oldEdit);
    form.addRow("新密码：", &newEdit);
    form.addRow("确认新密码：", &confirmEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(&buttons);

    QObject::connect(&buttons, &QDialogButtonBox::accepted, [&]() {
        if (newEdit.text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "新密码不能为空。");
            return;
        }
        if (newEdit.text() != confirmEdit.text()) {
            QMessageBox::warning(&dialog, "输入错误", "两次输入的新密码不一致。");
            return;
        }
        dialog.accept();
    });
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    oldPassword = oldEdit.text();
    newPassword = newEdit.text();
    return true;
}

bool promptTextDialog(QWidget* parent,
                      const QString& title,
                      const QString& label,
                      QString& text,
                      const QString& initialText = QString()) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(460, 280);
    QVBoxLayout layout(&dialog);

    auto* tipLabel = new QLabel(label, &dialog);
    auto* edit = new QPlainTextEdit(&dialog);
    edit->setPlainText(initialText);
    auto* buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

    layout.addWidget(tipLabel);
    layout.addWidget(edit);
    layout.addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, [&]() {
        if (edit->toPlainText().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "内容不能为空。");
            return;
        }
        dialog.accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    text = edit->toPlainText().trimmed();
    return true;
}

bool promptEnterpriseInfoDialog(QWidget* parent,
                                const EnterpriseUser& user,
                                QString& companyName,
                                QString& contactPerson,
                                QString& phone,
                                QString& email,
                                QString& address,
                                QString& introduction) {
    QDialog dialog(parent);
    dialog.setWindowTitle("修改企业信息");
    dialog.resize(520, 360);
    QFormLayout form(&dialog);

    QLineEdit companyNameEdit(qstr(user.companyName), &dialog);
    QLineEdit contactPersonEdit(qstr(user.contactPerson), &dialog);
    QLineEdit phoneEdit(qstr(user.phone), &dialog);
    QLineEdit emailEdit(qstr(user.email), &dialog);
    QLineEdit addressEdit(qstr(user.address), &dialog);
    QPlainTextEdit introductionEdit(&dialog);
    introductionEdit.setPlainText(qstr(user.introduction));
    introductionEdit.setMinimumHeight(120);

    form.addRow("企业名称：", &companyNameEdit);
    form.addRow("联系人：", &contactPersonEdit);
    form.addRow("电话：", &phoneEdit);
    form.addRow("邮箱：", &emailEdit);
    form.addRow("地址：", &addressEdit);
    form.addRow("企业简介：", &introductionEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(&buttons);

    QObject::connect(&buttons, &QDialogButtonBox::accepted, [&]() {
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

    companyName = companyNameEdit.text().trimmed();
    contactPerson = contactPersonEdit.text().trimmed();
    phone = phoneEdit.text().trimmed();
    email = emailEdit.text().trimmed();
    address = addressEdit.text().trimmed();
    introduction = introductionEdit.toPlainText().trimmed();
    return true;
}

bool promptPositionDialog(QWidget* parent,
                          const QString& dialogTitle,
                          const Position* position,
                          QString& title,
                          QString& location,
                          QString& salary,
                          QString& requirement,
                          QString& description) {
    QDialog dialog(parent);
    dialog.setWindowTitle(dialogTitle);
    dialog.resize(560, 420);
    QFormLayout form(&dialog);

    QLineEdit titleEdit(position == nullptr ? QString() : qstr(position->title), &dialog);
    QLineEdit locationEdit(position == nullptr ? QString() : qstr(position->location), &dialog);
    QLineEdit salaryEdit(position == nullptr ? QString() : qstr(position->salary), &dialog);
    QPlainTextEdit requirementEdit(&dialog);
    QPlainTextEdit descriptionEdit(&dialog);

    if (position != nullptr) {
        requirementEdit.setPlainText(qstr(position->requirement));
        descriptionEdit.setPlainText(qstr(position->description));
    }
    requirementEdit.setMinimumHeight(100);
    descriptionEdit.setMinimumHeight(120);

    form.addRow("岗位名称：", &titleEdit);
    form.addRow("工作地点：", &locationEdit);
    form.addRow("薪资待遇：", &salaryEdit);
    form.addRow("任职要求：", &requirementEdit);
    form.addRow("岗位描述：", &descriptionEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(&buttons);

    QObject::connect(&buttons, &QDialogButtonBox::accepted, [&]() {
        if (titleEdit.text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "岗位名称不能为空。");
            return;
        }
        dialog.accept();
    });
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    title = titleEdit.text().trimmed();
    location = locationEdit.text().trimmed();
    salary = salaryEdit.text().trimmed();
    requirement = requirementEdit.toPlainText().trimmed();
    description = descriptionEdit.toPlainText().trimmed();
    return true;
}
}  // namespace

DashboardWindow::DashboardWindow(std::shared_ptr<RecruitmentService> service,
                                 UserRole role,
                                 int currentUserId,
                                 QWidget* parent)
    : QMainWindow(parent),
      service_(std::move(service)),
      role_(role),
      currentUserId_(currentUserId) {
    buildUi();
    refreshData();
}

void DashboardWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    headerLabel_ = new QLabel(this);
    headerLabel_->setStyleSheet("font-size: 20px; font-weight: bold;");

    hintLabel_ = new QLabel("支持直接在窗口中修改数据，所有变更都会自动保存。", this);
    hintLabel_->setWordWrap(true);

    auto* refreshButton = new QPushButton("刷新数据", this);
    connect(refreshButton, &QPushButton::clicked, this, [this]() {
        service_->loadOrSeed();
        refreshData();
    });

    auto* topBar = new QHBoxLayout();
    topBar->addWidget(refreshButton);
    topBar->addStretch();

    if (role_ == UserRole::Admin) {
        auto* adminPasswordButton = new QPushButton("修改管理员密码", this);
        connect(adminPasswordButton, &QPushButton::clicked, this,
                &DashboardWindow::handleAdminChangePassword);
        topBar->addWidget(adminPasswordButton);
    }

    tabWidget_ = new QTabWidget(this);

    layout->addWidget(headerLabel_);
    layout->addLayout(topBar);
    layout->addWidget(hintLabel_);
    layout->addWidget(tabWidget_);

    setCentralWidget(central);
    resize(1180, 760);

    if (role_ == UserRole::Personal) {
        setWindowTitle("招聘系统 - 个人用户");
        buildPersonalTabs();
    } else if (role_ == UserRole::Enterprise) {
        setWindowTitle("招聘系统 - 企业用户");
        buildEnterpriseTabs();
    } else {
        setWindowTitle("招聘系统 - 管理员");
        buildAdminTabs();
    }
}

void DashboardWindow::buildPersonalTabs() {
    auto* profilePage = new QWidget(this);
    auto* profileLayout = new QVBoxLayout(profilePage);
    auto* profileButtons = new QHBoxLayout();
    auto* editInfoButton = new QPushButton("修改个人信息", profilePage);
    auto* editResumeButton = new QPushButton("修改简历", profilePage);
    auto* changePasswordButton = new QPushButton("修改密码", profilePage);
    auto* leaveMessageButton = new QPushButton("我要留言", profilePage);
    profileButtons->addWidget(editInfoButton);
    profileButtons->addWidget(editResumeButton);
    profileButtons->addWidget(changePasswordButton);
    profileButtons->addWidget(leaveMessageButton);
    profileButtons->addStretch();

    personalSummaryLabel_ = new QLabel(profilePage);
    personalSummaryLabel_->setWordWrap(true);
    personalResumeView_ = new QPlainTextEdit(profilePage);
    personalResumeView_->setReadOnly(true);

    profileLayout->addLayout(profileButtons);
    profileLayout->addWidget(personalSummaryLabel_);
    profileLayout->addWidget(personalResumeView_);

    auto* positionPage = new QWidget(this);
    auto* positionLayout = new QVBoxLayout(positionPage);
    auto* positionTop = new QHBoxLayout();
    personalPositionSearchEdit_ = new QLineEdit(positionPage);
    personalPositionSearchEdit_->setPlaceholderText("输入岗位、企业、地点关键字进行筛选");
    auto* applyButton = new QPushButton("申请选中岗位", positionPage);
    auto* refreshPositionButton = new QPushButton("刷新岗位列表", positionPage);
    positionTop->addWidget(personalPositionSearchEdit_);
    positionTop->addWidget(applyButton);
    positionTop->addWidget(refreshPositionButton);

    personalPositionsTable_ = new QTableWidget(positionPage);
    prepareTable(personalPositionsTable_,
                 {"岗位编号", "企业名称", "岗位名称", "地点", "薪资", "投递状态"});

    positionLayout->addLayout(positionTop);
    positionLayout->addWidget(personalPositionsTable_);

    auto* appliedPage = new QWidget(this);
    auto* appliedLayout = new QVBoxLayout(appliedPage);
    auto* appliedTop = new QHBoxLayout();
    auto* withdrawButton = new QPushButton("撤销选中申请", appliedPage);
    auto* refreshAppliedButton = new QPushButton("刷新申请记录", appliedPage);
    appliedTop->addWidget(withdrawButton);
    appliedTop->addWidget(refreshAppliedButton);
    appliedTop->addStretch();

    personalAppliedTable_ = new QTableWidget(appliedPage);
    prepareTable(personalAppliedTable_, {"岗位编号", "企业名称", "岗位名称", "地点", "薪资"});

    appliedLayout->addLayout(appliedTop);
    appliedLayout->addWidget(personalAppliedTable_);

    connect(editInfoButton, &QPushButton::clicked, this, &DashboardWindow::handlePersonalEditInfo);
    connect(editResumeButton, &QPushButton::clicked, this,
            &DashboardWindow::handlePersonalEditResume);
    connect(changePasswordButton, &QPushButton::clicked, this,
            &DashboardWindow::handlePersonalChangePassword);
    connect(leaveMessageButton, &QPushButton::clicked, this,
            &DashboardWindow::handlePersonalLeaveMessage);
    connect(applyButton, &QPushButton::clicked, this, &DashboardWindow::handlePersonalApplyPosition);
    connect(withdrawButton, &QPushButton::clicked, this,
            &DashboardWindow::handlePersonalWithdrawPosition);
    connect(refreshPositionButton, &QPushButton::clicked, this,
            [this]() { populatePersonalView(); });
    connect(refreshAppliedButton, &QPushButton::clicked, this,
            [this]() { populatePersonalView(); });
    connect(personalPositionSearchEdit_, &QLineEdit::textChanged, this,
            [this]() { populatePersonalView(); });

    tabWidget_->addTab(profilePage, "个人资料");
    tabWidget_->addTab(positionPage, "职位浏览");
    tabWidget_->addTab(appliedPage, "已申请岗位");
}

void DashboardWindow::buildEnterpriseTabs() {
    auto* infoPage = new QWidget(this);
    auto* infoLayout = new QVBoxLayout(infoPage);
    auto* infoButtons = new QHBoxLayout();
    auto* editInfoButton = new QPushButton("修改企业信息", infoPage);
    auto* changePasswordButton = new QPushButton("修改密码", infoPage);
    auto* leaveMessageButton = new QPushButton("我要留言", infoPage);
    infoButtons->addWidget(editInfoButton);
    infoButtons->addWidget(changePasswordButton);
    infoButtons->addWidget(leaveMessageButton);
    infoButtons->addStretch();

    enterpriseSummaryLabel_ = new QLabel(infoPage);
    enterpriseSummaryLabel_->setWordWrap(true);
    infoLayout->addLayout(infoButtons);
    infoLayout->addWidget(enterpriseSummaryLabel_);

    auto* positionPage = new QWidget(this);
    auto* positionLayout = new QVBoxLayout(positionPage);
    auto* positionButtons = new QHBoxLayout();
    auto* addButton = new QPushButton("发布岗位", positionPage);
    auto* editButton = new QPushButton("修改岗位", positionPage);
    auto* removeButton = new QPushButton("删除岗位", positionPage);
    auto* refreshButton = new QPushButton("刷新岗位", positionPage);
    positionButtons->addWidget(addButton);
    positionButtons->addWidget(editButton);
    positionButtons->addWidget(removeButton);
    positionButtons->addWidget(refreshButton);
    positionButtons->addStretch();

    enterprisePositionsTable_ = new QTableWidget(positionPage);
    prepareTable(enterprisePositionsTable_,
                 {"岗位编号", "岗位名称", "地点", "薪资", "启用状态"});
    positionLayout->addLayout(positionButtons);
    positionLayout->addWidget(enterprisePositionsTable_);

    auto* talentPage = new QWidget(this);
    auto* talentLayout = new QVBoxLayout(talentPage);
    auto* talentTop = new QHBoxLayout();
    enterpriseTalentSearchEdit_ = new QLineEdit(talentPage);
    enterpriseTalentSearchEdit_->setPlaceholderText("输入姓名、学历、经验或技能关键字进行筛选");
    auto* refreshTalentButton = new QPushButton("刷新人才列表", talentPage);
    talentTop->addWidget(enterpriseTalentSearchEdit_);
    talentTop->addWidget(refreshTalentButton);

    enterpriseTalentsTable_ = new QTableWidget(talentPage);
    prepareTable(enterpriseTalentsTable_,
                 {"人才编号", "姓名", "学历", "经验", "技能"});
    talentLayout->addLayout(talentTop);
    talentLayout->addWidget(enterpriseTalentsTable_);

    connect(editInfoButton, &QPushButton::clicked, this, &DashboardWindow::handleEnterpriseEditInfo);
    connect(changePasswordButton, &QPushButton::clicked, this,
            &DashboardWindow::handleEnterpriseChangePassword);
    connect(leaveMessageButton, &QPushButton::clicked, this,
            &DashboardWindow::handleEnterpriseLeaveMessage);
    connect(addButton, &QPushButton::clicked, this, &DashboardWindow::handleEnterpriseAddPosition);
    connect(editButton, &QPushButton::clicked, this, &DashboardWindow::handleEnterpriseEditPosition);
    connect(removeButton, &QPushButton::clicked, this,
            &DashboardWindow::handleEnterpriseRemovePosition);
    connect(refreshButton, &QPushButton::clicked, this,
            [this]() { populateEnterpriseView(); });
    connect(refreshTalentButton, &QPushButton::clicked, this,
            [this]() { populateEnterpriseView(); });
    connect(enterpriseTalentSearchEdit_, &QLineEdit::textChanged, this,
            [this]() { populateEnterpriseView(); });

    tabWidget_->addTab(infoPage, "企业信息");
    tabWidget_->addTab(positionPage, "岗位管理");
    tabWidget_->addTab(talentPage, "人才查询");
}

void DashboardWindow::buildAdminTabs() {
    auto* pendingPersonalPage = new QWidget(this);
    auto* pendingPersonalLayout = new QVBoxLayout(pendingPersonalPage);
    auto* pendingPersonalButtons = new QHBoxLayout();
    auto* approvePersonalButton = new QPushButton("通过选中个人用户", pendingPersonalPage);
    auto* rejectPersonalButton = new QPushButton("驳回选中个人用户", pendingPersonalPage);
    pendingPersonalButtons->addWidget(approvePersonalButton);
    pendingPersonalButtons->addWidget(rejectPersonalButton);
    pendingPersonalButtons->addStretch();

    adminPendingPersonalTable_ = new QTableWidget(pendingPersonalPage);
    prepareTable(adminPendingPersonalTable_,
                 {"编号", "用户名", "姓名", "电话", "状态"});
    pendingPersonalLayout->addLayout(pendingPersonalButtons);
    pendingPersonalLayout->addWidget(adminPendingPersonalTable_);

    auto* pendingEnterprisePage = new QWidget(this);
    auto* pendingEnterpriseLayout = new QVBoxLayout(pendingEnterprisePage);
    auto* pendingEnterpriseButtons = new QHBoxLayout();
    auto* approveEnterpriseButton = new QPushButton("通过选中企业用户", pendingEnterprisePage);
    auto* rejectEnterpriseButton = new QPushButton("驳回选中企业用户", pendingEnterprisePage);
    pendingEnterpriseButtons->addWidget(approveEnterpriseButton);
    pendingEnterpriseButtons->addWidget(rejectEnterpriseButton);
    pendingEnterpriseButtons->addStretch();

    adminPendingEnterpriseTable_ = new QTableWidget(pendingEnterprisePage);
    prepareTable(adminPendingEnterpriseTable_,
                 {"编号", "用户名", "企业名称", "联系人", "状态"});
    pendingEnterpriseLayout->addLayout(pendingEnterpriseButtons);
    pendingEnterpriseLayout->addWidget(adminPendingEnterpriseTable_);

    auto* messagePage = new QWidget(this);
    auto* messageLayout = new QVBoxLayout(messagePage);
    auto* messageTop = new QHBoxLayout();
    adminMessageSearchEdit_ = new QLineEdit(messagePage);
    adminMessageSearchEdit_->setPlaceholderText("输入发送方、内容或回复关键字进行筛选");
    auto* replyButton = new QPushButton("回复选中留言", messagePage);
    auto* deleteButton = new QPushButton("删除选中留言", messagePage);
    auto* refreshButton = new QPushButton("刷新留言", messagePage);
    messageTop->addWidget(adminMessageSearchEdit_);
    messageTop->addWidget(replyButton);
    messageTop->addWidget(deleteButton);
    messageTop->addWidget(refreshButton);

    adminMessageTable_ = new QTableWidget(messagePage);
    prepareTable(adminMessageTable_,
                 {"留言编号", "发送方", "名称", "内容", "处理状态", "回复内容"});
    messageLayout->addLayout(messageTop);
    messageLayout->addWidget(adminMessageTable_);

    connect(approvePersonalButton, &QPushButton::clicked, this,
            &DashboardWindow::handleAdminApprovePersonal);
    connect(rejectPersonalButton, &QPushButton::clicked, this,
            &DashboardWindow::handleAdminRejectPersonal);
    connect(approveEnterpriseButton, &QPushButton::clicked, this,
            &DashboardWindow::handleAdminApproveEnterprise);
    connect(rejectEnterpriseButton, &QPushButton::clicked, this,
            &DashboardWindow::handleAdminRejectEnterprise);
    connect(replyButton, &QPushButton::clicked, this, &DashboardWindow::handleAdminReplyMessage);
    connect(deleteButton, &QPushButton::clicked, this, &DashboardWindow::handleAdminDeleteMessage);
    connect(refreshButton, &QPushButton::clicked, this, [this]() { populateAdminView(); });
    connect(adminMessageSearchEdit_, &QLineEdit::textChanged, this,
            [this]() { populateAdminView(); });

    tabWidget_->addTab(pendingPersonalPage, "待审个人用户");
    tabWidget_->addTab(pendingEnterprisePage, "待审企业用户");
    tabWidget_->addTab(messagePage, "留言管理");
}

void DashboardWindow::refreshData() {
    if (role_ == UserRole::Personal) {
        populatePersonalView();
    } else if (role_ == UserRole::Enterprise) {
        populateEnterpriseView();
    } else {
        populateAdminView();
    }
}

void DashboardWindow::populatePersonalView() {
    const PersonalUser* user = service_->personalById(currentUserId_);
    if (user == nullptr) {
        headerLabel_->setText("未找到当前个人用户。");
        return;
    }

    headerLabel_->setText(QString("欢迎，%1")
                              .arg(qstr(user->name.empty() ? user->username : user->name)));

    personalSummaryLabel_->setText(
        QString("用户名：%1\n姓名：%2\n性别：%3\n电话：%4\n邮箱：%5\n审核状态：%6")
            .arg(qstr(user->username))
            .arg(qstr(user->name))
            .arg(qstr(user->gender))
            .arg(qstr(user->phone))
            .arg(qstr(user->email))
            .arg(qstr(RecruitmentService::statusText(user->status))));

    personalResumeView_->setPlainText(
        QString("学历：%1\n\n工作经验：%2\n\n技能特长：%3\n\n自我介绍：%4")
            .arg(qstr(user->resume.education))
            .arg(qstr(user->resume.experience))
            .arg(qstr(user->resume.skills))
            .arg(qstr(user->resume.selfIntroduction)));

    const QString keyword = personalPositionSearchEdit_ == nullptr
                                ? QString()
                                : personalPositionSearchEdit_->text().trimmed();
    const auto visiblePositions = service_->visiblePositions();
    int row = 0;
    personalPositionsTable_->setRowCount(0);
    for (const Position* position : visiblePositions) {
        const EnterpriseUser* enterprise = service_->enterpriseById(position->enterpriseId);
        const QString companyName =
            enterprise == nullptr ? "未知企业" : qstr(enterprise->companyName);

        if (!matchesFilter(keyword, {qstr(position->title), companyName, qstr(position->location),
                                     qstr(position->requirement), qstr(position->description)})) {
            continue;
        }

        const bool applied =
            std::find(user->appliedPositionIds.begin(), user->appliedPositionIds.end(),
                      position->id) != user->appliedPositionIds.end();

        personalPositionsTable_->insertRow(row);
        personalPositionsTable_->setItem(row, 0,
                                         new QTableWidgetItem(QString::number(position->id)));
        personalPositionsTable_->setItem(row, 1, new QTableWidgetItem(companyName));
        personalPositionsTable_->setItem(row, 2, new QTableWidgetItem(qstr(position->title)));
        personalPositionsTable_->setItem(row, 3, new QTableWidgetItem(qstr(position->location)));
        personalPositionsTable_->setItem(row, 4, new QTableWidgetItem(qstr(position->salary)));
        personalPositionsTable_->setItem(row, 5,
                                         new QTableWidgetItem(applied ? "已申请" : "可投递"));
        ++row;
    }

    const auto appliedPositions = service_->appliedPositionsForPersonal(currentUserId_);
    personalAppliedTable_->setRowCount(0);
    row = 0;
    for (const Position* position : appliedPositions) {
        const EnterpriseUser* enterprise = service_->enterpriseById(position->enterpriseId);
        personalAppliedTable_->insertRow(row);
        personalAppliedTable_->setItem(row, 0,
                                       new QTableWidgetItem(QString::number(position->id)));
        personalAppliedTable_->setItem(
            row, 1,
            new QTableWidgetItem(enterprise == nullptr ? "未知企业" : qstr(enterprise->companyName)));
        personalAppliedTable_->setItem(row, 2, new QTableWidgetItem(qstr(position->title)));
        personalAppliedTable_->setItem(row, 3, new QTableWidgetItem(qstr(position->location)));
        personalAppliedTable_->setItem(row, 4, new QTableWidgetItem(qstr(position->salary)));
        ++row;
    }
}

void DashboardWindow::populateEnterpriseView() {
    const EnterpriseUser* user = service_->enterpriseById(currentUserId_);
    if (user == nullptr) {
        headerLabel_->setText("未找到当前企业用户。");
        return;
    }

    headerLabel_->setText(QString("欢迎，%1").arg(qstr(user->companyName)));
    enterpriseSummaryLabel_->setText(
        QString("企业名称：%1\n联系人：%2\n电话：%3\n邮箱：%4\n地址：%5\n审核状态：%6\n\n企业简介：%7")
            .arg(qstr(user->companyName))
            .arg(qstr(user->contactPerson))
            .arg(qstr(user->phone))
            .arg(qstr(user->email))
            .arg(qstr(user->address))
            .arg(qstr(RecruitmentService::statusText(user->status)))
            .arg(qstr(user->introduction)));

    const auto companyPositions = service_->positionsForEnterprise(currentUserId_);
    enterprisePositionsTable_->setRowCount(0);
    int row = 0;
    for (const Position* position : companyPositions) {
        enterprisePositionsTable_->insertRow(row);
        enterprisePositionsTable_->setItem(row, 0,
                                           new QTableWidgetItem(QString::number(position->id)));
        enterprisePositionsTable_->setItem(row, 1, new QTableWidgetItem(qstr(position->title)));
        enterprisePositionsTable_->setItem(row, 2, new QTableWidgetItem(qstr(position->location)));
        enterprisePositionsTable_->setItem(row, 3, new QTableWidgetItem(qstr(position->salary)));
        enterprisePositionsTable_->setItem(
            row, 4, new QTableWidgetItem(position->active ? "启用中" : "已停用"));
        ++row;
    }

    const QString keyword = enterpriseTalentSearchEdit_ == nullptr
                                ? QString()
                                : enterpriseTalentSearchEdit_->text().trimmed();
    const auto talents = service_->approvedTalents();
    enterpriseTalentsTable_->setRowCount(0);
    row = 0;
    for (const PersonalUser* talent : talents) {
        if (!matchesFilter(keyword, {qstr(talent->name), qstr(talent->resume.education),
                                     qstr(talent->resume.experience),
                                     qstr(talent->resume.skills),
                                     qstr(talent->resume.selfIntroduction)})) {
            continue;
        }

        enterpriseTalentsTable_->insertRow(row);
        enterpriseTalentsTable_->setItem(row, 0,
                                         new QTableWidgetItem(QString::number(talent->id)));
        enterpriseTalentsTable_->setItem(row, 1, new QTableWidgetItem(qstr(talent->name)));
        enterpriseTalentsTable_->setItem(row, 2,
                                         new QTableWidgetItem(qstr(talent->resume.education)));
        enterpriseTalentsTable_->setItem(row, 3,
                                         new QTableWidgetItem(qstr(talent->resume.experience)));
        enterpriseTalentsTable_->setItem(row, 4,
                                         new QTableWidgetItem(qstr(talent->resume.skills)));
        ++row;
    }
}

void DashboardWindow::populateAdminView() {
    headerLabel_->setText("管理员后台");

    const auto pendingPersonal = service_->pendingPersonalUsers();
    adminPendingPersonalTable_->setRowCount(0);
    int row = 0;
    for (const PersonalUser* user : pendingPersonal) {
        adminPendingPersonalTable_->insertRow(row);
        adminPendingPersonalTable_->setItem(row, 0,
                                            new QTableWidgetItem(QString::number(user->id)));
        adminPendingPersonalTable_->setItem(row, 1, new QTableWidgetItem(qstr(user->username)));
        adminPendingPersonalTable_->setItem(row, 2, new QTableWidgetItem(qstr(user->name)));
        adminPendingPersonalTable_->setItem(row, 3, new QTableWidgetItem(qstr(user->phone)));
        adminPendingPersonalTable_->setItem(
            row, 4, new QTableWidgetItem(qstr(RecruitmentService::statusText(user->status))));
        ++row;
    }

    const auto pendingEnterprise = service_->pendingEnterpriseUsers();
    adminPendingEnterpriseTable_->setRowCount(0);
    row = 0;
    for (const EnterpriseUser* user : pendingEnterprise) {
        adminPendingEnterpriseTable_->insertRow(row);
        adminPendingEnterpriseTable_->setItem(row, 0,
                                              new QTableWidgetItem(QString::number(user->id)));
        adminPendingEnterpriseTable_->setItem(row, 1,
                                              new QTableWidgetItem(qstr(user->username)));
        adminPendingEnterpriseTable_->setItem(row, 2,
                                              new QTableWidgetItem(qstr(user->companyName)));
        adminPendingEnterpriseTable_->setItem(row, 3,
                                              new QTableWidgetItem(qstr(user->contactPerson)));
        adminPendingEnterpriseTable_->setItem(
            row, 4, new QTableWidgetItem(qstr(RecruitmentService::statusText(user->status))));
        ++row;
    }

    const QString keyword = adminMessageSearchEdit_ == nullptr
                                ? QString()
                                : adminMessageSearchEdit_->text().trimmed();
    const auto& allMessages = service_->messages();
    adminMessageTable_->setRowCount(0);
    row = 0;
    for (const Message& message : allMessages) {
        if (!matchesFilter(keyword, {qstr(message.senderType), qstr(message.senderName),
                                     qstr(message.content), qstr(message.reply)})) {
            continue;
        }

        adminMessageTable_->insertRow(row);
        adminMessageTable_->setItem(row, 0,
                                    new QTableWidgetItem(QString::number(message.id)));
        adminMessageTable_->setItem(row, 1, new QTableWidgetItem(qstr(message.senderType)));
        adminMessageTable_->setItem(row, 2, new QTableWidgetItem(qstr(message.senderName)));
        adminMessageTable_->setItem(row, 3, new QTableWidgetItem(qstr(message.content)));
        adminMessageTable_->setItem(row, 4,
                                    new QTableWidgetItem(message.handled ? "已处理" : "未处理"));
        adminMessageTable_->setItem(row, 5, new QTableWidgetItem(qstr(message.reply)));
        ++row;
    }
}

int DashboardWindow::selectedIdFromTable(QTableWidget* table, int column) const {
    if (table == nullptr) {
        return -1;
    }

    const int row = table->currentRow();
    if (row < 0) {
        return -1;
    }

    QTableWidgetItem* item = table->item(row, column);
    if (item == nullptr) {
        return -1;
    }

    bool ok = false;
    const int value = item->text().toInt(&ok);
    return ok ? value : -1;
}

void DashboardWindow::handlePersonalEditInfo() {
    const PersonalUser* user = service_->personalById(currentUserId_);
    if (user == nullptr) {
        QMessageBox::warning(this, "操作失败", "未找到当前个人用户。");
        return;
    }

    QString name;
    QString gender;
    QString phone;
    QString email;
    if (!promptPersonalInfoDialog(this, *user, name, gender, phone, email)) {
        return;
    }

    std::string error;
    if (!service_->updatePersonalInfo(currentUserId_, stdstr(name), stdstr(gender),
                                      stdstr(phone), stdstr(email), error)) {
        QMessageBox::warning(this, "保存失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "个人信息已更新。");
}

void DashboardWindow::handlePersonalEditResume() {
    const PersonalUser* user = service_->personalById(currentUserId_);
    if (user == nullptr) {
        QMessageBox::warning(this, "操作失败", "未找到当前个人用户。");
        return;
    }

    Resume resume;
    if (!promptResumeDialog(this, user->resume, resume)) {
        return;
    }

    std::string error;
    if (!service_->updatePersonalResume(currentUserId_, resume, error)) {
        QMessageBox::warning(this, "保存失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "简历已更新。");
}

void DashboardWindow::handlePersonalApplyPosition() {
    const int positionId = selectedIdFromTable(personalPositionsTable_);
    if (positionId < 0) {
        QMessageBox::warning(this, "请选择岗位", "请先在岗位列表中选中一条岗位记录。");
        return;
    }

    std::string error;
    if (!service_->applyForPosition(currentUserId_, positionId, error)) {
        QMessageBox::warning(this, "申请失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "岗位申请已提交。");
}

void DashboardWindow::handlePersonalWithdrawPosition() {
    const int positionId = selectedIdFromTable(personalAppliedTable_);
    if (positionId < 0) {
        QMessageBox::warning(this, "请选择申请记录", "请先在已申请岗位表中选中一条记录。");
        return;
    }

    if (QMessageBox::question(this, "确认撤销", "确定要撤销选中的岗位申请吗？") !=
        QMessageBox::Yes) {
        return;
    }

    std::string error;
    if (!service_->withdrawApplication(currentUserId_, positionId, error)) {
        QMessageBox::warning(this, "撤销失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "岗位申请已撤销。");
}

void DashboardWindow::handlePersonalChangePassword() {
    QString oldPassword;
    QString newPassword;
    if (!promptPasswordDialog(this, "修改个人密码", oldPassword, newPassword)) {
        return;
    }

    std::string error;
    if (!service_->changePersonalPassword(currentUserId_, stdstr(oldPassword),
                                          stdstr(newPassword), error)) {
        QMessageBox::warning(this, "修改失败", qstr(error));
        return;
    }

    QMessageBox::information(this, "操作成功", "密码修改成功。");
}

void DashboardWindow::handlePersonalLeaveMessage() {
    const PersonalUser* user = service_->personalById(currentUserId_);
    if (user == nullptr) {
        QMessageBox::warning(this, "操作失败", "未找到当前个人用户。");
        return;
    }

    QString content;
    if (!promptTextDialog(this, "我要留言", "请输入留言内容：", content)) {
        return;
    }

    const std::string senderName = user->name.empty() ? user->username : user->name;
    std::string error;
    if (!service_->addMessage("个人用户", senderName, stdstr(content), error)) {
        QMessageBox::warning(this, "提交失败", qstr(error));
        return;
    }

    QMessageBox::information(this, "操作成功", "留言提交成功。");
}

void DashboardWindow::handleEnterpriseEditInfo() {
    const EnterpriseUser* user = service_->enterpriseById(currentUserId_);
    if (user == nullptr) {
        QMessageBox::warning(this, "操作失败", "未找到当前企业用户。");
        return;
    }

    QString companyName;
    QString contactPerson;
    QString phone;
    QString email;
    QString address;
    QString introduction;
    if (!promptEnterpriseInfoDialog(this, *user, companyName, contactPerson, phone, email,
                                    address, introduction)) {
        return;
    }

    std::string error;
    if (!service_->updateEnterpriseInfo(currentUserId_, stdstr(companyName),
                                        stdstr(contactPerson), stdstr(phone), stdstr(email),
                                        stdstr(address), stdstr(introduction), error)) {
        QMessageBox::warning(this, "保存失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "企业信息已更新。");
}

void DashboardWindow::handleEnterpriseAddPosition() {
    QString title;
    QString location;
    QString salary;
    QString requirement;
    QString description;
    if (!promptPositionDialog(this, "发布岗位", nullptr, title, location, salary, requirement,
                              description)) {
        return;
    }

    std::string error;
    if (!service_->addPosition(currentUserId_, stdstr(title), stdstr(location), stdstr(salary),
                               stdstr(requirement), stdstr(description), error)) {
        QMessageBox::warning(this, "发布失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "岗位发布成功。");
}

void DashboardWindow::handleEnterpriseEditPosition() {
    const int positionId = selectedIdFromTable(enterprisePositionsTable_);
    if (positionId < 0) {
        QMessageBox::warning(this, "请选择岗位", "请先在岗位表中选中一条岗位记录。");
        return;
    }

    const Position* position = service_->positionById(positionId);
    if (position == nullptr) {
        QMessageBox::warning(this, "操作失败", "未找到该岗位。");
        return;
    }

    QString title;
    QString location;
    QString salary;
    QString requirement;
    QString description;
    if (!promptPositionDialog(this, "修改岗位", position, title, location, salary, requirement,
                              description)) {
        return;
    }

    std::string error;
    if (!service_->updatePosition(currentUserId_, positionId, stdstr(title), stdstr(location),
                                  stdstr(salary), stdstr(requirement), stdstr(description),
                                  error)) {
        QMessageBox::warning(this, "修改失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "岗位信息已更新。");
}

void DashboardWindow::handleEnterpriseRemovePosition() {
    const int positionId = selectedIdFromTable(enterprisePositionsTable_);
    if (positionId < 0) {
        QMessageBox::warning(this, "请选择岗位", "请先在岗位表中选中一条岗位记录。");
        return;
    }

    if (QMessageBox::question(this, "确认删除", "确定要删除选中的岗位吗？") !=
        QMessageBox::Yes) {
        return;
    }

    std::string error;
    if (!service_->removePosition(currentUserId_, positionId, error)) {
        QMessageBox::warning(this, "删除失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "岗位已删除。");
}

void DashboardWindow::handleEnterpriseChangePassword() {
    QString oldPassword;
    QString newPassword;
    if (!promptPasswordDialog(this, "修改企业密码", oldPassword, newPassword)) {
        return;
    }

    std::string error;
    if (!service_->changeEnterprisePassword(currentUserId_, stdstr(oldPassword),
                                            stdstr(newPassword), error)) {
        QMessageBox::warning(this, "修改失败", qstr(error));
        return;
    }

    QMessageBox::information(this, "操作成功", "密码修改成功。");
}

void DashboardWindow::handleEnterpriseLeaveMessage() {
    const EnterpriseUser* user = service_->enterpriseById(currentUserId_);
    if (user == nullptr) {
        QMessageBox::warning(this, "操作失败", "未找到当前企业用户。");
        return;
    }

    QString content;
    if (!promptTextDialog(this, "我要留言", "请输入留言内容：", content)) {
        return;
    }

    const std::string senderName =
        user->companyName.empty() ? user->username : user->companyName;
    std::string error;
    if (!service_->addMessage("企业用户", senderName, stdstr(content), error)) {
        QMessageBox::warning(this, "提交失败", qstr(error));
        return;
    }

    QMessageBox::information(this, "操作成功", "留言提交成功。");
}

void DashboardWindow::handleAdminApprovePersonal() {
    const int userId = selectedIdFromTable(adminPendingPersonalTable_);
    if (userId < 0) {
        QMessageBox::warning(this, "请选择用户", "请先选中一个待审个人用户。");
        return;
    }

    std::string error;
    if (!service_->reviewPersonalUser(userId, AuditStatus::Approved, error)) {
        QMessageBox::warning(this, "审核失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "个人用户已审核通过。");
}

void DashboardWindow::handleAdminRejectPersonal() {
    const int userId = selectedIdFromTable(adminPendingPersonalTable_);
    if (userId < 0) {
        QMessageBox::warning(this, "请选择用户", "请先选中一个待审个人用户。");
        return;
    }

    std::string error;
    if (!service_->reviewPersonalUser(userId, AuditStatus::Rejected, error)) {
        QMessageBox::warning(this, "审核失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "个人用户已驳回。");
}

void DashboardWindow::handleAdminApproveEnterprise() {
    const int userId = selectedIdFromTable(adminPendingEnterpriseTable_);
    if (userId < 0) {
        QMessageBox::warning(this, "请选择企业用户", "请先选中一个待审企业用户。");
        return;
    }

    std::string error;
    if (!service_->reviewEnterpriseUser(userId, AuditStatus::Approved, error)) {
        QMessageBox::warning(this, "审核失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "企业用户已审核通过。");
}

void DashboardWindow::handleAdminRejectEnterprise() {
    const int userId = selectedIdFromTable(adminPendingEnterpriseTable_);
    if (userId < 0) {
        QMessageBox::warning(this, "请选择企业用户", "请先选中一个待审企业用户。");
        return;
    }

    std::string error;
    if (!service_->reviewEnterpriseUser(userId, AuditStatus::Rejected, error)) {
        QMessageBox::warning(this, "审核失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "企业用户已驳回。");
}

void DashboardWindow::handleAdminReplyMessage() {
    const int messageId = selectedIdFromTable(adminMessageTable_);
    if (messageId < 0) {
        QMessageBox::warning(this, "请选择留言", "请先选中一条留言。");
        return;
    }

    QString reply;
    if (!promptTextDialog(this, "回复留言", "请输入回复内容：", reply)) {
        return;
    }

    std::string error;
    if (!service_->replyMessage(messageId, stdstr(reply), error)) {
        QMessageBox::warning(this, "回复失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "留言已回复。");
}

void DashboardWindow::handleAdminDeleteMessage() {
    const int messageId = selectedIdFromTable(adminMessageTable_);
    if (messageId < 0) {
        QMessageBox::warning(this, "请选择留言", "请先选中一条留言。");
        return;
    }

    if (QMessageBox::question(this, "确认删除", "确定要删除选中的留言吗？") !=
        QMessageBox::Yes) {
        return;
    }

    std::string error;
    if (!service_->deleteMessage(messageId, error)) {
        QMessageBox::warning(this, "删除失败", qstr(error));
        return;
    }

    refreshData();
    QMessageBox::information(this, "操作成功", "留言已删除。");
}

void DashboardWindow::handleAdminChangePassword() {
    QString oldPassword;
    QString newPassword;
    if (!promptPasswordDialog(this, "修改管理员密码", oldPassword, newPassword)) {
        return;
    }

    std::string error;
    if (!service_->changeAdminPassword(stdstr(oldPassword), stdstr(newPassword), error)) {
        QMessageBox::warning(this, "修改失败", qstr(error));
        return;
    }

    QMessageBox::information(this, "操作成功", "管理员密码修改成功。");
}
