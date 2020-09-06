/**
 * @file indirect-arronly.cc
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 *
 * TBD
 *
 * @section DESCRIPTION
 *
 *
 * The class implements a Btree with indirect arrays only.  Each node contains
 * an indirect array.  The design aims to have good search performance and
 * efficient solution for update consistency on NVM memory.  However, the node
 * size is limited to up to 128B.
 */

#include "lbtree.h"

/* ----------------------------------------------------------------- *
 useful structure
 * ----------------------------------------------------------------- */
static int last_slot_in_line[LEAF_KEY_NUM];

static void initUseful(void)
{
    // line 0
    last_slot_in_line[0]= 2;
    last_slot_in_line[1]= 2;
    last_slot_in_line[2]= 2;

    // line 1
    last_slot_in_line[3]= 6;
    last_slot_in_line[4]= 6;
    last_slot_in_line[5]= 6;
    last_slot_in_line[6]= 6;

    // line 2
    last_slot_in_line[7]= 10;
    last_slot_in_line[8]= 10;
    last_slot_in_line[9]= 10;
    last_slot_in_line[10]=10;

    // line 3
    last_slot_in_line[11]=13;
    last_slot_in_line[12]=13;
    last_slot_in_line[13]=13;
}

/* ----------------------------------------------------------------- *
 bulk load
 * ----------------------------------------------------------------- */

/* generate a btree on the input keys.
 After generating the btree, level is returned.
 leaf and non-leaf nodes will be bfill full.
 bfill should be between 0 and 1.
 */


/**
 * build a subtree containing: input[start_key, start_key+num_key-1]
 * stop at target_level  (leaf is at level 0)
 *
 * @param input          keyInput instance 
 * @param start_key      keyInput index for the first key
 * @param num_key        number of keys in the subtree
 * @param bfill          filling factor in (0.0,1.0] 
 * @param target_level   stop buidling the subtree at target level
 * @param pfirst         return pointers to the first node at each level
 * @param n_nodes        return number of nodes at each level 
 *
 * @retval the top level of the subtree (can be smaller than target_level
 * if the root level is smaller target_level)
 */
int lbtree::bulkloadSubtree(
    keyInput *input, int start_key, int num_key, 
    float bfill, int target_level,
    Pointer8B pfirst[], int n_nodes[])
{
    // We assume that a tree cannot be higher than 32 levels
    int ncur[32];     // current node at every level
    int top_level;    // top_level is the top level that this method builds

    assert(start_key>=0 && num_key > 0 && bfill>0.0 && bfill<=1.0
           && target_level>=0);


    // 1. compute leaf and nonleaf number of keys
    int leaf_fill_num= (int)((float)LEAF_KEY_NUM * bfill);
    leaf_fill_num= max(leaf_fill_num, 1);

    int nonleaf_fill_num= (int)((float)NON_LEAF_KEY_NUM * bfill);
    nonleaf_fill_num= max(nonleaf_fill_num, 1);


    // 2. compute number of nodes
    n_nodes[0]= ceiling(num_key, leaf_fill_num);
    top_level= 0;

    for (int i=1; n_nodes[i-1]>1 && i<=target_level; i++) {
       n_nodes[i]= ceiling(n_nodes[i-1], nonleaf_fill_num+1);
       top_level= i;
    } // end of for each nonleaf level


    // 3. allocate nodes
    pfirst[0]= nvmpool_alloc(sizeof(bleaf) * n_nodes[0]);
    for (int i=1; i<=top_level; i++) {
       pfirst[i]= mempool_alloc(sizeof(bnode) * n_nodes[i]);
    }
       // nvmpool_alloc/mempool_alloc call exit if out of memory


    // 4. populate nodes
    for (int ll=1; ll<=top_level; ll++) {
        ncur[ll]= 0;
        bnode *np= (bnode *)(pfirst[ll]);
        np->lock()= 0; np->num()= -1;
    }

    bleaf * leaf= pfirst[0];
    int nodenum= n_nodes[0];

    bleafMeta leaf_meta;
    leaf_meta.v.bitmap= ( ((1<<leaf_fill_num)-1)
                         <<(LEAF_KEY_NUM-leaf_fill_num));
    leaf_meta.v.lock= 0;
    leaf_meta.v.alt= 0;

    int key_id= start_key;
    for (int i=0; i<nodenum; i++) {
        bleaf *lp= &(leaf[i]);

        // compute number of keys in this leaf node
        int fillnum= leaf_fill_num; // in most cases
        if (i == nodenum-1) {
           fillnum= num_key - (nodenum-1)*leaf_fill_num;
           assert(fillnum>=1 && fillnum<=leaf_fill_num);

           leaf_meta.v.bitmap= ( ((1<<fillnum)-1)
                                <<(LEAF_KEY_NUM-fillnum));
        }

        // lbtree tends to leave the first line empty
        for (int j=LEAF_KEY_NUM-fillnum; j<LEAF_KEY_NUM; j++) {

            // get key from input
            key_type mykey= (key_type)(input->get_key(key_id));
            key_id ++;

            // entry
            lp->k(j) = mykey;
            lp->ch(j) = (void *)mykey;

            // hash 
            leaf_meta.v.fgpt[j]= hashcode1B(mykey);

        } // for each key in this leaf node

        // sibling pointer
        lp->next[0]= ((i<nodenum-1) ? &(leaf[i+1]) : NULL);
        lp->next[1]= NULL;

        // 2x8B meta
        lp->setBothWords(&leaf_meta);


        // populate nonleaf node
        Pointer8B child= lp;
        key_type  left_key= lp->k(LEAF_KEY_NUM-fillnum);

        // append (left_key, child) to level ll node
        // child is the level ll-1 node to be appended.
        // left_key is the left-most key in the subtree of child.
        for (int ll=1; ll<=top_level; ll++) {
           bnode *np= ((bnode *)(pfirst[ll])) + ncur[ll];

           // if the node has >=1 child
           if (np->num() >= 0) {
               int kk= np->num()+1;
               np->ch(kk)= child; np->k(kk)= left_key; 
               np->num()= kk;

               if ((kk==nonleaf_fill_num)&&(ncur[ll]<n_nodes[ll]-1)) { 
                   ncur[ll] ++; np ++;
                   np->lock()= 0; np->num()= -1;
               }
               break;
           }

           // new node
           np->ch(0)= child; np->num() = 0;

           // append this new node to parent
           child= np;

        } // for each nonleaf level

    } // end of foreach leaf node

    // 5. return
    return top_level;
}

