#pragma once

#include <string>

inline const std::string FRAG_HB_IMG9 = R"#(
precision highp float;
varying vec4 v_color;
varying vec2 v_texcoord;

uniform sampler2D tex;
uniform vec2 texSize;
uniform vec4 texSplit;

uniform vec2 topLeft;
uniform vec2 fullSize;
uniform vec2 fullSizeUntransformed;

void main() {
    highp vec2 pixCoord = vec2(gl_FragCoord);
    highp vec2 pixCoordOuter = pixCoord;
    highp vec2 originalPixCoord = v_texcoord;
    originalPixCoord *= fullSizeUntransformed;

    pixCoord -= topLeft;

    // center the pixes dont make it top-left
    //pixCoord += vec2(0.5, 0.5);

    vec4 pixColor = v_color;

    float x0 = pixCoord[0];
    float x1 = pixCoord[0] - fullSize[0];
    float y0 = pixCoord[1];
    float y1 = pixCoord[1] - fullSize[1];
    bool sides = false;
  
    // left/right 
    if (x0 >= 0.0 && x0 < texSplit[0]) {
      pixCoord[0] = x0;
      sides = true;
    } else if (x1 >= -(texSize[0] - texSplit[2]) && x1 < 0.0 ) {
      pixCoord[0] = x1 + texSize[0];
      sides = true;
    }

    if (sides) {
      // corners
      if (y0 >= 0.0 && y0 < texSplit[1]) {
        pixCoord[1] = y0;
      } else if (y1 >= -(texSize[1] - texSplit[3]) && y1 < 0.0 ) {
        pixCoord[1] = y1 + texSize[1];
      }
      // sides
      else {
        pixCoord[1] = mod((y0 - texSplit[1]), (texSplit[3]-texSplit[1])) + texSplit[1];
      }
      pixColor = texture2D(tex, pixCoord/texSize);
    }
 
    else {
      // top/bottom
      if (y0 >= 0.0 && y0 < texSplit[1]) {
        pixCoord[1] = y0;
        sides = true;
      } else if (y1 >= -(texSize[1] - texSplit[3]) && y1 < 0.0 ) {
        pixCoord[1] = y1 + texSize[1];
        sides = true;
      }

      if (sides) {
        pixCoord[0] = mod((x0 - texSplit[0]), (texSplit[2]-texSplit[0])) + texSplit[0];
        pixColor = texture2D(tex, pixCoord/texSize);
      }
    }

    if (pixColor.a == 0.0) {
      discard;
    }

    gl_FragColor = pixColor;
}
)#";


inline const std::string FRAG_HB_IMG9N = R"#(
precision highp float;
varying vec4 v_color;
varying vec2 v_texcoord;

uniform sampler2D tex;        // Border texture
uniform sampler2D normalMap;  // Normal map for lighting calculations
uniform vec2 texSize;         // Texture size in pixels
uniform vec4 texSplit;        // Slicing regions: [left, top, right, bottom]

uniform vec2 topLeft;         // Top-left position of the border
uniform vec2 fullSize;        // Size of the scaled border area
uniform vec2 fullSizeUntransformed; // Untransformed size of the target area
  
uniform vec2 lightPos;        // Light source position in 2D (screen space)
uniform vec3 lightColor;      // Light color (RGB)
uniform float lightIntensity; // Intensity of the light

