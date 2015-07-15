#include "logagent.h"             
#include <utility>
#include <dcmtk\oflog\oflog.h>

namespace DicomToStl
{
LogAgent::LogAgent(OFLogger& logger)
    : logger(logger)
{
}

LogAgent::~LogAgent()
{
    this->Stop();
}

void LogAgent::run()
{
    bool done = false;
    while (!done)
    {
        auto msg = Concurrency::receive(this->msgBuffer);
        switch (msg.first)
        {
        case MSG_INFO:
            OFLOG_INFO(this->logger, msg.second << OFendl);
            break;
        case MSG_WARN:
            OFLOG_WARN(this->logger, msg.second << OFendl);
            break;
        case MSG_ERROR:
            OFLOG_FATAL(this->logger, msg.second << OFendl);
            break;
        case MSG_STOP:
            done = true;
            break;
        default:
            OFLOG_INFO(this->logger, "Unknown log type" << OFendl);
            done = true;
            break;
        };
    }
    this->done();
}

void LogAgent::Start()
{
    this->start();
}

void LogAgent::Stop()
{
    Concurrency::asend(this->msgBuffer, std::make_pair(MSG_STOP, std::string("")));
    agent::wait(this);
}

void LogAgent::Log(MsgType type, const std::string& msg)
{
    Concurrency::asend(this->msgBuffer, std::make_pair(type, msg));
}

}