/**
 * build a top tree containing: ptrs/keys[0..num_key-1]
 * stop at target_level  
 *
 * @param ptrs           child node pointers
 * @param keys           left keys of subtrees rooted at child nodes
 * @param num_key        number of child nodes
 * @param bfill          filling factor in (0.0,1.0] 
 * @param cur_level      level of the child nodes
 * @param target_level   stop buidling the subtree at target level
 * @param pfirst         return pointers to the first node at each level
 * @param n_nodes        return number of nodes at each level 
 *
 * @retval the top level of the subtree (can be smaller than target_level
 * if the root level is smaller target_level)
 */
int lbtree::bulkloadToptree(
    Pointer8B ptrs[], key_type keys[], int num_key,
    float bfill, int cur_level, int target_level,
    Pointer8B pfirst[], int n_nodes[])
{
    // We assume that a tree cannot be higher than 32 levels
    int ncur[32];     // current node at every level
    int top_level;    // top_level is the top level that this method builds

    assert(num_key >= 2 && bfill>0.0 && bfill<=1.0 
           && cur_level >=0 && target_level>cur_level);

    // 1. compute nonleaf number of keys
    int nonleaf_fill_num= (int)((float)NON_LEAF_KEY_NUM * bfill);
    nonleaf_fill_num= max(nonleaf_fill_num, 1);


    // 2. compute number of nodes
    n_nodes[cur_level]= num_key;
    top_level= cur_level;

    for (int i=cur_level+1; n_nodes[i-1]>1 && i<=target_level; i++) {
       n_nodes[i]= ceiling(n_nodes[i-1], nonleaf_fill_num+1);
       top_level= i;
    } // end of for each nonleaf level


    // 3. allocate nodes
    for (int i=cur_level+1; i<=top_level; i++) {
       pfirst[i]= mempool_alloc(sizeof(bnode) * n_nodes[i]);
    }
       // mempool_alloc call exit if out of memory


    // 4. populate nodes
    for (int ll=cur_level+1; ll<=top_level; ll++) {
        ncur[ll]= 0;
        bnode *np= (bnode *)(pfirst[ll]);
        np->lock()= 0; np->num()= -1;
    }

    for (int i=0; i<num_key; i++) {
        Pointer8B child= ptrs[i];
        key_type  left_key= keys[i];

        // append (left_key, child) to level ll node
        // child is the level ll-1 node to be appended.
        // left_key is the left-most key in the subtree of child.
        for (int ll=cur_level+1; ll<=top_level; ll++) {
           bnode *np= ((bnode *)(pfirst[ll])) + ncur[ll];

           // if the node has >=1 child
           if (np->num() >= 0) {
               int kk= np->num()+1;
               np->ch(kk)= child; np->k(kk)= left_key; 
               np->num()= kk;

               if ((kk==nonleaf_fill_num)&&(ncur[ll]<n_nodes[ll]-1)) { 
                   ncur[ll] ++; np ++;
                   np->lock()= 0; np->num()= -1;
               }
               break;
           }

           // new node
           np->ch(0)= child; np->num() = 0;

           // append this new node to parent
           child= np;

        } // for each nonleaf level

    } // end of foreach key

    // 5. return
    return top_level;
}

/**
 * Obtain the node pointers and left keys for a given level
 *
 * @param pnode          subtree root
 * @param pnode_level    subtree root level
 * @param left_key       left key of the subtree
 * @param target_level   the level to get nodes and keys
 * @param ptrs           child node pointers (output)
 * @param keys           left keys of subtrees rooted at child nodes (output)
 * @param num_nodes      number of child nodes (output)
 * @param free_above_level_nodes  if nodes above target_level should be freed
 *
 */
void lbtree::getKeyPtrLevel(
Pointer8B pnode, int pnode_level, key_type left_key,
int target_level, Pointer8B ptrs[], key_type keys[], int &num_nodes,
bool free_above_level_nodes)
{
    // already at this target_level
    if (pnode_level == target_level) {
        ptrs[num_nodes]= pnode;
        keys[num_nodes]= left_key;
        num_nodes ++;
        return;
    }

    // pnode_level > target_level
    else if (pnode_level > target_level) {
        bnode *p= pnode;
        getKeyPtrLevel(p->ch(0), pnode_level-1, left_key,
                       target_level, ptrs, keys, num_nodes,
                       free_above_level_nodes);
        for (int i=1; i<=p->num(); i++) {
            getKeyPtrLevel(p->ch(i), pnode_level-1, p->k(i),
                       target_level, ptrs, keys, num_nodes,
                       free_above_level_nodes);
        }

        if (free_above_level_nodes) mempool_free_node(p);
    }
}


typedef struct BldThArgs {

    key_type start_key; // input
    key_type num_key;   // input

    int top_level;  // output
    int n_nodes[32];  // output
    Pointer8B pfirst[32];  // output

} BldThArgs;

