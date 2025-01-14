#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>

const int SYNC_HEADER_SIZE = 12;

bool convertToIso(const char* sourceFilename, const char* targetFilename) {
	FILE *source = fopen(sourceFilename, "rb");
	FILE *target = fopen(targetFilename, "wb");
	if (!source || !target) {
		std::cerr << "Error opening files!" << std::endl;
		if (source) fclose(source);
		if (target) fclose(target);
		return false;
	}

	std::cout << "Converting " << sourceFilename << " to " << targetFilename << "..." << std::endl;

	const unsigned char SYNC_HEADER[SYNC_HEADER_SIZE] = {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0};
	unsigned char buf[2352];
	int seek_header, seek_ecc, sector_size;

	// Read first 16 bytes to determine sector type
	fread(buf, sizeof(char), 16, source);
	if (memcmp(SYNC_HEADER, buf, SYNC_HEADER_SIZE) == 0) {
		// Sync header present, determine mode
		switch (buf[15]) {
			case 1:
				seek_header = 16;
				seek_ecc = 288;
				sector_size = 2352;
				break;
			case 2:
				seek_header = 24;
				seek_ecc = 280;
				sector_size = 2352;
				break;
			default:
				std::cerr << "Unsupported track mode." << std::endl;
				fclose(source);
				fclose(target);
				return false;
		}
	} else {
		// No sync header, default to Mode 2/2336
		seek_header = 8;
		seek_ecc = 280;
		sector_size = 2336;
	}

	fseek(source, 0, SEEK_SET);

	while (fread(buf, 1, sector_size, source) == sector_size) {
		fwrite(buf + seek_header, 1, 2048, target); // Write only data portion
		// No need to seek as fread reads the full sector size
	}

	fclose(target);
	fclose(source);
	std::cout << "Conversion completed." << std::endl;
	return true;
}

bool deleteFile(const char* filename) {
	std::cout << "Deleting " << filename << "..." << std::endl;
	if (std::remove(filename) != 0) {
		std::cerr << "Error deleting file: " << filename << std::endl;
		return false;
	}
	return true;
}

bool writeGdiFile() {
	std::cout << "Writing new disc.gdi file..." << std::endl;
	std::ofstream file("disc.gdi");
	if (!file) {
		std::cerr << "Error opening disc.gdi for writing!" << std::endl;
		return false;
	}

	file << "3\n";
	file << "1 0 4 2048 track01.iso 0\n";
	file << "2 450 0 2352 track02.raw 0\n";
	file << "3 45000 4 2048 track03.iso 0\n";

	file.close();
	std::cout << "New disc.gdi file created." << std::endl;
	return true;
}

int main() {
	if (!convertToIso("track01.bin", "track01.iso") || !deleteFile("track01.bin") ||
		!convertToIso("track03.bin", "track03.iso") || !deleteFile("track03.bin")) {
		std::cerr << "An error occurred during file processing." << std::endl;
		return 1;
	}

	if (!deleteFile("disc.gdi") || !writeGdiFile()) {
		std::cerr << "An error occurred during .gdi handling." << std::endl;
		return 1;
	}

	std::cout << "Operation completed successfully." << std::endl;
	std::cout << "Press Enter to close..." << std::endl;
	std::cin.get();
	return 0;
}