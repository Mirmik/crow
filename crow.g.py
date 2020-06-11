import licant.modules
import licant

#licant.module("crow.netkeep_crowker", 
#	sources=["crow/src/netkeep_crowker.cpp"]
#)

licant.module("crow.select", "impl",
	sources=["crow/select.cpp"],
	defines = ["CROW_WITHOUT_SELECT_FDS=0"], default=True
)

licant.module("crow.select", "stub",
	defines = ["CROW_WITHOUT_SELECT_FDS=1"]
)

licant.module("crow.diagnostic", "nos",
	sources=["crow/src/print.cpp"],
	default = True
)

licant.module("crow.threads", "linux", default=True,
	sources = ["crow/src/threads-posix.cpp"]
)

licant.modules.module("crow",
	srcdir = "crow",
	sources = [
		"src/tower.cpp", 
		"src/towerxx.cpp", 
		"src/packet.cpp",
		"src/packet_ptr.cpp",
		"src/alive.cpp",
		"src/hexer.c",
		"src/address.cpp",
		"src/gateway.cpp",
	
		"proto/acceptor.cpp",
		"proto/node.cpp",
		"proto/socket.cpp",
		"proto/node-sync.cpp",
		"proto/channel.cpp",
		"proto/channel-sync.cpp",
		"proto/pubsub.cpp",

		"addons/noslogger.cpp",
	],

	mdepends = [
		"crow.allocator",
		"crow.time",
		"crow.include",
		"crow.threads",
		"crow.diagnostic",
		"crow.select"
	]
)

licant.module("crow.crowker",
	#mdepends = ["crow.netkeep_crowker"]
)

licant.modules.module("crow.minimal",
	srcdir = "crow",
	sources = [
		"src/packet.cpp",
		"src/packet_ptr.cpp",
		"src/tower.cpp",
		"proto/node.cpp",
		"proto/node-sync.cpp",
		"proto/channel.cpp",
		"src/hexer.c"
	],
	mdepends = ["crow.include", "crow.diagnostic"]
)

licant.modules.module("crow.diagnostic", "debug",
	sources=["crow/src/print-debug.cpp"]
)

licant.modules.module("crow.include", 
	include_paths=["."])

licant.modules.module("crow.allocator", "malloc", 
	sources=[ "crow/src/allocation_malloc.cpp"], default=True)

licant.modules.module("crow.allocator", "pool", 
	sources=["crow/src/allocation_pool.cpp"])

licant.modules.module("crow.time", "chrono", 
	sources=["crow/src/stdtime.cpp"], default=True)

#######################################GATES#########################################

licant.modules.module("crow.udpgate", 
	sources=["crow/gates/udpgate.cpp"])

licant.modules.module("crow.serial_gstuff", 
	sources=["crow/gates/serial_gstuff.cpp"], 
	mdepends=[
		#"igris.protocols.gstuff"
	])

#####################################################################################
####################################PROTOCOLS########################################

licant.module("crow.protocol.pubsub",
	sources = ["crow/proto/pubsub.cpp"]
)

#####################################################################################