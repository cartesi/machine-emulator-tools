#!/usr/bin/env lua

local line = io.read('*a')
io.stderr:write(string.format("%8s:%d req %s\n",
	debug.getinfo(1).source,
	debug.getinfo(1).currentline,
	'>|' .. line .. '|<'))
io.write('that is all she wrote')
