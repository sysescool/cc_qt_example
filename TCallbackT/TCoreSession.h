#ifndef TCORESESSION_H
#define TCORESESSION_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <functional>
#include <unordered_map>
#include "Models.h"

class TCoreSession : public QObject
{
    Q_OBJECT

public:
    explicit TCoreSession(QObject *parent = nullptr);
    ~TCoreSession();
    
    // 连接到服务器
    void connectToServer(const QString& url);
    
    // 断开连接
    void disconnect();
    
    // 模板回调函数类型定义
    template<typename PayloadType>
    using CallbackFunction = std::function<Models::Response(int sequence, const PayloadType& payload)>;
    
    // 注册回调函数 - 自动从 PayloadType 获取 functionName
    template<typename PayloadType>
    void registerCallback(CallbackFunction<PayloadType> callback);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);

private:
    // 内部回调函数类型
    using InternalCallback = std::function<Models::Response(int sequence, const QJsonValue& payload)>;
    
    // 发送响应
    void sendResponse(const Models::Response& response);
    
    // 处理收到的请求
    void handleRequest(const Models::Request& request);
    
private:
    QWebSocket* m_webSocket;
    std::unordered_map<QString, InternalCallback> m_callbacks;
};

// 模板函数实现
template<typename PayloadType>
void TCoreSession::registerCallback(CallbackFunction<PayloadType> callback)
{
    // 从 PayloadType 获取 functionName
    QString functionName = PayloadType::functionName;
    
    // 包装用户回调函数，处理 JSON 到具体类型的转换
    InternalCallback internalCallback = [callback](int sequence, const QJsonValue& payload) -> Models::Response {
        try {
            // 将 JSON payload 转换为具体类型
            PayloadType typedPayload = PayloadType::fromPayload(payload);
            
            // 调用用户回调函数
            return callback(sequence, typedPayload);
        } catch (const std::exception& e) {
            // 转换失败，返回错误响应
            Models::Response errorResponse;
            errorResponse.statusCode = 400;
            errorResponse.error = "Payload conversion failed";
            errorResponse.errorReason = QString::fromStdString(e.what());
            errorResponse.sequence = sequence;
            return errorResponse;
        }
    };
    
    m_callbacks[functionName] = internalCallback;
}

#endif // TCORESESSION_H
