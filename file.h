#ifndef ren_cxx_filesystem__file_h
#define ren_cxx_filesystem__file_h

#include <cstdio>
#include <string>
#include <vector>
#include <memory>

#include "../ren-cxx-basics/extrastandard.h"

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
	void Expand(size_t AddSize);
	uint8_t *EmptyStart(void);
	uint8_t const *EmptyStart(void) const;
	void Fill(size_t);

	// Draining
	size_t Filled(void) const;
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
	FileT &Write(std::vector<uint8_t> const &Data);
	FileT &Read(std::vector<uint8_t> &Buffer);
	template <typename BufferT> FileT &Read(BufferT &Buffer)
	{
		// BufferT must provide the methods in ReadBufferT above
		Assert(Core);
		if (Buffer.Available() < 4096)
			Buffer.Expand(4096);
		auto ReadSize = fread(Buffer.EmptyStart(), 1, Buffer.Available(), Core); // TODO handle errors
		Buffer.Fill(ReadSize);
		return *this;
	}
	FileT &Seek(size_t Offset);

	~FileT(void);

	private:
		FileT(FILE *Core);
		FILE *Core;
};

}

#endif

