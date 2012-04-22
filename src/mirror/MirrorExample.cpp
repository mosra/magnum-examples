/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include <functional>
#include "Scene.h"
#include "Camera.h"
#include "MeshTools/GenerateFlatNormals.h"
#include "MeshTools/CombineIndexedArrays.h"
#include "Primitives/Cube.h"
#include "Primitives/Icosphere.h"
#include "Primitives/Capsule.h"
#include "Primitives/UVSphere.h"
#include "Shaders/PhongShader.h"

#include "FpsCounterExample.h"
#include "Floor.h"
#include "Player.h"
#include "PointLight.h"

using namespace std;
using namespace Corrade::Utility;
using namespace Magnum;
using namespace Magnum::Shaders;

namespace Magnum { namespace Examples {

class MirrorExample: public FpsCounterExample {
    public:
        inline MirrorExample(int& argc, char** argv): FpsCounterExample(argc, argv, "Mirror example") {
            PointLight* light = new PointLight(&scene);
            light->translate({-10.0f, 30.0f, 10.0f});

            /* Floor */
            (new Floor(&shader, light, &scene))
                ->scale(Vector3(50.0f))->translate(Vector3::yAxis(-1.0f));

            /* Cube */
            Primitives::Cube cube;
            vector<unsigned int> normalIndices;
            tie(normalIndices, *cube.normals(0)) = MeshTools::generateFlatNormals(*cube.indices(), *cube.vertices(0));
            *cube.indices() = MeshTools::combineIndexedArrays(
                make_tuple(cref(*cube.indices()), ref(*cube.vertices(0))),
                make_tuple(cref(normalIndices), ref(*cube.normals(0)))
            );
            (new Solid(cube, &shader, light, {0.0f, 0.0f, 0.8f}, &scene))
                ->translate({3.5f, 0.0f, -3.0f});

            /* Icosphere */
            Primitives::Icosphere<0> icosphere;
            tie(normalIndices, *icosphere.normals(0)) = MeshTools::generateFlatNormals(*icosphere.indices(), *icosphere.vertices(0));
            *icosphere.indices() = MeshTools::combineIndexedArrays(
                make_tuple(cref(*icosphere.indices()), ref(*icosphere.vertices(0))),
                make_tuple(cref(normalIndices), ref(*icosphere.normals(0)))
            );
            (new Solid(icosphere, &shader, light, {0.0f, 0.5f, 0.0f}, &scene))
                ->scale(Vector3(1.5f))->translate({-2.5f, 0.0f, -2.5f});

            /* Player */
            player = new Player(&shader, light, &scene);
            player->rotate(deg(5.0f), Vector3::yAxis());

            /* Player feet */
            (new Solid(Primitives::Capsule(16, 32, 2.5f), &shader, light, {0.6f, 0.2f, 0.2f}, player))
                ->scale(Vector3(0.4f))->translate({1.1f, 0.0f, 0.0f});
            (new Solid(Primitives::Capsule(16, 32, 2.5f), &shader, light, {0.6f, 0.2f, 0.2f}, player))
                ->scale(Vector3(0.4f))->translate({-1.1f, 0.0f, 0.0f});

            /* Player hands */
            (new Solid(Primitives::Capsule(16, 32, 2.5f), &shader, light, {0.6f, 0.2f, 0.2f}, player))
                ->scale(Vector3(0.4f))->rotate(deg(90.0f), Vector3::xAxis())->translate({0.75f, -1.85f, -0.3f});
            (new Solid(Primitives::Capsule(16, 32, 2.5f), &shader, light, {0.6f, 0.2f, 0.2f}, player))
                ->scale(Vector3(0.4f))->rotate(deg(90.0f), Vector3::xAxis())->translate({-0.75f, -1.85f, -0.3f});

            /* Player sword */
            playerSword = new Solid(Primitives::Capsule(16, 32, 4.5f), &shader, light, {0.0f, 0.0f, 0.0f}, player);
            playerSword->scale(Vector3(0.3f))->rotate(deg(-30.0f), Vector3::xAxis())
                ->rotate(deg(-45.0f), Vector3::yAxis())->translate({0.75f, -0.0f, -0.7f});
            (new Solid(Primitives::Capsule(16, 32, 15.5f), &shader, light, {0.0f, 1.0f, 1.0f}, playerSword))
                ->translate(Vector3::yAxis(15.0f))->scale(Vector3(0.43f));

            /* Player hair */
            (new Solid(Primitives::UVSphere(16, 32), &shader, light, {1.0f, 1.0f, 0.7f}, player))
                ->scale(Vector3(1.05f))->translate({0.0f, 1.3f, 0.3f});

            /* Enemy */
            Object* enemy = new Solid(Primitives::Capsule(16, 32, 2.5f), &shader, light, Vector3(0.25f), &scene);
            enemy->rotate(deg(180.0f), Vector3::yAxis())->translate({0.0f, 1.25f, -10.0f});

            /* Enemy armor */
            (new Solid(Primitives::Icosphere<0>(), &shader, light, Vector3(0.2f), enemy))
                ->scale(Vector3(1.4f))->translate({0.0f, 0.0f, 0.0f});

            /* Enemy beard */
            (new Solid(Primitives::Icosphere<0>(), &shader, light, Vector3(0.20f), enemy))
                ->scale(Vector3(0.4f))->translate({0.0f, 1.3f, -0.7f});

            /* Enemy feet */
            (new Solid(Primitives::Capsule(16, 32, 2.5f), &shader, light, Vector3(0.25f), enemy))
                ->scale(Vector3(0.4f))->translate({1.1f, 0.0f, 0.0f});
            (new Solid(Primitives::Capsule(16, 32, 2.5f), &shader, light, Vector3(0.25f), enemy))
                ->scale(Vector3(0.4f))->translate({-1.1f, 0.0f, 0.0f});

            /* Enemy hands */
            (new Solid(Primitives::Capsule(16, 32, 2.5f), &shader, light, Vector3(0.25f), enemy))
                ->scale(Vector3(0.4f))->rotate(deg(90.0f), Vector3::xAxis())->translate({0.75f, -1.85f, -0.3f});
            (new Solid(Primitives::Capsule(16, 32, 2.5f), &shader, light, Vector3(0.25f), enemy))
                ->scale(Vector3(0.4f))->rotate(deg(90.0f), Vector3::xAxis())->translate({-0.75f, -1.85f, -0.3f});

            /* Enemy sword */
            enemySword = new Solid(Primitives::Capsule(16, 32, 4.5f), &shader, light, Vector3(0.15f), enemy);
            enemySword->scale(Vector3(0.3f))->rotate(deg(-30.0f), Vector3::xAxis())
                ->rotate(deg(-45.0f), Vector3::yAxis())->translate({0.75f, -0.0f, -0.7f});
            (new Solid(Primitives::Capsule(16, 32, 15.5f), &shader, light, {0.9f, 0.0f, 0.0f}, enemySword))
                ->translate(Vector3::yAxis(15.0f))->scale(Vector3(0.43f));

            /* Enemy coat */
            (new Solid(Primitives::Cube(), &shader, light, Vector3(0.05f), enemy))
                ->scale({1.0f, 1.2f, 0.02f})->rotate(deg(-45.0f), Vector3::xAxis())->translate({0.0f, -0.2f, 1.2f});

            scene.setFeature(Scene::DepthTest, true);
            scene.setFeature(Scene::FaceCulling, true);

            /* Grab mouse and hide cursor */
            grabMouse = true;
            setMouseCursor(MouseCursor::None);
            setMouseTracking(true);

            setFpsCounterEnabled(true);
            setPrimitiveCounterEnabled(true);
            setSampleCounterEnabled(true);
        }

