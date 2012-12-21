#ifndef _STL_WRITER_H_
#define _STL_WRITER_H_

#include "triangulator.h"

#include <string>
#include <fstream>

namespace DicomToStl
{

class StlWriter 
{
public:
    StlWriter(const std::string& fileName, bool binary = false);
    virtual ~StlWriter();
    virtual void Write(const Triangle& tri);
private:
    StlWriter(const StlWriter&);
    StlWriter& operator=(const StlWriter&);
private:
    std::string fileName;
    std::ofstream file;
    bool isBinary;
    size_t triCount;
};

}
#endif
