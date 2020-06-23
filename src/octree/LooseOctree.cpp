/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2020 — Nghia Truong <nghiatruong.vn@gmail.com>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "LooseOctree.h"

#include <Corrade/Containers/GrowableArray.h>

namespace Magnum { namespace Examples {

OctreeNode::OctreeNode(LooseOctree* const tree, OctreeNode* const parent,
    const Vector3& nodeCenter, const Float halfWidth, const size_t depth):
    _center{nodeCenter},
    _bounds{nodeCenter - Vector3{halfWidth}, nodeCenter + Vector3{halfWidth}},
    _boundsExtended(nodeCenter - Vector3{2.0f*halfWidth},
        nodeCenter + Vector3{2.0f*halfWidth}),
    _halfWidth{halfWidth}, _depth{depth}, _tree{tree}, _parent{parent},
    _children{nullptr}, _maxDepth{tree->_maxDepth}, _isLeaf{true} {}

const OctreeNode& OctreeNode::childNode(const std::size_t childIdx) const {
    CORRADE_INTERNAL_ASSERT(_children);
    return _children->_nodes[childIdx];
}

void OctreeNode::removePointFromSubTree() {
    arrayResize(_nodePoints, 0);

    if(!_isLeaf) for(std::size_t childIdx = 0; childIdx < 8; ++childIdx)
        _children->_nodes[childIdx].removePointFromSubTree();
}

void OctreeNode::split() {
    if(!_isLeaf || _depth == _maxDepth) return;

    if(_isLeaf) {
        /*    6-------7
             /|      /|
            2-+-----3 |
            | |     | |   y
            | 4-----+-5   | z
            |/      |/    |/
            0-------1     +--x

            0   =>   0, 0, 0
            1   =>   0, 0, 1
            2   =>   0, 1, 0
            3   =>   0, 1, 1
            4   =>   1, 0, 0
            5   =>   1, 0, 1
            6   =>   1, 1, 0
            7   =>   1, 1, 1   */
        _children = _tree->requestChildrenFromPool();

        const auto childHalfWidth = _halfWidth*0.5f;
        for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
            Vector3 newCenter = _center;
            newCenter[0] += (childIdx & 1) ? childHalfWidth : -childHalfWidth;
            newCenter[1] += (childIdx & 2) ? childHalfWidth : -childHalfWidth;
            newCenter[2] += (childIdx & 4) ? childHalfWidth : -childHalfWidth;

            OctreeNode* const childNode = &_children->_nodes[childIdx];
            childNode->~OctreeNode(); /* call destructor to release resource */

            /* Placement new: re-use existing memory block, call constructor to
               re-initialize data */
            new(childNode) OctreeNode{_tree, this, newCenter, childHalfWidth,
                _depth + 1};
        }

        /* Must explicitly mark as non-leaf node, and must do this after all
           children nodes are ready */
        _isLeaf = false;
    }
}

void OctreeNode::removeAllDescendants() {
    if(_isLeaf) return;

    for(std::size_t childIdx = 0; childIdx < 8; ++childIdx)
        _children->_nodes[childIdx].removeAllDescendants();

    _tree->returnChildrenToPool(_children);
    _isLeaf = true;
}

void OctreeNode::removeEmptyDescendants() {
    if(_isLeaf) return;

    bool allEmpty = true;
    bool allLeaves = true;
    for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
        auto& pChildNode = _children->_nodes[childIdx];
        pChildNode.removeEmptyDescendants();
        allLeaves &= pChildNode.isLeaf();
        allEmpty &= (pChildNode.pointCount() == 0);
    }

    /* Remove all 8 children nodes iff they are all leaf nodes and all empty
       nodes */
    if(allEmpty && allLeaves) {
        _tree->returnChildrenToPool(_children);
        _isLeaf = true;
    }
}

void OctreeNode::keepPoint(OctreePoint& point) {
    point.nodePtr() = this;
    point.isValid() = true;
    arrayAppend(_nodePoints, &point);
}

void OctreeNode::insertPoint(OctreePoint& point) {
    if(_depth == _maxDepth) {
        keepPoint(point);
        return;
    }

    /* Split node if this is a leaf node */
    split();

    /* Compute the index of the child node that contains this point */
    const Vector3 ppos = point.position();
    std::size_t childIdx = 0;
    for(std::size_t dim = 0; dim < 3; ++dim)
        if(_center[dim] < ppos[dim]) childIdx |= (1ull << dim);

    _children->_nodes[childIdx].insertPoint(point);
}

LooseOctree::~LooseOctree() {
    /* Firstly clear data recursively */
    clear();

    /* Deallocate memory pool */
    CORRADE_ASSERT(_numAllocatedNodes ==
        (_freeNodeBlocks.size() + _activeNodeBlocks.size())*8 + 1,
        "Internal data corrupted, maybe all nodes were not returned from the tree", );

    for(OctreeNodeBlock* nodeBlock: _freeNodeBlocks)
        delete nodeBlock;
    for(OctreeNodeBlock* nodeBlock: _activeNodeBlocks)
        delete nodeBlock;
}

void LooseOctree::clear() {
    /* Return all tree nodes to memory pool except the root node */
    _rootNode.removeAllDescendants();

    clearPoints();

    /* Set state to imcomplete build */
    _completeBuild = false;
}

