#ifndef SSRTHREAD_HPP
#define SSRTHREAD_HPP
#include <QThread>

namespace Qv2ray::core::kernel {
class SSRThread : public QThread
{
    Q_OBJECT
public:
    explicit SSRThread();
    explicit SSRThread(
   int localPort,
   int remotePort,
   std::string local_addr,
   std::string remote_host,
   std::string method,
   std::string password,
   std::string obfs,
   std::string obfs_param,
   std::string protocol,
   std::string protocol_param,
   QString inboundTag
   );
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
}
#endif // SSRTHREAD_HPP
