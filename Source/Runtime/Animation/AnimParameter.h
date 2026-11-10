#pragma once

#include "Core/NameTypes.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include <type_traits>

enum class EAnimParameterType
{
	Int32,
	Bool,
	Float
};

union FAnimParameterValue
{
	int32 Int32;
	float Float;
	bool Bool;
};

struct FAnimParameter
{
	FName Name;
	EAnimParameterType Type;
	FAnimParameterValue Value;
};

class FAnimParameterTable
{
public:
	template <typename T>
	void AddParameter(const FName& Name, T InitValue)
	{
		if (HasParameter(Name))
		{
			return;
		}

		FAnimParameter NewParameter;
		NewParameter.Name = Name;
		if constexpr (std::is_same_v<T, int32>)
		{
			NewParameter.Type = EAnimParameterType::Int32;
			NewParameter.Value.Int32 = InitValue;
		}
		else if (std::is_same_v<T, float>)
		{
			NewParameter.Type = EAnimParameterType::Float;
			NewParameter.Value.Float = InitValue;
		}
		else if (std::is_same_v<T, bool>)
		{
			NewParameter.Type = EAnimParameterType::Bool;
			NewParameter.Value.Bool = InitValue;
		}

		int32 ParamIndex = Parameters.Emplace(NewParameter);
		ParameterNameToIndex[Name] = ParamIndex;
	}

	void RemoveParameter(FName Name);

	bool HasParameter(const FName& Name) const
	{
		return ParameterNameToIndex.Contains(Name);
	}

	const FAnimParameter& FindParameter(const FName& Name) const
	{
		int32 ParamIndex = ParameterNameToIndex[Name];
		return Parameters[ParamIndex];
	}

private:
	TMap<FName, int32, FNameHash> ParameterNameToIndex;
	TArray<FAnimParameter> Parameters;
};
