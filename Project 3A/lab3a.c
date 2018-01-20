#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "ext2_fs.h"

unsigned int blockSize;
struct ext2_inode* iNodes, *dirNodes;
int* validNodes, *validDirNodes;
int diskfd, outfd = STDOUT_FILENO;
int nodeCount = 0, dirCount = 0;

void directory (int offset, int i) {
	unsigned int byteCount = blockSize * offset;
	int count = 0;
	while(byteCount < blockSize*(offset+1)){
		int temp;
		pread(diskfd, &temp, 4, byteCount);
		if (temp == 0){
			pread(diskfd, &temp, 2, byteCount + 4);
			byteCount += temp;
			count += temp;
			continue;
		}
		struct ext2_dir_entry currDir;
		pread(diskfd, &currDir, 263, byteCount);
		char* name = malloc((currDir.name_len+1)*sizeof(char));
		pread(diskfd, name, currDir.name_len, byteCount + 8);
		name[currDir.name_len] = '\0';
		dprintf(outfd, "DIRENT,%d,%d,%d,%d,%d,\'%s\'\n", validDirNodes[i]+1, count, currDir.inode, currDir.rec_len, currDir.name_len, name);
		byteCount += currDir.rec_len;
		count += currDir.rec_len;
	}
}

void indDirectory(int offset, int count, int i){
	int temp;
	if (offset != 0){
		for(unsigned int j = 0; j < blockSize/4; j++){
			pread(diskfd, &temp, 4, offset*blockSize + j*4);
			if(count == 0){
				if(temp != 0) directory(offset, i);
			}
			else
				indDirectory(temp, count-1, i);
		}
	}
}

void indirect(int offset, int count, int i, int logic){
	int temp;
	if (offset != 0){
		for(unsigned int j = 0; j < blockSize/4; j++){
			pread(diskfd, &temp, 4, offset*blockSize + j*4);
			if (temp != 0){
				dprintf(outfd,"INDIRECT,%d,%d,%d,%d,%d\n", validNodes[i]+1, count, logic + (int)(j*pow(256,count-1)), offset, temp);
				if (count != 1)
					indirect(temp, count-1, i, logic + (int)(j*pow(256,count-1)));
			}
		}
	}
}

int main(int argc, char **argv){	
	if (argc != 2){
		fprintf(stderr,"ERROR: Wrong Number of Arguments");
		exit(1);
	}
	if ((diskfd = open(argv[1], O_RDONLY)) < 0) {
        fprintf(stderr,"ERROR: %s\n", strerror(errno));
        exit(1);
    }

    struct ext2_super_block supBlock;
    pread(diskfd, &supBlock, 1024, 1024);
    blockSize = 1024 << supBlock.s_log_block_size;
    dprintf(outfd, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",supBlock.s_blocks_count, supBlock.s_inodes_count, blockSize, supBlock.s_inode_size, 
    	supBlock.s_blocks_per_group, supBlock.s_inodes_per_group, supBlock.s_first_ino);
    
    struct ext2_group_desc group;
    pread(diskfd, &group, 32, 2048);
   	dprintf(outfd, "GROUP,0,%d,%d,%d,%d,%d,%d,%d\n", supBlock.s_blocks_count, supBlock.s_inodes_count, group.bg_free_blocks_count, 
   		group.bg_free_inodes_count, group.bg_block_bitmap, group.bg_inode_bitmap, group.bg_inode_table);

   	validNodes = malloc(supBlock.s_inodes_count*sizeof(int));
   	for(unsigned int i = 0; i < blockSize; i++) {
	   	int tempB = 0, tempI = 0, ones = 1;
	   	pread(diskfd, &tempB, 1, group.bg_block_bitmap*blockSize+i); 
	   	pread(diskfd, &tempI, 1, group.bg_inode_bitmap*blockSize+i); 
	   	for (unsigned int j = 0; j < 8; j++) {
	   		if ((tempB & ones) == 0)
	   			dprintf(outfd,"BFREE,%d\n", i*8 + j+1);
	   		if ((tempI&ones) == 0)
	   			dprintf(outfd,"IFREE,%d\n", i*8 + j+1);
	   		else if(((tempI & ones) != 0) && (i*8+j < supBlock.s_inodes_per_group)){
	   			validNodes[nodeCount] = i*8+j+1;
	   			nodeCount++;
	   		}
	   		ones = ones << 1;
	   	}
	}

	iNodes = malloc(supBlock.s_inodes_count*sizeof(struct ext2_inode));
	dirNodes = malloc(nodeCount*sizeof(struct ext2_inode));
	validDirNodes = malloc(nodeCount*sizeof(int));
	for(int i = 0; i < nodeCount; i++){
		int k = validNodes[i];
		pread(diskfd,&iNodes[i], 128, group.bg_inode_table*blockSize + k*128);

		if(iNodes[i].i_mode == 0 && iNodes[i].i_links_count == 0)
			continue;
		char cbuff[20],mbuff[20],abuff[20];
		time_t temp = iNodes[i].i_ctime;
		struct tm* ctime = gmtime(&temp);
		strftime(cbuff,18,"%D %H:%M:%S",ctime);
		temp = iNodes[i].i_mtime;
		struct tm* mtime = gmtime(&temp);
		strftime(mbuff,18,"%D %H:%M:%S",mtime);
		temp = iNodes[i].i_atime;
		struct tm* atime = gmtime(&temp);
		strftime(abuff,18,"%D %H:%M:%S",atime);
		
		char c;
		if(iNodes[i].i_mode&0x4000) {
			c = 'd';
			pread(diskfd,&dirNodes[dirCount], 128, group.bg_inode_table*blockSize + k*128);
			validDirNodes[dirCount] = k;
			dirCount++;
		}
		else if(iNodes[i].i_mode&0x8000) c = 'f';
		else if(iNodes[i].i_mode&0xA000) c = 's';
		else c = '?';

		long tempSize = (((long)iNodes[i].i_dir_acl) << 32) | iNodes[i].i_size;
		dprintf(outfd,"INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%ld,%d", k+1, c, iNodes[i].i_mode & 0x0FFF, iNodes[i].i_uid, iNodes[i].i_gid, iNodes[i].i_links_count, cbuff, 
			mbuff, abuff, tempSize, iNodes[i].i_blocks);
		for (int j = 0; j < 15; j++)
			dprintf(outfd,",%d",iNodes[i].i_block[j]);
		dprintf(outfd,"\n");
	}

	for(int i = 0; i < dirCount; i++){
		for(int j = 0; j < 12; j++){
			int offset = dirNodes[i].i_block[j];
			if(offset != 0) directory(offset, i);
		}
		indDirectory(dirNodes[i].i_block[12], 0, i);
		indDirectory(dirNodes[i].i_block[13], 1, i);
		indDirectory(dirNodes[i].i_block[14], 2, i);
	}

	for(int i = 0; i < nodeCount; i++){
		indirect(iNodes[i].i_block[12],1,i,12);
		indirect(iNodes[i].i_block[13],2,i,268);
		indirect(iNodes[i].i_block[14],3,i,65804);
	}
}