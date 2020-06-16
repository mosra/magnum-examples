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

namespace Magnum { namespace Examples {
OctreeNode::OctreeNode(LooseOctree* const tree, OctreeNode* const pParent,
                       const Vector3& nodeCenter, const Float halfWidth,
                       const size_t depth) :
    _pTree(tree),
    _pParent(pParent),
    _pChildren(nullptr),
    _center(nodeCenter),
    _lowerBound(nodeCenter - Vector3(halfWidth, halfWidth, halfWidth)),
    _upperBound(nodeCenter + Vector3(halfWidth, halfWidth, halfWidth)),
    _lowerBoundExtended(nodeCenter - 2 * Vector3(halfWidth, halfWidth, halfWidth)),
    _upperBoundExtended(nodeCenter + 2 * Vector3(halfWidth, halfWidth, halfWidth)),
    _halfWidth(halfWidth),
    _depth(depth),
    _maxDepth(tree->_maxDepth),
    _bIsLeaf(true) {}

OctreeNode* OctreeNode::getChildNode(const std::size_t childIdx) const {
    CORRADE_INTERNAL_ASSERT(_pChildren != nullptr);
    return &_pChildren->_nodes[childIdx];
}

void OctreeNode::removePointFromSubTree() {
    _nodePoints.clear();
    if(!isLeaf()) {
        for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
            _pChildren->_nodes[childIdx].removePointFromSubTree();
        }
    }
}

void OctreeNode::split() {
    if(!isLeaf() || _depth == _maxDepth) {
        return;
    }

    if(isLeaf()) {
        /*--------------------------------------------------------
         *
         *           6-------7
         *          /|      /|
         *         2-+-----3 |
         *         | |     | |   y
         *         | 4-----+-5   | z
         *         |/      |/    |/
         *         0-------1     +--x
         *
         *         0   =>   0, 0, 0
         *         1   =>   0, 0, 1
         *         2   =>   0, 1, 0
         *         3   =>   0, 1, 1
         *         4   =>   1, 0, 0
         *         5   =>   1, 0, 1
         *         6   =>   1, 1, 0
         *         7   =>   1, 1, 1
         *
         *--------------------------------------------------------*/

        _pChildren = _pTree->requestChildrenFromPool();

        const auto childHalfWidth = _halfWidth * static_cast<Float>(0.5);
        for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
            auto newCenter = _center;
            newCenter[0] += (childIdx & 1) ? childHalfWidth : -childHalfWidth;
            newCenter[1] += (childIdx & 2) ? childHalfWidth : -childHalfWidth;
            newCenter[2] += (childIdx & 4) ? childHalfWidth : -childHalfWidth;

            OctreeNode* const pChildNode = &_pChildren->_nodes[childIdx];

            /* Placement new: re-use existing memory block, just call constructor to re-initialize data */
            new(pChildNode) OctreeNode(_pTree, this, newCenter, childHalfWidth, _depth + 1u);
        }

        /* Must explicitly mark as non-leaf node, and must do this after all children nodes are ready */
        _bIsLeaf = false;
    }
}

void OctreeNode::removeAllDescendants() {
    if(isLeaf()) {
        return;
    }

    for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
        _pChildren->_nodes[childIdx].removeAllDescendants();
    }
    _pTree->returnChildrenToPool(_pChildren);
    _bIsLeaf = true;
}

void OctreeNode::removeEmptyDescendants() {
    if(isLeaf()) {
        return;
    }

    bool bAllEmpty  = true;
    bool bAllLeaves = true;
    for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
        auto& pChildNode = _pChildren->_nodes[childIdx];
        pChildNode.removeEmptyDescendants();
        bAllLeaves &= pChildNode.isLeaf();
        bAllEmpty  &= (pChildNode.getPointCount() == 0);
    }

    /* Remove all 8 children nodes iff they are all leaf nodes and all empty nodes */
    if(bAllEmpty && bAllLeaves) {
        _pTree->returnChildrenToPool(_pChildren);
        _bIsLeaf = true;
    }
}

void OctreeNode::keepPoint(OctreePoint* const pPoint) {
    pPoint->getNode() = this;
    pPoint->isValid() = true;
    _nodePoints.push_back(pPoint);
}

void OctreeNode::insertPoint(OctreePoint* const pPoint) {
    if(_depth == _maxDepth) {
        keepPoint(pPoint);
        return;
    }

    /* Split node if this is a leaf node */
    split();

    /* Compute the index of the child node that contains this point */
    const Vector3 ppos     = pPoint->getPosition();
    std::size_t   childIdx = 0;
    for(std::size_t dim = 0; dim < 3; ++dim) {
        if(_center[dim] < ppos[dim]) {
            childIdx |= (1ull << dim);
        }
    }
    _pChildren->_nodes[childIdx].insertPoint(pPoint);
}

LooseOctree::~LooseOctree() {
    /* Firstly clear data recursively */
    clear();

    /* Deallocate memory pool */
    if(_numAllocatedNodes != (_freeNodeBlocks.size() + _activeNodeBlocks.size()) * 8 + 1u) {
        Fatal{} << "Internal data corrupted, may be all nodes were not returned from tree";
    }

    for(const auto pNodeBlock: _freeNodeBlocks) {
        delete pNodeBlock;
    }
    for(const auto pNodeBlock: _activeNodeBlocks) {
        delete pNodeBlock;
    }

    /* Remove root node */
    delete _pRootNode;
}

