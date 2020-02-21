#pragma once
#include <QProcess>
#include "base/Qv2rayBase.hpp"
#include "APIBackend.hpp"
#include "shadowsocks.h"
namespace Qv2ray::core::kernel
{
    class SSRThread : public QThread
    {
        Q_OBJECT
    public:
        explicit SSRThread();
        explicit SSRThread(int local_port,const OUTBOUND& outbound,QString inboundTag);
        QString getInboundTag();
        void run() override;
        ~SSRThread() override;
    signals:
        void OnDataReady(QString tag, long dataUp, long dataDown);
        void onSSRThreadLog(QString);
    private:
       int localPort;
       int remotePort;
       std::string remote_host;
       std::string method;
       std::string password;
       std::string obfs;
       std::string obfs_param;
       std::string protocol;
       std::string protocol_param;
       QString inboundTag;
    };

    class V2rayKernelInstance : public QObject
    {
            Q_OBJECT
        public:
            explicit V2rayKernelInstance();
            ~V2rayKernelInstance() override;
            //
            // Speed
            long getTagSpeedUp(const QString &tag);
            long getTagSpeedDown(const QString &tag);
            long getTagDataUp(const QString &tag);
            long getTagDataDown(const QString &tag);
            long getAllDataUp();
            long getAllDataDown();
            long getAllSpeedUp();
            long getAllSpeedDown();
            //
            bool StartConnection(CONFIGROOT root);
            void StopConnection();
            bool KernelStarted = false;
            //
            static bool ValidateConfig(const QString &path);
            static bool ValidateKernel(const QString &vCorePath, const QString &vAssetsPath, QString *message);

        signals:
            void onProcessErrored();
            void onProcessOutputReadyRead(QString);

        public slots:
            void onAPIDataReady(QString tag, long totalUp, long totalDown);

        private:
            std::unique_ptr<SSRThread> ssrThread;
            APIWorkder *apiWorker;
            QProcess *vProcess;
            bool apiEnabled;
            QMap<QString, long> transferDataUp;
            QMap<QString, long> transferDataDown;
            QMap<QString, long> transferSpeedUp;
            QMap<QString, long> transferSpeedDown;
    };
}

using namespace Qv2ray::core::kernel;
