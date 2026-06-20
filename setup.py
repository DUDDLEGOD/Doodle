import sys
from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext

class my_build_ext(build_ext):
    def finalize_options(self):
        super().finalize_options()
        if sys.platform == 'win32':
            self.compiler = 'mingw32'

include_dirs = ['src']
library_dirs = []
libraries = []
extra_compile_args = ['-std=c99']
extra_link_args = []

if sys.platform == 'win32':
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
    cmdclass={
        'build_ext': my_build_ext,
    },
)

