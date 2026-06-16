#![no_std]
#![no_main]

use core::ptr;

pub type size_t = usize;
pub type uint16_t = u16;
pub type uint32_t = u32;
pub type uint64_t = u64;
pub type int32_t = i32;

// ============================================================
// Slab allocator helpers
// ============================================================

const SLAB_CLASSES: usize = 10;
const SLAB_SIZES: [usize; 10] = [16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192];

#[repr(C)]
struct slab_free {
    next: *mut slab_free,
}

#[repr(C)]
struct slab_page {
    next: *mut slab_page,
    class_idx: uint16_t,
    free_count: uint16_t,
    total_count: uint16_t,
    free_list: *mut slab_free,
}

/// Find slab class index via binary search (O(log n) vs C's O(n) linear scan)
#[no_mangle]
pub unsafe extern "C" fn rust_slab_find_class(size: size_t) -> int32_t {
    if size == 0 || size > SLAB_SIZES[SLAB_CLASSES - 1] {
        return -1;
    }
    let mut lo = 0usize;
    let mut hi = SLAB_CLASSES;
    while lo < hi {
        let mid = (lo + hi) / 2;
        if SLAB_SIZES[mid] < size {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if lo < SLAB_CLASSES && SLAB_SIZES[lo] >= size {
        lo as int32_t
    } else {
        -1
    }
}

/// Initialize a 4KB slab page, building the free list in-place
#[no_mangle]
pub unsafe extern "C" fn rust_slab_page_init(page: *mut u8, class_idx: uint32_t) -> int32_t {
    let block_size = SLAB_SIZES[class_idx as usize];
    let header_size = (core::mem::size_of::<slab_page>() + 15) & !15;
    if block_size == 0 || header_size >= 4096 {
        return -1;
    }
    let num_blocks = (4096 - header_size) / block_size;
    if num_blocks == 0 {
        return -1;
    }
    let sp = page as *mut slab_page;
    ptr::write(&mut (*sp).next, ptr::null_mut());
    ptr::write(&mut (*sp).class_idx, class_idx as uint16_t);
    ptr::write(&mut (*sp).free_count, num_blocks as uint16_t);
    ptr::write(&mut (*sp).total_count, num_blocks as uint16_t);
    let data = page.add(header_size);
    let mut head: *mut slab_free = ptr::null_mut();
    let mut i = num_blocks;
    while i > 0 {
        i -= 1;
        let block = data.add(i * block_size) as *mut slab_free;
        ptr::write(&mut (*block).next, head);
        head = block;
    }
    ptr::write(&mut (*sp).free_list, head);
    0
}

/// Allocate one block from a slab page, zeroed
#[no_mangle]
pub unsafe extern "C" fn rust_slab_alloc(sp: *mut slab_page) -> *mut u8 {
    if sp.is_null() || (*sp).free_list.is_null() || (*sp).free_count == 0 {
        return ptr::null_mut();
    }
    let block = (*sp).free_list as *mut u8;
    ptr::write(&mut (*sp).free_list, (*(*sp).free_list).next);
    ptr::write(&mut (*sp).free_count, (*sp).free_count - 1);
    let block_size = SLAB_SIZES[(*sp).class_idx as usize];
    rust_zero_mem(block, block_size);
    block
}

/// Free a block back to its slab page (poisons with 0xCC)
#[no_mangle]
pub unsafe extern "C" fn rust_slab_free(sp: *mut slab_page, ptr: *mut u8) {
    if (*sp).free_count >= (*sp).total_count { return; } /* double-free guard */
    let block = ptr as *mut slab_free;
    let block_size = SLAB_SIZES[(*sp).class_idx as usize];
    rust_memset(ptr, 0xCC, block_size);
    ptr::write(&mut (*block).next, (*sp).free_list);
    ptr::write(&mut (*sp).free_list, block);
    ptr::write(&mut (*sp).free_count, (*sp).free_count + 1);
}

/// Check if pointer belongs to a slab page (fast range check)
#[no_mangle]
pub unsafe extern "C" fn rust_slab_contains(sp: *mut slab_page, ptr: *mut u8) -> int32_t {
    let start = sp as *mut u8;
    if ptr >= start && ptr.offset_from(start) < 4096 {
        1
    } else {
        0
    }
}

// ============================================================
// Fixed-size hash map (open addressing, Robin Hood)
// Used by ARP cache for O(1) lookup replacing O(n) linear scan
// ============================================================

const HASHMAP_EMPTY: u64 = 0;
const HASHMAP_DELETED: u64 = 1;

#[repr(C)]
pub struct hashmap_entry {
    pub key: u64,
    pub value: u64,
    pub dist: u32, /* probe distance */
}

#[repr(C)]
pub struct hashmap_state {
    pub entries: *mut hashmap_entry,
    pub capacity: u32,
    pub size: u32,
    pub mask: u32,
}

#[no_mangle]
pub unsafe extern "C" fn rust_hashmap_init(
    entries: *mut hashmap_entry,
    capacity: u32,
) -> hashmap_state {
    /* Round up to next power of 2 */
    let mut cap = if capacity == 0 { 1u32 } else { capacity };
    cap -= 1;
    cap |= cap >> 1;
    cap |= cap >> 2;
    cap |= cap >> 4;
    cap |= cap >> 8;
    cap |= cap >> 16;
    cap += 1;
    let mask = cap - 1;
    let mut i = 0u32;
    while i < cap {
        (*entries.add(i as usize)).key = HASHMAP_EMPTY;
        (*entries.add(i as usize)).value = 0;
        (*entries.add(i as usize)).dist = 0;
        i += 1;
    }
    hashmap_state {
        entries,
        capacity: cap,
        size: 0,
        mask,
    }
}

#[no_mangle]
pub unsafe extern "C" fn rust_hashmap_get(
    state: *mut hashmap_state,
    key: u64,
) -> int32_t {
    if key <= HASHMAP_DELETED {
        return -1;
    }
    let st = &mut *state;
    let mut idx = rust_hash_u64(key, st.capacity) as usize;
    let mut dist = 0u32;
    loop {
        let entry = &*st.entries.add(idx);
        if entry.key == HASHMAP_EMPTY {
            return -1;
        }
        if entry.key == key {
            return entry.value as int32_t;
        }
        if dist > entry.dist {
            return -1;
        }
        idx = (idx + 1) & (st.mask as usize);
        dist += 1;
        if dist > st.capacity {
            return -1;
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn rust_hashmap_insert(
    state: *mut hashmap_state,
    key: u64,
    value: u64,
) -> int32_t {
    if key <= HASHMAP_DELETED {
        return -1;
    }
    let st = &mut *state;
    if st.size * 4 >= st.capacity * 3 {
        return -1;
    }
    let mut idx = rust_hash_u64(key, st.capacity) as usize;
    let mut dist = 0u32;
    let mut insert_key = key;
    let mut insert_val = value;
    loop {
        let entry = &mut *st.entries.add(idx);
        if entry.key == HASHMAP_EMPTY || entry.key == HASHMAP_DELETED {
            entry.key = insert_key;
            entry.value = insert_val;
            entry.dist = dist;
            st.size += 1;
            return 0;
        }
        if entry.key == key {
            entry.value = value;
            return 0;
        }
        if dist > entry.dist {
            /* Robin Hood: swap */
            let tmp_key = entry.key;
            let tmp_val = entry.value;
            let tmp_dist = entry.dist;
            entry.key = insert_key;
            entry.value = insert_val;
            entry.dist = dist;
            insert_key = tmp_key;
            insert_val = tmp_val;
            dist = tmp_dist;
        }
        idx = (idx + 1) & (st.mask as usize);
        dist += 1;
        if dist > st.capacity {
            return -1;
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn rust_hashmap_remove(
    state: *mut hashmap_state,
    key: u64,
) -> int32_t {
    if key <= HASHMAP_DELETED {
        return -1;
    }
    let st = &mut *state;
    let mut idx = rust_hash_u64(key, st.capacity) as usize;
    let mut dist = 0u32;
    loop {
        let entry = &mut *st.entries.add(idx);
        if entry.key == HASHMAP_EMPTY {
            return -1;
        }
        if entry.key == key {
            entry.key = HASHMAP_DELETED;
            entry.dist = 0;
            st.size -= 1;
            /* Backward shift deletion */
            let mut next = (idx + 1) & (st.mask as usize);
            while {
                let ne = &*st.entries.add(next);
                ne.key != HASHMAP_EMPTY && ne.dist > 0
            } {
                let ne = &mut *st.entries.add(next);
                let pe = &mut *st.entries.add(idx);
                pe.key = ne.key;
                pe.value = ne.value;
                pe.dist = ne.dist - 1;
                ne.key = HASHMAP_DELETED;
                ne.dist = 0;
                idx = next;
                next = (next + 1) & (st.mask as usize);
            }
            return 0;
        }
        if dist > entry.dist {
            return -1;
        }
        idx = (idx + 1) & (st.mask as usize);
        dist += 1;
        if dist > st.capacity {
            return -1;
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn rust_hashmap_size(state: *const hashmap_state) -> u32 {
    (*state).size
}

// ============================================================
// Optimized memory operations
// ============================================================

/// Copy memory using 4-byte aligned accesses
#[no_mangle]
pub unsafe extern "C" fn rust_memcpy(dst: *mut u8, src: *const u8, n: size_t) -> *mut u8 {
    let mut i = 0usize;
    while i + 4 <= n {
        *(dst.add(i) as *mut uint32_t) = *(src.add(i) as *const uint32_t);
        i += 4;
    }
    while i < n {
        *dst.add(i) = *src.add(i);
        i += 1;
    }
    dst
}

/// Set memory using 4-byte aligned stores
#[no_mangle]
pub unsafe extern "C" fn rust_memset(s: *mut u8, c: int32_t, n: size_t) -> *mut u8 {
    let b = c as u8;
    let pattern = (b as uint32_t) * 0x01010101;
    let mut i = 0usize;
    while i + 4 <= n {
        *(s.add(i) as *mut uint32_t) = pattern;
        i += 4;
    }
    while i < n {
        *s.add(i) = b;
        i += 1;
    }
    s
}

/// Compare memory using 8-byte aligned comparisons
#[no_mangle]
pub unsafe extern "C" fn rust_memcmp(s1: *const u8, s2: *const u8, n: size_t) -> int32_t {
    let mut i = 0usize;
    while i + 8 <= n {
        let a = *(s1.add(i) as *const uint64_t);
        let b = *(s2.add(i) as *const uint64_t);
        if a != b {
            for j in 0..8 {
                let va = *s1.add(i + j);
                let vb = *s2.add(i + j);
                if va != vb {
                    return (va as int32_t) - (vb as int32_t);
                }
            }
        }
        i += 8;
    }
    while i < n {
        let va = *s1.add(i);
        let vb = *s2.add(i);
        if va != vb {
            return (va as int32_t) - (vb as int32_t);
        }
        i += 1;
    }
    0
}

/// Zero memory using 8-byte stores
#[no_mangle]
pub unsafe extern "C" fn rust_zero_mem(ptr: *mut u8, len: size_t) {
    let mut i = 0usize;
    while i + 8 <= len {
        *(ptr.add(i) as *mut uint64_t) = 0;
        i += 8;
    }
    while i < len {
        *ptr.add(i) = 0;
        i += 1;
    }
}

// ============================================================
// Cache hash functions (FNV-1a, better distribution)
// ============================================================

/// Hash a u64 key -> bucket index using Murmur3 finalizer
#[no_mangle]
pub unsafe extern "C" fn rust_hash_u64(key: uint64_t, table_size: uint32_t) -> uint32_t {
    let mut h = key;
    h ^= h >> 33;
    h = h.wrapping_mul(0xFF51AFD7ED558CCDu64);
    h ^= h >> 33;
    h = h.wrapping_mul(0xC4CEB9FE1A85EC53u64);
    h ^= h >> 33;
    (h as uint32_t) % table_size
}

/// Hash a null-terminated string -> u64 using FNV-1a
#[no_mangle]
pub unsafe extern "C" fn rust_hash_string(str: *const u8) -> uint64_t {
    if str.is_null() {
        return 0;
    }
    let mut hash: uint64_t = 0xCBF29CE484222325;
    let mut i = 0usize;
    loop {
        let c = *str.add(i);
        if c == 0 {
            break;
        }
        hash ^= c as uint64_t;
        hash = hash.wrapping_mul(0x100000001B3u64);
        i += 1;
    }
    hash
}

/// Hash arbitrary bytes -> u64 using FNV-1a
#[no_mangle]
pub unsafe extern "C" fn rust_hash_data(data: *const u8, size: size_t) -> uint64_t {
    if data.is_null() || size == 0 {
        return 0;
    }
    let mut hash: uint64_t = 0xCBF29CE484222325;
    for i in 0..size {
        hash ^= *data.add(i) as uint64_t;
        hash = hash.wrapping_mul(0x100000001B3u64);
    }
    hash
}

// ============================================================
// LRU cache with O(1) hash lookup + doubly-linked list
// Replaces O(n) linear scan in cache.c
// ============================================================

const LRU_EMPTY_KEY: u64 = 0;

#[repr(C)]
pub struct lru_node {
    pub key: u64,
    pub value: u64,
    pub prev: int32_t,
    pub next: int32_t,
    pub hash_next: int32_t,
}

#[repr(C)]
pub struct lru_cache {
    pub nodes: *mut lru_node,
    pub hash_table: *mut int32_t,
    pub hash_size: u32,
    pub hash_mask: u32,
    pub capacity: u32,
    pub size: u32,
    pub head: int32_t,
    pub tail: int32_t,
}

/// Initialize LRU cache. nodes/hash_table must be pre-allocated.
#[no_mangle]
pub unsafe extern "C" fn rust_lru_init(
    nodes: *mut lru_node,
    hash_table: *mut int32_t,
    hash_size: u32,
    capacity: u32,
) -> lru_cache {
    let mask = hash_size - 1;
    let mut i = 0u32;
    while i < hash_size {
        *hash_table.add(i as usize) = -1;
        i += 1;
    }
    i = 0;
    while i < capacity {
        (*nodes.add(i as usize)).key = LRU_EMPTY_KEY;
        (*nodes.add(i as usize)).prev = -1;
        (*nodes.add(i as usize)).next = -1;
        (*nodes.add(i as usize)).hash_next = -1;
        i += 1;
    }
    lru_cache {
        nodes,
        hash_table,
        hash_size,
        hash_mask: mask,
        capacity,
        size: 0,
        head: -1,
        tail: -1,
    }
}

/// Move node to front of LRU list
unsafe fn lru_move_front(state: *mut lru_cache, idx: int32_t) {
    let st = &mut *state;
    if st.head == idx {
        return;
    }
    let node = &mut *st.nodes.add(idx as usize);
    let prev = node.prev;
    let next = node.next;

    /* Unlink */
    if prev >= 0 {
        (*st.nodes.add(prev as usize)).next = next;
    }
    if next >= 0 {
        (*st.nodes.add(next as usize)).prev = prev;
    }
    if st.tail == idx {
        st.tail = prev;
    }

    /* Link at front */
    node.prev = -1;
    node.next = st.head;
    if st.head >= 0 {
        (*st.nodes.add(st.head as usize)).prev = idx;
    }
    st.head = idx;
    if st.tail < 0 {
        st.tail = idx;
    }
}

/// LRU hash function
unsafe fn lru_hash(state: *const lru_cache, key: u64) -> u32 {
    (rust_hash_u64(key, (*state).hash_size) & (*state).hash_mask)
}

/// Get value by key. Returns value or -1 if not found. Moves to front.
#[no_mangle]
pub unsafe extern "C" fn rust_lru_get(
    state: *mut lru_cache,
    key: u64,
) -> int32_t {
    if key == LRU_EMPTY_KEY {
        return -1;
    }
    let st = &mut *state;
    let h = lru_hash(st, key) as usize;
    let mut idx = *st.hash_table.add(h);
    while idx >= 0 {
        let node = &*st.nodes.add(idx as usize);
        if node.key == key {
            lru_move_front(state, idx);
            return (*st.nodes.add(idx as usize)).value as int32_t;
        }
        idx = node.hash_next;
    }
    -1
}

/// Insert or update key-value pair. Returns evicted value or -1.
#[no_mangle]
pub unsafe extern "C" fn rust_lru_put(
    state: *mut lru_cache,
    key: u64,
    value: u64,
) -> int32_t {
    if key == LRU_EMPTY_KEY {
        return -1;
    }
    let st = &mut *state;
    let h = lru_hash(st, key) as usize;

    /* Check if key exists */
    let mut idx = *st.hash_table.add(h);
    while idx >= 0 {
        let node = &mut *st.nodes.add(idx as usize);
        if node.key == key {
            node.value = value;
            lru_move_front(state, idx);
            return -1;
        }
        idx = node.hash_next;
    }

    /* Find free slot */
    let mut free_slot: int32_t = -1;
    let mut i = 0u32;
    while i < st.capacity {
        if (*st.nodes.add(i as usize)).key == LRU_EMPTY_KEY {
            free_slot = i as int32_t;
            break;
        }
        i += 1;
    }

    /* Evict LRU if no free slot */
    let mut evicted_value: int32_t = -1;
    if free_slot < 0 {
        let victim = st.tail;
        if victim < 0 {
            return -1;
        }
        let victim_node = &mut *st.nodes.add(victim as usize);
        evicted_value = victim_node.value as int32_t;

        /* Remove victim from hash */
        let vh = lru_hash(st, victim_node.key) as usize;
        let mut pp = &mut (*st.hash_table.add(vh)) as *mut int32_t;
        while *pp >= 0 {
            if *pp == victim {
                *pp = victim_node.hash_next;
                break;
            }
            pp = &mut (*st.nodes.add(*pp as usize)).hash_next;
        }

        /* Unlink victim from list */
        if victim_node.prev >= 0 {
            (*st.nodes.add(victim_node.prev as usize)).next = victim_node.next;
        }
        if victim_node.next >= 0 {
            (*st.nodes.add(victim_node.next as usize)).prev = victim_node.prev;
        }
        if st.head == victim {
            st.head = victim_node.next;
        }
        st.tail = victim_node.prev;
        if st.tail >= 0 {
            (*st.nodes.add(st.tail as usize)).next = -1;
        }

        free_slot = victim;
        st.size -= 1;
    }

    /* Insert at front */
    let node = &mut *st.nodes.add(free_slot as usize);
    node.key = key;
    node.value = value;
    node.prev = -1;
    node.next = st.head;
    node.hash_next = *st.hash_table.add(h);
    *st.hash_table.add(h) = free_slot;
    if st.head >= 0 {
        (*st.nodes.add(st.head as usize)).prev = free_slot;
    }
    st.head = free_slot;
    if st.tail < 0 {
        st.tail = free_slot;
    }
    st.size += 1;
    evicted_value
}

/// Remove key from cache. Returns removed value or -1.
#[no_mangle]
pub unsafe extern "C" fn rust_lru_remove(
    state: *mut lru_cache,
    key: u64,
) -> int32_t {
    if key == LRU_EMPTY_KEY {
        return -1;
    }
    let st = &mut *state;
    let h = lru_hash(st, key) as usize;
    let mut pp = &mut (*st.hash_table.add(h)) as *mut int32_t;
    while *pp >= 0 {
        let idx = *pp;
        let node = &mut *st.nodes.add(idx as usize);
        if node.key == key {
            let val = node.value as int32_t;
            *pp = node.hash_next;

            if node.prev >= 0 {
                (*st.nodes.add(node.prev as usize)).next = node.next;
            }
            if node.next >= 0 {
                (*st.nodes.add(node.next as usize)).prev = node.prev;
            }
            if st.head == idx {
                st.head = node.next;
            }
            if st.tail == idx {
                st.tail = node.prev;
            }
            node.key = LRU_EMPTY_KEY;
            node.prev = -1;
            node.next = -1;
            node.hash_next = -1;
            st.size -= 1;
            return val;
        }
        pp = &mut node.hash_next;
    }
    -1
}

#[no_mangle]
pub unsafe extern "C" fn rust_lru_size(state: *const lru_cache) -> u32 {
    (*state).size
}

// ============================================================
// Process table RAII: automatic cleanup on exit
// Prevents fd leaks, zombie accumulation, resource orphans
// ============================================================

/// Process file descriptor entry for cleanup tracking
#[repr(C)]
pub struct proc_fd_info {
    pub in_use: u8,
    pub fd_type: u8, /* 0=none, 1=file, 2=pipe, 3=socket, 4=tty */
}

/// Clean up all resources for an exiting process.
/// Returns number of resources cleaned up.
#[no_mangle]
pub unsafe extern "C" fn rust_proc_cleanup_fds(
    fds: *const proc_fd_info,
    max_fds: u32,
    close_fn: extern "C" fn(u32) -> int32_t,
) -> int32_t {
    let mut cleaned = 0i32;
    let mut i = 0u32;
    while i < max_fds {
        let fd = &*fds.add(i as usize);
        if fd.in_use != 0 && fd.fd_type != 0 {
            close_fn(i);
            cleaned += 1;
        }
        i += 1;
    }
    cleaned
}

/// Validate process resource ownership.
/// Returns 1 if ptr belongs to the given PID's page range, 0 otherwise.
#[no_mangle]
pub unsafe extern "C" fn rust_proc_owns_page(
    ptr: *const u8,
    proc_start: uint64_t,
    proc_size: uint64_t,
) -> int32_t {
    let addr = ptr as uint64_t;
    if addr >= proc_start && addr < proc_start + proc_size {
        1
    } else {
        0
    }
}

/// Clean up kernel-side buffers associated with a process.
/// Zeros the buffer and resets write pointer.
#[no_mangle]
pub unsafe extern "C" fn rust_proc_reset_buffer(
    buf: *mut u8,
    size: uint64_t,
    wr_ptr: *mut uint64_t,
) {
    rust_zero_mem(buf, size as usize);
    *wr_ptr = 0;
}

// ============================================================
// Ring buffer operations (safe wrappers matching kernel_util API)
// ============================================================

/// Push a byte to ring buffer. Returns 0 on success, -1 on full.
#[no_mangle]
pub unsafe extern "C" fn rust_ringbuf_push(
    buf: *mut u8,
    size: uint64_t,
    rd: *mut uint64_t,
    wr: *mut uint64_t,
    count: *mut uint64_t,
    byte: u8,
) -> int32_t {
    if *count >= size {
        return -1;
    }
    *buf.add((*wr) as usize) = byte;
    *wr = (*wr + 1) % size;
    *count += 1;
    0
}

/// Pop a byte from ring buffer. Returns 0 on success, -1 on empty.
#[no_mangle]
pub unsafe extern "C" fn rust_ringbuf_pop(
    buf: *mut u8,
    size: uint64_t,
    rd: *mut uint64_t,
    wr: *mut uint64_t,
    count: *mut uint64_t,
    byte: *mut u8,
) -> int32_t {
    if *count == 0 {
        return -1;
    }
    *byte = *buf.add((*rd) as usize);
    *rd = (*rd + 1) % size;
    *count -= 1;
    0
}

// ============================================================
// Panic handler (panic=abort, should never be called)
// ============================================================

#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! {
    loop {}
}

// ============================================================
// Network packet validation (Rust-safe, prevents buffer overflows)
// Replaces unsafe C parsing in net.c
// ============================================================

/// Validate IPv4 header and compute payload offset.
/// Returns payload length, or 0 if packet is malformed.
#[no_mangle]
pub unsafe extern "C" fn rust_ip_validate(
    frame: *const u8,
    frame_len: u32,
    ip_hdr_len_out: *mut u32,
    payload_out: *mut *const u8,
    payload_len_out: *mut u32,
) -> i32 {
    if frame_len < 34 { /* eth(14) + ip(20) minimum */
        return -1;
    }
    let ip = frame.add(14) as *const u32;
    let ver_ihl = *((frame.add(14)) as *const u8);
    let ihl = (ver_ihl & 0x0F) as u32;
    if ihl < 5 || ihl > 15 {
        return -1;
    }
    let ip_hdr_len = ihl * 4;
    if ip_hdr_len > frame_len - 14 {
        return -1;
    }
    let total_len = u16::from_be(*(frame.add(16) as *const u16)) as u32;
    if total_len < ip_hdr_len {
        return -1;
    }
    if 14 + total_len > frame_len {
        return -1;
    }
    let payload_len = total_len - ip_hdr_len;
    *ip_hdr_len_out = ip_hdr_len;
    *payload_out = frame.add(14 + ip_hdr_len as usize);
    *payload_len_out = payload_len;
    0
}

/// Validate TCP header data offset and compute payload.
/// Returns 0 on success, -1 if malformed.
#[no_mangle]
pub unsafe extern "C" fn rust_tcp_validate(
    tcp_data: *const u8,
    tcp_len: u32,
    data_off_out: *mut u32,
    payload_out: *mut *const u8,
    payload_len_out: *mut u32,
) -> i32 {
    if tcp_len < 20 { /* TCP header minimum */
        return -1;
    }
    let data_off = ((*tcp_data.add(12) as u32) >> 4) * 4;
    if data_off < 20 || data_off > tcp_len {
        return -1;
    }
    let payload_len = tcp_len - data_off;
    *data_off_out = data_off;
    *payload_out = tcp_data.add(data_off as usize);
    *payload_len_out = payload_len;
    0
}

/// Compute IP checksum (Rust-safe version for validation)
#[no_mangle]
pub unsafe extern "C" fn rust_ip_checksum(data: *const u8, len: u32) -> u16 {
    let mut sum: u32 = 0;
    let mut i = 0u32;
    while i + 1 < len {
        let word = u16::from_be(*(data.add(i as usize) as *const u16)) as u32;
        sum += word;
        i += 2;
    }
    if i < len {
        sum += (*data.add(i as usize) as u32) << 8;
    }
    while sum >> 16 != 0 {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    !(sum as u16)
}
