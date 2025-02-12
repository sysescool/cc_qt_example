#include <QCoreApplication>
#include <QFile>
#include <QJsonObject>
#include <QMimeDatabase>

#include "simplehttpserver.h"


// 手动将其定义为 -1，不使用 ssl 特性。
#ifndef SIMPLE_HTTP_SERVER_SSL
#undef QT_FEATURE_ssl
#define QT_FEATURE_ssl -1
#endif

#if QT_CONFIG(ssl)
#  include <QSslCertificate>
#  include <QSslKey>
#  include <QSslServer>
#endif

// 初始化静态成员变量
QScopedPointer<SimpleHttpServer> SimpleHttpServer::m_instance;
QMutex SimpleHttpServer::m_mutex;
QByteArray SimpleHttpServer::faviconData;

// invalid literal suffix '_s'; literal operator or literal operator template 'operator ""_s' not found
// 这是 cpp17 引入的 用户定义字面量 (User-Defined Literals)
using namespace Qt::StringLiterals;  // 这里用的是 Qt 的字符串字面量

static inline QString host(const QHttpServerRequest &request)
{
    return QString::fromLatin1(request.value("Host"));
}

SimpleHttpServer::SimpleHttpServer(QObject *parent)
    :QObject(parent)
{
    SimpleHttpServer::loadFavicon();
    tcpserver.reset(new QTcpServer(this));
    registerAllTestPage();
    registerIcon();
    registerRoot();
    registerPathArgsTest();
    registerUserTest();
    registerJsonTest();
    registerOtherTest();
    registerAuthTest();
}

SimpleHttpServer* SimpleHttpServer::instance()
{
    // 双重检查锁定，确保线程安全
    if (m_instance.isNull()) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.isNull()) {
            m_instance.reset(new SimpleHttpServer());
        }
    }
    return m_instance.data();
}

SimpleHttpServer* SimpleHttpServer::setPort(quint16 port)
{
    this->port = port;
    return this;
}

bool SimpleHttpServer::start()
{
    // 第一步：尝试固定端口
    if (tcpserver->listen(QHostAddress::Any, port)) {
        qInfo() << "Using fixed port:" << port;
    }
    // 第二步：固定端口失败时尝试随机端口
    // else if (tcpserver->listen()) {
    //     qWarning() << "Fixed port" << port << "failed, using random port:" << tcpserver->serverPort();
    // }
    // 第三步：两个方案都失败
    else {
        qCritical() << "Listen attempts failed:" << tcpserver->errorString();
        return false;
    }

    // 绑定检查（无论固定还是随机端口都需要执行）
    if (!httpServer.bind(tcpserver.get())) {
        qCritical() << "HTTP server bind failed";
        return false;
    }

    tcpserver.take(); // 释放所有权给 httpserver

#if QT_CONFIG(ssl)
    QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
    const auto sslCertificateChain =
        QSslCertificate::fromPath(QStringLiteral(":/assets/certificate.crt"));
    if (sslCertificateChain.empty()) {
        qWarning() << QCoreApplication::translate("QHttpServerExample",
                                                  "Couldn't retrieve SSL certificate from file.");
        return -1;
    }
    QFile privateKeyFile(QStringLiteral(":/assets/private.key"));
    if (!privateKeyFile.open(QIODevice::ReadOnly)) {
        qWarning() << QCoreApplication::translate("QHttpServerExample",
                                                  "Couldn't open file for reading: %1")
                          .arg(privateKeyFile.errorString());
        return -1;
    }

    conf.setLocalCertificate(sslCertificateChain.front());
    conf.setPrivateKey(QSslKey(&privateKeyFile, QSsl::Rsa));

    privateKeyFile.close();

    auto sslserver = std::make_unique<QSslServer>();
    sslserver->setSslConfiguration(conf);
    if (!sslserver->listen() || !httpServer.bind(sslserver.get())) {
        qWarning() << QCoreApplication::translate("QHttpServerExample",
                                                  "Server failed to listen on a port.");
        return -1;
    }
    quint16 sslPort = sslserver->serverPort();
    sslserver.release();


    qInfo().noquote()
        << QCoreApplication::translate("QHttpServerExample",
                                       "Running on http://127.0.0.1:%1/ and "
                                       "https://127.0.0.1:%2/ (Press CTRL+C to quit)")
               .arg(port).arg(sslPort);
#else
    qInfo().noquote()
        << QCoreApplication::translate("QHttpServerExample",
                                       "Running on http://127.0.0.1:%1/"
                                       "(Press CTRL+C to quit)").arg(port);
#endif

    return true;
}

// 在应用启动时读取并缓存图标
void SimpleHttpServer::loadFavicon() {
    const QString resourcePath = ":/Prefix1/resource/logo.png";
    QFile file(resourcePath);
    if (file.open(QIODevice::ReadOnly)) {
        faviconData = file.readAll();
        file.close();
    } else {
        qWarning() << "Favicon not found:" << resourcePath;
    }
}

