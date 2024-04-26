-- I have no experience with storage API in OTS
-- or I am a Lua expert. However I took a look in
-- internet to understand the basics of OTS storages: https://otland.net/threads/lua-understanding-storages.189075/

-- Let's make the storage IDS more readable
local STORAGE_IDS = {["LOGGED_IN"] = 1000}

-- and make the table of storage ids immutable
setmetatable(STORAGE_IDS, {__newindex = function(t, k, v) error("Immutable table!") end,
                           __index = function(t, k) return t[k] end })

-- Symbolic variables (no const in Lua)
local NOT_USED_STORAGE = -1


local function releaseStorage(player)
    player:setStorageValue(STORAGE_IDS["LOGGED_IN"], NOT_USED_STORAGE)
end

-- function onLogout sets the storage for LOGGED_IN key
function onLogout(player)
  -- cache the key for login
  local loggedInKey = STORAGE_IDS["LOGGED_IN"]
  if player:getStorageValue(loggedInKey) == 1 then
    addEvent(releaseStorage, loggedInKey, player)
    return true
  else
    return false
  end
end
