
function Init()
    Log("Init AnimTest")
end

function BeginPlay()
    Log("BeginPlay")
end

function Tick(deltaTime)
    local mesh = owner:FindComponentByClass(SkeletalMeshComponent.StaticClass())    
    if(mesh == nil) then
        Log("No Skeletal Mesh Component mesh")
        return
    end
    
    local animInstance = mesh:GetAnimInstance()
    if(animInstance == nil) then
        Log("No Anim Instance")
        return
    end
        
    if(Input.IsKeyPressed(KeyCode.Num5)) then
        animInstance:SetInt("Action", 0)
        Log("Action -> " .. animInstance:GetInt("Action"))
    
    elseif(Input.IsKeyPressed(KeyCode.Num6)) then
        animInstance:SetInt("Action", 1)
        Log("Attack -> " .. animInstance:GetInt("Action"))
    
    elseif(Input.IsKeyPressed(KeyCode.Num7)) then
        animInstance:SetInt("Action", 2)
        Log("Hit -> " .. animInstance:GetInt("Action"))
    
    elseif(Input.IsKeyPressed(KeyCode.Num8)) then
        animInstance:SetInt("Action", 3)
        Log("Jump -> " .. animInstance:GetInt("Action"))
    elseif(Input.IsKeyPressed(KeyCode.Num9)) then
        local ok = animInstance:PlayState("Test", "Attack", 0.2)
        Log("PlayState -> " .. tostring(ok))
    end

    UpdateMovement(deltaTime, animInstance)
end

function UpdateMovement(deltaTime, animInstance)
    local bMoving = Input.IsKeyDown(KeyCode.W) or Input.IsKeyDown(KeyCode.A) or Input.IsKeyDown(KeyCode.S) or Input.IsKeyDown(KeyCode.D)

    if not bMoving then
        animInstance:SetFloat("Speed", 0.0)
        return
    end

    local speed = 100.0
    if Input.IsKeyDown(KeyCode.Shift) then
        speed = 200.0
    end

    if Input.IsKeyDown(KeyCode.S) then
       speed = -speed
    end

    animInstance:SetFloat("Speed", speed)
end


function EndPlay()
end
