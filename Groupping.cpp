#include "stdafx.h"
#include "Groupping.hpp"

bool IsNeighbour(const Segment* a, const Segment* b)
{
    int aSizeX = a->maxx - a->minx;
    int aSizeY = a->maxy - a->miny;
    int bSizeX = b->maxx - b->minx;
    int bSizeY = b->maxy - b->miny;

    // letters should be similar size
    if (aSizeX > 2 * bSizeX)
        return false;
    if (aSizeY > 2 * bSizeY)
        return false;
    if (bSizeX > 2 * aSizeX)
        return false;
    if (bSizeY > 2 * aSizeY)
        return false;

    if (a->minx > b->maxx + bSizeX)
        return false;
    if (a->miny > b->maxy + bSizeY)
        return false;
    if (b->minx > a->maxx + aSizeX)
        return false;
    if (b->miny > a->maxy + aSizeY)
        return false;

    return true;
}

Node::Node()
{
    dist = -1;
    parent = nullptr;
    color = NODE_WHITE;
}

void PerformSegmentGroupping(std::vector<Segment*>& letterCandidates,
                             std::vector<SegmentGroup>& result)
{
    // build graph
    std::vector<Node*> nodes;
    for (size_t i = 0; i < letterCandidates.size(); ++i)
    {
        Node* node = new Node;
        node->segment = letterCandidates[i];
        nodes.push_back(node);
    }

    // create edges
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        for (size_t j = i + 1; j < nodes.size(); ++j)
        {
            if (IsNeighbour(nodes[i]->segment, nodes[j]->segment))
            {
                nodes[i]->neighbours.push_back(nodes[j]);
                nodes[j]->neighbours.push_back(nodes[i]);
            }
        }
    }

    // Breadth First Search algorithm

    while (!nodes.empty())
    {
        std::stack<Node*> nodesQueue;
        nodes[0]->color = NODE_GRAY;
        nodes[0]->dist = 0;
        nodesQueue.push(nodes[0]);

        while (!nodesQueue.empty())
        {
            Node* u = nodesQueue.top();
            nodesQueue.pop();

            for (Node* v : u->neighbours)
            {
                if (v->color == NODE_WHITE)
                {
                    v->color == NODE_GRAY;
                    v->dist = u->dist + 1;
                    v->parent = u;
                    nodesQueue.push(v);
                }
            }
            u->color = NODE_BLACK;
        }

        SegmentGroup group;
        for (auto iter = nodes.begin(); iter != nodes.end();)
        {
            Node* node = *iter;
            if (node->color == NODE_BLACK)
            {
                group.push_back(node->segment);
                delete node;
                iter = nodes.erase(iter);
            }
            else
                ++iter;
        }
        result.push_back(group);
    }
}