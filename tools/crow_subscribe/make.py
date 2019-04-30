#!/usr/bin/env python3
#coding: utf-8

import licant
from licant.cxx_modules import application
from licant.libs import include
import os

licant.libs.include("igris")
licant.libs.include("crow")

application("crow_subscribe",
	sources = ["main.cpp"],
	include_modules = [
		("crow"),
		("crow.allocator", "malloc"),
		("crow.time", "chrono"),

		("crow.udpgate"),

		("igris", "posix"),
		("igris.trent"),
		("igris.inet", "posix"),
		("igris.print", "cout"),
		("igris.dprint", "cout"),
		("igris.syslock", "mutex"),
	],
	cxx_flags = ""
)

@licant.routine
def install():
	licant.do("crow_subscribe")
	os.system("cp crow_subscribe /usr/local/bin")

licant.ex("crow_subscribe")