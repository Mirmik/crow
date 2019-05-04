import licant.modules

licant.modules.module("crow.include",
	include_paths=["."]
)

licant.modules.module("crow",
	srcdir = "crow/src",
	sources = [
		"tower.c", 
		"packet.c",
		"node.c",
		"channel.c",
		"pubsub.c",
		"hexer.c",
		"print.cpp",
	],
	include_paths=["."],
	mdepends=[
		"igris.include", 
		"igris.util",

		"igris.dprint",
		"igris.syslock",

		"nos.print",
		"nos.fprint",
		"nos.include",

		"crow.allocator",
		"crow.time"
	]
)

licant.modules.module("crow.allocator", "malloc",
	sources=[
		"crow/src/allocation_malloc.c"
	],
	default=True
)

licant.modules.module("crow.udpgate",
	sources=[
		"crow/gates/udpgate.c"
	]
)

licant.modules.module("crow.serial_gstuff",
	sources=[
		"crow/gates/serial_gstuff.c"
	]
)

licant.modules.module("crow.time", "chrono",
	sources=[
		"crow/src/stdtime.cpp"
	],
	default=True
)