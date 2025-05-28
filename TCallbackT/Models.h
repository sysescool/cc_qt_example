#ifndef MODELS_H
#define MODELS_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QString>
#include <QVariant>

namespace Models {

// 通用请求结构
struct Request {
    QString functionName;  // n
    QJsonValue payload;    // p
    int sequence;          // s
    
    static Request fromJson(const QJsonObject& json) {
        Request req;
        req.functionName = json["n"].toString();
        req.payload = json["p"];
        req.sequence = json["s"].toInt();
        return req;
    }
};

// 通用响应结构
struct Response {
    int statusCode = 200;       // c
    QString error;              // e
    QString errorReason;        // er
    QJsonValue result;          // r
    int sequence = 0;           // s
    
    QJsonObject toJson() const {
        QJsonObject json;
        json["c"] = statusCode;
        json["e"] = error.isEmpty() ? QJsonValue() : QJsonValue(error);
        json["er"] = errorReason.isEmpty() ? QJsonValue() : QJsonValue(errorReason);
        json["r"] = result;
        json["s"] = sequence;
        return json;
    }
};

// 具体业务模型示例 - 读取文件
struct ReadFileRequest {
    static constexpr const char* functionName = "rf";
    
    QString filePath;
    
    static ReadFileRequest fromPayload(const QJsonValue& payload) {
        ReadFileRequest req;
        req.filePath = payload.toString();
        return req;
    }
};

struct ReadFileResponse {
    QString content;
    
    QJsonValue toJsonValue() const {
        QJsonObject obj;
        obj["content"] = content;
        return obj;
    }
};

} // namespace Models

#endif // MODELS_H
