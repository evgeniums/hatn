#!/bin/bash
# package-fileencryptor.sh — build a relocatable archive with the hatn file encryptor.
#
# The archive contains everything needed on another Linux/macOS server:
#   * libhatnfileencryptor.{so,dylib}  - C API library loaded by the Go server
#                                        (evgo/pkg/crypt_bridge, via purego dlopen)
#   * hatn-file-encryptor, hatn-file-decryptor - standalone CLI tools
#   * all non-system shared libraries they depend on (hatn modules, boost, cares, ...)
#
# Archive layout (root folder hatn-fileencryptor-<version>-<os>-<arch>/):
#   native/                 library + its dependencies; RUNPATH/rpath = $ORIGIN / @loader_path
#                           Copy this folder next to <label>-app-server / <label>-console-admin:
#                           crypt_bridge looks for <exe-dir>/native/libhatnfileencryptor.<ext>
#   bin/                    hatn-file-encryptor, hatn-file-decryptor (rpath -> ../native)
#   plugins/crypt/          hatn crypt plugin(s) for the CLI tools (only if present in the build)
#   config-example.jsonc    example of the "fileencryptor" config section
#   install.sh              copies native/ (and optionally the tools) to the server folder
#   README.txt
#
# The script must run on the OS it packages for (Linux: in the same environment/mount layout
# as the build, e.g. the docker build container, so that the RUNPATH of the binaries resolves;
# macOS: on the build machine). It never modifies the build tree - everything is staged and patched
# in a temporary copy.
#
# Usage:
#   package-fileencryptor.sh --build-dir <dir> [--output <dir>] [--version <ver>] [--name <name>]
#                            [--lib-dir <dir>]... [--bundle-cxx] [--strip] [--keep-stage]
#
#   --build-dir   build output folder containing hatn-file-encryptor, hatn-file-decryptor and
#                 libhatnfileencryptor[d].{so,dylib} (e.g. <build>/tools/fileencryptor). Required.
#   --output      folder for the resulting .tar.gz (default: current directory).
#   --version     version string for the archive name (default: hatn VERSION file, else "unknown").
#   --name        archive root folder name (default: hatn-fileencryptor-<version>-<os>-<arch>).
#   --lib-dir     extra folder to search for dependencies that cannot be resolved otherwise
#                 (repeatable; macOS: used to resolve @rpath references).
#   --bundle-cxx  Linux only: also bundle libstdc++/libgcc_s (use when the target server may have
#                 an older C++ runtime than the build machine). glibc is never bundled.
#   --strip       strip symbols from the staged copies.
#   --keep-stage  keep the temporary staging folder (printed at the end).
#
# Environment:
#   PATCHELF      patchelf binary (Linux, default: patchelf from PATH; required).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

die() { echo "ERROR: $*" >&2; exit 1; }
info() { echo "[package] $*"; }

BUILD_DIR=""
OUTPUT_DIR="$PWD"
VERSION=""
NAME=""
BUNDLE_CXX=0
STRIP_BINS=0
KEEP_STAGE=0
EXTRA_LIB_DIRS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)  BUILD_DIR="${2:?--build-dir needs a value}"; shift 2 ;;
        --output)     OUTPUT_DIR="${2:?--output needs a value}"; shift 2 ;;
        --version)    VERSION="${2:?--version needs a value}"; shift 2 ;;
        --name)       NAME="${2:?--name needs a value}"; shift 2 ;;
        --lib-dir)    EXTRA_LIB_DIRS+=("${2:?--lib-dir needs a value}"); shift 2 ;;
        --bundle-cxx) BUNDLE_CXX=1; shift ;;
        --strip)      STRIP_BINS=1; shift ;;
        --keep-stage) KEEP_STAGE=1; shift ;;
        -h|--help)    sed -n '2,45p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *)            die "unknown option: $1 (see --help)" ;;
    esac
done