void LooseOctree::clearPoints() {
    /* Recursively clear point data */
    _rootNode.removePointFromSubTree();

    /* Clear the main point data array */
    arrayResize(_octreePoints, Containers::NoInit, 0);
}

void LooseOctree::setPoints(Containers::Array<Vector3>& points) {
    clearPoints();

    arrayResize(_octreePoints, Containers::NoInit, points.size());
    for(std::size_t i = 0; i != points.size(); ++i)
        _octreePoints[i] = OctreePoint{points, i};
}

std::size_t LooseOctree::maxNumPointInNodes() const {
    std::size_t count = 0;
    for(const OctreeNodeBlock* nodeBlock: _activeNodeBlocks)
        for(std::size_t childIdx = 0; childIdx < 8; ++childIdx)
            count = Math::max(count, nodeBlock->_nodes[childIdx].pointCount());

    return count;
}

void LooseOctree::build() {
    /* Compute max depth that the tree can reach */
    _maxDepth = 0;
    std::size_t numLevelNodes = 1;
    std::size_t maxNumTreeNodes = 1;
    Float nodeHalfWidth = _halfWidth;

    while(nodeHalfWidth > _minHalfWidth) {
        ++_maxDepth;
        numLevelNodes *= 8;
        maxNumTreeNodes += numLevelNodes;
        nodeHalfWidth *= 0.5f;
    }

    _rootNode._maxDepth = _maxDepth;
    rebuild();
    _completeBuild = true;

    Debug{} << "Octree info:";
    Debug{} << "  Center:" << _center;
    Debug{} << "  Half width:" << _halfWidth;
    Debug{} << "  Min half width:" << _minHalfWidth;
    Debug{} << "  Max depth:" << _maxDepth;
    Debug{} << "  Max tree nodes:" << maxNumTreeNodes;
}

void LooseOctree::update() {
    if(!_completeBuild) build();

    if(_alwaysRebuild) rebuild();
    else incrementalUpdate();
}

void LooseOctree::rebuild() {
    /* Recursively remove all tree nodes other than root node */
    _rootNode.removeAllDescendants();

    /* Clear root node point data */
    _rootNode.removePointFromSubTree();

    /* Populate all points to tree nodes in a top-down manner */
    populatePoints();
}

void LooseOctree::populatePoints() {
    for(OctreePoint& point: _octreePoints)
        _rootNode.insertPoint(point);
}

void LooseOctree::incrementalUpdate() {
    checkValidity();
    removeInvalidPointsFromNodes();
    reinsertInvalidPointsToNodes();

    /* Recursively remove all empty nodes, returning them to memory pool for
       recycling */
    _rootNode.removeEmptyDescendants();
}

void LooseOctree::checkValidity() {
    const OctreeNode* const rootNodePtr = &_rootNode;
    for(OctreePoint& point: _octreePoints) {
        OctreeNode* pNode = point.nodePtr();
        const Vector3 ppos = point.position();

        if(!pNode->looselyContains(ppos) && pNode != rootNodePtr) {
            /* Go up, find the node tightly containing it (or stop if reached
               the root node) */
            while(pNode != rootNodePtr) {
                pNode = pNode->_parent;
                if(pNode->contains(ppos) || pNode == rootNodePtr) {
                    point.isValid() = false;
                    point.nodePtr() = pNode;
                    break;
                }
            }

        } else point.isValid() = pNode != rootNodePtr;
    }
}

void LooseOctree::removeInvalidPointsFromNodes() {
    for(OctreeNodeBlock* const pNodeBlock: _activeNodeBlocks) {
        for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
            auto& pointList = pNodeBlock->_nodes[childIdx]._nodePoints;
            for(std::size_t i = 0, iend = pointList.size(); i < iend; ++i) {
                OctreePoint* const point = pointList[i];
                if(!point->isValid()) {
                    pointList[i] = pointList[iend - 1];
                    arrayResize(pointList, iend - 1);
                    --iend;
                }
            }
        }
    }
}

void LooseOctree::reinsertInvalidPointsToNodes() {
    for(OctreePoint& point: _octreePoints)
        if(!point.isValid()) _rootNode.insertPoint(point);
}

OctreeNodeBlock* LooseOctree::requestChildrenFromPool() {
    if(_freeNodeBlocks.size() == 0) {
        /* Allocate more node blocks and put to the pool */
        constexpr std::size_t numAllocations = 16;
        for(std::size_t i = 0; i < numAllocations; ++i)
            arrayAppend(_freeNodeBlocks, new OctreeNodeBlock);

        _numAllocatedNodes += numAllocations*8;
    }

    OctreeNodeBlock* const nodeBlock = _freeNodeBlocks.back();
    arrayResize(_freeNodeBlocks, _freeNodeBlocks.size() - 1);
    _activeNodeBlocks.insert(nodeBlock);
    return nodeBlock;
}

void LooseOctree::returnChildrenToPool(OctreeNodeBlock*& nodeBlock) {
    arrayAppend(_freeNodeBlocks, nodeBlock);
    _activeNodeBlocks.erase(nodeBlock);
    nodeBlock = nullptr;
}

}}