//
// bulkload using multiple threads
//
int lbtree::bulkload (int keynum, keyInput *input, float bfill)
{
    // 1. allocate BldThArgs[]
    int num_threads= ((keynum>worker_thread_num*10) ?  worker_thread_num : 1);

    BldThArgs *bta= new BldThArgs[num_threads];
    if (!bta) {perror("malloc"); exit(1);}


    // 2. one thread?
    if (num_threads == 1) {
        bta[0].top_level= bulkloadSubtree(
                               input, 0, keynum, bfill, 31,
                               bta[0].pfirst, bta[0].n_nodes);
        tree_meta->root_level=  bta[0].top_level;
        tree_meta->tree_root=   bta[0].pfirst[tree_meta->root_level];
        tree_meta->setFirstLeaf(bta[0].pfirst[0]);

        // if this assertion is false, then the tree has > 31 levels
        assert(bta[0].n_nodes[bta[0].top_level] == 1);

        delete[] bta;
        return tree_meta->root_level;
    }


    // 3. compute start num_key for each thread
    int kn_per_thread= floor(keynum, num_threads);
    int kn_max= keynum-(num_threads-1)*kn_per_thread;

    for (int i=0; i<num_threads; i++) {
       bta[i].start_key= i*kn_per_thread;
       bta[i].num_key= ((i<num_threads-1)? kn_per_thread : kn_max);
    }

        // 0 .. num_threads-2: kn_per_thread keys
        // num_threads-1:      kn_max keys  (kn_max >= kn_per_thread)


    // 4. create threads to bulkload subtrees
    std::thread threads[num_threads];
    for (int i=0; i<num_threads; i++) {
       threads[i] = std::thread([=](){
                       worker_id= i;
                       keyInput *cursor= input->openCursor(
                                       bta[i].start_key, bta[i].num_key);
                       bta[i].top_level= bulkloadSubtree(
                               cursor, bta[i].start_key, bta[i].num_key,
                               bfill, 31,
                               bta[i].pfirst, bta[i].n_nodes);
                       input->closeCursor(cursor);
                    });
    }
    for (int i=0; i<num_threads; i++) threads[i].join();

    // connect the sibling pointers
    for (int i=1; i<num_threads; i++) {
        bleaf *lp= (bleaf *)(bta[i-1].pfirst[0]) + bta[i-1].n_nodes[0] - 1;
        lp->next[0]= bta[i].pfirst[0];
    }


    // 5. put the ptr to the top nonleaf nodes into an array
    //    using the min top level
    int level= bta[0].top_level;  // subtree 0 .. num_threads-2

    Pointer8B top_ptrs[num_threads*3];  // should be < 2*num_threads
    key_type  top_keys[num_threads*3];
    int num_nodes= 0;

    for (int i=0; i<num_threads; i++) {
       bleaf *lp =  bta[i].pfirst[0];
       key_type left_key= lp->k(LEAF_KEY_NUM - lp->num());
       getKeyPtrLevel(bta[i].pfirst[bta[i].top_level], 
                      bta[i].top_level, left_key,
                      level, top_ptrs, top_keys, num_nodes, true);
    }

    // Otherwise, top_keys[] and top_ptrs[] are not large enough (can't be true)
    assert(num_nodes <= sizeof(top_keys)/sizeof(key_type));


    // 6. build the top nonleaf nodes
    bta[0].top_level= bulkloadToptree(top_ptrs, top_keys, num_nodes, bfill, 
                                      level, 31, bta[0].pfirst, bta[0].n_nodes);

    tree_meta->root_level=  bta[0].top_level;
    tree_meta->tree_root=   bta[0].pfirst[tree_meta->root_level];
    tree_meta->setFirstLeaf(bta[0].pfirst[0]);

    // if this assertion is false, then the tree has > 31 levels
    assert(bta[0].n_nodes[bta[0].top_level] == 1);

    // 7. free BldThArgs[]
    delete[] bta;

    return tree_meta->root_level;
}


/* ----------------------------------------------------------------- *
 look up
 * ----------------------------------------------------------------- */

/* leaf is level 0, root is level depth-1 */

void * lbtree::lookup (key_type key, int *pos)
{
    bnode *p;
    bleaf *lp;
    int i,t,m,b;
    key_type r;
    
    unsigned char key_hash= hashcode1B(key);
    int ret_pos;
    
Again1:
    // 1. RTM begin
    if(_xbegin() != _XBEGIN_STARTED) goto Again1;

    // 2. search nonleaf nodes
    p = tree_meta->tree_root;
    
    for (i=tree_meta->root_level; i>0; i--) {
        
        // prefetch the entire node
        NODE_PREF(p);

        // if the lock bit is set, abort
        if (p->lock()) {_xabort(1); goto Again1;}
        
        // binary search to narrow down to at most 8 entries
        b=1; t=p->num();
        while (b+7<=t) {
            m=(b+t) >>1;
            r= key - p->k(m);
            if (r>0) b=m+1;
            else if (r<0) t = m-1;
            else {p=p->ch(m); goto inner_done;}
        }
        
        // sequential search (which is slightly faster now)
        for (; b<=t; b++)
            if (key < p->k(b)) break;
        p = p->ch(b-1);
        
    inner_done: ;
    }
    
    // 3. search leaf node
    lp= (bleaf *)p;

    // prefetch the entire node
    LEAF_PREF (lp);

    // if the lock bit is set, abort
    if (lp->lock) {_xabort(2); goto Again1;}

    // SIMD comparison
       // a. set every byte to key_hash in a 16B register 
    __m128i key_16B = _mm_set1_epi8((char)key_hash);

       // b. load meta into another 16B register
    __m128i fgpt_16B= _mm_load_si128((const __m128i*)lp);

       // c. compare them
    __m128i cmp_res = _mm_cmpeq_epi8(key_16B, fgpt_16B);

       // d. generate a mask
    unsigned int mask= (unsigned int)
                        _mm_movemask_epi8(cmp_res);  // 1: same; 0: diff

    // remove the lower 2 bits then AND bitmap
    mask=  (mask >> 2)&((unsigned int)(lp->bitmap));

    // search every matching candidate
    ret_pos= -1;
    while (mask) {
        int jj = bitScan(mask)-1;  // next candidate

        if (lp->k(jj) == key) { // found
           ret_pos= jj;
           break;
        }

        mask &= ~(0x1<<jj);  // remove this bit
    } // end while

    // 4. RTM commit
    _xend();

    *pos=ret_pos;
    return (void *)lp;
}

