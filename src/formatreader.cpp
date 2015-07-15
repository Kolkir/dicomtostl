#include "formatreader.h"

#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmdata/dcdeftag.h>

#include "vec3.h"

#include <algorithm>

using namespace std;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


namespace DicomToStl
{

void ReadFormatDcmFiles(const std::vector<std::string>& files, 
                        OFLogger& logger,
                        int& dx,
                        int& dy,
                        Vec3& spacing,
                        SlicesPositions& slicesPositions)
{
    // Read files format
    bool formatIsRead = false;

    float rowspacing(1);
    float colspacing(1);
    OFString tmpString;

    slicesPositions.clear();
    slicesPositions.reserve(files.size());

    OFLOG_INFO(logger, "Start reading format properties ..." << OFendl);

    for_each(files.begin(), files.end(),
        [&](const string& fName)
    {
        DcmFileFormat fileformat;
        OFCondition status = fileformat.loadFile(fName.c_str());
        if (!status.bad()) 
        {
            DcmDataset *dataset = fileformat.getDataset();
            if (!formatIsRead)
            {
                formatIsRead = true;
                if (dataset->findAndGetOFStringArray(DCM_Rows, tmpString).good()) 
                {
                    dy = atoi(tmpString.c_str());
                }
                if (dataset->findAndGetOFStringArray(DCM_Columns, tmpString).good()) 
                {
                    dx = atoi(tmpString.c_str());
                }
                OFString rowspacing_str, colspacing_str;
                if (dataset->findAndGetOFString(DCM_PixelSpacing, rowspacing_str, 0).good() &&
                    dataset->findAndGetOFString(DCM_PixelSpacing, colspacing_str, 1).good()) 
                {
                    rowspacing = static_cast<float>(atof(rowspacing_str.c_str()));
                    colspacing = static_cast<float>(atof(colspacing_str.c_str()));
                }
            }

            OFString tmpStrPosX;
            OFString tmpStrPosY;
            OFString tmpStrPosZ;
            Vec3 pos;
            // Position is given by z-component of ImagePositionPatient
            if (dataset->findAndGetOFString(DCM_ImagePositionPatient, tmpStrPosX, 0).good() &&
                dataset->findAndGetOFString(DCM_ImagePositionPatient, tmpStrPosY, 1).good() &&
                dataset->findAndGetOFString(DCM_ImagePositionPatient, tmpStrPosZ, 2).good())
            {
                pos.x = static_cast<float>(atof(tmpStrPosX.c_str()));
                pos.y = static_cast<float>(atof(tmpStrPosY.c_str()));
                pos.z = static_cast<float>(atof(tmpStrPosZ.c_str()));
            }
            slicesPositions.push_back(make_pair(fName, pos));
        }
    });

    // Determine in which direction the slices are arranged and sort by position.
    // Furthermore the slice spacing is determined.
    float slicespacing = 1;
    float imagePositionZ = -1.f;
    if (slicesPositions.size() > 1) 
    {
        Vec3 delta = VecAbs(slicesPositions[1].second - slicesPositions[0].second);
        float maxPosDelta = std::max(delta.x, std::max(delta.y, delta.z));
        if (maxPosDelta == delta.x) 
        {
            std::sort(slicesPositions.begin(), slicesPositions.end(), [](const decltype(slicesPositions[0])& a, const decltype(slicesPositions[0])& b) -> bool {return a.second.x < b.second.x;});
            imagePositionZ = slicesPositions[slicesPositions.size()-1].second.x;
        } 
        else if (maxPosDelta == delta.y) 
        {
            std::sort(slicesPositions.begin(), slicesPositions.end(), [](const decltype(slicesPositions[0])& a, const decltype(slicesPositions[0])& b) -> bool {return a.second.y < b.second.y;});
            imagePositionZ = slicesPositions[slicesPositions.size()-1].second.y;
        }
        else if (maxPosDelta == delta.z) 
        {
            std::sort(slicesPositions.begin(), slicesPositions.end(), [](const decltype(slicesPositions[0])& a, const decltype(slicesPositions[0])& b) -> bool {return a.second.z < b.second.z;});
            imagePositionZ = slicesPositions[slicesPositions.size()-1].second.z;
        }
        slicespacing = VecLength(slicesPositions[slicesPositions.size()-1].second - slicesPositions[0].second) / (slicesPositions.size()-1);
        if (slicespacing == 0.f) 
        {
            slicespacing = 1.f;
        }
    } 

    spacing.x = colspacing;
    spacing.y = rowspacing;
    spacing.z = slicespacing;

    OFLOG_INFO(logger, "Image properties : width " << dx << " height " << dy << OFendl);
    OFLOG_INFO(logger, "Spacing properties : x " << colspacing << " y " << rowspacing << " z " << slicespacing << OFendl);
}

}
