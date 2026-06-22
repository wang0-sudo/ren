#pragma once

#include <functional>

#include <QString>

#include "学习服务.h"

class QNetworkAccessManager;

class OpenAiClient {
public:
    struct Result {
        QString text;
        QString error;
    };

    using CompletionHandler = std::function<void(const Result&)>;

    OpenAiClient();
    ~OpenAiClient();

    OpenAiClient(const OpenAiClient&) = delete;
    OpenAiClient& operator=(const OpenAiClient&) = delete;

    bool isConfigured(QString* error = nullptr) const;
    QString configuredModel() const;
    void requestWrongQuestionAnalysis(const QuestionProgress& progress, CompletionHandler handler);

private:
    QString apiKey() const;
    QString responsesUrl() const;
    QString buildPrompt(const QuestionProgress& progress) const;

    QNetworkAccessManager* manager_ = nullptr;
};
