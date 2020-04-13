################################################################################
# Setup
################################################################################

AddOption('--with-symbols', dest='debug', action='store_true', default=False, help="build with debug symbols")
AddOption('--with-assertions', dest='assertions', action='store_true', default=False, help="build with debug symbols")
AddOption('--non-optimized', dest='optimize', action='store_false', default=True, help="create non optimized build")
AddOption('--prefix', dest='prefix', action='store', default='#/install', help="install location")
AddOption('--run', dest='run', action='store_true', default=False, help="run programs")

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
# Methods
################################################################################

def RunProgram(self, program, args):
    if GetOption('run'):
        run = self.Command(target = 'output.log',
                           source = [program[0].abspath],
                           action = '$SOURCE %s | tee $TARGET' % (' '.join(args)))
        self.AlwaysBuild(run)
AddMethod(Environment, RunProgram)

################################################################################
# SConscripts
################################################################################

Export('env')

SConscript('dep/SConscript', variant_dir='build_dep', duplicate=0)
SConscript('src/SConscript', variant_dir='build', duplicate=0)
