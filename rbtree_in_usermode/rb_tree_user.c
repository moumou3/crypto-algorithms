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
  struct rb_root root_stable_tree;
  struct rb_root root_unstable_tree;
  char* hash;
};

static struct rb_root root_hash_tree = RB_ROOT;


struct hash_node* hash_tree_search_insert(char *hash) {
  struct rb_node **new;
  struct rb_root *root;
  struct rb_node *parent = NULL;
  struct hash_node *hash_node;
  struct hash_node *tree_hash_node;

  root = &root_hash_tree;
  new = &root->rb_node;
  while (*new) {
    struct hash_node *tree_hash_node;
    char *tree_hash;
    int ret;
    cond_resched();
    tree_hash_node = rb_entry(*new, struct hash_node, node);
    ret = memcmp(hash, tree_hash_node->hash, SHA1_BLOCK_SIZE);

    parent = *new;
    if (ret < 0) {
      new = &parent->rb_left;
    } else if (ret > 0) {
      new = &parent->rb_right;
    } else {
      return  tree_hash_node;
    }
  }

  hash_node = kmalloc(sizeof(struct hash_node), GFP_KERNEL);
  if (hash_node) {
    hash_node->root_stable_tree = kmalloc(sizeof(struct rb_root), GFP_KERNEL);
    hash_node->root_unstable_tree = kmalloc(sizeof(struct rb_root), GFP_KERNEL);
    hash_node->root_stable_tree = RB_ROOT;
    hash_node->root_unstable_tree = RB_ROOT;
    hash_node->hash = hash;
  }
  rb_link_node(&hash_node->node, parent, new);
  rb_insert_color(&hash_node->node, root);
  return hash_node;


}

int main() {
  struct rmap_item *rmap_item = argitem;
  struct page *page = argpage;
  struct rb_node *root_stable_tree;
  struct page *kpage;
  struct stable_node stable_node;
  hash_node = hash_tree_search_insert(hash);
  stable_node = page_stable_node(page);
  if (stable_node && rmap_item->head == stable_node) {
    return;
  }
  kpage = stable_tree_search(page, hashnode->root_stable_tree);
  if (kpage == page && rmap_item->head == stable_node) {
    put_page(kpage);
    return;
  }
  remove_rmap_item_from_tree(rmap_item, hashnode->root_unstable_tree);
  if (kpage) {
    err = try_to_merge_with_ksm_page(rmap_item, page, kpage);
    if (!err) {
      lock_page(kpage);
      stable_tree_append(rmap_item, page_stable_node(kpage));
      unlock_page(kpage);
    }
    put_page(kpage);
    return;
  }

  checksum = calc_checksum(page);
  if (rmap_item->oldchecksum != checksum) {
    rmap_item->oldchecksum = checksum;
    return;
  }
  if (ksm_use_zero_pages &&  (checksum == zero_checksum)) {
    down_read(&mm->mmap_sem);
    vma = find_mergeable_vma(mm, rmap_item->address);
    err = try_to_merge_one_page(vma, page,
                                ZERO_PAGE(rmap_item->address));
    up_read(&mm->mmap_sem);
    /*
     * In case of failure, the page was not really empty, so we
     * need to continue. Otherwise we're done.
     */
    if (!err)
      return;
  }
  tree_rmap_item = unstable_tree_search_insert(rmap_item, page, &tree_page, hash_node->root_unstable_tree);
  if (tree_rmap_item) {
    kpage = try_to_merge_two_pages(rmap_item, page, tree_rmap_item, tree_page);
    put_page(tree_page);
    if (kpage) {
      lock_page(page);
      stable_node = stable_tree_insert(kpage, hashnode->root_stable_tree);
      if (stable_node) {
        stable_tree_append(tree_rmap_item, stable_node);
        stable_tree_append(rmap_item, stable_node);
      }
      unlock_page(kpage);
      if (!stable_node) {
        break_cow(tree_rmap_item);
        break_cow(rmap_item);
      }
    }
  }




  return 0;
}
