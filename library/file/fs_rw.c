#include "fs.h"
#include "string.h"
#include "stdlib.h"

/* bit manipulation
 * see implementation in fs.c */
int bit_test(unsigned char *map, int i);

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* disk read/write
 * see implementation in fs.c */
int block_read(int block_no, void* dst);
int block_write(int block_no, void* src);

int block_writing = -1;

/* block allocate and free
 * see implementation in fs.c */
int alloc_block();
void free_block(int blk_id);
unsigned char buffer_cache2[BLOCK_SIZE];
/* global fs_t instance */
fs_t fs;



// [lab6-ex1] initialize fs
// This function reads superblock and initialize the global fs variable "fs".
//  -- Read the definition of "fs_t" and "super_t" in "fs.h"
//  -- you should read superblock from disk by "block_read"
//  -- you should check the magic number; if not match, FATAL
//  -- you should update the "total_blk"
//  -- you should update the "avail_blks" by counting empty blocks in bitmap
//     (bit_test is a helper function to count)
void fs_init(m_uint32 super_blk_id) {
    // TODO: your code here
    
    struct fs_super superblock;
    block_read(super_blk_id, &superblock);

    // Step 2: Check the magic number
    if (superblock.magic != 0x6640) {
        FATAL("Magic number mismatch. Invalid file system.");
    } else {
       
        fs.total_blks = superblock.disk_size;

        block_read(1,fs.bitmap);

        fs.avail_blks = 0;
        int i;
        for (i = 0; i < fs.total_blks; ++i) {
            if (!bit_test(fs.bitmap, i)) {
                fs.avail_blks++;
            }
       
        }
        
    }

   // FATAL("fs_init is not implemented");




}

// [lab6-ex2]
// This function reads inodes from disk.
// You should load a block from the inode arrary on disk
// which contains the inode "ino":
//   -- calculate which block contains inode "ino"
//      (note: "ino" starts from FS_ROOT_INODE)
//   -- load the block to fs.buffer_cache
//      (note: inode array starts at block INODEARR_BLOCK_START)
//   -- return the inode pointer (pointing in fs.buffer_cache)
inode_t *load_inode(int ino) {
    // TODO: your code here

 

    int inode_arr_idx = (ino - FS_ROOT_INODE);

    // printf("reading %d\n",inode_arr_idx + 2);

    int block_number = inode_arr_idx / 8 + INODEARR_BLOCK_START;


    block_read(block_number , &fs.buffer_cache);

    block_writing = block_number;
    
    
    int inode_index_within_block = inode_arr_idx % 8;

    // printf("adding this %d\n",inode_index_within_block);

    inode_t *inode = &((inode_t *)fs.buffer_cache)[inode_index_within_block];

    //inode_t * inode = fs.buffer_cache + (64 * inode_index_within_block) ;
    

    return inode;
}

// [lab6-ex2]
// flush the contents of "fs.buffer_cache" (a list of inodes) back to disk
// notes:
//   -- you need to remember the block id of the cached inodes
//   -- later when you update files, remember to flush inodes when things change
void flush_inode() {
    // TODO: your code here
    block_write(block_writing, &fs.buffer_cache);

}


// [lab6-ex2]
// read "len" bytes of contents starting from "offset"
// in the file whose inode number is "ino".
// You should:
//   -- load the inode data structure
//   -- calculate which blocks (directed or indirected) contain the wanted data
//   -- copy the data to "buf"
//   -- return 0 if success; otherwise, return -1
// notes:
//   -- assume "buf" is the size of "len"
//   -- for simplicity, "len" will not exceed 1 block
int fs_read(int ino, int offset, int len, char *buf) {
    ASSERTX(len <= BLOCK_SIZE);

    // TODO: your code here
    // printf("inode is %d\n",ino);
    // printf("OFFSET %d\n",offset);
    // printf("LEN %d\n",len);
    inode_t *inode = load_inode(ino);
  
    // printf("INODE MODE %d\n",inode->mode);

    //printf("block reading %d\n",inode->ptrs[0]);

    int start_block_no = offset / BLOCK_SIZE;

    int end_block_no = (offset + len - 1) / BLOCK_SIZE;


    unsigned char start_block[BLOCK_SIZE]; 
    unsigned char end_block[BLOCK_SIZE]; 
    if(start_block_no > 9){
        unsigned char indirect_block[BLOCK_SIZE];
        block_read(inode->indirect_ptr,&indirect_block);
        m_uint32 *indirect_ptrs = indirect_block;
        block_read(indirect_ptrs[start_block_no-10],&start_block );
    } else {
        block_read(inode->ptrs[start_block_no],&start_block );
    }

    int start_offset_in_block = offset % BLOCK_SIZE;

    int bytes_to_copy_from_start_block = MIN(BLOCK_SIZE - start_offset_in_block, len);
    memcpy(buf, start_block + start_offset_in_block, bytes_to_copy_from_start_block);

    if(start_block_no < end_block_no){
        if(end_block_no > 9){
            unsigned char indirect_block[BLOCK_SIZE];
            block_read(inode->indirect_ptr,&indirect_block);
            m_uint32 *indirect_ptrs = indirect_block;
            block_read(indirect_ptrs[end_block_no-10],&end_block );
        } else {
            block_read(inode->ptrs[end_block_no],&end_block );
        }

        int end_offset_in_block = len-bytes_to_copy_from_start_block;
        memcpy(buf + bytes_to_copy_from_start_block, end_block, end_offset_in_block);

    }
  

    return 0;
}


