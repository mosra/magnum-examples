#ifndef Magnum_Examples_OctreeExample_LooseOctree_h
#define Magnum_Examples_OctreeExample_LooseOctree_h
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

#include <unordered_set>
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Reference.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector3.h>

namespace Magnum { namespace Examples {

class OctreeNode;
class LooseOctree;
struct OctreeNodeBlock;

class OctreePoint {
    public:
        explicit OctreePoint(Containers::Array<Vector3>& points, std::size_t idx):
            _points{points}, _idx{idx} {}

        std::size_t idx() const { return _idx; }
        Vector3 position() const { return (*_points)[_idx]; }
        OctreeNode*& nodePtr() { return _node; }
        bool& isValid() { return _valid; }

    private:
        /* the original point set */
        Containers::Reference<const Containers::Array<Vector3>> _points;

        /* index of this point in the original point set */
        std::size_t _idx;

        /* pointer to the octree node containing this point */
        OctreeNode* _node = nullptr;

        /* Flag to keep track of point validity. During tree update, it is
           true if:

           1. the point is still contained in the tree node that it has
              previously been inserted to, and
           2. depth of the current node reaches maxDepth */
        bool _valid = true;
};

class OctreeNode {
    public:
        /* Default constructor called during memory allocation */
        OctreeNode() = default;

        /* Constructor called during node splitting */
        explicit OctreeNode(LooseOctree* tree, OctreeNode* parent,
            const Vector3& nodeCenter, Float halfWidth, std::size_t depth);

        OctreeNode(const OctreeNode&) = delete;
        OctreeNode& operator=(const OctreeNode&) = delete;

        /* Common properties */
        bool isLeaf() const { return _isLeaf; }
        const Vector3 center() const { return _center; }
        Float halfWidth() const { return _halfWidth; }
        std::size_t depth() const { return _depth; }

        /* Get a child node (child idx from 0 to 7) */
        const OctreeNode& childNode(const std::size_t childIdx) const;

        /* Return the point list in the current node */
        const Containers::Array<OctreePoint*>& pointList() const { return _nodePoints; }

        /* Return the number of points holding at this node */
        std::size_t pointCount() const { return _nodePoints.size(); }

        /* Recursively remove point list from the subtree. The point data still
           exists in the main octree list. */
        void removePointFromSubTree();

        /* Split node (requesting 8 children nodes from memory pool) */
        void split();

        /* Recursively remove all descendant nodes (return them back to memory
           pool). As a result, after calling to this function, the current node
           will become a leaf node. */
        void removeAllDescendants();

        /* Recursively remove all descendant nodes that are empty (all 8
           children of a node are removed at the same time) */
        void removeEmptyDescendants();

        /* Keep the point at this node as cannot pass it down further to any
           child node */
        void keepPoint(OctreePoint& point);

        /* Insert a point point into the subtree in a top-down manner */
        void insertPoint(OctreePoint& point);

        /* Check if given point is contained in the node boundary (bounding
           box) */
        bool contains(const Vector3& point) const {
            return _bounds.contains(point);
        }

        /* Check if given point is contained in the node loose boundary
           (which is 2x bigger than the bounding box) */
        bool looselyContains(const Vector3& point) const {
            return _boundsExtended.contains(point);
        }

        /* Check if given bound is contained in the node loose boundary (which
           is 2x bigger than the bounding box) */
        bool looselyContains(const Range3D& bounds) const {
            return _boundsExtended.contains(bounds);
        }

        /* Check if the given bound is overlapped with the node loose boundary
           (which is 2X bigger than the bounding box) */
        bool looselyOverlaps(const Range3D& bounds) const {
            return Math::intersects(bounds, _boundsExtended);
        }

    private:
        friend LooseOctree;

        /* center of this node */
        const Vector3 _center;

        /* lower/upper bound of the node */
        const Range3D _bounds;

        /* extended bounds of the node, 2x bigger than the exact AABB */
        const Range3D _boundsExtended;

        /* half width of the node */
        const Float _halfWidth = 0;

        /* depth of this node (depth > 0, depth = 1 starting at the root node) */
        const std::size_t _depth = 0;

        /* pointer to the octree */
        LooseOctree* _tree = nullptr;

        /* pointer to the parent node */
        OctreeNode* _parent = nullptr;

        /* pointer to a memory block containing 8 children nodes */
        OctreeNodeBlock* _children = nullptr;

        /* maximum depth level possible */
        std::size_t _maxDepth = 0;

        bool _isLeaf = true;

