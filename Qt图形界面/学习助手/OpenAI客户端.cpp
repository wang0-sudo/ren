#include "OpenAI客户端.h"

#include <memory>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>

namespace {
constexpr const char* kDefaultResponsesUrl = "https://api.openai.com/v1/responses";
constexpr const char* kDefaultModel = "gpt-5-mini";

QString readJsonErrorMessage(const QJsonObject& object) {
    const QJsonValue errorValue = object.value("error");
    if (!errorValue.isObject()) {
        return {};
    }

    const QString message = errorValue.toObject().value("message").toString().trimmed();
    return message;
}

QString readOutputText(const QJsonObject& object) {
    const QString directText = object.value("output_text").toString().trimmed();
    if (!directText.isEmpty()) {
        return directText;
    }

    QStringList parts;
    const QJsonArray outputArray = object.value("output").toArray();
    for (const QJsonValue& outputValue : outputArray) {
        const QJsonObject outputObject = outputValue.toObject();
        const QJsonArray contentArray = outputObject.value("content").toArray();
        for (const QJsonValue& contentValue : contentArray) {
            const QJsonObject contentObject = contentValue.toObject();
            const QString type = contentObject.value("type").toString();
            if (type != "output_text" && type != "text") {
                continue;
            }

            const QString text = contentObject.value("text").toString().trimmed();
            if (!text.isEmpty()) {
                parts << text;
            }
        }
    }

    return parts.join("\n\n");
}

QString joinTags(const std::vector<std::string>& tags) {
    QStringList parts;
    for (const auto& tag : tags) {
        parts << QString::fromStdString(tag);
    }
    return parts.join(", ");
}
}  // namespace

OpenAiClient::OpenAiClient()
    : manager_(new QNetworkAccessManager()) {}

OpenAiClient::~OpenAiClient() {
    delete manager_;
    manager_ = nullptr;
}

bool OpenAiClient::isConfigured(QString* error) const {
    if (!apiKey().isEmpty()) {
        return true;
    }

    if (error != nullptr) {
        *error =
            "OPENAI_API_KEY is not set. Start Qt Creator or the app from a shell where OPENAI_API_KEY is available.";
    }
    return false;
}

QString OpenAiClient::configuredModel() const {
    const QString model = QString::fromUtf8(qgetenv("OPENAI_MODEL")).trimmed();
    return model.isEmpty() ? QString::fromUtf8(kDefaultModel) : model;
}

void OpenAiClient::requestWrongQuestionAnalysis(const QuestionProgress& progress, CompletionHandler handler) {
    QString configError;
    if (!isConfigured(&configError)) {
        handler(Result{QString(), configError});
        return;
    }

    QNetworkRequest request{QUrl{responsesUrl()}};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + apiKey().toUtf8());

    QJsonObject payload;
    payload.insert("model", configuredModel());
    payload.insert("instructions",
                   QString::fromUtf8("You are a patient computer science study coach. "
                                     "Answer in Simplified Chinese. Focus on correcting misconceptions, "
                                     "teaching the concept clearly, and giving one follow-up practice question."));
    payload.insert("input", buildPrompt(progress));

    QNetworkReply* reply = manager_->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, handler = std::move(handler)]() mutable {
        std::unique_ptr<QNetworkReply, void (*)(QNetworkReply*)> guard(reply, [](QNetworkReply* value) {
            value->deleteLater();
        });

        Result result;
        const QByteArray responseBytes = reply->readAll();

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(responseBytes, &parseError);

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            QString apiMessage;
            if (parseError.error == QJsonParseError::NoError && document.isObject()) {
                apiMessage = readJsonErrorMessage(document.object());
            }

            result.error = apiMessage.isEmpty()
                               ? QString("OpenAI request failed (%1): %2").arg(statusCode).arg(reply->errorString())
                               : QString("OpenAI request failed (%1): %2").arg(statusCode).arg(apiMessage);
            handler(result);
            return;
        }

        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            result.error = "OpenAI returned an invalid JSON response.";
            handler(result);
            return;
        }

        const QJsonObject object = document.object();
        const QString apiMessage = readJsonErrorMessage(object);
        if (!apiMessage.isEmpty()) {
            result.error = QString("OpenAI request failed: %1").arg(apiMessage);
            handler(result);
            return;
        }

        result.text = readOutputText(object);
        if (result.text.trimmed().isEmpty()) {
            result.error = "OpenAI returned no text output.";
        }
        handler(result);
    });
}

QString OpenAiClient::apiKey() const {
    return QString::fromUtf8(qgetenv("OPENAI_API_KEY")).trimmed();
}

QString OpenAiClient::responsesUrl() const {
    const QString overrideUrl = QString::fromUtf8(qgetenv("OPENAI_RESPONSES_URL")).trimmed();
    return overrideUrl.isEmpty() ? QString::fromUtf8(kDefaultResponsesUrl) : overrideUrl;
}

QString OpenAiClient::buildPrompt(const QuestionProgress& progress) const {
    const QString tags = progress.question.tags.empty() ? QString("none") : joinTags(progress.question.tags);

    QString prompt = QString::fromUtf8(
                         "请用简体中文分析这道计算机专业学习题，并严格按下面结构输出：\n"
                         "1. 错因分析\n"
                         "2. 核心知识点\n"
                         "3. 记忆方法\n"
                         "4. 解题步骤\n"
                         "5. 一道同类型练习题\n"
                         "6. 练习题参考答案\n\n"
                         "题目编号：%1\n"
                         "科目：%2\n"
                         "主题：%3\n"
                         "难度：%4/5\n"
                         "累计错题次数：%5\n"
                         "总复习次数：%6\n"
                         "标签：%7\n"
                         "题目：%8\n"
                         "标准答案：%9\n"
                         "已有解释：%10\n")
                         .arg(progress.question.id)
                         .arg(QString::fromStdString(progress.question.subject))
                         .arg(QString::fromStdString(progress.question.topic))
                         .arg(progress.question.difficulty)
                         .arg(progress.stats.pendingMistakes)
                         .arg(progress.stats.totalReviews)
                         .arg(tags)
                         .arg(QString::fromStdString(progress.question.prompt))
                         .arg(QString::fromStdString(progress.question.answer))
                         .arg(QString::fromStdString(progress.question.explanation));

    if (!progress.question.imageName.empty()) {
        prompt += QString::fromUtf8("附图文件名：%1\n").arg(QString::fromStdString(progress.question.imageName));
        prompt += QString::fromUtf8("注意：当前请求未直接附带图片内容，请不要凭空猜测图片细节。\n");
    }

    return prompt;
}
