"""Runs PyInstaller (arguments as for `python -m PyInstaller`) in an environment that has the obsolete
`enum34` distribution installed (anaconda's base site-packages, shared by packaging/.venv).

PyInstaller refuses to start when the enum34 metadata exists. Here it is inert: the standard library's
enum.py comes first on sys.path, so nothing imports (or bundles) site-packages/enum. Only that case is let
through; any other backport, or an enum34 that actually shadows the standard library, still stops the build.
"""
import enum
import importlib.metadata as md
import os
import sys
import sysconfig

_distribution = md.distribution
_STDLIB_ENUM = os.path.dirname(os.path.abspath(enum.__file__)) == os.path.abspath(sysconfig.get_paths()["stdlib"])


def distribution(name):
    if name == "enum34" and _STDLIB_ENUM:
        raise md.PackageNotFoundError(name)
    return _distribution(name)


md.distribution = distribution

if __name__ == "__main__":
    from PyInstaller.__main__ import run
    sys.exit(run(sys.argv[1:]))
