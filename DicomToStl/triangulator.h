#ifndef _TRIANGULATOR_H_
#define _TRIANGULATOR_H_

#include "vec3.h"

#include <vector>
#include <tuple>

namespace DicomToStl
{

typedef std::tuple<Vec3,Vec3,Vec3> Triangle;
typedef std::vector<Triangle> Triangles;

struct GridCell
{
    GridCell() : p(8), val(8) {}
    std::vector<Vec3> p;
    std::vector<int> val;
};

class StlWriter;

void TriangulateGridCell(const GridCell& cell, int isolevel, StlWriter& stlWriter);

}

#endif
