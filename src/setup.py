from setuptools import setup, Extension

doodle_module = Extension(
    '_doodle',
    sources=['expose_raylib.c', 'mparser.c', 'dutils.c', 'daudio.c'],
    include_dirs=['.'],          # Local header search path
    library_dirs=['.'],          # Local library search path
    libraries=['raylib', 'gdi32', 'winmm'],        # Links against native raylib binary and windows libs
    extra_compile_args=['-std=c99']
)

setup(
    name='doodle',
    version='1.0',
    description='Hardware-accelerated DOM-based UI & 2D Game Engine',
    ext_modules=[doodle_module],
    install_requires=['raylib']   # Pulls raylib-python-cffi for the python layer
)
