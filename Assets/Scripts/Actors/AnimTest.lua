
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

-- WASD 입력을 Speed 파라미터로만 넘긴다.
-- BlendSpace(Locomotion)가 Idle(0) / Walk(100) / Jog(300)을 섞고,
-- 실제 이동은 루트 모션이 담당하므로 여기서 액터 위치를 건드리지 않는다.
local WalkSpeed = 100.0
local SprintSpeed = 300.0

function UpdateMovement(deltaTime, animInstance)
    local bMoving =
        Input.IsKeyDown(KeyCode.W) or
        Input.IsKeyDown(KeyCode.S) or
        Input.IsKeyDown(KeyCode.A) or
        Input.IsKeyDown(KeyCode.D)

    if not bMoving then
        animInstance:SetFloat("Speed", 0.0)
        return
    end

    local speed = WalkSpeed
    if Input.IsKeyDown(KeyCode.Shift) then
        speed = SprintSpeed
    end

    if Input.IsKeyDown(KeyCode.S) then
        speed = speed * -1.0
    end

    Log("Speed " .. tostring(speed))

    animInstance:SetFloat("Speed", speed)
end


function EndPlay()
end
