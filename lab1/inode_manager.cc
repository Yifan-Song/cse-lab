#include "inode_manager.h"
// disk layer -----------------------------------------
int current_time;
disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  if(id < BLOCK_NUM && buf){
    memcpy(buf, blocks[id], BLOCK_SIZE);
    //*buf = *blocks[id];
  }
}

void
disk::write_block(blockid_t id, const char *buf)
{
  if(id < BLOCK_NUM && buf){
    memcpy(blocks[id], buf, BLOCK_SIZE);
    //*blocks[id] = *buf;
  }
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
  for(int i = 1+INODE_NUM; i < BLOCK_NUM+1; i++){
    if(using_blocks.find(i) == using_blocks.end()){
      printf("\tim: alloc_block %d\n", i);
      using_blocks[i] = 1;
      return i;
    }
  }
  return 0;
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  if(using_blocks.find(id) != using_blocks.end()){
    using_blocks.erase(id);
    printf("\tim: free_block %d\n", id);
  }
  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
  inode_t new_inode;
  new_inode.type = type;
  new_inode.size = 0;
  new_inode.ctime = current_time;
  new_inode.atime = current_time;
  new_inode.mtime = current_time;
  current_time ++;
  for(int i = 1;i < INODE_NUM+1;i++){
    if(get_inode(i) == NULL){
      put_inode(i,&new_inode);
      printf("\tim: alloc_inode %d\n", i);
      return i;
    }
  }
  return 0;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
  inode_t* current_inode = get_inode(inum);
  if (current_inode == NULL){
    return;
  }
  printf("\tim: free_inode %d\n", inum);
  current_inode->type = 0;
  current_inode->ctime = 0;
  current_inode->atime = 0;
  current_inode->mtime = 0;
  put_inode(inum, current_inode);
  return;
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;
  ino->ctime = current_time;
  current_time ++;
  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

int blockNumOfSize(int size){
  if(size <= 0){
    return 0;
  }
  int block_num = size / BLOCK_SIZE;
  if((block_num * BLOCK_SIZE) < size){
    block_num ++;
  }
  return block_num;
}

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_Out
   */
  inode_t* current_inode = get_inode(inum);
  if(current_inode == NULL){
    return;
  }
  printf("\tim: read_file %d\n", inum);
  int block_num = blockNumOfSize(current_inode->size);
  char *data = (char *)malloc(BLOCK_SIZE * block_num);
  //int indirectBlock[NINDIRECT];
  if(block_num <= NDIRECT){
    for(int i = 0; i < block_num; i++){
      bm->read_block(current_inode->blocks[i], data + i*BLOCK_SIZE);
    }
  }
  else{
    for(int i = 0; i < NDIRECT; i++){
      bm->read_block(current_inode->blocks[i], data + i*BLOCK_SIZE);
    }
    int indirect_block_num = current_inode->blocks[NDIRECT];
    int indirect_block[NINDIRECT];
    bm->read_block(indirect_block_num, (char *)indirect_block);
    for(int i = NDIRECT; i < block_num; i++){
      int index = i - NDIRECT;
      bm->read_block(indirect_block[index], data + i*BLOCK_SIZE);
    }
  }
  *buf_out = data;
  *size = current_inode->size;
  return;
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */
  inode_t* current_inode = get_inode(inum);
  if(current_inode == NULL){
    return;
  }
  printf("\tim: write_file %d\n", inum);
  int newBlockNum = blockNumOfSize(size);
  int oldBlockNum = blockNumOfSize(current_inode->size);
  if(newBlockNum == 0 && newBlockNum > MAXFILE){
    fprintf(stderr, "data size error\n");
    return;
  }
  if(newBlockNum >= oldBlockNum){
    if(newBlockNum <= NDIRECT){
      for(int i = 0; i < oldBlockNum; i++){
	bm->write_block(current_inode->blocks[i], buf + i*BLOCK_SIZE);
      }
      for(int i = oldBlockNum; i < newBlockNum; i++){
	int new_block = bm->alloc_block();
	current_inode->blocks[i] = new_block;
	bm->write_block(new_block, buf + i*BLOCK_SIZE);
      }
    }
    else{
      if(oldBlockNum > NDIRECT){
	for(int i = 0; i < NDIRECT; i++){
          bm->write_block(current_inode->blocks[i], buf + i*BLOCK_SIZE);
        }
	int indirect_block_num = current_inode->blocks[NDIRECT];
        int indirect_block[NINDIRECT];
        bm->read_block(indirect_block_num, (char *)indirect_block);
        for(int i = NDIRECT; i < oldBlockNum; i++){
	  int index = i - NDIRECT;
          bm->write_block(indirect_block[index], buf + i*BLOCK_SIZE);
	}
        for(int i = oldBlockNum; i < newBlockNum; i++){
	  int new_block = bm->alloc_block();
	  indirect_block[i-NDIRECT] = new_block;
	  bm->write_block(new_block, buf + i*BLOCK_SIZE);
	}
	bm->write_block(indirect_block_num, (char *)indirect_block);
      }
      else{
        for(int i = 0; i < oldBlockNum; i++){
          bm->write_block(current_inode->blocks[i], buf + i*BLOCK_SIZE);
        }
	for(int i = oldBlockNum; i < NDIRECT; i++){
          int new_block = bm->alloc_block();
          current_inode->blocks[i] = new_block;
          bm->write_block(new_block, buf + i*BLOCK_SIZE);
        }
	int indirect_block_num = bm->alloc_block();
	current_inode->blocks[NDIRECT] = indirect_block_num;
	int indirect_block[NINDIRECT];
	for(int i = NDIRECT; i < newBlockNum; i++){
	  int new_block = bm->alloc_block();
          indirect_block[i-NDIRECT] = new_block;
          bm->write_block(new_block, buf + i*BLOCK_SIZE);
	}
	bm->write_block(indirect_block_num, (char *)indirect_block);
      }
    }
  }
  else{
    if(oldBlockNum <= NDIRECT){
      for(int i = 0; i < newBlockNum; i++){
        bm->write_block(current_inode->blocks[i], buf + i*BLOCK_SIZE);
      }
      for(int i = newBlockNum; i < oldBlockNum; i++){
        bm->free_block(current_inode->blocks[i]);
	current_inode->blocks[i] = 0;
      }
    }
    else{
      if(newBlockNum <= NDIRECT){
	for(int i = 0; i < newBlockNum; i++){
          bm->write_block(current_inode->blocks[i], buf + i*BLOCK_SIZE);
        }
	for(int i = newBlockNum; i < NDIRECT; i++){
	  bm->free_block(current_inode->blocks[i]);
          current_inode->blocks[i] = 0;
	}
	int indirect_block_num = current_inode->blocks[NDIRECT];
        int indirect_block[NINDIRECT];
        bm->read_block(indirect_block_num, (char *)indirect_block);
	for(int i = NDIRECT; i < oldBlockNum; i++){
	  bm->free_block(indirect_block[i-NDIRECT]);
	  indirect_block[i-NDIRECT] = 0;
	}
	bm->free_block(current_inode->blocks[NDIRECT]);
	current_inode->blocks[NDIRECT] = 0;
      }
      else{
	for(int i = 0; i < NDIRECT; i++){
          bm->write_block(current_inode->blocks[i], buf + i*BLOCK_SIZE);
        }
	int indirect_block_num = current_inode->blocks[NDIRECT];
        int indirect_block[NINDIRECT];
        bm->read_block(indirect_block_num, (char *)indirect_block);
	for(int i = NDIRECT; i < newBlockNum; i++){
          int index = i - NDIRECT;
          bm->write_block(indirect_block[index], buf + i*BLOCK_SIZE);
        }
	for(int i = newBlockNum; i < oldBlockNum; i++){
	  bm->free_block(indirect_block[i-NDIRECT]);
	  indirect_block[i-NDIRECT] = 0;
	}
      }
    }
  }
  current_inode->mtime = current_time;
  current_inode->atime = current_time;
  current_time ++;
  current_inode->size = size;
  put_inode(inum, current_inode);
  return;
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  inode_t* current_node = get_inode(inum);
  if(current_node == NULL){
    return;
  }
  a.size = current_node->size;
  a.type = current_node->type;
  a.atime = current_node->atime;
  a.mtime = current_node->mtime;
  a.ctime = current_node->ctime;
  return;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  inode_t* current_inode = get_inode(inum);
  if(current_inode == NULL){
    return;
  }
  int blockNum = blockNumOfSize(current_inode->size);
  if(blockNum <= NDIRECT){
    for(int i = 0; i < blockNum; i++){
      bm->free_block(current_inode->blocks[i]);
    }
  }
  else{
    for(int i = 0; i < NDIRECT; i++){
      bm->free_block(current_inode->blocks[i]);
    }
    int indirect_block_num = current_inode->blocks[NDIRECT];
    int indirect_block[NINDIRECT];
    bm->read_block(indirect_block_num, (char *)indirect_block);
    for(int i = NDIRECT; i < blockNum; i++){
      bm->free_block(indirect_block[i-NDIRECT]);
    }
    bm->free_block(current_inode->blocks[NDIRECT]);
  }
  free_inode(inum);
  return;
}
