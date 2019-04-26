#include <stdio.h>
#include <stdlib.h>


struct stable_node {
  struct rb_node node;
  struct hlist_head hlist;
  unsigned long kpfn;
};

struct rmap_item {
  struct rmap_item *rmap_list;
  //struct mm_struct *mm;
  unsigned long address;
  unsigned int oldchecksum;
  union {
    struct rb_node node;
    struct {
      struct stable_node *head;
      struct hlist_node hlist;
    };
  };
};
struct hash_node {
  struct rb_node node;
  struct rb_node root_stable_tree;
  struct rb_node root_unstable_tree;
  char* hash;
};

static struct rb_node *root_stable_tree = NULL;
static struct rb_node *root_unstable_tree = NULL;


struct root_stable_tree hash_tree_search_insert(char *hash) {

}

int main() {
 struct rmap_item *rmap_item = argitem;
 struct page *page = argpage;
  struct rb_node *root_stable_tree;
  struct page *kpage;
  stable_node = page_stable_node(page);
  root_stable_tree = hash_tree_search_insert(hash);
  kpage = stable_tree_search(page, root_stable_tree);
  if (kpage == page && rmap_item->head == stable_node) {
    put_page(kpage);
    return;
  }
  
    


  



  return 0;
}
