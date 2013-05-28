#ifndef _VOLUME_READER_H_
#define _VOLUME_READER_H_

#include "triangulator.h"
#include "formatreader.h"

#include <vector>
#include <string>
#include <functional>

class OFLogger;

namespace DicomToStl
{

void ReadVolumeFromDcmFiles(int dx,
                            int dy,
                            const Vec3& spacing,
                            const SlicesPositions& slicesPositions, 
                            int isoLevel, 
                            const std::string& fileName,
                            bool binaryStl,
                            OFLogger& logger, 
                            std::function<bool (void)> needBreak);

double EstimateProcessingTime(int dx,
                              int dy,
                              const Vec3& spacing,
                              const SlicesPositions& slicesPositions, 
                              int isoLevel,
                              const std::string& fileName,
                              bool binaryStl,
                              OFLogger& logger);
}

#endif