#include "study_service.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

#include <sqlite3.h>

namespace {
constexpr const char* kLegacyHeaderV1 = "STUDY_ASSISTANT_V1";
constexpr const char* kLegacyHeaderV2 = "STUDY_ASSISTANT_V2";
constexpr const char* kLegacyHeaderV3 = "STUDY_ASSISTANT_V3";
constexpr const char* kLegacyHeaderV4 = "STUDY_ASSISTANT_V4";
constexpr double kMinEaseFactor = 1.3;
constexpr double kMaxEaseFactor = 3.0;

std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

bool containsAllTags(const std::vector<std::string>& owned, const std::vector<std::string>& required) {
    for (const auto& tag : required) {
        if (std::find(owned.begin(), owned.end(), tag) == owned.end()) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> splitLine(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, '\t')) {
        fields.push_back(field);
    }
    return fields;
}

std::string sqliteError(sqlite3* db) {
    return db == nullptr ? "SQLite database handle is null." : sqlite3_errmsg(db);
}

std::string extensionOf(const std::string& path) {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return "";
    }

    std::string extension = path.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return extension;
}

std::string fileNameOf(const std::string& path) {
    if (trim(path).empty()) {
        return "";
    }

    const auto filePath = std::filesystem::u8path(path);
    return filePath.filename().u8string();
}

bool readBinaryFile(const std::string& path, std::vector<unsigned char>& bytes, std::string& error) {
    bytes.clear();
    if (trim(path).empty()) {
        return true;
    }

    std::ifstream input(std::filesystem::u8path(path), std::ios::binary);
    if (!input.is_open()) {
        error = "Failed to open image file: " + path;
        return false;
    }

    bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    if (input.bad()) {
        error = "Failed to read image file: " + path;
        bytes.clear();
        return false;
    }
    return true;
}
}  // namespace

StudyService::StudyService(std::string storagePath)
    : storagePath_(std::move(storagePath)) {}

StudyService::~StudyService() {
    closeDatabase();
}

void StudyService::loadOrSeed() {
    if (!load()) {
        return;
    }

    bool hasData = false;
    std::string error;
    if (!hasQuestions(hasData, error)) {
        return;
    }
    if (hasData) {
        return;
    }

    const std::string legacyPath = legacyTextPathFor(storagePath_);
    if (fileExists(legacyPath) && importLegacyTextFile(legacyPath, error)) {
        return;
    }

    seedDemoData();
}

bool StudyService::load() {
    std::string error;
    if (!openDatabase(error)) {
        return false;
    }
    return ensureSchema(error);
}

bool StudyService::save() const {
    return db_ != nullptr;
}

const std::vector<Question> StudyService::questions() const {
    std::string error;
    const auto records = fetchAllRecords(error);

    std::vector<Question> list;
    list.reserve(records.size());
    for (const auto& record : records) {
        list.push_back(record.question);
    }
    return list;
}

std::vector<Question> StudyService::questionsMatching(const StudyFilter& filter) const {
    std::string error;
    const auto records = fetchAllRecords(error);

    std::vector<Question> list;
    for (const auto& record : records) {
        if (satisfiesFilter(record, filter)) {
            list.push_back(record.question);
        }
    }
    return list;
}

std::vector<QuestionProgress> StudyService::questionProgressesMatching(const StudyFilter& filter) const {
    std::string error;
    const auto records = fetchAllRecords(error);

    std::vector<QuestionProgress> list;
    for (const auto& record : records) {
        if (!satisfiesFilter(record, filter)) {
            continue;
        }
        list.push_back(QuestionProgress{record.question, record.stats, computeScore(record)});
    }

    std::sort(list.begin(), list.end(), [](const QuestionProgress& left, const QuestionProgress& right) {
        return left.question.id < right.question.id;
    });
    return list;
}

std::vector<QuestionProgress> StudyService::wrongQuestions() const {
    std::string error;
    const auto records = fetchAllRecords(error);

    std::vector<QuestionProgress> list;
    for (const auto& record : records) {
        if (record.stats.pendingMistakes <= 0) {
            continue;
        }
        list.push_back(QuestionProgress{record.question, record.stats, computeScore(record)});
    }

    std::sort(list.begin(), list.end(), [](const QuestionProgress& left, const QuestionProgress& right) {
        if (left.stats.pendingMistakes != right.stats.pendingMistakes) {
            return left.stats.pendingMistakes > right.stats.pendingMistakes;
        }
        return left.score > right.score;
    });
    return list;
}

std::optional<QuestionProgress> StudyService::questionProgress(int questionId) const {
    std::string error;
    const auto record = fetchRecordById(questionId, error);
    if (!record.has_value()) {
        return std::nullopt;
    }
    return QuestionProgress{record->question, record->stats, computeScore(*record)};
}

std::vector<std::string> StudyService::allSubjects() const {
    std::string error;
    return fetchDistinctColumnValues("SELECT DISTINCT subject FROM questions ORDER BY subject COLLATE NOCASE", error);
}

std::vector<std::string> StudyService::allTopics() const {
    std::string error;
    return fetchDistinctColumnValues("SELECT DISTINCT topic FROM questions ORDER BY topic COLLATE NOCASE", error);
}

std::vector<std::string> StudyService::allTags() const {
    std::string error;
    return fetchDistinctColumnValues("SELECT name FROM tags ORDER BY name COLLATE NOCASE", error);
}

bool StudyService::openDatabase(std::string& error) {
    if (db_ != nullptr) {
        return true;
    }

    if (sqlite3_open(storagePath_.c_str(), &db_) != SQLITE_OK) {
        error = sqliteError(db_);
        closeDatabase();
        return false;
    }

    if (!execute("PRAGMA foreign_keys = ON", error)) {
        closeDatabase();
        return false;
    }

    return true;
}

