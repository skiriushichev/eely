#include <bgfx_shader.sh>

uniform vec4 u_color = vec4(1.0, 1.0, 1.0, 1.0);

void main() {
    gl_FragColor = u_color;
}
