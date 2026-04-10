#version 330 core

in vec3 vPosition;
in vec2 vUV;
in vec3 vWorldPos;

uniform int scale_0;
uniform int scale_1;
uniform float line_scale_0;
uniform float line_scale_1;
uniform vec4 color_0;
uniform vec4 color_1;

out vec4 FragColor;

float pristineGrid(vec2 uv, vec2 lineWidth)
{
    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);
    
    vec2 uvDeriv = vec2(length(vec2(ddx.x, ddy.x)), length(vec2(ddx.y, ddy.y)));
    bvec2 invertLine = bvec2(lineWidth.x > 0.5, lineWidth.y > 0.5);
    
    vec2 targetWidth = vec2(
        invertLine.x ? 1.0 - lineWidth.x : lineWidth.x,
        invertLine.y ? 1.0 - lineWidth.y : lineWidth.y
    );
    
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * 1.5;
    vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
    
    gridUV.x = invertLine.x ? gridUV.x : 1.0 - gridUV.x;
    gridUV.y = invertLine.y ? gridUV.y : 1.0 - gridUV.y;
    
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    
    grid2 *= clamp(targetWidth / drawWidth, 0.0, 1.0);
    grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));
    grid2.x = invertLine.x ? 1.0 - grid2.x : grid2.x;
    grid2.y = invertLine.y ? 1.0 - grid2.y : grid2.y;
    
    return mix(grid2.x, 1.0, grid2.y);
}

// Volume grid effect using world position
float volumeGrid(vec3 worldPos, float scale, float lineWidth)
{
    vec3 gridCoord = worldPos * scale;
    
    // Grid in XY plane
    vec2 gridUV_xy = fract(gridCoord.xy);
    vec2 gridDelta_xy = abs(vec2(dFdx(gridCoord.x), dFdy(gridCoord.y)));
    
    // Grid in XZ plane
    vec2 gridUV_xz = fract(gridCoord.xz);
    vec2 gridDelta_xz = abs(vec2(dFdx(gridCoord.x), dFdy(gridCoord.z)));
    
    // Grid in YZ plane
    vec2 gridUV_yz = fract(gridCoord.yz);
    vec2 gridDelta_yz = abs(vec2(dFdy(gridCoord.y), dFdy(gridCoord.z)));
    
    // Convert to grid lines (0 at lines, 1 at centers)
    vec2 grid_xy = abs(gridUV_xy * 2.0 - 1.0);
    vec2 grid_xz = abs(gridUV_xz * 2.0 - 1.0);
    vec2 grid_yz = abs(gridUV_yz * 2.0 - 1.0);
    
    // Anti-alias
    vec2 lineAA_xy = gridDelta_xy * 1.5;
    vec2 lineAA_xz = gridDelta_xz * 1.5;
    vec2 lineAA_yz = gridDelta_yz * 1.5;
    
    // Apply smoothstep for anti-aliasing
    vec2 line_xy = smoothstep(lineWidth + lineAA_xy, lineWidth - lineAA_xy, grid_xy);
    vec2 line_xz = smoothstep(lineWidth + lineAA_xz, lineWidth - lineAA_xz, grid_xz);
    vec2 line_yz = smoothstep(lineWidth + lineAA_yz, lineWidth - lineAA_yz, grid_yz);
    
    // Combine all three planes
    float grid_lines = max(max(max(line_xy.x, line_xy.y), max(line_xz.x, line_xz.y)), max(line_yz.x, line_yz.y));
    
    return grid_lines;
}

void main()
{
    // Surface grid (UV-based)
    float grid_0 = pristineGrid(vUV * float(scale_0), vec2(line_scale_0));
    float grid_1 = pristineGrid(vUV * float(scale_1), vec2(line_scale_1));
    
    // Dark gray background
    vec3 final_color = vec3(0.2, 0.2, 0.2);
    
    // Blend big orange grid lines
    final_color = mix(final_color, color_0.rgb, vec3(grid_0));
    
    // Blend small white grid lines
    final_color = mix(final_color, color_1.rgb, vec3(grid_1));
    
    FragColor = vec4(final_color, 1.0);
}
