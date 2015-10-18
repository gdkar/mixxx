# -*- coding: utf-8 -*-

import os
import util
from mixxx import Feature
import SCons.Script as SCons
import depends
class HSS1394(Feature):
    def description(self): return "HSS1394 MIDI device support"
    def enabled(self, build):
        if build.platform_is_windows or build.platform_is_osx:
            build.flags['hss1394'] = util.get_flags(build.env, 'hss1394', 1)
        else:
            build.flags['hss1394'] = util.get_flags(build.env, 'hss1394', 0)
        if int(build.flags['hss1394']): return True
        return False
    def add_options(self, build, vars):
        if build.platform_is_windows or build.platform_is_osx:
            vars.Add('hss1394','Set to 1 to enable HSS1394 MIDI device support.', 1)
    def configure(self, build, conf):
        if not self.enabled(build): return
        if build.platform_is_windows or build.platform_is_osx:
#            if not conf.CheckHeader('HSS1394/HSS1394.h'):  # WTF this gives tons of cmath errors on MSVC
#                raise Exception('Did not find HSS1394 development headers')
            if not conf.CheckLib(['libhss1394', 'hss1394']):
                raise Exception('Did not find HSS1394 development library')
        build.env.Append(CPPDEFINES='__HSS1394__')
        if build.platform_is_windows and build.static_dependencies: conf.CheckLib('user32')
    def sources(self, build):
        return ['controllers/midi/hss1394controller.cpp',
                'controllers/midi/hss1394enumerator.cpp']
class HID(Feature):
    HIDAPI_INTERNAL_PATH = '#lib/hidapi-0.8.0-pre'
    def description(self): return "HID controller support"
    def enabled(self, build):
        build.flags['hid'] = util.get_flags(build.env, 'hid', 1)
        if int(build.flags['hid']): return True
        return False
    def add_options(self, build, vars): vars.Add('hid', 'Set to 1 to enable HID controller support.', 1)
    def configure(self, build, conf):
        if not self.enabled(build): return
        # TODO(XXX) allow external hidapi install, but for now we just use our
        # internal one.
        build.env.Append( CPPPATH=[os.path.join(self.HIDAPI_INTERNAL_PATH, 'hidapi')])
        if build.platform_is_linux:
            build.env.ParseConfig(
                'pkg-config libusb-1.0 --silence-errors --cflags --libs')
            if (not conf.CheckLib(['libusb-1.0', 'usb-1.0']) or not conf.CheckHeader('libusb-1.0/libusb.h')):
                raise Exception( 'Did not find the libusb 1.0 development library or its header file')
            # Optionally add libpthread and librt. Some distros need this.
            conf.CheckLib(['pthread', 'libpthread'])
            conf.CheckLib(['rt', 'librt'])
            # -pthread tells GCC to do the right thing regardless of system
            build.env.Append(CCFLAGS='-pthread')
            build.env.Append(LINKFLAGS='-pthread')
        elif build.platform_is_windows and not conf.CheckLib(['setupapi', 'libsetupapi']):
            raise Exception('Did not find the setupapi library, exiting.')
        elif build.platform_is_osx: build.env.AppendUnique(FRAMEWORKS=['IOKit', 'CoreFoundation'])
        build.env.Append(CPPDEFINES='__HID__')
    def sources(self, build):
        sources = ['controllers/hid/hidcontroller.cpp',
                   'controllers/hid/hidenumerator.cpp',
                   'controllers/hid/hidcontrollerpresetfilehandler.cpp']
        if build.platform_is_windows:
            # Requires setupapi.lib which is included by the above check for
            # setupapi.
            sources.append( os.path.join(self.HIDAPI_INTERNAL_PATH, "windows/hid.c"))
        elif build.platform_is_linux:
            sources.append( os.path.join(self.HIDAPI_INTERNAL_PATH, 'linux/hid-libusb.c'))
        elif build.platform_is_osx:
            sources.append( os.path.join(self.HIDAPI_INTERNAL_PATH, 'mac/hid.c'))
        return sources
