#ifndef _FORMAT_READER_H_
#define _FORMAT_READER_H_

#include "vec3.h"

#include <string>
#include <vector>

class OFLogger;

namespace DicomToStl
{
typedef std::vector<std::pair<std::string, Vec3> > SlicesPositions;

void ReadFormatDcmFiles(const std::vector<std::string>& files, 
                        OFLogger& logger,
                        int& dx,
                        int& dy,
                        Vec3& spacing,
                        SlicesPositions& slicesPositions);

}

#endif