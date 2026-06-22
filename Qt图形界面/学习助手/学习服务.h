#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

struct Question {
    int id = 0;
    std::string subject;
    std::string topic;
    std::string prompt;
    std::string answer;
    std::string explanation;
    std::string imageName;
    std::vector<unsigned char> imageData;
    std::vector<std::string> tags;
    int difficulty = 3;
};

struct ReviewStats {
    int questionId = 0;
    std::chrono::system_clock::time_point lastReviewed{};
    int intervalDays = 1;
    double easeFactor = 2.5;
    int streak = 0;
    int totalReviews = 0;
    int correctReviews = 0;
    int wrongReviews = 0;
    int pendingMistakes = 0;
};

struct StudyFilter {
    std::string subject;
    std::string topic;
    int minDifficulty = 1;
    int maxDifficulty = 5;
    std::vector<std::string> requiredTags;
};

struct Recommendation {
    Question question;
    ReviewStats stats;
    double score = 0.0;
};

struct QuestionProgress {
    Question question;
    ReviewStats stats;
    double score = 0.0;
};

class StudyService {
public:
    explicit StudyService(std::string storagePath = "study_data.db");
    ~StudyService();

    StudyService(const StudyService&) = delete;
    StudyService& operator=(const StudyService&) = delete;

    void loadOrSeed();
    bool load();
    bool save() const;

    const std::vector<Question> questions() const;
    std::vector<Question> questionsMatching(const StudyFilter& filter) const;
    std::vector<QuestionProgress> questionProgressesMatching(const StudyFilter& filter = {}) const;
    std::vector<QuestionProgress> wrongQuestions() const;
    std::optional<QuestionProgress> questionProgress(int questionId) const;
    std::vector<std::string> allSubjects() const;
    std::vector<std::string> allTopics() const;
    std::vector<std::string> allTags() const;

    int createQuestion(const std::string& subject,
                       const std::string& topic,
                       const std::string& prompt,
                       const std::string& answer,
                       const std::string& explanation,
                       const std::string& imageName,
                       const std::vector<unsigned char>& imageData,
                       const std::vector<std::string>& tags,
                       int difficulty,
                       std::string& error);
    bool updateQuestion(int id,
                        const std::string& subject,
                        const std::string& topic,
                        const std::string& prompt,
                        const std::string& answer,
                        const std::string& explanation,
                        const std::string& imageName,
                        const std::vector<unsigned char>& imageData,
                        const std::vector<std::string>& tags,
                        int difficulty,
                        std::string& error);
    bool removeQuestion(int id, std::string& error);

    std::optional<Recommendation> recommendNext(const StudyFilter& filter = {}) const;
    bool recordReview(int questionId, bool correct, std::string& error);

    bool importFromFile(const std::string& sourcePath, std::string& error);
    bool exportToFile(const std::string& targetPath, std::string& error) const;
    static std::vector<std::string> splitTags(const std::string& rawTags);
    static bool loadImageFromFile(const std::string& sourcePath,
                                  std::string& imageName,
                                  std::vector<unsigned char>& imageData,
                                  std::string& error);

private:
    struct QuestionRecord {
        Question question;
        ReviewStats stats;
    };

    bool openDatabase(std::string& error);
    void closeDatabase();
    bool ensureSchema(std::string& error) const;
    bool ensureQuestionColumn(const std::string& columnName,
                              const std::string& columnDefinition,
                              std::string& error) const;
    bool hasQuestions(bool& hasData, std::string& error) const;
    bool execute(const std::string& sql, std::string& error) const;
    std::vector<QuestionRecord> fetchAllRecords(std::string& error) const;
    std::optional<QuestionRecord> fetchRecordById(int id, std::string& error) const;
    std::vector<std::string> fetchTagsForQuestion(int questionId, std::string& error) const;
    std::vector<std::string> fetchDistinctColumnValues(const std::string& sql, std::string& error) const;
    bool replaceAllRecords(const std::vector<QuestionRecord>& records, std::string& error);
    bool syncQuestionTags(int questionId, const std::vector<std::string>& tags, std::string& error);
    bool backfillNormalizedTags(std::string& error);
    bool backfillEmbeddedImages(std::string& error);
    bool importLegacyTextFile(const std::string& sourcePath, std::string& error);
    bool exportLegacyTextFile(const std::string& targetPath, std::string& error) const;
    bool importSqliteDatabase(const std::string& sourcePath, std::string& error);
    bool exportSqliteDatabase(const std::string& targetPath, std::string& error) const;

    static QuestionRecord recordFromStatement(sqlite3_stmt* statement);
    static std::string legacyTextPathFor(const std::string& databasePath);
    static bool fileExists(const std::string& path);
    static bool looksLikeLegacyTextFile(const std::string& path);
    static std::string joinTags(const std::vector<std::string>& tags);
    static std::string escape(const std::string& value);
    static std::string unescape(const std::string& value);
    static std::string timePointToString(std::chrono::system_clock::time_point tp);
    static std::chrono::system_clock::time_point stringToTimePoint(const std::string& text);

    double computeScore(const QuestionRecord& record) const;
    bool satisfiesFilter(const QuestionRecord& record, const StudyFilter& filter) const;
    bool validateQuestion(const std::string& subject,
                          const std::string& topic,
                          const std::string& prompt,
                          const std::string& answer,
                          int difficulty,
                          std::string& error) const;
    void seedDemoData();

    sqlite3* db_ = nullptr;
    std::string storagePath_;
};
