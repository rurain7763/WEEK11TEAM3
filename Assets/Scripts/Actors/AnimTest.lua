local MouseSensitivity = 0.1

function Init()
    Log("Init AnimTest")
end

function BeginPlay()
    Log("BeginPlay")
end

function Tick(deltaTime)
    if owner:IsInputEnabled() and Input.IsMouseDown(MouseButton.Right) then
        local dx, dy = Input.GetMouseDelta()
        owner:AddYawInput(dx * MouseSensitivity)
        owner:AddPitchInput(dy * MouseSensitivity)
    end

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
        animInstance:ResetTrigger("Attack")
        animInstance:ResetTrigger("Hit")
        animInstance:ResetTrigger("Jump")
        animInstance:PlayState("test", "Locomotion", 0.2)
    end
    
    -- 전이 규칙 판단
    if(Input.IsMousePressed(MouseButton.Left)) then
        animInstance:SetTrigger("Attack")
        Log("Attack")
    end

    if(Input.IsKeyPressed(KeyCode.Num1)) then
        animInstance:SetTrigger("Hit")
        Log("Hit")
    
    elseif(Input.IsKeyPressed(KeyCode.Space)) then
        animInstance:SetTrigger("Jump")
        Log("Jump")
    -- 상태 이름으로 직접 전환 
    elseif(Input.IsKeyPressed(KeyCode.Num2)) then
        local ok = animInstance:PlayState("test", "Attack", 0.2)
        Log("PlayState -> " .. tostring(ok))
    end

    UpdateMovement(deltaTime, animInstance)
end

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

    if owner:IsInputEnabled() then
        -- 루트 모션이 월드 공간으로 변환되기 전에 캐릭터를 시점의 수평 방향으로 돌린다.
        local cameraYaw = owner:GetControlRotation().Yaw
        owner:SetActorRotation(Rotator.new(0.0, cameraYaw, 0.0))
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