    protected:
        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            player->camera()->setViewport(size);
            warpMouseCursor(player->camera()->viewport()/2);
        }

        inline void drawEvent() {
            player->camera()->draw();
            swapBuffers();

            if(grabMouse)
                warpMouseCursor(player->camera()->viewport()/2);

            sleep(5);
            redraw();
        }

        void keyEvent(Key key, const Magnum::Math::Vector2< int >& position) {
            if(key == Key::End) {
                grabMouse = !grabMouse;
                setMouseCursor(grabMouse ? MouseCursor::None : MouseCursor::Default);
            } else if(key == Key::Left)
                player->translate(Vector3::xAxis(-0.125f), false);
             else if(key == Key::Right)
                player->translate(Vector3::xAxis(0.125f), false);
             else if(key == Key::Up)
                player->translate(Vector3::zAxis(-0.35f), false);
             else if(key == Key::Down)
                player->translate(Vector3::zAxis(0.35f), false);
        }

        virtual void mouseEvent(MouseButton button, MouseState state, const Magnum::Math::Vector2< int >& position) {
            if(state == MouseState::Down) {
                enemySword->rotate(deg(30.0f), Vector3::yAxis());
                playerSword->rotate(deg(30.0f), Vector3::yAxis());
            } else {
                enemySword->rotate(deg(-30.0f), Vector3::yAxis());
                playerSword->rotate(deg(-30.0f), Vector3::yAxis());
            }
        }

        void mouseMoveEvent(const Math::Vector2<int>& position) {
            if(!grabMouse) return;

            player->rotate(rad(PI*(position.x()*2.0f/player->camera()->viewport().x() - 1)), Vector3::yAxis(), false);

            /* Don't rotate under the floor */
            Matrix4 yRotation = Matrix4::rotation(rad(PI*(position.y()*2.0f/player->camera()->viewport().y() - 1)), Vector3::xAxis())*player->camera()->transformation();
            if(abs(Vector3::dot((yRotation.rotation()*Vector3::yAxis()).normalized(), Vector3(0.0f, 1.0f, -0.5f).normalized()) < 0.6f))
                return;

            player->camera()->setTransformation(yRotation);
        }

    private:
        PhongShader shader;
        Scene scene;
        Player* player;
        bool grabMouse;

        Object* enemySword;
        Object* playerSword;
};

}}

MAGNUM_EXAMPLE_MAIN(Magnum::Examples::MirrorExample)