void SimpleHttpServer::registerAllTestPage()
{
    // http://127.0.0.1:port/test
    httpServer.route("/test", [&]() {
        // 使用 QStringLiteral 包裹所有字符串部分
        QString html = QStringLiteral(uR"(
            <!DOCTYPE html>
            <html>
            <head><title>Test Links</title></head>
            <body>
            <h1>Available Test Endpoints (Port: %1)</h1>
            <ul>
        )").arg(port);

        // 使用 arg() 进行参数替换
        const QString baseUrl = QStringLiteral("http://localhost:%1").arg(port);

        html += QStringLiteral(uR"(
            <li><a href="%1/">/</a> - Hello World</li>
            <li><a href="%1/query">/query</a></li>
            <li><a href="%1/query/123">/query/&lt;id&gt;</a></li>
            <li><a href="%1/query/123/log/0.5">/query/&lt;id&gt;/log/&lt;threshold&gt;</a></li>
            <li><a href="%1/user/456">/user/&lt;id&gt;</a></li>
            <li><a href="%1/user/456/detail/2023">/user/&lt;id&gt;/detail/&lt;year&gt;</a></li>
            <li><a href="%1/json/">/json/</a></li>
            <li><a href="%1/remote_address">/remote_address</a> - Show client IP</li>
            <li><a href="%1/auth">/auth</a> - Basic Auth (Aladdin:open sesame)</li>
            </ul>
            <h3>CURL Test Command:</h3>
            <code>curl -u Aladdin:open\ sesame %1/auth</code>
            </body>
            </html>
        )").arg(baseUrl); //.arg(baseUrl);

        return QHttpServerResponse("text/html; charset=utf-8", html.toUtf8());
    });
}

void SimpleHttpServer::registerIcon() {
    // 添加路由处理
    httpServer.route("/favicon.ico", []() {
        if (faviconData.isEmpty()) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
        }

        // 获取 MIME 类型（自动检测）
        QMimeDatabase mimeDatabase;
        QMimeType mimeType = mimeDatabase.mimeTypeForFileNameAndData(":favicon.ico", faviconData);

        // 构建响应（带缓存头）
        QHttpServerResponse response(faviconData);
        QHttpHeaders headers;
        headers.append("Content-Type", mimeType.name().toUtf8());
        headers.append("Cache-Control", "public, max-age=86400"); // 24小时缓存
        response.setHeaders(headers);

        return response;
    });
}
void SimpleHttpServer::registerRoot() {
    // http://127.0.0.1:54055/
    httpServer.route("/", []() {
        return "Hello world";
    });
}
void SimpleHttpServer::registerPathArgsTest() {
    // http://127.0.0.1:54055/query
    httpServer.route("/query", [] (const QHttpServerRequest &request) {
        return host(request) + u"/query/"_s;
    });

    // http://127.0.0.1:54055/query/1234567
    httpServer.route("/query/", [] (qint32 id, const QHttpServerRequest &request) {
        return u"%1/query/%2"_s.arg(host(request)).arg(id);
    });

    // http://127.0.0.1:54055/query/1234567/log
    httpServer.route("/query/<arg>/log", [] (qint32 id, const QHttpServerRequest &request) {
        return u"%1/query/%2/log"_s.arg(host(request)).arg(id);
    });

    // http://127.0.0.1:54055/query/1234567/log/0.5
    httpServer.route("/query/<arg>/log/", [] (qint32 id, float threshold,
                                             const QHttpServerRequest &request) {
        return u"%1/query/%2/log/%3"_s.arg(host(request)).arg(id).arg(threshold);
    });
}
void SimpleHttpServer::registerUserTest() {
    // http://127.0.0.1:54055/user/456
    httpServer.route("/user/", [] (const qint32 id) {
        return u"User "_s + QString::number(id);
    });

    // http://127.0.0.1:54055/user/456/detail
    httpServer.route("/user/<arg>/detail", [] (const qint32 id) {
        return u"User %1 detail"_s.arg(id);
    });

    // http://127.0.0.1:54055/user/456/detail/2023
    httpServer.route("/user/<arg>/detail/", [] (const qint32 id, const qint32 year) {
        return u"User %1 detail year - %2"_s.arg(id).arg(year);
    });
}
void SimpleHttpServer::registerJsonTest() {
    // http://localhost:54055/json/
    httpServer.route("/json/", [] {
        return QJsonObject{
            {
                {"key1", "1"},
                {"key2", "2"},
                {"key3", "3"}
            }
        };
    });
}

void SimpleHttpServer::registerAuthTest() {
    // Basic authentication example (RFC 7617)
    // http://localhost:54055/auth 使用用户名：Aladdin  密码：open sesame
    httpServer.route("/auth", [](const QHttpServerRequest &request) {
        auto auth = request.value("authorization").simplified();

        if (auth.size() > 6 && auth.first(6).toLower() == "basic ") {
            auto token = auth.sliced(6);
            auto userPass = QByteArray::fromBase64(token);

            if (auto colon = userPass.indexOf(':'); colon > 0) {
                auto userId = userPass.first(colon);
                auto password = userPass.sliced(colon + 1);

                if (userId == "Aladdin" && password == "open sesame")
                    return QHttpServerResponse("text/plain", "Success\n");
            }
        }
        QHttpServerResponse resp("text/plain", "Authentication required\n",
                                 QHttpServerResponse::StatusCode::Unauthorized);
        auto h = resp.headers();
        h.append(QHttpHeaders::WellKnownHeader::WWWAuthenticate,
                 R"(Basic realm="Simple example", charset="UTF-8")");
        resp.setHeaders(std::move(h));
        return std::move(resp);
    });
}

void SimpleHttpServer::registerOtherTest() {
    // http://localhost:54055/assets/style.css  需要确保 style.css 资源存在
    httpServer.route("/assets/<arg>", [] (const QUrl &url) {
        return QHttpServerResponse::fromFile(u":/assets/"_s + url.path());
    });

    // http://localhost:54055/remote_address
    httpServer.route("/remote_address", [](const QHttpServerRequest &request) {
        return request.remoteAddress().toString();
    });
}
