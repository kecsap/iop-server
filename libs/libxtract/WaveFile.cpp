
#include "WaveFile.h"

#include <fstream>
#include <iostream>
#include <cassert>
#include <cstring>
#include <stdint.h>

namespace
{
	struct RIFFChunk
	{
		uint32_t chunkID;
		uint32_t chunkSize;
		uint32_t format;
	};
	struct fmtChunk
	{
		uint32_t chunkID;
		uint32_t chunkSize;
		uint16_t audioFormat;
		uint16_t numChannels;
		uint32_t sampleRate;
		uint32_t byteRate;
		uint16_t blockAlign;
		uint16_t bitsPerSample;
	};
	struct WaveHeader
	{
		RIFFChunk riff;
		fmtChunk fmt;
	};
}

WaveFile::WaveFile() : data(NULL), size(0)
{
}
WaveFile::WaveFile(const std::string &filename) : data(NULL), size(0)
{
	Load(filename);
}
WaveFile::~WaveFile()
{
	Unload();
}

bool WaveFile::Load(const std::string &filename)
{
	if (IsLoaded())
	{
		Unload();
	}

	std::fstream file(filename.c_str(), std::ios::in | std::ios::binary);

	if (!file.is_open())
	{
		std::cerr << "Error: Could not open file." << std::endl;
		return false;
	}

	WaveHeader header;
	std::memset(&header, 0, sizeof(WaveHeader));

	while (file.peek() != std::char_traits<char>::eof())
	{
		uint32_t chunkID;
		uint32_t chunkSize;

		file.read(reinterpret_cast<char*>(&chunkID), sizeof(uint32_t));
		file.read(reinterpret_cast<char*>(&chunkSize), sizeof(uint32_t));

		switch (chunkID)
		{
//		case 'FFIR':
		case 0x46464952:
			{
				header.riff.chunkID = chunkID;
				header.riff.chunkSize = chunkSize;
				file.read(reinterpret_cast<char*>(&header.riff.format), sizeof(uint32_t));

//				if (header.riff.format != 'EVAW')
				if (header.riff.format != 0x45564157)
				{
					std::cerr << "Error: Not a valid WAVE file." << std::endl;
					return false;
				}

				break;
			}
//		case ' tmf':
		case 0x20746d66:
			{
				header.fmt.chunkID = chunkID;
				header.fmt.chunkSize = chunkSize;
				file.read(reinterpret_cast<char*>(&header.fmt.audioFormat), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&header.fmt.numChannels), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&header.fmt.sampleRate), sizeof(uint32_t));
				file.read(reinterpret_cast<char*>(&header.fmt.byteRate), sizeof(uint32_t));
				file.read(reinterpret_cast<char*>(&header.fmt.blockAlign), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&header.fmt.bitsPerSample), sizeof(uint16_t));

				if (header.fmt.audioFormat != PCM &&
                    header.fmt.audioFormat != WAVE_FORMAT_IEEE_FLOAT)
				{
					std::cerr << "Error: Not in valid format" << std::endl;
					return false;
				}
				if (header.fmt.bitsPerSample % 2 != 0)
				{
					std::cerr << "Error: Invalid number of bits per sample" << std::endl;
					return false;
				}
				if (header.fmt.byteRate != (header.fmt.sampleRate * header.fmt.numChannels * header.fmt.bitsPerSample / 8))
				{
					std::cerr << "Error: Invalid byte rate" << std::endl;
					return false;
				}
				if (header.fmt.blockAlign != (header.fmt.numChannels * header.fmt.bitsPerSample / 8))
				{
					std::cerr << "Error: Invalid block align" << std::endl;
					return false;
				}

				break;
			}
//		case 'atad':
		case 0x61746164:
			{
				assert(data == NULL);
				size = chunkSize;
				data = new char[size];
				file.read(data, chunkSize);

				break;
			}
		default:
			{
				file.ignore(chunkSize);

				break;
			}
		}
	}

	// Check that we got all chunks
//	if (header.riff.chunkID != 'FFIR')
	if (header.riff.chunkID != 0x46464952)
	{
		std::cerr << "Error: Missing RIFF chunk." << std::endl;
		return false;
	}
//	if (header.fmt.chunkID != ' tmf')
	if (header.fmt.chunkID != 0x20746d66)
	{
		std::cerr << "Error: Missing fmt chunk." << std::endl;
		return false;
	}
	if (data == NULL || size == 0)
	{
		std::cerr << "Error: Missing data chunk." << std::endl;
		return false;
	}

	// Fill meta struct
	meta.audioFormat   = static_cast<AudioFormat>(header.fmt.audioFormat);
	meta.numChannels   = header.fmt.numChannels;
	meta.sampleRate    = header.fmt.sampleRate;
	meta.bitsPerSample = header.fmt.bitsPerSample;

	return true;
}
void WaveFile::Unload()
{
	delete[] data;
	data = NULL;
	size = 0;
}
