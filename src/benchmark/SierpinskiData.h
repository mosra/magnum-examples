#ifndef SierpinskiData_h
#define SierpinskiData_h

#include <Magnum.h>

class SierpinskiData {
    public:
        SierpinskiData(size_t iterations);

        std::vector<Magnum::Vector4> vertices;
        std::vector<Magnum::Vector3> colors;
        std::vector<unsigned int> indices;

    private:
        void subdivide(size_t iteration, size_t first, size_t second, size_t third, size_t fourth);

        size_t iterations;
};

#endif
