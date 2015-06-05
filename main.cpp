#include "stdafx.h"
#include "Segment.hpp"
#include "Groupping.hpp"

#define HISTOGRAM_CUT 15
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
* Simple sharpening filter
*/
cv::Mat Sharpen(const cv::Mat& m)
{
    const float filter[3][3] =
    {
        { -1.0f, -2.0f, -1.0f },
        { -2.0f, 16.0f, -2.0f },
        { -1.0f, -2.0f, -1.0f },
    };

    cv::Mat output(m.rows, m.cols, CV_8UC3);
    for (int i = 0; i < m.rows; ++i)
    {
        for (int j = 0; j < m.cols; ++j)
        {
            float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f;
            for (int k = 0; k < 3; ++k)
                for (int l = 0; l < 3; ++l)
                {
                    int y = std::max(0, std::min(m.rows - 1, i + k - 1));
                    int x = std::max(0, std::min(m.cols - 1, j + l - 1));
                    cv::Vec3b color = m.at<cv::Vec3b>(y, x);
                    sumR += filter[k][l] * (float)color[2];
                    sumG += filter[k][l] * (float)color[1];
                    sumB += filter[k][l] * (float)color[0];
                }

            sumR = std::min(255.0f, std::max(0.0f, sumR / 4.0f));
            sumG = std::min(255.0f, std::max(0.0f, sumG / 4.0f));
            sumB = std::min(255.0f, std::max(0.0f, sumB / 4.0f));

            output.at<cv::Vec3b>(i, j) = cv::Vec3b((uchar)sumB, (uchar)sumG, (uchar)sumR);
        }
    }

    return output;
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

int MomentCalculator(int argc, char** argv)
{
    cv::Mat image, binaryImage;

    for (int i = 2; i < argc; ++i)
    {
        const char* name = argv[i];

        image = cv::imread(argv[i], cv::IMREAD_COLOR);
        if (image.empty())
            continue;

        binaryImage = Preprocess(image, COLOR_TRESHOLD);

        Segment seg;
        seg.FromImage(binaryImage, 0);
        seg.Process();

        Moments moments = seg.CalculateMoments();
        std::cout << moments << std::endl;
    }

    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Pass a file name as a parameter" << std::endl;
        return -1;
    }

    if (strcmp(argv[1], "--moments") == 0 ||
        strcmp(argv[1], "-m") == 0)
    {
        return MomentCalculator(argc, argv);
    }

    cv::Mat original;
    original = cv::imread("Test/10.jpg", cv::IMREAD_COLOR);
    if (original.empty())
    {
        std::cout << "Could not open or find the image" << std::endl;
        return 1;
    }

    cv::Mat image = Sharpen(original);

    cv::Mat binaryImage = Preprocess(image, COLOR_TRESHOLD);
    cv::namedWindow("POBR - Binary image", cv::WINDOW_AUTOSIZE);
    cv::imshow("POBR - Binary image", binaryImage);

    std::vector<Segment*> segments;
    cv::Mat pixelGroups = CalculatePixelGroups(binaryImage, segments);
    cv::Mat segmentsVisual = VisualizeSegments(pixelGroups, segments);
    //cv::Mat segmentsVisual = VisualizePixelGroups(pixelGroups);


    std::vector<Segment*> letterCandidates;
    for (Segment* seg : segments)
    {
        if (seg->Classify() > 0)
        {
            letterCandidates.push_back(seg);
            /*
            cv::Scalar color = cv::Scalar(0.0, 255.0, 20.0);
            cv::Rect rect = cv::Rect(seg->minx - 1, seg->miny - 1,
                                     seg->maxx - seg->minx + 2, seg->maxy - seg->miny + 2);
            cv::rectangle(original, rect, color, 1);
            */
        }
    }

    std::vector<SegmentGroup> groups;
    PerformSegmentGroupping(letterCandidates, groups);
    std::cout << "Groups found: " << groups.size() << std::endl;

    int validGroups = 0;
    for (const auto& group : groups)
    {
        if (group.size() == 7)
        {
            int minx = 1000000, miny = 1000000, maxx = 0, maxy = 0;
            for (const Segment* seg : group)
            {
                minx = std::min(minx, seg->minx);
                maxx = std::max(maxx, seg->maxx);
                miny = std::min(miny, seg->miny);
                maxy = std::max(maxy, seg->maxy);
            }

            std::cout << "Group #" << validGroups++ <<
                "  minX=" << minx << ", minY=" << miny <<
                ", maxX=" << maxx << ", maxY=" << maxy << std::endl;

            // draw valid group
            cv::Scalar color = cv::Scalar(0.0, 255.0, 20.0);
            cv::Rect rect = cv::Rect(minx - 1, miny - 1, maxx - minx + 2, maxy - miny + 2);
            cv::rectangle(original, rect, color, 2);
        }
    }

    cv::namedWindow("POBR - original image", cv::WINDOW_AUTOSIZE);
    cv::imshow("POBR - original image", original);

    cv::waitKey(0);
    return 0;
}