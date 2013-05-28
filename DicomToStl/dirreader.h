#ifndef _DIR_READER_H_
#define _DIR_READER_H_

#include <string>
#include <vector>

namespace DicomToStl
{

struct StudyPair
{
    std::string StudyID;
    std::string SeriesNumber;
    std::string Description;
};

bool IsDICOMDIR(std::string dirName);

typedef std::vector<std::string> FileNames; 

void GetFileNamesFromOSDir(const std::string& dirName, FileNames& files);

void GetFileNamesFromDICOMDIR(const std::string& dirName, const StudyPair& pair, FileNames& files); 

typedef std::vector<StudyPair> StudyPairs; 

void GetStudiesPairsFromDir(const std::string& dirName, StudyPairs& pairs);
}

#endif