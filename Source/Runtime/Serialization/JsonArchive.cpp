#include "EnginePCH.h"
#include "JsonArchive.h"

#include "Engine/World.h"
#include "Engine/Level.h"
#include "Component/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectHash.h"
#include "GameFramework/Actor/StaticMeshActor.h"
#include "TypeSerializer.h"
#include "Asset/AssetManager.h"
#include "Sequence/MovieSceneSequence.h"
#include "Animation/AnimSequence.h"
#include "Animation/Graph/AnimGraph.h"
#include "Animation/BlendSpace1D.h"

#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/StructuredArchive.h"
#include "Serialization/StructuredArchiveSlots.h"
#include "Serialization/Formatter/JsonArchiveOutputFormatter.h"
#include "Serialization/Formatter/JsonArchiveInputFormatter.h"

namespace
{
	constexpr int32 SceneVersion = 4;   // 포맷이 바뀌었으므로 올린다
	constexpr int32 SequenceVersion = 2;
	constexpr int32 AnimSequenceVersion = 1;
	constexpr int32 AnimGraphVersion = 1;
	constexpr int32 Blend1DVersion = 1;
}

bool FJsonArchive::SaveWorldToBytes(UWorld* World, TArray<uint8>& OutBytes)
{
	return SaveObjectToBytes(World, "World", SceneVersion, OutBytes);
}

bool FJsonArchive::LoadWorldFromBytes(UWorld* World, const TArray<uint8>& Bytes)
{
	if (!World)
		return false;

	FMemoryReader Reader(Bytes);
	FJsonArchiveInputFormatter Formatter(Reader);   
	if (Reader.IsError())
	{
		HTR_LOG(Error, "Load: invalid scene data");
		return false;                               
	}

	FStructuredArchive Ar(Formatter);
	FStructuredArchive::FRecord Root = Ar.Open().EnterRecord();

	int32 Version = 0;
	Root << SA_VALUE("Version", Version);
	if (Version != SceneVersion)
	{
		HTR_LOG(Error, "Load: scene version {} is not supported (expected {})", Version, SceneVersion);
		return false;                               
	}

	World->ClearWorld();
	World->Serialize(Root.EnterRecord("World"));
	Ar.Close();

	World->GetScene().BuildBVH();
	return !Reader.IsError();
}

bool FJsonArchive::SaveActorsToBytes(const TArray<AActor*>& Actors, TArray<uint8>& OutBytes)
{
	// 씬 저장과 같이 Transient 액터(에디터 전용 등)는 복사하지 않는다.
	TArray<AActor*> SavedActors;
	for (AActor* Actor : Actors)
	{
		if (Actor && !Actor->HasAnyFlags(EObjectFlags::RF_Transient))
			SavedActors.Add(Actor);
	}
	if (SavedActors.IsEmpty())
		return false;

	OutBytes.Reset();
	FMemoryWriter Writer(OutBytes);
	{
		FJsonArchiveOutputFormatter Formatter(Writer);
		FStructuredArchive Ar(Formatter);

		FStructuredArchive::FRecord Root = Ar.Open().EnterRecord();
		int32 Version = SceneVersion;
		Root << SA_VALUE("Version", Version);

		int32 Num = SavedActors.Num();
		FStructuredArchive::FArray ActorArray = Root.EnterArray("Actors", Num);
		for (AActor* Actor : SavedActors)
		{
			ULevel::SaveActorRecord(ActorArray.EnterElement().EnterRecord(), Actor);
		}

		Ar.Close();
	}

	return !Writer.IsError();
}

