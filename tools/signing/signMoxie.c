/*
*******************************************************************************    
*   BOLOS TEE Tools 
*   (c) 2016 Ledger
*   
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*   limitations under the License.
********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libelf.h>
#include <gelf.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "secp256k1.h"
#include "sha2.h"

#ifndef EM_MOXIE
#define EM_MOXIE                223  /* Official Moxie */
#endif // EM_MOXIE

#ifndef EM_MOXIE_OLD
#define EM_MOXIE_OLD            0xFEED /* Old Moxie */
#endif // EM_MOXIE_OLD

int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

bool readPrivateKey(uint8_t *privateKey) {
	int i = 0;
	printf("Enter private key:");
	for(i=0; i<32; i++) {
		char tmp[3];
		tmp[0] = getch();
		if (tmp[0] == '\n') {
			return false;
		}
		tmp[1] = getch();
		if (tmp[1] == '\n') {
			return false;
		}
		tmp[2] = '\0';
		privateKey[i] = strtol(tmp, NULL, 16);
	}
	printf("\n");
	return true;
}

void hashU32(SHA256_CTX *sha, uint32_t data) {
	unsigned char buffer[4];
	buffer[0] = (data >> 24);
	buffer[1] = ((data >> 16) & 0xff);
	buffer[2] = ((data >> 8) & 0xff);
	buffer[3] = (data & 0xff);
	sha256_Update(sha, buffer, 4);
}

int main(int argc, char **argv) {
	uint32_t currentProgram = 1;
	uint8_t privateKey[32];
	SHA256_CTX sha;
	uint32_t entryPoint;
	int fd;
	Elf *elfFile;
	GElf_Ehdr elfMainHeader;
	size_t elfProgramSize;
	size_t i;
	size_t stringIndexSize;

	if (argc < 3) {
		fprintf(stderr, "Usage : %s moxie_ELF_program_to_parse target_signature_file\n", argv[0]);
		return 1;
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		fprintf(stderr, "Failed to initialize ELF parsing library\n");
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open ELF %s\n", argv[1]);
		return 1;		
	}

	elfFile = elf_begin(fd, ELF_C_READ, NULL);
	if (elfFile == NULL) {
		fprintf(stderr, "Failed to parse ELF file\n");
		goto error;
	}

	if (elf_kind(elfFile) != ELF_K_ELF) {
		fprintf(stderr, "Invalid ELF file\n");
		goto error;
	}
	if (elf_getshdrstrndx(elfFile, &stringIndexSize) != 0) {
		fprintf(stderr, "Failed to retrieve string index size\n");
		goto error;
	}
	if (gelf_getehdr(elfFile, &elfMainHeader) != &elfMainHeader) {
		fprintf(stderr, "Failed to parse ELF main header\n");
		goto error;
	}
	if ((elfMainHeader.e_ident[EI_CLASS] != ELFCLASS32) ||
			(elfMainHeader.e_ident[EI_DATA] != ELFDATA2LSB) ||
			((elfMainHeader.e_machine != EM_MOXIE)
				&& (elfMainHeader.e_machine != EM_MOXIE_OLD))) {
				fprintf(stderr, "Usupported ELF binary type\n");
				goto error;
  }

	if (elf_getphdrnum(elfFile, &elfProgramSize) != 0) {
		fprintf(stderr, "Error reading number of ELF program headers\n");
    goto error;
	}

  sha256_Init(&sha);

  for (i=0; i<elfProgramSize; i++) {
  	uint8_t flag = 0;
		GElf_Phdr elfHeader;
		if (gelf_getphdr(elfFile, i, &elfHeader) != &elfHeader) {
			fprintf(stderr, "Failed to parse ELF program header %ld\n", i);
			goto error;
		}
		if (elfHeader.p_type != PT_LOAD) {
			continue;
		}
		if (((elfHeader.p_flags & PF_W) == 0)) {
			flag |= 0x01;
		}
		sha256_Update(&sha, &flag, 1);
		hashU32(&sha, elfHeader.p_vaddr);
		hashU32(&sha, elfHeader.p_vaddr + elfHeader.p_memsz);
		hashU32(&sha, elfHeader.p_filesz);
		off_t current = lseek(fd, 0, SEEK_CUR);		
		uint32_t fileOffset = 0;
		lseek(fd, elfHeader.p_offset, SEEK_SET);
		while (fileOffset != elfHeader.p_filesz) {
			uint8_t tmp[4096];
			uint32_t chunkSize = (fileOffset + sizeof(tmp) > elfHeader.p_filesz ? elfHeader.p_filesz - fileOffset : sizeof(tmp));
			if (read(fd, tmp, chunkSize) != chunkSize) {
				fprintf(stderr, "Failed to read ELF data\n");
				goto error;
			}
			sha256_Update(&sha, tmp, chunkSize);
			fileOffset += chunkSize;
		}
		lseek(fd, current, SEEK_SET);
  }
  hashU32(&sha, elfMainHeader.e_entry);

  if (!readPrivateKey(privateKey)) {
		fprintf(stderr, "User aborted\n");
		goto error;
  }  

	uint8_t hash[32];
	uint8_t der[100];
	size_t derLength = sizeof(der);
	memset(der, 0, sizeof(der));
	secp256k1_ecdsa_signature sig;	
	secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
	sha256_Final(hash, &sha);	

	int result = secp256k1_ecdsa_sign(ctx, &sig, hash, privateKey, NULL, NULL);
	secp256k1_context_destroy(ctx);
	memset(privateKey, 0, sizeof(privateKey));
	if (result == 0) {
		fprintf(stderr, "Signature failed\n");
		goto error;
	}
	if (secp256k1_ecdsa_signature_serialize_der(ctx, der, &derLength, &sig) == 0) {
		fprintf(stderr, "Signature serialization failed\n");
		goto error;
	}

	close(fd);

	fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		fprintf(stderr, "Error creating signature file\n");
		return 1;
	}
	if (write(fd, der, sizeof(der)) != sizeof(der)) {
		close(fd);
		fprintf(stderr, "Error writing signature\n");
		return 1;		
	}

	close(fd);

	return 0;

error:
	close(fd);
	return 1;
}


