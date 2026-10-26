
	// supports hybrid arrays of size up to 2^64-1

#include "hybridBV/hybridId.h"

extern float FactorId = 1.0; // Factor * length reads => rebuild as static

static inline int mustFlatten(hybridId B)
  { return (B->bv.dyn->accesses >= FactorId*B->bv.dyn->size);
  }

static const float TrfFactor = 0.125; // TrfFactor * MaxLeafSize to justify transferLeft/Right

static const float AlphaFactor = 0.65; // balance factor 3/5 < . < 1

static const int MinLeavesToBalance = 5; // min number of leaves to balance the tree

static const float MinFillFactor = 0.3; // less than this involves rebuild.
                                // Must be <= NewFraction/2

	// creates an empty hybridId 

hybridId hybridIdCreate (uint width)

   { hybridId B = myalloc(sizeof(struct s_hybridId));
     B->type = tLeaf;
     B->bv.leaf = leafIdCreate(width);
     return B; 
   }

hybridId hybridIdCreateFrom64NoFree (uint64_t *data, uint64_t n, uint width)

{ hybridId B = myalloc(sizeof(struct s_hybridId));
    if (n > leafIdNewSize(width))
    { B->type = tStatic;
        B->bv.stat = leafIdCreateFrom64NoFree(data,n,width,1);
    }
    else
    { B->type = tLeaf;
        B->bv.leaf = leafIdCreateFrom64NoFree(data,n,width,0);
    }
    return B;
}
	// converts an array of uint64_t into a hybridId of n elements
	// of width width. data is pointed to and will be freed 

hybridId hybridIdCreateFrom64 (uint64_t *data, uint64_t n, uint width)

   { hybridId B = myalloc(sizeof(struct s_hybridId));
     if (n > leafIdNewSize(width))
        { B->type = tStatic;
          B->bv.stat = leafIdCreateFrom64(data,n,width,1);
        }
     else 
        { B->type = tLeaf;
          B->bv.leaf = leafIdCreateFrom64(data,n,width,0);
        } 
     return B;
   }

	// converts an array of uint32_t into a hybridId of n elements
	// of width width. data is pointed to and will be freed 

hybridId hybridIdCreateFrom32 (uint32_t *data, uint64_t n, uint width)

   { hybridId B = myalloc(sizeof(struct s_hybridId));
     if (n > leafIdNewSize(width))
        { B->type = tStatic;
          B->bv.stat = leafIdCreateFrom32(data,n,width,1);
        }
     else 
        { B->type = tLeaf;
          B->bv.leaf = leafIdCreateFrom32(data,n,width,0);
        } 
     return B;
   }


hybridId hybridIdClone (hybridId B)

{ hybridId BC = myalloc(sizeof(struct s_hybridId));
    BC->type = B->type;
    if (B->type == tLeaf) BC->bv.leaf = leafIdClone(B->bv.leaf);
    else if (B->type == tStatic) BC->bv.stat = leafIdClone(B->bv.stat);
    else {
        BC->bv.dyn->accesses = B->bv.dyn->accesses;
        BC->bv.dyn->leaves = B->bv.dyn->leaves;
        BC->bv.dyn->size = B->bv.dyn->size;
        BC->bv.dyn->width = B->bv.dyn->width;
        BC->bv.dyn->left = hybridIdClone(B->bv.dyn->left);
        BC->bv.dyn->right = hybridIdClone(B->bv.dyn->right);
    }
    return BC;
}

	// destroys B, frees data 

void hybridIdDestroy (hybridId B)

   { if (B->type == tLeaf) leafIdDestroy(B->bv.leaf);
     else if (B->type == tStatic) leafIdDestroy(B->bv.stat);
     else { hybridIdDestroy(B->bv.dyn->left);
            hybridIdDestroy(B->bv.dyn->right);
            myfree(B->bv.dyn);
          }
     myfree(B);
   }

        // creates a static version of B, rewriting it but not its address

        // reads B into D[j..] without unpacking 

