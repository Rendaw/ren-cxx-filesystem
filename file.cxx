#include "file.h"

#include <cstring>

#include "../ren-cxx-basics/error.h"

ReadBufferT::ReadBufferT(size_t Size) : 
	Data(std::make_unique<uint8_t[]>(Size)),
	Start(0), 
	Stop(0), 
	Total(Size)
{
}

size_t ReadBufferT::Available(void) const
{
	return Total - Stop;
}
	
void ReadBufferT::Ensure(size_t NeededSize)
{
	if (Available() < NeededSize)
		Expand(NeededSize);
}

void ReadBufferT::Expand(size_t AddSize)
{
	auto const OldStart = Start;
	auto Potential = Start + (Total - Stop);
	if (AddSize > Potential)
	{
		auto Difference = Potential - AddSize;
		auto Replacement = std::make_unique<uint8_t[]>(Total + Difference * 2); // or something
		memcpy(&Replacement[0], &Data[Start], Filled());
		Data = std::move(Replacement);
	}
	else
	{
		memmove(&Data[0], &Data[Start], Filled());
	}
	Start = 0;
	Stop -= OldStart;
}

uint8_t *ReadBufferT::EmptyStart(void)
{ 
	return &Data[Stop]; 
}

uint8_t const *ReadBufferT::EmptyStart(void) const
{ 
	return &Data[Stop]; 
}

void ReadBufferT::Fill(size_t FillSize)
{ 
	Stop += FillSize; 
	AssertLTE(Stop, Total); 
}
	
size_t ReadBufferT::Filled(void) const
{ 
	return Stop - Start; 
}

uint8_t *ReadBufferT::FilledStart(void)
{ 
	return &Data[Start]; 
}

uint8_t const *ReadBufferT::FilledStart(void) const
{ 
	return &Data[Start]; 
}

uint8_t *ReadBufferT::FilledStart(size_t RequiredSize, size_t Offset)
{
	auto OutStart = Start + Offset;
	if (Stop - OutStart < RequiredSize) return nullptr;
	return &Data[OutStart];
}

uint8_t const *ReadBufferT::FilledStart(size_t RequiredSize, size_t Offset) const
{
	auto OutStart = Start + Offset;
	if (Stop - OutStart < RequiredSize) return nullptr;
	return &Data[OutStart];
}

void ReadBufferT::Consume(size_t Size)
{ 
	Start += Size; 
	AssertLTE(Start, Stop); 
}

namespace Filesystem
{

FileT FileT::OpenRead(std::string const &Path) { return FileT(Path, fopen_read(Path)); }
FileT FileT::OpenWrite(std::string const &Path) { return FileT(Path, fopen_write(Path)); }
FileT FileT::OpenAppend(std::string const &Path) { return FileT(Path, fopen_append(Path)); }
FileT FileT::OpenModify(std::string const &Path) { return FileT(Path, fopen_modify(Path)); }

FileT::FileT(void) : Core(nullptr) {}

FileT::FileT(FileT &&Other) : Core(Other.Core) 
{ 
	Other.Core = nullptr; 
}

FileT &FileT::operator =(FileT &&Other) 
{ 
	Core = Other.Core; 
	Other.Core = nullptr; 
	return *this; 
}

FileT::operator bool(void) const
{
	if (!Core) return false;
	if (ferror(Core)) return false;
	if (feof(Core)) return false;
	return true;
}

FileT &FileT::Write(std::vector<uint8_t> const &Data)
{
	Assert(Core);
	fwrite(&Data[0], Data.size(), 1, Core); // TODO handle errors
	return *this;
}
	
FileT &FileT::Write(std::string const &Data)
{
	Assert(Core);
	fwrite(Data.c_str(), Data.size(), 1, Core); // TODO handle errors
	return *this;
}

FileT &FileT::Read(std::vector<uint8_t> &Buffer)
{
	Assert(Core);
	if (Buffer.empty()) Buffer.resize(4096);
	fread(&Buffer[0], 1, Buffer.size(), Core); // TODO handle errors
	return *this;
}
	
std::vector<uint8_t> FileT::ReadAll(void)
{
	ReadBufferT Buffer;
	while (*this) Read(Buffer);
	return std::vector<uint8_t>(Buffer.FilledStart(), Buffer.FilledStart() + Buffer.Filled());
}

FileT &FileT::Seek(size_t Offset) 
{ 
	Assert(Core);
	fseek(Core, Offset, SEEK_SET); 
	return *this;
}

FileT::~FileT(void) 
{ 
	if (Core) fclose(Core); 
}

FileT::FileT(std::string const &Path, FILE *Core) : Core(Core) 
{
	if (!Core)
		throw ConstructionErrorT() << "Unable to open file [" << Path << "]";
}

}

