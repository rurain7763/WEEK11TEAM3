#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"

class UAsset : public UObject
{
	DECLARE_CLASS(UAsset, UObject)

public:
	virtual ~UAsset() = default;

	void SetPath(FString Path) { AssetPath = Path; }
	const FString& GetPath() const { return AssetPath; }

	// 실제 저장 파일 경로. 연결된 파일이 없으면 빈 문자열이다.
	void SetAssetFilePath(FString Path) { AssetFilePath = Path; }
	const FString& GetAssetFilePath() const { return AssetFilePath; }

private:
	FString AssetPath;
	FString AssetFilePath;
};

