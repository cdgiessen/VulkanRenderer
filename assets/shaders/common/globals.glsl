
const float PI = 3.14159265359;

layout (set = 0, binding = 0) uniform GlobalData { 
    float time; 
    float delta_time;
    uint32_t frame_count;
}
global;