class Bulk(Feature):
    def description(self): return "USB Bulk controller support"
    def enabled(self, build):
        # For now only make Bulk default on Linux only. Turn on for all
        # platforms after the 1.11.0 release.
        is_default = 1 if build.platform_is_linux else 0
        build.flags['bulk'] = util.get_flags(build.env, 'bulk', is_default)
        if int(build.flags['bulk']): return True
        return False
    def add_options(self, build, vars):
        is_default = 1 if build.platform_is_linux else 0
        vars.Add('bulk', 'Set to 1 to enable USB Bulk controller support.', is_default)
    def configure(self, build, conf):
        if not self.enabled(build): return
        build.env.ParseConfig( 'pkg-config libusb-1.0 --silence-errors --cflags --libs')
        if (not conf.CheckLib(['libusb-1.0', 'usb-1.0']) or not conf.CheckHeader('libusb-1.0/libusb.h')):
            raise Exception( 'Did not find the libusb 1.0 development library or its header file, exiting!')
        build.env.Append(CPPDEFINES='__BULK__')
    def sources(self, build):
        sources = ['controllers/bulk/bulkcontroller.cpp',
                   'controllers/bulk/bulkenumerator.cpp']
        if not int(build.flags['hid']):
            sources.append('controllers/hid/hidcontrollerpresetfilehandler.cpp')
        return sources
class VinylControl(Feature):
    def description(self): return "Vinyl Control"
    def enabled(self, build):
        build.flags['vinylcontrol'] = util.get_flags(build.env,'vinylcontrol', 0)
        # Existence of the macappstore option forces vinylcontrol off due to
        # licensing issues.
        if build.flags.has_key('macappstore') and int(build.flags['macappstore']): return False
        if int(build.flags['vinylcontrol']): return True
        return False
    def add_options(self, build, vars): vars.Add('vinylcontrol', 'Set to 1 to enable vinyl control support', 1)
    def configure(self, build, conf):
        if not self.enabled(build): return
        build.env.Append(CPPDEFINES='__VINYLCONTROL__')
        build.env.Append(CPPPATH='#lib/xwax')
        build.env.Append(CPPPATH='#lib/scratchlib')
    def sources(self, build):
        sources = ['vinylcontrol/vinylcontrol.cpp',
                   'vinylcontrol/vinylcontrolxwax.cpp',
                   'preferences/dlgprefvinyl.cpp',
                   'vinylcontrol/vinylcontrolsignalwidget.cpp',
                   'vinylcontrol/vinylcontrolmanager.cpp',
                   'vinylcontrol/vinylcontrolprocessor.cpp',
                   'vinylcontrol/steadypitch.cpp',
                   'engine/vinylcontrolcontrol.cpp', ]
        if build.platform_is_windows:
            sources.append("#lib/xwax/timecoder_win32.cpp")
            sources.append("#lib/xwax/lut_win32.cpp")
        else:
            sources.append("#lib/xwax/timecoder.c")
            sources.append("#lib/xwax/lut.c")
        return sources
class Vamp(Feature):
    INTERNAL_LINK = False
    INTERNAL_VAMP_PATH = '#lib/vamp-2.3'
    def description(self): return "Vamp Analysers support"
    def enabled(self, build):
        build.flags['vamp'] = util.get_flags(build.env, 'vamp', 1)
        if int(build.flags['vamp']): return True
        return False
    def add_options(self, build, vars): vars.Add('vamp', 'Set to 1 to enable vamp analysers', 1)
    def configure(self, build, conf):
        if not self.enabled(build): return
        # If there is no system vamp-hostdk installed, then we'll directly link
        # the vamp-hostsdk.
        if not conf.CheckLib(['vamp-hostsdk']):
            # For header includes
            build.env.Append(CPPPATH=[self.INTERNAL_VAMP_PATH])
            self.INTERNAL_LINK = True
        # Needed on Linux at least. Maybe needed elsewhere?
        if build.platform_is_linux:
            # Optionally link libdl and libX11. Required for some distros.
            conf.CheckLib(['dl', 'libdl'])
            conf.CheckLib(['X11', 'libX11'])
        # FFTW3 support
        have_fftw3_h = conf.CheckHeader('fftw3.h')
        have_fftw3 = conf.CheckLib('fftw3', autoadd=False)
        if have_fftw3_h and have_fftw3 and build.platform_is_linux:
            build.env.Append(CPPDEFINES='HAVE_FFTW3')
            build.env.ParseConfig('pkg-config fftw3 --silence-errors --cflags --libs')
    def sources(self, build):
        sources = ['vamp/vampanalyser.cpp',
                   'vamp/vamppluginloader.cpp',
                   'anqueue/analyserbeats.cpp',
                   'preferences/dlgprefbeats.cpp']
        if self.INTERNAL_LINK:
            hostsdk_src_path = '%s/src/vamp-hostsdk' % self.INTERNAL_VAMP_PATH
            sources.extend(path % hostsdk_src_path for path in
                           ['%s/PluginBufferingAdapter.cpp',
                            '%s/PluginChannelAdapter.cpp',
                            '%s/PluginHostAdapter.cpp',
                            '%s/PluginInputDomainAdapter.cpp',
                            '%s/PluginLoader.cpp',
                            '%s/PluginSummarisingAdapter.cpp',
                            '%s/PluginWrapper.cpp',
                            '%s/RealTime.cpp'])
        return sources
