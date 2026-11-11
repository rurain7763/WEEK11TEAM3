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
    end

    -- 상태 이름으로 직접 전환
    if(Input.IsKeyPressed(KeyCode.Num2)) then
        local ok = animInstance:PlayState("test", "Attack", 0.2)
        Log("PlayState -> " .. tostring(ok))
    end

    UpdateJump(animInstance)
    UpdateMovement(deltaTime, animInstance)
end

--   올라가는 중(ZVelocity > 0)  : Jump_Start
--   내려오는 중(ZVelocity <= 0) : Jump_Loop
--   바닥에 닿음(Grounded)       : Jump_Land
local JumpSpeed = 8.0

function UpdateJump(animInstance)
    local movement = owner:FindComponentByClass(CharacterMovementComponent.StaticClass())
    if movement == nil then
        return
    end

    if Input.IsKeyPressed(KeyCode.Space) and movement:IsGrounded() then
        movement:Jump(JumpSpeed)
        animInstance:SetTrigger("Jump")
        Log("Jump")
    end

    animInstance:SetFloat("ZVelocity", movement:GetVelocity().z)
    animInstance:SetBool("Grounded", movement:IsGrounded())
end

-- BlendSpace(Locomotion) Idle(0) / Walk(100) / Jog(300)
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
