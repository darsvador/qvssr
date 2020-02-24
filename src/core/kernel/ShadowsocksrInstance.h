#ifndef SHADOWSOCKSRINSTANCE_H
#define SHADOWSOCKSRINSTANCE_H
#include "httpproxy.hpp"
#include <QThread>
#include "base/Qv2rayBase.hpp"
namespace Qv2ray::core::kernel {
class SSRThread : public QThread
{
    Q_OBJECT
public:
    explicit SSRThread();
    explicit SSRThread(std::string local_addr,int local_port,const OUTBOUND& outbound,QString inboundTag);
    QString getInboundTag();
    void run() override;
    ~SSRThread() override;
signals:
    void OnDataReady(QString tag, long dataUp, long dataDown);
    void onSSRThreadLog(QString);
private:
   int localPort;
   int remotePort;
   std::string local_addr;
   std::string remote_host;
   std::string method;
   std::string password;
   std::string obfs;
   std::string obfs_param;
   std::string protocol;
   std::string protocol_param;
   QString inboundTag;
};
class V2rayKernelInstance;
class ShadowsocksrInstance
{
public:
    ShadowsocksrInstance();
    bool restartShadowsocksRThread(V2rayKernelInstance*,const CONFIGROOT& root);
    void stop();
private:
    std::unique_ptr<HttpProxy> httpProxy;
    std::unique_ptr<SSRThread> ssrThread;
};
}

#endif // SHADOWSOCKSRINSTANCE_H
