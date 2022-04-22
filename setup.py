from skbuild import setup

setup(
    name="sort-tracker",
    version="1.0.6",
    description="SORT tracker",
    author='M.Ali Zarrinzade',
    author_email="ali.zarrinzadeh@gmail.com",
    packages=['sort'],
    install_requires = [
        'numpy',
    ],
    # python_requires=">=3.7",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    license="GPL-3.0",
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'Programming Language :: Python :: 3',
    ],
    url="https://github.com/MrGolden1/sort-python",
    project_urls={
        "Bug Tracker": "https://github.com/MrGolden1/sort-python/issues",
    },
)