#include <QObject>
#include <QWidget>
#include "QvCoreInteractions.h"
#include "QvUtils.h"

namespace Qv2ray
{
    bool Qv2Instance::VerifyVConfigFile(const QString path)
    {
        if (ValidateV2rayCoreExe()) {
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert("V2RAY_LOCATION_ASSET", QString::fromStdString(GetGlobalConfig().v2AssetsPath));
            QProcess process;
            process.setProcessEnvironment(env);
            process.start(QString::fromStdString(Utils::GetGlobalConfig().v2CorePath), QStringList() << "-test"
                          << "-config" << path,
                          QIODevice::ReadWrite | QIODevice::Text);

            if (!process.waitForFinished()) {
                qDebug() << "v2ray core failed with exit code " << process.exitCode();
                return false;
            }

            QString output = QString(process.readAllStandardOutput());

            if (!output.contains("Configuration OK")) {
                Utils::QvMessageBox(nullptr, QObject::tr("ConfigurationError"), output.mid(output.indexOf("anti-censorship.") + 17));
                return false;
            } else
                return true;
        } else
            return false;
    }

    Qv2Instance::Qv2Instance(QWidget *parent): _config(Utils::GetGlobalConfig())
    {
        QProcess *proc = new QProcess();
        vProcess = proc;
        QObject::connect(vProcess, SIGNAL(readyReadStandardOutput()), parent, SLOT(parent->updateLog()));
        Status = STOPPED;
    }

    bool Qv2Instance::ValidateV2rayCoreExe()
    {
        auto path = QString::fromStdString(Utils::GetGlobalConfig().v2CorePath);

        if (!QFile::exists(path)) {
            Utils::QvMessageBox(nullptr, QObject::tr("CoreNotFound"), QObject::tr("CoreFileNotFoundExplainationAt:") + path);
            return false;
        } else return true;
    }

    bool Qv2Instance::Start()
    {
        if (Status != STOPPED) {
            return false;
        }

        Status = STARTING;

        if (ValidateV2rayCoreExe()) {
            if (VerifyVConfigFile(QV2RAY_GENERATED_CONFIG_DIRPATH + "config.json")) {
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.insert("V2RAY_LOCATION_ASSET", QString::fromStdString(GetGlobalConfig().v2AssetsPath));
                vProcess->setProcessEnvironment(env);
                vProcess->start(QString::fromStdString(_config.v2CorePath), QStringList() << "-config"
                                << QV2RAY_GENERATED_CONFIG_DIRPATH + "config.json",
                                QIODevice::ReadWrite | QIODevice::Text);
                vProcess->waitForStarted();
                Status = STARTED;
                return true;
            } else {
                Status = STOPPED;
                return false;
            }
        } else {
            Status = STOPPED;
            return false;
        }
    }

    void Qv2Instance::Stop()
    {
        vProcess->close();
        Status = STOPPED;
    }

    QString Qv2Instance::readOutput()
    {
        return vProcess->readAll();
    }

    Qv2Instance::~Qv2Instance()
    {
        Stop();
        delete vProcess;
    }
}
