print("Help! Being squished!")
print(require)

--[[
local a = setmetatable({},{
	__index = setmetatable({88},{
		__index = print
		})
	})
]]--

-- Following string caused a crash because of lua bug, does not anymore
local a = setmetatable({},{__gc = function() error("Uh-oh") end})


warn("Um, hello.")
error("Sorry, we're closed.")
