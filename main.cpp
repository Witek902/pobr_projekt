#include "stdafx.h"
#include "Segment.hpp"

#define HISTOGRAM_CUT 20
#define COLOR_TRESHOLD 0.5f

/**
 * This function preprocesses input image. The following steps are prformed:
 * 1. Conversion to grayscale and histogram calculation.
 * 2. Histogram scaling (removing brightest and darkest pixels).
 * 3. Applying threshold and generating binary image (8UC1 format).
 */
cv::Mat Preprocess(const cv::Mat& m, float treshold = 0.5f)
{
    assert(3 == m.channels());
    assert(CV_8UC3 == m.type());

    int histogram[256] = { 0 };

    // convert to grayscale and calculate histogram
    cv::Mat grayscale(m.rows, m.cols, CV_32F); // temporary image
    for (int i = 0; i < m.rows; ++i)
    {
        for (int j = 0; j < m.cols; ++j)
        {
            cv::Vec3b color = m.at<cv::Vec3b>(i, j);

            float fValue = 0.299f * (float)color[2] + 0.587f * (float)color[1] + 0.114f * (float)color[0];
            if (fValue > 255.0f)
                fValue = 255.0f;
            unsigned char value = (char)(fValue);
            histogram[value]++;

            grayscale.at<float>(i, j) = fValue / 255.0f;
        }
    }

    const int totalPixels = m.rows * m.cols;
    int counter;
    int lowScale = 0;
    int highScale = 255;

    // reject darkest pixels
    counter = 0;
    for (int i = 0; i < 256; ++i)
    {
        counter += histogram[i];
        if (counter < totalPixels / HISTOGRAM_CUT)
            lowScale = i;
    }

    // reject brightest pixels
    counter = 0;
    for (int i = 255; i >= 0; --i)
    {
        counter += histogram[i];
        if (counter < totalPixels / HISTOGRAM_CUT)
            highScale = i;
    }

    // generate final binary image 
    cv::Mat result(m.rows, m.cols, CV_8UC1);
    for (int i = 0; i < m.rows; ++i)
    {
        for (int j = 0; j < m.cols; ++j)
        {
            float value = grayscale.at<float>(i, j);
            value -= (float)lowScale / 256.0f;
            value /= (float)(highScale - lowScale) / 256.0f;
            result.at<uchar>(i, j) = value > treshold ? 255 : 0;
        }
    }

    return result;
}

/**
 * 
 */
cv::Mat CalculatePixelGroups(const cv::Mat& input, std::vector<Segment*>& outputSegments)
{
    assert(CV_8UC1 == input.type());

    std::map<int, int> labelAliasMap;

    cv::Mat groupMap(input.rows, input.cols, CV_32SC1);
    int id = 0;

    for (int i = 0; i < input.rows; ++i)
    {
        for (int j = 0; j < input.cols; ++j)
        {
            uchar currVal = input.at<uchar>(i, j);
            bool sameAsLeft = false;
            bool sameAsTop = false;

            if (i > 0)
                sameAsTop = (currVal == input.at<uchar>(i - 1, j));

            if (j > 0)
                sameAsLeft = (currVal == input.at<uchar>(i, j - 1));

            if (sameAsTop && sameAsLeft)
            {
                int topLabel = labelAliasMap[groupMap.at<int>(i - 1, j)];
                int leftLabel = labelAliasMap[groupMap.at<int>(i, j - 1)];
                if (topLabel != leftLabel)
                {
                    int minLabel = std::min(topLabel, leftLabel);
                    int maxLabel = std::max(topLabel, leftLabel);
                    groupMap.at<int>(i, j) = minLabel;
                    labelAliasMap[maxLabel] = minLabel;
                    continue;
                }
            }

            if (sameAsTop)
            {
                groupMap.at<int>(i, j) = labelAliasMap[groupMap.at<int>(i - 1, j)];
            }
            else if (sameAsLeft)
            {
                groupMap.at<int>(i, j) = labelAliasMap[groupMap.at<int>(i, j - 1)];
            }
            else // create unique label
            {
                int label = id++;
                groupMap.at<int>(i, j) = label;
                labelAliasMap[label] = label;
            }
        }
    }

    // create segments for each unique (merged) label
    std::map<int, Segment*> segments;
    for (auto label : labelAliasMap)
    {
        auto it = segments.find(label.second);
        if (it == segments.end())
        {
            segments[label.second] = new Segment;
        }
    }

    // fill segments with pixels
    cv::Mat groupMapMerged(input.rows, input.cols, CV_32SC1);
    for (int i = 0; i < input.rows; ++i)
    {
        for (int j = 0; j < input.cols; ++j)
        {
            int label = groupMap.at<int>(i, j);
            int finalLabel = labelAliasMap[label];
            groupMapMerged.at<int>(i, j) = finalLabel;
            segments[finalLabel]->pixels.push_back(Pixel(j, i));
        }
    }

    std::cout << "Initial pixel groups: " << segments.size() << std::endl;

    // reject invalid segments (too small, too big)
    int rejected = 0;
    outputSegments.clear();
    for (auto it : segments)
    {
        Segment* segment = it.second;
        segment->Process();
        if (segment->CanReject(input.cols, input.rows))
        {
            rejected++;
            delete segment;
            continue;
        }

        outputSegments.push_back(segment);
    }

    std::cout << "Rejected pixel groups: " << rejected << std::endl;
    return groupMapMerged;
}

