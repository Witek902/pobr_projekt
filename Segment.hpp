/**
 * POBR - projekt
 * 
 * @author Michal Witanowski
 */

#pragma once

struct Moments
{
    double S;
    double L;

    double I[11];
    double W9;
};

struct Pixel
{
    int x;
    int y;
    Pixel(int x, int y) : x(x), y(y)
    {
    }
};

#define LETTER_NUM 6
#define LETTER_A 0
#define LETTER_G 1
#define LETTER_M 2
#define LETTER_N 3
#define LETTER_S 4 
#define LETTER_U 5

struct Letters
{
    double letters[LETTER_NUM];
};

class Segment
{
private:


public:
    std::vector<Pixel> pixels;
    int minx;
    int miny;
    int maxx;
    int maxy;

    /**
     * Build segment from binary image.
     * @param m   Input image
     * @param ref Reference image value
     */
    void FromImage(const cv::Mat& m, uchar ref = 0);

    /**
     * Calculate bounding box
     */
    void Process();

    /**
     * Check if the segment should be considered in further calculations
     */
    bool CanReject(int imageWidth, int imageHeight) const;

    /**
     * Calculate invariant moments
     */
    Moments CalculateMoments() const;

    int Classify() const;
};


/// printing functions

std::ostream& operator<<(std::ostream& o, const Segment& segment);
std::ostream& operator<<(std::ostream& o, const Moments& moments);
std::ostream& operator<<(std::ostream& o, const Letters& letters);