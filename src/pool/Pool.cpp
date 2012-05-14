#include <array>
#include <PluginManager/PluginManager.h>
#include <Scene.h>
#include <Camera.h>
#include <Light.h>
#include <Trade/AbstractImporter.h>

#include "AbstractExample.h"
#include "Quad.h"
#include "Ducks.h"

#include "configure.h"

using namespace std;
using namespace Corrade::PluginManager;

namespace Magnum { namespace Examples {

class Pool: public AbstractExample {
    public:
        Pool(int& argc, char** argv): AbstractExample(argc, argv, "Pool"), manager(PLUGIN_IMPORTER_DIR) {
            /* Every scene needs a camera */
            camera = new Camera(&scene);
            camera->setPerspective(deg(35.0f), 0.1f, 100.0f);
            camera->setClearColor({0.9f, 0.95f, 1.0f});
            camera->translate({0.0f, 0.0f, 3.5f})->rotate(deg(-15.0f), Vector3::xAxis())->rotate(deg(25.0f), Vector3::yAxis());
            Camera::setFeature(Camera::Feature::DepthTest, true);

            /* Light */
            array<Light*, PoolShader::LightCount> lights;
            (lights[0] = new Light(&scene))->translate({0.0f, -3.0f, 0.0f});
            (lights[1] = new Light(&scene))->translate({-0.0f, 2.5f, -0.0f});

            /* Add quad and duck */
            new Quad(&manager, lights, &scene);
            Ducks::tryCreate(&manager, lights[1], &scene);
        }

    protected:
        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            camera->setViewport(size);
        }

        inline void drawEvent() {
            camera->draw();
            swapBuffers();

            Corrade::Utility::sleep(30);
            redraw();
        }

        virtual void keyEvent(Key key, const Magnum::Math::Vector2<int>& position) {
            if(key == Key::Left)
                camera->rotate(deg(5.0f), Vector3::yAxis());
            else if(key == Key::Right)
                camera->rotate(deg(-5.0f), Vector3::yAxis());
            else if(key == Key::Up)
                camera->rotate(deg(-5.0f), camera->transformation()[0].xyz());
            else if(key == Key::Down)
                camera->rotate(deg(5.0f), camera->transformation()[0].xyz());
            else if(key == Key::PageUp)
                camera->translate(Vector3::zAxis(-0.2f), Object::Transformation::Local);
            else if(key == Key::PageDown)
                camera->translate(Vector3::zAxis(0.2f), Object::Transformation::Local);

            redraw();
        }

    private:
        PluginManager<Trade::AbstractImporter> manager;
        Scene scene;
        Camera* camera;
};

}}

MAGNUM_EXAMPLE_MAIN(Magnum::Examples::Pool)
