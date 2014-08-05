from distutils.core import setup, Extension

PYMMSPA4PG_MODULE = Extension('pymmspa4pg',
                           include_dirs=['/usr/local/include'],
                           libraries=['mmspa4pg'],
                           library_dirs=['/usr/local/lib'],
                           sources=['pymmspa4pg_module.c'])

setup(name='pymmspa4pg',
      version='0.1',
      description='Python binding of multimodal shortest path algorithms',
      author='Lu LIU',
      author_email='nudtlliu@gmail.com',
      ext_modules=[PYMMSPA4PG_MODULE])
