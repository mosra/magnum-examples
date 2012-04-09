#include <Camera.h>
#include <Scene.h>

#include "FpsCounterExample.h"
#include "Sierpinski.h"

using namespace Corrade::Utility;
using namespace Magnum;
using namespace Magnum::Examples;

enum Mode {
    DepthCull,
    DepthNoCull,
    NoDepthCull,
    NoDepthNoCull
};

class Benchmark: public FpsCounterExample {
    public:
        Benchmark(int& argc, char** argv): FpsCounterExample(argc, argv, "Benchmark"), interpolated(true), flat(false), iterations(0), tipsify(false), mode(DepthCull) {
            Debug() << "Depth test enabled, face culling enabled";
            Debug() << "Using shader with color interpolation";

            camera = new Camera(&scene);
            camera->setPerspective(deg(35.0f), 0.1f, 100.0f);
            camera->translate({0.0f, 0.0f, 3.0f});

            o = new Sierpinski(&scene);
            o->setIterations(iterations, tipsify);
            o->shader = &interpolated;

            scene.setFeature(Scene::Feature::DepthTest, true);
            scene.setFeature(Scene::Feature::FaceCulling, true);

            setMinimalCounterDuration(10.0f);
            setFpsCounterEnabled(true);
            setPrimitiveCounterEnabled(true);
            setSampleCounterEnabled(true);
        }

        inline ~Benchmark() {
            printCounterStatistics();
        }

    protected:
        void viewportEvent(const Math::Vector2<GLsizei>& size) {
            camera->setViewport(size);

            FpsCounterExample::viewportEvent(size);
        }

        void drawEvent() {
            camera->draw();
            swapBuffers();
            camera->rotate(deg(0.02f), {1.0f, 1.0f, 1.0f});
            redraw();
        }

        void keyEvent(Key key, const Magnum::Math::Vector2<int>& position) {
            printCounterStatistics();
            resetCounter();

            if(key == Key::PageUp)
                o->setIterations(++iterations, tipsify);
            else if(key == Key::PageDown && iterations != 0)
                o->setIterations(--iterations, tipsify);
            else if(key == Key::F1)
                o->setIterations(iterations, (tipsify = !tipsify));
            else if(key == Key::F2) {
                mode = (mode+1)%4;

                switch(mode) {
                    case Mode::DepthCull:
                        Debug() << "Depth test enabled, face culling enabled";
                        scene.setFeature(Scene::DepthTest, true);
                        scene.setFeature(Scene::FaceCulling, true);
                        break;
                    case Mode::DepthNoCull:
                        Debug() << "Depth test enabled, face culling disabled";
                        scene.setFeature(Scene::DepthTest, true);
                        scene.setFeature(Scene::FaceCulling, false);
                        break;
                    case Mode::NoDepthCull:
                        Debug() << "Depth test disabled, face culling enabled";
                        scene.setFeature(Scene::DepthTest, false);
                        scene.setFeature(Scene::FaceCulling, true);
                        break;
                    case Mode::NoDepthNoCull:
                        Debug() << "Ddepth test disabled, face culling disabled";
                        scene.setFeature(Scene::DepthTest, false);
                        scene.setFeature(Scene::FaceCulling, false);
                        break;
                }
            } else if(key == Key::F3) {
                if(o->shader == &interpolated) {
                    Debug() << "Using shader without color interpolation";
                    o->shader = &flat;
                } else {
                    Debug() << "Using shader with color interpolation";
                    o->shader = &interpolated;
                }
            }

            resetCounter();
            redraw();
        }

    private:
        Scene scene;
        Camera* camera;
        ColorShader interpolated, flat;
        Sierpinski* o;
        size_t iterations;
        bool tipsify;
        int mode;
};

MAGNUM_EXAMPLE_MAIN(Benchmark)
