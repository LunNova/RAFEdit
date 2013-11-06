#include "stdafx.h"
#include <cstdio>
#include <cstdlib>
#include <string>

namespace RAF
{
	// Header that appears at start of the directory file
	public struct Header_t
	{
		// Magic value used to identify the file type, must be 0x18BE0EF0
		unsigned long	mMagic;

		// Version of the archive format, must be 1
		unsigned long	mVersion;
	};

	// Table of contents appears directly after header
	public struct TableOfContents_t
	{
		// An index that is used by the runtime, do not modify
		unsigned long	mMgrIndex;

		// Offset to the file list from the beginning of the file
		unsigned long	mFileListOffset;

		// Offset to the string table from the beginning of the file
		unsigned long	mStringTableOffset;
	};

	// Header of the file list
	public struct FileListHeader_t
	{
		// Number of entries in the list
		unsigned long	mCount;
	};

	// An entry in the file list describes a file that has been archived
	public struct FileListEntry_t
	{
		// Hash of the string name
		unsigned long	mHash;

		// Offset to the start of the archived file in the data file
		unsigned long	mOffset;

		// Size of this archived file
		unsigned long	mSize;

		// Index of the name of the archvied file in the string table
		unsigned long	mFileNameStringTableIndex;
	};

	public struct PathListHeader_t
	{
		unsigned long	mBytes;
		unsigned long	mCount;
	};

	public struct PathListEntry_t
	{
		unsigned long	mOffset;
		unsigned long	mLength;
	};


	ref class RAFFile {
		~RAFFile() { this->!RAFFile(); }
		!RAFFile();
	public:
		RAFFile();
		void dump();
		void read(std::string file);
		void write(std::string file);
		void unpack(std::string* outputDirectory, std::string* datFileIn);
		void update(std::string* datFile);
		void update(std::string* replacementDirectory, std::string* datFileIn, std::string* datFileOut);
		Header_t* header;
		TableOfContents_t* toc;
		unsigned long fileListHeaderCount;
		FileListEntry_t* fileListEntry;
		PathListHeader_t* pathListHeader;
		PathListEntry_t* pathListEntry;
		char* strings;
		int pathListOffset;
	};
}