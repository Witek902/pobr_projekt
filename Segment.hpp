#pragma once

struct Pixel
{
    int x;
    int y;
    Pixel(int x, int y) : x(x), y(y)
    {
    }
};

class Segment
{
public:
    std::vector<Pixel> pixels;

    int minx;
    int miny;
    int maxx;
    int maxy;

    /*
    TODO:
    1. rejecting too small and too big
    2. calculating moments, etc.
    */

    /**
     * Calculate bounding box
     */
    void Process();

    /**
     * Check if the segment should be considered in further calculations
     */
    bool CanReject(int imageWidth, int imageHeight);
};