static void packedRead (hybridId B, uint64_t *D, uint64_t j)

   { uint64_t lsize;
     uint width;
     if (B->type == tDynamic)
        { lsize = hybridIdLength(B->bv.dyn->left);
          packedRead(B->bv.dyn->left,D,j);
          packedRead(B->bv.dyn->right,D,j+lsize);
	  return;
        }
     width = hybridIdWidth(B);
     if (B->type == tStatic)
          copyBits(D,j*width,B->bv.stat->data,0,hybridIdLength(B)*width);
     else copyBits(D,j*width,B->bv.leaf->data,0,hybridIdLength(B)*width);
   }

        // collects all the descending elements into an array, destroys bv.dyn

static uint64_t* collect (hybridId B, uint64_t len, uint width)

   { uint64_t *D;
     D = (uint64_t*)myalloc(((len*width+w64-1)/w64)*sizeof(uint64_t));
     packedRead (B,D,0);
     hybridIdDestroy(B->bv.dyn->left);
     hybridIdDestroy(B->bv.dyn->right);
     myfree(B->bv.dyn);
     return D;
   }

        // gives number of leaves

static inline uint64_t hybridIdLeaves (hybridId B)

   { uint num;
     if (B->type == tLeaf) return 1;
     if (B->type == tDynamic) return B->bv.dyn->leaves;
     num = leafIdNewSize(B->bv.stat->width);
     return (leafIdLength(B->bv.stat)+num-1)/num;
   }

        // converts into a leaf if it's short or into a static otherwise
	// delta gives the difference in leaves (new - old)

static void flatten (hybridId B, int64_t *delta)

   { uint64_t len;
     uint64_t *D;
     uint width;
     
     if (B->type != tDynamic) return;
     width = hybridIdWidth(B);
     len = hybridIdLength(B);
    *delta = - hybridIdLeaves(B);
     D = collect(B,len,width);
     if (len > leafIdNewSize(width)) // creates a static
        { B->type = tStatic;
          B->bv.stat = leafIdCreateStaticFromPacked(D,len,width);
        }
     else // a leaf
        { B->type = tLeaf;
          B->bv.leaf = leafIdCreateFromPacked(D,0,len,width);
	  myfree(D);
        }
     *delta += hybridIdLeaves(B);
   }

       // halves a static array into leaves, leaving a leaf covering i
        // returns a dynamicId and destroys B

static dynamicId splitFrom (uint64_t *data, uint64_t n, uint width, uint64_t i)

   { hybridId HB;
     dynamicId DB,finalDB;
     uint bnum; // size in elements of blocks to create
     uint bsize; // bit size of blocks to create
     uint64_t *segment;
     uint64_t nblock;
     uint64_t start,mid,end;

     HB = NULL;
     bnum = leafIdNewSize(width);
     bsize = bnum * width;
     nblock = (n+bnum-1)/bnum; // total blocks
     start = 0;
     end = n;
     while (nblock >= 2) {
        DB = (dynamicId)myalloc(sizeof(struct s_dynamicId));
        if (HB == NULL) finalDB = DB;
        else {
           HB->type = tDynamic;
           HB->bv.dyn = DB;
           }
        DB->size = n;
	DB->width = width;
	DB->leaves = nblock;
        DB->accesses = 0;
        mid = start+(nblock/2)*bnum;
        if (i/bnum < nblock/2) { // split the left half
                // create right half
           DB->right = HB = (hybridId)myalloc(sizeof(struct s_hybridId));
           if (n - (nblock/2)*bnum > leafIdNewSize(width)) { // create a static
              segment = (uint64_t*)myalloc((((end-mid)*width+w64-1)/w64)
					   * sizeof(uint64_t));
              copyBits(segment,0,data,mid*width,(end-mid)*width);
              HB->type = tStatic;
              HB->bv.stat = 
		  leafIdCreateStaticFromPacked(segment,n-(nblock/2)*bnum,width);
              }
           else { // create a leaf
              HB->type = tLeaf;
              HB->bv.leaf =
		  leafIdCreateFromPacked(data,mid,n-(nblock/2)*bnum,width);
              }
                // continue for left half
           end = mid;
           nblock = nblock/2;
           n = nblock * bnum;
           DB->left = HB = (hybridId)myalloc(sizeof(struct s_hybridId));
           }
        else { // split the right half
                // create left half
           DB->left = HB = (hybridId)myalloc(sizeof(struct s_hybridId));
           if ((nblock/2)*bnum > leafIdNewSize(width)) { // create a static
              segment = (uint64_t*)myalloc((((mid-start)*width+w64-1)/w64)
					   * sizeof(uint64_t));
              copyBits(segment,0,data,start*width,(mid-start)*width);
              HB->type = tStatic;
	      HB->bv.stat = 
		  leafIdCreateStaticFromPacked(segment,(nblock/2)*bnum,width);
              }
           else { // create a leaf
              HB->type = tLeaf;
              HB->bv.leaf =
		  leafIdCreateFromPacked(data,start,(nblock/2)*bnum,width);
              }
                // continue for right half
           start = mid;
           n = n-(nblock/2)*bnum;
	   i = i-(nblock/2)*bnum;
           nblock = nblock - nblock/2;
           DB->right = HB = (hybridId)myalloc(sizeof(struct s_hybridId));
           }
        }
        // finally, the leaf where i lies
     HB->type = tLeaf;
     HB->bv.leaf = leafIdCreateFromPacked(data,start,n,width);
     return finalDB;
   }

