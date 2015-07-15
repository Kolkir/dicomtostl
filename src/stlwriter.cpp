#include "stlwriter.h"

#include <windows.h>

#include <fstream>
#include <algorithm>

namespace DicomToStl
{

namespace
{
Vec3 MakeNormal(const Triangle& t)
{
    Vec3 a = std::get<1>(t) - std::get<0>(t);
    Vec3 b = std::get<2>(t) - std::get<1>(t);
    Vec3 n = VecCross(a,b);
    VecNormalize(n);
    return n;
}
}

StlWriter::StlWriter(const std::string& fileName, bool binary)
    : fileName(fileName)
    , isBinary(binary)
    , triCount(0)
{
    if (!file)
    {
        throw std::invalid_argument("Can't create output file");
    }
    if (!this->isBinary)
    {
        file.open(fileName.c_str());
        file << "solid\n";
    }
    else
    {
        file.open((fileName + "b").c_str(), std::ios::binary);
    }
}

StlWriter::~StlWriter()
{
    file.flush();
    if (!this->isBinary)
    {
        file << "endsolid\n";
    }
    else
    {
        file.close();
        file.open(fileName.c_str(), std::ios::binary);
        char header[80] = {0};
        file.write(header, sizeof(header));
        file.write(reinterpret_cast<char*>(&this->triCount), sizeof(this->triCount));

        {
            std::ifstream triFile((fileName + "b").c_str(), std::ios::binary);
            const size_t COPY_BUF_SIZE = 1024;
            std::vector<char> buffer(COPY_BUF_SIZE);

            while (triFile.read(buffer.data(), COPY_BUF_SIZE))
            {
                file.write(buffer.data(), triFile.gcount());
            }
            if (triFile.gcount() > 0)
            {
                file.write(buffer.data(), triFile.gcount());
            }
        }
        file.close();
        DeleteFile((fileName + "b").c_str());
    }
}

void StlWriter::Write(const Triangle& tri)
{
    Vec3 n = MakeNormal(tri);
    if (!this->isBinary)
    {
        file << "facet normal " << n.x << " " << n.y << " " << n.z << "\n";
        file << "outer loop\n";
        file << "Vector " << std::get<0>(tri).x << " " << std::get<0>(tri).y << " " << std::get<0>(tri).z << "\n";
        file << "Vector " << std::get<1>(tri).x << " " << std::get<1>(tri).y << " " << std::get<1>(tri).z << "\n";
        file << "Vector " << std::get<2>(tri).x << " " << std::get<2>(tri).y << " " << std::get<2>(tri).z << "\n";
        file << "endloop\n";
        file << "endfacet\n";
    }
    else
    {
        file.write(reinterpret_cast<char*>(&n.x), sizeof(n.x));
        file.write(reinterpret_cast<char*>(&n.y), sizeof(n.y));
        file.write(reinterpret_cast<char*>(&n.z), sizeof(n.z));

        file.write(reinterpret_cast<const char*>(&std::get<0>(tri).x), sizeof(std::get<0>(tri).x));
        file.write(reinterpret_cast<const char*>(&std::get<0>(tri).y), sizeof(std::get<0>(tri).y));
        file.write(reinterpret_cast<const char*>(&std::get<0>(tri).z), sizeof(std::get<0>(tri).z));

        file.write(reinterpret_cast<const char*>(&std::get<1>(tri).x), sizeof(std::get<1>(tri).x));
        file.write(reinterpret_cast<const char*>(&std::get<1>(tri).y), sizeof(std::get<1>(tri).y));
        file.write(reinterpret_cast<const char*>(&std::get<1>(tri).z), sizeof(std::get<1>(tri).z));

        file.write(reinterpret_cast<const char*>(&std::get<2>(tri).x), sizeof(std::get<2>(tri).x));
        file.write(reinterpret_cast<const char*>(&std::get<2>(tri).y), sizeof(std::get<2>(tri).y));
        file.write(reinterpret_cast<const char*>(&std::get<2>(tri).z), sizeof(std::get<2>(tri).z));

        unsigned short abc(0);
        file.write(reinterpret_cast<char*>(&abc), sizeof(abc));
    }
    ++triCount;
}

}
