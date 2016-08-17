
#Python setup.py sdist

from distutils.core import setup

setup(name = 'SWMMOutputAPI',
      version = '2.0',
      description = 'Python Wrapper for SWMM Output API',
      author = 'Bryant E. McDonnell',
      author_email='bemcdonnell@gmail.com',
      url = 'https://github.com/bemcdonnell/SWMMOutputAPI.git',
      package_dir = {'SWMMOutputAPI':'SWMMOutputAPI'},
      package_data = {'SWMMOutputAPI':\
                 ['../*.py',
                 'data/outputAPI_winx86.dll']}
      )