static dynamicId split (leafId B, uint64_t i)

   { dynamicId DB;
     DB = splitFrom(B->data,leafIdLength(B),B->width,i);
     leafIdDestroy(B);
     return DB;
   }

       // balance by rebuilding: flattening + splitting
        // assumes B is dynamic

static inline int canBalance (uint64_t n, uint width, int dleft, int dright)

   { uint b = leafIdNewSize(width); // size of leaves to create in split
     uint64_t left = (((n+b-1)/b)/2)*b; // size of left part
     uint64_t right = n-left; // size of right part
     if (left+dleft > AlphaFactor*(n+dleft+dright)) return 0;
     if (right+dright > AlphaFactor*(n+dleft+dright)) return 0;
     return 1;
   }

static void balance (hybridId B, uint64_t i, int64_t *delta)

   { uint64_t len = hybridIdLength(B);
     uint width = hybridIdWidth(B);
     uint64_t *D;
     *delta = - hybridIdLeaves(B);
     D = collect(B,len,width);
     B->bv.dyn = splitFrom(D,len,width,i);
     *delta += hybridIdLeaves(B);
     myfree(D);
   }

        // splits a full leaf into two
        // returns a hybridId and destroys B

static dynamicId splitLeaf (leafId B)

   { hybridId HB,HB1,HB2;
     dynamicId DB;
     leafId LB;
     uint bnum;
     uint64_t *segment;

     bnum = leafIdMaxSize(B->width) / 2; // elements in new leaves
     LB = leafIdCreateFromPacked(B->data,0,bnum,B->width);
     HB1 = (hybridId)myalloc(sizeof(struct s_hybridId));
     HB1->type = tLeaf;
     HB1->bv.leaf = LB;
     LB = leafIdCreateFromPacked(B->data,bnum,bnum,B->width);
     HB2 = (hybridId)myalloc(sizeof(struct s_hybridId));
     HB2->type = tLeaf;
     HB2->bv.leaf = LB;
     DB = (dynamicId)myalloc(sizeof(struct s_dynamicId));
     DB->size = B->size;
     DB->width = B->width;
     DB->accesses = 0;
     DB->leaves = 2;
     DB->left = HB1;
     DB->right = HB2;
     leafIdDestroy(B);
     return DB;
   }

        // merge the two leaf children of B into a leaf
        // returns a leafId and destroys B

static leafId mergeLeaves (dynamicId B)

   { leafId LB1,LB2;

     LB1 = B->left->bv.leaf;
     LB2 = B->right->bv.leaf;
     copyBits(LB1->data,LB1->size*B->width,LB2->data,0,LB2->size*B->width);
     LB1->size += LB2->size;
     leafIdDestroy(LB2);
     myfree(B);
     return LB1;
   }

        // transfers elements from the right to the left leaves children of B
	// tells if it transferred something

