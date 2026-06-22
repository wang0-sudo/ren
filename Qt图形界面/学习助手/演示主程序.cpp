#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

#include "学习服务.h"

namespace {
std::string readLine(const std::string& label) {
    std::cout << label;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

int readInt(const std::string& label) {
    while (true) {
        std::cout << label;
        int value = 0;
        if (std::cin >> value) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }

        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a valid integer.\n";
    }
}

void printTags(const std::vector<std::string>& tags) {
    if (tags.empty()) {
        std::cout << "(none)";
        return;
    }

    for (std::size_t index = 0; index < tags.size(); ++index) {
        if (index > 0) {
            std::cout << ", ";
        }
        std::cout << tags[index];
    }
}

void printQuestion(const Question& question) {
    std::cout << "[" << question.id << "] " << question.subject << " / " << question.topic << '\n';
    std::cout << "Difficulty: " << question.difficulty << '\n';
    std::cout << "Tags: ";
    printTags(question.tags);
    std::cout << '\n';
    if (question.imageData.empty()) {
        std::cout << "Image: " << (question.imageName.empty() ? "(none)" : question.imageName + " (metadata only)")
                  << '\n';
    } else {
        std::cout << "Image: "
                  << (question.imageName.empty() ? "(embedded image)" : question.imageName)
                  << " | bytes: " << question.imageData.size() << '\n';
    }
    std::cout << "Prompt: " << question.prompt << '\n';
    std::cout << "Answer: " << question.answer << '\n';
    std::cout << "Explanation: " << question.explanation << "\n\n";
}

void printStats(const ReviewStats& stats) {
    std::cout << "Reviews: " << stats.totalReviews
              << " | Correct: " << stats.correctReviews
              << " | Wrong: " << stats.wrongReviews
              << " | Pending mistakes: " << stats.pendingMistakes << '\n';
    std::cout << "Streak: " << stats.streak
              << " | Interval days: " << stats.intervalDays
              << " | Ease factor: " << std::fixed << std::setprecision(2) << stats.easeFactor << '\n';
}

void printProgress(const QuestionProgress& progress) {
    printQuestion(progress.question);
    printStats(progress.stats);
    std::cout << "Priority score: " << std::fixed << std::setprecision(2) << progress.score << "\n\n";
}

StudyFilter readFilter() {
    StudyFilter filter;
    filter.subject = readLine("Subject (blank for all): ");
    filter.topic = readLine("Topic (blank for all): ");
    filter.requiredTags = StudyService::splitTags(readLine("Required tags, comma separated (blank for all): "));
    filter.minDifficulty = readInt("Min difficulty (1-5): ");
    filter.maxDifficulty = readInt("Max difficulty (1-5): ");

    if (filter.minDifficulty < 1) {
        filter.minDifficulty = 1;
    }
    if (filter.maxDifficulty > 5) {
        filter.maxDifficulty = 5;
    }
    if (filter.minDifficulty > filter.maxDifficulty) {
        std::swap(filter.minDifficulty, filter.maxDifficulty);
    }
    return filter;
}

void printMenu() {
    std::cout << "Study Assistant Demo\n";
    std::cout << "1. List all questions\n";
    std::cout << "2. Recommend next question\n";
    std::cout << "3. Add a question\n";
    std::cout << "4. Mark review result\n";
    std::cout << "5. Delete a question\n";
    std::cout << "6. Show wrong-question book\n";
    std::cout << "7. Filter questions\n";
    std::cout << "8. Export data\n";
    std::cout << "9. Import data\n";
    std::cout << "0. Exit\n";
    std::cout << "Select: ";
}
}  // namespace

