import os
import sys
import shutil
from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext

FORCE_MINGW = os.environ.get('DOODLE_FORCE_MINGW') == '1'
IS_CI = bool(
    os.environ.get('CIBUILDWHEEL') or 
    os.environ.get('GITHUB_ACTIONS') or 
    os.environ.get('CI') or 
    'cibw-run' in sys.executable or
    'runneradmin' in sys.executable
)

if sys.platform == 'win32' and not FORCE_MINGW and not IS_CI:
    if not shutil.which('cl.exe'):
        mingw_bin = r'C:\msys64\ucrt64\bin'
        if os.path.exists(mingw_bin):
            os.environ['PATH'] = mingw_bin + os.pathsep + os.environ['PATH']
            FORCE_MINGW = True
            print("MSVC (cl.exe) not found. Automatically falling back to MSYS2 MinGW compiler.")

class my_build_ext(build_ext):
    def finalize_options(self):
        super().finalize_options()
        if sys.platform == 'win32' and FORCE_MINGW:
            self.compiler = 'mingw32'


include_dirs = ['src']
library_dirs = []
libraries = []
extra_compile_args = ['-std=c99', '-O3', '-ffast-math', '-msse3']
extra_link_args = []

if sys.platform.startswith('linux'):
    include_dirs.append('/usr/local/include')
    library_dirs.append('/usr/local/lib')
    extra_link_args.extend([
        '-Wl,-Bstatic', '-lraylib', '-Wl,-Bdynamic',
        '-lGL', '-lm', '-lpthread', '-ldl', '-lrt', '-lX11'
    ])
elif sys.platform == 'darwin':
    if os.path.exists('raylib'):
        include_dirs.append('raylib/src')
        library_dirs.append('raylib/build/raylib')
        extra_link_args.extend([
            'raylib/build/raylib/libraylib.a',
            '-framework', 'OpenGL',
            '-framework', 'Cocoa',
            '-framework', 'IOKit',
            '-framework', 'CoreAudio',
            '-framework', 'CoreVideo'
        ])
    else:
        include_dirs.append('/usr/local/include')
        library_dirs.append('/usr/local/lib')
        extra_link_args.extend([
            '/usr/local/lib/libraylib.a',
            '-framework', 'OpenGL',
            '-framework', 'Cocoa',
            '-framework', 'IOKit',
            '-framework', 'CoreAudio',
            '-framework', 'CoreVideo'
        ])
elif sys.platform == 'win32':
    local_mingw_raylib = r'third_party\raylib_dist\raylib-5.0_win64_mingw-w64'
    if os.path.exists(local_mingw_raylib):
        include_dirs.append(os.path.join(local_mingw_raylib, 'include'))
        library_dirs.append(os.path.join(local_mingw_raylib, 'lib'))
    elif os.path.exists('raylib'):
        include_dirs.append('raylib/src')
        library_dirs.append('raylib/build/raylib/Release')
    else:
        include_dirs.append(r'C:\raylib\include')
        library_dirs.append(r'C:\raylib\lib')
    libraries.extend(['raylib', 'gdi32', 'winmm', 'user32', 'shell32', 'psapi'])

    # MSVC doesn't support -std=c99 compile flag
    if not FORCE_MINGW:
        extra_compile_args = []


doodle_module = Extension(
    'doodle._doodle',
    sources=[
        'src/expose_raylib.c', 'src/mparser.c', 'src/dutils.c', 'src/daudio.c',
        'src/color.c', 'src/unit.c', 'src/string_utils.c', 'src/dom_utils.c',
        'src/html_parser.c', 'src/css_parser.c', 'src/layout.c', 'src/renderer.c',
        'src/particles.c', 'src/cache.c', 'src/profiler.c'
    ],
    include_dirs=include_dirs,
    library_dirs=library_dirs,
    libraries=libraries,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args
)

setup(
    packages=find_packages(),
    ext_modules=[doodle_module],
    cmdclass={
        'build_ext': my_build_ext,
    },
)
