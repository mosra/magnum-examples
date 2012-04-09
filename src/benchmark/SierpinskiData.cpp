#include "SierpinskiData.h"

#include <cstdlib>

using namespace Magnum;

SierpinskiData::SierpinskiData(size_t iterations): vertices{
    {0.0f, 0.5f, 0.0f},
    {-0.5f, -0.5f, 0.5f},
    {0.5f, -0.5f, 0.5f},
    {0.0f, -0.5f, -0.5f}
}, iterations(iterations) {
    subdivide(0, 0, 1, 2, 3);
}

void SierpinskiData::subdivide(size_t iteration, size_t first, size_t second, size_t third, size_t fourth) {
    if(iteration == iterations) {
        indices.push_back(second);
        indices.push_back(third);
        indices.push_back(first);

        indices.push_back(third);
        indices.push_back(fourth);
        indices.push_back(first);

        indices.push_back(fourth);
        indices.push_back(second);
        indices.push_back(first);

        indices.push_back(second);
        indices.push_back(fourth);
        indices.push_back(third);

    } else {
        size_t firstThird = vertices.size();
        vertices.push_back((vertices[first]+vertices[third])/2);

        size_t firstSecond = vertices.size();
        vertices.push_back((vertices[first]+vertices[second])/2);

        size_t firstFourth = vertices.size();
        vertices.push_back((vertices[first]+vertices[fourth])/2);

        size_t secondThird = vertices.size();
        vertices.push_back((vertices[second]+vertices[third])/2);

        size_t secondFourth = vertices.size();
        vertices.push_back((vertices[second]+vertices[fourth])/2);

        size_t thirdFourth = vertices.size();
        vertices.push_back((vertices[third]+vertices[fourth])/2);

        subdivide(iteration+1, first, firstSecond, firstThird, firstFourth);
        subdivide(iteration+1, firstSecond, second, secondThird, secondFourth);
        subdivide(iteration+1, firstThird, secondThird, third, thirdFourth);
        subdivide(iteration+1, firstFourth, thirdFourth, secondFourth, fourth);
    }

    /* Generate random colors */
    if(iteration == 0) {
        for(size_t i = 0; i != vertices.size(); ++i) {
            Vector3 color;
            for(size_t j = 0; j != 3; ++j)
                color[j] = rand()/float(RAND_MAX);
            colors.push_back(color);
        }
    }
}
