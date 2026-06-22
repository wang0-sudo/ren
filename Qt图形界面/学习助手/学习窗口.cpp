#include "学习窗口.h"

#include "OpenAI客户端.h"

#include <algorithm>

#include <QByteArray>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {
QString toQString(const std::string& value) {
    return QString::fromStdString(value);
}

QString joinTags(const std::vector<std::string>& tags) {
    QStringList parts;
    for (const auto& tag : tags) {
        parts << toQString(tag);
    }
    return parts.join(", ");
}

QByteArray toByteArray(const std::vector<unsigned char>& data) {
    if (data.empty()) {
        return {};
    }
    return QByteArray(reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));
}

std::vector<unsigned char> toByteVector(const QByteArray& data) {
    return std::vector<unsigned char>(data.begin(), data.end());
}

class QuestionDialog : public QDialog {
public:
    explicit QuestionDialog(QWidget* parent = nullptr)
        : QDialog(parent) {
        setWindowTitle("Question Editor");
        resize(620, 640);

        auto* layout = new QVBoxLayout(this);
        auto* form = new QFormLayout();

        subjectEdit_ = new QLineEdit(this);
        topicEdit_ = new QLineEdit(this);
        tagsEdit_ = new QLineEdit(this);
        imageInfoEdit_ = new QLineEdit(this);
        imageInfoEdit_->setReadOnly(true);
        imageInfoEdit_->setPlaceholderText("No image selected");
        difficultySpin_ = new QSpinBox(this);
        difficultySpin_->setRange(1, 5);
        difficultySpin_->setValue(3);

        promptEdit_ = new QTextEdit(this);
        answerEdit_ = new QTextEdit(this);
        explanationEdit_ = new QTextEdit(this);
        promptEdit_->setMinimumHeight(90);
        answerEdit_->setMinimumHeight(70);
        explanationEdit_->setMinimumHeight(90);
        imagePreviewLabel_ = new QLabel(this);
        imagePreviewLabel_->setMinimumHeight(140);
        imagePreviewLabel_->setAlignment(Qt::AlignCenter);
        imagePreviewLabel_->setStyleSheet("border: 1px dashed #ccb9a5; border-radius: 8px; background: #fffdf9;");
        imagePreviewLabel_->setText("No image selected");

        auto* imageRow = new QHBoxLayout();
        auto* browseButton = new QPushButton("Browse", this);
        auto* clearButton = new QPushButton("Clear", this);
        imageRow->addWidget(imageInfoEdit_, 1);
        imageRow->addWidget(browseButton);
        imageRow->addWidget(clearButton);

        form->addRow("Subject", subjectEdit_);
        form->addRow("Topic", topicEdit_);
        form->addRow("Tags", tagsEdit_);
        form->addRow("Image", imageRow);
        form->addRow("Difficulty", difficultySpin_);
        form->addRow("Prompt", promptEdit_);
        form->addRow("Answer", answerEdit_);
        form->addRow("Explanation", explanationEdit_);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QObject::connect(browseButton, &QPushButton::clicked, this, [this]() {
            const QString path = QFileDialog::getOpenFileName(this,
                                                              "Select Question Image",
                                                              QString(),
                                                              "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
            if (!path.isEmpty()) {
                QFile file(path);
                if (!file.open(QIODevice::ReadOnly)) {
                    QMessageBox::warning(this, "Question Image", "Failed to open the selected image file.");
                    return;
                }

                const QByteArray bytes = file.readAll();
                QPixmap pixmap;
                if (!pixmap.loadFromData(bytes)) {
                    QMessageBox::warning(this, "Question Image", "The selected file is not a supported image.");
                    return;
                }

                imageData_ = bytes;
                imageName_ = QFileInfo(path).fileName();
                imageInfoEdit_->setText(imageName_);
                updatePreview();
            }
        });
        QObject::connect(clearButton, &QPushButton::clicked, this, [this]() {
            imageData_.clear();
            imageName_.clear();
            imageInfoEdit_->clear();
            updatePreview();
        });

        layout->addLayout(form);
        layout->addWidget(imagePreviewLabel_);
        layout->addWidget(buttons);
    }

    void setQuestion(const Question& question) {
        subjectEdit_->setText(toQString(question.subject));
        topicEdit_->setText(toQString(question.topic));
        tagsEdit_->setText(joinTags(question.tags));
        imageName_ = toQString(question.imageName);
        imageData_ = toByteArray(question.imageData);
        imageInfoEdit_->setText(imageName_);
        difficultySpin_->setValue(question.difficulty);
        promptEdit_->setPlainText(toQString(question.prompt));
        answerEdit_->setPlainText(toQString(question.answer));
        explanationEdit_->setPlainText(toQString(question.explanation));
        updatePreview();
    }

    QString subject() const { return subjectEdit_->text().trimmed(); }
    QString topic() const { return topicEdit_->text().trimmed(); }
    QString tags() const { return tagsEdit_->text().trimmed(); }
    QString imageName() const { return imageName_.trimmed(); }
    std::vector<unsigned char> imageData() const { return toByteVector(imageData_); }
    int difficulty() const { return difficultySpin_->value(); }
    QString prompt() const { return promptEdit_->toPlainText().trimmed(); }
    QString answer() const { return answerEdit_->toPlainText().trimmed(); }
    QString explanation() const { return explanationEdit_->toPlainText().trimmed(); }

private:
    void updatePreview() {
        if (imageData_.isEmpty()) {
            imagePreviewLabel_->setPixmap(QPixmap());
            imagePreviewLabel_->setText(imageName_.trimmed().isEmpty() ? "No image selected"
                                                                       : "Image metadata loaded, but bytes are unavailable");
            return;
        }

        QPixmap pixmap;
        if (!pixmap.loadFromData(imageData_)) {
            imagePreviewLabel_->setPixmap(QPixmap());
            imagePreviewLabel_->setText("Unable to load image");
            return;
        }

        imagePreviewLabel_->setText(QString());
        imagePreviewLabel_->setPixmap(
            pixmap.scaled(420, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    QLineEdit* subjectEdit_ = nullptr;
    QLineEdit* topicEdit_ = nullptr;
    QLineEdit* tagsEdit_ = nullptr;
    QLineEdit* imageInfoEdit_ = nullptr;
    QSpinBox* difficultySpin_ = nullptr;
    QTextEdit* promptEdit_ = nullptr;
    QTextEdit* answerEdit_ = nullptr;
    QTextEdit* explanationEdit_ = nullptr;
    QLabel* imagePreviewLabel_ = nullptr;
    QString imageName_;
    QByteArray imageData_;
};
}  // namespace

StudyAssistantWindow::StudyAssistantWindow(std::shared_ptr<StudyService> service, QWidget* parent)
    : QWidget(parent), service_(std::move(service)), openAiClient_(std::make_unique<OpenAiClient>()) {
    buildUi();
    wireUi();
    applyWindowStyle();
    refreshFilters();
    refreshQuestions();
}

void StudyAssistantWindow::buildUi() {
    setWindowTitle("Study Assistant");
    resize(1280, 780);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(16);

    auto* titleLabel = new QLabel("Study Assistant", this);
    titleLabel->setObjectName("titleLabel");
    auto* subtitleLabel = new QLabel("SQLite-backed question bank with review tracking and quick practice tools.", this);
    subtitleLabel->setObjectName("subtitleLabel");

    auto* headerLayout = new QVBoxLayout();
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);
    root->addLayout(headerLayout);

    auto* filterFrame = new QFrame(this);
    filterFrame->setObjectName("panelFrame");
    auto* filterLayout = new QGridLayout(filterFrame);
    filterLayout->setContentsMargins(16, 16, 16, 16);
    filterLayout->setHorizontalSpacing(12);
    filterLayout->setVerticalSpacing(10);

    subjectCombo_ = new QComboBox(filterFrame);
    topicCombo_ = new QComboBox(filterFrame);
    tagFilterEdit_ = new QLineEdit(filterFrame);
    tagFilterEdit_->setPlaceholderText("Comma-separated tags");
    minDifficultySpin_ = new QSpinBox(filterFrame);
    minDifficultySpin_->setRange(1, 5);
    minDifficultySpin_->setValue(1);
    maxDifficultySpin_ = new QSpinBox(filterFrame);
    maxDifficultySpin_->setRange(1, 5);
    maxDifficultySpin_->setValue(5);

    auto* applyFilterButton = new QPushButton("Apply Filter", filterFrame);
    auto* resetFilterButton = new QPushButton("Reset", filterFrame);

    filterLayout->addWidget(new QLabel("Subject", filterFrame), 0, 0);
    filterLayout->addWidget(subjectCombo_, 0, 1);
    filterLayout->addWidget(new QLabel("Topic", filterFrame), 0, 2);
    filterLayout->addWidget(topicCombo_, 0, 3);
    filterLayout->addWidget(new QLabel("Tags", filterFrame), 0, 4);
    filterLayout->addWidget(tagFilterEdit_, 0, 5);
    filterLayout->addWidget(new QLabel("Min Diff", filterFrame), 1, 0);
    filterLayout->addWidget(minDifficultySpin_, 1, 1);
    filterLayout->addWidget(new QLabel("Max Diff", filterFrame), 1, 2);
    filterLayout->addWidget(maxDifficultySpin_, 1, 3);
    filterLayout->addWidget(applyFilterButton, 1, 4);
    filterLayout->addWidget(resetFilterButton, 1, 5);
    root->addWidget(filterFrame);

    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    auto* leftFrame = new QFrame(this);
    leftFrame->setObjectName("panelFrame");
    auto* leftLayout = new QVBoxLayout(leftFrame);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(12);

    summaryLabel_ = new QLabel(leftFrame);
    summaryLabel_->setObjectName("summaryLabel");
    questionTable_ = new QTableWidget(leftFrame);
    questionTable_->setColumnCount(8);
    questionTable_->setHorizontalHeaderLabels(
        {"ID", "Subject", "Topic", "Diff", "Image", "Pending", "Reviews", "Tags"});
    questionTable_->horizontalHeader()->setStretchLastSection(true);
    questionTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    questionTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    questionTable_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    questionTable_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    questionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    questionTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    questionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    questionTable_->setAlternatingRowColors(true);

    auto* actionRow = new QHBoxLayout();
    auto* addButton = new QPushButton("Add", leftFrame);
    editButton_ = new QPushButton("Edit", leftFrame);
    deleteButton_ = new QPushButton("Delete", leftFrame);
    auto* refreshButton = new QPushButton("Refresh", leftFrame);
    auto* recommendButton = new QPushButton("Recommend", leftFrame);
    actionRow->addWidget(addButton);
    actionRow->addWidget(editButton_);
    actionRow->addWidget(deleteButton_);
    actionRow->addWidget(refreshButton);
    actionRow->addWidget(recommendButton);
    actionRow->addStretch(1);

    leftLayout->addWidget(summaryLabel_);
    leftLayout->addWidget(questionTable_, 1);
    leftLayout->addLayout(actionRow);

    auto* rightFrame = new QFrame(this);
    rightFrame->setObjectName("panelFrame");
    auto* rightLayout = new QVBoxLayout(rightFrame);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(12);

    statsLabel_ = new QLabel(rightFrame);
    statsLabel_->setWordWrap(true);
    statsLabel_->setObjectName("statsLabel");

    auto* promptGroup = new QGroupBox("Prompt", rightFrame);
    auto* promptLayout = new QVBoxLayout(promptGroup);
    promptView_ = new QTextEdit(promptGroup);
    promptView_->setReadOnly(true);
    promptLayout->addWidget(promptView_);

    auto* answerGroup = new QGroupBox("Answer", rightFrame);
    auto* answerLayout = new QVBoxLayout(answerGroup);
    answerView_ = new QTextEdit(answerGroup);
    answerView_->setReadOnly(true);
    answerLayout->addWidget(answerView_);

    auto* explanationGroup = new QGroupBox("Explanation", rightFrame);
    auto* explanationLayout = new QVBoxLayout(explanationGroup);
    explanationView_ = new QTextEdit(explanationGroup);
    explanationView_->setReadOnly(true);
    explanationLayout->addWidget(explanationView_);

    auto* aiGroup = new QGroupBox("AI Coach", rightFrame);
    auto* aiLayout = new QVBoxLayout(aiGroup);
    aiAnalysisView_ = new QTextEdit(aiGroup);
    aiAnalysisView_->setReadOnly(true);
    aiAnalysisView_->setPlaceholderText("Select a question and click AI Explain.");
    aiLayout->addWidget(aiAnalysisView_);

    auto* imageGroup = new QGroupBox("Image", rightFrame);
    auto* imageLayout = new QVBoxLayout(imageGroup);
    imageMetaLabel_ = new QLabel(imageGroup);
    imageMetaLabel_->setWordWrap(true);
    imagePreviewLabel_ = new QLabel(imageGroup);
    imagePreviewLabel_->setMinimumHeight(190);
    imagePreviewLabel_->setAlignment(Qt::AlignCenter);
    imagePreviewLabel_->setStyleSheet("border: 1px dashed #ccb9a5; border-radius: 10px; background: #fffdf9;");
    imageLayout->addWidget(imageMetaLabel_);
    imageLayout->addWidget(imagePreviewLabel_);

    auto* reviewRow = new QHBoxLayout();
    aiExplainButton_ = new QPushButton("AI Explain", rightFrame);
    markCorrectButton_ = new QPushButton("Mark Correct", rightFrame);
    markWrongButton_ = new QPushButton("Mark Wrong", rightFrame);
    auto* exportButton = new QPushButton("Export", rightFrame);
    auto* importButton = new QPushButton("Import", rightFrame);
    reviewRow->addWidget(aiExplainButton_);
    reviewRow->addWidget(markCorrectButton_);
    reviewRow->addWidget(markWrongButton_);
    reviewRow->addStretch(1);
    reviewRow->addWidget(exportButton);
    reviewRow->addWidget(importButton);

    statusLabel_ = new QLabel(rightFrame);
    statusLabel_->setObjectName("statusLabel");
    statusLabel_->setWordWrap(true);

    rightLayout->addWidget(statsLabel_);
    rightLayout->addWidget(promptGroup, 2);
    rightLayout->addWidget(imageGroup, 2);
    rightLayout->addWidget(answerGroup, 1);
    rightLayout->addWidget(explanationGroup, 2);
    rightLayout->addWidget(aiGroup, 2);
    rightLayout->addLayout(reviewRow);
    rightLayout->addWidget(statusLabel_);

    contentLayout->addWidget(leftFrame, 3);
    contentLayout->addWidget(rightFrame, 2);
    root->addLayout(contentLayout, 1);

    QObject::connect(applyFilterButton, &QPushButton::clicked, this, [this]() { refreshQuestions(); });
    QObject::connect(resetFilterButton, &QPushButton::clicked, this, [this]() {
        subjectCombo_->setCurrentIndex(0);
        topicCombo_->setCurrentIndex(0);
        tagFilterEdit_->clear();
        minDifficultySpin_->setValue(1);
        maxDifficultySpin_->setValue(5);
        refreshQuestions();
    });
    QObject::connect(addButton, &QPushButton::clicked, this, [this]() { addQuestion(); });
    QObject::connect(editButton_, &QPushButton::clicked, this, [this]() { editCurrentQuestion(); });
    QObject::connect(deleteButton_, &QPushButton::clicked, this, [this]() { deleteCurrentQuestion(); });
    QObject::connect(refreshButton, &QPushButton::clicked, this, [this]() { refreshQuestions(); });
    QObject::connect(recommendButton, &QPushButton::clicked, this, [this]() { recommendQuestion(); });
    QObject::connect(aiExplainButton_, &QPushButton::clicked, this, [this]() { requestAiAnalysis(); });
    QObject::connect(markCorrectButton_, &QPushButton::clicked, this, [this]() { markCurrentQuestion(true); });
    QObject::connect(markWrongButton_, &QPushButton::clicked, this, [this]() { markCurrentQuestion(false); });
    QObject::connect(exportButton, &QPushButton::clicked, this, [this]() { exportData(); });
    QObject::connect(importButton, &QPushButton::clicked, this, [this]() { importData(); });
}

void StudyAssistantWindow::wireUi() {
    QObject::connect(questionTable_, &QTableWidget::itemSelectionChanged, this, [this]() { updateDetails(); });
}

void StudyAssistantWindow::applyWindowStyle() {
    setStyleSheet(
        "QWidget {"
        "  background: #f4efe7;"
        "  color: #2d241f;"
        "  font-family: 'Segoe UI';"
        "  font-size: 13px;"
        "}"
        "#titleLabel {"
        "  font-size: 28px;"
        "  font-weight: 700;"
        "  color: #1f4037;"
        "}"
        "#subtitleLabel {"
        "  color: #6b5b53;"
        "  font-size: 14px;"
        "}"
        "#panelFrame {"
        "  background: #fffaf3;"
        "  border: 1px solid #d7c6b4;"
        "  border-radius: 16px;"
        "}"
        "#summaryLabel, #statsLabel {"
        "  color: #4b3e36;"
        "  font-size: 13px;"
        "}"
        "#statusLabel {"
        "  color: #8c3d2b;"
        "  font-weight: 600;"
        "}"
        "QPushButton {"
        "  background: #1f6f5f;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 8px 14px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "  background: #18594d;"
        "}"
        "QLineEdit, QComboBox, QSpinBox, QTextEdit, QTableWidget {"
        "  background: #fffdf9;"
        "  border: 1px solid #ccb9a5;"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "}"
        "QGroupBox {"
        "  font-weight: 700;"
        "  border: 1px solid #d6c7b7;"
        "  border-radius: 12px;"
        "  margin-top: 8px;"
        "  padding-top: 12px;"
        "}"
        "QHeaderView::section {"
        "  background: #efe3d3;"
        "  color: #4a3931;"
        "  padding: 6px;"
        "  border: none;"
        "  font-weight: 700;"
        "}");
}

StudyFilter StudyAssistantWindow::currentFilter() const {
    StudyFilter filter;
    if (subjectCombo_->currentIndex() > 0) {
        filter.subject = subjectCombo_->currentText().toStdString();
    }
    if (topicCombo_->currentIndex() > 0) {
        filter.topic = topicCombo_->currentText().toStdString();
    }
    filter.requiredTags = StudyService::splitTags(tagFilterEdit_->text().toStdString());
    filter.minDifficulty = minDifficultySpin_->value();
    filter.maxDifficulty = maxDifficultySpin_->value();
    if (filter.minDifficulty > filter.maxDifficulty) {
        std::swap(filter.minDifficulty, filter.maxDifficulty);
    }
    return filter;
}

std::optional<int> StudyAssistantWindow::selectedQuestionId() const {
    const int row = questionTable_->currentRow();
    if (row < 0 || row >= static_cast<int>(currentRows_.size())) {
        return std::nullopt;
    }
    return currentRows_[row].question.id;
}

std::optional<QuestionProgress> StudyAssistantWindow::selectedProgress() const {
    const int row = questionTable_->currentRow();
    if (row < 0 || row >= static_cast<int>(currentRows_.size())) {
        return std::nullopt;
    }
    return currentRows_[row];
}

void StudyAssistantWindow::refreshFilters() {
    const QString previousSubject = subjectCombo_->currentText();
    const QString previousTopic = topicCombo_->currentText();

    const QSignalBlocker subjectBlocker(subjectCombo_);
    const QSignalBlocker topicBlocker(topicCombo_);

    subjectCombo_->clear();
    topicCombo_->clear();
    subjectCombo_->addItem("All Subjects");
    topicCombo_->addItem("All Topics");

    for (const auto& subject : service_->allSubjects()) {
        subjectCombo_->addItem(toQString(subject));
    }
    for (const auto& topic : service_->allTopics()) {
        topicCombo_->addItem(toQString(topic));
    }

    const int subjectIndex = subjectCombo_->findText(previousSubject);
    subjectCombo_->setCurrentIndex(subjectIndex >= 0 ? subjectIndex : 0);
    const int topicIndex = topicCombo_->findText(previousTopic);
    topicCombo_->setCurrentIndex(topicIndex >= 0 ? topicIndex : 0);
}

void StudyAssistantWindow::refreshQuestions() {
    currentRows_ = service_->questionProgressesMatching(currentFilter());

    questionTable_->setRowCount(static_cast<int>(currentRows_.size()));
    for (int row = 0; row < static_cast<int>(currentRows_.size()); ++row) {
        const auto& item = currentRows_[row];
        questionTable_->setItem(row, 0, new QTableWidgetItem(QString::number(item.question.id)));
        questionTable_->setItem(row, 1, new QTableWidgetItem(toQString(item.question.subject)));
        questionTable_->setItem(row, 2, new QTableWidgetItem(toQString(item.question.topic)));
        questionTable_->setItem(row, 3, new QTableWidgetItem(QString::number(item.question.difficulty)));
        questionTable_->setItem(row,
                                4,
                                new QTableWidgetItem(item.question.imageData.empty()
                                                         ? (item.question.imageName.empty() ? "No" : "Meta")
                                                         : "Yes"));
        questionTable_->setItem(row, 5, new QTableWidgetItem(QString::number(item.stats.pendingMistakes)));
        questionTable_->setItem(row, 6, new QTableWidgetItem(QString::number(item.stats.totalReviews)));
        questionTable_->setItem(row, 7, new QTableWidgetItem(joinTags(item.question.tags)));
    }

    QStringList tagList;
    for (const auto& tag : service_->allTags()) {
        tagList << toQString(tag);
    }
    summaryLabel_->setText(QString("Showing %1 questions. Known tags: %2")
                               .arg(currentRows_.size())
                               .arg(tagList.isEmpty() ? QString("none") : tagList.join(", ")));

    if (!currentRows_.empty()) {
        questionTable_->selectRow(0);
    } else {
        questionTable_->clearSelection();
        updateDetails();
    }
    setStatusMessage("Study list refreshed.");
}

void StudyAssistantWindow::selectQuestionById(int questionId) {
    for (int row = 0; row < static_cast<int>(currentRows_.size()); ++row) {
        if (currentRows_[row].question.id == questionId) {
            questionTable_->selectRow(row);
            questionTable_->scrollToItem(questionTable_->item(row, 0));
            return;
        }
    }
}

void StudyAssistantWindow::updateDetails() {
    const auto progress = selectedProgress();
    const bool hasSelection = progress.has_value();

    editButton_->setEnabled(hasSelection);
    deleteButton_->setEnabled(hasSelection);
    markCorrectButton_->setEnabled(hasSelection);
    markWrongButton_->setEnabled(hasSelection);
    aiExplainButton_->setEnabled(hasSelection && !aiRequestInFlight_);

    if (!hasSelection) {
        statsLabel_->setText("Select a question to inspect its review stats.");
        imageMetaLabel_->setText("No image attached.");
        imagePreviewLabel_->setPixmap(QPixmap());
        imagePreviewLabel_->setText("No image selected");
        promptView_->clear();
        answerView_->clear();
        explanationView_->clear();
        refreshAiPanel();
        return;
    }

    statsLabel_->setText(
        QString("Question #%1 | Reviews %2 | Correct %3 | Wrong %4 | Pending mistakes %5 | Interval %6 day(s) | Ease %7")
            .arg(progress->question.id)
            .arg(progress->stats.totalReviews)
            .arg(progress->stats.correctReviews)
            .arg(progress->stats.wrongReviews)
            .arg(progress->stats.pendingMistakes)
            .arg(progress->stats.intervalDays)
            .arg(progress->stats.easeFactor, 0, 'f', 2));
    promptView_->setPlainText(toQString(progress->question.prompt));
    answerView_->setPlainText(toQString(progress->question.answer));
    explanationView_->setPlainText(toQString(progress->question.explanation));

    const QString imageName = toQString(progress->question.imageName).trimmed();
    if (progress->question.imageData.empty()) {
        if (imageName.isEmpty()) {
            imageMetaLabel_->setText("No image attached.");
            imagePreviewLabel_->setPixmap(QPixmap());
            imagePreviewLabel_->setText("No image selected");
            refreshAiPanel();
            return;
        }

        imageMetaLabel_->setText(QString("Attached image: %1 (metadata only)").arg(imageName));
        imagePreviewLabel_->setPixmap(QPixmap());
        imagePreviewLabel_->setText("Image bytes are not available in the current record.");
        refreshAiPanel();
        return;
    }

    imageMetaLabel_->setText(QString("Attached image: %1").arg(imageName.isEmpty() ? QString("embedded_image") : imageName));
    QPixmap pixmap;
    if (!pixmap.loadFromData(toByteArray(progress->question.imageData))) {
        imagePreviewLabel_->setPixmap(QPixmap());
        imagePreviewLabel_->setText("Embedded image could not be decoded.");
        refreshAiPanel();
        return;
    }

    imagePreviewLabel_->setText(QString());
    imagePreviewLabel_->setPixmap(
        pixmap.scaled(420, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    refreshAiPanel();
}

void StudyAssistantWindow::setStatusMessage(const QString& message) {
    statusLabel_->setText(message);
}

void StudyAssistantWindow::addQuestion() {
    QuestionDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    std::string error;
    const int newId = service_->createQuestion(dialog.subject().toStdString(),
                                               dialog.topic().toStdString(),
                                               dialog.prompt().toStdString(),
                                               dialog.answer().toStdString(),
                                               dialog.explanation().toStdString(),
                                               dialog.imageName().toStdString(),
                                               dialog.imageData(),
                                               StudyService::splitTags(dialog.tags().toStdString()),
                                               dialog.difficulty(),
                                               error);
    if (newId == 0) {
        QMessageBox::warning(this, "Create Question", toQString(error));
        return;
    }

    refreshFilters();
    refreshQuestions();
    selectQuestionById(newId);
    setStatusMessage(QString("Created question #%1.").arg(newId));
}

void StudyAssistantWindow::editCurrentQuestion() {
    const auto progress = selectedProgress();
    if (!progress.has_value()) {
        setStatusMessage("Select a question first.");
        return;
    }

    QuestionDialog dialog(this);
    dialog.setWindowTitle("Edit Question");
    dialog.setQuestion(progress->question);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    std::string error;
    if (!service_->updateQuestion(progress->question.id,
                                  dialog.subject().toStdString(),
                                  dialog.topic().toStdString(),
                                  dialog.prompt().toStdString(),
                                  dialog.answer().toStdString(),
                                  dialog.explanation().toStdString(),
                                  dialog.imageName().toStdString(),
                                  dialog.imageData(),
                                  StudyService::splitTags(dialog.tags().toStdString()),
                                  dialog.difficulty(),
                                  error)) {
        QMessageBox::warning(this, "Edit Question", toQString(error));
        return;
    }

    refreshFilters();
    refreshQuestions();
    selectQuestionById(progress->question.id);
    setStatusMessage(QString("Updated question #%1.").arg(progress->question.id));
}

void StudyAssistantWindow::deleteCurrentQuestion() {
    const auto questionId = selectedQuestionId();
    if (!questionId.has_value()) {
        setStatusMessage("Select a question first.");
        return;
    }

    if (QMessageBox::question(this,
                              "Delete Question",
                              QString("Delete question #%1? This cannot be undone.").arg(*questionId)) !=
        QMessageBox::Yes) {
        return;
    }

    std::string error;
    if (!service_->removeQuestion(*questionId, error)) {
        QMessageBox::warning(this, "Delete Question", toQString(error));
        return;
    }

    refreshFilters();
    refreshQuestions();
    setStatusMessage(QString("Deleted question #%1.").arg(*questionId));
}

void StudyAssistantWindow::markCurrentQuestion(bool correct) {
    const auto questionId = selectedQuestionId();
    if (!questionId.has_value()) {
        setStatusMessage("Select a question first.");
        return;
    }

    std::string error;
    if (!service_->recordReview(*questionId, correct, error)) {
        QMessageBox::warning(this, "Record Review", toQString(error));
        return;
    }

    refreshQuestions();
    selectQuestionById(*questionId);
    setStatusMessage(correct ? "Marked selected question as correct." : "Marked selected question as wrong.");
}

void StudyAssistantWindow::recommendQuestion() {
    const auto recommendation = service_->recommendNext(currentFilter());
    if (!recommendation.has_value()) {
        setStatusMessage("No question matched the current filter.");
        return;
    }

    selectQuestionById(recommendation->question.id);
    setStatusMessage(QString("Recommended question #%1 with priority score %2.")
                         .arg(recommendation->question.id)
                         .arg(recommendation->score, 0, 'f', 2));
}

void StudyAssistantWindow::exportData() {
    const QString path = QFileDialog::getSaveFileName(this,
                                                      "Export Study Data",
                                                      "学习助手导出数据.db",
                                                      "SQLite DB (*.db);;Legacy Text (*.txt)");
    if (path.isEmpty()) {
        return;
    }

    std::string error;
    if (!service_->exportToFile(path.toStdString(), error)) {
        QMessageBox::warning(this, "Export Data", toQString(error));
        return;
    }
    setStatusMessage(QString("Exported study data to %1.").arg(path));
}

void StudyAssistantWindow::importData() {
    const QString path = QFileDialog::getOpenFileName(this,
                                                      "Import Study Data",
                                                      QString(),
                                                      "Supported Files (*.db *.txt)");
    if (path.isEmpty()) {
        return;
    }

    std::string error;
    if (!service_->importFromFile(path.toStdString(), error)) {
        QMessageBox::warning(this, "Import Data", toQString(error));
        return;
    }

    refreshFilters();
    refreshQuestions();
    setStatusMessage(QString("Imported study data from %1.").arg(path));
}

void StudyAssistantWindow::requestAiAnalysis() {
    const auto progress = selectedProgress();
    if (!progress.has_value()) {
        setStatusMessage("Select a question first.");
        return;
    }

    QString configError;
    if (!openAiClient_->isConfigured(&configError)) {
        aiAnalysisQuestionId_ = progress->question.id;
        aiAnalysisText_.clear();
        aiAnalysisView_->setPlainText(configError);
        QMessageBox::information(this, "OpenAI Setup", configError);
        setStatusMessage("OpenAI API key is missing.");
        return;
    }

    aiRequestInFlight_ = true;
    aiAnalysisQuestionId_ = progress->question.id;
    aiAnalysisText_.clear();
    aiAnalysisView_->setPlainText(
        QString("Sending question #%1 to OpenAI with model %2...")
            .arg(progress->question.id)
            .arg(openAiClient_->configuredModel()));
    aiExplainButton_->setEnabled(false);
    setStatusMessage(QString("Analyzing question #%1 with OpenAI...").arg(progress->question.id));

    const int requestQuestionId = progress->question.id;
    openAiClient_->requestWrongQuestionAnalysis(*progress, [this, requestQuestionId](const OpenAiClient::Result& result) {
        aiRequestInFlight_ = false;
        aiAnalysisQuestionId_ = requestQuestionId;
        aiAnalysisText_ = result.error.isEmpty() ? result.text : result.error;
        refreshAiPanel();

        if (!result.error.isEmpty()) {
            setStatusMessage(QString("OpenAI analysis failed for question #%1.").arg(requestQuestionId));
            return;
        }

        setStatusMessage(QString("OpenAI analysis is ready for question #%1.").arg(requestQuestionId));
    });
}

void StudyAssistantWindow::refreshAiPanel() {
    const auto progress = selectedProgress();
    if (!progress.has_value()) {
        aiAnalysisView_->setPlainText("Select a question and click AI Explain.");
        return;
    }

    if (aiRequestInFlight_ && aiAnalysisQuestionId_ == progress->question.id) {
        return;
    }

    if (aiAnalysisQuestionId_ == progress->question.id && !aiAnalysisText_.trimmed().isEmpty()) {
        aiAnalysisView_->setPlainText(aiAnalysisText_);
        return;
    }

    aiAnalysisView_->setPlainText(
        QString("Question #%1 is ready. Click AI Explain to generate a personalized analysis.")
            .arg(progress->question.id));
}
