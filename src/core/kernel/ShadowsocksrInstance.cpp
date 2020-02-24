#include "ShadowsocksrInstance.h"
#include "common/QvHelpers.hpp"
#include "KernelInteractions.hpp"
#include "core/connection/ConnectionIO.hpp"
#include "core/CoreUtils.hpp"
#include "shadowsocks.h"
namespace Qv2ray::core::kernel
{
SSRThread::SSRThread(){}
SSRThread::SSRThread(std::string local_addr,int local_port,const OUTBOUND& outbound,QString inbound_tag):local_addr(local_addr),localPort(local_port),inboundTag(inbound_tag){
            auto ssrServer = StructFromJsonString<ShadowSocksRServerObject>(JsonToString(outbound["settings"].toObject()["servers"].toArray().first().toObject()));
            remotePort=ssrServer.port;
            remote_host=ssrServer.address.toStdString();
            method=ssrServer.method.toStdString();
            password=ssrServer.password.toStdString();
            obfs=ssrServer.obfs.toStdString();
            obfs_param=ssrServer.obfs_param.toStdString();
            protocol=ssrServer.protocol.toStdString();
            protocol_param=ssrServer.protocol_param.toStdString();
}
QString SSRThread::getInboundTag()
{
   return inboundTag;
}
void SSRThread::run()
{
            profile_t profile;
            profile.remote_host=remote_host.data();
            profile.local_addr=local_addr.empty()? NULL:local_addr.data();
            profile.method=method.data();
            profile.timeout=600;
            profile.password=password.data();
            profile.obfs=obfs.data();
            profile.obfs_param=obfs_param.data();
            profile.protocol=protocol.data();
            profile.protocol_param=protocol_param.data();
            profile.remote_port=remotePort;
            profile.local_port=localPort;
            profile.mtu=0;//we don't use udp relay, therefore we set mtu to zero.
            profile.mode=0;//we don't use udp relay, therefore we set mode to zero.
            profile.acl=NULL;
            profile.log=NULL;
            profile.fast_open=1;
            profile.mptcp=0;
            start_ss_local_server(profile);
}
SSRThread::~SSRThread()
{
    if(isRunning())
    {
        stop_ss_local_server();
        wait();
    }
}
ShadowsocksrInstance::ShadowsocksrInstance()
{

}

bool ShadowsocksrInstance::restartShadowsocksRThread(V2rayKernelInstance * kernel, const CONFIGROOT &root)
{
            int socks_local_port=0;
            int http_local_port=0;
            QString tag;
            QString listen_address;
            for(const auto& item:root["inbounds"].toArray())
            {
                auto protocol=item.toObject()["protocol"].toString(QObject::tr("N/A"));
                if(protocol=="socks")
                {
                    tag = item.toObject()["tag"].toString("");
                    socks_local_port = item.toObject()["port"].toInt(0);
                    listen_address =item.toObject()["listen"].toString("");
                } else if(protocol=="http"){
                    http_local_port= item.toObject()["port"].toInt(0);
                }
            }
            if(socks_local_port==0)
            {
                return false;
            }
            OUTBOUND outbound = OUTBOUND(root["outbounds"].toArray().first().toObject());
            ssrThread=make_unique<SSRThread>(listen_address.toStdString(),socks_local_port,outbound,tag);
            ssrThread->connect(ssrThread.get(),&SSRThread::onSSRThreadLog,kernel,&V2rayKernelInstance::onProcessOutputReadyRead);
            ssrThread->connect(ssrThread.get(), &SSRThread::OnDataReady, kernel, &V2rayKernelInstance::onAPIDataReady);
            ssrThread->start();
            if(http_local_port!=0)
            {
                httpProxy=make_unique<HttpProxy>();
                httpProxy->httpListen(QHostAddress{listen_address},http_local_port,socks_local_port);

            }
            return true;
}

void ShadowsocksrInstance::stop()
{
        if(ssrThread!=nullptr&&ssrThread->isRunning())
        {
            ssrThread=make_unique<SSRThread>();
        }
        httpProxy=nullptr;
}

}
