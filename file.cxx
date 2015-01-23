#include "file.h"

namespace Filesystem
{

File File::OpenRead(std::string const &Path) { return File(fopen_read(Path)); }
File File::OpenWrite(std::string const &Path) { return File(fopen_write(Path)); }
File File::OpenAppend(std::string const &Path) { return File(fopen_append(Path)); }
File File::OpenModify(std::string const &Path) { return File(fopen_modify(Path)); }

File::operator bool(void) const
{
	if (!Core) return false;
	if (ferror(Core)) return false;
	if (feof(Core)) return false;
	return true;
}

void File::Write(std::vector<uint8_t> const &Data)
{
	fwrite(&Data[0], Data.size(), 1, Core); // TODO handle errors
}

void File::Read(std::vector<uint8_t> &Data)
{
	if (Data.empty()) Data.resize(4096);
	fread(&Data[0], 1, Data.size(), Core); // TODO handle errors
}

void File::Seek(size_t Offset) { fseek(Core, Offset, SEEK_SET); }

File::~File(void) { if (Core) fclose(Core); }

File::File(FILE *Core) : Core(Core) {}

}

