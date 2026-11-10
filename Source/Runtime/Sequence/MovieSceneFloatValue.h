#pragma once

#include "Math/CurveInterpMode.h"

struct FMovieSceneFloatValue
{
	ECurveInterpMode InterpMode = ECurveInterpMode::Constant;
	float Value = 0.f;
};
