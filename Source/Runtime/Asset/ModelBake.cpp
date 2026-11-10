#include "EnginePCH.h"
#include "Asset/ModelBake.h"

#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/Formatter/BinaryArchiveFormatter.h"
#include "Serialization/MathSerialization.h"
#include <fstream>
#include <filesystem>
#include <cstring>

namespace
{
constexpr uint32 ModelBakeVersion = 1;

struct FModelBakeHeader
{
	char Magic[4] = { 'M', 'O', 'D', 'B' }; // 'M''o'del 'D'ata 'B'ake
	uint32 Version = ModelBakeVersion;
};
}

template <typename ElementType, typename ElementSerializer>
void FModelBake::SerializeArray(FStructuredArchive::FRecord& Record, const char* Name, TArray<ElementType>& Array, ElementSerializer Serializer)
{
	int32 Count = Array.Num();

	auto ArrayRecord = Record.EnterArray(Name, Count);
	if (Record.GetUnderlyingArchive().IsLoading())
	{
		Array.SetNum(Count);
	}

	for (int32 i = 0; i < Count; ++i)
	{
		auto ElementRecord = ArrayRecord.EnterElement().EnterRecord();
		Serializer(ElementRecord, Array[i]);
	}
}

bool FModelBake::ReadBaked(const FString& Path, FModelData& OutModelData)
{
	std::ifstream In(Path, std::ios::binary);
	if (!In)
	{
		return false;   // .bin이 없음 (처음 로드)
	}

	int32 FileSize = static_cast<int32>(std::filesystem::file_size(Path));

	TArray<uint8> Bytes;
	Bytes.SetNum(FileSize);
	In.read(reinterpret_cast<char*>(Bytes.GetData()), FileSize);

	FMemoryReader Reader(Bytes);
	if (Reader.IsError())
	{
		return false;
	}

	FModelBakeHeader Header;
	Reader.Serialize(&Header, sizeof(Header));

	const FModelBakeHeader Expected;
	if (std::memcmp(Header.Magic, Expected.Magic, sizeof(Header.Magic)) != 0 || Header.Version != Expected.Version)
	{
		HTR_LOG(Warning, "[BAKE] {}: invalid header or version, re-cooking", Path);
		return false;
	}

	FBinaryArchiveFormatter Formatter(Reader);
	FStructuredArchive Archive(Formatter);

	auto Root = Archive.Open().EnterRecord();
	SerializeModelData(Root, OutModelData);
	Archive.Close();

	if (Reader.IsError())
	{
		HTR_LOG(Warning, "[BAKE] {}: failed to read data, re-cooking", Path);
		return false;
	}

	OutModelData.MeshData.BuildAABB();
	OutModelData.MeshData.BuildTriangleBVH();

	return true;
}

bool FModelBake::WriteBaked(const FString& Path, FModelData& ModelData)
{
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);

	FModelBakeHeader Header;
	Writer.Serialize(&Header, sizeof(Header));

	FBinaryArchiveFormatter Formatter(Writer);
	FStructuredArchive Archive(Formatter);

	auto Root = Archive.Open().EnterRecord();
	SerializeModelData(Root, ModelData);
	Archive.Close();

	if (Writer.IsError())
	{
		HTR_LOG(Warning, "[BAKE] {}: failed to write data", Path);
		return false;
	}

	std::ofstream Out(Path, std::ios::binary);
	if (!Out)
	{
		return false;
	}

	Out.write(reinterpret_cast<const char*>(Bytes.GetData()), Bytes.Num());

	return true;
}

void FModelBake::SerializeModelNode(FStructuredArchive::FRecord& Record, FModelNode& Node)
{
	Record << SA_VALUE("Name", Node.Name);
	Record << SA_VALUE("ParentNodeIndex", Node.ParentNodeIndex);
	Record << SA_VALUE("LocalTransform", Node.LocalTransform);
}

void FModelBake::SerializeModelBone(FStructuredArchive::FRecord& Record, FModelBone& Bone)
{
	Record << SA_VALUE("NodeIndex", Bone.NodeIndex);
	Record << SA_VALUE("ParentBoneIndex", Bone.ParentBoneIndex);
	Record << SA_VALUE("InverseBindTransform", Bone.InverseBindTransform);
}

void FModelBake::SerializeModelMesh(FStructuredArchive::FRecord& Record, FModelMesh& Mesh)
{
	Record << SA_VALUE("NodeIndex", Mesh.NodeIndex);
	Record << SA_VALUE("FirstSection", Mesh.FirstSection);
	Record << SA_VALUE("SectionCount", Mesh.SectionCount);
}