// [lab6-ex3]
// return the size of the file "ino"
// (note: check "inode_t" in fs.h)
int fs_fsize(int ino) {
    // TODO: your code here

    inode_t *inode = load_inode(ino);

    return inode->size;
    
}


// [lab6-ex3]
// write "len" bytes of data to the file ("ino") starting from "offset"
// notes:
//   -- use "alloc_block" to allocate a block if needed
//   -- remember to update file size
//   -- remember to flush the inode
//   -- remember to flush the modified blocks
//   -- return 0 if success; otherwise, return -1
int fs_write(int ino, int offset, int len, const char *buf) {
    // TODO: your code here

    inode_t *inode = load_inode(ino);

    int start_block_no = offset / BLOCK_SIZE;
    int end_block_no = (offset + len - 1) / BLOCK_SIZE;

    inode->size = (offset + len) > inode->size ? offset+len : inode->size;
    flush_inode();


    unsigned char start_block[BLOCK_SIZE]; 
    unsigned char end_block[BLOCK_SIZE]; 

    if(start_block_no > 9){
        if(inode->indirect_ptr <= 0){
            //alloc indirect bloc
            inode->indirect_ptr = alloc_block();
            flush_inode();

            unsigned char indirect_block[BLOCK_SIZE] = {0};
            block_write(inode->indirect_ptr, &indirect_block);
        }
        //read indirect block
        unsigned char indirect_block[BLOCK_SIZE];
        block_read(inode->indirect_ptr,&indirect_block);
        m_uint32 *indirect_ptrs = indirect_block;

        //find ptr in indirect block
        int block_ptr = indirect_ptrs[start_block_no-10];
        if(block_ptr <= 0){
            indirect_ptrs[start_block_no-10] = alloc_block();
            block_write(inode->indirect_ptr,indirect_ptrs);
        }

        block_read(indirect_ptrs[start_block_no-10],&start_block);
    } else {
       // printf("here\n");
        if(inode->ptrs[start_block_no] <= 0){
            inode->ptrs[start_block_no] = alloc_block();
            flush_inode();
      
        }
     
        block_read(inode->ptrs[start_block_no],&start_block );
    }

    int start_offset_in_block = offset % BLOCK_SIZE;

    int bytes_to_copy_from_start_block = MIN(BLOCK_SIZE - start_offset_in_block, len);

    memcpy(start_block + start_offset_in_block, buf, bytes_to_copy_from_start_block);

    if (start_block_no > 9) {
        unsigned char indirect_block[BLOCK_SIZE];
        block_read(inode->indirect_ptr, &indirect_block);
        m_uint32 *indirect_ptrs = (m_uint32 *)indirect_block;
        block_write(indirect_ptrs[start_block_no - 10], &start_block);
        block_write(inode->indirect_ptr, &indirect_block);  // Ensure the indirect block is written back
    } else {
        block_write(inode->ptrs[start_block_no], &start_block);
    }

   

    if(start_block_no < end_block_no){
        if(end_block_no > 9){
            if(inode->indirect_ptr <= 0){
               
               
                inode->indirect_ptr = alloc_block();
                flush_inode();

                unsigned char indirect_block[BLOCK_SIZE] = {0};
                block_write(inode->indirect_ptr, &indirect_block);
              
            }
            //read indirect block
            unsigned char indirect_block[BLOCK_SIZE];
            block_read(inode->indirect_ptr,&indirect_block);
            m_uint32 *indirect_ptrs = indirect_block;

            //find ptr in indirect block
            m_uint32 block_ptr = indirect_ptrs[end_block_no-10];

         
            
            if(block_ptr <= 0){
               
                indirect_ptrs[end_block_no-10] = alloc_block();
                block_write(inode->indirect_ptr,indirect_block);
                
            }

            block_read(indirect_ptrs[end_block_no-10],&end_block);
           
        } else {
            if(inode->ptrs[end_block_no] <= 0){
                inode->ptrs[end_block_no] = alloc_block();
                flush_inode();
            }
            block_read(inode->ptrs[end_block_no],&end_block );
        }

        int end_offset_in_block = len-bytes_to_copy_from_start_block;
        memcpy(end_block,buf + bytes_to_copy_from_start_block,  end_offset_in_block);

        // Write the modified start block back to disk
        if (end_block_no > 9) {
            unsigned char indirect_block[BLOCK_SIZE];
            block_read(inode->indirect_ptr, &indirect_block);
            m_uint32 *indirect_ptrs = (m_uint32 *)indirect_block;
            block_write(indirect_ptrs[end_block_no - 10], &end_block);
            block_write(inode->indirect_ptr, &indirect_block);  // Ensure the indirect block is written back
        } else {
         block_write(inode->ptrs[end_block_no], &end_block);
        }

    }

    

    return 0;
}

