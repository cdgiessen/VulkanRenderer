#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout(location = 0) in VS_OUT {
    vec3 normal;
} gs_in[];

layout(location = 0) out vec4 v_color;

const float MAGNITUDE = 1.0;

void GenerateLine(int index)
{
    gl_Position = gl_in[index].gl_Position;
	v_color = vec4(1.0, 1.0, 0.0, 1.0);
    EmitVertex();
   
	gl_Position = gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * MAGNITUDE;
	v_color = vec4(1.0, 0.0, 1.0, 1.0);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    GenerateLine(0); // first vertex normal
    GenerateLine(1); // second vertex normal
    GenerateLine(2); // third vertex normal
}  