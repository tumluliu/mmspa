from distutils.core import setup, Extension

MMSPA_MODULE = Extension('pymmspa',
                         include_dirs=['/usr/local/include'],
                         libraries=['mmspa'],
                         library_dirs=['/usr/local/lib'],
                         sources=['pymmspa.c'])

setup(name='pymmspa',
      version=1.0,
      description='Python binding of multimodal shortest path algorithms',
      author='Lu LIU',
      author_email='nudtlliu@gmail.com',
      ext_modules=[MMSPA_MODULE])
