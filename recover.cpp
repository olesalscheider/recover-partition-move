#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <unistd.h> 

#include <cstddef>
#include <string>
#include <cstring>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
    std::string filename = "/dev/sdc";
    size_t block_size = 16 * 1024UL * 1024UL;

    int fd;
    fd = open(filename.c_str(), O_RDWR | O_LARGEFILE);
    if (fd < 0) {
        std::cerr << "Failed to open file" << std::endl;
    }
    size_t mmap_size = lseek64(fd, 0, SEEK_END);
    lseek64(fd, 0, SEEK_SET);
    std::cout << "File size is " << mmap_size << std::endl;
    char * mmaped_data = (char*) mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmaped_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    size_t start_offset = (size_t(mmap_size * 0.2) / block_size) * block_size;

    bool valid = false;
    size_t sameCount = 0;
    size_t search_offs = 2 * 1024UL * 1024UL * 1024UL;
    size_t offs = start_offset;
    while (true) {
        const char* ref_block = mmaped_data + offs;
        const char* cmp_block = ref_block - search_offs;
        if (std::memcmp(ref_block, cmp_block, block_size) == 0) {
            std::cout << "Found matching block for " << int64_t(offs) << " with search offset " << search_offs << std::endl;
            sameCount++;

            if (sameCount == 1024UL * 1024UL * 1024UL / block_size) {
                std::cout << "I found 1 GB matching data. This is enough for me!" << std::endl;
                valid = true;
                break;
            }

            offs -= block_size;
        } else {
            sameCount = 0;
            offs -= 8 * block_size;
        }

        if (offs - search_offs < 0) {
            break;
        }
    }

    size_t ctr = 0;
    if (valid) {
        std::cout << "Performing restore operation" << std::endl;
        while (true) {
            char* read_block = mmaped_data + offs;
            char* write_block = read_block - search_offs;
            // perform copy
            std::memcpy(write_block, read_block, block_size);

            ctr++;
            if (ctr % 64 == 0) {
                std::cout << "Copied " << ctr / 64 << "GB" << std::endl;
            }

            offs += block_size;
            if (offs + block_size > mmap_size) {
                break;
            }
        }
    }
    std::cout << "Finished restoring" << std::endl;
    munmap(mmaped_data, mmap_size);

    return 0;
}

