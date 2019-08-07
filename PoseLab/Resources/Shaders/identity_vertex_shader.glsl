#version 330 core

in  vec4 qt_vertex;       // POINTS TO VERTICES PROVIDED BY USER ON CPU
out vec2 qt_coordinate;   // OUTPUT COORDINATE TO FRAGMENT SHADER

void main(void)
{
    // COPY THE VERTEX COORDINATE TO THE GL POSITION
    gl_Position = qt_vertex;
    qt_coordinate = (vec2(qt_vertex.x, -qt_vertex.y) + 1.0)/2.0;
}
