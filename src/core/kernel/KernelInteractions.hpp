#pragma once
#include <QProcess>
#include "base/Qv2rayBase.hpp"
#include "APIBackend.hpp"
#include "shadowsocks.h"
namespace Qv2ray::core::kernel
{

    struct SSRThreadDeleter{
        void operator()(QThread * thread){
            if(threadStart) {
            stop_ss_local_server(); thread->wait();
            }
            delete thread;
        }
        bool threadStart=false;
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
            std::unique_ptr<QThread,SSRThreadDeleter> ssrThread;
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