class ColorDiagnostics(Feature):
    def description(self):
        return "Color Diagnostics"
    def enabled(self, build):
        build.flags['color'] = util.get_flags(build.env, 'color', 0)
        return bool(int(build.flags['color']))
    def add_options(self, build, vars):
        vars.Add('color', "Set to 1 to enable Clang color diagnostics.", 0)
    def configure(self, build, conf):
        if not self.enabled(build): return
        if not build.compiler_is_clang: raise Exception('Color diagnostics are only available using clang.')
        build.env.Append(CCFLAGS='-fcolor-diagnostics')
class PerfTools(Feature):
    def description(self): return "Google PerfTools"
    def enabled(self, build):
        build.flags['perftools'] = util.get_flags(build.env, 'perftools', 0)
        build.flags['perftools_profiler'] = util.get_flags(build.env, 'perftools_profiler', 0)
        if int(build.flags['perftools']): return True
        return False
    def add_options(self, build, vars):
        vars.Add( "perftools", "Set to 1 to enable linking against libtcmalloc and Google's performance tools. You must install libtcmalloc from google-perftools to use this option.", 0)
        vars.Add("perftools_profiler", "Set to 1 to enable linking against libprofiler, Google's CPU profiler. You must install libprofiler from google-perftools to use this option.", 0)
    def configure(self, build, conf):
        if not self.enabled(build): return
        build.env.Append(LIBS="tcmalloc")
        if int(build.flags['perftools_profiler']):build.env.Append(LIBS="profiler")
class QDebug(Feature):
    def description(self): return "Debugging message output"
    def enabled(self, build):
        build.flags['qdebug'] = util.get_flags(build.env, 'qdebug', 0)
        if build.platform_is_windows:
            if build.build_is_debug:
                # Turn general debugging flag on too if debug build is specified
                build.flags['qdebug'] = 1
        if int(build.flags['qdebug']): return True
        return False
    def add_options(self, build, vars):
        vars.Add('qdebug', 'Set to 1 to enable verbose console debug output.', 1)
    def configure(self, build, conf):
        if not self.enabled(build): build.env.Append(CPPDEFINES='QT_NO_DEBUG_OUTPUT')
class Verbose(Feature):
    def description(self): return "Verbose compilation output"
    def enabled(self, build):
        build.flags['verbose'] = util.get_flags(build.env, 'verbose', 1)
        if int(build.flags['verbose']): return True
        return False
    def add_options(self, build, vars):
        vars.Add('verbose', 'Compile files verbosely.', 1)
    def configure(self, build, conf):
        if not self.enabled(build):
            build.env['CCCOMSTR'] = '[CC] $SOURCE'
            build.env['CXXCOMSTR'] = '[CXX] $SOURCE'
            build.env['ASCOMSTR'] = '[AS] $SOURCE'
            build.env['ARCOMSTR'] = '[AR] $TARGET'
            build.env['RANLIBCOMSTR'] = '[RANLIB] $TARGET'
            build.env['LDMODULECOMSTR'] = '[LD] $TARGET'
            build.env['LINKCOMSTR'] = '[LD] $TARGET'

            build.env['QT5_LUPDATECOMSTR'] = '[LUPDATE] $SOURCE'
            build.env['QT5_LRELEASECOMSTR'] = '[LRELEASE] $SOURCE'
            build.env['QT5_QRCCOMSTR'] = '[QRC] $SOURCE'
            build.env['QT5_UICCOMSTR'] = '[UIC5] $SOURCE'
            build.env['QT5_MOCCOMSTR'] = '[MOC] $SOURCE'


