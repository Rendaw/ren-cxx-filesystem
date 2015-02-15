#include <cassert>
#include <iostream>
#include <set>

#include "../path.h"
#include "../file.h"

int main(int, char **)
{
#ifdef WINDOWS
#define SEP "\\"
	std::string Prefix("c:");
#else
#define SEP "/"
	std::string Prefix("");
#endif
	AssertE(Filesystem::PathT::Absolute(Prefix + "/").Render(), Prefix + SEP);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/").Depth(), 0u);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a").Render(), Prefix + SEP "a");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a").Depth(), 1u);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/").Render(), Prefix + SEP "a");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/").Depth(), 1u);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/1").Render(), Prefix + SEP "a" SEP "1");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/1").Depth(), 2u);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/1/").Render(), Prefix + SEP "a" SEP "1");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/./").Render(), Prefix + SEP);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/./").Render(), Prefix + SEP "a");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/./1").Render(), Prefix + SEP "a" SEP "1");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/..").Render(), Prefix + SEP);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/../1").Render(), Prefix + SEP "1");
	Assert(Filesystem::PathT::Absolute(Prefix + "/").Contains(Filesystem::PathT::Absolute(Prefix + SEP)));
	Assert(!Filesystem::PathT::Absolute(Prefix + "/a").Contains(Filesystem::PathT::Absolute(Prefix + SEP)));
	Assert(Filesystem::PathT::Absolute(Prefix + "/").Contains(Filesystem::PathT::Absolute(Prefix + SEP "a")));
	Assert(!Filesystem::PathT::Absolute(Prefix + "/a").Contains(Filesystem::PathT::Absolute(Prefix + SEP "b")));
	Assert(!Filesystem::PathT::Absolute(Prefix + "/a/1").Contains(Filesystem::PathT::Absolute(Prefix + SEP "a" SEP "2")));
	AssertE(Filesystem::PathT::Absolute(Prefix + "/c.txt").Filename(), "c.txt");
	Assert(Filesystem::PathT::Here().Enter("filesystemtesttree").Enter("a").Enter("1.txt").Exists());
	Assert(!Filesystem::PathT::Here().Enter("filesystemtesttree").Enter("a").Enter("9.txt").Exists());
	AssertE(Filesystem::PathT::Absolute(Prefix + "/c.txt").Directory(), Prefix + SEP);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/c.txt").Filename(), "c.txt");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/c.txt").Directory(), Prefix + SEP "a");

	{
		//std::vector<std::string> Files, Dirs;
		std::set<std::string> Files, Dirs; // tup fuse error?  Was seeing a duplicated directory.
		Filesystem::PathT::Here().Enter("filesystemtesttree").Enter("a").List(
			[&](Filesystem::PathT &&Path, bool IsFile, bool IsDir)
		{
			std::cout << "Checking listed file: " << Path << std::endl;
			if (IsFile) Files.insert(Path.Filename());
			else if (IsDir) Dirs.insert(Path.Filename());
			else Assert(false);
			return true;
		});
		AssertE(Files.size(), 3u);
		AssertE(Files.count("1.txt"), 1u);
		AssertE(Files.count("2.txt"), 1u);
		AssertE(Files.count("3.txt"), 1u);
		/*{ bool Found = false; for (auto const &Elem : Files) if (Elem == "1.txt") Found = true; Assert(Found); }
		{ bool Found = false; for (auto const &Elem : Files) if (Elem == "2.txt") Found = true; Assert(Found); }
		{ bool Found = false; for (auto const &Elem : Files) if (Elem == "3.txt") Found = true; Assert(Found); }*/
		AssertE(Dirs.size(), 1u);
		AssertE(Dirs.count("a1"), 1u);
	}

	// ascii
	{
		std::string 
			Test1("ruby"), 
			Test2("venezuela"), 
			Test3("lovely");

		auto Root = Filesystem::PathT::Qualify(Test3);
		Assert(Root.CreateDirectory());
		Assert(Root.Exists());
		Assert(Root.GoTo());

		auto TestPath = Filesystem::PathT::Qualify(Test1 + "/" + Test2 + "/" + Test3 + ".txt");
		AssertE(TestPath.Filename(), Test3 + ".txt");
		AssertE(Filesystem::PathT::Qualify(Prefix + "/").Enter(Test1).Render(), Prefix + SEP + Test1);

		Assert(TestPath.Exit().CreateDirectory());
		{
			auto Created = Filesystem::FileT::OpenWrite(TestPath);
			Assert(Created);
		}
		{
			auto Opened = Filesystem::FileT::OpenRead(TestPath);
			Assert(Opened);
		}
		Assert(TestPath.Exit().DeleteDirectory());

		Assert(Root.Exit().GoTo());
		Assert(Root.DeleteDirectory());
		Assert(!Root.Exists());
	}

	// utf8
	{
		std::string 
			Test1("\xE5\xAD\x90\xE4\xBE\x9B"), 
			Test2("\xE5\xA4\xA7\x20\xE4\xBA\xBA"), 
			Test3("\xE3\x83\x95\xE3\x82\xA6\xE3\x83\x81\xE3\x83\xA7\xE3\x82\xA6\xE7\xA7\x91");

		auto Root = Filesystem::PathT::Qualify(Test3);
		Root.CreateDirectory();
		Assert(Root.Exists());
		Assert(Root.GoTo());

		auto TestPath = Filesystem::PathT::Qualify(Test1 + "/" + Test2 + "/" + Test3 + ".txt");
		AssertE(TestPath.Filename(), Test3 + ".txt");
		AssertE(Filesystem::PathT::Qualify(Prefix + "/").Enter(Test1).Render(), Prefix + SEP + Test1);

		Assert(TestPath.Exit().CreateDirectory());
		{
			auto Created = Filesystem::FileT::OpenWrite(TestPath);
			Assert(Created);
		}
		{
			auto Opened = Filesystem::FileT::OpenRead(TestPath);
			Assert(Opened);
		}
		Assert(TestPath.Exit().DeleteDirectory());

		Assert(Root.Exit().GoTo());
		Assert(Root.DeleteDirectory());
		Assert(!Root.Exists());
	}
}

