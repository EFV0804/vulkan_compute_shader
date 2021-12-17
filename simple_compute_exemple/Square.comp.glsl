#version 450 core

layout (local_size_x = 256) in;


layout(set=0, binding=0) readonly buffer inBuffer{
    int inData[];
};


layout(set=0, binding=1) buffer outBuffer{
    int outData[];
};


void main(void) {
    int global_id = int(gl_GlobalInvocationID.x);

    outData[global_id] = inData[global_id] * inData[global_id];
}