void StudyService::closeDatabase() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool StudyService::ensureSchema(std::string& error) const {
    const char* questionSql =
        "CREATE TABLE IF NOT EXISTS questions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "subject TEXT NOT NULL,"
        "topic TEXT NOT NULL,"
        "prompt TEXT NOT NULL,"
        "answer TEXT NOT NULL,"
        "explanation TEXT NOT NULL DEFAULT '',"
        "image_name TEXT NOT NULL DEFAULT '',"
        "image_data BLOB,"
        "tags TEXT NOT NULL DEFAULT '',"
        "difficulty INTEGER NOT NULL,"
        "last_reviewed INTEGER NOT NULL DEFAULT 0,"
        "interval_days INTEGER NOT NULL DEFAULT 1,"
        "ease_factor REAL NOT NULL DEFAULT 2.5,"
        "streak INTEGER NOT NULL DEFAULT 0,"
        "total_reviews INTEGER NOT NULL DEFAULT 0,"
        "correct_reviews INTEGER NOT NULL DEFAULT 0,"
        "wrong_reviews INTEGER NOT NULL DEFAULT 0,"
        "pending_mistakes INTEGER NOT NULL DEFAULT 0"
        ")";
    if (!execute(questionSql, error)) {
        return false;
    }

    const char* tagSql =
        "CREATE TABLE IF NOT EXISTS tags ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL UNIQUE"
        ")";
    if (!execute(tagSql, error)) {
        return false;
    }

    const char* questionTagSql =
        "CREATE TABLE IF NOT EXISTS question_tags ("
        "question_id INTEGER NOT NULL,"
        "tag_id INTEGER NOT NULL,"
        "PRIMARY KEY (question_id, tag_id),"
        "FOREIGN KEY (question_id) REFERENCES questions(id) ON DELETE CASCADE,"
        "FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE"
        ")";
    if (!execute(questionTagSql, error)) {
        return false;
    }

    if (!execute("CREATE INDEX IF NOT EXISTS idx_questions_subject ON questions(subject)", error)) {
        return false;
    }
    if (!execute("CREATE INDEX IF NOT EXISTS idx_questions_topic ON questions(topic)", error)) {
        return false;
    }
    if (!execute("CREATE INDEX IF NOT EXISTS idx_question_tags_tag ON question_tags(tag_id)", error)) {
        return false;
    }

    if (!ensureQuestionColumn("image_path", "TEXT NOT NULL DEFAULT ''", error)) {
        return false;
    }
    if (!ensureQuestionColumn("image_name", "TEXT NOT NULL DEFAULT ''", error)) {
        return false;
    }
    if (!ensureQuestionColumn("image_data", "BLOB", error)) {
        return false;
    }

    if (!const_cast<StudyService*>(this)->backfillEmbeddedImages(error)) {
        return false;
    }
    return const_cast<StudyService*>(this)->backfillNormalizedTags(error);
}

bool StudyService::ensureQuestionColumn(const std::string& columnName,
                                        const std::string& columnDefinition,
                                        std::string& error) const {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, "PRAGMA table_info(questions)", -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return false;
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        const auto* value = sqlite3_column_text(statement, 1);
        const std::string existingName = value == nullptr ? "" : reinterpret_cast<const char*>(value);
        if (existingName == columnName) {
            sqlite3_finalize(statement);
            return true;
        }
    }
    sqlite3_finalize(statement);

    return execute("ALTER TABLE questions ADD COLUMN " + columnName + " " + columnDefinition, error);
}

bool StudyService::hasQuestions(bool& hasData, std::string& error) const {
    hasData = false;
    const char* sql = "SELECT COUNT(*) FROM questions";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return false;
    }

    const int stepResult = sqlite3_step(statement);
    if (stepResult == SQLITE_ROW) {
        hasData = sqlite3_column_int(statement, 0) > 0;
        sqlite3_finalize(statement);
        return true;
    }

    sqlite3_finalize(statement);
    error = sqliteError(db_);
    return false;
}

bool StudyService::execute(const std::string& sql, std::string& error) const {
    char* sqliteMessage = nullptr;
    const int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &sqliteMessage);
    if (result == SQLITE_OK) {
        return true;
    }

    if (sqliteMessage != nullptr) {
        error = sqliteMessage;
        sqlite3_free(sqliteMessage);
    } else {
        error = sqliteError(db_);
    }
    return false;
}

std::vector<StudyService::QuestionRecord> StudyService::fetchAllRecords(std::string& error) const {
    std::vector<QuestionRecord> records;
    if (db_ == nullptr) {
        error = "SQLite database is not open.";
        return records;
    }

    const char* sql =
        "SELECT id, subject, topic, prompt, answer, explanation, image_name, image_data, tags, difficulty, "
        "last_reviewed, interval_days, ease_factor, streak, total_reviews, correct_reviews, wrong_reviews, "
        "pending_mistakes "
        "FROM questions ORDER BY id";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return records;
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        records.push_back(recordFromStatement(statement));
    }
    sqlite3_finalize(statement);

    for (auto& record : records) {
        record.question.tags = fetchTagsForQuestion(record.question.id, error);
        if (!error.empty()) {
            return {};
        }
    }
    return records;
}

