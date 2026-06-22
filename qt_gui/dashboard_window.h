#pragma once

#include <memory>

#include <QMainWindow>

#include "recruitment_service.h"

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTabWidget;
class QTableWidget;

class DashboardWindow : public QMainWindow {
    Q_OBJECT

public:
    DashboardWindow(std::shared_ptr<RecruitmentService> service,
                    UserRole role,
                    int currentUserId,
                    QWidget* parent = nullptr);

private:
    void buildUi();
    void buildPersonalTabs();
    void buildEnterpriseTabs();
    void buildAdminTabs();
    void refreshData();
    void populatePersonalView();
    void populateEnterpriseView();
    void populateAdminView();
    int selectedIdFromTable(QTableWidget* table, int column = 0) const;

    void handlePersonalEditInfo();
    void handlePersonalEditResume();
    void handlePersonalApplyPosition();
    void handlePersonalWithdrawPosition();
    void handlePersonalChangePassword();
    void handlePersonalLeaveMessage();

    void handleEnterpriseEditInfo();
    void handleEnterpriseAddPosition();
    void handleEnterpriseEditPosition();
    void handleEnterpriseRemovePosition();
    void handleEnterpriseChangePassword();
    void handleEnterpriseLeaveMessage();

    void handleAdminApprovePersonal();
    void handleAdminRejectPersonal();
    void handleAdminApproveEnterprise();
    void handleAdminRejectEnterprise();
    void handleAdminReplyMessage();
    void handleAdminDeleteMessage();
    void handleAdminChangePassword();

    std::shared_ptr<RecruitmentService> service_;
    UserRole role_;
    int currentUserId_;

    QLabel* headerLabel_ = nullptr;
    QLabel* hintLabel_ = nullptr;
    QTabWidget* tabWidget_ = nullptr;

    QLabel* personalSummaryLabel_ = nullptr;
    QPlainTextEdit* personalResumeView_ = nullptr;
    QLineEdit* personalPositionSearchEdit_ = nullptr;
    QTableWidget* personalPositionsTable_ = nullptr;
    QTableWidget* personalAppliedTable_ = nullptr;

    QLabel* enterpriseSummaryLabel_ = nullptr;
    QTableWidget* enterprisePositionsTable_ = nullptr;
    QLineEdit* enterpriseTalentSearchEdit_ = nullptr;
    QTableWidget* enterpriseTalentsTable_ = nullptr;

    QTableWidget* adminPendingPersonalTable_ = nullptr;
    QTableWidget* adminPendingEnterpriseTable_ = nullptr;
    QLineEdit* adminMessageSearchEdit_ = nullptr;
    QTableWidget* adminMessageTable_ = nullptr;
};
