from setuptools import setup
import CAmpPython


setup(name='CAmpPython',
      version='1.0',
      author='David and Evana',
      author_email='',
      description="C code generator for Asychronous management protocols",
      long_description="The camp command takes in a JSON file which models a protocol and returns c and h files. These files are created by using the formatted information of the different data types that are described in the JSON. Can create the implemented files and then general files.",
      url='',
      license='',
      classifiers=[],
      keywords='AMP converter',
      packages=['CAmpPython'],
      package_data={'':['data/name_registry.txt']},
      entry_points = {
            'console_scripts':[
                  'camp=CAmpPython.CAmpPython:main',
            ],
      },
)