std::optional<StudyService::QuestionRecord> StudyService::fetchRecordById(int id, std::string& error) const {
    if (db_ == nullptr) {
        error = "SQLite database is not open.";
        return std::nullopt;
    }

    const char* sql =
        "SELECT id, subject, topic, prompt, answer, explanation, image_name, image_data, tags, difficulty, "
        "last_reviewed, interval_days, ease_factor, streak, total_reviews, correct_reviews, wrong_reviews, "
        "pending_mistakes "
        "FROM questions WHERE id = ?";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return std::nullopt;
    }

    sqlite3_bind_int(statement, 1, id);
    const int stepResult = sqlite3_step(statement);
    if (stepResult == SQLITE_ROW) {
        auto record = recordFromStatement(statement);
        sqlite3_finalize(statement);
        record.question.tags = fetchTagsForQuestion(record.question.id, error);
        if (!error.empty()) {
            return std::nullopt;
        }
        return record;
    }

    sqlite3_finalize(statement);
    if (stepResult != SQLITE_DONE) {
        error = sqliteError(db_);
    }
    return std::nullopt;
}

int StudyService::createQuestion(const std::string& subject,
                                 const std::string& topic,
                                 const std::string& prompt,
                                 const std::string& answer,
                                 const std::string& explanation,
                                 const std::string& imageName,
                                 const std::vector<unsigned char>& imageData,
                                 const std::vector<std::string>& tags,
                                 int difficulty,
                                 std::string& error) {
    if (!validateQuestion(subject, topic, prompt, answer, difficulty, error)) {
        return 0;
    }
    if (db_ == nullptr && !load()) {
        error = "Failed to open SQLite database.";
        return 0;
    }
    if (!execute("BEGIN IMMEDIATE TRANSACTION", error)) {
        return 0;
    }

    const char* sql =
        "INSERT INTO questions "
        "(subject, topic, prompt, answer, explanation, image_name, image_data, tags, difficulty, last_reviewed, "
        "interval_days, ease_factor, streak, total_reviews, correct_reviews, wrong_reviews, pending_mistakes) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        return 0;
    }

    const std::string subjectValue = trim(subject);
    const std::string topicValue = trim(topic);
    const std::string promptValue = trim(prompt);
    const std::string answerValue = trim(answer);
    const std::string explanationValue = trim(explanation);
    const std::string imageNameValue = trim(imageName);
    const std::string tagsValue = joinTags(tags);
    const auto lastReviewed = std::chrono::system_clock::now() - std::chrono::hours(24 * difficulty);

    sqlite3_bind_text(statement, 1, subjectValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, topicValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, promptValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, answerValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 5, explanationValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 6, imageNameValue.c_str(), -1, SQLITE_TRANSIENT);
    if (imageData.empty()) {
        sqlite3_bind_null(statement, 7);
    } else {
        sqlite3_bind_blob(statement,
                          7,
                          imageData.data(),
                          static_cast<int>(imageData.size()),
                          SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(statement, 8, tagsValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 9, difficulty);
    sqlite3_bind_int64(statement, 10, static_cast<sqlite3_int64>(
                                         std::chrono::duration_cast<std::chrono::seconds>(
                                             lastReviewed.time_since_epoch())
                                             .count()));
    sqlite3_bind_int(statement, 11, 1);
    sqlite3_bind_double(statement, 12, 2.5);
    sqlite3_bind_int(statement, 13, 0);
    sqlite3_bind_int(statement, 14, 0);
    sqlite3_bind_int(statement, 15, 0);
    sqlite3_bind_int(statement, 16, 0);
    sqlite3_bind_int(statement, 17, 0);

    const int stepResult = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (stepResult != SQLITE_DONE) {
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        error = sqliteError(db_);
        return 0;
    }

    const int questionId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    if (!syncQuestionTags(questionId, tags, error)) {
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        return 0;
    }
    if (!execute("COMMIT", error)) {
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        return 0;
    }

    return questionId;
}

bool StudyService::updateQuestion(int id,
                                  const std::string& subject,
                                  const std::string& topic,
                                  const std::string& prompt,
                                  const std::string& answer,
                                  const std::string& explanation,
                                  const std::string& imageName,
                                  const std::vector<unsigned char>& imageData,
                                  const std::vector<std::string>& tags,
                                  int difficulty,
                                  std::string& error) {
    if (!validateQuestion(subject, topic, prompt, answer, difficulty, error)) {
        return false;
    }
    if (db_ == nullptr && !load()) {
        error = "Failed to open SQLite database.";
        return false;
    }
    if (!execute("BEGIN IMMEDIATE TRANSACTION", error)) {
        return false;
    }

    const char* sql =
        "UPDATE questions SET subject = ?, topic = ?, prompt = ?, answer = ?, explanation = ?, image_name = ?, "
        "image_data = ?, image_path = '', tags = ?, difficulty = ? WHERE id = ?";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        return false;
    }

    const std::string subjectValue = trim(subject);
    const std::string topicValue = trim(topic);
    const std::string promptValue = trim(prompt);
    const std::string answerValue = trim(answer);
    const std::string explanationValue = trim(explanation);
    const std::string imageNameValue = trim(imageName);
    const std::string tagsValue = joinTags(tags);

    sqlite3_bind_text(statement, 1, subjectValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, topicValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, promptValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, answerValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 5, explanationValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 6, imageNameValue.c_str(), -1, SQLITE_TRANSIENT);
    if (imageData.empty()) {
        sqlite3_bind_null(statement, 7);
    } else {
        sqlite3_bind_blob(statement,
                          7,
                          imageData.data(),
                          static_cast<int>(imageData.size()),
                          SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(statement, 8, tagsValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 9, difficulty);
    sqlite3_bind_int(statement, 10, id);

    const int stepResult = sqlite3_step(statement);
    const int changedRows = sqlite3_changes(db_);
    sqlite3_finalize(statement);

    if (stepResult != SQLITE_DONE) {
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        error = sqliteError(db_);
        return false;
    }
    if (changedRows == 0) {
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        error = "Question not found.";
        return false;
    }
    if (!syncQuestionTags(id, tags, error)) {
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        return false;
    }
    if (!execute("COMMIT", error)) {
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        return false;
    }
    return true;
}

bool StudyService::removeQuestion(int id, std::string& error) {
    if (db_ == nullptr && !load()) {
        error = "Failed to open SQLite database.";
        return false;
    }

    const char* sql = "DELETE FROM questions WHERE id = ?";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return false;
    }

    sqlite3_bind_int(statement, 1, id);
    const int stepResult = sqlite3_step(statement);
    const int changedRows = sqlite3_changes(db_);
    sqlite3_finalize(statement);

    if (stepResult != SQLITE_DONE) {
        error = sqliteError(db_);
        return false;
    }
    if (changedRows == 0) {
        error = "Question not found.";
        return false;
    }
    return true;
}

std::optional<Recommendation> StudyService::recommendNext(const StudyFilter& filter) const {
    std::string error;
    const auto records = fetchAllRecords(error);

    std::optional<Question> bestQuestion;
    std::optional<ReviewStats> bestStats;
    double bestScore = -1.0;

    for (const auto& record : records) {
        if (!satisfiesFilter(record, filter)) {
            continue;
        }

        const double score = computeScore(record);
        if (score > bestScore) {
            bestScore = score;
            bestQuestion = record.question;
            bestStats = record.stats;
        }
    }

    if (!bestQuestion.has_value() || !bestStats.has_value()) {
        return std::nullopt;
    }

    return Recommendation{*bestQuestion, *bestStats, bestScore};
}

bool StudyService::recordReview(int questionId, bool correct, std::string& error) {
    auto record = fetchRecordById(questionId, error);
    if (!record.has_value()) {
        if (error.empty()) {
            error = "Question not found.";
        }
        return false;
    }

    ReviewStats& stats = record->stats;
    stats.lastReviewed = std::chrono::system_clock::now();
    ++stats.totalReviews;

    if (correct) {
        ++stats.correctReviews;
        ++stats.streak;
        if (stats.streak == 1) {
            stats.intervalDays = 1;
        } else if (stats.streak == 2) {
            stats.intervalDays = 3;
        } else {
            stats.intervalDays = std::max(1, static_cast<int>(std::round(stats.intervalDays * stats.easeFactor)));
        }
        stats.easeFactor = std::min(kMaxEaseFactor, stats.easeFactor + 0.1);
        if (stats.pendingMistakes > 0) {
            --stats.pendingMistakes;
        }
    } else {
        ++stats.wrongReviews;
        stats.streak = 0;
        stats.intervalDays = 1;
        stats.easeFactor = std::max(kMinEaseFactor, stats.easeFactor - 0.2);
        ++stats.pendingMistakes;
    }

    const char* sql =
        "UPDATE questions SET last_reviewed = ?, interval_days = ?, ease_factor = ?, streak = ?, "
        "total_reviews = ?, correct_reviews = ?, wrong_reviews = ?, pending_mistakes = ? WHERE id = ?";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return false;
    }

    sqlite3_bind_int64(statement, 1, static_cast<sqlite3_int64>(
                                         std::chrono::duration_cast<std::chrono::seconds>(
                                             stats.lastReviewed.time_since_epoch())
                                             .count()));
    sqlite3_bind_int(statement, 2, stats.intervalDays);
    sqlite3_bind_double(statement, 3, stats.easeFactor);
    sqlite3_bind_int(statement, 4, stats.streak);
    sqlite3_bind_int(statement, 5, stats.totalReviews);
    sqlite3_bind_int(statement, 6, stats.correctReviews);
    sqlite3_bind_int(statement, 7, stats.wrongReviews);
    sqlite3_bind_int(statement, 8, stats.pendingMistakes);
    sqlite3_bind_int(statement, 9, questionId);

    const int stepResult = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (stepResult != SQLITE_DONE) {
        error = sqliteError(db_);
        return false;
    }
    return true;
}

