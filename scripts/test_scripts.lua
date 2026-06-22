-- mover.lua
function on_start()
    printToConsole("Script started! Owner: " .. self:GetName())
end

function on_update(dt)
    local pos = self:GetPosition()
    pos.x = pos.x + 1.0 * dt
    self:SetPosition(pos)
end

function on_destroy()
    printToConsole("Script destroyed")
end