/* ------------------------------------- *
   quick sort the keys in leaf node
 * ------------------------------------- */


// pos[] will contain sorted positions
void lbtree::qsortBleaf(bleaf *p, int start, int end, int pos[])
{
    if (start >= end) return;

    int pos_start= pos[start];
    key_type key= p->k(pos_start);  // pivot
    int l, r;
        
    l= start;  r=end;
    while (l<r) {
        while ((l<r) && (p->k(pos[r])>key)) r--;
        if (l<r) {
            pos[l]= pos[r];
            l++;
        }
        while ((l<r) && (p->k(pos[l])<=key)) l++;
        if (l<r) {
            pos[r]= pos[l];
            r--;
        }
    }
    pos[l]= pos_start;
    qsortBleaf(p, start, l-1, pos);
    qsortBleaf(p, l+1, end, pos);
}

/* ---------------------------------------------------------- *
 
 insertion: insert (key, ptr) pair into unsorted_leaf_bmp
 
 * ---------------------------------------------------------- */

void lbtree::insert (key_type key, void *ptr)
{
    // record the path from root to leaf
    // parray[level] is a node on the path
    // child ppos[level] of parray[level] == parray[level-1]
    //
    Pointer8B parray[32];  // 0 .. root_level will be used
    short     ppos[32];    // 1 .. root_level will be used
    bool      isfull[32];  // 0 .. root_level will be used

    unsigned char key_hash= hashcode1B(key);
    volatile long long sum;

    /* Part 1. get the positions to insert the key */
  { bnode *p;
    bleaf *lp;
    int i,t,m,b;
    key_type r;
    
Again2:
    // 1. RTM begin
    if(_xbegin() != _XBEGIN_STARTED) {
        // random backoff
        // sum= 0; 
        // for (int i=(rdtsc() % 1024); i>0; i--) sum += i;
        goto Again2;
    }

    // 2. search nonleaf nodes
    p = tree_meta->tree_root;
    
    for (i=tree_meta->root_level; i>0; i--) {
        
        // prefetch the entire node
        NODE_PREF(p);

        // if the lock bit is set, abort
        if (p->lock()) {_xabort(3); goto Again2;}

        parray[i]= p;
        isfull[i]= (p->num() == NON_LEAF_KEY_NUM);

        // binary search to narrow down to at most 8 entries
        b=1; t=p->num();
        while (b+7<=t) {
            m=(b+t) >>1;
            r= key - p->k(m);
            if (r>0) b=m+1;
            else if (r<0) t = m-1;
            else {p=p->ch(m); ppos[i]=m; goto inner_done;}
        }
        
        // sequential search (which is slightly faster now)
        for (; b<=t; b++)
            if (key < p->k(b)) break;
        p = p->ch(b-1); ppos[i]= b-1;
        
    inner_done: ;
    }
    
    // 3. search leaf node
    lp= (bleaf *)p;

    // prefetch the entire node
    LEAF_PREF (lp);

    // if the lock bit is set, abort
    if (lp->lock) {_xabort(4); goto Again2;}

    parray[0]= lp;

    // SIMD comparison
       // a. set every byte to key_hash in a 16B register 
    __m128i key_16B = _mm_set1_epi8((char)key_hash);

       // b. load meta into another 16B register
    __m128i fgpt_16B= _mm_load_si128((const __m128i*)lp);

       // c. compare them
    __m128i cmp_res = _mm_cmpeq_epi8(key_16B, fgpt_16B);

       // d. generate a mask
    unsigned int mask= (unsigned int)
                        _mm_movemask_epi8(cmp_res);  // 1: same; 0: diff

    // remove the lower 2 bits then AND bitmap
    mask=  (mask >> 2)&((unsigned int)(lp->bitmap));

    // search every matching candidate
    while (mask) {
        int jj = bitScan(mask)-1;  // next candidate

        if (lp->k(jj) == key) { // found: do nothing, return
           _xend();
           return;
        }

        mask &= ~(0x1<<jj);  // remove this bit
    } // end while

    // 4. set lock bits before exiting the RTM transaction
    lp->lock= 1;

    isfull[0]= lp->isFull();
    if (isfull[0]) {
        for (i=1; i<=tree_meta->root_level; i++) {
            p= parray[i];
            p->lock()= 1;
            if (! isfull[i]) break;
        }
    }

    // 5. RTM commit
    _xend();

  } // end of Part 1

    /* Part 2. leaf node */
  {
    bleaf *lp= parray[0];
    bleafMeta meta= *((bleafMeta *)lp);


    /* 1. leaf is not full */
    if (! isfull[0]) {

       meta.v.lock= 0;  // clear lock in temp meta

       // 1.1 get first empty slot
       uint16_t bitmap= meta.v.bitmap;
       int slot= bitScan(~bitmap)-1;

       // 1.2 set leaf.entry[slot]= (k, v);
       // set fgpt, bitmap in meta
       lp->k(slot)= key;
       lp->ch(slot)= ptr;
       meta.v.fgpt[slot]= key_hash;
       bitmap |= (1<<slot); 

       // 1.3 line 0: 0-2; line 1: 3-6; line 2: 7-10; line 3: 11-13
       // in line 0?
       if (slot<3) {
           // 1.3.1 write word 0
           meta.v.bitmap= bitmap;
           lp->setWord0(&meta);

           // 1.3.2 flush
           clwb(lp); sfence();

           return;
       }

       // 1.4 line 1--3
       else {
         int last_slot= last_slot_in_line[slot];
         int from= 0;
         for (int to=slot+1; to<=last_slot; to++) {
            if ((bitmap&(1<<to))==0) {
               // 1.4.1 for each empty slot in the line
               // copy an entry from line 0
               lp->ent[to]= lp->ent[from];
               meta.v.fgpt[to]= meta.v.fgpt[from];
               bitmap |= (1<<to); bitmap &= ~(1<<from);
               from ++;
            }
         }

         // 1.4.2 flush the line containing slot
         clwb(&(lp->k(slot))); sfence();

         // 1.4.3 change meta and flush line 0
         meta.v.bitmap= bitmap;
         lp->setBothWords(&meta);
         clwb(lp); sfence();

         return;
       }
    } // end of not full

    /* 2. leaf is full, split */

    // 2.1 get sorted positions
    int sorted_pos[LEAF_KEY_NUM];
    for (int i=0; i<LEAF_KEY_NUM; i++) sorted_pos[i]=i;
    qsortBleaf(lp, 0, LEAF_KEY_NUM-1, sorted_pos);

    // 2.2 split point is the middle point
    int split= (LEAF_KEY_NUM/2);  // [0,..split-1] [split,LEAF_KEY_NUM-1]
    key_type split_key= lp->k(sorted_pos[split]);

    // 2.3 create new node
    bleaf * newp = (bleaf *)nvmpool_alloc_node(LEAF_SIZE);

    // 2.4 move entries sorted_pos[split .. LEAF_KEY_NUM-1]
    uint16_t freed_slots= 0;
    for (int i=split; i<LEAF_KEY_NUM; i++) {
        newp->ent[i]= lp->ent[sorted_pos[i]];
        newp->fgpt[i]= lp->fgpt[sorted_pos[i]];

        // add to freed slots bitmap
        freed_slots |= (1<<sorted_pos[i]);
    }
    newp->bitmap= (((1<<(LEAF_KEY_NUM - split))-1) << split);
    newp->lock= 0; newp->alt= 0;

       // remove freed slots from temp bitmap
    meta.v.bitmap &= ~freed_slots;

    newp->next[0]= lp->next[lp->alt];
    lp->next[1-lp->alt]= newp;

       // set alt in temp bitmap
    meta.v.alt= 1 - lp->alt;

    // 2.5 key > split_key: insert key into new node
    if (key > split_key) {
        newp->k(split-1)= key; newp->ch(split-1)= ptr;
        newp->fgpt[split-1]= key_hash;
        newp->bitmap |= 1<<(split-1);

        if (tree_meta->root_level > 0) meta.v.lock= 0;  // do not clear lock of root
    }
    
    // 2.6 clwb newp, clwb lp line[3] and sfence
    LOOP_FLUSH(clwb, newp, LEAF_LINE_NUM); 
    clwb(&(lp->next[0]));
    sfence();

    // 2.7 clwb lp and flush: NVM atomic write to switch alt and set bitmap
    lp->setBothWords(&meta);
    clwb(lp); sfence();

    // 2.8 key < split_key: insert key into old node 
    if (key <= split_key) {

       // note: lock bit is still set
       if (tree_meta->root_level > 0) meta.v.lock= 0;  // do not clear lock of root

       // get first empty slot
       uint16_t bitmap= meta.v.bitmap;
       int slot= bitScan(~bitmap)-1;

       // set leaf.entry[slot]= (k, v);
       // set fgpt, bitmap in meta
       lp->k(slot)= key;
       lp->ch(slot)= ptr;
       meta.v.fgpt[slot]= key_hash;
       bitmap |= (1<<slot); 

       // line 0: 0-2; line 1: 3-6; line 2: 7-10; line 3: 11-13
       // in line 0?
       if (slot<3) {
           // write word 0
           meta.v.bitmap= bitmap;
           lp->setWord0(&meta);
           // flush
           clwb(lp); sfence();
       }
       // line 1--3
       else {
         int last_slot= last_slot_in_line[slot];
         int from= 0;
         for (int to=slot+1; to<=last_slot; to++) {
            if ((bitmap&(1<<to))==0) {
               // for each empty slot in the line
               // copy an entry from line 0
               lp->ent[to]= lp->ent[from];
               meta.v.fgpt[to]= meta.v.fgpt[from];
               bitmap |= (1<<to); bitmap &= ~(1<<from);
               from ++;
            }
         }

         // flush the line containing slot
         clwb(&(lp->k(slot))); sfence();

         // change meta and flush line 0
         meta.v.bitmap= bitmap;
         lp->setBothWords(&meta);
         clwb(lp); sfence();
       }
    }

    key= split_key; ptr= newp;
    /* (key, ptr) to be inserted in the parent non-leaf */

  } // end of Part 2

    /* Part 3. nonleaf node */
    {bnode *p, *newp;
     int    n, i, pos, r, lev, total_level;
        
#define   LEFT_KEY_NUM		((NON_LEAF_KEY_NUM)/2)
#define   RIGHT_KEY_NUM		((NON_LEAF_KEY_NUM) - LEFT_KEY_NUM)
        
        total_level = tree_meta->root_level;
        lev = 1;
        
        while (lev <= total_level) {
            
            p = parray[lev];
            n = p->num();
            pos = ppos[lev] + 1;  // the new child is ppos[lev]+1 >= 1
            
            /* if the non-leaf is not full, simply insert key ptr */
            
            if (n < NON_LEAF_KEY_NUM) {
                for (i=n; i>=pos; i--) p->ent[i+1]= p->ent[i];

                p->k(pos) = key; p->ch(pos) = ptr; 
                p->num() = n+1; 
                sfence();

                // unlock after all changes are globally visible
                p->lock()= 0;
                return;
            }
            
            /* otherwise allocate a new non-leaf and redistribute the keys */
            newp = (bnode *)mempool_alloc_node(NONLEAF_SIZE);
            
            /* if key should be in the left node */
            if (pos <= LEFT_KEY_NUM) {
                for (r=RIGHT_KEY_NUM, i=NON_LEAF_KEY_NUM; r>=0; r--, i--) {
                    newp->ent[r]= p->ent[i];
                }
                /* newp->key[0] actually is the key to be pushed up !!! */
                for (i=LEFT_KEY_NUM-1; i>=pos; i--) p->ent[i+1]= p->ent[i];

                p->k(pos) = key; p->ch(pos) = ptr;
            } 
            /* if key should be in the right node */
            else {
                for (r=RIGHT_KEY_NUM, i=NON_LEAF_KEY_NUM; i>=pos; i--, r--){
                    newp->ent[r]= p->ent[i];
                }
                newp->k(r) = key; newp->ch(r) = ptr; r--;
                for (;r>=0; r--, i--) {
                    newp->ent[r]= p->ent[i];
                }
            } /* end of else */
            
            key = newp->k(0); ptr = newp;

            p->num() = LEFT_KEY_NUM;  
            if (lev < total_level) p->lock()= 0; // do not clear lock bit of root
            newp->num() = RIGHT_KEY_NUM; newp->lock()= 0;

            lev ++;
        } /* end of while loop */
        
        /* root was splitted !! add another level */
        newp = (bnode *)mempool_alloc_node(NONLEAF_SIZE);
        
        newp->num() = 1; newp->lock()= 1;
        newp->ch(0) = tree_meta->tree_root; newp->ch(1) = ptr; newp->k(1) = key;
        sfence();  // ensure new node is consistent

        void *old_root= tree_meta->tree_root;
        tree_meta->root_level = lev;
        tree_meta->tree_root = newp;  
        sfence();   // tree root change is globablly visible
                    // old root and new root are both locked

        // unlock old root
        if (total_level > 0) { // previous root is a nonleaf
           ((bnode *)old_root)->lock()= 0;
        }
        else { // previous root is a leaf
           ((bleaf *)old_root)->lock= 0;
        }

        // unlock new root
        newp->lock()= 0;

        return;
        
#undef RIGHT_KEY_NUM
#undef LEFT_KEY_NUM
    }
}

