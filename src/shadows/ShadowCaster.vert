/* magnum-shadows - A Cascading/Parallel-Split Shadow Mapping example
 * Written in 2016 by Bill Robinson airbaggins@gmail.com
 *
 * To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to
 * this software to the public domain worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * Credit is appreciated, but not required.
 * */
uniform highp mat4 transformationMatrix;

in highp vec4 position;

void main()
{
    gl_Position = transformationMatrix * position;
}
