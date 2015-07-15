#ifndef _LOG_AGENT_H_
#define _LOG_AGENT_H_

#include <agents.h>
#include <string>
#include <sstream>

class OFLogger;

namespace DicomToStl
{

class LogAgent : public Concurrency::agent
{
public:
    enum MsgType
    {
        MSG_STOP,
        MSG_INFO,
        MSG_WARN,
        MSG_ERROR
    };
    LogAgent(OFLogger& logger);
    virtual ~LogAgent();
    virtual void run();
    void Start();
    void Stop();
    void Log(MsgType type, const std::string& msg);
private:
    LogAgent(const LogAgent&);
    LogAgent& operator=(const LogAgent&);
private:
    Concurrency::unbounded_buffer<std::pair<MsgType, std::string> > msgBuffer;
    OFLogger& logger;
};

}

#endif
