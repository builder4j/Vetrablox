-- based on Novetus' 2007M CSServer (MIT License, https://github.com/Novetus/Novetus_src)
local placeId, port = ...
local server  = game:service("NetworkServer")
local runService = game:service("RunService")
local players = game:service("Players")

if placeId and placeId > 0 then
    game:Load("http://localhost/asset/?id=" .. placeId)
end

pcall(function() game:service("Visit"):SetUploadUrl("") end)
server:Start(port, 20)
runService:Run()

local function applyBodyColors(player)
    local ok, data = pcall(function()
        return game:HttpGet("http://localhost/Game/BodyColors.ashx?userId=" .. player.userId)
    end)
    if not ok or data == nil then
        print("BodyColors fetch failed for '" .. player.Name .. "': " .. tostring(data))
        return
    end

    local parts = { "Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg" }
    local i = 1
    for color in string.gmatch(data, "%d+") do
        local part = player.Character and player.Character:findFirstChild(parts[i])
        if part then part.BrickColor = BrickColor.new(tonumber(color)) end
        i = i + 1
    end
end

local function spawnCharacter(player)
    player:LoadCharacter()
    applyBodyColors(player)
end

players.PlayerAdded:connect(function(player)
    print("Player '" .. player.Name .. "' with ID '" .. player.userId .. "' added")
    spawnCharacter(player)

    coroutine.resume(coroutine.create(function()
        while player.Parent ~= nil do
            wait(0.1)
            local character = player.Character
            if character ~= nil then
                local humanoid = character:findFirstChild("Humanoid")
                if (humanoid ~= nil and humanoid.Health == 0) or character.Parent == nil then
                    wait(5)
                    spawnCharacter(player)
                end
            end
        end
    end))
end)

players.PlayerRemoving:connect(function(player)
    print("Player '" .. player.Name .. "' with ID '" .. player.userId .. "' leaving")
end)

pcall(function() game.Close:connect(function() server:Stop() end) end)
