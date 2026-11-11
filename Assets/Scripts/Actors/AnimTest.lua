local MouseSensitivity = 0.1
local mesh
local animInstance
local handleNotifyDelegateKey
local overviewCamera
local viewingOverview = false

function Init()
    Log("Init AnimTest")
end

function HandleNotify(name)
    if name == "Footstep" then
        GamePlayStatics.PlaySound("Assets/Sounds/Pop.wav", 1.0)
    end
end

function BeginPlay()
    mesh = owner:FindComponentByClass(SkeletalMeshComponent.StaticClass())    
    if(mesh == nil) then
        Log("No Skeletal Mesh Component mesh")
        return
    end
    
    animInstance = mesh:GetAnimInstance()
    if(animInstance == nil) then
        Log("No Anim Instance")
        return
    end

    handleNotifyDelegateKey = animInstance:BindNoifyCallback(HandleNotify)

    overviewCamera = GamePlayStatics.GetActorByName(owner, "Overview_Camera")
    if overviewCamera then
        GamePlayStatics.SetControlRotation(owner, Rotator.new(17.0, owner:GetControlRotation().Yaw, 0.0))
        GamePlayStatics.SetViewTarget(owner, overviewCamera)
        viewingOverview = true
    end

    Log("BeginPlay")
end

function Tick()
    if not animInstance then
        return
    end

    if not owner:IsInputEnabled() then
        animInstance:SetFloat("Speed", 0.0)
        UpdateJump(animInstance, false)
        return
    end

    if overviewCamera and Input.IsKeyPressed(KeyCode.C) then
        viewingOverview = not viewingOverview
        GamePlayStatics.SetViewTarget(owner, viewingOverview and overviewCamera or owner)
    end

    if Input.IsMouseDown(MouseButton.Right) then
        local dx, dy = Input.GetMouseDelta()
        if dx ~= 0 or dy ~= 0 then
            if viewingOverview then
                GamePlayStatics.SetViewTarget(owner, owner)
                viewingOverview = false
            end
            owner:AddYawInput(dx * MouseSensitivity)
            owner:AddPitchInput(dy * MouseSensitivity)
        end
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

    UpdateJump(animInstance, true)
    UpdateMovement(animInstance)
end

--   올라가는 중(ZVelocity > 0)  : Jump_Start
--   내려오는 중(ZVelocity <= 0) : Jump_Loop
--   바닥에 닿음(Grounded)       : Jump_Land
local JumpSpeed = 8.0

function UpdateJump(animInstance, inputEnabled)
    local movement = owner:FindComponentByClass(CharacterMovementComponent.StaticClass())
    if movement == nil then
        return
    end

    local grounded = movement:IsGrounded()
    if inputEnabled and grounded and Input.IsKeyPressed(KeyCode.Space) then
        movement:Jump(JumpSpeed)
        animInstance:SetTrigger("Jump")
        Log("Jump")
    end

    animInstance:SetFloat("ZVelocity", movement:GetVelocity().z)
    animInstance:SetBool("Grounded", grounded)
end

-- Locomotion BlendSpace sample values.
local WalkSpeed = 100.0
local SprintSpeed = 200.0
local BackwardSpeed = -200.0

function UpdateMovement(animInstance)
    local forward = (Input.IsKeyDown(KeyCode.W) and 1 or 0) - (Input.IsKeyDown(KeyCode.S) and 1 or 0)
    local right = (Input.IsKeyDown(KeyCode.D) and 1 or 0) - (Input.IsKeyDown(KeyCode.A) and 1 or 0)
    if forward == 0 and right == 0 then
        animInstance:SetFloat("Speed", 0.0)
        return
    end

    local cameraYaw = owner:GetControlRotation().Yaw
    local yawOffset = math.deg(math.atan(right, math.abs(forward)))
    if forward < 0 then
        yawOffset = -yawOffset
    end
    owner:SetActorRotation(Rotator.new(0.0, cameraYaw + yawOffset, 0.0))

    local speed = Input.IsKeyDown(KeyCode.Shift) and SprintSpeed or WalkSpeed
    if forward < 0 then
        speed = BackwardSpeed
    end
    animInstance:SetFloat("Speed", speed)
end


function EndPlay()
    local activeInstance = mesh and mesh:GetAnimInstance()
    if activeInstance and handleNotifyDelegateKey then
        activeInstance:UnbindNotifyCallback(handleNotifyDelegateKey)
    end
    mesh = nil
    animInstance = nil
    handleNotifyDelegateKey = nil
    overviewCamera = nil
    viewingOverview = false
end
