#include "stdafx.h"
#include "Segment.hpp"

std::ostream& operator<<(std::ostream& o, const Moments& moments)
{
    for (int i = 1; i < 11; i++)
    {
        o << std::setw(13) << std::setprecision(5) << moments.I[i] << ' ';
    }
    o << std::setw(13) << std::setprecision(5) << moments.W9 << ' ';
    return o;
}

std::ostream& operator<<(std::ostream& o, const Segment& segment)
{
    o << "min = [" << segment.minx << ", " << segment.miny <<
        "], max = [" << segment.maxx << ", " << segment.maxy << ']';
    return o;
}

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

bool Segment::CanReject(int imageWidth, int imageHeight) const
{
    return
        (maxx - minx < 8) ||
        (maxy - miny < 8) ||
        (maxx - minx > imageWidth / 4) ||
        (maxy - miny > imageHeight / 4);
}

Moments Segment::CalculateMoments() const
{
    /// center of AABB
    int boxCx = (maxx - minx) / 2;
    int boxCy = (maxy - miny) / 2;

    /// calculate raw moments
    double M[4][4] =
    {
        { 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0 },
    };

    for (const Pixel& p : pixels)
    {
        double x = static_cast<double>(p.x - boxCx);
        double y = static_cast<double>(p.y - boxCy);

        M[0][0] += 1.0;
        M[1][0] += x;
        M[0][1] += y;
        M[1][1] += x * y;
        M[2][0] += x * x;
        M[0][2] += y * y;
        M[1][2] += x * y * y;
        M[2][1] += x * x * y;
        M[3][0] += x * x * x;
        M[0][3] += y * y * y;
    }

    double cx = M[1][0] / M[0][0];
    double cy = M[0][1] / M[0][0];

    /// calculate central momemnts
    double m[4][4] =
    {
        { 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0 },
    };

    m[0][0] = M[0][0];
    m[0][1] = M[0][1] - (M[0][1] / M[0][0]) * M[0][0];
    m[1][0] = M[1][0] - (M[1][0] / M[0][0]) * M[0][0];
    m[1][1] = M[1][1] - (M[1][0] * M[0][1]) / M[0][0];
    m[2][0] = M[2][0] - (M[1][0] * M[1][0]) / M[0][0];
    m[0][2] = M[0][2] - (M[0][1] * M[0][1]) / M[0][0];
    m[2][1] = M[2][1] - 2 * M[1][1] * cx - M[2][0] * cy + 2 * M[0][1] * cx * cx;
    m[1][2] = M[1][2] - 2 * M[1][1] * cy - M[0][2] * cx + 2 * M[1][0] * cy * cy;
    m[3][0] = M[3][0] - 3 * M[2][0] * cx + 2 * M[1][0] * cx * cx;
    m[0][3] = M[0][3] - 3 * M[0][2] * cy + 2 * M[0][1] * cy * cy;


    /// calculate scale invariant moments
    Moments moments;
    moments.I[0] = 0.0;
    moments.I[1] = (m[2][0] + m[0][2]) / pow(m[0][0], 2);
    moments.I[2] = (pow(m[2][0] - m[0][2], 2) + 4 * m[1][1] * m[1][1]) / pow(m[0][0], 4);
    moments.I[3] = (pow(m[3][0] - 3 * m[1][2], 2) + pow(3 * m[2][1] - m[0][3], 2)) / pow(m[0][0], 5);
    moments.I[4] = (pow(m[3][0] + m[1][2], 2) + pow(m[2][1] + m[0][3], 2)) / pow(m[0][0], 5);
    moments.I[5] = ((m[3][0] - 3 * m[1][2]) * (m[3][0] + m[1][2]) * (pow(m[3][0] + m[1][2], 2) - 3 * pow(m[2][1] + m[0][3], 2)) + (3 * m[2][1] - m[0][3]) * (m[2][1] + m[0][3]) * (3 * pow(m[3][0] + m[1][2], 2) - pow(m[2][1] + m[0][3], 2))) / pow(m[0][0], 10);
    moments.I[6] = ((m[2][0] - m[0][2]) * (pow(m[3][0] + m[1][2], 2) - pow(m[2][1] + m[0][3], 2)) + 4 * m[1][1] * (m[3][0] + m[1][2]) * (m[2][1] + m[0][3])) / pow(m[0][0], 7);
    moments.I[7] = (m[2][0] * m[0][2] - m[1][1] * m[1][1]) / pow(m[0][0], 4);
    moments.I[8] = (m[3][0] * m[1][2] + m[2][1] * m[0][3] - m[1][2] * m[1][2] - m[2][1] * m[2][1]) / pow(m[0][0], 5);
    moments.I[9] = (m[2][0] * (m[2][1] * m[0][3] - m[1][2] * m[1][2]) + m[0][2] * (m[0][3] * m[1][2] - m[2][1] * m[2][1]) - m[1][1] * (m[3][0] * m[0][3] - m[2][1] * m[1][2])) / pow(m[0][0], 7);
    moments.I[10] = (pow(m[3][0] * m[0][3] - m[1][2] * m[2][1], 2) - 4 * (m[3][0] * m[1][2] - m[2][1] * m[2][1]) * (m[0][3] * m[2][1] - m[1][2])) / pow(m[0][0], 10);

    /*
    // from wikipedia

    m[0][0] = M[0][0];
    m[0][1] = 0.0;
    m[1][0] = 0.0;
    m[1][1] = M[1][1] - cx * M[0][1];
    m[2][0] = M[2][0] - cx * M[1][0];
    m[2][0] = M[0][2] - cy * M[0][1];
    m[2][1] = M[2][1] - 2.0 * cx * M[1][1] - cy * M[2][0] + 2.0 * cx * cx * M[0][1];
    m[1][2] = M[1][2] - 2.0 * cy * M[1][1] - cx * M[0][2] + 2.0 * cy * cy * M[0][1];
    m[3][0] = M[3][0] - 3.0 * cx * M[2][0] + 2.0 * cx * cx * M[1][0];
    m[0][3] = M[0][3] - 3.0 * cy * M[0][2] + 2.0 * cy * cy * M[0][1];
    */

    // create image containing the segment
    cv::Mat img(maxy-miny+1, maxx-minx+1, CV_8SC1, cvScalar(0.0f));
    for (const Pixel& p : pixels)
        img.at<uchar>(p.y - miny, p.x - minx) = 255;

    // calculate circumference
    int L = 0;
    for (int i = 0; i < img.rows; ++i)
    {
        for (int j = 0; j < img.cols; ++j)
        {
            if (img.at<uchar>(i, j) == 255)
            {
                if (i == 0 || j == 0 || i == img.rows - 1 || j == img.cols - 1)
                    L++;
                else
                {
                    if (img.at<uchar>(i - 1, j) != 255) L++;
                    if (img.at<uchar>(i + 1, j) != 255) L++;
                    if (img.at<uchar>(i, j - 1) != 255) L++;
                    if (img.at<uchar>(i, j + 1) != 255) L++;
                }
            }
        }
    }
    moments.L = static_cast<double>(L);
    moments.S = static_cast<double>(M[0][0]);

    moments.W9 = 2.0 * sqrt(3.14159 * moments.S) / moments.L;

    return moments;
}

