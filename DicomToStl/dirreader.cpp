#include "dirreader.h"

#include <dcmtk/dcmdata/dcdicdir.h>
#include <dcmtk/dcmdata/dcdeftag.h>

#include <algorithm>

using namespace std;

namespace DicomToStl
{

namespace
{

bool IsDICOMDIR(std::string dirName)
{
    std::transform(dirName.begin(), dirName.end(), dirName.begin(), ::tolower);
    auto pos = dirName.size();
    return (pos >= 8 && (dirName.substr(pos - 8, 8) == "DICOMDIR" || dirName.substr(pos - 8, 8) == "dicomdir"));
}

void GetFileNamesFromOSDir(const std::string& dirName, FileNames& files)
{
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    std::string slash = "";
    if (dirName.back() != '\\')
    {
        slash = "\\";
    }
   
    std::string searchPath = dirName + "\\*.dcm";

    // Find the first file in the directory.

    hFind = FindFirstFile(searchPath.c_str(), &ffd);

    if (INVALID_HANDLE_VALUE == hFind) 
    {
        return;
    } 
   
    // List all the files in the directory with some info about them.

    do
    {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //skip sub directories
        }
        else
        {
            files.push_back(dirName + slash +ffd.cFileName);
        }
    }
    while (FindNextFile(hFind, &ffd) != 0);
}

void GetFileNamesFromDICOMDIR(const std::string& fileName, FileNames& files)
{
    string dir = fileName.substr(0, fileName.length() - string("DICOMDIR").length());

    DcmDicomDir dcmDicomDir(fileName.c_str());
    DcmDirectoryRecord& root = dcmDicomDir.getRootRecord();
    OFCondition status;
    DcmDirectoryRecord* StudyRecord = NULL;
    DcmDirectoryRecord* PatientRecord = NULL;
    DcmDirectoryRecord* SeriesRecord = NULL;
    DcmDirectoryRecord* FileRecord = NULL;
    OFString tmpString;
 
    // Analyze DICOMDIR
    while ((PatientRecord = root.nextSub(PatientRecord)) != NULL) 
    {
        while ((StudyRecord = PatientRecord->nextSub(StudyRecord)) != NULL) 
        {
            // Read all series and filter according to SeriesInstanceUID
            while ((SeriesRecord = StudyRecord->nextSub(SeriesRecord)) != NULL) 
            {
                while ((FileRecord = SeriesRecord->nextSub(FileRecord)) != NULL) 
                {
                    char *referencedFileID=NULL;
                    if (FileRecord->findAndGetOFStringArray(DCM_ReferencedFileID, tmpString).good())
                    {
                        referencedFileID = const_cast<char*>(tmpString.c_str());
                    }
                    files.push_back(dir + referencedFileID);
                }
            }
        }
    }
}
}

FileNames GetFileNamesFromDir(const std::string& dirName)
{
    FileNames files;

    if (IsDICOMDIR(dirName))
    {
        GetFileNamesFromDICOMDIR(dirName, files);
    }
    else
    {
        GetFileNamesFromOSDir(dirName, files);
    }

    return files;
}

}
