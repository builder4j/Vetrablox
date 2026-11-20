-- based on Novetus' 2007M CSConnect (MIT License, https://github.com/Novetus/Novetus_src)
local client = game:service("NetworkClient")
local player = game:service("Players"):CreateLocalPlayer(1)

player:SetSuperSafeChat(false)
pcall(function() player:SetUnder13(false) end)
pcall(function() player.Name = "Player" end)
pcall(function() game:service("Visit"):SetUploadUrl("") end)

local function dieError(message)
    game:SetMessage(message)
    wait(math.huge)
end

client.ConnectionAccepted:connect(function(peer, replicator)
    replicator.Disconnection:connect(function(peer, lostConnection)
        game:SetMessage("You have lost connection to the game")
    end)

    local marker = nil
    local ok, err = pcall(function()
        game:SetMessageBrickCount()
        marker = replicator:SendMarker()
    end)
    if not ok then dieError(err) end

    marker.Received:connect(function()
        game:ClearMessage()
    end)
end)

client.ConnectionRejected:connect(function()
    dieError("Failed to connect to the Game. (Connection rejected)")
end)

client.ConnectionFailed:connect(function(peer, code)
    dieError("Failed to connect to the Game. (Connection failed, " .. tostring(code) .. ")")
end)

game:SetMessage("Connecting to server...")
client:Connect("127.0.0.1", 53640, 0, 20)
