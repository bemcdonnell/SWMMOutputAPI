
from distutils.core import setup



VERSION = '0.2.1'  
AUTHOR_NAME = 'Bryant E. McDonnell (EmNet LLC)'
AUTHOR_EMAIL = 'bemcdonnell@gmail.com'



setup(name='SWMMOutputAPI',
      version=VERSION,
      description='Python Wrapper for SWMM5 Binary Output API',
      author=AUTHOR_NAME,
      url='https://github.com/bemcdonnell/SWMMOutputAPI.git',
      author_email=AUTHOR_EMAIL,

      package_dir = {'':'SWMMOutputAPI'},
      packages=[''],
      package_data = {'':
                      ['src/*.c',\
                       'src/*.h',\
                       'data/outputAPI_winx86.dll']},
      license="BSD2 License",
      classifiers=[
          "License :: OSI Approved :: BSD2 License",
          "Programming Language :: Python :: 2.7",
      ]
)