/* ---------------------------------------------------------- *
 
 deletion
 
 lazy delete - insertions >= deletions in most cases
 so no need to change the tree structure frequently
 
 So unless there is no key in a leaf or no child in a non-leaf, 
 the leaf and non-leaf won't be deleted.
 
 * ---------------------------------------------------------- */
void lbtree::del (key_type key)
{
    // record the path from root to leaf
    // parray[level] is a node on the path
    // child ppos[level] of parray[level] == parray[level-1]
    //
    Pointer8B parray[32];  // 0 .. root_level will be used
    short     ppos[32];    // 0 .. root_level will be used
    bleaf *   leaf_sibp= NULL;  // left sibling of the target leaf

    unsigned char key_hash= hashcode1B(key);
    volatile long long sum;

    /* Part 1. get the positions to insert the key */
  { bnode *p;
    bleaf *lp;
    int i,t,m,b;
    key_type r;
    
Again3:
    // 1. RTM begin
    if(_xbegin() != _XBEGIN_STARTED) {
        // random backoff
        // sum= 0; 
        // for (int i=(rdtsc() % 1024); i>0; i--) sum += i;
        goto Again3;
    }

    // 2. search nonleaf nodes
    p = tree_meta->tree_root;
    
    for (i=tree_meta->root_level; i>0; i--) {
        
        // prefetch the entire node
        NODE_PREF(p);

        // if the lock bit is set, abort
        if (p->lock()) {_xabort(5); goto Again3;}

        parray[i]= p;

        // binary search to narrow down to at most 8 entries
        b=1; t=p->num();
        while (b+7<=t) {
            m=(b+t) >>1;
            r= key - p->k(m);
            if (r>0) b=m+1;
            else if (r<0) t = m-1;
            else {p=p->ch(m); ppos[i]=m; goto inner_done;}
        }
        
        // sequential search (which is slightly faster now)
        for (; b<=t; b++)
            if (key < p->k(b)) break;
        p = p->ch(b-1); ppos[i]= b-1;
        
    inner_done: ;
    }
    
    // 3. search leaf node
    lp= (bleaf *)p;

    // prefetch the entire node
    LEAF_PREF (lp);

    // if the lock bit is set, abort
    if (lp->lock) {_xabort(6); goto Again3;}

    parray[0]= lp;

    // SIMD comparison
       // a. set every byte to key_hash in a 16B register 
    __m128i key_16B = _mm_set1_epi8((char)key_hash);

       // b. load meta into another 16B register
    __m128i fgpt_16B= _mm_load_si128((const __m128i*)lp);

       // c. compare them
    __m128i cmp_res = _mm_cmpeq_epi8(key_16B, fgpt_16B);

       // d. generate a mask
    unsigned int mask= (unsigned int)
                        _mm_movemask_epi8(cmp_res);  // 1: same; 0: diff

    // remove the lower 2 bits then AND bitmap
    mask=  (mask >> 2)&((unsigned int)(lp->bitmap));

    // search every matching candidate
    i= -1;
    while (mask) {
        int jj = bitScan(mask)-1;  // next candidate

        if (lp->k(jj) == key) { // found: good
           i= jj;
           break;
        }

        mask &= ~(0x1<<jj);  // remove this bit
    } // end while

    if (i < 0) { // not found: do nothing
       _xend();
       return;
    }

    ppos[0]= i;

    // 4. set lock bits before exiting the RTM transaction
    lp->lock= 1;

    if (lp->num() == 1) {

        // look for its left sibling
        for (i=1; i<=tree_meta->root_level; i++) {
            if (ppos[i]>=1) break;
        }

        if (i <=  tree_meta->root_level) {
            p= parray[i];
            p= p->ch(ppos[i]-1); i--;

            for (; i>=1; i--) {
               p= p->ch(p->num());
            }

            leaf_sibp= (bleaf *)p;
            if (leaf_sibp->lock) {_xabort(7); goto Again3;}

            // lock leaf_sibp
            leaf_sibp->lock= 1;
        }

        // lock affected ancestors
        for (i=1; i<=tree_meta->root_level; i++) {
            p= (bnode *) parray[i];  
            p->lock()= 1;
            
            if (p->num() >= 1) break;  // at least 2 children, ok to stop
        }
    }

    // 5. RTM commit
    _xend();

  } // end of Part 1

    /* Part 2. leaf node */
  {
    bleaf *lp= parray[0];

    /* 1. leaf contains more than one key */
    /*    If this leaf node is the root, we cannot delete the root. */
    if ((lp->num()>1)||(tree_meta->root_level==0)) {
       bleafMeta meta= *((bleafMeta *)lp);

       meta.v.lock= 0;  // clear lock in temp meta
       meta.v.bitmap &= ~(1<<ppos[0]);  // mark the bitmap to delete the entry
       lp->setWord0(&meta);
       clwb(lp); sfence();

       return;

    } // end of more than one key

    /* 2. leaf has only one key: remove the leaf node */
    
        /* if it has a left sibling */
        if (leaf_sibp != NULL) {
            // remove it from sibling linked list
            leaf_sibp->next[leaf_sibp->alt]= lp->next[lp->alt];
            clwb(&(leaf_sibp->next[0])); sfence();

            leaf_sibp->lock=0;  // lock bit is not protected.  
                                // It will be reset in recovery
        }

        /* or it is the first child, so let's modify the first_leaf */
        else {
            tree_meta->setFirstLeaf(lp->next[lp->alt]);  // the method calls clwb+sfence
        }

     // free the deleted leaf node
     nvmpool_free_node(lp);

  } // end of Part 2

    /* Part 3: non-leaf node */
    {bnode *p, *sibp, *parp;
     int    n, i, pos, r, lev;

        lev = 1;

        while (1) {
            p = parray[lev];
            n = p->num();
            pos = ppos[lev];

            /* if the node has more than 1 children, simply delete */
            if (n > 0) {
                if (pos == 0) {
                    p->ch(0)= p->ch(1); 
                    pos= 1;  // move the rest
                }
                for (i=pos; i<n; i++) p->ent[i]= p->ent[i+1];
                p->num()= n - 1;
                sfence();  
                // all changes are globally visible now

                // root is guaranteed to have 2 children
                if ((p->num()==0) && (lev >= tree_meta->root_level)) // root
                    break;

                p->lock()= 0;
                return;
            }

            /* otherwise only 1 ptr */
            mempool_free_node(p);

            lev++;
        } /* end of while */

        // p==root has 1 child? so delete the root
        tree_meta->root_level = tree_meta->root_level - 1;
        tree_meta->tree_root = p->ch(0); // running transactions will abort
        sfence();

        mempool_free_node (p);
        return;
    }
}

