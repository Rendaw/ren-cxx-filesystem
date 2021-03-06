#ifndef ren_cxx_filesystem__file_h
#define ren_cxx_filesystem__file_h

#include <cstdio>
#include <string>
#include <vector>
#include <memory>

#include "../ren-cxx-basics/extrastandard.h"
#include "../ren-cxx-basics/error.h"

#ifdef __WIN32
inline FILE *fopen_read(std::string const &Filename)
	{ return _wfopen(&ToNativeString(Filename)[0], L"r"); }
inline FILE *fopen_write(std::string const &Filename)
	{ return _wfopen(&ToNativeString(Filename)[0], L"w"); }
inline FILE *fopen_modify(std::string const &Filename)
	{ return _wfopen(&ToNativeString(Filename)[0], L"r+"); }
inline FILE *fopen_append(std::string const &Filename)
	{ return _wfopen(&ToNativeString(Filename)[0], L"a"); }
#else
inline FILE *fopen_read(std::string const &Filename)
	{ return ::fopen(Filename.c_str(), "r"); }
inline FILE *fopen_write(std::string const &Filename)
	{ return ::fopen(Filename.c_str(), "w"); }
inline FILE *fopen_modify(std::string const &Filename)
	{ return ::fopen(Filename.c_str(), "r+"); }
inline FILE *fopen_append(std::string const &Filename)
	{ return ::fopen(Filename.c_str(), "a"); }
#endif

struct ReadBufferT
{
	ReadBufferT(size_t Size = 4096);

	// Filling
	size_t Available(void) const;
	void Ensure(size_t NeededSize);
	void Expand(size_t AddSize);
	uint8_t *EmptyStart(void);
	uint8_t const *EmptyStart(void) const;
	void Fill(size_t);

	// Draining
	size_t Filled(void) const;
	uint8_t *FilledStart(void);
	uint8_t const *FilledStart(void) const;
	uint8_t *FilledStart(size_t RequiredSize, size_t Offset = 0);
	uint8_t const *FilledStart(size_t RequiredSize, size_t Offset = 0) const;
	void Consume(size_t ReduceSize);

	private:
		std::unique_ptr<uint8_t[]> Data;
		size_t Start, Stop, Total;
};

namespace Filesystem
{

struct FileT
{
	static FileT OpenRead(std::string const &Path);
	static FileT OpenWrite(std::string const &Path);
	static FileT OpenAppend(std::string const &Path);
	static FileT OpenModify(std::string const &Path);
	
	FileT(void);
	FileT(FileT &&Other);
	FileT(FileT const &Other) = delete;
	FileT &operator =(FileT &&Other);
	FileT &operator =(FileT const &Other) = delete;

	operator bool(void) const;
	void Write(std::vector<uint8_t> const &Data);
	void Write(std::string const &Data);
	bool Read(std::vector<uint8_t> &Buffer);
	template <typename BufferT> bool Read(BufferT &Buffer)
	{
		// BufferT must provide the methods in ReadBufferT above
		Assert(Core);
		if (!*this) return false;
		if (Buffer.Available() < 4096)
			Buffer.Expand(4096);
		auto ReadSize = fread(Buffer.EmptyStart(), 1, Buffer.Available(), Core);
		if ((ReadSize == 0) && ferror(Core)) 
			throw SYSTEM_ERROR << "Error reading from [" << Path << "]: " << strerror(errno);
		Buffer.Fill(ReadSize);
		return true;
	}
	std::vector<uint8_t> ReadAll(void);
	FileT &Seek(size_t Offset);
	size_t Tell(void) const;

	~FileT(void);

	private:
		FileT(std::string const &File, FILE *Core);
		std::string Path;
		FILE *Core;
};

}

#endif

