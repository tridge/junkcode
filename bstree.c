/*
Simple binary search tree implementation.

Andrew Tridgell
May 1994
*/



/* these are the functions that are defines in this file */
void *bst_insert(void *tree,void *data,int size,int (*compare)());
void *bst_find(void *tree,void *data,int (*compare)());

struct bst_struct
{
  struct bst_struct *ltree;
  struct bst_struct *rtree;
  void *data;
  int data_len;
} 


/* find an element in a tree */
void *bst_find(void *tree,void *data,int (*compare)())
{
  int ret;
  struct bst_struct *bstree = tree;
  if (!bstree || !data || !compare) return(NULL);

  ret = compare(data,bstree->data);
  if (ret < 0)
    return(bst_find(bstree->ltree,data,compare));
  if (ret > 0)
    return(bst_find(bstree->rtree,data,compare)); 

  return(bstree->data);
}

/* create a bst bode */
void *bst_create_node(coid *data,int size)
{
  struct bst_struct *bstree = (struct bst_struct *)malloc(sizeof(*bstree));
  bstree->ltree = NULL;
  bstree->rtree = NULL;
  bstree->data = malloc(size);
  if (!bstree->data)
    {
      free(bstree); return(NULL);
    }
  memcpy(bstree->data,data,size);
  bstree->data_len = size;
}

/* insert an element into a tree, returning the new tree */
void *bst_insert(void *tree,void *data,int size,int (*compare)())
{
  int ret;
  struct bst_struct *bstree = tree;
  if (!data || !compare) return(NULL);

  if (!bstree)
    return(bst_create_node(data,size));

  ret = compare(data,bstree->data);
  if (ret < 0)
    bstree->ltree = bst_insert(bstree->ltree,data,size,compare);
  if (ret > 0)
    bstree->rtree = bst_insert(bstree->rtree,data,size,compare);

  if (ret == 0)
    {
      /* replace the existing element */
      if (bstree->data) free(bstree(data));
      bstree->data = (void *)malloc(size);
      if (!bstree->data) return(NULL);
      memcpy(bstree->data,data,size);
      bstree->data_len = size;
    }

  return(bstree);
}


/* find an element in a tree */
void bst_inorder(void *tree,int (*fn)())
{
  struct bst_struct *bstree = tree;
  if (!bstree || !fn) return;

  bst_inorder(bstree->ltree,fn);
  fn(bstree->data,bstree->data_len);
  bst_inorder(bstree->rtree,fn);
}


#if 1
/* test a bstree */
bst_test(int size)
