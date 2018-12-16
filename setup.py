#!/usr/bin/env python3
#coding: utf-8

from wheel.bdist_wheel import bdist_wheel as bdist_wheel_
from setuptools import setup, Extension, Command
from distutils.util import get_platform
import os

import licant

licant.include("gxx")
licant.cxx_objects("gxx-objects", mdepends = ["gxx"] )
gxxopts = licant.core.core.get("gxx-objects").lopts

class bdist_wheel(bdist_wheel_):
	def finalize_options(self):
		from sys import platform as _platform
		platform_name = get_platform()

		if _platform == "linux" or _platform == "linux2":
			# Linux
			if platform_name == "linux-i686":
				platform_name = 'manylinux1_i686'
			else:
				platform_name = 'manylinux1_x86_64'
			
		bdist_wheel_.finalize_options(self)
		self.universal = True
		self.plat_name_supplied = True
		self.plat_name = platform_name

pycrow_lib = Extension("pycrow.libservoce",
	sources = gxxopts["sources"],
	extra_compile_args=['-fPIC', '-std=c++14'],
	extra_link_args=['-Wl,-rpath,$ORIGIN/libs'],
	include_dirs = ["crow"] + gxxopts["include_paths"],
	libraries = [],
)

setup(
	name = 'pycrow',
	version = '0.0.1',
	license='MIT',
	description = 'Messaging system',
	author = 'Sorokin Nikolay',
	author_email = 'mirmikns@yandex.ru',
	url = 'https://mirmik.github.io/zencad/',
	keywords = ['testing', 'cad'],
	classifiers = [],

	include_package_data=True,
	ext_modules = [pycrow_lib],
	cmdclass = {"bdist_wheel" : bdist_wheel}
)
