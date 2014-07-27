from distutils.core import setup, Extension

PYMMSPA_MODULE = Extension('pymmspa',
                           include_dirs=['/usr/local/include'],
                           libraries=['mmspa'],
                           library_dirs=['/usr/local/lib'],
                           sources=['pymmspa_module.c'])

setup(name='pymmspa',
      version='0.1',
      description='Python binding of multimodal shortest path algorithms',
      author='Lu LIU',
      author_email='nudtlliu@gmail.com',
      ext_modules=[PYMMSPA_MODULE])