        /* Store all octree points holding at this node */
        Containers::Array<OctreePoint*> _nodePoints;
};

/* Data structure to store a memory block of 8 tree nodes at a time. This can
   reduce node allocation/merging/slitting overhead. */
struct OctreeNodeBlock {
    OctreeNode _nodes[8];
};

/* Loose octree: each tree node has a loose boundary which is exactly twice big
   as its exact boundary. During tree update, a primitive is moved around from
   node to node. If removed from a node, the primitive is moving up to find the
   lowest node that exactly contains it, then it is move down to the lowest
   level possible, stopping at a node that loosely contains it. */
class LooseOctree {
    public:
        /* Center is the center of the tree, which also is the center of the
           root node; width is of the octree bounding box; minWidth is minimum
           allowed width of the tree nodes. */
        explicit LooseOctree(const Vector3& center, const Float halfWidth,
            const Float minHalfWidth): _center{center}, _halfWidth{halfWidth},
            _minHalfWidth{minHalfWidth}, _rootNode{this, nullptr, center,
            halfWidth, 0}, _numAllocatedNodes{1} {}

        /* Cleanup memory here */
        ~LooseOctree();

        /* Common properties */
        const Vector3 center() const { return _center; }
        const OctreeNode& rootNode() const { return _rootNode; }
        Float halfWidth() const { return _halfWidth; }
        Float minHalfWidth() const { return _minHalfWidth; }
        std::size_t maxDepth() const { return _maxDepth; }
        std::size_t numAllocatedNodes() const { return _numAllocatedNodes; }

        /* Clear all data, but still keep allocated nodes in memory pool */
        void clear();

        /* Completely remove all octree point data */
        void clearPoints();

        /* Set points data for the tree. The points will not be populated to
           tree nodes until calling to build(). The given point set will
           overwrite the existing points in the tree (only one point set is
           allowed at a time). */
        void setPoints(Containers::Array<Vector3>& points);

        /* Count the maximum number of points stored in a tree node */
        std::size_t maxNumPointInNodes() const;

        /* True rebuilds the tree from scratch in every update, false
           incrementally updates from the current state */
        void setAlwaysRebuild(const bool alwaysRebuild) {
            _alwaysRebuild = alwaysRebuild;
        }

        /* Get all memory block of active nodes */
        const std::unordered_set<OctreeNodeBlock*>& activeTreeNodeBlocks() const {
            return _activeNodeBlocks;
        }

        /* Build the tree for the first time */
        void build();

        /* Update tree after data has changed */
        void update();

    private:
        friend OctreeNode;

        /* Rebuild the tree from scratch */
        void rebuild();

        /* Populate point to tree nodes, from top (root node) down to leaf
           nodes */
        void populatePoints();

        /* Incrementally update octree from current state */
        void incrementalUpdate();

        /* For each point, check if it is still loosely contained in the tree
           node */
        void checkValidity();

        /* Remove all invalid points from the tree nodes previously contained
           them */
        void removeInvalidPointsFromNodes();

        /* For each invalid point, insert it back to the tree in a top-down
           manner starting from the lowest ancestor node that tightly contains
           it (that node was found during validity check) */
        void reinsertInvalidPointsToNodes();

        /* Request a block of 8 tree nodes from memory pool (this is called
           only during splitting node). If the memory pool is exhausted, 64
           more blocks will be allocated from the system memory. */
        OctreeNodeBlock* requestChildrenFromPool();

        /* Return 8 children nodes to memory pool (this is called only during
           destroying descendant nodes) */
        void returnChildrenToPool(OctreeNodeBlock*& pNodeBlock);

        /* center of the tree */
        const Vector3 _center;

        /* width of the tree bounding box */
        const Float   _halfWidth;

        /* minimum width allowed for the tree nodes */
        const Float _minHalfWidth;

        /* max depth of the tree, which is computed based on _minWidth */
        std::size_t _maxDepth;

        /* root node, should not be reassigned throughout the existence of the
           tree */
        OctreeNode _rootNode;

        /* Store the free node blocks (8 nodes) that can be used right away */
        Containers::Array<OctreeNodeBlock*> _freeNodeBlocks;

        /* Set of node blocks that are in use (node blocks that have been taken
           from memory pool). Need to be a set to easily erase. */
        std::unordered_set<OctreeNodeBlock*> _activeNodeBlocks;

        /* Count of the total number of allocated nodes so far */
        std::size_t _numAllocatedNodes;

        /* Store octree point data */
        Containers::Array<OctreePoint> _octreePoints;

        bool _alwaysRebuild = false;
        bool _completeBuild = false;
};

}}

#endif
