#include "EnginePCH.h"
#include "Animation/AnimParameter.h"

void FAnimParameterTable::RemoveParameter(FName Name)
{
	if (HasParameter(Name))
	{
		return;
	}

	int32 ParamIndex = ParameterNameToIndex[Name];

	FAnimParameter& Last = Parameters.Last();
	ParameterNameToIndex[Last.Name] = ParamIndex;

	Parameters.RemoveAtSwap(ParamIndex);
	ParameterNameToIndex.Remove(Name);
}