static int transferLeft (dynamicId B)

   { leafId LB1,LB2;
     uint i,trf;
     uint64_t *segment;

     LB1 = B->left->bv.leaf;
     LB2 = B->right->bv.leaf;
     trf = (LB2->size-LB1->size+1)/2;
     if (trf < leafIdMaxSize(B->width) * TrfFactor) return 0;
     copyBits(LB1->data,LB1->size*B->width,LB2->data,0,trf*B->width);
     LB1->size += trf;
     LB2->size -= trf;
     segment = (uint64_t*)myalloc(((leafIdMaxSize(B->width)*B->width+w64-1)/w64)
				  * sizeof(uint64_t));
     copyBits(segment,0,LB2->data,trf*B->width,LB2->size*B->width);
     memcpy(LB2->data,segment,(LB2->size*B->width+7)/8);
     myfree(segment);
     return 1;
   }

        // transfers elems from the left to the right leaves children of B
	// tells if it transferred something

static int transferRight (dynamicId B)

   { leafId LB1,LB2;
     uint i,trf;
     uint64_t *segment;

     LB1 = B->left->bv.leaf;
     LB2 = B->right->bv.leaf;
     trf = (LB1->size-LB2->size+1)/2;
     if (trf < leafIdMaxSize(B->width) * TrfFactor) return 0;
     segment = (uint64_t*)myalloc(((leafIdMaxSize(B->width)*B->width+w64-1)/w64)
				  *sizeof(uint64_t));
     memcpy(segment,LB2->data,(LB2->size*B->width+7)/8);
     copyBits(LB2->data,0,LB1->data,(LB1->size-trf)*B->width,trf*B->width);
     copyBits(LB2->data,trf*B->width,segment,0,LB2->size*B->width);
     LB1->size -= trf;
     LB2->size += trf;
     myfree(segment);
     return 1;
   }

	// writes B to file, which must be opened for writing

void hybridIdSave (hybridId B, FILE *file)

   { int64_t delta;
     flatten(B,&delta);
	// not as elegant as I thought :-)
     if (B->type == tStatic) leafIdSave(B->bv.stat,file);
     else leafIdSave(B->bv.leaf,file);
   }

	// loads hybridId from file, which must be opened for reading

hybridId hybridIdLoad (FILE *file)

   { leafId LB;
     hybridId B = myalloc(sizeof(struct s_hybridId));
     LB = leafIdLoad(file);
	// not as elegant as I thought :-)
     if (LB->isStat)
        { B->type = tStatic;
          B->bv.stat = LB;
        }
     else
        { B->type = tLeaf;
          B->bv.leaf = LB;
        }
     return B;
   }

	// gives space of hybridId in w-bit words

uint64_t hybridIdSpace (hybridId B)

   { uint64_t s;
     s = (sizeof(struct s_hybridId)*8+w64-1)/w64;
     if (B->type == tLeaf) return s+leafIdSpace(B->bv.leaf);
     else if (B->type == tStatic) return s+leafIdSpace(B->bv.stat);
     return s+(sizeof(struct s_dynamicId)*8+w64-1)/w64+
            hybridIdSpace(B->bv.dyn->left)+hybridIdSpace(B->bv.dyn->right);
   }


	// gives number of elements 

extern inline uint64_t hybridIdLength (hybridId B)

   { if (B->type == tLeaf) return leafIdLength(B->bv.leaf);
     if (B->type == tStatic) return leafIdLength(B->bv.stat);
     return B->bv.dyn->size;
   }

        // gives width of elements 

extern inline uint hybridIdWidth (hybridId B)

   { if (B->type == tStatic) return B->bv.stat->width;
     if (B->type == tLeaf) return B->bv.leaf->width;
     return B->bv.dyn->width;
   }

        // checks balance of B, assumed to be dynamic, and rotates once if needs

	// sets value for B[i] = v, assumes i is right and v fits in width

