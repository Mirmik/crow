import pycrow
import pycrow.libcrow

from pycrow.libcrow import *

def spin():
	while 1:
		pycrow.libcrow.onestep()

def diagnostic(pack=False, live=False):
	if pack:
		pycrow.libcrow.diagnostic()

	if live:
		pycrow.libcrow.live_diagnostic()

def incoming_handler(func):
	pycrow.libcrow.set_incoming_handler(func)

subs = dict()
subs_init = False

def subscribe_handler_impl(packet):
	thm = packet.theme()
	subs[thm](packet)

def subscribe(theme, handler, **kwargs):
	subs[theme] = handler

	if not subs_init:
		pycrow.libcrow.set_subscribe_handler(subscribe_handler_impl)

	return pycrow.libcrow.subscribe(theme, **kwargs)


def use_environment_crowker():
	pycrow.libcrow.set_crowker(pycrow.libcrow.environment_crowker())