class Profiling(Feature):
    def description(self): return "gprof/Saturn profiling support"
    def enabled(self, build):
        build.flags['profiling'] = util.get_flags(build.env, 'profiling', 0)
        if int(build.flags['profiling']):
            if build.platform_is_linux or build.platform_is_osx or build.platform_is_bsd: return True
        return False
    def add_options(self, build, vars):
        if not build.platform_is_windows:
            vars.Add('profiling',
                     '(DEVELOPER) Set to 1 to enable profiling using gprof (Linux) or Saturn (OS X)', 0)
    def configure(self, build, conf):
        if not self.enabled(build): return
        if build.platform_is_linux or build.platform_is_bsd:
            build.env.Append(CCFLAGS='-pg')
            build.env.Append(LINKFLAGS='-pg')
        elif build.platform_is_osx:
            build.env.Append(CCFLAGS='-finstrument-functions')
            build.env.Append(LINKFLAGS='-lSaturn')
class TestSuite(Feature):
    def description(self): return "Mixxx Test Suite"
    def enabled(self, build):
        build.flags['test'] = util.get_flags(build.env, 'test', 0) or 'test' in SCons.BUILD_TARGETS
        if int(build.flags['test']): return True
        return False
    def add_options(self, build, vars):
        vars.Add('test', 'Set to 1 to build Mixxx test fixtures.', 0)
    def configure(self, build, conf):
        if not self.enabled(build): return
    def sources(self, build):
        # Build the gtest library, but don't return any sources.

        # Clone our main environment so we don't change any settings in the
        # Mixxx environment
        test_env = build.env.Clone()
        # -pthread tells GCC to do the right thing regardless of system
        if build.toolchain_is_gnu:
            test_env.Append(CCFLAGS='-pthread')
            test_env.Append(LINKFLAGS='-pthread')
        test_env.Append(CPPPATH="#lib/gtest-1.7.0/include")
        gtest_dir = test_env.Dir("#lib/gtest-1.7.0")
        # gtest_dir.addRepository(build.env.Dir('#lib/gtest-1.5.0'))
        # build.env['EXE_OUTPUT'] = '#/lib/gtest-1.3.0/bin'  # example,
        # optional
        test_env['LIB_OUTPUT'] = '#/lib/gtest-1.7.0/lib'
        env = test_env
        SCons.Export('env')
        env.SConscript(env.File('SConscript', gtest_dir))
        # build and configure gmock
        test_env.Append(CPPPATH="#lib/gmock-1.7.0/include")
        gmock_dir = test_env.Dir("#lib/gmock-1.7.0")
        # gmock_dir.addRepository(build.env.Dir('#lib/gmock-1.5.0'))
        test_env['LIB_OUTPUT'] = '#/lib/gmock-1.7.0/lib'
        env.SConscript(env.File('SConscript', gmock_dir))
        return []
class Shoutcast(Feature):
    def description(self): return "Shoutcast Broadcasting (OGG/MP3)"
    def enabled(self, build):
        build.flags['shoutcast'] = util.get_flags(build.env, 'shoutcast', 1)
        if int(build.flags['shoutcast']): return True
        return False
    def add_options(self, build, vars):
        vars.Add('shoutcast', 'Set to 1 to enable shoutcast support', 1)
    def configure(self, build, conf):
        if not self.enabled(build): return
        libshout_found = conf.CheckLib(['libshout', 'shout'])
        build.env.Append(CPPDEFINES='__SHOUTCAST__')
        if not libshout_found:
            raise Exception('Could not find libshout or its development headers. Please install it or compile Mixxx without Shoutcast support using the shoutcast=0 flag.')
        if build.platform_is_windows and build.static_dependencies:
            conf.CheckLib('winmm')
            conf.CheckLib('ws2_32')
    def sources(self, build):
        depends.Qt.uic(build)('preferences/dlgprefshoutcastdlg.ui')
        return ['preferences/dlgprefshoutcast.cpp',
                'shoutcast/shoutcastmanager.cpp',
                'engine/sidechain/engineshoutcast.cpp']
