#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSysInfo>
#include "TCoreSession.h"
#include "Models.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 创建核心会话
    TCoreSession session;
    
    // 1. 注册读取文件的回调函数
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
    
    // 2. 注册写入文件的回调函数
    session.registerCallback<Models::WriteFileRequest>(
        [](int sequence, const Models::WriteFileRequest& request) -> Models::Response {
            qDebug() << "处理写入文件请求，序列号:" << sequence << "文件路径:" << request.filePath;
            
            Models::Response response;
            response.sequence = sequence;
            
            QFile file(request.filePath);
            QIODevice::OpenMode mode = QIODevice::WriteOnly | QIODevice::Text;
            if (request.append) {
                mode |= QIODevice::Append;
            }
            
            if (file.open(mode)) {
                QTextStream out(&file);
                out << request.content;
                
                Models::WriteFileResponse writeResponse;
                writeResponse.message = "File written successfully";
                writeResponse.bytesWritten = request.content.toUtf8().size();
                
                response.statusCode = 200;
                response.result = writeResponse.toJsonValue();
            } else {
                response.statusCode = 500;
                response.error = "File write error";
                response.errorReason = QString("Cannot write to file: %1").arg(request.filePath);
            }
            
            return response;
        });
    
    // 3. 注册列出目录的回调函数
    session.registerCallback<Models::ListDirectoryRequest>(
        [](int sequence, const Models::ListDirectoryRequest& request) -> Models::Response {
            qDebug() << "处理列出目录请求，序列号:" << sequence << "目录路径:" << request.directoryPath;
            
            Models::Response response;
            response.sequence = sequence;
            
            QDir dir(request.directoryPath);
            if (dir.exists()) {
                Models::ListDirectoryResponse dirResponse;
                
                QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
                if (request.includeHidden) {
                    filters |= QDir::Hidden;
                }
                
                QFileInfoList entries = dir.entryInfoList(filters);
                for (const QFileInfo& info : entries) {
                    Models::ListDirectoryResponse::FileInfo fileInfo;
                    fileInfo.name = info.fileName();
                    fileInfo.type = info.isDir() ? "directory" : "file";
                    fileInfo.size = info.size();
                    fileInfo.lastModified = info.lastModified().toString(Qt::ISODate);
                    dirResponse.files.append(fileInfo);
                }
                
                response.statusCode = 200;
                response.result = dirResponse.toJsonValue();
            } else {
                response.statusCode = 404;
                response.error = "Directory not found";
                response.errorReason = QString("Directory does not exist: %1").arg(request.directoryPath);
            }
            
            return response;
        });
    
    // 4. 注册获取系统信息的回调函数
    session.registerCallback<Models::GetSystemInfoRequest>(
        [](int sequence, const Models::GetSystemInfoRequest& request) -> Models::Response {
            qDebug() << "处理获取系统信息请求，序列号:" << sequence;
            
            Models::Response response;
            response.sequence = sequence;
            
            Models::GetSystemInfoResponse sysResponse;
            sysResponse.systemInfo.osName = QSysInfo::productType();
            sysResponse.systemInfo.osVersion = QSysInfo::productVersion();
            sysResponse.systemInfo.cpuInfo = QSysInfo::currentCpuArchitecture();
            // 注意：获取内存和磁盘信息需要平台特定的代码，这里用示例值
            sysResponse.systemInfo.totalMemory = 8589934592; // 8GB 示例
            sysResponse.systemInfo.availableMemory = 4294967296; // 4GB 示例
            sysResponse.systemInfo.totalDisk = 1099511627776; // 1TB 示例
            sysResponse.systemInfo.availableDisk = 549755813888; // 512GB 示例
            
            response.statusCode = 200;
            response.result = sysResponse.toJsonValue();
            
            return response;
        });
    
    // 连接到测试服务器
    session.connectToServer("ws://localhost:8765");
    
    return a.exec();
}
