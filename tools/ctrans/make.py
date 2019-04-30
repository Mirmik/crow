#!/usr/bin/env python3
#coding: utf-8

import licant
from licant.cxx_modules import application
from licant.libs import include
import os

licant.libs.include("crow")
licant.libs.include("igris")

application("ctrans", 
	sources = ["main.c"],
	include_modules = [
		("crow"),
		("crow.allocator", "malloc"),
		("crow.time", "chrono"),
	
		("crow.udpgate"),
		("crow.serial_gstuff"),
		
		("igris", "posix"),
		("igris.inet", "posix"),
		("igris.print", "cout"),
		("igris.dprint", "cout"),
		("igris.syslock", "mutex"),
		("igris.serial"),
	],
	cxx_flags = "-Wextra -Wall",
	libs = ["pthread", "readline"]
)

@licant.routine
def install():
	licant.do("ctrans")
	os.system("cp ctrans /usr/local/bin")

licant.ex("ctrans")