#!/usr/bin/env python3
#coding: utf-8

import licant
import licant.libs
from licant.cxx_modules import application
from licant.modules import submodule

licant.libs.include("igris")
licant.libs.include("crow")

application("target",
	sources = ["main.cpp"],
	include_paths = ["../.."],
	include_modules = [
		("igris", "posix"),
		("igris.print", "cout"),
		("igris.dprint", "cout"),
		("igris.log2", "impl"),
		("igris.syslock", "mutex"),

		("crow"),
		("crow.allocator", "malloc"),
		("crow.time", "chrono"),
		
	],
)

licant.ex("target")