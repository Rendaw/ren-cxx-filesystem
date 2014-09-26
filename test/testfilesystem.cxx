#include <cassert>
#include <iostream>

#include "../filesystem.h"

int main(int, char **)
{
#ifdef WINDOWS
	std::string Prefix("c:");
#else
	std::string Prefix("");
#endif
	AssertE(Filesystem::PathT::Absolute(Prefix + "/")->Render(), Prefix + "/");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/")->Depth(), 0);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a")->Render(), Prefix + "/a");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a")->Depth(), 1);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/")->Render(), Prefix + "/a");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/")->Depth(), 1);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/1")->Render(), Prefix + "/a/1");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/1")->Depth(), 2);
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/1/")->Render(), Prefix + "/a/1");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/./")->Render(), Prefix + "/");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/./")->Render(), Prefix + "/a");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/./1")->Render(), Prefix + "/a/1");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/..")->Render(), Prefix + "/");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/../1")->Render(), Prefix + "/1");
	Assert(Filesystem::PathT::Absolute(Prefix + "/")->Contains(Filesystem::PathT::Absolute(Prefix + "/")));
	Assert(!Filesystem::PathT::Absolute(Prefix + "/a")->Contains(Filesystem::PathT::Absolute(Prefix + "/")));
	Assert(Filesystem::PathT::Absolute(Prefix + "/")->Contains(Filesystem::PathT::Absolute(Prefix + "/a")));
	Assert(!Filesystem::PathT::Absolute(Prefix + "/a")->Contains(Filesystem::PathT::Absolute(Prefix + "/b")));
	Assert(!Filesystem::PathT::Absolute(Prefix + "/a/1")->Contains(Filesystem::PathT::Absolute(Prefix + "/a/2")));
	AssertE(Filesystem::PathT::Absolute(Prefix + "/c.txt")->Filename(), "c.txt");
	Assert(Filesystem::PathT::Here()->Enter("filesystemtesttree")->Enter("a")->Enter("1.txt")->Exists());
	Assert(!Filesystem::PathT::Here()->Enter("filesystemtesttree")->Enter("a")->Enter("9.txt")->Exists());
	AssertE(Filesystem::PathT::Absolute(Prefix + "/c.txt")->Directory(), Prefix + "/");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/c.txt")->Filename(), "c.txt");
	AssertE(Filesystem::PathT::Absolute(Prefix + "/a/c.txt")->Directory(), Prefix + "/a");

	// ascii
	{
		std::string 
			Test1("ruby"), 
			Test2("venezuela"), 
			Test3("lovely");

		auto Root = Filesystem::PathT::Qualify(Test3);
		Root->CreateDirectory();
		Assert(Root->Exists());
		Assert(Root->GoTo());

		auto TestPath = Filesystem::PathT::Qualify(Test1 + "/" + Test2 + "/" + Test3 + ".txt");
		AssertE(TestPath->Filename(), Test3 + ".txt");
		AssertE(Filesystem::PathT::Qualify(Prefix + "/")->Enter(Test1)->Render(), Prefix + "/" + Test1);

		Assert(TestPath->Exit()->CreateDirectory());
		auto Created = Filesystem::fopen(TestPath->Render(), "w");
		Assert(Created);
		fclose(Created);
		auto Opened = Filesystem::fopen(TestPath->Render(), "r");
		Assert(Created);
		fclose(Opened);
		Assert(TestPath->Exit()->DeleteDirectory());

		Assert(Root->Exit()->GoTo());
		Assert(Root->DeleteDirectory());
		Assert(!Root->Exists());
	}

	// utf8
	{
		std::string 
			Test1("\xE5\xAD\x90\xE4\xBE\x9B"), 
			Test2("\xE5\xA4\xA7\x20\xE4\xBA\xBA"), 
			Test3("\xE3\x83\x95\xE3\x82\xA6\xE3\x83\x81\xE3\x83\xA7\xE3\x82\xA6\xE7\xA7\x91");

		auto Root = Filesystem::PathT::Qualify(Test3);
		Root->CreateDirectory();
		Assert(Root->Exists());
		Assert(Root->GoTo());

		auto TestPath = Filesystem::PathT::Qualify(Test1 + "/" + Test2 + "/" + Test3 + ".txt");
		AssertE(TestPath->Filename(), Test3 + ".txt");
		AssertE(Filesystem::PathT::Qualify(Prefix + "/")->Enter(Test1)->Render(), Prefix + "/" + Test1);

		Assert(TestPath->Exit()->CreateDirectory());
		auto Created = Filesystem::fopen(TestPath->Render(), "w");
		Assert(Created);
		fclose(Created);
		auto Opened = Filesystem::fopen(TestPath->Render(), "r");
		Assert(Created);
		fclose(Opened);
		Assert(TestPath->Exit()->DeleteDirectory());

		Assert(Root->Exit()->GoTo());
		Assert(Root->DeleteDirectory());
		Assert(!Root->Exists());
	}
}

