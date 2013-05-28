#include "volumereader.h"
#include "logagent.h"
#include "stlwriter.h"

#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmdata/dcdeftag.h>

#include "vec3.h"

#include <algorithm>
#include <memory>

#include <ppl.h>
#include <agents.h>

#include "timer.h"

using namespace std;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


namespace DicomToStl
{

namespace
{

typedef vector<int> ImgBuf;
typedef std::vector<GridCell> CellsBuf;
typedef Concurrency::unbounded_buffer<std::pair<ImgBuf*, ImgBuf*>> MsgImgBuf;
typedef Concurrency::unbounded_buffer<CellsBuf*> MsgCellsBuf;

bool ReadDcmFile(const string& fileName, vector<int>& buffer, LogAgent& logAgent);
void BuildGridCells(vector<GridCell>& cells, int dx, float z1, float z2, 
                     const Vec3& spacing, const vector<int>& topSlice, const vector<int>& bottomSlice);

class FileReadAgent : public Concurrency::agent
{
public:

    FileReadAgent(std::function<bool (void)> needBreak,
                const SlicesPositions& slicesPositions,
                MsgImgBuf& freeBuffers,
                MsgImgBuf& filledBuffers,
                LogAgent& logAgent)
        : needBreak(needBreak),
          slicesPositions(slicesPositions),
          freeBuffers(freeBuffers),
          filledBuffers(filledBuffers),
          logAgent(logAgent)
    {
    }

    virtual void run()
    {
        auto i = slicesPositions.begin();
        auto n = next(i);
        auto e = slicesPositions.end();
   
        MsgImgBuf::type buffers;

        bool skipFailedFiles = false;
        for(;n != e; ++i, ++n)
        {
            if (needBreak())
            {
                break;
            }

            if (!skipFailedFiles)
            {
                buffers = Concurrency::receive(this->freeBuffers);
            }

            if (ReadDcmFile(i->first, *buffers.first, logAgent) &&
                ReadDcmFile(n->first, *buffers.second, logAgent))
            {
                Concurrency::send(this->filledBuffers, buffers);
                skipFailedFiles = false;
            }
            else
            {
                skipFailedFiles = true;
            }
        }
        buffers.first = nullptr;
        buffers.second = nullptr;
        Concurrency::send(this->filledBuffers, buffers);

        this->done();
    }
private:
    FileReadAgent(const FileReadAgent&);
    FileReadAgent& operator= (const FileReadAgent&);
private:
    std::function<bool (void)> needBreak;
    const SlicesPositions& slicesPositions;
    MsgImgBuf& freeBuffers;
    MsgImgBuf& filledBuffers;
    LogAgent& logAgent;
};

class BuildGridAgent : public Concurrency::agent
{
public:

    BuildGridAgent(MsgImgBuf& freeBuffers,
                   MsgImgBuf& filledBuffers,
                   MsgCellsBuf& freeCells,
                   MsgCellsBuf& filledCells,
                   int dx,
                   Vec3 spacing)
        : freeBuffers(freeBuffers),
          filledBuffers(filledBuffers),
          freeCells(freeCells),
          filledCells(filledCells),
          dx(dx),
          spacing(spacing)
    {
    }

    virtual void run()
    {
        bool done = false;
        int z = 0;
        while (!done)
        {
            auto buffers = Concurrency::receive(this->filledBuffers);
            if (buffers.first != nullptr &&
                buffers.second != nullptr)
            {
                auto cells = Concurrency::receive(this->freeCells);
                BuildGridCells(*cells, dx, z * spacing.z, (z + 1) * spacing.z, 
                                    spacing,
                                    *buffers.first, *buffers.second);
                Concurrency::send(this->freeBuffers, buffers);
                Concurrency::send(this->filledCells, cells);
                ++z;
            }
            else
            {
                CellsBuf* p = nullptr;
                Concurrency::send(this->filledCells, p);
                done = true;
            }
        }
        this->done();
    }
private:
    BuildGridAgent(const BuildGridAgent&);
    BuildGridAgent& operator= (const BuildGridAgent&);
private:
    MsgImgBuf& freeBuffers;
    MsgImgBuf& filledBuffers;
    MsgCellsBuf& freeCells;
    MsgCellsBuf& filledCells;
    int dx;
    Vec3 spacing;
};

class TriangulateAgent : public Concurrency::agent
{
public:

