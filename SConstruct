################################################################################
# Setup
################################################################################

AddOption('--with-symbols', dest='debug', action='store_true', default=False, help="build with debug symbols")
AddOption('--with-assertions', dest='assertions', action='store_true', default=False, help="build with debug symbols")
AddOption('--non-optimized', dest='optimize', action='store_false', default=True, help="create non optimized build")
AddOption('--prefix', dest='prefix', action='store', default='#/install', help="install location")

env = Environment(tools = ['default', 'clang', 'clangxx'])

env.Append(CXXFLAGS = ['-std=c++17', '-fcoroutines-ts', '-stdlib=libc++'])

if GetOption('debug'):
    env.Append(CXXFLAGS = ['-g'])
if GetOption('optimize'):
    env.Append(CXXFLAGS = ['-O3'])
if not GetOption('assertions'):
    env.Append(CXXFLAGS = ['-DNDEBUG'])

env['PREFIX'] = GetOption('prefix')
env.Alias('install', env['PREFIX'])

################################################################################
# SConscripts
################################################################################

Export('env')

SConscript('dep/SConscript', variant_dir='build_dep', duplicate=0)
SConscript('src/SConscript', variant_dir='build', duplicate=0)