bool FJsonArchive::SpawnActorsFromBytes(UWorld* World, const TArray<uint8>& Bytes, TArray<AActor*>& OutActors)
{
	OutActors.Reset();
	if (!World || Bytes.IsEmpty())
		return false;

	FMemoryReader Reader(Bytes);
	FJsonArchiveInputFormatter Formatter(Reader);
	if (Reader.IsError())
		return false;

	FStructuredArchive Ar(Formatter);
	FStructuredArchive::FRecord Root = Ar.Open().EnterRecord();

	int32 Version = 0;
	Root << SA_VALUE("Version", Version);
	if (Version != SceneVersion)
		return false;

	int32 Num = 0;
	FStructuredArchive::FArray ActorArray = Root.EnterArray("Actors", Num);
	for (int32 i = 0; i < Num; ++i)
	{
		if (AActor* Actor = World->GetPersistentLevel()->SpawnActorFromRecord(ActorArray.EnterElement().EnterRecord()))
		{
			OutActors.Add(Actor);
		}
	}
	Ar.Close();

	return !Reader.IsError();
}

bool FJsonArchive::SaveWorld(UWorld* World, const FString& Path)
{
	TArray<uint8> Bytes;
	return SaveWorldToBytes(World, Bytes) && WriteFile(Path, Bytes);
}

bool FJsonArchive::LoadWorld(UWorld* World, const FString& Path)
{
	TArray<uint8> Bytes;
	return ReadFile(Path, Bytes) && LoadWorldFromBytes(World, Bytes);
}

bool FJsonArchive::SaveSequence(UMovieSceneSequence* Sequence, const FString& Path)
{
	TArray<uint8> Bytes;
	return SaveSequenceToBytes(Sequence, Bytes) && WriteFile(Path, Bytes);
}

bool FJsonArchive::SaveSequenceToBytes(UMovieSceneSequence* Sequence, TArray<uint8>& OutBytes)
{
	return SaveObjectToBytes(Sequence, "Sequence", SequenceVersion, OutBytes);
}

bool FJsonArchive::LoadSequence(UMovieSceneSequence* Sequence, const FString& Path)
{
	TArray<uint8> Bytes;
	return ReadFile(Path, Bytes) && LoadSequenceFromBytes(Sequence, Bytes);
}

bool FJsonArchive::LoadSequenceFromBytes(UMovieSceneSequence* Sequence, const TArray<uint8>& Bytes)
{
	return LoadObjectFromBytes(Sequence, "Sequence", SequenceVersion, Bytes);
}

bool FJsonArchive::SaveAnimSequence(UAnimSequence* Sequence, const FString& Path)
{
	TArray<uint8> Bytes;
	return SaveAnimSequenceToBytes(Sequence, Bytes) && WriteFile(Path, Bytes);
}

bool FJsonArchive::SaveAnimSequenceToBytes(UAnimSequence* Sequence, TArray<uint8>& OutBytes)
{
	return SaveObjectToBytes(Sequence, "AnimSequence", AnimSequenceVersion, OutBytes);
}

bool FJsonArchive::LoadAnimSequence(UAnimSequence* Sequence, const FString& Path)
{
	TArray<uint8> Bytes;
	return ReadFile(Path, Bytes) && LoadAnimSequenceFromBytes(Sequence, Bytes);
}

bool FJsonArchive::LoadAnimSequenceFromBytes(UAnimSequence* Sequence, const TArray<uint8>& Bytes)
{
	return LoadObjectFromBytes(Sequence, "AnimSequence", AnimSequenceVersion, Bytes);
}

bool FJsonArchive::SaveAnimGraph(UAnimGraph* Graph, const FString& Path)
{
	TArray<uint8> Bytes;
	return SaveAnimGraphToBytes(Graph, Bytes) && WriteFile(Path, Bytes);
}

bool FJsonArchive::SaveAnimGraphToBytes(UAnimGraph* Graph, TArray<uint8>& OutBytes)
{
	return SaveObjectToBytes(Graph, "AnimGraph", AnimGraphVersion, OutBytes);
}

bool FJsonArchive::LoadAnimGraph(UAnimGraph* Graph, const FString& Path)
{
	TArray<uint8> Bytes;
	return ReadFile(Path, Bytes) && LoadAnimGraphFromBytes(Graph, Bytes);
}

