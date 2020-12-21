#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ext2_fs.h"
#include <math.h>
#include <time.h>
#define EXT2_S_IFLNK 0xA000
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFDIR 0x4000

int fd = -1;
struct ext2_super_block sblock;
struct ext2_group_desc group_desc;
struct ext2_inode inode_table;
unsigned int blocksize = 0;
unsigned int inodesize = 0;

void recurse_inode(int orig_inode, int level, int* block_ptr, int curr_block_num, int logical_counter);
//void recurse_inode(int orig_inode, int level, int* block_ptr);

int main(int argc, char** argv){
    //check valid args
    if(argc != 2){
        fprintf(stderr, "Invalid args: ./lab3a fsimage.img\n");
        exit(1);
    }

    fd = open(argv[1], O_RDONLY);
    if(fd < 0){
        fprintf(stderr, "Error opening specified file\n");
        exit(1);
    }

    //SUPERBLOCK SUMMARY

    //read superblock
    if(pread(fd, &sblock, sizeof(sblock), 1024)==-1){
        fprintf(stderr, "pread error\n");
        exit(2);
    }

    //verify superblock
    if(sblock.s_magic!=EXT2_SUPER_MAGIC){
        fprintf(stderr, "Error verifying superblock\n");
        exit(2);
    }

    //get block size
    blocksize = EXT2_MIN_BLOCK_SIZE << sblock.s_log_block_size;
    
    //print out superblock info
    int nblocks = sblock.s_blocks_count;
    int ninodes = sblock.s_inodes_count;
    int sinode = sblock.s_inode_size;
    int gblocks = sblock.s_blocks_per_group;
    int ginodes = sblock.s_inodes_per_group;
    int nonres = sblock.s_first_ino;
    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", nblocks, ninodes, blocksize, sinode, gblocks, ginodes, nonres);
    //GROUP SUMMARY
    //int groupsize = gblocks * blocksize;
    if(pread(fd, &group_desc, sizeof(group_desc), 1024+blocksize)==-1){
        fprintf(stderr, "Error reading in group\n");
        exit(2);
    }
    //group number = 0
    //total number of blocks in group = gblocks    
    int num_free_blocks = group_desc.bg_free_blocks_count;

    int num_free_inodes = group_desc.bg_free_inodes_count;

    int block_bitmap = group_desc.bg_block_bitmap;

    int inode_bitmap = group_desc.bg_inode_bitmap;

    int first_inodes = group_desc.bg_inode_table;



    //print out group info
    printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", 0, nblocks, ginodes, num_free_blocks, num_free_inodes, block_bitmap, inode_bitmap, first_inodes);

    // FREE BLOCK ENTRIES
    char* arr = (char*)malloc(blocksize * sizeof(char));

    if(pread(fd, arr, blocksize, blocksize*block_bitmap)==-1){
        fprintf(stderr, "Error reading in bitmap\n");
        exit(2);
    }
    int numfree = 0;
    //we have gblocks blocks in this group                                                                                             
    int numbytes = (nblocks/8) + 1;
    int index = 1;
    for(int i=0; i<numbytes; i++){
        char curbyte = arr[i];
        for(int j=0; j<8; j++){
            if(index > nblocks){
                break;
            }
            char shift = curbyte >> j;
            int bool = shift&1;
            if(bool == 0){
                //block is free                                                                                                        
                numfree+=1;
                printf("BFREE,%d\n", index);
            }
            index+=1;
        }
    }
/*    
    int block_bitmap_buffer[blocksize/4];
    if(pread(fd, block_bitmap_buffer, blocksize, (blocksize*block_bitmap))==-1){
        fprintf(stderr, "Error reading in group\n");
        exit(2);
    }
    for(int i = 0; i < nblocks; i++){
	int curr_bit = 0x01 & (block_bitmap_buffer[i/32] >> (i%32));
	if(curr_bit == 0x0){
	    printf("BFREE,%i\n",i);
	}
    }
*/
    // FREE INODE ENTRIES
    char* arr_inode = (char*)malloc(blocksize * sizeof(char));

    if(pread(fd, arr_inode, blocksize, blocksize*inode_bitmap)==-1){
        fprintf(stderr, "Error reading in bitmap\n");
        exit(2);
    }
    numfree = 0;
    //we have gblocks blocks in this group                                                                                 
    numbytes = (nblocks/8) + 1;
    index = 1;
    for(int i=0; i<numbytes; i++){
        char curbyte = arr_inode[i];
        for(int j=0; j<8; j++){
            if(index > nblocks){
                break;
            }
            char shift = curbyte >> j;
            int bool = shift&1;
            if(bool == 0){
                //block is free                                                                                                        
                numfree+=1;
                printf("IFREE,%d\n", index);
            }
            index+=1;
        }
    }
/*
    int inode_bitmap_buffer[blocksize/4];
    if(pread(fd, inode_bitmap_buffer, blocksize, (blocksize*inode_bitmap))==-1){
        fprintf(stderr, "Error reading in group\n");
        exit(2);
    }
    for(int i = 0; i < nblocks; i++){
	int curr_bit = 0x01 & (inode_bitmap_buffer[i/32] >> (i%32));
	if(curr_bit == 0x0){
	    printf("IFREE,%i\n",i);
	}
    }
*/  

    //INODE SUMMARY
    for(int i = 0; i<ninodes; i++){
	if(pread(fd, &inode_table, sizeof(inode_table), blocksize*first_inodes+(sizeof(inode_table)*i))==-1){
	    fprintf(stderr, "Error reading in group\n");
	    exit(2);
	}
	if(inode_table.i_links_count == 0 || inode_table.i_mode == 0){
	    continue;
	}
	char filetype = '?';
	int mode = inode_table.i_mode;
	if((mode&S_IFLNK) == S_IFLNK)
	    filetype = 's';
	else if((mode&S_IFREG) == S_IFREG)
	    filetype = 'f';
	else if((mode&S_IFDIR) == S_IFDIR)
	    filetype = 'd';

	char abuff[100];
	char cbuff[100];
	char mbuff[100];

	struct tm *atime;
	struct tm *ctime;
	struct tm *mtime;
	time_t seconds = inode_table.i_mtime;
	mtime = gmtime(&seconds);
	strftime(mbuff, 100, "%m/%d/%y %H:%M:%S", mtime);
	
	seconds = inode_table.i_atime;
	atime = gmtime(&seconds);
	strftime(abuff, 100, "%m/%d/%y %H:%M:%S", atime);
	
	seconds = inode_table.i_ctime;
	ctime = gmtime(&seconds);
	strftime(cbuff, 100, "%m/%d/%y %H:%M:%S", ctime);
	/*
	seconds = inode_table.i_mtime;
	mtime = gmtime(&seconds);
	*/
	


	
	printf("INODE,%d,%c,%o,%d,%d,%d,", i+1,filetype, mode&0xFFF, inode_table.i_uid, inode_table.i_gid, inode_table.i_links_count);
	printf("%s,%s,%s,",cbuff,mbuff,abuff);

	printf("%d,%d",inode_table.i_size, inode_table.i_blocks);
	if(filetype == 'f' || filetype == 'd' || (filetype == 's' && inode_table.i_size >=60))
	    for(int j = 0; j < 15; j++){
		printf(",%d",inode_table.i_block[j]);
	    }
	
	printf("\n");

	// DIRECTORY ENTRIES
	int byte_offset = 0;
	if(filetype == 'd'){	
	    struct ext2_dir_entry dir_entry;
	    if(pread(fd, &dir_entry, sizeof(dir_entry), blocksize*inode_table.i_block[0]) == -1){
		//first_inodes+(sizeof(inode_table)*i))==-1){
		fprintf(stderr, "Error reading in group\n");
		exit(2);
	    }
	    
	    while(dir_entry.inode != 0 && byte_offset < 1024){
		printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",i+1,byte_offset,dir_entry.inode,dir_entry.rec_len,dir_entry.name_len,dir_entry.name);
		byte_offset += dir_entry.rec_len;
		if(pread(fd, &dir_entry, sizeof(dir_entry), blocksize*inode_table.i_block[0]+byte_offset) == -1){
		    //first_inodes+(sizeof(inode_table)*i))==-1){
		    fprintf(stderr, "Error reading in group\n");
		    exit(2);
		}
	    }
	}
	// INDIRECT BLOCK
	//int counter = 0;
	for(int j = 0; j < 3; j++){
	    if(inode_table.i_block[12+j] != 0){
		int block_ptr_low[256];
		//if(j == 1)
		    //counter = 256;
		//if(j == 2)
		//counter = 256*256;
		pread(fd, block_ptr_low, 1024, blocksize*inode_table.i_block[12+j]);
		recurse_inode(i+1, 1+j, block_ptr_low, inode_table.i_block[12+j], j+1);
	    }
	}
    }
}

