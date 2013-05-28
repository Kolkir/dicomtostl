#include <dcmtk/ofstd/ofconapp.h>
#include <dcmtk/oflog/oflog.h>

#include "dirreader.h"
#include "volumereader.h"
#include "formatreader.h"
using namespace DicomToStl;

#include <iostream>
#include <functional>

#include <ppl.h>

#define SHORTCOL 3
#define LONGCOL 20

bool NeedBreak(HANDLE handleIn, OFLogger& logger);

void FormatTime(double time, size_t& hours, size_t& minutes, size_t& seconds);

int main(int argc, char* argv[])
{
    HANDLE handleIn = GetStdHandle(STD_INPUT_HANDLE);

    try
    {
        OFConsoleApplication app("DicomToStl");
        OFCommandLine cmd;
        OFLogger logger = OFLog::getLogger("DicomToStl");

        cmd.addParam("dcmdir-in",  "DICOM input directory");
        cmd.addParam("stldir-out", "STL output directory");

        cmd.addOption("--isolevel", "-il",  1, "Edge value to build iso surface", "Signed integer value");
        cmd.addOption("--stlbinary", "-sbin", "Generate binary STL file");

        cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
        cmd.addOption("--help", "-h", "print this help text and exit", OFCommandLine::AF_Exclusive);
        
        OFLog::addOptions(cmd);

        if (app.parseCommandLine(cmd, argc, argv, OFCommandLine::PF_ExpandWildcards))
        {
            OFLog::configureFromCommandLine(cmd, app);

            const char* dcmdir = nullptr;
            cmd.getParam(1, dcmdir);
            const char* stldir = nullptr;
            cmd.getParam(2, stldir);

            int isoLevel = 0;
            if (cmd.findOption("--isolevel"))
            { 
                const char* isoLevelStr = nullptr;
                app.checkValue(cmd.getValue(isoLevelStr));
                std::stringstream buf;
                buf << isoLevelStr;
                buf >> isoLevel;
            }
            bool binaryStl = false;
            if (cmd.findOption("--stlbinary"))
            { 
                binaryStl = true;
            }

            std::string outDir = stldir;
            if (outDir.back() != '\\')
            {
                outDir += "\\";
            }

            FileNames files = GetFileNamesFromDir(dcmdir);
            if (!files.empty())
            {
                auto posStart = files[0].find_last_of('\\') + 1;
                auto posEnd = files[0].find_last_of('.');
                std::string fileName = files[0].substr(posStart, posEnd - posStart);
                fileName = outDir + fileName + ".stl";

                int dy(0);
                int dx(0);
                Vec3 spacing;
                SlicesPositions slicesPositions;
                ReadFormatDcmFiles(files, logger, dx, dy, spacing, slicesPositions);

                double time = EstimateProcessingTime(dx, dy, spacing, slicesPositions, isoLevel, fileName, binaryStl, logger);
                size_t hours(0);
                size_t minutes(0);
                size_t seconds(0);
                FormatTime(time, hours, minutes, seconds);

                OFLOG_INFO(logger, "Approximate processing time is : " << hours << " hours " << minutes << " minutes " << seconds << " seconds" << OFendl); 

                if (cmd.findOption("--verbose"))
                {
                    char answer(0);
                    std::cout << "Would you like to continue? [y/n]";
                    std::cin >> answer;
                    if (tolower(answer) == 'n')
                    {
                        return 1;
                    }
                }

                OFLOG_INFO(logger, "Start parsing DICOM files ..." << OFendl);
                ReadVolumeFromDcmFiles(dx, dy, spacing, slicesPositions, isoLevel, fileName, binaryStl, logger, std::bind(NeedBreak, handleIn, logger));
            }
            else
            {
                OFLOG_WARN(logger, "There are no files in the input directory" << OFendl);
            }
            OFLOG_INFO(logger, "Processing finished" << OFendl);
        }
    }
    catch(std::exception& err)
    {
        std::cerr << "C++ exception : " << err.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Unhanded exception\n";
    }
    return 1;
}

bool NeedBreak(HANDLE handleIn, OFLogger& logger)
{
    INPUT_RECORD recordIn;
    DWORD numberOfEvents(0);

    if (GetNumberOfConsoleInputEvents(handleIn, &numberOfEvents))
    {
        if (numberOfEvents >= 1)
        {
            ReadConsoleInput(handleIn, &recordIn, 1, &numberOfEvents);

            if (recordIn.EventType == KEY_EVENT && 
            recordIn.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
            {
                OFLOG_INFO(logger, "Processing terminated" << OFendl);
                return true;
            }
        }
    }
    return false;
}

void FormatTime(double time, size_t& hours, size_t& minutes, size_t& seconds)
{
    size_t millisecondsInHour = 3600000;
    size_t millisecondsInMinute = 60000;
    hours = static_cast<size_t>(time / millisecondsInHour);
    minutes = static_cast<size_t>((time - hours * millisecondsInHour) / millisecondsInMinute);
    seconds = static_cast<size_t>((time - hours * millisecondsInHour - minutes * millisecondsInMinute) / 1000);
}