void hybridIdWrite (hybridId B, uint64_t i, uint64_t v)

   { uint64_t lsize;
     int dif;
     if (B->type == tStatic) {
        B->type = tDynamic;
        B->bv.dyn = split(B->bv.stat,i);
        }
     if (B->type == tLeaf)
        return leafIdWrite(B->bv.leaf,i,v);
     B->bv.dyn->accesses = 0; // reset
     lsize = hybridIdLength(B->bv.dyn->left);
     if (i < lsize) hybridIdWrite(B->bv.dyn->left,i,v);
     else hybridIdWrite(B->bv.dyn->right,i-lsize,v);
   }

        // changing leaves is uncommon and only then we need to recompute
        // leaves. we do our best to avoid this overhead in typical operations

static void irecompute (hybridId B, uint64_t i)

   { uint64_t lsize;
     if (B->type == tDynamic) {
        lsize = hybridIdLength(B->bv.dyn->left);
        if (i < lsize) irecompute(B->bv.dyn->left,i);
        else irecompute(B->bv.dyn->right,i-lsize);
        B->bv.dyn->leaves = hybridIdLeaves(B->bv.dyn->left) +
                            hybridIdLeaves(B->bv.dyn->right);
        }
   }

static void rrecompute (hybridId B, uint64_t i, uint64_t l)

   { uint64_t lsize;
     if (B->type == tDynamic) {
        lsize = hybridIdLength(B->bv.dyn->left);
        if (i+l < lsize) rrecompute(B->bv.dyn->left,i,l);
        else if (i >= lsize) rrecompute(B->bv.dyn->right,i-lsize,l);
        else { rrecompute(B->bv.dyn->left,i,lsize-i);
               rrecompute(B->bv.dyn->right,0,l-(lsize-i));
             }
        B->bv.dyn->leaves = hybridIdLeaves(B->bv.dyn->left) +
                            hybridIdLeaves(B->bv.dyn->right);
        }
   }

	// inserts v at B[i], assumes i is right and v fits in width

void insert (hybridId B, uint64_t i, uint64_t v, uint *recalc)

   { uint64_t lsize,rsize;
     int64_t delta;
     uint width;
     if (B->type == tStatic) {
        B->type = tDynamic;
        B->bv.dyn = split(B->bv.stat,i); // does not change #leaves!
        }
     if (B->type == tLeaf) {
        if (leafIdLength(B->bv.leaf) == leafIdMaxSize(B->bv.leaf->width))//split
           { B->type = tDynamic;
             B->bv.dyn = splitLeaf(B->bv.leaf);
	     *recalc = 1; // leaf added
           }
        else
           { leafIdInsert(B->bv.leaf,i,v);
             return;
           }
        }
     B->bv.dyn->accesses = 0; // reset
     width = B->bv.dyn->width;
     lsize = hybridIdLength(B->bv.dyn->left);
     rsize = hybridIdLength(B->bv.dyn->right);
     if (i < lsize) {
        if ((lsize == leafIdMaxSize(width))      // will overflow if leaf
             && (rsize < leafIdMaxSize(width))   // can avoid if leaf
             && (B->bv.dyn->left->type == tLeaf) // both are leaves
             && (B->bv.dyn->right->type == tLeaf)
             && transferRight(B->bv.dyn)) { // avoided, transferred to the right
	   insert(B,i,v,recalc);
	   return;
	   }
        if ((lsize+1 > AlphaFactor*(lsize+rsize+1))
             && (lsize+rsize >= MinLeavesToBalance*leafIdMaxSize(width)) 
	     && canBalance(lsize+rsize,width,1,0)) { // too biased
	   delta = 0;
           balance(B,i,&delta);
	   if (delta) *recalc = 1;
           insert(B,i,v,recalc);
           return;
           }
        insert(B->bv.dyn->left,i,v,recalc);
        }
     else {
        if ((rsize == leafIdMaxSize(width))       // will overflow if leaf
             && (lsize < leafIdMaxSize(width))    // can avoid if leaf
	     && (B->bv.dyn->right->type == tLeaf) // both are leaves
             && (B->bv.dyn->left->type == tLeaf) 
             && transferLeft(B->bv.dyn)) { // avoided, transferred to the left
	   insert(B,i,v,recalc);
	   return;
	   }
        if ((rsize+1 > AlphaFactor*(lsize+rsize+1)) 
             && (lsize+rsize >= MinLeavesToBalance*leafIdMaxSize(width)) 
	     && canBalance(lsize+rsize,width,0,1))    { // too biased
	   delta = 0;
           balance(B,i,&delta);
	   if (delta) *recalc = 1;
           insert(B,i,v,recalc);
           return;
           }
        insert(B->bv.dyn->right,i-lsize,v,recalc);
        }
     B->bv.dyn->size++;
   }