void FModelBake::SerializeModelVertexInfluences(FStructuredArchive::FRecord& Record, FModelVertexInfluences& Influences)
{
	FArchive& Ar = Record.GetUnderlyingArchive();
	Record << SA_VALUE("InfluenceCount", Influences.InfluenceCount);

	if (Influences.InfluenceCount < 0 || Influences.InfluenceCount > MaxBoneInfluenceCount)
	{
		return;
	}

	int32 IndexCount = Influences.InfluenceCount;
	auto IndicesArrayRecord = Record.EnterArray("BoneIndices", IndexCount);
	if (IndexCount != Influences.InfluenceCount)
	{
		return;
	}

	for (int32 i = 0; i < IndexCount; ++i)
	{
		auto ElementRecord = IndicesArrayRecord.EnterElement().EnterRecord();
		ElementRecord << SA_VALUE("BoneIndex", Influences.BoneIndices[i]);
	}

	int32 WeightCount = Influences.InfluenceCount;
	auto WeightsArrayRecord = Record.EnterArray("Weights", WeightCount);
	if (WeightCount != Influences.InfluenceCount)
	{
		return;
	}

	for (int32 i = 0; i < WeightCount; ++i)
	{
		auto ElementRecord = WeightsArrayRecord.EnterElement().EnterRecord();
		ElementRecord << SA_VALUE("Weight", Influences.Weights[i]);
	}
}

void FModelBake::SerializeVertex(FStructuredArchive::FRecord& Record, FVertexPNCT& Vertex)
{
	Record << SA_VALUE("Position", Vertex.Position);
	Record << SA_VALUE("Normal", Vertex.Normal);
	Record << SA_VALUE("Tangent", Vertex.Tangent);
	Record << SA_VALUE("Color", Vertex.Color);
	Record << SA_VALUE("UV", Vertex.UV);
}

void FModelBake::SerializeMeshSection(FStructuredArchive::FRecord& Record, FMeshSection& Section)
{
	Record << SA_VALUE("StartIndex", Section.StartIndex);
	Record << SA_VALUE("IndexCount", Section.IndexCount);
	Record << SA_VALUE("MaterialSlotIndex", Section.MaterialSlotIndex);
}

void FModelBake::SerializeMaterialSlot(FStructuredArchive::FRecord& Record, FMeshMaterialSlot& Slot)
{
	Record << SA_VALUE("Name", Slot.Name);
	Record << SA_VALUE("BaseColor", Slot.BaseColor);
	Record << SA_VALUE("DiffuseTexturePath", Slot.DiffuseTexturePath);
	Record << SA_VALUE("NormalTexturePath", Slot.NormalTexturePath);
	Record << SA_VALUE("Shininess", Slot.Shininess);
	Record << SA_VALUE("SpecularColor", Slot.SpecularColor);
}

void FModelBake::SerializeBoneAnimationTrack(FStructuredArchive::FRecord& Record, FBoneAnimationTrack& Track)
{
	Record << SA_VALUE("BoneIndex", Track.BoneIndex);
	Record << SA_VALUE("KeyCount", Track.KeyCount);
	SerializeArray(Record, "PosKeys", Track.PosKeys, [](FStructuredArchive::FRecord& Record, FVector& Value) { Record << SA_VALUE("Position", Value); });
	SerializeArray(Record, "RotationKeys", Track.RotKeys, [](FStructuredArchive::FRecord& Record, FQuat& Value) { Record << SA_VALUE("Rotation", Value); });
	SerializeArray(Record, "ScaleKeys", Track.ScaleKeys, [](FStructuredArchive::FRecord& Record, FVector& Value) { Record << SA_VALUE("Scale", Value); });
}

void FModelBake::SerializeBoneAnimation(FStructuredArchive::FRecord& Record, FModelBoneAnimation& Animation)
{
	Record << SA_VALUE("Name", Animation.Name);
	Record << SA_VALUE("Duration", Animation.Duration);
	Record << SA_VALUE("SampleRate", Animation.SampleRate);
	SerializeArray(Record, "Tracks", Animation.Tracks, SerializeBoneAnimationTrack);
}

void FModelBake::SerializeModelData(FStructuredArchive::FRecord& Record, FModelData& ModelData)
{
	Record << SA_VALUE("IsSkeletal", ModelData.IsSkeletal);

	SerializeArray(Record, "Vertices", ModelData.MeshData.Vertices, SerializeVertex);
	SerializeArray(Record, "Indices", ModelData.MeshData.Indices, [](FStructuredArchive::FRecord& Record, uint32& Value) { Record << SA_VALUE("Index", Value); });
	SerializeArray(Record, "Sections", ModelData.MeshData.Sections, SerializeMeshSection);
	SerializeArray(Record, "MaterialSlots", ModelData.MeshData.MaterialSlots, SerializeMaterialSlot);
	SerializeArray(Record, "Nodes", ModelData.Nodes, SerializeModelNode);
	SerializeArray(Record, "Bones", ModelData.Bones, SerializeModelBone);
	SerializeArray(Record, "Meshs", ModelData.Meshs, SerializeModelMesh);
	SerializeArray(Record, "VertexInfluencesList", ModelData.VertexInfluencesList, SerializeModelVertexInfluences);
	SerializeArray(Record, "Animations", ModelData.Animations, SerializeBoneAnimation);
}