void LooseOctree::clear() {
    /* Return all tree nodes to memory pool except the root node */
    _pRootNode->removeAllDescendants();

    clearPoints();

    /* Set state to imcomplete build */
    _bCompleteBuild = false;
}

void LooseOctree::clearPoints() {
    /* Recursively clear point data */
    _pRootNode->removePointFromSubTree();

    /* Clear the main point data array */
    _octreePoints.clear();
}

void LooseOctree::addPointSet(std::vector<Vector3>& points) {
    const std::size_t nPoints = points.size();
    if(nPoints == 0) {
        return;
    }
    const std::size_t oldNPoints = _octreePoints.size();
    _octreePoints.resize(oldNPoints + nPoints);
    for(std::size_t idx = oldNPoints; idx < oldNPoints + nPoints; ++idx) {
        const auto pPoint = &_octreePoints[idx];
        new(pPoint) OctreePoint(&points, idx); /* placement new */
    }
}

std::size_t LooseOctree::getMaxNumPointInNodes() const {
    std::size_t count { 0 };
    for(const OctreeNodeBlock* pNodeBlock : _activeNodeBlocks) {
        for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
            const auto& pNode = pNodeBlock->_nodes[childIdx];
            if(count < pNode.getPointCount()) {
                count = pNode.getPointCount();
            }
        }
    }
    return count;
}

void LooseOctree::build() {
    /* Compute max depth that the tree can reach */
    _maxDepth = 0u;
    std::size_t numLevelNodes   = 1u;
    std::size_t maxNumTreeNodes = 1u;
    Float       nodeHalfWidth   = _halfWidth;

    while(nodeHalfWidth > _minHalfWidth) {
        ++_maxDepth;
        numLevelNodes   *= 8;
        maxNumTreeNodes += numLevelNodes;
        nodeHalfWidth   *= static_cast<Float>(0.5);
    }
    _pRootNode->_maxDepth = _maxDepth;
    rebuild();
    _bCompleteBuild = true;

    Debug() << "Octree info:";
    Debug() << "       Center:" << _center;
    Debug() << "       Half width:" << _halfWidth;
    Debug() << "       Min half width:" << _minHalfWidth;
    Debug() << "       Max depth:" << _maxDepth;
    Debug() << "       Max tree nodes:" << maxNumTreeNodes;
}

void LooseOctree::update() {
    if(!_bCompleteBuild) { build(); }
    (!_bAlwaysRebuild) ? incrementalUpdate() : rebuild();
}

void LooseOctree::rebuild() {
    /* Recursively remove all tree nodes other than root node */
    _pRootNode->removeAllDescendants();

    /* Clear root node point data */
    _pRootNode->removePointFromSubTree();

    /* Populate all points to tree nodes in a top-down manner */
    populatePoints();
}

void LooseOctree::populatePoints() {
    for(auto& point : _octreePoints) {
        _pRootNode->insertPoint(&point);
    }
}

void LooseOctree::incrementalUpdate() {
    checkValidity();
    removeInvalidPointsFromNodes();
    reinsertInvalidPointsToNodes();

    /* Recursively remove all empty nodes, returning them to memory pool for recycling */
    _pRootNode->removeEmptyDescendants();
}

void LooseOctree::checkValidity() {
    for(OctreePoint& point : _octreePoints) {
        OctreeNode*   pNode = point.getNode();
        const Vector3 ppos  = point.getPosition();
        if(!pNode->looselyContains(ppos) && pNode != _pRootNode) {
            /* Go up, find the node tightly containing it (or stop if reached the root node) */
            while(pNode != _pRootNode) {
                pNode = pNode->_pParent;
                if(pNode->contains(ppos) || pNode == _pRootNode) {
                    point.isValid() = false;
                    point.getNode() = pNode;
                    break;
                }
            }
        } else {
            point.isValid() = (pNode != _pRootNode) ? true : false;
        }
    }
}

void LooseOctree::removeInvalidPointsFromNodes() {
    for(OctreeNodeBlock* const pNodeBlock: _activeNodeBlocks) {
        for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
            auto& pointList = pNodeBlock->_nodes[childIdx]._nodePoints;
            for(size_t i = 0, iend = pointList.size(); i < iend; ++i) {
                OctreePoint* pPoint = pointList[i];
                if(!pPoint->isValid()) {
                    pointList[i] = pointList[iend - 1];
                    pointList.resize(iend - 1);
                    --iend;
                }
            }
        }
    }
}

void LooseOctree::reinsertInvalidPointsToNodes() {
    for(OctreePoint& point : _octreePoints) {
        if(!point.isValid()) {
            _pRootNode->insertPoint(&point);
        }
    }
}

OctreeNodeBlock* LooseOctree::requestChildrenFromPool() {
    if(_freeNodeBlocks.size() == 0) {
        /* Allocate more node blocks and put to the pool */
        static constexpr std::size_t numAllocations { 16 };
        for(std::size_t i = 0; i < numAllocations; ++i) {
            const auto pNodeBlock = new OctreeNodeBlock;
            _freeNodeBlocks.push_back(pNodeBlock);
        }
        _numAllocatedNodes += numAllocations * 8;
    }

    const auto pNodeBlock = _freeNodeBlocks.back();
    _freeNodeBlocks.pop_back();
    _activeNodeBlocks.insert(pNodeBlock);
    return pNodeBlock;
}

void LooseOctree::returnChildrenToPool(OctreeNodeBlock*& pNodeBlock) {
    _freeNodeBlocks.push_back(pNodeBlock);
    _activeNodeBlocks.erase(pNodeBlock);
    pNodeBlock = nullptr;
}
} }