void hybridIdInsert (hybridId B, uint64_t i, uint64_t v)

   { uint recalc = 0;
     insert(B,i,v,&recalc);
     if (recalc) irecompute(B,i); // we went to the leaf now holding i
   }

	// deletes B[i], assumes i is right

static void delete (hybridId B, uint64_t i, uint *recalc)

   { uint64_t lsize,rsize;
     hybridId B2;
     int64_t delta;
     uint width;
     if (B->type == tStatic) {
        B->type = tDynamic;
        B->bv.dyn = split(B->bv.stat,i);
        }
     if (B->type == tLeaf)
        return leafIdDelete(B->bv.leaf,i);
     B->bv.dyn->accesses = 0; // reset
     width = B->bv.dyn->width;
     lsize = hybridIdLength(B->bv.dyn->left);
     rsize = hybridIdLength(B->bv.dyn->right);
     if (i < lsize) {  
        if ((rsize > AlphaFactor*(lsize+rsize-1)) 
             && (lsize+rsize >= MinLeavesToBalance*leafIdMaxSize(width)) 
	     && canBalance(lsize+rsize,width,-1,0))   { // too biased
	   delta = 0;
           balance(B,i,&delta);
	   if (delta) *recalc = 1;
           delete(B,i,recalc);
	   return;
           }
        delete(B->bv.dyn->left,i,recalc);
        if (lsize == 1) { // left child is now of size zero, remove
           hybridIdDestroy(B->bv.dyn->left);
           B2 = B->bv.dyn->right;
           myfree(B->bv.dyn);
           *B = *B2;
	   *recalc = 1;
           return;
           }
        }
     else { 
        if ((lsize > AlphaFactor*(lsize+rsize-1)) 
             && (lsize+rsize >= MinLeavesToBalance*leafIdMaxSize(width)) 
	     && canBalance(lsize+rsize,width,0,-1))   { // too biased
	   delta = 0;
           balance(B,i,&delta);
	   if (delta) *recalc = 1;
           delete(B,i,recalc);
	   return;
           }
        delete(B->bv.dyn->right,i-lsize,recalc);
        if (rsize == 1) { // right child is now of size zero, remove
           hybridIdDestroy(B->bv.dyn->right);
           B2 = B->bv.dyn->left;
           myfree(B->bv.dyn);
           *B = *B2;
	   *recalc = 1;
           return;
           }
        }
     B->bv.dyn->size--;
     if (B->bv.dyn->size <= leafIdNewSize(B->bv.dyn->width)){ // merge leaves
        B->bv.leaf = mergeLeaves(B->bv.dyn);
        B->type = tLeaf;
	*recalc = 1;
        }
     else if (B->bv.dyn->size <
              B->bv.dyn->leaves * leafIdNewSize(B->bv.dyn->width) * MinFillFactor) {
        delta = 0;
        flatten(B,&delta);
        if (delta) *recalc = 1;
        }
   }

void hybridIdDelete (hybridId B, uint64_t i)

   { uint recalc = 0;
     delete(B,i,&recalc);
     if (recalc) { // the node is now at i-1 or at i, hard to know
        irecompute(B,i-1);
        irecompute(B,i);
        }
   }

        // flattening is uncommon and only then we need to recompute
        // leaves. we do our best to avoid this overhead in typical queries

