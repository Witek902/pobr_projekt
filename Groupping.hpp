/**
 * POBR - projekt
 * 
 * @author Michal Witanowski
 */

#pragma once

#include "Segment.hpp"

#define NODE_WHITE 0
#define NODE_GRAY 1
#define NODE_BLACK 2

typedef std::vector<Segment*> SegmentGroup;

// Node definition for BFS (Breadth First Search)
struct Node
{
    std::vector<Node*> neighbours;
    Segment* segment;
    Node* parent;
    int color;
    int dist;

    Node();
};

void PerformSegmentGroupping(std::vector<Segment*>& letterCandidates,
                             std::vector<SegmentGroup>& result);