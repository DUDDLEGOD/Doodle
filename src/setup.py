from setuptools import setup, Extension

doodle_module = Extension(
    'doodle',
    sources=['expose_raylib.c', 'mparser.c', 'dutils.c'],
    include_dirs=['.'],          # Local header search path
    libraries=['raylib'],        # Links against native raylib binary
    extra_compile_args=['-std=c99']
)

setup(
    name='doodle',
    version='1.0',
    description='Hardware-accelerated DOM-based UI & 2D Game Engine',
    ext_modules=[doodle_module],
    install_requires=['raylib']   # Pulls raylib-python-cffi for the python layer
)
