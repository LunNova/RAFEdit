
#include "stdafx.h"
#include "raf.h"
#include "string.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <direct.h>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace System;
using namespace std;
using namespace RAF;
using namespace System::IO;
using namespace System::Runtime::InteropServices;
using namespace Ionic::Zlib;

namespace RAF {
	/* I'm lazy, via http://niallohiggins.com/2009/01/08/mkpath-mkdir-p-alike-in-c-for-unix/
	works like mkdir(1) used as "mkdir -p" */
	int mkdirp(const char *pathname, bool file)
	{
		char parent[2048], *p;
		/* make a parent directory path */
		strncpy_s(parent, sizeof(parent)-1, pathname, sizeof(parent));
		parent[sizeof(parent)-1] = '\0';
		for (p = parent + strlen(parent); *p != '\\' && *p != '/' && p > parent; p--);
		*p = '\0';
		/* try make parent directory */
		if (p != parent) {
			mkdirp(parent, false);
			if (_mkdir(parent) == 0)
				return 0;
			if (errno == EEXIST)
				return 0;
		}
		/* make this one if parent has been made */
		/* if it already exists that is fine */
		return -1;
	}

	RAFFile::RAFFile() {
		header = new Header_t;
		toc = new TableOfContents_t;
		pathListHeader = new PathListHeader_t;
	};
	RAFFile::!RAFFile() {
		free(header);
		free(toc);
		free(fileListEntry);
		free(pathListHeader);
		free(pathListEntry);
		free(strings);
	}
	void RAFFile::dump() {
		Console::WriteLine("RAF file version: " + (*header).mVersion
			+ "\n\nTOC"
			+ "\nmgrIndex: " + (*toc).mMgrIndex
			+ "\nfileListOffset: " + (*toc).mFileListOffset
			+ "\nstringTableOffset: " + (*toc).mStringTableOffset
			+ "\n\nFile List"
			+ "\ncount: " + fileListHeaderCount
			+ "\n\nEntries:"
			);

		//system("PAUSE");
		for (unsigned long i = 0; i < fileListHeaderCount; i++) {
			FileListEntry_t f = fileListEntry[i];
			PathListEntry_t p = pathListEntry[f.mFileNameStringTableIndex];
			int stringOffset = p.mOffset - pathListOffset;
			assert(stringOffset >= 0);
			Console::Write(
				""
				+ "\nName: " + gcnew System::String(&strings[stringOffset])
				+ "\nHash: " + f.mHash
				+ "\nOffset: " + f.mOffset
				+ "\nSize: " + f.mSize
				+ "\nEntry: " + i + "/" + fileListHeaderCount
				+ "\nf_: " + i * sizeof(FileListEntry_t));

		}
	};
	void RAFFile::update(std::string* datFile) {
		update(NULL, datFile, NULL);
	}
	void RAFFile::unpack(std::string* outputDirectory, std::string* datFileIn) {
		FILE* in;
		fopen_s(&in, datFileIn->c_str(), "rb");
		Console::WriteLine("Opened " + gcnew String(datFileIn->c_str()));
		unsigned long pos = 0;
		for (unsigned long i = 0; i < fileListHeaderCount; i++) {
			FileListEntry_t* f = &fileListEntry[i];
			PathListEntry_t p = pathListEntry[f->mFileNameStringTableIndex];
			int stringOffset = p.mOffset - pathListOffset;
			char * curName = &strings[stringOffset];
			unsigned char* buf = (unsigned char*)malloc(f->mSize);
			fseek(in, f->mOffset, SEEK_SET);
			if (fread(buf, f->mSize, 1, in) == 0) {
				Console::WriteLine("Read failed for " + gcnew System::String(curName));
			}
			Console::WriteLine("Read " + f->mSize + " bytes for " + gcnew String(curName));
			array<Byte>^ decompressed_buf;
			if (buf[0] == 0x78 && buf[1] == 0x9C) {
				try {
					array<Byte>^ out_buf = gcnew array<Byte>(f->mSize);
					Marshal::Copy(IntPtr(buf), out_buf, 0, Int32(f->mSize));
					decompressed_buf = Ionic::Zlib::ZlibStream::UncompressBuffer(out_buf);
					free(buf);
					buf = new unsigned char[decompressed_buf->Length];
					Marshal::Copy(decompressed_buf, 0, IntPtr(buf), decompressed_buf->Length);
				}
				catch (ZlibException^ e) {
					Console::WriteLine("Assuming " + gcnew System::String(curName) + " is not compressed: ZlibException " + e->Message);
				}
			}
			FILE* out;
			string file(*outputDirectory);
			file.append("/").append(curName);
			fopen_s(&out, file.c_str(), "wb");
			if (!out) {
				mkdirp(file.c_str(), true);
				int e = fopen_s(&out, file.c_str(), "wb");
				if (!out) {
					Console::WriteLine("Failed to make file " + gcnew String(file.c_str()) + ", code: " + e);
				}
			}
			fwrite(buf, decompressed_buf->Length, 1, out);
			Console::WriteLine("Wrote " + decompressed_buf->Length + " bytes for " + gcnew String(file.c_str()));
			free(buf);
			if (out) {
				fclose(out);
			}
		}
		fclose(in);
	}
	void RAFFile::update(std::string* replacementDirectory, std::string* datFileIn, std::string* datFileOut) {
		FILE* in;
		FILE* out;
		fopen_s(&in, datFileIn->c_str(), "rb");
		fopen_s(&out, datFileOut->c_str(), "wb");
		Console::WriteLine("Opened " + gcnew String(datFileIn->c_str()) + " and " + gcnew String(datFileOut->c_str()));
		//FILE* out = 
		unsigned long pos = 0;
		for (unsigned long i = 0; i < fileListHeaderCount; i++) {
			FileListEntry_t* f = &fileListEntry[i];
			PathListEntry_t p = pathListEntry[f->mFileNameStringTableIndex];
			int stringOffset = p.mOffset - pathListOffset;
			char * curName = &strings[stringOffset];
			bool add = true;
			if (replacementDirectory != NULL) {
				FILE* replacement;
				string file(*replacementDirectory);
				file.append("/").append(curName);
				Console::WriteLine("Checking for " + gcnew String(file.c_str()));
				fopen_s(&replacement, file.c_str(), "rb");
				if (replacement != NULL) {
					add = false;
					fseek(replacement, 0, SEEK_END);
					f->mSize = ftell(replacement);
					fseek(replacement, 0, SEEK_SET);
					unsigned char* buf = (unsigned char*)malloc(f->mSize);
					fread(buf, f->mSize, 1, replacement);
					fclose(replacement);
					array<Byte>^ compressed_buf;
					{
						array<Byte>^ out_buf = gcnew array<Byte>(f->mSize);
						Marshal::Copy(IntPtr(buf), out_buf, 0, Int32(f->mSize));
						compressed_buf = Ionic::Zlib::ZlibStream::CompressBuffer(out_buf);
					}
					free(buf);
					f->mSize = compressed_buf->Length;
					buf = new unsigned char[compressed_buf->Length];
					Marshal::Copy(compressed_buf, 0, IntPtr(buf), compressed_buf->Length);
					buf[1] = 0x9C;
					fwrite(buf, f->mSize, 1, out);
					free(buf);
				}
			}
			if (add) {
				char* buf = (char*) malloc(f->mSize);
				fseek(in, f->mOffset, SEEK_SET);
				if (fread(buf, f->mSize, 1, in) == 0) {
					Console::WriteLine("Read failed for "
						+ "\nName: " + gcnew System::String(curName)
						+ "\nHash: " + f->mHash
						+ "\nOffset: " + f->mOffset
						+ "\nSize: " + f->mSize
						+ "\nEntry: " + i + "/" + fileListHeaderCount
						+ "\nf_: " + i * sizeof(FileListEntry_t));
				}
				fwrite(buf, f->mSize, 1, out);
				free(buf);
				
				Console::WriteLine("At " + pos + " for file " + gcnew String(curName));
			}
			f->mOffset = pos;
			pos += f->mSize;
			assert(ftell(out) == pos);
		}
		fclose(in);
		fclose(out);
	};
	void RAFFile::write(std::string file) {
		FILE* f;
		fopen_s(&f, file.c_str(), "wb");
		fwrite(header, sizeof(Header_t), 1, f);
		toc->mFileListOffset = sizeof TableOfContents_t + sizeof Header_t;
		toc->mStringTableOffset = toc->mFileListOffset + sizeof FileListHeader_t + sizeof FileListEntry_t * fileListHeaderCount;
		fwrite(toc, sizeof(TableOfContents_t), 1, f);
		pin_ptr<unsigned long> pinnedCount = &fileListHeaderCount;
		fwrite(pinnedCount, sizeof (unsigned long), 1, f);
		fwrite(fileListEntry, sizeof FileListEntry_t, fileListHeaderCount, f);
		fwrite(pathListHeader, sizeof PathListHeader_t, 1, f);
		fwrite(pathListEntry, sizeof PathListEntry_t, pathListHeader->mCount, f);
		fwrite(strings, pathListHeader->mBytes - pathListOffset, 1, f);
		fclose(f);
	}
	void RAFFile::read(std::string file) {
		FILE* f;
		fopen_s(&f, file.c_str(), "rb");
		fread(header, sizeof(Header_t), 1, f);
		fread(toc, sizeof(TableOfContents_t), 1, f);
		
		fseek(f, (*toc).mFileListOffset, SEEK_SET);
		pin_ptr<unsigned long> pinnedCount = &fileListHeaderCount;
		fread(pinnedCount, sizeof(unsigned long), 1, f);
		fileListEntry = (FileListEntry_t*) malloc(sizeof(FileListEntry_t)* fileListHeaderCount);
		fread(fileListEntry, sizeof(FileListEntry_t), fileListHeaderCount, f);

		unsigned long offset = (*toc).mStringTableOffset;
		fseek(f, offset, SEEK_SET);
		fread(pathListHeader, sizeof PathListHeader_t, 1, f);

		unsigned long count = pathListHeader->mCount;
		pathListEntry = (PathListEntry_t*) malloc(sizeof PathListEntry_t * count);
		fread(pathListEntry, sizeof PathListEntry_t, count, f);

		if (ferror(f)) {
			Console::WriteLine("Error " + ferror(f) + " reading.");
		}
		else if (feof(f)) {
			Console::WriteLine("Reached EOF.");
		}


		pathListOffset = sizeof PathListHeader_t + sizeof PathListEntry_t * count;
		strings = (char*) malloc(pathListHeader->mBytes);
		fread(strings, pathListHeader->mBytes - pathListOffset, 1, f);

		Console::WriteLine("Read " + pathListHeader->mBytes + " string bytes");

		if (ferror(f)) {
			Console::WriteLine("Error " + ferror(f) + " reading.");
		}
		else if (feof(f)) {
			Console::WriteLine("Reached EOF.");
		}
		fclose(f);
	};
};