void Segment::FromImage(const cv::Mat& m, uchar ref)
{
    for (int i = 0; i < m.rows; ++i)
    {
        for (int j = 0; j < m.cols; ++j)
        {
            if (m.at<uchar>(i, j) == ref)
                pixels.push_back(Pixel(j, i));
        }
    }
}

int Segment::Classify() const
{
    Moments moments = CalculateMoments();

    if (moments.I[1] < 0.18)
        return 0;
    if (moments.I[1] > 0.29)
        return 0;

    if (moments.I[2] < 1.0e-5)
        return 0;
    if (moments.I[2] > 0.04)
        return 0;

    if (moments.I[3] < 1.0e-10)
        return 0;
    if (moments.I[3] > 0.006)
        return 0;

    if (moments.I[4] < 1.0e-9)
        return 0;
    if (moments.I[4] > 0.0006)
        return 0;

    if (moments.I[7] < 0.008)
        return 0;
    if (moments.I[7] > 0.018)
        return 0;

    if (moments.W9 < 0.29)
        return 0;
    if (moments.W9 > 0.59)
        return 0;

    return 1;
}

std::ostream& operator<<(std::ostream& o, const Letters& letters)
{
    o << "S = " << std::setprecision(2) << letters.letters[LETTER_S] << ", ";
    o << "A = " << std::setprecision(2) << letters.letters[LETTER_A] << ", ";
    o << "M = " << std::setprecision(2) << letters.letters[LETTER_M] << ", ";
    o << "U = " << std::setprecision(2) << letters.letters[LETTER_U] << ", ";
    o << "N = " << std::setprecision(2) << letters.letters[LETTER_N] << ", ";
    o << "G = " << std::setprecision(2) << letters.letters[LETTER_G] << ", ";
    return o;
}