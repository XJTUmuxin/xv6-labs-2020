// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct entry {
  uint64 key;
  int value;
  struct entry *next;
};
struct entry *table[NBUCKET];
struct entry entrys[NBUF];
struct spinlock locks[NBUCKET];

struct {
  struct spinlock eviction_lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.eviction_lock, "bcache");

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->lastuse = 0;
    b->refcnt = 0;
  }
  for(int i=0;i<NBUCKET;++i){
    initlock(&locks[i],"bcache.bucket");
  }

  // put all the buf in the table[0]
  for(int i=0;i<NBUF;++i){
    struct entry *e = &entrys[i];
    e->key = 0;
    e->value = i;
    e->next = table[0];
    table[0] = e;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // Is the block already cached?
  uint64 dev_blockno = ((uint64)dev<<32)+blockno;
  struct entry* e;

  int i = dev_blockno % NBUCKET;

  acquire(&locks[i]);
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == dev_blockno) break;
  }

  if(e){
    b = &bcache.buf[e->value];
    b->refcnt++;
    release(&locks[i]);
    acquiresleep(&b->lock);
    return b;
  }

  // Not cached.

  release(&locks[i]);

  acquire(&bcache.eviction_lock);

  // check again
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == dev_blockno) break;
  }

  if(e){
    b = &bcache.buf[e->value];
    acquire(&locks[i]);
    b->refcnt++;
    release(&locks[i]);
    release(&bcache.eviction_lock);
    acquiresleep(&b->lock);
    return b;
  }

  // still not cached, find a new buf 

  uint min_lastuse = __UINT32_MAX__;
  struct entry *pre=0,*curr=0,*before_e=0;
  int holding_lock = -1;
  int is_head = 0;
  for(int bucket=0;bucket<NBUCKET;++bucket){
    int new_found = 0;
    acquire(&locks[bucket]);
    if(table[bucket]){
      if((bcache.buf[table[bucket]->value].refcnt == 0) && (bcache.buf[table[bucket]->value].lastuse<min_lastuse)
  ){
        e = table[bucket];
        min_lastuse = bcache.buf[table[bucket]->value].lastuse;
        is_head = 1;
        new_found = 1;
      }
      pre = table[bucket];
      curr = pre->next;
      while(curr){
        if(bcache.buf[curr->value].refcnt==0 && bcache.buf[curr->value].lastuse<min_lastuse){
          e = curr;
          before_e = pre;
          min_lastuse = bcache.buf[curr->value].lastuse;
          is_head = 0;
          new_found = 1;
        }
        pre = curr;
        curr = pre->next;
      }
    }
    if(new_found){
      if(holding_lock!=-1){
        release(&locks[holding_lock]);
      }
      holding_lock = bucket;
    }
    else{
      release(&locks[bucket]);
    }
  }

  // find a buf
  if(e){
    if(holding_lock != i){
      // remove entry
      if(is_head){
        table[holding_lock] = e->next;
      }
      else{
        before_e->next = e->next;
      }
      release(&locks[holding_lock]);
      acquire(&locks[i]);
      e->next = table[i];
      table[i] = e;
    }
    e->key = dev_blockno;
    b = &bcache.buf[e->value];
    b->dev = dev;
    b->blockno = blockno;
    b->refcnt++;
    b->valid = 0;
    release(&locks[i]);
    release(&bcache.eviction_lock);
    acquiresleep(&b->lock);
    return b; 
  }
  //
  else{
    panic("bget: no buf");
  }
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint64 dev_blockno = ((uint64)(b->dev)<<32)+(b->blockno);

  int i = dev_blockno % NBUCKET;
  acquire(&locks[i]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->lastuse = ticks;
  }
  
  release(&locks[i]);
}

void
bpin(struct buf *b) {
  uint64 dev_blockno = ((uint64)(b->dev)<<32)+(b->blockno);

  int i = dev_blockno % NBUCKET;
  acquire(&locks[i]);
  b->refcnt++;
  release(&locks[i]);
}

void
bunpin(struct buf *b) {
  uint64 dev_blockno = ((uint64)(b->dev)<<32)+(b->blockno);

  int i = dev_blockno % NBUCKET;
  acquire(&locks[i]);
  b->refcnt--;
  release(&locks[i]);
}


