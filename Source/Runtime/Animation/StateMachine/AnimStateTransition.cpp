#include "EnginePCH.h"
#include "Animation/StateMachine/AnimStateTransition.h"

bool FAnimTransitionCondition::Evaluate(const FAnimParameter& Param) const
{
	switch (Param.Type)
	{
		case EAnimParameterType::Int32: return CompareValues(Param.Value.Int32, Value.Int32);
		case EAnimParameterType::Float: return CompareValues(Param.Value.Float, Value.Float);
		case EAnimParameterType::Bool: return CompareValues(Param.Value.Bool, Value.Bool);
		case EAnimParameterType::Trigger: return Param.Value.Trigger;
	}

	throw std::runtime_error("Unknown parameter type");
}