/* ----------------------------------------------------------------- *
 randomize
 * ----------------------------------------------------------------- */

void lbtree::randomize (Pointer8B pnode, int level)
{int i;

    if (level > 0) {
        bnode *p= pnode;
        for (int i=0; i<=p->num(); i++) {
            randomize (p->ch(i), level-1);
        }
    }
    else {
        bleaf * lp = pnode;

        int pos[LEAF_KEY_NUM];
        int num= 0;

        // 1. get all entries
        unsigned short bmp= lp->bitmap;
        for (int i=0; i<LEAF_KEY_NUM; i++) {
            if (bmp & (1<<i)) {
                pos[num++]= i;
            }
        }

        // 2. randomly shuffle the entries
        for (int i=0; i<num*2; i++) {
           int aa= (int)(drand48()*num);  // [0,num-1]
           int bb= (int)(drand48()*num);

           if (aa != bb) {
               swap(lp->fgpt[pos[aa]], lp->fgpt[pos[bb]]);
               swap(lp->ent[pos[aa]],  lp->ent[pos[bb]]);
           }
        }
    }
}

/* ----------------------------------------------------------------- *
 print
 * ----------------------------------------------------------------- */
void lbtree::print (Pointer8B pnode, int level)
{
    if (level > 0) {
        bnode *p= pnode;

        printf("%*cnonleaf lev=%d num=%d\n", 10+level*4, '+', level, p->num());

        print (p->ch(0), level-1);
        for (int i=1; i<=p->num(); i++) {
            printf ("%*c%lld\n", 10+level*4, '+', p->k(i));
            print (p->ch(i), level-1);
        }
    }
    else {
        bleaf * lp = pnode;

        unsigned short bmp= lp->bitmap;
        for (int i=0; i<LEAF_KEY_NUM; i++) {
            if (bmp & (1<<i)) {
                printf ("[%2d] hash=%02x key=%lld\n", i, lp->fgpt[i], lp->k(i));
            }
        }

        bleaf * pnext= lp->nextSibling();
        if (pnext != NULL) {
            int first_pos= bitScan(pnext->bitmap)-1;
            printf ("->(%lld)\n", pnext->k(first_pos));
        }
        else
            printf ("->(null)\n");
    }
}

