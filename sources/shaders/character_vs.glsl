struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
  vec2 UV;
  vec3 BoneColor;
};

uniform mat4 Transform;
uniform mat4 ViewProjection;


layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 UV;
layout(location = 3) in vec4 BoneWeights;
layout(location = 4) in uvec4 BoneIndex;

out VsOutput vsOutput;

vec3 getBoneColor(uint boneIndex)
{
  float h = fract(sin(float(boneIndex) * 12.9898) * 43758.5453);
  float s = 0.8;
  float v = 0.9;
  float c = v * s;
  float x = c * (1.0 - abs(mod(h * 6.0, 2.0) - 1.0));
  float m = v - c;
  
  vec3 rgb;
  if (h < 0.166666)
    rgb = vec3(c, x, 0.0);
  else if (h < 0.333333)
    rgb = vec3(x, c, 0.0);
  else if (h < 0.5)
    rgb = vec3(0.0, c, x);
  else if (h < 0.666666)
    rgb = vec3(0.0, x, c);
  else if (h < 0.833333)
    rgb = vec3(x, 0.0, c);
  else
    rgb = vec3(c, 0.0, x);
  
  return rgb + m;
}

void main()
{

  vec3 VertexPosition = (Transform * vec4(Position, 1)).xyz;
  vsOutput.EyespaceNormal = (Transform * vec4(Normal, 0)).xyz;

  gl_Position = ViewProjection * vec4(VertexPosition, 1);
  vsOutput.WorldPosition = VertexPosition;

  vsOutput.UV = UV;

  vec3 boneColor = vec3(0.0);
  if (BoneWeights.x > 0.0)
    boneColor += BoneWeights.x * getBoneColor(BoneIndex.x);
  if (BoneWeights.y > 0.0)
    boneColor += BoneWeights.y * getBoneColor(BoneIndex.y);
  if (BoneWeights.z > 0.0)
    boneColor += BoneWeights.z * getBoneColor(BoneIndex.z);
  if (BoneWeights.w > 0.0)
    boneColor += BoneWeights.w * getBoneColor(BoneIndex.w);
  
  vsOutput.BoneColor = boneColor;

}