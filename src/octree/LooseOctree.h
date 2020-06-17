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

#include <Corrade/Containers/GrowableArray.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector3.h>

#include <unordered_set>

namespace Magnum { namespace Examples {
/* Forward declaration */
class OctreeNode;
class LooseOctree;
struct OctreeNodeBlock;

class OctreePoint {
public:
    OctreePoint() = default;
    OctreePoint(Containers::Array<Vector3>* points_, std::size_t idx_) : _points(points_), _idx(idx_) {}
    std::size_t idx() const { return _idx; }
    Vector3 position() const { return (*_points)[_idx]; }
    OctreeNode*& nodePtr() { return _node; }
    bool& isValid() { return _valid; }

private:
    const Containers::Array<Vector3>* _points; /* the original point set */
    std::size_t _idx;                          /* index of this point in the original point set */
    OctreeNode* _node { nullptr };             /* pointer to the octree node containing this point */

    /* Flag to keep track of point validity
     * During tree update, it is true if:
     *  1) the point is still contained in the tree node that it has previously been inserted to, and
     *  2) depth of the current node reaches maxDepth
     */
    bool _valid { true };
};

class OctreeNode {
    friend class LooseOctree;
public:
    OctreeNode(const OctreeNode&) = delete;
    OctreeNode& operator=(const OctreeNode&) = delete;

    /* Default constructor called during memory allocation */
    OctreeNode() :
        _tree(nullptr),
        _parent(nullptr),
        _children(nullptr),
        _center(Vector3(0, 0, 0)),
        _halfWidth(0),
        _depth(0),
        _maxDepth(0),
        _isLeaf(true) {}

    /* Constructor called during node splitting */
    explicit OctreeNode(LooseOctree* const tree, OctreeNode* const parent,
                        const Vector3& nodeCenter, const Float halfWidth,
                        const std::size_t depth);

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

    /*
     * Recursively remove point list from the subtree
     * The point data still exists in the main octree list
     */
    void removePointFromSubTree();

    /* Split node (requesting 8 children nodes from memory pool) */
    void split();

    /*
     * Recursively remove all descendant nodes (return them back to memory pool)
     * As a result, after calling to this function, the current node will become a leaf node
     */
    void removeAllDescendants();

    /*
     * Recursively remove all descendant nodes that are empty
     *  (all 8 children of a node are removed at the same time)
     */
    void removeEmptyDescendants();

    /* Keep the point at this node as cannot pass it down further to any child node */
    void keepPoint(OctreePoint* const pPoint);

    /* Insert a point point into the subtree in a top-down manner */
    void insertPoint(OctreePoint* const pPoint);

    /*
     * Check if the given point is contained in the node boundary (bounding box)
     */
    bool contains(const Vector3& point) const { return _bounds.contains(point); }

    /*
     * Check if the given point is contained in the node loose boundary (which is 2X bigger than the bounding box)
     */
    bool looselyContains(const Vector3& point) const { return _boundsExtended.contains(point); }

    /*
     * Check if the given bound is contained in the node loose boundary (which is 2X bigger than the bounding box)
     */
    bool looselyContains(const Vector3& lower, const Vector3& upper) const {
        return (lower > _boundsExtended.min()).all() && (upper < _boundsExtended.max()).all();
    }

    /*
     * Check if the given bound is overlapped with the node loose boundary (which is 2X bigger than the bounding box)
     */
    bool looselyOverlaps(const Vector3& lower, const Vector3& upper) const {
        return (upper > _boundsExtended.min()).all() && (lower < _boundsExtended.max()).all();
    }

private:
    const Vector3     _center;         /* center of this node */
    const Range3D     _bounds;         /* the lower/upper bound of the node */
    const Range3D     _boundsExtended; /* the extended bounds of the node, which is 2X bigger than the exact AABB */
    const Float       _halfWidth;      /* the half width of the node */
    const std::size_t _depth;          /* depth of this node (depth > 0, depth = 1 starting at the root node) */

    LooseOctree*     _tree;            /* pointer to the octree */
    OctreeNode*      _parent;          /* pointer to the parent node */
    OctreeNodeBlock* _children;        /* pointer to a memory block containing 8 children nodes */
    std::size_t      _maxDepth;        /* the maximum depth level possible */
    bool             _isLeaf;