[[ -n "$BUILD_DIR" ]] || die "--build-dir is required (see --help)"
[[ -d "$BUILD_DIR" ]] || die "build dir not found: $BUILD_DIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"
mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR="$(cd "$OUTPUT_DIR" && pwd)"

# ---- Platform ---------------------------------------------------------------
case "$(uname -s)" in
    Linux)  OS=linux;  LIB_EXT=so ;;
    Darwin) OS=macos;  LIB_EXT=dylib ;;
    *)      die "unsupported OS: $(uname -s)" ;;
esac
ARCH="$(uname -m)"
[[ "$ARCH" == "aarch64" ]] && ARCH=arm64
[[ "$ARCH" == "amd64" ]] && ARCH=x86_64

if [[ -z "$VERSION" ]]; then
    if [[ -f "$SCRIPT_DIR/../../VERSION" ]]; then
        VERSION="$(tr -d '[:space:]' < "$SCRIPT_DIR/../../VERSION")"
    fi
    VERSION="${VERSION:-unknown}"
fi
[[ -n "$NAME" ]] || NAME="hatn-fileencryptor-${VERSION}-${OS}-${ARCH}"

# ---- Locate build artifacts -------------------------------------------------
ENC_BIN="$BUILD_DIR/hatn-file-encryptor"
DEC_BIN="$BUILD_DIR/hatn-file-decryptor"
[[ -f "$ENC_BIN" ]] || die "not found: $ENC_BIN"
[[ -f "$DEC_BIN" ]] || die "not found: $DEC_BIN"

# Debug builds add a "d" suffix to the library name; the Go bridge expects the plain name,
# so the staged copy is always named libhatnfileencryptor.<ext>.
LIB_SRC=""
for cand in "$BUILD_DIR/libhatnfileencryptor.$LIB_EXT" "$BUILD_DIR/libhatnfileencryptord.$LIB_EXT"; do
    if [[ -f "$cand" ]]; then LIB_SRC="$cand"; break; fi
done
[[ -n "$LIB_SRC" ]] || die "libhatnfileencryptor[d].$LIB_EXT not found in $BUILD_DIR"
LIB_NAME="libhatnfileencryptor.$LIB_EXT"

PLUGINS_SRC=""
if [[ -d "$BUILD_DIR/plugins/crypt" ]]; then PLUGINS_SRC="$BUILD_DIR/plugins/crypt"; fi

# ---- Staging ----------------------------------------------------------------
TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/hatn-fileencryptor-pkg.XXXXXX")"
STAGE="$TMP_ROOT/$NAME"
cleanup() { if [[ $KEEP_STAGE -eq 0 ]]; then rm -rf "$TMP_ROOT"; else info "stage kept: $STAGE"; fi; }
trap cleanup EXIT

mkdir -p "$STAGE/bin" "$STAGE/native"
cp "$ENC_BIN" "$STAGE/bin/hatn-file-encryptor"
cp "$DEC_BIN" "$STAGE/bin/hatn-file-decryptor"
cp "$LIB_SRC" "$STAGE/native/$LIB_NAME"
chmod u+w "$STAGE/bin/"* "$STAGE/native/"*

