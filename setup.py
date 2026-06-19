import sys
from setuptools import setup, Extension, find_packages

include_dirs = ['src']
library_dirs = []
libraries = []
extra_compile_args = ['-std=c99']
extra_link_args = []

if sys.platform.startswith('linux'):
    include_dirs.append('/usr/local/include')
    library_dirs.append('/usr/local/lib')
    extra_link_args.extend([
        '-Wl,-Bstatic', '-lraylib', '-Wl,-Bdynamic',
        '-lGL', '-lm', '-lpthread', '-ldl', '-lrt', '-lX11'
    ])
elif sys.platform == 'darwin':
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
    include_dirs.append(r'C:\raylib\include')
    library_dirs.append(r'C:\raylib\lib')
    libraries.extend(['raylib', 'gdi32', 'winmm', 'user32', 'shell32'])

doodle_module = Extension(
    '_doodle',
    sources=['src/expose_raylib.c', 'src/mparser.c', 'src/dutils.c', 'src/daudio.c'],
    include_dirs=include_dirs,
    library_dirs=library_dirs,
    libraries=libraries,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args
)

setup(
    packages=find_packages(),
    ext_modules=[doodle_module],
)