static void recompute (hybridId B, uint64_t i, int64_t delta)

   { uint64_t lsize;
     if (B->type == tDynamic) {
        B->bv.dyn->leaves += delta;
        lsize = hybridIdLength(B->bv.dyn->left);
        if (i < lsize) recompute(B->bv.dyn->left,i,delta);
        else recompute(B->bv.dyn->right,i-lsize,delta);
        }
   }

	// access B[i], assumes i is right

uint64_t access (hybridId B, uint64_t i, int64_t *delta)

   { uint64_t lsize;
     if (B->type == tDynamic)
        { B->bv.dyn->accesses++;
	  if (mustFlatten(B))
               flatten(B,delta);
          else
             { lsize = hybridIdLength(B->bv.dyn->left);
               if (i < lsize) return access(B->bv.dyn->left,i,delta);
               return access(B->bv.dyn->right,i-lsize,delta);
             }
        }
     if (B->type == tLeaf) return leafIdAccess(B->bv.leaf,i);
     return leafIdAccess(B->bv.stat,i);
   }

uint64_t hybridIdAccess (hybridId B, uint64_t i)

   { int64_t delta = 0;
     uint64_t answ = access(B,i,&delta);
     if (delta) recompute(B,i,delta);
     return answ;
   }

        // read values [i..i+l-1], onto D[0...], of uint64_t

static void sread64 (hybridId B, uint64_t i, uint64_t l, uint64_t *D, 
		     uint *recomp)

   { uint64_t lsize;
     int64_t delta;
     if (B->type == tDynamic)
        { B->bv.dyn->accesses++;
	  if (mustFlatten(B)) {
             delta = 0;
	     flatten(B,&delta);
	     if (delta) *recomp = 1;
	     }
          else {
            lsize = hybridIdLength(B->bv.dyn->left);
            if (i+l < lsize) sread64(B->bv.dyn->left,i,l,D,recomp);
            else if (i >= lsize) sread64(B->bv.dyn->right,i-lsize,l,D,recomp);
            else { sread64(B->bv.dyn->left,i,lsize-i,D,recomp);
                   sread64(B->bv.dyn->right,0,l-(lsize-i),D+(lsize-i),recomp);
                 }
            return;
            }
        }
     if (B->type == tLeaf) leafIdRead64(B->bv.leaf,i,l,D);
     else leafIdRead64(B->bv.stat,i,l,D);
   }

void hybridRead64 (hybridId B, uint64_t i, uint64_t l, uint64_t *D)

   { uint recomp = 0; 
     sread64(B,i,l,D,&recomp);
     if (recomp) rrecompute(B,i,l);
   }

        // read values [i..i+l-1], onto D[0...], of uint32_t

static void sread32 (hybridId B, uint64_t i, uint64_t l, uint32_t *D, 
		     uint *recomp)

   { uint64_t lsize;
     int64_t delta;
     if (B->type == tDynamic)
        { B->bv.dyn->accesses++;
	  if (mustFlatten(B)) {
             delta = 0;
             flatten(B,&delta);
             if (delta) *recomp = 1;
             }
          else {
            lsize = hybridIdLength(B->bv.dyn->left);
            if (i+l < lsize) sread32(B->bv.dyn->left,i,l,D,recomp);
            else if (i >= lsize) sread32(B->bv.dyn->right,i-lsize,l,D,recomp);
            else { sread32(B->bv.dyn->left,i,lsize-i,D,recomp);
                   sread32(B->bv.dyn->right,0,l-(lsize-i),D+(lsize-i),recomp);
                 }
            return;
            }
        }
     if (B->type == tLeaf) leafIdRead32(B->bv.leaf,i,l,D);
     else leafIdRead32(B->bv.stat,i,l,D);
   }

void hybridRead32 (hybridId B, uint64_t i, uint64_t l, uint32_t *D)

   { uint recomp = 0;
     sread32(B,i,l,D,&recomp);
     if (recomp) rrecompute(B,i,l);
   }