bool StudyService::importFromFile(const std::string& sourcePath, std::string& error) {
    if (looksLikeLegacyTextFile(sourcePath)) {
        return importLegacyTextFile(sourcePath, error);
    }
    return importSqliteDatabase(sourcePath, error);
}

bool StudyService::exportToFile(const std::string& targetPath, std::string& error) const {
    if (extensionOf(targetPath) == ".txt") {
        return exportLegacyTextFile(targetPath, error);
    }
    return exportSqliteDatabase(targetPath, error);
}

std::vector<std::string> StudyService::splitTags(const std::string& rawTags) {
    std::vector<std::string> tags;
    std::stringstream stream(rawTags);
    std::string part;
    while (std::getline(stream, part, ',')) {
        const std::string cleaned = trim(part);
        if (!cleaned.empty()) {
            tags.push_back(cleaned);
        }
    }
    return tags;
}

bool StudyService::loadImageFromFile(const std::string& sourcePath,
                                     std::string& imageName,
                                     std::vector<unsigned char>& imageData,
                                     std::string& error) {
    imageName.clear();
    imageData.clear();

    const std::string trimmedPath = trim(sourcePath);
    if (trimmedPath.empty()) {
        return true;
    }

    if (!readBinaryFile(trimmedPath, imageData, error)) {
        return false;
    }

    imageName = fileNameOf(trimmedPath);
    if (imageName.empty()) {
        error = "Failed to read image file name: " + trimmedPath;
        imageData.clear();
        return false;
    }
    return true;
}

