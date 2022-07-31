from setuptools import setup, Extension
import numpy as np

pl_module = Extension("PythonLoopback", sources=["PythonLoopback.cpp"], include_dirs=[np.get_include()])

setup(
    name="PythonLoopback",
    packages=["PythonLoopback"],
    version="0.3",
    license="GPLv3",
    description="Allows Python to get information about audio currently playing on the system",
    author="Callum O'Riley",
    author_email="callumchristopheroriley@gmail.com",
    url="https://github.com/callumoriley/PythonLoopback",
    keywords=["windows", "audio", "loopback"]
    python_requires=">=3.10.0",
    platforms=["Windows"],
    ext_modules=[pl_module],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Build Tools',
        'License :: OSI Approved :: GPLv3',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.10',
  ],
)
