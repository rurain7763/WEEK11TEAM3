local SLIDER_HALF_WIDTH = 1.55
local MAX_SPEED = 200.0

-- Time in seconds, then the Speed input shared by both exhibits.
local speedKeys = {
    {0.00, 0}, {0.55, 0}, {1.25, 100}, {1.95, 100},
    {2.55, 200}, {3.00, 200}, {3.70, 0}, {4.60, 0},
    {5.35, -200}, {6.15, -200}, {6.90, 0}, {8.60, 0},
}

local elapsed
local previewMesh
local rootMotionMesh
local sliders = {}

local function GetCycleSpeed(time)
    for index = 2, #speedKeys do
        local nextKey = speedKeys[index]
        if time <= nextKey[1] then
            local previousKey = speedKeys[index - 1]
            local alpha = (time - previousKey[1]) / (nextKey[1] - previousKey[1])
            return previousKey[2] + (nextKey[2] - previousKey[2]) * alpha
        end
    end
    return speedKeys[#speedKeys][2]
end

local function SetSpeed(mesh, speed)
    local animInstance = mesh and mesh:GetAnimInstance()
    if animInstance then
        animInstance:SetFloat("Speed", speed)
    end
end

function Init()
    elapsed = 0.0
end

function BeginPlay()
    local preview = GamePlayStatics.GetActorByName(owner, "BlendSpace_Preview")
    previewMesh = preview and preview:FindComponentByClass(SkeletalMeshComponent.StaticClass())
    rootMotionMesh = owner:FindComponentByClass(SkeletalMeshComponent.StaticClass())
    for _, name in ipairs({"Preview_Speed_Handle", "Movement_Speed_Handle"}) do
        local handle = GamePlayStatics.GetActorByName(owner, name)
        if handle then
            sliders[#sliders + 1] = {actor = handle, center = handle:GetActorLocation()}
        end
    end
end

function Tick(deltaTime)
    elapsed = (elapsed + deltaTime) % speedKeys[#speedKeys][1]
    local speed = GetCycleSpeed(elapsed)
    SetSpeed(previewMesh, speed)
    SetSpeed(rootMotionMesh, speed)

    for _, slider in ipairs(sliders) do
        local center = slider.center
        -- Positive world X points left in the overview.
        local x = center.x - speed / MAX_SPEED * SLIDER_HALF_WIDTH
        if math.abs(slider.actor:GetActorLocation().x - x) > 0.0001 then
            slider.actor:SetActorLocation(Vector.new(x, center.y, center.z))
        end
    end
end

function EndPlay()
    previewMesh = nil
    rootMotionMesh = nil
    sliders = {}
end
