
from setuptools import setup, find_packages



VERSION = '0.2.1'  
AUTHOR_NAME = 'Bryant E. McDonnell (EmNet LLC)'
AUTHOR_EMAIL = 'bemcdonnell@gmail.com'



setup(name='SWMMOutputAPI',
      version=VERSION,
      description='Python Wrapper for SWMM5 Binary Output File',
      author=AUTHOR_NAME,
      url='https://github.com/bemcdonnell/SWMMOutputAPI.git',
      author_email=AUTHOR_EMAIL,

      package_dir = {'':'swmmoutputapi'},
      packages=[''],
      package_data = {'':
                      ['src/*.c',\
                       'src/*.h',\
                       'data/outputAPI_winx86.dll',\
                       'license.txt']},
      include_package_data=True,
      license="BSD2 License",
      keywords = "swmm5",
      classifiers=[
          "License :: OSI Approved :: BSD2 License",
          "Programming Language :: Python :: 2.7",
          "Development Status :: 4 - Beta",
      ]
)
