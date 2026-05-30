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
