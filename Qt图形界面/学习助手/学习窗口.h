#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <QString>
#include <QWidget>

#include "OpenAI客户端.h"
#include "学习服务.h"

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTextEdit;

class StudyAssistantWindow : public QWidget {
public:
    explicit StudyAssistantWindow(std::shared_ptr<StudyService> service, QWidget* parent = nullptr);

private:
    void buildUi();
    void wireUi();
    void applyWindowStyle();

    StudyFilter currentFilter() const;
    std::optional<int> selectedQuestionId() const;
    std::optional<QuestionProgress> selectedProgress() const;

    void refreshFilters();
    void refreshQuestions();
    void selectQuestionById(int questionId);
    void updateDetails();
    void setStatusMessage(const QString& message);

    void addQuestion();
    void editCurrentQuestion();
    void deleteCurrentQuestion();
    void markCurrentQuestion(bool correct);
    void recommendQuestion();
    void exportData();
    void importData();
    void requestAiAnalysis();
    void refreshAiPanel();

    std::shared_ptr<StudyService> service_;
    std::unique_ptr<OpenAiClient> openAiClient_;
    std::vector<QuestionProgress> currentRows_;
    int aiAnalysisQuestionId_ = 0;
    bool aiRequestInFlight_ = false;
    QString aiAnalysisText_;

    QComboBox* subjectCombo_ = nullptr;
    QComboBox* topicCombo_ = nullptr;
    QLineEdit* tagFilterEdit_ = nullptr;
    QSpinBox* minDifficultySpin_ = nullptr;
    QSpinBox* maxDifficultySpin_ = nullptr;
    QTableWidget* questionTable_ = nullptr;
    QLabel* summaryLabel_ = nullptr;
    QLabel* statsLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* imageMetaLabel_ = nullptr;
    QLabel* imagePreviewLabel_ = nullptr;
    QTextEdit* promptView_ = nullptr;
    QTextEdit* answerView_ = nullptr;
    QTextEdit* explanationView_ = nullptr;
    QTextEdit* aiAnalysisView_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QPushButton* markCorrectButton_ = nullptr;
    QPushButton* markWrongButton_ = nullptr;
    QPushButton* aiExplainButton_ = nullptr;
};
