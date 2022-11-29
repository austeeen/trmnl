from setuptools import setup

setup(
    name="Major Mines",
    packages=["src"],
    entry_points={"console_scripts": ["play-mines=src.mines:main"]}
)
