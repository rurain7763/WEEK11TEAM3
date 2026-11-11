#pragma once

#include <cassert>
#include <unordered_set>
#include "Core/Types.h"


template <typename T, typename KeyFuncs = std::hash<T>>
class TSet
{
public:
	using SetType = std::unordered_set<T, KeyFuncs>;

	TSet() = default;
	~TSet() = default;

	inline SetType::iterator begin()
	{
		return mSet.begin();
	}

	inline SetType::iterator end()
	{
		return mSet.end();
	}

	inline SetType::const_iterator begin() const
	{
		return mSet.cbegin();
	}

	inline SetType::const_iterator end() const
	{
		return mSet.cend();
	}

	inline bool Add(const T& data)
	{
		return mSet.insert(data).second;
	}

	// Returns removed count
	inline int32 Remove(const T& data)
	{
		return static_cast<int32>(mSet.erase(data));
	}

	inline SetType::iterator Remove(SetType::iterator Pos)
	{
		return mSet.erase(Pos);
	}

	inline int32 Num() const
	{
		return static_cast<int32>(mSet.size());
	}

	inline void Reset()
	{
		mSet.clear();
	}

	inline bool Contains(const T& data) const
	{
		return mSet.find(data) != mSet.end();
	}

	bool IsEmpty() const
	{
		return mSet.empty();
	}

	void Reserve(int32 capacity)
	{
		mSet.reserve(capacity);
	}

	/*
	void Empty(int32 ExpectedNumElements = 0)
	*/

private:
	SetType mSet;
};