std::vector<std::string> StudyService::fetchTagsForQuestion(int questionId, std::string& error) const {
    std::vector<std::string> tags;
    const char* sql =
        "SELECT t.name "
        "FROM tags t "
        "INNER JOIN question_tags qt ON qt.tag_id = t.id "
        "WHERE qt.question_id = ? "
        "ORDER BY t.name COLLATE NOCASE";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return tags;
    }

    sqlite3_bind_int(statement, 1, questionId);
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const auto* value = sqlite3_column_text(statement, 0);
        if (value != nullptr) {
            tags.emplace_back(reinterpret_cast<const char*>(value));
        }
    }
    sqlite3_finalize(statement);
    return tags;
}

std::vector<std::string> StudyService::fetchDistinctColumnValues(const std::string& sql, std::string& error) const {
    std::vector<std::string> values;
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return values;
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        const auto* value = sqlite3_column_text(statement, 0);
        if (value != nullptr) {
            values.emplace_back(reinterpret_cast<const char*>(value));
        }
    }
    sqlite3_finalize(statement);
    return values;
}

bool StudyService::syncQuestionTags(int questionId, const std::vector<std::string>& tags, std::string& error) {
    const char* deleteSql = "DELETE FROM question_tags WHERE question_id = ?";
    sqlite3_stmt* deleteStatement = nullptr;
    if (sqlite3_prepare_v2(db_, deleteSql, -1, &deleteStatement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return false;
    }
    sqlite3_bind_int(deleteStatement, 1, questionId);
    if (sqlite3_step(deleteStatement) != SQLITE_DONE) {
        error = sqliteError(db_);
        sqlite3_finalize(deleteStatement);
        return false;
    }
    sqlite3_finalize(deleteStatement);

    const char* insertTagSql = "INSERT OR IGNORE INTO tags(name) VALUES (?)";
    const char* insertRelationSql =
        "INSERT OR IGNORE INTO question_tags(question_id, tag_id) "
        "SELECT ?, id FROM tags WHERE name = ?";

    sqlite3_stmt* tagStatement = nullptr;
    sqlite3_stmt* relationStatement = nullptr;
    if (sqlite3_prepare_v2(db_, insertTagSql, -1, &tagStatement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return false;
    }
    if (sqlite3_prepare_v2(db_, insertRelationSql, -1, &relationStatement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        sqlite3_finalize(tagStatement);
        return false;
    }

    const auto normalizedTags = splitTags(joinTags(tags));
    for (const auto& tag : normalizedTags) {
        sqlite3_reset(tagStatement);
        sqlite3_clear_bindings(tagStatement);
        sqlite3_bind_text(tagStatement, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(tagStatement) != SQLITE_DONE) {
            error = sqliteError(db_);
            sqlite3_finalize(tagStatement);
            sqlite3_finalize(relationStatement);
            return false;
        }

        sqlite3_reset(relationStatement);
        sqlite3_clear_bindings(relationStatement);
        sqlite3_bind_int(relationStatement, 1, questionId);
        sqlite3_bind_text(relationStatement, 2, tag.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(relationStatement) != SQLITE_DONE) {
            error = sqliteError(db_);
            sqlite3_finalize(tagStatement);
            sqlite3_finalize(relationStatement);
            return false;
        }
    }

    sqlite3_finalize(tagStatement);
    sqlite3_finalize(relationStatement);
    return true;
}

bool StudyService::backfillNormalizedTags(std::string& error) {
    const char* sql = "SELECT id, tags FROM questions ORDER BY id";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return false;
    }

    std::vector<std::pair<int, std::vector<std::string>>> questionTags;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const int questionId = sqlite3_column_int(statement, 0);
        const auto* raw = sqlite3_column_text(statement, 1);
        const std::string rawTags = raw == nullptr ? "" : reinterpret_cast<const char*>(raw);
        questionTags.push_back({questionId, splitTags(rawTags)});
    }
    sqlite3_finalize(statement);

    for (const auto& entry : questionTags) {
        const auto existingTags = fetchTagsForQuestion(entry.first, error);
        if (!error.empty()) {
            return false;
        }
        if (!existingTags.empty() || entry.second.empty()) {
            continue;
        }
        if (!syncQuestionTags(entry.first, entry.second, error)) {
            return false;
        }
    }
    return true;
}

bool StudyService::backfillEmbeddedImages(std::string& error) {
    struct PendingImageUpdate {
        int questionId = 0;
        std::string imageName;
        std::vector<unsigned char> imageData;
    };

    const char* selectSql = "SELECT id, image_path, image_name, image_data FROM questions ORDER BY id";
    sqlite3_stmt* selectStatement = nullptr;
    if (sqlite3_prepare_v2(db_, selectSql, -1, &selectStatement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        return false;
    }

    std::vector<PendingImageUpdate> updates;
    while (sqlite3_step(selectStatement) == SQLITE_ROW) {
        const int questionId = sqlite3_column_int(selectStatement, 0);
        const auto* rawPath = sqlite3_column_text(selectStatement, 1);
        const auto* rawName = sqlite3_column_text(selectStatement, 2);
        const std::string imagePath = rawPath == nullptr ? "" : reinterpret_cast<const char*>(rawPath);
        const std::string existingImageName = rawName == nullptr ? "" : reinterpret_cast<const char*>(rawName);
        const bool hasEmbeddedData = sqlite3_column_type(selectStatement, 3) != SQLITE_NULL;

        if (hasEmbeddedData || trim(imagePath).empty()) {
            continue;
        }

        PendingImageUpdate update;
        update.questionId = questionId;
        std::string fileError;
        if (!loadImageFromFile(imagePath, update.imageName, update.imageData, fileError)) {
            continue;
        }
        if (update.imageName.empty()) {
            update.imageName = existingImageName.empty() ? fileNameOf(imagePath) : existingImageName;
        }
        updates.push_back(std::move(update));
    }
    sqlite3_finalize(selectStatement);

    if (updates.empty()) {
        return true;
    }

    if (!execute("BEGIN IMMEDIATE TRANSACTION", error)) {
        return false;
    }

    const char* updateSql = "UPDATE questions SET image_name = ?, image_data = ?, image_path = '' WHERE id = ?";
    sqlite3_stmt* updateStatement = nullptr;
    if (sqlite3_prepare_v2(db_, updateSql, -1, &updateStatement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        return false;
    }

    for (const auto& update : updates) {
        sqlite3_reset(updateStatement);
        sqlite3_clear_bindings(updateStatement);

        sqlite3_bind_text(updateStatement, 1, update.imageName.c_str(), -1, SQLITE_TRANSIENT);
        if (update.imageData.empty()) {
            sqlite3_bind_blob(updateStatement, 2, "", 0, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_blob(updateStatement,
                              2,
                              update.imageData.data(),
                              static_cast<int>(update.imageData.size()),
                              SQLITE_TRANSIENT);
        }
        sqlite3_bind_int(updateStatement, 3, update.questionId);

        if (sqlite3_step(updateStatement) != SQLITE_DONE) {
            error = sqliteError(db_);
            sqlite3_finalize(updateStatement);
            std::string rollbackError;
            execute("ROLLBACK", rollbackError);
            return false;
        }
    }

    sqlite3_finalize(updateStatement);
    return execute("COMMIT", error);
}

bool StudyService::replaceAllRecords(const std::vector<QuestionRecord>& records, std::string& error) {
    if (!execute("BEGIN IMMEDIATE TRANSACTION", error)) {
        return false;
    }

    if (!execute("DELETE FROM questions", error)) {
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        return false;
    }

    const char* sql =
        "INSERT INTO questions "
        "(id, subject, topic, prompt, answer, explanation, image_name, image_data, tags, difficulty, "
        "last_reviewed, interval_days, ease_factor, streak, total_reviews, correct_reviews, wrong_reviews, "
        "pending_mistakes) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqliteError(db_);
        std::string rollbackError;
        execute("ROLLBACK", rollbackError);
        return false;
    }

    for (const auto& record : records) {
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);

        const std::string tagsValue = joinTags(record.question.tags);
        sqlite3_bind_int(statement, 1, record.question.id);
        sqlite3_bind_text(statement, 2, record.question.subject.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 3, record.question.topic.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 4, record.question.prompt.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 5, record.question.answer.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 6, record.question.explanation.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 7, record.question.imageName.c_str(), -1, SQLITE_TRANSIENT);
        if (record.question.imageData.empty()) {
            sqlite3_bind_null(statement, 8);
        } else {
            sqlite3_bind_blob(statement,
                              8,
                              record.question.imageData.data(),
                              static_cast<int>(record.question.imageData.size()),
                              SQLITE_TRANSIENT);
        }
        sqlite3_bind_text(statement, 9, tagsValue.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(statement, 10, record.question.difficulty);
        sqlite3_bind_int64(statement, 11, static_cast<sqlite3_int64>(
                                             std::chrono::duration_cast<std::chrono::seconds>(
                                                 record.stats.lastReviewed.time_since_epoch())
                                                 .count()));
        sqlite3_bind_int(statement, 12, record.stats.intervalDays);
        sqlite3_bind_double(statement, 13, record.stats.easeFactor);
        sqlite3_bind_int(statement, 14, record.stats.streak);
        sqlite3_bind_int(statement, 15, record.stats.totalReviews);
        sqlite3_bind_int(statement, 16, record.stats.correctReviews);
        sqlite3_bind_int(statement, 17, record.stats.wrongReviews);
        sqlite3_bind_int(statement, 18, record.stats.pendingMistakes);

        if (sqlite3_step(statement) != SQLITE_DONE) {
            error = sqliteError(db_);
            sqlite3_finalize(statement);
            std::string rollbackError;
            execute("ROLLBACK", rollbackError);
            return false;
        }

        if (!syncQuestionTags(record.question.id, record.question.tags, error)) {
            sqlite3_finalize(statement);
            std::string rollbackError;
            execute("ROLLBACK", rollbackError);
            return false;
        }
    }

    sqlite3_finalize(statement);
    return execute("COMMIT", error);
}

bool StudyService::importLegacyTextFile(const std::string& sourcePath, std::string& error) {
    std::ifstream input(std::filesystem::u8path(sourcePath));
    if (!input.is_open()) {
        error = "Failed to open legacy text file.";
        return false;
    }

    std::string header;
    std::getline(input, header);
    if (header != kLegacyHeaderV1 && header != kLegacyHeaderV2 && header != kLegacyHeaderV3 &&
        header != kLegacyHeaderV4) {
        error = "Unsupported legacy text format.";
        return false;
    }

    std::vector<QuestionRecord> records;
    std::string line;
    while (std::getline(input, line)) {
        if (trim(line).empty()) {
            continue;
        }

        const auto fields = splitLine(line);
        if (fields.size() != 12 && fields.size() != 16 && fields.size() != 17) {
            error = "Corrupted legacy text file.";
            return false;
        }

        QuestionRecord record;
        record.question.id = std::stoi(fields[0]);
        record.question.subject = unescape(fields[1]);
        record.question.topic = unescape(fields[2]);
        record.question.prompt = unescape(fields[3]);
        record.question.answer = unescape(fields[4]);
        record.question.explanation = unescape(fields[5]);
        record.question.tags = splitTags(unescape(fields[6]));
        record.question.difficulty = std::stoi(fields[7]);
        record.stats.questionId = record.question.id;
        record.stats.lastReviewed = stringToTimePoint(fields[8]);
        record.stats.intervalDays = std::stoi(fields[9]);
        record.stats.easeFactor = std::stod(fields[10]);
        record.stats.streak = std::stoi(fields[11]);
        if (fields.size() >= 16) {
            record.stats.totalReviews = std::stoi(fields[12]);
            record.stats.correctReviews = std::stoi(fields[13]);
            record.stats.wrongReviews = std::stoi(fields[14]);
            record.stats.pendingMistakes = std::stoi(fields[15]);
        }
        if (fields.size() == 17) {
            const std::string imageField = unescape(fields[16]);
            if (header == kLegacyHeaderV4) {
                record.question.imageName = imageField;
            } else {
                std::string imageLoadError;
                loadImageFromFile(imageField, record.question.imageName, record.question.imageData, imageLoadError);
                if (record.question.imageName.empty()) {
                    record.question.imageName = fileNameOf(imageField);
                }
            }
        }
        records.push_back(record);
    }

    return replaceAllRecords(records, error);
}

bool StudyService::exportLegacyTextFile(const std::string& targetPath, std::string& error) const {
    std::string fetchError;
    const auto records = fetchAllRecords(fetchError);
    if (!fetchError.empty()) {
        error = fetchError;
        return false;
    }

    std::ofstream output(std::filesystem::u8path(targetPath), std::ios::trunc);
    if (!output.is_open()) {
        error = "Failed to open export file.";
        return false;
    }

    output << kLegacyHeaderV4 << '\n';
    for (const auto& record : records) {
        output << record.question.id << '\t'
               << escape(record.question.subject) << '\t'
               << escape(record.question.topic) << '\t'
               << escape(record.question.prompt) << '\t'
               << escape(record.question.answer) << '\t'
               << escape(record.question.explanation) << '\t'
               << escape(joinTags(record.question.tags)) << '\t'
               << record.question.difficulty << '\t'
               << timePointToString(record.stats.lastReviewed) << '\t'
               << record.stats.intervalDays << '\t'
               << record.stats.easeFactor << '\t'
               << record.stats.streak << '\t'
               << record.stats.totalReviews << '\t'
               << record.stats.correctReviews << '\t'
               << record.stats.wrongReviews << '\t'
               << record.stats.pendingMistakes << '\t'
               << escape(record.question.imageName) << '\n';
    }
    return true;
}

bool StudyService::importSqliteDatabase(const std::string& sourcePath, std::string& error) {
    sqlite3* sourceDb = nullptr;
    if (sqlite3_open(sourcePath.c_str(), &sourceDb) != SQLITE_OK) {
        error = sqliteError(sourceDb);
        if (sourceDb != nullptr) {
            sqlite3_close(sourceDb);
        }
        return false;
    }

    sqlite3_backup* backup = sqlite3_backup_init(db_, "main", sourceDb, "main");
    if (backup == nullptr) {
        error = sqliteError(db_);
        sqlite3_close(sourceDb);
        return false;
    }

    const int stepResult = sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
    sqlite3_close(sourceDb);
    if (stepResult != SQLITE_DONE) {
        error = sqliteError(db_);
        return false;
    }

    if (!ensureSchema(error)) {
        return false;
    }
    return true;
}

bool StudyService::exportSqliteDatabase(const std::string& targetPath, std::string& error) const {
    std::remove(targetPath.c_str());

    sqlite3* targetDb = nullptr;
    if (sqlite3_open(targetPath.c_str(), &targetDb) != SQLITE_OK) {
        error = sqliteError(targetDb);
        if (targetDb != nullptr) {
            sqlite3_close(targetDb);
        }
        return false;
    }

    sqlite3_backup* backup = sqlite3_backup_init(targetDb, "main", db_, "main");
    if (backup == nullptr) {
        error = sqliteError(targetDb);
        sqlite3_close(targetDb);
        return false;
    }

    const int stepResult = sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
    sqlite3_close(targetDb);
    if (stepResult != SQLITE_DONE) {
        error = sqliteError(db_);
        return false;
    }
    return true;
}

StudyService::QuestionRecord StudyService::recordFromStatement(sqlite3_stmt* statement) {
    auto textAt = [statement](int column) -> std::string {
        const auto* value = sqlite3_column_text(statement, column);
        return value == nullptr ? "" : reinterpret_cast<const char*>(value);
    };

    QuestionRecord record;
    record.question.id = sqlite3_column_int(statement, 0);
    record.question.subject = textAt(1);
    record.question.topic = textAt(2);
    record.question.prompt = textAt(3);
    record.question.answer = textAt(4);
    record.question.explanation = textAt(5);
    record.question.imageName = textAt(6);
    if (sqlite3_column_type(statement, 7) != SQLITE_NULL) {
        const auto* blob =
            reinterpret_cast<const unsigned char*>(sqlite3_column_blob(statement, 7));
        const int size = sqlite3_column_bytes(statement, 7);
        if (blob != nullptr && size >= 0) {
            record.question.imageData.assign(blob, blob + size);
        }
    }
    record.question.tags = splitTags(textAt(8));
    record.question.difficulty = sqlite3_column_int(statement, 9);
    record.stats.questionId = record.question.id;
    record.stats.lastReviewed =
        std::chrono::system_clock::time_point(std::chrono::seconds(sqlite3_column_int64(statement, 10)));
    record.stats.intervalDays = sqlite3_column_int(statement, 11);
    record.stats.easeFactor = sqlite3_column_double(statement, 12);
    record.stats.streak = sqlite3_column_int(statement, 13);
    record.stats.totalReviews = sqlite3_column_int(statement, 14);
    record.stats.correctReviews = sqlite3_column_int(statement, 15);
    record.stats.wrongReviews = sqlite3_column_int(statement, 16);
    record.stats.pendingMistakes = sqlite3_column_int(statement, 17);
    return record;
}

std::string StudyService::legacyTextPathFor(const std::string& databasePath) {
    const auto dot = databasePath.find_last_of('.');
    if (dot == std::string::npos) {
        return databasePath + ".txt";
    }
    return databasePath.substr(0, dot) + ".txt";
}

bool StudyService::fileExists(const std::string& path) {
    std::ifstream input(std::filesystem::u8path(path));
    return input.good();
}

bool StudyService::looksLikeLegacyTextFile(const std::string& path) {
    if (extensionOf(path) == ".txt") {
        return true;
    }

    std::ifstream input(std::filesystem::u8path(path));
    if (!input.is_open()) {
        return false;
    }

    std::string header;
    std::getline(input, header);
    return header == kLegacyHeaderV1 || header == kLegacyHeaderV2 || header == kLegacyHeaderV3 ||
           header == kLegacyHeaderV4;
}

std::string StudyService::joinTags(const std::vector<std::string>& tags) {
    std::ostringstream stream;
    for (std::size_t index = 0; index < tags.size(); ++index) {
        if (index > 0) {
            stream << ',';
        }
        stream << tags[index];
    }
    return stream.str();
}

std::string StudyService::escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '\t':
                escaped += "\\t";
                break;
            case '\n':
                escaped += "\\n";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }
    return escaped;
}

std::string StudyService::unescape(const std::string& value) {
    std::string unescaped;
    unescaped.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (value[index] == '\\' && index + 1 < value.size()) {
            const char next = value[index + 1];
            switch (next) {
                case '\\':
                    unescaped.push_back('\\');
                    ++index;
                    continue;
                case 't':
                    unescaped.push_back('\t');
                    ++index;
                    continue;
                case 'n':
                    unescaped.push_back('\n');
                    ++index;
                    continue;
                default:
                    break;
            }
        }
        unescaped.push_back(value[index]);
    }
    return unescaped;
}

