#!/usr/bin/env python3
#coding: utf-8

import licant
from licant.cxx_modules import application
from licant.libs import include
import os

licant.libs.include("crow")

licant.libs.include("nos")
licant.libs.include("igris")

gates = [
	"crow.udpgate",
	#"crow.serial_gstuff",
]

mdepends = ["crow"]
mdepends.extend(gates)

application("ctrans", 
	sources = ["main.c"],
	mdepends = mdepends,
	cxx_flags = "-Wextra -Wall",
	libs = ["pthread", "readline"],
	defines=[
		"GATES_UDPGATE",
		#"GATES_SERIAL_GSTUFF_GATE",
		"CONSOLE_INTERFACE"
	]
)

@licant.routine
def install():
	licant.do("ctrans")
	os.system("cp ctrans /usr/local/bin")

licant.ex("ctrans")