#include <Scene.h>
#include <Camera.h>

#include "AbstractExample.h"
#include "Quad.h"

namespace Magnum { namespace Examples {

class Pool: public AbstractExample {
    public:
        inline Pool(int& argc, char** argv): AbstractExample(argc, argv, "Pool") {
            /* Every scene needs a camera */
            camera = new Camera(&scene);
            camera->setPerspective(deg(35.0f), 0.1f, 100.0f);
            camera->setClearColor(Vector3(0.6f));
            camera->translate({0.0f, 0.25f, 3.0f})->rotate(deg(-15.0f), Vector3::xAxis())->rotate(deg(45.0f), Vector3::yAxis());

            /* Add triangle to the scene */
            new Quad(&scene);
        }

    protected:
        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            camera->setViewport(size);
        }

        inline void drawEvent() {
            camera->draw();
            swapBuffers();
        }

    private:
        Scene scene;
        Camera* camera;
};

}}

MAGNUM_EXAMPLE_MAIN(Magnum::Examples::Pool)
