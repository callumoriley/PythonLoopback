from setuptools import setup, Extension
import numpy as np

pl_module = Extension("PythonLoopback", sources=["PythonLoopback.cpp"], include_dirs=[np.get_include()])

setup(
    name="PythonLoopback",
    version="0.3",
    description="Allows Python to get information about audio currently playing on the system",
    python_requires=">=3.10.0",
    platforms=["Windows"],
    ext_modules=[pl_module]
)