bool FJsonArchive::LoadAnimGraphFromBytes(UAnimGraph* Graph, const TArray<uint8>& Bytes)
{
	return LoadObjectFromBytes(Graph, "AnimGraph", AnimGraphVersion, Bytes);
}

bool FJsonArchive::SaveBlend1D(UBlendSpace1D* Blend1D, const FString& Path)
{
	TArray<uint8> Bytes;
	return SaveBlend1DToBytes(Blend1D, Bytes) && WriteFile(Path, Bytes);
}

bool FJsonArchive::SaveBlend1DToBytes(UBlendSpace1D* Blend1D, TArray<uint8>& OutBytes)
{
	return SaveObjectToBytes(Blend1D, "Blend1D", Blend1DVersion, OutBytes);
}

bool FJsonArchive::LoadBlend1DGraph(UBlendSpace1D* Blend1D, const FString& Path)
{
	TArray<uint8> Bytes;
	return ReadFile(Path, Bytes) && LoadBlend1DFromBytes(Blend1D, Bytes);
}

bool FJsonArchive::LoadBlend1DFromBytes(UBlendSpace1D* Blend1D, const TArray<uint8>& Bytes)
{
	return LoadObjectFromBytes(Blend1D, "Blend1D", Blend1DVersion, Bytes);
}

bool FJsonArchive::WriteFile(const FString& Path, const TArray<uint8>& Bytes)
{
	std::ofstream File(Path, std::ios::binary);
	if (!File.is_open())
	{
		return false;
	}
	
	File.write(reinterpret_cast<const char*>(Bytes.GetData()), Bytes.Num());
	File.close();
	
	return !File.fail();
}

bool FJsonArchive::ReadFile(const FString& Path, TArray<uint8>& OutBytes)
{
	std::error_code Error;

	const auto Size = std::filesystem::file_size(Path, Error);
	if (Error || Size > static_cast<uintmax_t>(INT32_MAX))
	{
		HTR_LOG(Warning, "Load: could not read {}", Path);
		return false;
	}

	std::ifstream File(Path, std::ios::binary);
	if (!File.is_open())
	{
		return false;
	}
	
	OutBytes.SetNum(static_cast<int32>(Size));
	File.read(reinterpret_cast<char*>(OutBytes.GetData()), OutBytes.Num());

	return !File.fail();
}

bool FJsonArchive::SaveObjectToBytes(UObject* Object, const char* RecordName, int32 Version, TArray<uint8>& OutBytes)
{
	if (!Object)
	{
		return false;
	}

	OutBytes.Reset();
	FMemoryWriter Writer(OutBytes);
	{
		// Formatter must flush its JSON before checking Writer for errors.
		FJsonArchiveOutputFormatter Formatter(Writer);
		FStructuredArchive Ar(Formatter);

		auto Root = Ar.Open().EnterRecord();
		Root << SA_VALUE("Version", Version);

		Object->Serialize(Root.EnterRecord(RecordName));
		Ar.Close();
	}

	return !Writer.IsError();
}

bool FJsonArchive::LoadObjectFromBytes(UObject* Object, const char* RecordName, int32 ExpectedVersion, const TArray<uint8>& Bytes)
{
	if (!Object)
	{
		return false;
	}

	FMemoryReader Reader(Bytes);
	FJsonArchiveInputFormatter Formatter(Reader);
	if (Reader.IsError())
	{
		HTR_LOG(Error, "Load: invalid {} data", RecordName);
		return false;
	}

	FStructuredArchive Ar(Formatter);

	auto Root = Ar.Open().EnterRecord();
	
	int32 Version = 0;
	Root << SA_VALUE("Version", Version);
	
	if (Reader.IsError() || Version != ExpectedVersion)
	{
		HTR_LOG(Error, "Load: {} version {} is not supported (expected {})", RecordName, Version, ExpectedVersion);
		return false;
	}

	Object->Serialize(Root.EnterRecord(RecordName));
	Ar.Close();

	return !Reader.IsError();
}