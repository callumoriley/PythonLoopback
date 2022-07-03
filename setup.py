from setuptools import setup, Extension

pl_module = Extension("PythonLoopback", sources=["PythonLoopback.cpp"])

setup(
    name="PythonLoopback",
    version="0.1",
    description="Allows Python to get information about audio currently playing on the system",
    ext_modules=[pl_module]
)