void main() {
    highp vec2 pixCoord = vec2(gl_FragCoord);  // Current fragment coordinate
    highp vec2 pixCoordOuter = pixCoord;
    highp vec2 originalPixCoord = v_texcoord;
    originalPixCoord *= fullSizeUntransformed;

    pixCoord -= topLeft;
    
    vec3 lightDir = vec3(lightPos - pixCoord, 300.0);
    vec4 pixColor = v_color;

    float x0 = pixCoord[0];
    float x1 = pixCoord[0] - fullSize[0];
    float y0 = pixCoord[1];
    float y1 = pixCoord[1] - fullSize[1];
    bool sides = false;
  
    // Determine the slice region (9-slice logic)
    // Left/Right Sides
    if (x0 >= 0.0 && x0 < texSplit[0]) {
        pixCoord[0] = x0;
        sides = true;
    } else if (x1 >= -(texSize[0] - texSplit[2]) && x1 < 0.0 ) {
        pixCoord[0] = x1 + texSize[0];
        sides = true;
    }

    if (sides) {
        // Corners
        if (y0 >= 0.0 && y0 < texSplit[1]) {
            pixCoord[1] = y0;
        } else if (y1 >= -(texSize[1] - texSplit[3]) && y1 < 0.0 ) {
            pixCoord[1] = y1 + texSize[1];
        }
        // Sides
        else {
            pixCoord[1] = mod((y0 - texSplit[1]), (texSplit[3]-texSplit[1])) + texSplit[1];
        }
        pixColor = texture2D(tex, pixCoord / texSize);
    } else {
        // Top/Bottom Sides
        if (y0 >= 0.0 && y0 < texSplit[1]) {
            pixCoord[1] = y0;
            sides = true;
        } else if (y1 >= -(texSize[1] - texSplit[3]) && y1 < 0.0 ) {
            pixCoord[1] = y1 + texSize[1];
            sides = true;
        }

        if (sides) {
            pixCoord[0] = mod((x0 - texSplit[0]), (texSplit[2]-texSplit[0])) + texSplit[0];
            pixColor = texture2D(tex, pixCoord / texSize);
        }
    }

    if (pixColor.a == 0.0) {
        //if(length(lightDir.xy) > 10.0)
          discard;
        //else
        //  pixColor = vec4(1.0,1.0,1.0,1.0);
    }

    // Fetch the normal from the normal map
    vec3 normal = texture2D(normalMap, pixCoord / texSize).rgb;
    normal = normalize(normal * 2.0 - 1.0); // Convert from [0,1] to [-1,1]
    normal.y *= -1.0;

    // Calculate light direction
    //vec2 lightDir = lightPos - pixCoord;
    float distance = length(lightDir) * 0.010;
    lightDir = normalize(lightDir);

    // Calculate diffuse lighting
    float diffuse = max(dot(normal, lightDir), 0.0);

    // Attenuation based on distance
    float attenuation = lightIntensity / (1.0 + 0.1 * distance + 0.01 * distance * distance);

    // Combine light and color
    vec3 lighting = lightColor * diffuse * attenuation;

    // Final pixel color
    gl_FragColor = vec4(pixColor.rgb * lighting, pixColor.a);
}
)#";

// makes a stencil with chamfered corners
// omitted gradient handling entirely
inline const std::string FRAG_HB_CHAM = R"#(
precision highp float;
varying vec4 v_color;
varying vec2 v_texcoord;

uniform vec2 topLeft;
uniform vec2 fullSize;
uniform vec2 fullSizeUntransformed;
uniform float radius;
uniform float radiusOuter;
uniform float thick;

uniform vec4 f_color;
uniform float alpha;

void main() {

    highp vec2 pixCoord = vec2(gl_FragCoord);
    highp vec2 pixCoordOuter = pixCoord;
    highp vec2 originalPixCoord = v_texcoord;
    originalPixCoord *= fullSizeUntransformed;
    float additionalAlpha = 1.0;
    float sqrt2 = 1.4142135;

    vec4 pixColor = vec4(1.0, 1.0, 1.0, 1.0);

    bool done = false;

    pixCoord -= topLeft + fullSize * 0.5;
    pixCoord *= vec2(lessThan(pixCoord, vec2(0.0))) * -2.0 + 1.0;

    // center the pixes dont make it top-lef6
    pixCoord += vec2(1.0, 1.0) / fullSize;
    pixCoordOuter = pixCoord;

    pixCoord -= fullSize * 0.5 - radius;
    pixCoordOuter -= fullSize * 0.5 - radiusOuter;

    if (min(pixCoord.x, pixCoord.y) > (-sqrt2 + 1.0) * thick && radius > 0.0) {
        float s = pixCoord.x + pixCoord.y;
        float sO = pixCoordOuter.x + pixCoordOuter.y;
        if (s < radius) {
            float normalized = smoothstep(1.0, 0.0, (radius - thick * sqrt2) - s);
            additionalAlpha *= normalized;
        } else
            additionalAlpha = 0.0;
        done = true;
    } 

    // now check for other shit
    if (!done) {
        // distance to all straight bb borders
        float distanceT = originalPixCoord[1];
        float distanceB = fullSizeUntransformed[1] - originalPixCoord[1];
        float distanceL = originalPixCoord[0];
        float distanceR = fullSizeUntransformed[0] - originalPixCoord[0];

        // get the smallest
        float smallest = min(min(distanceT, distanceB), min(distanceL, distanceR));
        
        if (smallest > thick)
            discard;
    }

    if (additionalAlpha == 0.0)
        discard;

    pixColor = f_color;
    pixColor.rgb *= pixColor[3];

    pixColor *= alpha * additionalAlpha;

    gl_FragColor = pixColor;
    gl_FragColor[3] = 1.0;
}
)#";
