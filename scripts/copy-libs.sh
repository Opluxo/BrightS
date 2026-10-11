#!/bin/bash
# 递归拷贝动态库及其依赖到目标目录
# 用法: copy-libs.sh <dest_dir> <lib1> <lib2> ...

SRC=/usr/lib/aarch64-linux-gnu
DEST="$1"
shift
LIBS="$@"

mkdir -p "$DEST"

# 不拷 glibc 家族的库（我们自己维护 /System/lib）
SKIP="^libc\.so|^ld-linux|^libpthread|^libdl\.so|^librt\.so|^libm\.so|^libgcc_s"

declare -A copied

copy_one() {
    local name="$1"
    echo "$name" | grep -qE "$SKIP" && return
    [ -n "${copied[$name]}" ] && return

    local path="$SRC/$name"
    [ ! -f "$path" ] && path=$(find /usr/lib /lib -name "$name" 2>/dev/null | head -1)
    if [ ! -f "$path" ]; then
        echo "  WARN: not found: $name" >&2
        return
    fi

    cp -L "$path" "$DEST/$name"
    copied[$name]=1
    echo "  copied: $name"

    for dep in $(ldd "$path" 2>/dev/null | grep '=>' | awk '{print $1}'); do
        copy_one "$dep"
    done
}

for lib in $LIBS; do
    copy_one "$lib"
done

echo ""
echo "Total: $(ls -1 "$DEST" | wc -l) files"
