#ifndef filesystem_h
#define filesystem_h

#include "../ren-cxx-basics/type.h"
#include "filesystem_string.h"

namespace Filesystem
{

#ifdef __WIN32
inline FILE *fopen_read(std::string const &Filename)
	{ return _wfopen(&ToNativeString(Filename)[0], L"r"); }
inline FILE *fopen_write(std::string const &Filename)
	{ return _wfopen(&ToNativeString(Filename)[0], L"w"); }
#else
inline FILE *fopen_read(std::string const &Filename)
	{ return ::fopen(Filename.c_str(), "r"); }
inline FILE *fopen_write(std::string const &Filename)
	{ return ::fopen(Filename.c_str(), "w"); }
#endif

struct PathSettingsT
{
	OptionalT<std::string> const WindowsDrive;
	std::string const Separator;
};

struct PathT;
struct PathElementT
{
	PathElementT(PathSettingsT const &Settings);
	~PathElementT(void);
	
	std::string Render(void) const;
	std::string const &Filename(void) const;
	std::string Directory(void) const;
	OptionalT<std::string> Extension(void) const;

	size_t Depth(void) const;

	bool Contains(PathElementT const *Other) const;
	
	PathT Enter(std::string const &Value) const;
	PathT EnterRaw(std::string const &Raw) const;
	PathT Exit(void) const;

	bool Exists(void) const;
	bool FileExists(void) const;
	bool DirectoryExists(void) const;

	bool List(std::function<bool(PathT &&Path, bool IsFile, bool IsDir)> const &Callback) const;

	bool Delete(void) const;
	bool DeleteDirectory(void) const;
	bool CreateDirectory(void) const;

	bool GoTo(void) const;

	private:
		friend struct PathT;
		std::string const Value;
		mutable size_t Count = 0;
		VariantT<PathElementT const *, PathSettingsT *> const Parent;

		PathElementT(PathElementT const *Parent, std::string const &Value);
};

struct PathT
{
	static PathT Absolute(std::string const &Raw);
	static PathT Here(void);
	static PathT Qualify(std::string const &Raw);
	static PathT Temp(bool File = true, OptionalT<PathT> const &Base = {});

	PathT(PathElementT const *Element);
	PathT(PathSettingsT const &Settings);
	PathT(void);
	PathT(PathT const &Other);
	~PathT(void);

	PathT &operator =(PathT const &Other);

	void Set(PathElementT const *Element);
	void Clear(void);

	PathElementT const *operator ->(void) const;
	operator PathElementT const *(void) const;

	private:
		PathElementT const *Element;
};

}

inline std::ostream &operator <<(std::ostream &Stream, Filesystem::PathT const &Value)
	{ return Stream << Value->Render(); }

#endif

