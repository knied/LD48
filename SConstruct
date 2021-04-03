import os

################################################################################
# Setup
################################################################################

AddOption('--for_debug', dest='debug', action='store_true', default=False, help="build with debug symbols")
#AddOption('--with-assertions', dest='assertions', action='store_true', default=False, help="build with debug symbols")
#AddOption('--non-optimized', dest='optimize', action='store_false', default=True, help="create non optimized build")
AddOption('--prefix', dest='prefix', action='store', default='#/install', help="install location")
AddOption('--run', dest='run', action='store_true', default=False, help="run programs")

env = Environment(tools = ['default', 'clang', 'clangxx'])
if not 'HOMEBREW_PREFIX' in os.environ:
    print("Error: HOMEBREW_PREFIX needs to point to your Homebrew prefix. Typically '/usr/local' or '/opt/homebrew'")
    Exit(1)
homebrew_prefix = os.environ['HOMEBREW_PREFIX']
llvm = os.path.join(homebrew_prefix, 'opt/llvm')
libev = os.path.join(homebrew_prefix, 'opt/libev')
libressl = os.path.join(homebrew_prefix, 'opt/libressl')

env.PrependENVPath('PATH', os.path.join(llvm, 'bin'))
if not 'WASI_SYSROOT' in os.environ:
    print("Error: WASI_SYSROOT needs to point to a precompiled wasi sysroot")
    Exit(1)
env['WASI_SYSROOT'] = os.environ['WASI_SYSROOT']

env.Append(CXXFLAGS = ['-std=c++2a', '-fcoroutines-ts',
                       '-Wall', '-Wextra', '-Wc++2a-compat', '-Werror'],
           LIBPATH = [os.path.join(libev, 'lib'), os.path.join(libressl, 'lib')],
           CPPPATH = [os.path.join(libev, 'include'), os.path.join(libressl, 'include')])

if GetOption('debug'):
    env.Append(CXXFLAGS = ['-g'])
else:
    env.Append(CXXFLAGS = ['-O3'])
    env.Append(CXXFLAGS = ['-DNDEBUG'])

env['PREFIX'] = GetOption('prefix')
env.Alias('install', env['PREFIX'])

################################################################################
# Methods
################################################################################

def RunProgram(self, dependencies, program, args):
    if GetOption('run'):
        run = self.Command(target = 'output.log',
                           source = [program[0].abspath],
                           action = '$SOURCE %s | tee $TARGET' % (' '.join(args)))
        Depends(run, dependencies)
        self.AlwaysBuild(run)
AddMethod(Environment, RunProgram)

def Wasm(self, target, sources):
    wasm_env = self.Clone()
    wasm_env.Append(CXXFLAGS = ['--target=wasm32-wasi',
                                '--sysroot=%s' % (env['WASI_SYSROOT']),
                                '-fvisibility=hidden',
                                '-fno-exceptions'],
                    LINKFLAGS = ['--target=wasm32-wasi',
                                 '--sysroot=%s' % (env['WASI_SYSROOT']),
                                 '-nostartfiles',
                                 '-Wl,--export-dynamic',
                                 '-Wl,--import-memory',
                                 '-Wl,--no-entry'])
    return wasm_env.Program(target, sources)
AddMethod(Environment, Wasm)

################################################################################
# SConscripts
################################################################################

Export('env')

SConscript('dep/SConscript', variant_dir='build_dep', duplicate=0)
SConscript('src/SConscript', variant_dir='build', duplicate=0)
