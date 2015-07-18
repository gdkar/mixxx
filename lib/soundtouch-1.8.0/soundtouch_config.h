#ifndef SOUNDTOUCH_CONFIG_H
#define SOUNDTOUCH_CONFIG_H

// NOTE(rryan): This file is not included in SoundTouch. It is generated by
// autoconf. None of the usual HAVE_XXX macros autoconf generates are used by
// SoundTouch so I dropped them.

// Do not use x86 optimizations. This is set by our SCons script if we are
// building without optimizations or if the target platform is not x86.
// #undef SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS

// Use Float as Sample type. The Mixxx engine uses float samples.
#define SOUNDTOUCH_FLOAT_SAMPLES 1
#endif /* SOUNDTOUCH_CONFIG_H */