/* ----------------------------------------------------------------- *
 check structure integrity
 * ----------------------------------------------------------------- */

/**
 * get min and max key in the given leaf p
 */
void lbtree::getMinMaxKey (bleaf *p, key_type &min_key, key_type &max_key)
{
    unsigned short bmp= p->bitmap;
    max_key= MIN_KEY;
    min_key= MAX_KEY;
    
    for (int i=0; i<LEAF_KEY_NUM; i++) {
        if (bmp & (1<<i)) {
            if (p->k(i) > max_key) max_key = p->k(i);
            if (p->k(i) < min_key) min_key = p->k(i);
        }
    }
}

void lbtree::checkFirstLeaf(void)
{
    // get left-most leaf node
    bnode *p= tree_meta->tree_root;
    for (int i= tree_meta->root_level; i>0; i--) p= p->ch(0);

    if ((bleaf *)p != *(tree_meta->first_leaf)) {
       printf("first leaf %p != %p\n", *(tree_meta->first_leaf), p);
       exit(1);
    }
}

/**
 * recursively check the subtree rooted at pnode
 *
 * If it encounters an error, the method will print an error message and exit.
 *
 * @param pnode   the subtree root
 * @param level   the level of pnode
 * @param start   return the start key of this subtree
 * @param end     return the end key of this subtree
 * @param ptr     ptr is the leaf before this subtree. 
 *                Upon return, ptr is the last leaf of this subtree.
 */
