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
// This is copied from: https://github.com/jrl-umi3218/SpaceVecAlg
inline double sinc_inv(double x)
{
    const double taylor_0_bound = std::numeric_limits<double>::epsilon();
    const double taylor_2_bound = std::sqrt(taylor_0_bound);
    const double taylor_n_bound = std::sqrt(taylor_2_bound);

    // We use the 4th order taylor series around 0 of x/sin(x) to compute
    // this function:
    //      2      4
    //     x    7⋅x     ⎛ 6⎞
    // 1 + ── + ──── + O⎝x ⎠
    //     6    360
    // this approximation is valid around 0.
    // if x is far from 0, our approximation is not valid
    // since x^6 becomes non negligable we use the normal computation of the function
    // (i.e. taylor_2_bound^6 + taylor_0_bound == taylor_0_bound but
    //       taylor_n_bound^6 + taylor_0_bound != taylor_0).

    if (std::abs(x) >= taylor_n_bound)
        return (x / std::sin(x));

    // x is bellow taylor_n_bound so we don't care of the 6th order term of
    // the taylor series.
    // We set the 0 order term.
    double result = 1.;

    if (std::abs(x) >= taylor_0_bound) {
        // x is above the machine epsilon so x^2 is meaningful.
        double x2 = x * x;
        result += x2 / 6.;

        if (std::abs(x) >= taylor_2_bound) {
            // x is upper the machine sqrt(epsilon) so x^4 is meaningful.
            result += 7. * (x2 * x2) / 360.;
        }
    }

    return result;
}

inline Eigen::Vector3d rotation_error(const Eigen::Matrix3d& R_ab, const Eigen::Matrix3d& R_ac)
{
    Eigen::Matrix3d E_bc = R_ac * R_ab.transpose();

    Eigen::Vector3d w;
    double acos_v = (E_bc.trace() - 1) * 0.5;
    double theta = std::acos(std::min(std::max(acos_v, -1.), 1.));

    w << -E_bc(2, 1) + E_bc(1, 2),
        -E_bc(0, 2) + E_bc(2, 0),
        -E_bc(1, 0) + E_bc(0, 1);

    w *= sinc_inv(theta) * 0.5;

    // return R_ab.transpose() * w; // this is in local frame
    return w;
}

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