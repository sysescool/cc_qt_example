#ifndef SIMPLEHTTPSERVER_H
#define SIMPLEHTTPSERVER_H

#include <QObject>
#include <QHttpServer>
#include <QTcpServer>

class SimpleHttpServer : public QObject
{
    Q_OBJECT

public:
    // 获取单例实例的静态方法
    static SimpleHttpServer* instance();

    SimpleHttpServer* setPort(quint16 port);

    bool start();

private:
    // 私有构造，防止外部创建  (这里可以不用 QObject, 它始终为 nullptr)
    explicit SimpleHttpServer(QObject *parent = nullptr);

    // 禁用拷贝构造和赋值运算
    SimpleHttpServer(const SimpleHttpServer&) = delete;
    SimpleHttpServer& operator=(const SimpleHttpServer&) = delete;

    // 静态成员变量，用于保存单例实例
    static QScopedPointer<SimpleHttpServer> m_instance;
    static QMutex m_mutex; // 用于线程安全

    // 在全局或静态变量中缓存文件内容
    static QByteArray faviconData;

    quint16 port;

    static void loadFavicon();

    void registerAllTestPage();
    void registerIcon();
    void registerRoot();
    void registerPathArgsTest();
    void registerUserTest();
    void registerJsonTest();
    void registerOtherTest();
    void registerAuthTest();

    QHttpServer httpServer;
    QScopedPointer<QTcpServer> tcpserver;
};

#endif // SIMPLEHTTPSERVER_H