std::string StudyService::timePointToString(std::chrono::system_clock::time_point tp) {
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
    return std::to_string(seconds);
}

std::chrono::system_clock::time_point StudyService::stringToTimePoint(const std::string& text) {
    if (trim(text).empty()) {
        return {};
    }
    const auto seconds = std::stoll(text);
    return std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
}

double StudyService::computeScore(const QuestionRecord& record) const {
    const auto now = std::chrono::system_clock::now();
    const auto elapsedHours =
        std::chrono::duration_cast<std::chrono::hours>(now - record.stats.lastReviewed).count();
    const double elapsedDays = static_cast<double>(elapsedHours) / 24.0;
    const double dueRatio = elapsedDays / std::max(1, record.stats.intervalDays);
    const double difficultyBoost = 1.0 + (record.question.difficulty - 1) * 0.15;
    const double streakPenalty = 1.0 / (1.0 + record.stats.streak * 0.2);
    return dueRatio * difficultyBoost * streakPenalty;
}

bool StudyService::satisfiesFilter(const QuestionRecord& record, const StudyFilter& filter) const {
    if (!filter.subject.empty() && record.question.subject != filter.subject) {
        return false;
    }
    if (!filter.topic.empty() && record.question.topic != filter.topic) {
        return false;
    }
    if (record.question.difficulty < filter.minDifficulty || record.question.difficulty > filter.maxDifficulty) {
        return false;
    }
    return containsAllTags(record.question.tags, filter.requiredTags);
}