    /* Store all octree points holding at this node */
    Containers::Array<OctreePoint*> _nodePoints;
};

/*
 * This is a data structure to store a memory block of 8 tree nodes at a time
 * This can reduce node allocation/merging/slitting overhead
 */
struct OctreeNodeBlock {
    OctreeNode _nodes[8];
};

/*
 * Loose octree: each tree node has a loose boundary which is exactly twice big as its exact boundary
 * During tree update, a primitive is moved around from node to node
 * If removed from a node, the primitive is moving up to find the lowest node that exactly contains it,
 *  then it is move down to the lowest level possible, stopping at a node that loosely contains it
 */
class LooseOctree {
    friend class OctreeNode;
public:
    /*
     * \param center The center of the tree, which also is the center of the root node
     * \param width Width of the octree bounding box
     * \param minWidth Minimum allowed width of the tree nodes
     */
    explicit LooseOctree(const Vector3& center, const Float halfWidth, const Float minHalfWidth) :
        _center(center),
        _halfWidth(halfWidth),
        _minHalfWidth(minHalfWidth),
        _rootNode(this, nullptr, center, halfWidth, 0),
        _numAllocatedNodes(1u) {}

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

    /*
     * Set points data for the tree
     * (the points will not be populated to tree nodes until calling to build())
     * The given point set will overwrite the existing points in the tree
     *  (only one point set is allowed at a time)
     */
    void setPoints(Containers::Array<Vector3>& points);

    /* Count the maximum number of points stored in a tree node */
    std::size_t maxNumPointInNodes() const;

    /*
     * true: rebuilt the tree from scratch in every update
     * false: incrementally update from the current state
     */
    void setAlwaysRebuild(const bool alwaysRebuild) { _alwaysRebuild = alwaysRebuild; }

    /* Get all memory block of active nodes */
    const std::unordered_set<OctreeNodeBlock*>& getActiveTreeNodeBlocks() const
    { return _activeNodeBlocks; }

    /* Build the tree for the first time */
    void build();

    /* Update tree after data has changed */
    void update();

private:
    /* Rebuild the tree from scratch */
    void rebuild();

    /* Populate point to tree nodes, from top (root node) down to leaf nodes */
    void populatePoints();

    /* Incrementally update octree from current state */
    void incrementalUpdate();

    /* For each point, check if it is still loosely contained in the tree node */
    void checkValidity();

    /* Remove all invalid points from the tree nodes previously contained them */
    void removeInvalidPointsFromNodes();

    /*
     * For each invalid point, insert it back to the tree in a top-down manner
     * starting from the lowest ancestor node that tightly contains it (that node was found during validity check)
     */
    void reinsertInvalidPointsToNodes();

    /*
     * Request a block of 8 tree nodes from memory pool (this is called only during splitting node)
     * If the memory pool is exhausted, 64 more blocks will be allocated from the system memory
     */
    OctreeNodeBlock* requestChildrenFromPool();

    /*
     * Return 8 children nodes to memory pool (this is called only during destroying descendant nodes)
     */
    void returnChildrenToPool(OctreeNodeBlock*& pNodeBlock);

    const Vector3 _center;     /* center of the tree */
    const Float   _halfWidth;  /* width of the tree bounding box */

    const Float _minHalfWidth; /* minimum width allowed for the tree nodes */
    std::size_t _maxDepth;     /* max depth of the tree, which is computed based on _minWidth */

    OctreeNode _rootNode;      /* root node, should not be reassigned throughout the existence of the tree */

    /* Store the free node blocks (8 nodes) that can be used right away */
    Containers::Array<OctreeNodeBlock*> _freeNodeBlocks;

    /* Set of node blocks that are in use (node blocks that have been taken from memory pool)
     * Need to be a set to easily erase */
    std::unordered_set<OctreeNodeBlock*> _activeNodeBlocks;

    std::size_t _numAllocatedNodes; /* count the total number of allocated nodes so far */

    /* Store octree point data */
    Containers::Array<OctreePoint> _octreePoints;

    bool _alwaysRebuild { false };
    bool _completeBuild { false };
};
} }

#endif
