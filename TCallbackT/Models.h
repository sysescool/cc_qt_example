#ifndef MODELS_H
#define MODELS_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

// =================== 基础模板类 ===================

// 请求基类模板
template<typename Derived, const char* FuncName>
class RequestBase {
public:
    static constexpr const char* functionName = FuncName;
    
    static Derived fromPayload(const QJsonValue& payload) {
        return Derived::parseFromPayload(payload);
    }
};

// 响应基类模板
template<typename Derived>
class ResponseBase {
public:
    QJsonValue toJsonValue() const {
        return static_cast<const Derived*>(this)->serialize();
    }
};

// =================== 具体业务模型 ===================

// 函数名常量定义
constexpr const char READ_file[] = "rf";
constexpr const char write_file[] = "wf";
constexpr const char list_directory[] = "ld";
constexpr const char execute_command[] = "ec";
constexpr const char get_system_info[] = "gsi";

// 1. 读取文件
class ReadFileRequest : public RequestBase<ReadFileRequest, READ_file>
{
public:
    QString filePath;
    
    static ReadFileRequest parseFromPayload(const QJsonValue& payload) {
        ReadFileRequest req;
        req.filePath = payload.toString();
        return req;
    }
};

class ReadFileResponse : public ResponseBase<ReadFileResponse> {
public:
    QString content;
    
    QJsonValue serialize() const {
        QJsonObject obj;
        obj["content"] = content;
        return obj;
    }
};

// 2. 写入文件
class WriteFileRequest : public RequestBase<WriteFileRequest, write_file> {
public:
    QString filePath;
    QString content;
    bool append = false;
    
    static WriteFileRequest parseFromPayload(const QJsonValue& payload) {
        WriteFileRequest req;
        QJsonObject obj = payload.toObject();
        req.filePath = obj["path"].toString();
        req.content = obj["content"].toString();
        req.append = obj["append"].toBool(false);
        return req;
    }
};

class WriteFileResponse : public ResponseBase<WriteFileResponse> {
public:
    QString message;
    int bytesWritten = 0;
    
    QJsonValue serialize() const {
        QJsonObject obj;
        obj["message"] = message;
        obj["bytesWritten"] = bytesWritten;
        return obj;
    }
};

// 3. 列出目录
class ListDirectoryRequest : public RequestBase<ListDirectoryRequest, list_directory> {
public:
    QString directoryPath;
    bool includeHidden = false;
    
    static ListDirectoryRequest parseFromPayload(const QJsonValue& payload) {
        ListDirectoryRequest req;
        if (payload.isString()) {
            req.directoryPath = payload.toString();
        } else {
            QJsonObject obj = payload.toObject();
            req.directoryPath = obj["path"].toString();
            req.includeHidden = obj["includeHidden"].toBool(false);
        }
        return req;
    }
};

class ListDirectoryResponse : public ResponseBase<ListDirectoryResponse> {
public:
    struct FileInfo {
        QString name;
        QString type;  // "file" or "directory"
        qint64 size;
        QString lastModified;
    };
    
    QList<FileInfo> files;
    
    QJsonValue serialize() const {
        QJsonObject obj;
        QJsonArray filesArray;
        
        for (const auto& file : files) {
            QJsonObject fileObj;
            fileObj["name"] = file.name;
            fileObj["type"] = file.type;
            fileObj["size"] = file.size;
            fileObj["lastModified"] = file.lastModified;
            filesArray.append(fileObj);
        }
        
        obj["files"] = filesArray;
        return obj;
    }
};

// 4. 执行命令
class ExecuteCommandRequest : public RequestBase<ExecuteCommandRequest, execute_command> {
public:
    QString command;
    QStringList arguments;
    QString workingDirectory;
    
    static ExecuteCommandRequest parseFromPayload(const QJsonValue& payload) {
        ExecuteCommandRequest req;
        QJsonObject obj = payload.toObject();
        req.command = obj["command"].toString();
        req.workingDirectory = obj["workingDirectory"].toString();
        
        QJsonArray argsArray = obj["arguments"].toArray();
        for (const auto& arg : argsArray) {
            req.arguments.append(arg.toString());
        }
        
        return req;
    }
};

// class ExecuteCommandResponse : public ResponseBase<ExecuteCommandResponse> {
// public:
//     QString stdout;
//     QString stderr;
//     int exitCode = 0;

//     QJsonValue serialize() const {
//         QJsonObject obj;
//         obj["stdout"] = stdout;
//         obj["stderr"] = stderr;
//         obj["exitCode"] = exitCode;
//         return obj;
//     }
// };

// 5. 获取系统信息
class GetSystemInfoRequest : public RequestBase<GetSystemInfoRequest, get_system_info> {
public:
    QStringList requestedInfo; // 可以指定需要哪些信息，如 ["os", "cpu", "memory"]
    
    static GetSystemInfoRequest parseFromPayload(const QJsonValue& payload) {
        GetSystemInfoRequest req;
        if (payload.isArray()) {
            QJsonArray array = payload.toArray();
            for (const auto& item : array) {
                req.requestedInfo.append(item.toString());
            }
        } else if (payload.isNull()) {
            // 如果 payload 为 null，返回所有信息
            req.requestedInfo = {"os", "cpu", "memory", "disk"};
        }
        return req;
    }
};

class GetSystemInfoResponse : public ResponseBase<GetSystemInfoResponse> {
public:
    struct SystemInfo {
        QString osName;
        QString osVersion;
        QString cpuInfo;
        qint64 totalMemory;
        qint64 availableMemory;
        qint64 totalDisk;
        qint64 availableDisk;
    } systemInfo;
    
    QJsonValue serialize() const {
        QJsonObject obj;
        obj["osName"] = systemInfo.osName;
        obj["osVersion"] = systemInfo.osVersion;
        obj["cpuInfo"] = systemInfo.cpuInfo;
        obj["totalMemory"] = systemInfo.totalMemory;
        obj["availableMemory"] = systemInfo.availableMemory;
        obj["totalDisk"] = systemInfo.totalDisk;
        obj["availableDisk"] = systemInfo.availableDisk;
        return obj;
    }
};

// =================== 辅助宏（可选使用）===================

// 简化请求类定义的宏
#define DEFINE_SIMPLE_REQUEST(ClassName, FuncName, PayloadType) \
    constexpr const char ClassName##_func[] = FuncName; \
    class ClassName : public RequestBase<ClassName, ClassName##_func> { \
    public: \
        PayloadType data; \
        static ClassName parseFromPayload(const QJsonValue& payload) { \
            ClassName req; \
            req.data = payload.to##PayloadType(); \
            return req; \
        } \
    };

// 简化响应类定义的宏
#define DEFINE_SIMPLE_RESPONSE(ClassName, FieldName, FieldType) \
    class ClassName : public ResponseBase<ClassName> { \
    public: \
        FieldType FieldName; \
        QJsonValue serialize() const { \
            QJsonObject obj; \
            obj[#FieldName] = FieldName; \
            return obj; \
        } \
    };

} // namespace Models

#endif // MODELS_H