bool StudyService::validateQuestion(const std::string& subject,
                                    const std::string& topic,
                                    const std::string& prompt,
                                    const std::string& answer,
                                    int difficulty,
                                    std::string& error) const {
    if (trim(subject).empty()) {
        error = "Subject cannot be empty.";
        return false;
    }
    if (trim(topic).empty()) {
        error = "Topic cannot be empty.";
        return false;
    }
    if (trim(prompt).empty()) {
        error = "Prompt cannot be empty.";
        return false;
    }
    if (trim(answer).empty()) {
        error = "Answer cannot be empty.";
        return false;
    }
    if (difficulty < 1 || difficulty > 5) {
        error = "Difficulty must be between 1 and 5.";
        return false;
    }
    return true;
}

void StudyService::seedDemoData() {
    std::string error;
    createQuestion("Operating Systems",
                   "Process Scheduling",
                   "What is the difference between a process and a thread?",
                   "A process owns resources independently, while threads share a process address space.",
                   "Processes are isolated execution units; threads are lightweight execution paths inside a process.",
                   "",
                   {},
                   {"os", "process", "thread"},
                   2,
                   error);
    createQuestion("Data Structures",
                   "Binary Tree",
                   "Why can an inorder traversal of a BST produce a sorted sequence?",
                   "Because BST nodes satisfy left < root < right recursively.",
                   "The BST ordering property guarantees that inorder visits values in nondecreasing order.",
                   "",
                   {},
                   {"tree", "bst", "traversal"},
                   2,
                   error);
    createQuestion("Computer Networks",
                   "Transport Layer",
                   "Why does TCP need a three-way handshake?",
                   "It confirms both sides can send and receive while synchronizing initial sequence numbers.",
                   "Without the third step, one side cannot be sure the other received its starting sequence information.",
                   "",
                   {},
                   {"network", "tcp"},
                   3,
                   error);
    createQuestion("Operating Systems",
                   "Memory Management",
                   "What problem does paging solve?",
                   "Paging avoids the need for contiguous physical memory and reduces external fragmentation.",
                   "By splitting memory into fixed-size pages and frames, allocation becomes more flexible.",
                   "",
                   {},
                   {"os", "memory", "paging"},
                   4,
                   error);
}
