#include "filesystem.h"

#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#include <direct.h>
#include <shlobj.h>

struct SeedRandomT
{
	SeedRandomT(void)
	{
		srand(time(NULL));
	}
} static SeedRandom;

#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif

namespace Filesystem
{

PathElementT::PathElementT(PathSettingsT const &Settings) : Parent(new PathSettingsT(Settings)) { }

PathElementT::~PathElementT(void)
{
	if (Parent.Is<PathElementT const *>())
	{
		auto Element = Parent.Get<PathElementT const *>();
		Element->Count -= 1;
		if (Element->Count == 0) delete Element;
	}
	else
	{
		Assert(Parent.Is<PathSettingsT *>());
		delete Parent.Get<PathSettingsT *>();
	}
}

std::string PathElementT::Render(void) const
{
	PathSettingsT *Root = nullptr;
	std::list<PathElementT const *> Parts;
	{
		VariantT <PathElementT const *, PathSettingsT *> Part(this);
		while (Part.Is<PathElementT const *>())
		{
			Parts.push_front(Part.Get<PathElementT const *>());
			Part = Part.Get<PathElementT const *>()->Parent;
		}
		Root = Part.Get<PathSettingsT *>();
	}
	std::stringstream Out;
	if (Root->WindowsDrive) Out << *Root->WindowsDrive;
	if (Parts.size() == 1)
	{
		Out << Root->Separator;
	}
	else
	{
		if (!Parts.empty()) Parts.pop_front();
		for (auto &Part : Parts) Out << Root->Separator << Part->Value;
	}
	return Out.str();
}

std::string const &PathElementT::Filename(void) const { return Value; }

std::string PathElementT::Directory(void) const { return Parent.Is<PathElementT const *>() ? Parent.Get<PathElementT const *>()->Render() : Render(); }

OptionalT<std::string> PathElementT::Extension(void) const
{
	std::smatch Matches;
	std::regex_search(Value, Matches, std::regex("\\.([^.]+)$"));
	for (auto &Match : Matches) return Match.str();
	return {};
}

size_t PathElementT::Depth(void) const
{
	size_t Count = 0;
	VariantT <PathElementT const *, PathSettingsT *> Part(this);
	while (Part.Is<PathElementT const *>())
	{
		++Count;
		Part = Part.Get<PathElementT const *>()->Parent;
	}
	return Count - 1;
}

bool PathElementT::Contains(PathElementT const *Other) const
{
	PathElementT const *Candidates[2]{this, Other};
	PathSettingsT *PathSettings[2];
	std::vector<PathElementT const *> Parts[2];
	for (size_t Candidate = 0; Candidate < 2; ++Candidate)
	{
		VariantT <PathElementT const *, PathSettingsT *> Part(Candidates[Candidate]);
		while (Part.Is<PathElementT const *>())
		{
			Parts[Candidate].push_back(Part.Get<PathElementT const *>());
			Part = Part.Get<PathElementT const *>()->Parent;
		}
		std::reverse(Parts[Candidate].begin(), Parts[Candidate].end());
		PathSettings[Candidate] = Part.Get<PathSettingsT *>();
	}
	if (PathSettings[0]->WindowsDrive != PathSettings[1]->WindowsDrive) return false;
	if (Parts[1].size() < Parts[0].size()) return false;
	for (size_t Part = 0; Part < Parts[0].size(); ++Part)
		if (Parts[0][Part]->Value != Parts[1][Part]->Value) return false;
	return true;
}

PathT PathElementT::Enter(std::string const &Value) const
{
	for (size_t pos = 0; pos < Value.size(); pos++) AssertNE(Value[pos], 0);
	return PathT(new PathElementT(this, Value));
}

std::regex ElementRegex("([^/\\\\]*)[/\\\\]+");
PathT PathElementT::EnterRaw(std::string const &Raw) const
{
	std::string Text = Raw;
	PathT Out(this);
	auto HandleMatch = [&](auto const &Match)
	{
		if (Match.length() == 0) {}
		else if (Match == ".") {}
		else if (Match == "..") Out = Out->Exit();
		else Out = Out->Enter(Match);
	};
	std::smatch Matches;
	while (std::regex_search(Text, Matches, ElementRegex))
	{
		HandleMatch(Matches[1]);
		Text = Matches.suffix().str();
	}
	HandleMatch(Text);
	return Out;
}

PathT PathElementT::Exit(void) const
{
	if (Parent.Is<PathSettingsT *>()) throw ConstructionErrorT() << "Cannot exit root path.";
	return {Parent.Get<PathElementT const *>()};
}

bool PathElementT::Exists(void) const
{
#ifdef _WIN32
	return GetFileAttributesW(&ToNativeString("\\\\?\\" + Render())[0]) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat StatResultBuffer;
	return stat(Render().c_str(), &StatResultBuffer) == 0;
#endif
}

bool PathElementT::FileExists(void) const
{
#ifdef _WIN32
	auto Attributes = GetFileAttributesW(&ToNativeString("\\\\?\\" + Render())[0]); // Doesn't work for some reason -- mixed slashes?
	//auto Attributes = GetFileAttributesW(&ToNativeString(Render())[0]);
	if (Attributes == 0xFFFFFFFF) return false;
	if (Attributes == 0x10) return false;
	return true;
#else
	struct stat StatResultBuffer;
	int Result = stat(Render().c_str(), &StatResultBuffer);
	if (Result != 0) return false;
	return S_ISREG(StatResultBuffer.st_mode);
#endif
}

bool PathElementT::DirectoryExists(void) const
{
#ifdef _WIN32
        return GetFileAttributesW(&ToNativeString("\\\\?\\" + Render())[0]) & 0x10;
        //return GetFileAttributesW(&ToNativeString(Render())[0]) & 0x10;
#else
        struct stat StatResultBuffer;
        int Result = stat(Render().c_str(), &StatResultBuffer);
        if (Result != 0) return false;
        return S_ISDIR(StatResultBuffer.st_mode);
#endif
}

static bool ProcessDirectoryContents(PathT const &DirectoryName, std::function<void(std::string const &Element, bool IsFile, bool IsDir)> const &Process)
{
#ifdef _WIN32
        WIN32_FIND_DATAW ElementInfo;
        auto DirectoryResource = FindFirstFileW(
		&ToNativeString(DirectoryName->Render() + "\\*")[0],
		&ElementInfo);
        if (DirectoryResource == INVALID_HANDLE_VALUE) return false;

        do
        {
                auto FindName = FromNativeString(ElementInfo.cFileName, wcslen(ElementInfo.cFileName));
		if (FindName == ".") continue;
		if (FindName == "..") continue;
                Process(
			FindName,
			ElementInfo.dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY,
			!(ElementInfo.dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY));
        } while (FindNextFileW(DirectoryResource, &ElementInfo) != 0);

        FindClose(DirectoryResource);
#else
        auto DirectoryResource = opendir(DirectoryName->Render().c_str());
        if (DirectoryResource == nullptr) return false;

        dirent *ElementInfo;
        while ((ElementInfo = readdir(DirectoryResource)) != nullptr)
        {
		std::string ElementName(ElementInfo->d_name);
                if ((ElementName == ".") || (ElementName == "..")) continue;
                Process(
			ElementName,
			ElementInfo->d_type != DT_DIR, 
			ElementInfo->d_type == DT_DIR);
        }

        closedir(DirectoryResource);
#endif
	return true;
}

bool PathElementT::List(std::function<bool(PathT &&Path, bool IsFile, bool IsDir)> const &Callback) const
{
        return ProcessDirectoryContents(PathT(this), [&](std::string const &Element, bool IsFile, bool IsDir)
	{
		Callback(Enter(Element), IsFile, IsDir);
	});
}

bool PathElementT::Delete(void) const
{
#ifdef _WIN32
	return _wunlink(&ToNativeString(Render())[0]) == 0;
#else
	return unlink(Render().c_str()) == 0;
#endif
}

bool PathElementT::DeleteDirectory(void) const
{
	bool Failed = false;
	std::list<std::pair<PathT, bool>> Directories{{PathT(this), false}};
	while (!Directories.empty())
	{
		if (!Directories.back().second)
		{
			Directories.back().second = true;
			Directories.back().first->List([&](PathT &&Path, bool IsFile, bool IsDir)
			{
				if (!IsFile && !IsDir) return false; // Can't delete special files, probably
				if (IsFile) Failed = !Path->Delete();
				if (Failed) return false;
				if (IsDir) Directories.push_back({std::move(Path), false});
				return true;
			});
			if (Failed) return false;
		}
		else
		{
#ifdef _WIN32
			if (RemoveDirectoryW(&ToNativeString(Directories.back().first->Render())[0]) == 0)
				return false;
#else
			if (rmdir(Directories.back().first->Render().c_str()) != 0)
				if (errno != ENOENT) return false;
#endif
			Directories.pop_back();
		}
	}
	return true;
}

bool PathElementT::CreateDirectory(void) const
{
	std::list<PathElementT const *> Parts;
	{
		VariantT <PathElementT const *, PathSettingsT *> Part(this);
		while (Part.Is<PathElementT const *>())
		{
			Parts.push_front(Part.Get<PathElementT const *>());
			Part = Part.Get<PathElementT const *>()->Parent;
		}
	}
	Parts.pop_front();
	for (auto &Part : Parts)
	{
#ifdef _WIN32
		auto Result = CreateDirectoryW(&ToNativeString("\\\\?\\" + Part->Render())[0], nullptr);
		if ((Result == 0) && (GetLastError() != ERROR_ALREADY_EXISTS)) 
			return false;
#else
		auto Result = mkdir(Part->Render().c_str(), 0777);
		if (Result == -1 && errno != EEXIST)
			return false;
#endif
	}
	return true;
}

bool PathElementT::GoTo(void) const
{
#ifdef _WIN32
	return SetCurrentDirectoryW(&ToNativeString(Render())[0]);
#else
	return chdir(Render().c_str()) == 0;
#endif
}

PathElementT::PathElementT(PathElementT const *Parent, std::string const &Value) : Value(Value), Parent(Parent)
{
	Assert(Parent);
	Assert(this->Parent.Is<PathElementT const *>());
	++Parent->Count;
}

static std::regex const DriveRegex("^([a-zA-Z]:)(.*)$");
PathT PathT::Absolute(std::string const &Raw)
{
#ifdef _WIN32
	std::smatch DriveMatch;
	if (!Assert(std::regex_search(Raw, DriveMatch, DriveRegex))) throw ConstructionErrorT() << "Windows absolute paths must contain drive.  This path is invalid: " << Raw;
	return PathT(PathSettingsT{DriveMatch[1].str(), "\\"})->EnterRaw(DriveMatch[2]);
#else
	return PathT(PathSettingsT{{}, std::string(1, Raw[0])})->EnterRaw(Raw);
#endif
}

PathT PathT::Here(void)
{
#ifdef _WIN32
	std::vector<wchar_t> Buffer(GetCurrentDirectoryW(0, nullptr));
	if (!GetCurrentDirectoryW(Buffer.size(), &Buffer[0]))
		throw ConstructionErrorT() << "Couldn't obtain working directory!";
	return Absolute(FromNativeString(&Buffer[0], Buffer.size() - 1));
#else
	std::vector<char> Buffer(FILENAME_MAX);
	if (!getcwd(&Buffer[0], Buffer.size())) throw ConstructionErrorT() << "Couldn't obtain working directory!";
	return Absolute(std::string(&Buffer[0]));
#endif
}

PathT PathT::Qualify(std::string const &Raw)
{
	if (Raw.empty()) return Here();
	if (Raw[0] == '/') return Absolute(Raw);
	if (Raw[0] == '\\') return Absolute(Raw);
	std::smatch DriveMatch;
	if (std::regex_search(Raw, DriveMatch, DriveRegex)) return Absolute(Raw);
	return Here()->EnterRaw(Raw);
}

PathT PathT::Temp(bool File, OptionalT<PathT> const &Base)
{
#ifdef _WIN32
	std::vector<wchar_t> BaseString;
	if (Base)
	{
		BaseString = ToNativeString((*Base)->Render());
		BaseString.pop_back();
	}
	else
	{
		// Get temp dir
		BaseString.resize(MAX_PATH);
		auto Length = GetTempPathW(BaseString.size(), &BaseString[0]);
		if (Length <= 0) throw ConstructionErrorT() << "Could not find temporary file directory.";
		BaseString.resize(Length);
	}

	if (File)
	{
		std::vector<wchar_t> TemporaryFilename;
		TemporaryFilename.resize(MAX_PATH);
		auto Length = GetTempFileNameW(&BaseString[0], L"zar", 0, &TemporaryFilename[0]);
		if (Length == 0) throw ConstructionErrorT() << "Could not determine a temporary filename.";
		Length = GetLongPathNameW(&TemporaryFilename[0], &TemporaryFilename[0], TemporaryFilename.size());
		AssertGTE(Length, 0);
		if ((unsigned int)Length > TemporaryFilename.size())
		{
			TemporaryFilename.resize(Length);
			Length = GetLongPathNameW(&TemporaryFilename[0], &TemporaryFilename[0], TemporaryFilename.size());
			AssertLTE((unsigned int)Length, TemporaryFilename.size());
		}
		else TemporaryFilename.resize(Length);
		if (Length == 0) throw ConstructionErrorT() << "Could not qualify the temporary filename.";
		return Absolute(FromNativeString(TemporaryFilename));
	}
	else
	{
		auto const Start = BaseString.size();
		BaseString.resize(BaseString.size() + 9);
		BaseString[Start + 8] = 0;
		for (size_t Attempt = 0; Attempt < 3; ++Attempt)
		{
			for (size_t Offset = 0; Offset < 8; ++Offset)
			{
				static wchar_t const Characters[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
				auto Index = rand() % (sizeof(Characters) / sizeof(wchar_t) - 1);
				BaseString[Start + Offset] = Characters[Index];
			}
			auto Result = _wmkdir(&BaseString[0]);
			if (Result == -1)
				std::cout << "Failed to create temporary directory (" << strerror(errno) << ")." << std::endl;
			else return Absolute(FromNativeString(&BaseString[0], BaseString.size() - 1));
		}
		throw ConstructionErrorT() << "Failed to create temporary directory 3 times (" << strerror(errno) << ").";
	}
#else
	std::vector<char> BaseString;
	if (Base)
	{
		auto Native = (*Base)->Render();
		BaseString.insert(BaseString.end(), Native.begin(), Native.end());
	}
	else
	{
		char const *Buffer = getenv("TMPDIR");
		if (Buffer == nullptr) Buffer = getenv("P_tmpdir");
		if (Buffer == nullptr) Buffer = "/tmp";
		BaseString.insert(BaseString.end(), Buffer, Buffer + strlen(Buffer));
	}

	// Create temp file
	static char const *Template = "/XXXXXX";
	BaseString.insert(BaseString.end(), Template, Template + sizeof(Template));
	AssertE(BaseString.back(), 0);
	if (File)
	{
		auto Result = mkstemp(&BaseString[0]);
		if (Result < 0) throw ConstructionErrorT() << "Failed to create temporary file with template " << std::string(&BaseString[0], BaseString.size()) << ".";
		close(Result);
		return Absolute(std::string(&BaseString[0], BaseString.size() - 1));
	}
	else
	{
		auto Result = mkdtemp(&BaseString[0]);
		if (Result <= 0) throw ConstructionErrorT() << "Failed to create temporary directory with template " << std::string(&BaseString[0], BaseString.size()) << ".";
		return Absolute(Result);
	}
#endif
}

PathT::PathT(PathElementT const *Element) { Set(Element); }

PathT::PathT(PathSettingsT const &Settings) { Set(new PathElementT(Settings)); }

PathT::PathT(void) { Set(new PathElementT(PathSettingsT{{}, "/"})); }

PathT::PathT(PathT const &Other) { Set(Other.Element); }

void PathT::Set(PathElementT const *Element)
{
	this->Element = Element;
	Element->Count += 1;
}

PathT::~PathT(void)
{
	Element->Count -= 1;
	if (Element->Count == 0) delete Element;
}

PathT &PathT::operator =(PathT const &Other) { Set(Other.Element); return *this; }

PathElementT const *PathT::operator ->(void) const { return Element; }

PathT::operator PathElementT const *(void) const { return Element; }

}

