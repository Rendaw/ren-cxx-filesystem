DoOnce 'ren-cxx-filesystem/Tupfile.lua'

for Index, Source in ipairs(tup.glob '*.cxx')
do
	Executable = Define.Executable
	{
		Name = tup.base(Source),
		Sources = Item(Source),
		Objects = FilesystemObjects,
	}

	Define.Test
	{
		Executable = Executable
	}
end

