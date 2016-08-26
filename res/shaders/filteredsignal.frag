#version 430

in vec4 v_position;
in vec2 v_texcoord;

uniform vec2 framebufferSize;
uniform vec4 axesColor;
uniform vec4 lowColor;
uniform vec4 midColor;
uniform vec4 highColor;

uniform int waveformLength;

uniform float allGain;
uniform float lowGain;
uniform float midGain;
uniform float highGain;
uniform float firstVisualIndex;
uniform float lastVisualIndex;


layout(std430, binding = 1) readonly buffer waveformData {
    uint data[];
};

out layout(location = 0) vec4 f_color0;
// Alpha-compsite two colors, putting one on top of the other
vec4 composite(vec4 under, vec4 over) {
    float a_out = 1. - (1. - over.a) * (1. - under.a);
    return clamp(vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out), vec4(0.), vec4(1.));
}


vec4 getWaveformData(uint idx)
{
    return unpackUnorm4x8(data[uint(idx)]);
}

void main(void) {
    f_color0 = vec4(0.);
    vec2 pixel = gl_FragCoord.xy;
    float new_currentIndex = mix(firstVisualIndex,lastVisualIndex, v_texcoord.x);
    vec4 outputColor = vec4(0.0,0.0,0.0,0.0);
    bool lowShowing = false;
    bool midShowing = false;
    bool highShowing = false;
    bool lowShowingUnscaled = false;
    bool midShowingUnscaled = false;
    bool highShowingUnscaled = false;
    // We don't exit early if the waveform data is not valid because we may want
    // to show other things (e.g. the axes lines) even when we are on a pixel
    // that does not have valid waveform data.
    f_color0 = composite(f_color0, vec4(axesColor.rgb, (smoothstep(-3./framebufferSize.y,-2./framebufferSize.y,v_texcoord.y)
                                                      - smoothstep(2./framebufferSize.y,3./framebufferSize.y,v_texcoord.y))));

    if (new_currentIndex >= 0 && new_currentIndex < waveformLength || true) {

      float mag = abs(v_texcoord.y * .9);
      vec4 wav = getWaveformData(uint(new_currentIndex)) * allGain * vec4(lowGain,midGain,highGain,1.);;
      vec3 wf = (wav.rgb - mag) ;
           wf = smoothstep(0.,1.,wf);
      float level = (wav.a - mag);
            level = (smoothstep(-5.,0.,level) - smoothstep(0.,5.,level));
      f_color0 = composite(f_color0, vec4(lowColor.rgb,wf.b));
      f_color0 = composite(f_color0, vec4(midColor.rgb, wf.g));
      f_color0 = composite(f_color0, vec4(highColor.rgb, wf.r));
      f_color0 = composite(f_color0, vec4(0.7, 0.7, 0.7, level * 0.5));
/*
      vec4 new_currentDataUnscaled = getWaveformData(uint(floor(new_currentIndex))) * allGain;
      vec4 new_currentData         = new_currentDataUnscaled;

      new_currentData.x *= lowGain;
      new_currentData.y *= midGain;
      new_currentData.z *= highGain;

      //(vrince) debug see pre-computed signal
      f_color0 = new_currentData;
      f_color0.a = 1.0;

      // Represents the [-1, 1] distance of this pixel. Subtracting this from
      // the signal data in new_currentData, we can tell if a signal band should
      // show in this pixel if the component is > 0.
      float ourDistance = abs(v_texcoord.y);

      vec4 signalDistance = new_currentData - ourDistance;
      lowShowing = signalDistance.x >= 0.0;
      midShowing = signalDistance.y >= 0.0;
      highShowing = signalDistance.z >= 0.0;

      // Now do it all over again for the unscaled version of the waveform,
      // which we will draw at very low opacity.
      vec4 signalDistanceUnscaled = new_currentDataUnscaled - ourDistance;
      lowShowingUnscaled = signalDistanceUnscaled.x >= 0.0;
      midShowingUnscaled = signalDistanceUnscaled.y >= 0.0;
      highShowingUnscaled = signalDistanceUnscaled.z >= 0.0;*/
    }

    // Draw the axes color as the lowest item on the screen.
    // TODO(owilliams): The "4" in this line makes sure the axis gets
    // rendered even when the waveform is fairly short.  Really this
    // value should be based on the size of the widget.
/*    if (abs(framebufferSize.y / 2 - pixel.y) <= 1) {
      outputColor.xyz = mix(outputColor.xyz, axesColor.xyz, axesColor.w);
      outputColor.w = 1.0;
    }

    if (lowShowingUnscaled) {
      float lowAlpha = 0.2;
      outputColor.xyz = mix(outputColor.xyz, lowColor.xyz, lowAlpha);
      outputColor.w = 1.0;
    }
    if (midShowingUnscaled) {
      float midAlpha = 0.2;
      outputColor.xyz = mix(outputColor.xyz, midColor.xyz, midAlpha);
      outputColor.w = 1.0;
    }
    if (highShowingUnscaled) {
      float highAlpha = 0.2;
      outputColor.xyz = mix(outputColor.xyz, highColor.xyz, highAlpha);
      outputColor.w = 1.0;
    }

    if (lowShowing) {
      float lowAlpha = 0.8;
      outputColor.xyz = mix(outputColor.xyz, lowColor.xyz, lowAlpha);
      outputColor.w = 1.0;
    }

    if (midShowing) {
      float midAlpha = 0.85;
      outputColor.xyz = mix(outputColor.xyz, midColor.xyz, midAlpha);
      outputColor.w = 1.0;
    }

    if (highShowing) {
      float highAlpha = 0.9;
      outputColor.xyz = mix(outputColor.xyz, highColor.xyz, highAlpha);
      outputColor.w = 1.0;
    }

    f_color0 = outputColor;*/
}
