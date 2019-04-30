#!/usr/bin/env python3
#coding: utf-8

import licant
from licant.cxx_modules import application
from licant.libs import include
import os

licant.libs.include("igris")
licant.libs.include("crow")

application("crowker", 
	sources = ["main.cpp", "brocker.cpp"],
	include_modules = [
		("crow"),
		("crow.allocator", "malloc"),
		("crow.time", "chrono"),
		
		("crow.udpgate"),
		
		("igris", "posix"),
#		("igris.inet", "posix"),
		("igris.print", "cout"),
		("igris.dprint", "cout"),
		("igris.syslock", "mutex"),
	],
	cxx_flags = ""
)

@licant.routine
def install():
	licant.do("crowker")
	os.system("cp crowker /usr/local/bin")

licant.ex("crowker")