void recurse_inode(int orig_inode, int level, int* block_ptr, int curr_block_num, int orig_level){
    for(int i = 0; i < 256; i++){
	if(block_ptr[i] != 0){
	    int logical_offset = 12 + i;
	    if(orig_level == 3 && level == 2)
		logical_offset = 256*256 + 12 + 256+ 256*i;
	    if(orig_level == 3 && level == 1)
		logical_offset += 256*256+256;
	    if(orig_level == 2 && level == 1)
		logical_offset += 256;
	    else if(level == 2 && orig_level == 2)
		logical_offset = 12+256+(256*i);

	    if(level == 3)
		logical_offset = 12+256+(256*256)+(256*256*i);
	    
	    //printf("INDIRECT,%d,%d,%d,%d,%d,%d,%d\n",orig_inode,level,logical_offset,curr_block_num,block_ptr[i], orig_level,i);
	    printf("INDIRECT,%d,%d,%d,%d,%d\n",orig_inode,level,logical_offset,curr_block_num,block_ptr[i]);
	    if(level > 1){
		int next_block_ptr[256];
		if(pread(fd, next_block_ptr, 1024, blocksize*block_ptr[i])==-1){
		    fprintf(stderr, "Error reading in group\n");
		    exit(2);
		}
		//if(level == 3)
		//logical_counter += 256*256;
		recurse_inode(orig_inode, level-1, next_block_ptr, block_ptr[i], orig_level);
	    }
	}
    }
}