    TriangulateAgent(MsgCellsBuf& freeCells,
                     MsgCellsBuf& filledCells,
                     int isoLevel,
                     std::string fileName,
                     bool binaryStl)
        : freeCells(freeCells),
          filledCells(filledCells),
          isoLevel(isoLevel),
          fileName(fileName),
          binaryStl(binaryStl)
    {}
    virtual void run()
    {
        {
            StlWriter stlWriter(fileName, binaryStl);
            bool done = false;
            while (!done)
            {
                auto cells = Concurrency::receive(this->filledCells);
                if (cells != nullptr)
                {
                    std::for_each(cells->begin(), cells->end(),
                    [&](const GridCell& cell)
                    {
                        TriangulateGridCell(cell, isoLevel, stlWriter);
                    });

                    Concurrency::send(this->freeCells, cells);
                }
                else
                {
                     done = true;
                }
            }
        }
        this->done();
    }
private:
    TriangulateAgent(const TriangulateAgent&);
    TriangulateAgent& operator= (const TriangulateAgent&);
private:
    MsgCellsBuf& freeCells;
    MsgCellsBuf& filledCells;
    int isoLevel;
    std::string fileName;
    bool binaryStl;
};

bool ReadDcmFile(const string& fileName, vector<int>& buffer, LogAgent& logAgent)
{
    DcmFileFormat fileformat;

    OFCondition status = fileformat.loadFile(fileName.c_str(), EXS_Unknown,
                                             EGL_withoutGL, DCM_MaxReadLength, ERM_autoDetect);
    if (status.good()) 
    {
        DcmDataset *dataset = fileformat.getDataset();

        std::shared_ptr<DicomImage> image(new DicomImage(&fileformat, dataset->getOriginalXfer()));
        image->hideAllOverlays();

        if (image->getStatus() == EIS_Normal)
        {
            if (!image->isMonochrome())
            {
                image.reset(image->createMonochromeImage());
            }
            image->setNoVoiTransformation();
            const DiPixel* pixelData = image->getInterData();
            if (pixelData->getRepresentation() == EPR_Uint16)
            {
                std::copy(static_cast<const unsigned short*>(pixelData->getData()), 
                         static_cast<const unsigned short*>(pixelData->getData()) + pixelData->getCount(), 
                         buffer.begin());
            }
            else if (pixelData->getRepresentation() == EPR_Sint16)
            {
                std::copy(static_cast<const short*>(pixelData->getData()), 
                         static_cast<const short*>(pixelData->getData()) + pixelData->getCount(), 
                         buffer.begin());
            }
            else if (pixelData->getRepresentation() == EPR_Uint8 || pixelData->getRepresentation() == EPR_MinUnsigned)
            {
                std::copy(static_cast<const unsigned char*>(pixelData->getData()), 
                         static_cast<const unsigned char*>(pixelData->getData()) + pixelData->getCount(), 
                         buffer.begin());
            }
            else if (pixelData->getRepresentation() == EPR_Sint8 || pixelData->getRepresentation() == EPR_MinSigned)
            {
                std::copy(static_cast<const char*>(pixelData->getData()), 
                         static_cast<const char*>(pixelData->getData()) + pixelData->getCount(), 
                         buffer.begin());
            }
            else
            {
                stringstream buf;
                buf << "File " << fileName << " have unsupported format";
                logAgent.Log(LogAgent::MSG_WARN, buf.str());
                return false;
            }
            stringstream buf;
            buf << "File " << fileName << " processed";
            logAgent.Log(LogAgent::MSG_INFO, buf.str());
            return true;
        }
    }
    else
    {
        stringstream buf;
        buf << "Can't read file " << fileName;
        logAgent.Log(LogAgent::MSG_WARN, buf.str());
    }
    return false;
}

template<class T>
class PixelReader
{
public:
    PixelReader(const vector<T>& buf, int w) : buf(buf), w(w) {}
    T GetPixel(int x, int y)
    {
        return buf[x + w *y];
    }
private:
    PixelReader& operator=(const PixelReader&);
    const vector<T>& buf;
    int w;
};

void BuildGridCells(vector<GridCell>& cells, int dx, float z1, float z2, 
                     const Vec3& spacing, const vector<int>& topSlice, const vector<int>& bottomSlice)
{
    int cellsWidth = dx - 1;
   
    PixelReader<int> topReader(topSlice, dx);
    PixelReader<int> bottomReader(bottomSlice, dx);

    Concurrency::parallel_for(size_t(0), cells.size() - 1,
        [&](size_t index)
    {
        int y = index / cellsWidth;
        int x = index - y * cellsWidth;
        
        GridCell cell;
        cell.p[4] = Vec3(x * spacing.x, y * spacing.y, z1);
        cell.val[4] = topReader.GetPixel(x ,y);
        cell.p[5] = Vec3((x + 1) * spacing.x, y * spacing.y, z1);
        cell.val[5] = topReader.GetPixel(x + 1, y);
        cell.p[6] = Vec3((x + 1) * spacing.x, (y + 1) * spacing.y, z1);
        cell.val[6] = topReader.GetPixel(x + 1, y + 1);
        cell.p[7] = Vec3(x * spacing.x, (y + 1) * spacing.y, z1);
        cell.val[7] = topReader.GetPixel(x, y + 1);

        cell.p[0] = Vec3(x * spacing.x, y * spacing.y, z2);
        cell.val[0] = bottomReader.GetPixel(x, y);
        cell.p[1] = Vec3((x + 1) * spacing.x, y * spacing.y, z2);
        cell.val[1] = bottomReader.GetPixel(x + 1, y);
        cell.p[2] = Vec3((x + 1) * spacing.x, (y + 1) * spacing.y, z2);
        cell.val[2] = bottomReader.GetPixel(x + 1, y + 1);
        cell.p[3] = Vec3(x * spacing.x, (y + 1) * spacing.y, z2);
        cell.val[3] = bottomReader.GetPixel(x, y + 1);

        cells[index] = cell;
    });
}

}

void ReadVolumeFromDcmFiles(int dx,
                            int dy,
                            const Vec3& spacing,
                            const SlicesPositions& slicesPositions, 
                            int isoLevel, 
                            const std::string& fileName, 
                            bool binaryStl,
                            OFLogger& logger, 
                            std::function<bool (void)> needBreak)
{
    OFLOG_INFO(logger, "Start triangulation ..." << OFendl);

    size_t bufLen = dx * dy;
    
    ImgBuf topSlice1(bufLen);
    ImgBuf bottomSlice1(bufLen);
    ImgBuf topSlice2(bufLen);
    ImgBuf bottomSlice2(bufLen);

    CellsBuf cells1((dx - 1) * (dy - 1));
    CellsBuf cells2((dx - 1) * (dy - 1));

    MsgImgBuf freeBuffers;
    MsgImgBuf filledBuffers;
    MsgCellsBuf freeCells;
    MsgCellsBuf filledCells;

    LogAgent logAgent(logger);
    logAgent.Start();

    FileReadAgent frAgent(needBreak, slicesPositions, freeBuffers, filledBuffers, logAgent);
    BuildGridAgent bgAgent(freeBuffers, filledBuffers, freeCells, filledCells, dx, spacing);
    TriangulateAgent trAgent(freeCells, filledCells, isoLevel, fileName, binaryStl);

    Concurrency::send(freeBuffers, make_pair(&topSlice1, &bottomSlice1));
    Concurrency::send(freeBuffers, make_pair(&topSlice2, &bottomSlice2));

    Concurrency::send(freeCells, &cells1);
    Concurrency::send(freeCells, &cells2);

    frAgent.start();
    bgAgent.start();
    trAgent.start();

    Concurrency::agent* agents[3]={&frAgent, &bgAgent, &trAgent};
    Concurrency::agent::wait_for_all(3, agents);

    logAgent.Stop();
}

double EstimateProcessingTime(int dx,
                              int dy,
                              const Vec3& spacing,
                              const SlicesPositions& slicesPositions, 
                              int isoLevel,
                              const std::string& fileName,
                              bool binaryStl,
                              OFLogger& logger)
{
    OFLOG_INFO(logger, "Start time estimation ..." << OFendl);

    cpptask::Timer timer;
    timer.Start();

    auto i = slicesPositions.begin();
    auto n = next(i);

    size_t bufLen = dx * dy;
    ImgBuf topSlice(bufLen);
    ImgBuf bottomSlice(bufLen);

    CellsBuf cells((dx - 1) * (dy - 1));

    LogAgent logAgent(logger);
    logAgent.Start();

    double time = 0;
    {
        StlWriter stlWriter(fileName, binaryStl);

        int z = 0;
        if (ReadDcmFile(i->first, topSlice, logAgent) &&
            ReadDcmFile(n->first, bottomSlice, logAgent))
        {
            BuildGridCells(cells, dx, z * spacing.z, (z + 1) * spacing.z, 
                            spacing,
                            topSlice, bottomSlice);
            std::for_each(cells.begin(), cells.end(),
                [&](const GridCell& cell)
            {
                TriangulateGridCell(cell, isoLevel, stlWriter);
            });
        }

        logAgent.Stop();

        time = timer.End();

        time = time * (slicesPositions.size() / 2);
    }

    ::DeleteFile(fileName.c_str());

    return time;
}

}