int main() {
    StudyService service("学习助手数据.db");
    service.loadOrSeed();

    while (true) {
        printMenu();
        int choice = 0;
        if (!(std::cin >> choice)) {
            return 0;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << '\n';

        if (choice == 1) {
            const auto questions = service.questions();
            std::cout << "Total questions: " << questions.size() << "\n\n";
            for (const auto& question : questions) {
                printQuestion(question);
            }
            continue;
        }

        if (choice == 2) {
            const auto recommendation = service.recommendNext();
            if (!recommendation.has_value()) {
                std::cout << "No question available.\n\n";
                continue;
            }

            QuestionProgress progress{recommendation->question, recommendation->stats, recommendation->score};
            printProgress(progress);
            continue;
        }

        if (choice == 3) {
            const std::string subject = readLine("Subject: ");
            const std::string topic = readLine("Topic: ");
            const std::string prompt = readLine("Prompt: ");
            const std::string answer = readLine("Answer: ");
            const std::string explanation = readLine("Explanation: ");
            const std::string imagePath = readLine("Image file path to embed (optional): ");
            const std::string tagsLine = readLine("Tags (comma separated): ");
            const int difficulty = readInt("Difficulty (1-5): ");

            std::string error;
            std::string imageName;
            std::vector<unsigned char> imageData;
            if (!StudyService::loadImageFromFile(imagePath, imageName, imageData, error)) {
                std::cout << "Failed: " << error << "\n\n";
                continue;
            }
            const int id = service.createQuestion(subject,
                                                  topic,
                                                  prompt,
                                                  answer,
                                                  explanation,
                                                  imageName,
                                                  imageData,
                                                  StudyService::splitTags(tagsLine),
                                                  difficulty,
                                                  error);
            if (id == 0) {
                std::cout << "Failed: " << error << "\n\n";
            } else {
                std::cout << "Created question #" << id << "\n\n";
            }
            continue;
        }

        if (choice == 4) {
            const int questionId = readInt("Question id: ");
            const int correctFlag = readInt("Correct? (1=yes, 0=no): ");

            std::string error;
            if (!service.recordReview(questionId, correctFlag == 1, error)) {
                std::cout << "Failed: " << error << "\n\n";
                continue;
            }

            std::cout << "Review recorded.\n";
            const auto progress = service.questionProgress(questionId);
            if (progress.has_value()) {
                printStats(progress->stats);
                std::cout << '\n';
            }
            continue;
        }

        if (choice == 5) {
            const int questionId = readInt("Question id to delete: ");
            std::string error;
            if (!service.removeQuestion(questionId, error)) {
                std::cout << "Failed: " << error << "\n\n";
            } else {
                std::cout << "Question removed.\n\n";
            }
            continue;
        }

        if (choice == 6) {
            const auto wrongBook = service.wrongQuestions();
            if (wrongBook.empty()) {
                std::cout << "Wrong-question book is empty.\n\n";
                continue;
            }

            std::cout << "Wrong-question book size: " << wrongBook.size() << "\n\n";
            for (const auto& progress : wrongBook) {
                printProgress(progress);
            }
            continue;
        }

        if (choice == 7) {
            const StudyFilter filter = readFilter();
            const auto questions = service.questionsMatching(filter);
            std::cout << "\nMatched questions: " << questions.size() << "\n\n";
            for (const auto& question : questions) {
                printQuestion(question);
            }
            continue;
        }

        if (choice == 8) {
            const std::string targetPath = readLine("Export path: ");
            std::string error;
            if (!service.exportToFile(targetPath, error)) {
                std::cout << "Failed: " << error << "\n\n";
            } else {
                std::cout << "Exported to " << targetPath << "\n\n";
            }
            continue;
        }

        if (choice == 9) {
            const std::string sourcePath = readLine("Import path: ");
            std::string error;
            if (!service.importFromFile(sourcePath, error)) {
                std::cout << "Failed: " << error << "\n\n";
            } else {
                std::cout << "Imported data successfully.\n\n";
            }
            continue;
        }

        if (choice == 0) {
            break;
        }

        std::cout << "Unknown selection.\n\n";
    }

    return 0;
}
