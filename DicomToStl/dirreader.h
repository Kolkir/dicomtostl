#ifndef _DIR_READER_H_
#define _DIR_READER_H_

#include <string>
#include <vector>

namespace DicomToStl
{

typedef std::vector<std::string> FileNames; 
FileNames GetFileNamesFromDir(const std::string& dirName);
}

#endif