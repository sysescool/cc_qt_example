#include "TCoreSession.h"
#include <QDebug>
#include <QJsonParseError>

TCoreSession::TCoreSession(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
{
    connect(m_webSocket, &QWebSocket::connected, this, &TCoreSession::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &TCoreSession::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &TCoreSession::onTextMessageReceived);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &TCoreSession::onError);
}

TCoreSession::~TCoreSession()
{
    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }
}

void TCoreSession::connectToServer(const QString& url)
{
    qDebug() << "正在连接到服务器:" << url;
    m_webSocket->open(QUrl(url));
}

void TCoreSession::disconnect()
{
    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }
}

void TCoreSession::onConnected()
{
    qDebug() << "已连接到 PaaSServer";
}

void TCoreSession::onDisconnected()
{
    qDebug() << "已断开与 PaaSServer 的连接";
}

void TCoreSession::onTextMessageReceived(const QString& message)
{
    qDebug() << "收到消息:" << message;
    
    // 解析 JSON 消息
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON 解析错误:" << parseError.errorString();
        return;
    }
    
    if (!doc.isObject()) {
        qWarning() << "消息不是 JSON 对象";
        return;
    }
    
    // 转换为请求模型
    Models::Request request = Models::Request::fromJson(doc.object());
    
    // 处理请求
    handleRequest(request);
}

void TCoreSession::onError(QAbstractSocket::SocketError error)
{
    qWarning() << "WebSocket 错误:" << m_webSocket->errorString();
}

void TCoreSession::sendResponse(const Models::Response& response)
{
    QJsonDocument doc(response.toJson());
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "发送响应:" << jsonString;
    m_webSocket->sendTextMessage(jsonString);
}

void TCoreSession::handleRequest(const Models::Request& request)
{
    auto it = m_callbacks.find(request.functionName);
    
    if (it == m_callbacks.end()) {
        // 未找到对应的回调函数
        Models::Response errorResponse;
        errorResponse.statusCode = 404;
        errorResponse.error = "Function not found";
        errorResponse.errorReason = QString("No callback registered for function: %1").arg(request.functionName);
        errorResponse.sequence = request.sequence;
        
        sendResponse(errorResponse);
        return;
    }
    
    // 调用回调函数
    Models::Response response = it->second(request.sequence, request.payload);
    
    // 发送响应
    sendResponse(response);
}
