from setuptools import setup, Extension

pl_module = Extension("PythonLoopback", sources=["PythonLoopback.cpp"])

setup(
    name="PythonLoopback",
    version="0.2",
    description="Allows Python to get information about audio currently playing on the system",
    python_requires="==3.7.9",
    ext_modules=[pl_module]
)