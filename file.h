#ifndef ren_cxx_filesystem__file_h
#define ren_cxx_filesystem__file_h

#include <cstdio>
#include <string>
#include <vector>

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

namespace Filesystem
{

struct File
{
	static File OpenRead(std::string const &Path);
	static File OpenWrite(std::string const &Path);
	static File OpenAppend(std::string const &Path);
	static File OpenModify(std::string const &Path);

	operator bool(void) const;
	void Write(std::vector<uint8_t> const &Data);
	void Read(std::vector<uint8_t> &Data);
	void Seek(size_t Offset);

	~File(void);

	private:
		File(FILE *Core);
		FILE *Core;
};

}

#endif

