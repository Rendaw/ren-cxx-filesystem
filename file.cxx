#include "file.h"

#include <cstring>

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

FileT::FileT(FileT &&Other) : Path(std::move(Other.Path)), Core(Other.Core) 
{ 
	Other.Core = nullptr; 
}

FileT &FileT::operator =(FileT &&Other) 
{ 
	Path = std::move(Other.Path);
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

void FileT::Write(std::vector<uint8_t> const &Data)
{
	Assert(Core);
	auto Result = fwrite(&Data[0], Data.size(), 1, Core);
	if ((Result == 0) && ferror(Core)) 
		throw SYSTEM_ERROR << "Error writing to [" << Path << "]: " << strerror(errno);
}
	
void FileT::Write(std::string const &Data)
{
	Assert(Core);
	auto Result = fwrite(Data.c_str(), Data.size(), 1, Core);
	if ((Result == 0) && ferror(Core)) 
		throw SYSTEM_ERROR << "Error writing to [" << Path << "]: " << strerror(errno);
}

bool FileT::Read(std::vector<uint8_t> &Buffer)
{
	Assert(Core);
	if (!*this) return false;
	if (Buffer.empty()) Buffer.resize(4096);
	auto Result = fread(&Buffer[0], 1, Buffer.size(), Core); // TODO handle errors
	if ((Result == 0) && ferror(Core)) 
		throw SYSTEM_ERROR << "Error reading from [" << Path << "]: " << strerror(errno);
	Buffer.resize(Result);
	return true;
}
	
std::vector<uint8_t> FileT::ReadAll(void)
{
	ReadBufferT Buffer;
	while (Read(Buffer)) {}
	return std::vector<uint8_t>(Buffer.FilledStart(), Buffer.FilledStart() + Buffer.Filled());
}

FileT &FileT::Seek(size_t Offset) 
{ 
	Assert(Core);
	fseek(Core, Offset, SEEK_SET); 
	return *this;
}

size_t FileT::Tell(void) const
{
	Assert(Core);
	return ftell(Core);
}

FileT::~FileT(void) 
{ 
	if (Core) fclose(Core); 
}

FileT::FileT(std::string const &Path, FILE *Core) : Path(Path), Core(Core) 
{
	if (!Core)
		throw CONSTRUCTION_ERROR << "Unable to open file [" << Path << "]";
}

}