PLUGIN_FILES=()
if [[ -n "$PLUGINS_SRC" ]]; then
    mkdir -p "$STAGE/plugins/crypt"
    for f in "$PLUGINS_SRC"/*."$LIB_EXT"; do
        [[ -f "$f" ]] || continue
        cp "$f" "$STAGE/plugins/crypt/"
        chmod u+w "$STAGE/plugins/crypt/$(basename "$f")"
        PLUGIN_FILES+=("$STAGE/plugins/crypt/$(basename "$f")")
    done
fi

ROOTS=("$STAGE/bin/hatn-file-encryptor" "$STAGE/bin/hatn-file-decryptor" "$STAGE/native/$LIB_NAME")
if [[ ${#PLUGIN_FILES[@]} -gt 0 ]]; then ROOTS+=("${PLUGIN_FILES[@]}"); fi

# ============================================================================
# Linux: ldd closure + patchelf
# ============================================================================
package_linux()
{
    local patchelf="${PATCHELF:-patchelf}"
    command -v "$patchelf" >/dev/null 2>&1 || die "patchelf not found (set PATCHELF or install it)"
    command -v ldd >/dev/null 2>&1 || die "ldd not found"

    # Libraries that always come from the target OS.
    local sys_excl='^(libc|libdl|libpthread|libm|librt|libutil|libresolv|libnsl|libanl|ld-linux[^ ]*|linux-vdso)\.so'
    if [[ $BUNDLE_CXX -eq 0 ]]; then
        sys_excl='^(libc|libdl|libpthread|libm|librt|libutil|libresolv|libnsl|libanl|libgcc_s|libstdc\+\+|ld-linux[^ ]*|linux-vdso)\.so'
    fi

    # Resolve dependencies of the ORIGINAL binaries: the staged copies have the build RUNPATH
    # too, so ldd on them is equivalent, but resolving from the originals is clearer in errors.
    local root missing line name path
    for root in "${ROOTS[@]}"; do
        missing="$(ldd "$root" 2>/dev/null | awk '/not found/ { print $1 }' || true)"
        if [[ -n "$missing" ]]; then
            echo "ERROR: ldd cannot resolve for $(basename "$root"):" >&2
            echo "$missing" | sed 's/^/  /' >&2
            echo "  Run this script in the same environment/mount layout as the build" >&2
            echo "  (binary RUNPATH must resolve) or set LD_LIBRARY_PATH to the lib folders." >&2
            exit 1
        fi
    done

    info "collecting shared libraries (ldd)..."
    for root in "${ROOTS[@]}"; do
        while IFS= read -r line; do
            # "<name> => <path> (0x...)"
            name="$(echo "$line" | awk '{print $1}')"
            path="$(echo "$line" | awk '/=>/ {print $3}')"
            [[ -n "$path" && -f "$path" ]] || continue
            if echo "$name" | grep -E -q "$sys_excl"; then continue; fi
            if [[ ! -f "$STAGE/native/$name" ]]; then
                cp -L "$path" "$STAGE/native/$name"
                chmod u+w "$STAGE/native/$name"
                echo "  $name <- $path"
            fi
        done < <(ldd "$root" 2>/dev/null)
    done

    if [[ $STRIP_BINS -eq 1 ]]; then
        info "stripping..."
        local f
        for f in "$STAGE"/bin/* "$STAGE"/native/*; do strip --strip-unneeded "$f" 2>/dev/null || true; done
        if [[ ${#PLUGIN_FILES[@]} -gt 0 ]]; then
            for f in "${PLUGIN_FILES[@]}"; do strip --strip-unneeded "$f" 2>/dev/null || true; done
        fi
    fi

    info "setting RUNPATH..."
    local f
    for f in "$STAGE"/native/*; do "$patchelf" --set-rpath '$ORIGIN' "$f"; done
    for f in "$STAGE"/bin/*; do "$patchelf" --set-rpath '$ORIGIN/../native' "$f"; done
    if [[ ${#PLUGIN_FILES[@]} -gt 0 ]]; then
        for f in "${PLUGIN_FILES[@]}"; do "$patchelf" --set-rpath '$ORIGIN/../../native' "$f"; done
    fi
}

verify_linux()
{
    info "verifying staged package..."
    local f out bad
    local files=("$STAGE"/bin/* "$STAGE"/native/*)
    if [[ ${#PLUGIN_FILES[@]} -gt 0 ]]; then files+=("${PLUGIN_FILES[@]}"); fi
    for f in "${files[@]}"; do
        # Without LD_LIBRARY_PATH: only the patched RUNPATH and system paths may resolve.
        out="$(env -u LD_LIBRARY_PATH ldd "$f" 2>&1 || true)"
        if echo "$out" | grep -q "not found"; then
            echo "$out" | grep "not found" | sed "s|^|  $(basename "$f"): |" >&2
            die "unresolved dependencies in staged $(basename "$f")"
        fi
        # Anything resolved outside the package must be a system library.
        bad="$(echo "$out" | awk '/=>/ {print $3}' \
                | grep -v -E "^($STAGE/|/lib/|/lib64/|/usr/lib/|/usr/lib64/)" || true)"
        if [[ -n "$bad" ]]; then
            echo "$bad" | sed "s|^|  $(basename "$f"): |" >&2
            die "staged $(basename "$f") resolves libraries outside the package"
        fi
    done
    smoke_test
}

# ============================================================================
# macOS: otool closure + install_name_tool + ad-hoc codesign
# ============================================================================
SEARCH_DIRS=()

add_search_dir()
{
    local d="$1" e
    [[ -d "$d" ]] || return 0
    if [[ ${#SEARCH_DIRS[@]} -gt 0 ]]; then
        for e in "${SEARCH_DIRS[@]}"; do [[ "$e" == "$d" ]] && return 0; done
    fi
    SEARCH_DIRS+=("$d")
}

# Print absolute LC_RPATH entries of a Mach-O file (from the ORIGINAL file location).
original_rpaths()
{
    local f="$1" d p
    d="$(dirname "$f")"
    while IFS= read -r p; do
        case "$p" in
            @loader_path*|@executable_path*) p="$d${p#@*_path}" ;;
        esac
        echo "$p"
    done < <(otool -l "$f" | awk '/cmd LC_RPATH/ {r=1} r && $1=="path" {print $2; r=0}')
}

# Resolve a dependency reference to a real file; empty output if not found.
resolve_mac_dep()
{
    local dep="$1" base d
    base="$(basename "$dep")"
    case "$dep" in
        /*) [[ -e "$dep" ]] && { echo "$dep"; return 0; } ;;
    esac
    if [[ ${#SEARCH_DIRS[@]} -gt 0 ]]; then
        for d in "${SEARCH_DIRS[@]}"; do
            if [[ -e "$d/$base" ]]; then echo "$d/$base"; return 0; fi
        done
    fi
    return 0
}

is_system_ref()
{
    case "$1" in
        /usr/lib/*|/System/*) return 0 ;;
    esac
    return 1
}

package_macos()
{
    command -v otool >/dev/null 2>&1 || die "otool not found (install Xcode command line tools)"
    command -v install_name_tool >/dev/null 2>&1 || die "install_name_tool not found"
    command -v codesign >/dev/null 2>&1 || die "codesign not found"

    # Search folders: rpaths of the original binaries, the build folder, user-provided folders.
    local f p
    for f in "$ENC_BIN" "$DEC_BIN" "$LIB_SRC"; do
        while IFS= read -r p; do add_search_dir "$p"; done < <(original_rpaths "$f")
    done
    add_search_dir "$BUILD_DIR"
    # The library links the openssl plugin directly (@rpath), it lives in the plugins folder.
    if [[ -n "$PLUGINS_SRC" ]]; then add_search_dir "$PLUGINS_SRC"; fi
    if [[ -n "$PLUGINS_SRC" ]]; then
        for f in "$PLUGINS_SRC"/*.dylib; do
            [[ -f "$f" ]] || continue
            while IFS= read -r p; do add_search_dir "$p"; done < <(original_rpaths "$f")
        done
    fi
    if [[ ${#EXTRA_LIB_DIRS[@]} -gt 0 ]]; then
        for p in "${EXTRA_LIB_DIRS[@]}"; do add_search_dir "$p"; done
    fi

    info "collecting shared libraries (otool)..."
    local todo=("${ROOTS[@]}") i=0 file base new_rpath dep dbase src
    while [[ $i -lt ${#todo[@]} ]]; do
        file="${todo[$i]}"
        i=$((i + 1))
        base="$(basename "$file")"

        if [[ $STRIP_BINS -eq 1 ]]; then strip -x "$file" 2>/dev/null || true; fi

        case "$file" in
            "$STAGE"/bin/*)            new_rpath='@loader_path/../native' ;;
            "$STAGE"/plugins/crypt/*)  new_rpath='@loader_path/../../native' ;;
            *)                         new_rpath='@loader_path' ;;
        esac

        # Own install name (dylibs only): @rpath/<staged name>
        case "$file" in
            *.dylib) install_name_tool -id "@rpath/$base" "$file" ;;
        esac

        # Replace all rpaths with the single relative one.
        while IFS= read -r p; do
            [[ -n "$p" ]] && install_name_tool -delete_rpath "$p" "$file"
        done < <(otool -l "$file" | awk '/cmd LC_RPATH/ {r=1} r && $1=="path" {print $2; r=0}')
        install_name_tool -add_rpath "$new_rpath" "$file"

        while IFS= read -r dep; do
            [[ -n "$dep" ]] || continue
            dbase="$(basename "$dep")"
            [[ "$dbase" == "$base" ]] && continue          # own id
            is_system_ref "$dep" && continue

            if [[ ! -f "$STAGE/native/$dbase" ]]; then
                src="$(resolve_mac_dep "$dep")"
                [[ -n "$src" ]] || die "cannot resolve '$dep' (needed by $base); use --lib-dir"
                cp -L "$src" "$STAGE/native/$dbase"
                chmod u+w "$STAGE/native/$dbase"
                echo "  $dbase <- $src"
                # The dependency's own rpaths may point to more search folders.
                while IFS= read -r p; do add_search_dir "$p"; done < <(original_rpaths "$src")
                todo+=("$STAGE/native/$dbase")
            fi
            if [[ "$dep" != "@rpath/$dbase" ]]; then
                install_name_tool -change "$dep" "@rpath/$dbase" "$file"
            fi
        done < <(otool -L "$file" | awk 'NR>1 {print $1}')
    done

    # install_name_tool invalidates signatures; arm64 refuses to load unsigned code.
    info "ad-hoc signing..."
    local all=("$STAGE"/bin/* "$STAGE"/native/*)
    if [[ ${#PLUGIN_FILES[@]} -gt 0 ]]; then all+=("${PLUGIN_FILES[@]}"); fi
    for f in "${all[@]}"; do codesign --force --sign - "$f" >/dev/null 2>&1 || die "codesign failed for $f"; done
}

verify_macos()
{
    info "verifying staged package..."
    local f dep
    local files=("$STAGE"/bin/* "$STAGE"/native/*)
    if [[ ${#PLUGIN_FILES[@]} -gt 0 ]]; then files+=("${PLUGIN_FILES[@]}"); fi
    for f in "${files[@]}"; do
        while IFS= read -r dep; do
            [[ -n "$dep" ]] || continue
            is_system_ref "$dep" && continue
            case "$dep" in
                @rpath/*)
                    [[ -f "$STAGE/native/${dep#@rpath/}" ]] \
                        || die "$(basename "$f") references missing $dep" ;;
                *)
                    die "$(basename "$f") has non-relocatable reference: $dep" ;;
            esac
        done < <(otool -L "$f" | awk 'NR>1 {print $1}')
    done
    smoke_test
}

# Run each CLI tool; only a loader failure is fatal (the tools may exit non-zero without args).
smoke_test()
{
    local t out
    for t in hatn-file-encryptor hatn-file-decryptor; do
        out="$(env -u LD_LIBRARY_PATH -u DYLD_LIBRARY_PATH "$STAGE/bin/$t" --help 2>&1 || true)"
        if echo "$out" | grep -q -i -E "Library not loaded|cannot open shared object|image not found|symbol not found"; then
            echo "$out" | head -5 >&2
            die "$t does not start from the staged package"
        fi
    done
    info "CLI tools start from the staged package"
}

# ---- Run --------------------------------------------------------------------
info "packaging for $OS/$ARCH, version $VERSION"
info "build dir: $BUILD_DIR"
if [[ "$OS" == "linux" ]]; then package_linux; else package_macos; fi

# ---- Extras -----------------------------------------------------------------
if [[ -f "$SCRIPT_DIR/config-example.jsonc" ]]; then
    cp "$SCRIPT_DIR/config-example.jsonc" "$STAGE/config-example.jsonc"
fi

cat > "$STAGE/install.sh" <<'EOF'
#!/bin/bash
# Install the hatn file encryptor into a Go server folder.
#   ./install.sh <server-bin-dir> [<tools-dir>]
# Copies native/ to <server-bin-dir>/native (where evgo crypt_bridge looks for
# libhatnfileencryptor). If <tools-dir> is given, the CLI tools are also installed there as
# <tools-dir>/bin, with their own <tools-dir>/native (and <tools-dir>/plugins).
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEST="${1:?usage: install.sh <server-bin-dir> [<tools-dir>]}"
mkdir -p "$DEST"
rm -rf "$DEST/native"
cp -R "$HERE/native" "$DEST/native"
echo "Installed library: $DEST/native/$(ls "$HERE/native" | grep '^libhatnfileencryptor\.' | head -1)"
if [[ $# -ge 2 ]]; then
    TOOLS="$2"
    mkdir -p "$TOOLS"
    rm -rf "$TOOLS/bin" "$TOOLS/native" "$TOOLS/plugins"
    cp -R "$HERE/bin" "$TOOLS/bin"
    cp -R "$HERE/native" "$TOOLS/native"
    if [[ -d "$HERE/plugins" ]]; then cp -R "$HERE/plugins" "$TOOLS/plugins"; fi
    echo "Installed tools:   $TOOLS/bin/hatn-file-encryptor, $TOOLS/bin/hatn-file-decryptor"
fi
EOF
chmod +x "$STAGE/install.sh"

cat > "$STAGE/README.txt" <<EOF
hatn file encryptor ${VERSION} (${OS}/${ARCH})

native/    libhatnfileencryptor.${LIB_EXT} and all libraries it needs (self-contained, relocatable).
bin/       hatn-file-encryptor, hatn-file-decryptor command line tools.
plugins/   hatn crypt plugin(s) for the CLI tools (if present).

Go server (evgo crypt_bridge)
  The server loads <exe-dir>/native/libhatnfileencryptor.${LIB_EXT}. Either run
      ./install.sh /path/to/server/bin
  or set WHITEM_FILE_ENCRYPTOR_LIB=/abs/path/to/native/libhatnfileencryptor.${LIB_EXT}.
  The server config needs a top-level "fileencryptor" section, see config-example.jsonc.

CLI tools
  bin/hatn-file-encryptor --config <server.jsonc> --plain <in> --target <out>
  bin/hatn-file-decryptor --config <server.jsonc> --encrypted <in> --target <out>
  (run with --help for all options). The tools find the crypt plugin through
  app.plugin_folders of the config or ./plugins/crypt relative to the current folder;
  when installed by this archive, use the absolute path of the plugins/ folder
  in "app.plugin_folders" if the tools are started from elsewhere.

Requirements on the target
  ${OS}/${ARCH}$( [[ "$OS" == "linux" ]] && echo ", glibc not older than on the build machine" || echo ", macOS not older than the build deployment target" ).
EOF

# ---- Verify and pack --------------------------------------------------------
if [[ "$OS" == "linux" ]]; then verify_linux; else verify_macos; fi

ARCHIVE="$OUTPUT_DIR/$NAME.tar.gz"
info "creating $ARCHIVE"
tar -C "$TMP_ROOT" -czf "$ARCHIVE" "$NAME"

info "done: $ARCHIVE ($(du -h "$ARCHIVE" | awk '{print $1}'))"
