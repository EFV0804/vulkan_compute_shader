#version 450 core

layout (local_size_x = 256) in;

struct Vertex{
    vec3 pos;
    vec3 col;
};

layout(set = 0, binding=0) buffer inBuffer{
    Vertex vertices[];
} inData;


layout(set = 0, binding=1) buffer outBuffer{

    Vertex vertices[];

}outData;


void main(void) {
    int global_id = int(gl_GlobalInvocationID.x);

    outData.vertices[global_id].pos = inData.vertices[global_id].pos;
}
