#version 450

//layout (location = 0) in vec3 positions;
//layout (location = 1) in vec3 colors;
//
//layout (location = 0) out vec3 frag_color;
//
//void main() {
//    gl_Position = vec4(positions, 1.0f);
//    frag_color = colors;
//}

layout (location = 0) out vec3 frag_color;

void main()
{
    //const array of positions for the triangle
    const vec3 positions[3] = vec3[3](
    vec3(1.f, 1.f, 0.0f),
    vec3(-1.f, 1.f, 0.0f),
    vec3(0.f, -1.f, 0.0f)
    );

    //const array of colors for the triangle
    const vec3 colors[3] = vec3[3](
    vec3(1.0f, 0.0f, 0.0f), //red
    vec3(0.0f, 1.0f, 0.0f), //green
    vec3(00.f, 0.0f, 1.0f)  //blue
    );

    //output the position of each vertex
    gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
    frag_color = colors[gl_VertexIndex];
}