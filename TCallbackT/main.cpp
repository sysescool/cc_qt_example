#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include "TCoreSession.h"
#include "Models.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 创建核心会话
    TCoreSession session;
    
    // 注册读取文件的回调函数 - 不需要指定 functionName，自动从类型获取
    session.registerCallback<Models::ReadFileRequest>( 
        [](int sequence, const Models::ReadFileRequest& request) -> Models::Response {
            qDebug() << "处理读取文件请求，序列号:" << sequence << "文件路径:" << request.filePath;
            
            Models::Response response;
            response.sequence = sequence;
            
            QFile file(request.filePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                
                Models::ReadFileResponse fileResponse;
                fileResponse.content = content;
                
                response.statusCode = 200;
                response.result = fileResponse.toJsonValue();
            } else {
                response.statusCode = 500;
                response.error = "File read error";
                response.errorReason = QString("Cannot read file: %1").arg(request.filePath);
            }
            
            return response;
        });
    
    // 连接到测试服务器
    session.connectToServer("ws://localhost:8765");
    
    return a.exec();
}
