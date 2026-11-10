#pragma once

#include "Animation/AnimParameter.h"
#include <stdexcept>

enum class EAnimComparisonOperator
{
	Equal,
	NotEqual,
	Greater,
	Less,
	GreaterEqual,
	LessEqual,
};

struct FAnimTransitionCondition
{
	FName ParameterName;
	EAnimComparisonOperator ComparisonOperator;
	FAnimParameterValue Value;

	template <typename T>
	bool CompareValues(const T& A, const T& B) const
	{
		switch (ComparisonOperator)
		{
			case EAnimComparisonOperator::Equal: return A == B;
			case EAnimComparisonOperator::NotEqual: return A != B;
			case EAnimComparisonOperator::Greater: return A > B;
			case EAnimComparisonOperator::Less: return A < B;
			case EAnimComparisonOperator::GreaterEqual: return A >= B;
			case EAnimComparisonOperator::LessEqual: return A <= B;
		}

		throw std::runtime_error("Unknown animation comparison operator");
	}

	bool Evaluate(const FAnimParameter& Param) const;
};

struct FAnimStateTransition
{
	FAnimTransitionCondition Condition;
	int32 NextState = -1;
	float CrossfadeDuration = 0.f;
	bool bHasExitTime = false;
	float ExitTime = 1.f;
};

struct FAnimActiveTransition
{
	float CrossfadeDuration = 0.f;
	float ElapsedTime = 0.f;
	int32 NextState = -1;
	int32 PreviousState = -1;
};
