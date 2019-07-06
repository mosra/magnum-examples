#ifndef MAGNUmatEXAMPLES_DART_EXAMPLE_HELPERS_H
#define MAGNUmatEXAMPLES_DART_EXAMPLE_HELPERS_H

/*
    This file is part of Magnum.
    Original authors — credit is appreciated but not required:
        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
        2019 — Konstantinos Chatzilygeroudis <costashatz@gmail.com>
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

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/SVD>

#include <dart/dynamics/BodyNode.hpp>
#include <dart/dynamics/BoxShape.hpp>
#include <dart/dynamics/FreeJoint.hpp>
#include <dart/dynamics/Skeleton.hpp>
#include <dart/dynamics/WeldJoint.hpp>

namespace {
inline dart::dynamics::SkeletonPtr createBox(const std::string& name = "box", const Eigen::Vector3d& color = Eigen::Vector3d(0.8, 0., 0.))
{
    const double box_size = 0.06;

    const double box_density = 260; // kg/m^3
    const double box_mass = box_density * std::pow(box_size, 3.);
    /* Create a Skeleton with the given name */
    dart::dynamics::SkeletonPtr box = dart::dynamics::Skeleton::create(name);

    /* Create a body for the box */
    dart::dynamics::BodyNodePtr body =
        box->createJointAndBodyNodePair<dart::dynamics::FreeJoint>(nullptr).second;

    /* Create a shape for the box */
    std::shared_ptr<dart::dynamics::BoxShape> box_shape(
        new dart::dynamics::BoxShape(Eigen::Vector3d(box_size,
                                        box_size,
                                        box_size)));
    auto shapeNode = body->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(box_shape);
    shapeNode->getVisualAspect()->setColor(color);

    /* Set up inertia for the box */
    dart::dynamics::Inertia inertia;
    inertia.setMass(box_mass);
    inertia.setMoment(box_shape->computeInertia(box_mass));
    body->setInertia(inertia);

    box->getDof("Joint_pos_z")->setPosition(box_size / 2.0);

    return box;
}

inline dart::dynamics::SkeletonPtr createFloor()
{
    /* Create a floor */
    dart::dynamics::SkeletonPtr floorSkel = dart::dynamics::Skeleton::create("floor");
    /* Give the floor a body */
    dart::dynamics::BodyNodePtr body =
        floorSkel->createJointAndBodyNodePair<dart::dynamics::WeldJoint>(nullptr).second;

    /* Give the floor a shape */
    double floor_width = 10.0;
    double floor_height = 0.1;
    std::shared_ptr<dart::dynamics::BoxShape> box(
            new dart::dynamics::BoxShape(Eigen::Vector3d(floor_width, floor_width, floor_height)));
    auto shapeNode
        = body->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(box);
    shapeNode->getVisualAspect()->setColor(Eigen::Vector3d(0.3, 0.3, 0.4));

    /* Put the floor into position */
    Eigen::Isometry3d tf(Eigen::Isometry3d::Identity());
    tf.translation() = Eigen::Vector3d(0.0, 0.0, -floor_height / 2.0);
    body->getParentJoint()->setTransformFromParentBodyNode(tf);

    return floorSkel;
}}

#endif