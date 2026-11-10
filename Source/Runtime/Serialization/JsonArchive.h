#pragma once

class UObject;
class UWorld;
class AActor;
class UMovieSceneSequence;
class UAnimGraph;
class UBlendSpace1D;

class FJsonArchive
{
public:
	static bool SaveWorld(UWorld* World, const FString& Path);
	static bool LoadWorld(UWorld* World, const FString& Path);
	static bool LoadWorldFromBytes(UWorld* World, const TArray<uint8>& Bytes);
	static bool SaveWorldToBytes(UWorld* World, TArray<uint8>& OutBytes);

	// 액터 목록을 씬 파일과 같은 형식으로 직렬화한다 (에디터 복사). 저장할 액터가 없으면 false다.
	static bool SaveActorsToBytes(const TArray<AActor*>& Actors, TArray<uint8>& OutBytes);
	// SaveActorsToBytes 결과로 World에 새 액터를 만든다 (에디터 붙여넣기). 기존 액터는 건드리지 않는다.
	static bool SpawnActorsFromBytes(UWorld* World, const TArray<uint8>& Bytes, TArray<AActor*>& OutActors);

	static bool SaveSequence(UMovieSceneSequence* Sequence, const FString& Path);
	static bool SaveSequenceToBytes(UMovieSceneSequence* Sequence, TArray<uint8>& OutBytes);

	static bool LoadSequence(UMovieSceneSequence* Sequence, const FString& Path);
	static bool LoadSequenceFromBytes(UMovieSceneSequence* Sequence, const TArray<uint8>& Bytes);

	static bool SaveAnimGraph(UAnimGraph* Graph, const FString& Path);
	static bool SaveAnimGraphToBytes(UAnimGraph* Graph, TArray<uint8>& OutBytes);

	static bool LoadAnimGraph(UAnimGraph* Graph, const FString& Path);
	static bool LoadAnimGraphFromBytes(UAnimGraph* Graph, const TArray<uint8>& Bytes);

	static bool SaveBlend1D(UBlendSpace1D* Blend1D, const FString& Path);
	static bool SaveBlend1DToBytes(UBlendSpace1D* Blend1D, TArray<uint8>& OutBytes);

	static bool LoadBlend1DGraph(UBlendSpace1D* Blend1D, const FString& Path);
	static bool LoadBlend1DFromBytes(UBlendSpace1D* Blend1D, const TArray<uint8>& Bytes);

private:
	static bool WriteFile(const FString& Path, const TArray<uint8>& Bytes);
	static bool ReadFile(const FString& Path, TArray<uint8>& OutBytes);
	static bool SaveObjectToBytes(UObject* Object, const char* RecordName, int32 Version, TArray<uint8>& OutBytes);
	static bool LoadObjectFromBytes(UObject* Object, const char* RecordName, int32 ExpectedVersion, const TArray<uint8>& Bytes);
};
