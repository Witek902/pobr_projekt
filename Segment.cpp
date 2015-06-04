#include "stdafx.h"
#include "Segment.hpp"

void Segment::Process()
{
    minx = 1000000;
    miny = 1000000;
    maxx = 0;
    maxy = 0;

    for (const Pixel& p : pixels)
    {
        minx = std::min(minx, p.x);
        miny = std::min(miny, p.y);
        maxx = std::max(maxx, p.x);
        maxy = std::max(maxy, p.y);
    }
}

bool Segment::CanReject(int imageWidth, int imageHeight)
{
    return
        (maxx - minx < 7) ||
        (maxy - miny < 7) ||
        (maxx - minx > imageWidth / 4) ||
        (maxy - miny > imageHeight / 4);
}