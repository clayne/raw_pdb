// Copyright 2011-2022, Molecular Matters GmbH <office@molecular-matters.com>
// See LICENSE.txt for licensing details (2-clause BSD License: https://opensource.org/licenses/BSD-2-Clause)

#include "Examples_PCH.h"
#include "ExampleMemoryMappedFile.h"
#include "PDB.h"
#include "PDB_RawFile.h"
#include "PDB_InfoStream.h"
#include "PDB_DBIStream.h"
#include "PDB_TPIStream.h"

namespace
{
	PDB_NO_DISCARD static bool IsError(PDB::ErrorCode errorCode)
	{
		switch (errorCode)
		{
			case PDB::ErrorCode::Success:
				return false;

			case PDB::ErrorCode::InvalidSuperBlock:
				printf("Invalid Superblock\n");
				return true;

			case PDB::ErrorCode::InvalidFreeBlockMap:
				printf("Invalid free block map\n");
				return true;

			case PDB::ErrorCode::InvalidSignature:
				printf("Invalid stream signature\n");
				return true;

			case PDB::ErrorCode::InvalidStreamIndex:
				printf("Invalid stream index\n");
				return true;

			case PDB::ErrorCode::UnknownVersion:
				printf("Unknown version\n");
				return true;
		}

		// only ErrorCode::Success means there wasn't an error, so all other paths have to assume there was an error
		return true;
	}

	PDB_NO_DISCARD static bool HasValidDBIStreams(const PDB::RawFile& rawPdbFile, const PDB::DBIStream& dbiStream)
	{
		// check whether the DBI stream offers all sub-streams we need
		if (IsError(dbiStream.HasValidImageSectionStream(rawPdbFile)))
		{
			return false;
		}
		
		if (IsError(dbiStream.HasValidPublicSymbolStream(rawPdbFile)))
		{
			return false;
		}

		if (IsError(dbiStream.HasValidGlobalSymbolStream(rawPdbFile)))
		{
			return false;
		}

		if (IsError(dbiStream.HasValidSectionContributionStream(rawPdbFile)))
		{
			return false;
		}

		return true;
	}
}


// declare all examples
extern void ExampleSymbols(const PDB::RawFile&, const PDB::DBIStream&);
extern void ExampleContributions(const PDB::RawFile&, const PDB::DBIStream&);
extern void ExampleFunctionSymbols(const PDB::RawFile&, const PDB::DBIStream&);
extern void ExampleTypes(const PDB::TPIStream&);

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Incorrect usage\n");

		return 1;
	}

	printf("Opening PDB file %s\n", argv[1]);

	// try to open the PDB file and check whether all the data we need is available
	MemoryMappedFile::Handle pdbFile = MemoryMappedFile::Open(argv[1]);
	if (!pdbFile.baseAddress)
	{
		printf("Cannot memory-map file %s\n", argv[1]);

		return 1;
	}

	if (IsError(PDB::ValidateFile(pdbFile.baseAddress)))
	{
		MemoryMappedFile::Close(pdbFile);

		return 2;
	}

	const PDB::RawFile rawPdbFile = PDB::CreateRawFile(pdbFile.baseAddress);
	if (IsError(PDB::HasValidDBIStream(rawPdbFile)))
	{
		MemoryMappedFile::Close(pdbFile);

		return 3;
	}

	const PDB::InfoStream infoStream(rawPdbFile);
	if (infoStream.UsesDebugFastLink())
	{
		printf("PDB was linked using unsupported option /DEBUG:FASTLINK\n");

		MemoryMappedFile::Close(pdbFile);

		return 4;
	}

	const PDB::DBIStream dbiStream = PDB::CreateDBIStream(rawPdbFile);
	if (!HasValidDBIStreams(rawPdbFile, dbiStream))
	{
		MemoryMappedFile::Close(pdbFile);

		return 5;
	}

	const PDB::TPIStream tpiStream = PDB::CreateTPIStream(rawPdbFile);
	if (PDB::HasValidTPIStream(rawPdbFile) != PDB::ErrorCode::Success)
	{
		MemoryMappedFile::Close(pdbFile);

		return 5;
	}

	// run all examples
	ExampleContributions(rawPdbFile, dbiStream);
	ExampleSymbols(rawPdbFile, dbiStream);
	ExampleFunctionSymbols(rawPdbFile, dbiStream);
	ExampleTypes(tpiStream);

	MemoryMappedFile::Close(pdbFile);

	return 0;
}
