#!/usr/bin/env python3
#coding: utf-8

import licant
from licant.cxx_modules import application
from licant.libs import include

licant.libs.include("igris")
licant.libs.include("crow")

application("target", 
	sources = ["main.cpp"],
	include_modules = [
		("crow"),
		("crow.allocator", "malloc"),
		("crow.time", "chrono"),
#		("crow.node"),
		
		("igris", "posix"),
		("igris.inet", "posix"),
		("igris.print", "cout"),
		("igris.dprint", "cout"),
		("igris.syslock", "mutex"),
	],
	cxx_flags = ""
)

licant.ex("target")