inline int FastRand(int x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x);
    return x;
}

cv::Mat VisualizeSegments(const cv::Mat& input, const std::vector<Segment*> segments)
{
    assert(CV_32SC1 == input.type());

    cv::Mat visual(input.rows, input.cols, CV_8SC3, cvScalar(0.0f));

    int id = 0;
    for (const Segment* segment : segments)
    {
        int r = FastRand(id);
        int g = FastRand(id + 171050183);
        int b = FastRand(id + 101531671);
        for (const Pixel& p : segment->pixels)
        {
            visual.at<cv::Vec3b>(p.y, p.x) = cv::Vec3b(r % 256, g % 256, b % 256);
        }
        id++;
    }

    return visual;
}

cv::Mat VisualizePixelGroups(const cv::Mat& input)
{
    assert(CV_32SC1 == input.type());

    cv::Mat visual(input.rows, input.cols, CV_8SC3, cvScalar(0.0f));
    for (int i = 0; i < input.rows; ++i)
    {
        for (int j = 0; j < input.cols; ++j)
        {
            int groupId = input.at<int>(i, j);
            int r = FastRand(groupId);
            int g = FastRand(groupId + 171050183);
            int b = FastRand(groupId + 101531671);
            visual.at<cv::Vec3b>(i, j) = cv::Vec3b(r % 256, g % 256, b % 256);
        }
    }

    return visual;
}

/*

TODO:
* sharpening
* moment calculation
* segmentation
* prepare moments databese for SAMSUNG letters

 */

int main(int argc, char** argv)
{
    if (argc < 1)
    {
        std::cout << "Pass a file name as a parameter" << std::endl;
        return -1;
    }

    cv::Mat image;
    image = cv::imread("Test/10_small.png", cv::IMREAD_COLOR);
    //image = cv::imread("Test/10_small.png", cv::IMREAD_COLOR);
    if (image.empty())
    {
        std::cout << "Could not open or find the image" << std::endl;
        return 1;
    }

    cv::Mat binaryImage = Preprocess(image, COLOR_TRESHOLD);
    cv::namedWindow("Binary image", cv::WINDOW_AUTOSIZE);
    cv::imshow("Binary image", binaryImage);

    std::vector<Segment*> segments;
    cv::Mat pixelGroups = CalculatePixelGroups(binaryImage, segments);
    cv::Mat segmentsVisual = VisualizeSegments(pixelGroups, segments);
    //cv::Mat segmentsVisual = VisualizePixelGroups(pixelGroups);
    cv::namedWindow("Segments", cv::WINDOW_AUTOSIZE);
    cv::imshow("Segments", segmentsVisual);

    for (Segment* seg : segments)
    {
        std::cout <<
            "min = [" << seg->minx << ", " << seg->miny <<
            "], max = [" << seg->maxx << ", " << seg->maxy << "]" << std::endl;
    }

    cv::waitKey(0);
    return 0;
}