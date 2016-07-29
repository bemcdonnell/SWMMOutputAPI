
#Python setup.py sdist

from distutils.core import setup

setup(name = 'SWMMOutputAPI',
      version = '1.0',
      description = 'Python Wrapper for SWMM Output API',
      author = 'Bryant E. McDonnell',
      author_email='bemcdonnell@gmail.com',
      url = 'https://github.com/bemcdonnell/SWMMOutputAPI.git',
      scripts = ['_toolkitpyswmm.py',
                 'SWMMOutputReader.py',
                 'data/outputAPI_winx86.dll'])