void lbtree::check (Pointer8B pnode, int level, key_type &start, key_type &end, bleaf * &ptr)
{
    if (pnode.isNull()) {
        printf ("level %d: null child pointer\n", level + 1);
        exit (1);
    }
    
    if (level == 0) { // leaf node
        bleaf *lp = pnode;

        if (((unsigned long long)lp)%256 != 0) {
            printf ("leaf(%p): not aligned at 256B\n", lp); exit (1);
        }
        
        // check number of keys
        if (lp->num() < 1) {// empty node!
            printf ("leaf(%p): empty\n", lp); exit (1);
        }

        // get min max
        getMinMaxKey(lp, start, end);

        // check fingerprints
        unsigned short bmp= lp->bitmap;
        for (int i=0; i<LEAF_KEY_NUM; i++) {
            if (bmp & (1<<i)) {
                if (hashcode1B(lp->k(i)) != lp->fgpt[i]) {
                    printf ("leaf(%lld): hash code for %lld is wrong\n", start, lp->k(i)); 
                    exit(1);
                }
            }
        }

        // check lock bit
        if (lp->lock != 0) {
            printf ("leaf(%lld): lock bit == 1\n", start); 
            exit(1);
        }

        // check sibling pointer
        if ((ptr) && (ptr->nextSibling()!=lp)) {
            printf ("leaf(%lld): sibling broken from previous node\n", start); 
            fflush(stdout);

            /* output more info */
            bleaf *pp= (bleaf *)ptr;
            key_type ss, ee;
            getMinMaxKey(pp, ss, ee);
            printf("previous(%lld - %lld) -> ", ss, ee);

            pp= pp->nextSibling();
            if (pp == NULL) {
                printf("nil\n");
            }
            else {
                getMinMaxKey(pp, ss, ee);
                printf("(%lld - %lld)\n", ss, ee);
            }

            exit(1);
        }

        ptr= lp;
    }

    else { // nonleaf node
        key_type curstart, curend;
        int i;
        bleaf *curptr;
        
        bnode * p = pnode;

        if (((unsigned long long)p)%64!= 0) {
            printf ("nonleaf level %d(%p): not aligned at 64B\n", level, p); exit (1);
        }
        
        // check num of keys
        if (p->num()<0) {
            printf ("nonleaf level %d(%p): num<0\n", level, p); exit (1);
        }

        // check child 0
        curptr = ptr;
        check (p->ch(0), level-1, curstart, curend, curptr);
        start = curstart;
        if (p->num()>=1 && curend >= p->k(1)) {
            printf ("nonleaf level %d(%lld): key order wrong at child 0\n", level, p->k(1));
            exit (1);
        }
        
        // check child 1..num-1
        for (i=1; i<p->num(); i++) {
            check (p->ch(i), level-1, curstart, curend, curptr);
            if (!(p->k(i)<=curstart && curend<p->k(i+1)) ) {
                printf ("nonleaf level %d(%lld): key order wrong at child %d(%lld)\n",
                        level, p->k(1), i, p->k(i));
                exit (1);
            }
        }

        // check child num (when num>=1)
        if (i == p->num()) {
           check (p->ch(i), level-1, curstart, curend, curptr);
           if (curstart < p->k(i)) {
               printf ("nonleaf level %d(%lld): key order wrong at last child %d(%lld)\n", 
                        level, p->k(1), i, p->k(i));
               exit (1);
           }
        }
        end = curend;

        // check lock bit
        if (p->lock() != 0) {
            printf ("nonleaf level %d(%lld): lock bit is set\n", level, p->k(1)); 
            exit(1);
        }
        
        ptr = curptr;
    }
}


/* ------------------------------------------------------------------------- */
/*                              driver                                       */
/* ------------------------------------------------------------------------- */
tree * initTree(void *nvm_addr, bool recover)
{
    tree *mytree = new lbtree(nvm_addr, recover);
    return mytree;
}

int main (int argc, char *argv[])
{
    printf("NON_LEAF_KEY_NUM= %d, LEAF_KEY_NUM= %d, nonleaf size= %lu, leaf size= %lu\n",
           NON_LEAF_KEY_NUM, LEAF_KEY_NUM, sizeof(bnode), sizeof(bleaf));
    assert((sizeof(bnode) == NONLEAF_SIZE)&&(sizeof(bleaf) == LEAF_SIZE));

    initUseful();

    return parse_command (argc, argv);
}
