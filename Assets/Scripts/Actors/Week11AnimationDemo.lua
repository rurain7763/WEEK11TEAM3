local SLIDER_HALF_WIDTH = 1.55
local END_PAUSE = 1.6

local meshOnly
local rootMotionMesh
local startLocation
local meshOnlySlider
local rootMotionSlider
local endTime

local function FindSlider(name)
    local actor = GamePlayStatics.GetActorByName(owner, name)
    if actor then
        return {actor = actor, center = actor:GetActorLocation()}
    end
end

local function UpdateSlider(slider, progress)
    if slider then
        local center = slider.center
        -- Positive world X points left in the overview.
        local x = center.x + (1.0 - 2.0 * progress) * SLIDER_HALF_WIDTH
        if math.abs(slider.actor:GetActorLocation().x - x) > 0.0001 then
            slider.actor:SetActorLocation(Vector.new(x, center.y, center.z))
        end
    end
end

function Init()
    endTime = 0.0
end

function BeginPlay()
    local meshOnlyActor = GamePlayStatics.GetActorByName(owner, "MeshOnly_Demo")
    meshOnly = meshOnlyActor and meshOnlyActor:FindComponentByClass(SkeletalMeshComponent.StaticClass())
    rootMotionMesh = owner:FindComponentByClass(SkeletalMeshComponent.StaticClass())
    startLocation = owner:GetActorLocation()
    meshOnlySlider = FindSlider("Preview_Progress_Handle")
    rootMotionSlider = FindSlider("Movement_Progress_Handle")
end

function Tick(deltaTime)
    local meshOnlyAnim = meshOnly and meshOnly:GetAnimInstance()
    local rootMotionAnim = rootMotionMesh and rootMotionMesh:GetAnimInstance()
    if not meshOnlyAnim or not rootMotionAnim then
        return
    end

    local meshOnlyProgress = meshOnlyAnim:GetNormalizedTime()
    local rootMotionProgress = rootMotionAnim:GetNormalizedTime()

    if meshOnlyProgress >= 1.0 and rootMotionProgress >= 1.0 then
        endTime = endTime + deltaTime
        if endTime >= END_PAUSE then
            meshOnlyAnim:PlayState("Comparison", "Jog", 0.0)
            rootMotionAnim:PlayState("Comparison", "Jog", 0.0)
            owner:SetActorLocation(startLocation)
            endTime = 0.0
            meshOnlyProgress = 0.0
            rootMotionProgress = 0.0
        end
    end

    UpdateSlider(meshOnlySlider, math.max(0.0, meshOnlyProgress))
    UpdateSlider(rootMotionSlider, math.max(0.0, rootMotionProgress))
end

function EndPlay()
    meshOnly = nil
    rootMotionMesh = nil
    startLocation = nil
    meshOnlySlider = nil
    rootMotionSlider = nil
end