class Optimize(Feature):
    LEVEL_OFF = 'off'
    LEVEL_PORTABLE = 'portable'
    LEVEL_NATIVE = 'native'
    LEVEL_LEGACY = 'legacy'
    LEVEL_DEFAULT = LEVEL_PORTABLE
    def description(self): return "Optimization and Tuning"
    @staticmethod
    def get_optimization_level():
        optimize_level = SCons.ARGUMENTS.get('optimize', Optimize.LEVEL_DEFAULT)
        try:
            optimize_integer = int(optimize_level)
            if optimize_integer == 0: optimize_level = Optimize.LEVEL_OFF
            elif optimize_integer == 1: optimize_level = Optimize.LEVEL_LEGACY
            elif optimize_integer in xrange(2, 10): optimize_level = Optimize.LEVEL_PORTABLE
        except: pass
        # Support common aliases for off.
        if optimize_level in ('none', 'disable', 'disabled'): optimize_level = Optimize.LEVEL_OFF
        if optimize_level not in (Optimize.LEVEL_OFF, Optimize.LEVEL_PORTABLE, Optimize.LEVEL_NATIVE, Optimize.LEVEL_LEGACY):
            raise Exception("optimize={} is not supported. Use portable, native, legacy or off" .format(optimize_level))
        return optimize_level
    def enabled(self, build):
        build.flags['optimize'] = Optimize.get_optimization_level()
        return build.flags['optimize'] != Optimize.LEVEL_OFF
    def add_options(self, build, vars):
        vars.Add( 'optimize', 'Set to:\n' \
                              '  portable: sse2 CPU (>= Pentium 4)\n' \
                              '  native: optimized for the CPU of this system\n' \
                              '  legacy: pure i386 code' \
                              '  off: no optimization' \
                        , Optimize.LEVEL_DEFAULT)

    def configure(self, build, conf):
        if not self.enabled(build): return
        optimize_level = build.flags['optimize']
        if optimize_level == Optimize.LEVEL_OFF:
            self.status = "off: no optimization"
            return
        if build.toolchain_is_msvs:
            # /GL : http://msdn.microsoft.com/en-us/library/0zza0de8.aspx
            # !!! /GL is incompatible with /ZI, which is set by mscvdebug
            build.env.Append(CCFLAGS='/GL')
            # Use the fastest floating point math library
            # http://msdn.microsoft.com/en-us/library/e7s85ffb.aspx
            # http://msdn.microsoft.com/en-us/library/ms235601.aspx
            build.env.Append(CCFLAGS='/fp:fast')
            # Do link-time code generation (and don't show a progress indicator
            # -- this relies on ANSI control characters and tends to overwhelm
            # Jenkins logs) Should we turn on PGO ?
            # http://msdn.microsoft.com/en-us/library/xbf3tbeh.aspx
            build.env.Append(LINKFLAGS='/LTCG:NOSTATUS')
            # Suggested for unused code removal
            # http://msdn.microsoft.com/en-us/library/ms235601.aspx
            # http://msdn.microsoft.com/en-us/library/xsa71f43.aspx
            # http://msdn.microsoft.com/en-us/library/bxwfs976.aspx
            build.env.Append(CCFLAGS='/Gy')
            build.env.Append(LINKFLAGS='/OPT:REF')
            build.env.Append(LINKFLAGS='/OPT:ICF')
            # Don't worry about aligning code on 4KB boundaries
            # build.env.Append(LINKFLAGS = '/OPT:NOWIN98')
            # ALBERT: NOWIN98 is not supported in MSVC 2010.

            # http://msdn.microsoft.com/en-us/library/59a3b321.aspx
            # In general, you should pick /O2 over /Ox
            build.env.Append(CCFLAGS='/O2')
            if optimize_level == Optimize.LEVEL_PORTABLE:
                # portable-binary: sse2 CPU (>= Pentium 4)
                self.status = "portable: sse2 CPU (>= Pentium 4)"
                build.env.Append(CCFLAGS='/arch:SSE2')
                build.env.Append(CPPDEFINES=['__SSE__', '__SSE2__'])
            elif optimize_level == Optimize.LEVEL_NATIVE:
                self.status = "native: tuned for this CPU (%s)" % build.machine
                build.env.Append(CCFLAGS='/favor:' + build.machine)
            elif optimize_level == Optimize.LEVEL_LEGACY: self.status = "legacy: pure i386 code"
            else:
                # Not possible to reach this code if enabled is written
                # correctly.
                raise Exception("optimize={} is not supported. Use portable, native, legacy or off".format(optimize_level))
            # SSE and SSE2 are core instructions on x64
            if build.machine_is_64bit:
                build.env.Append(CPPDEFINES=['__SSE__', '__SSE2__'])
        elif build.toolchain_is_gnu:
            # Common flags to all optimizations.
            # -ffast-math will pevent a performance penalty by denormals
            # (floating point values almost Zero are treated as Zero)
            # unfortunately that work only on 64 bit CPUs or with sse2 enabled

            # the following optimisation flags makes the engine code ~3 times
            # faster, measured on a Atom CPU.
            build.env.Append(CCFLAGS='-O3 -ffast-math -funroll-loops -Ofast')

            # set -fomit-frame-pointer when we don't profile.
            # Note: It is only included in -O on machines where it does not
            # interfere with debugging
            if not int(build.flags['profiling']): build.env.Append(CCFLAGS='-fomit-frame-pointer')
            if optimize_level == Optimize.LEVEL_PORTABLE:
                # portable: sse2 CPU (>= Pentium 4)
                if build.architecture_is_x86:
                    self.status = "portable: sse2 CPU (>= Pentium 4)"
                    build.env.Append(CCFLAGS='-mtune=generic')
                    # -mtune=generic pick the most common, but compatible options.
                    # on arm platforms equivalent to -march=arch
                    if not build.machine_is_64bit:
                        # the sse flags are not set by default on 32 bit builds
                        # but are not supported on arm builds
                        build.env.Append(CCFLAGS='-msse2 -mfpmath=sse')
                else: self.status = "portable"
                # this sets macros __SSE2_MATH__ __SSE_MATH__ __SSE2__ __SSE__
                # This should be our default build for distribution
                # It's a little sketchy, but turning on SSE2 will gain
                # 100% performance in our filter code and allows us to
                # turns on denormal zeroing.
                # We don't really support CPU's earlier than Pentium 4,
                # which is the class of CPUs this decision affects.
                # The downside of this is that we aren't truly
                # i386 compatible, so builds that claim 'i386' will crash.
                # -- rryan 2/2011
                # Note: SSE2 is a core part of x64 CPUs
            elif optimize_level == Optimize.LEVEL_NATIVE:
                self.status = "native: tuned for this CPU (%s)" % build.machine
                build.env.Append(CCFLAGS='-march=native')
                # http://en.chys.info/2010/04/what-exactly-marchnative-means/
                # Note: requires gcc >= 4.2.0
                # macros like __SSE2_MATH__ __SSE_MATH__ __SSE2__ __SSE__
                # are set automaticaly
                if build.architecture_is_x86 and not build.machine_is_64bit:
                    # the sse flags are not set by default on 32 bit builds
                    # but are not supported on arm builds
                    build.env.Append(CCFLAGS='-msse2 -mfpmath=sse')
            elif optimize_level == Optimize.LEVEL_LEGACY:
                if build.architecture_is_x86:
                    self.status = "legacy: pure i386 code"
                    build.env.Append(CCFLAGS='-mtune=generic')
                    # -mtune=generic pick the most common, but compatible options.
                    # on arm platforms equivalent to -march=arch
                else: self.status = "legacy"
            else:
                # Not possible to reach this code if enabled is written
                # correctly.
                raise Exception("optimize={} is not supported. Use portable, native, legacy or off".format(optimize_level))
            # what others do:
            # soundtouch uses just -O3 in Ubuntu Trusty
            # rubberband uses just -O2 in Ubuntu Trusty
            # fftw3 (used by rubberband) in Ubuntu Trusty
            # -O3 -fomit-frame-pointer -mtune=native -malign-double
            # -fstrict-aliasing -fno-schedule-insns -ffast-math
class AutoDjCrates(Feature):
    def description(self): return "Auto-DJ crates (for random tracks)"
    def enabled(self, build):
        build.flags['autodjcrates'] = util.get_flags(build.env, 'autodjcrates', 1)
        if int(build.flags['autodjcrates']): return True
        return False
    def add_options(self, build, vars):
        vars.Add('autodjcrates','Set to 1 to enable crates as a source for random Auto-DJ tracks.', 1)
    def configure(self, build, conf):
        if not self.enabled(build): return
        build.env.Append(CPPDEFINES='__AUTODJCRATES__')
    def sources(self, build):return ['library/dao/autodjcratesdao.cpp']
