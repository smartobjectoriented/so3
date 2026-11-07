# Copyright (C) 2006  OpenedHand LTD
# Copyright (C) 2023-2026 EDGEMTech Ltd
#
# SPDX-License-Identifier: MIT

# Point to an empty file so any user's custom settings don't break things
QUILTRCFILE ?= "${STAGING_ETCDIR_NATIVE}/quiltrc"

PATCH_GIT_USER_NAME ?= "OpenEmbedded"
PATCH_GIT_USER_EMAIL ?= "oe.patch@oe"

inherit terminal

python () {
    if d.getVar('PATCHTOOL') == 'git' and d.getVar('PATCH_COMMIT_FUNCTIONS') == '1':
        extratasks = bb.build.tasksbetween('do_unpack', 'do_patch', d)
        try:
            extratasks.remove('do_unpack')
        except ValueError:
            # For some recipes do_unpack doesn't exist, ignore it
            pass

        d.appendVarFlag('do_patch', 'prefuncs', ' patch_task_patch_prefunc')
        for task in extratasks:
            d.appendVarFlag(task, 'postfuncs', ' patch_task_postfunc')

    # Append patch_save_pristine after every other do_unpack postfunc — in
    # particular do_handle_fetch_git from base.bbclass, which is what
    # actually populates ${S} on git-fetched recipes. Done here (parse-time
    # anonymous python) so we run AFTER base.bbclass's `do_unpack[postfuncs]
    # = ...` overwrite, preserving the run order
    # handle_fetch_git → patch_save_pristine.
    d.appendVarFlag('do_unpack', 'postfuncs', ' patch_save_pristine')
}

python patch_task_patch_prefunc() {
    # Prefunc for do_patch
    srcsubdir = d.getVar('S')

    workdir = os.path.abspath(d.getVar('WORKDIR'))
    testsrcdir = os.path.abspath(srcsubdir)
    if (testsrcdir + os.sep).startswith(workdir + os.sep):
        # Double-check that either workdir or S or some directory in-between is a git repository
        found = False
        while testsrcdir != workdir:
            if os.path.exists(os.path.join(testsrcdir, '.git')):
                found = True
                break
            if testsrcdir == workdir:
                break
            testsrcdir = os.path.dirname(testsrcdir)
        if not found:
            bb.fatal('PATCHTOOL = "git" set for source tree that is not a git repository. Refusing to continue as that may result in commits being made in your metadata repository.')

    patchdir = os.path.join(srcsubdir, 'patches')
    if os.path.exists(patchdir):
        if os.listdir(patchdir):
            d.setVar('PATCH_HAS_PATCHES_DIR', '1')
        else:
            os.rmdir(patchdir)
}

python patch_task_postfunc() {
    # Prefunc for task functions between do_unpack and do_patch
    import oe.patch
    import shutil
    func = d.getVar('BB_RUNTASK')
    srcsubdir = d.getVar('S')

    if os.path.exists(srcsubdir):
        if func == 'do_patch':
            haspatches = (d.getVar('PATCH_HAS_PATCHES_DIR') == '1')
            patchdir = os.path.join(srcsubdir, 'patches')
            if os.path.exists(patchdir):
                shutil.rmtree(patchdir)
                if haspatches:
                    stdout, _ = bb.process.run('git status --porcelain patches', cwd=srcsubdir)
                    if stdout:
                        bb.process.run('git checkout patches', cwd=srcsubdir)
        stdout, _ = bb.process.run('git status --porcelain .', cwd=srcsubdir)
        if stdout:
            useroptions = []
            oe.patch.GitApplyTree.gitCommandUserOptions(useroptions, d=d)
            bb.process.run('git add .; git %s commit -a -m "Committing changes from %s\n\n%s"' % (' '.join(useroptions), func, oe.patch.GitApplyTree.ignore_commit_prefix + ' - from %s' % func), cwd=srcsubdir)
}

def src_patches(d, all=False, expand=True):
    import oe.patch
    return oe.patch.src_patches(d, all, expand)

def should_apply(parm, d):
    """Determine if we should apply the given patch"""
    import oe.patch
    return oe.patch.should_apply(parm, d)

should_apply[vardepsexclude] = "DATE SRCDATE"

python patch_do_patch() {
    import oe.patch

    patchsetmap = {
        "patch": oe.patch.PatchTree,
        "quilt": oe.patch.QuiltTree,
        "git": oe.patch.GitApplyTree,
    }

    cls = patchsetmap[d.getVar('PATCHTOOL') or 'quilt']

    resolvermap = {
        "noop": oe.patch.NOOPResolver,
        "user": oe.patch.UserResolver,
    }

    rcls = resolvermap[d.getVar('PATCHRESOLVE') or 'user']

    classes = {}

    s = d.getVar('S')

    os.putenv('PATH', d.getVar('PATH'))

    # We must use one TMPDIR per process so that the "patch" processes
    # don't generate the same temp file name.
 
    import tempfile
    process_tmpdir = tempfile.mkdtemp()
    os.environ['TMPDIR'] = process_tmpdir

    for patch in src_patches(d):
        _, _, local, _, _, parm = bb.fetch.decodeurl(patch)

        if "patchdir" in parm:
            patchdir = parm["patchdir"]
            if not os.path.isabs(patchdir):
                patchdir = os.path.join(s, patchdir)
            if not os.path.isdir(patchdir):
                bb.fatal("Target directory '%s' not found, patchdir '%s' is incorrect in patch file '%s'" %
                    (patchdir, parm["patchdir"], parm['patchname']))
        else:
            patchdir = s

        if not patchdir in classes:
            patchset = cls(patchdir, d)
            resolver = rcls(patchset, oe_terminal)
            classes[patchdir] = (patchset, resolver)
            patchset.Clean()
        else:
            patchset, resolver = classes[patchdir]

        bb.note("Applying patch '%s' (%s)" % (parm['patchname'], oe.path.format_display(local, d)))
        try:
            patchset.Import({"file":local, "strippath": parm['striplevel']}, True)
        except Exception as exc:
            bb.utils.remove(process_tmpdir, True)
            bb.fatal("Importing patch '%s' with striplevel '%s'\n%s" % (parm['patchname'], parm['striplevel'], repr(exc).replace("\\n", "\n")))
        try:
            resolver.Resolve()
        except bb.BBHandledException as e:
            bb.utils.remove(process_tmpdir, True)
            bb.fatal("Applying patch '%s' on target directory '%s'\n%s" % (parm['patchname'], patchdir, repr(e).replace("\\n", "\n")))

    bb.utils.remove(process_tmpdir, True)
    del os.environ['TMPDIR']
}
patch_do_patch[vardepsexclude] = "PATCHRESOLVE"

addtask patch after do_unpack
do_patch[dirs] = "${WORKDIR}"

def get_next_number(patchdir):
    highest_number = 0
	
    for file in os.listdir(patchdir):
        if file[:4].isdigit():
            number = int(file[:4])
            if number > highest_number:
                highest_number = number

    # Increment the highest number by one
    new_number = highest_number + 1
    
    return new_number
   
do_diffcompose[nostamp] = "1"

# Save an untouched copy of ${S} right after do_unpack (+ its
# do_handle_fetch_git postfunc) populates it, BEFORE do_patch mutates the
# tree. do_diffcompose later compares this pristine snapshot against
# IB_TARGET so that the staged patches reflect the FULL divergence of
# IB_TARGET from upstream — enabling do_updiff to consolidate per-file
# patches instead of appending an ever-growing chain.

python patch_save_pristine() {
    import os
    import shutil
    s = d.getVar('S')
    if not s or not os.path.isdir(s):
        return
    pristine = s + '.pristine'
    if os.path.isdir(pristine):
        shutil.rmtree(pristine)
    shutil.copytree(s, pristine, symlinks=True)
}

# Directory holding the recipe's numbered patchset (NNNN-${PF}/ and its
# NNNN-${PF}-patches.inc), where do_diffcompose stages and do_updiff merges.
# Defaults to the recipe's own files/. A bbappend that carries the patchset
# in its own layer directory must point it there (with :=, so THISDIR is the
# bbappend's): otherwise updiff never sees the existing patches and builds a
# second, unused patchset next to the base recipe.
IB_PATCHSET_DIR ??= "${FILE_DIRNAME}/files"

# Stage what was edited in IB_TARGET since do_attach_infrabase, the baseline
# being the attach manifest: one "pristine -> IB_TARGET" diff per file in
# ${IB_PATCHSET_DIR}/${PF}/, listed in .edited for do_updiff. A file the user did
# not touch is not looked at, so a patch set updated behind the tree stays intact.
#
# Without a manifest, every file differing from pristine counts as edited.

do_diffcompose[prefuncs] += "do_check_attach"

python patch_do_diffcompose() {
    import fnmatch
    import os
    import re
    import shutil
    import subprocess

    source_dir = d.getVar('S') + '.pristine'
    if not os.path.isdir(source_dir):
        bb.fatal(
            f"do_diffcompose: pristine source {source_dir} missing. "
            f"Run './scripts/build.sh -xc {d.getVar('PN')}' first so that "
            f"do_unpack regenerates the pristine snapshot, then re-run updiff."
        )
    target_dir = d.getVar('IB_TARGET')
    output_dir = os.path.join(d.getVar('IB_PATCHSET_DIR'), d.getVar('PF'))

    if os.path.isdir(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)

    # Build artefacts that leak into IB_TARGET when the user runs `make`
    # before `updiff`, matched against every file and directory name.

    exclude_patterns = [
        # compiler / linker object artefacts. NOTE: *.lds is NOT excluded —
        # SO3 ships hand-written linker scripts (e.g. arch/*/so3.lds) and
        # filtering them silently kills the corresponding source patch.
        '*.o', '*.a', '*.ko', '*.so', '*.cmd', '*.su', '*.o.d', '*.d',
        '*.mod', '*.mod.*',
        # kbuild auto-generated headers and asm. NOTE: *.lds is generated
        # only for some files (e.g. arch/arm64/kernel/module.lds.S → module.lds);
        # we cannot blacklist `*.lds` globally so individual names go below.
        'autoconf.h', 'asm-offsets.h', 'asm-offsets.s',
        'devicetable-offsets.h', 'devicetable-offsets.s',
        'elfconfig.h', 'module.lds',
        # lex / yacc / bison generated parsers — kconfig, dtc, and any other
        # consumer of flex/bison. Sources are *.l / *.y (kept); outputs are
        # *.lex.c, *.tab.c, *.tab.h (excluded).
        'lex.zconf.c', 'zconf.tab.c', 'zconf.hash.c',
        '*lex.lex.c', '*.lex.c', '*.tab.c', '*.tab.h', '*lexer.l.c',
        # kconfig output / state files
        '.config', '.config.old', 'auto.conf', 'auto.conf.cmd',
        'tristate.conf', 'config_data', 'config_data.gz', 'config_data.h',
        # generated SO3 syscall tables
        'syscall_number.h', 'syscall_table.h', 'syscall_table.h.in',
        # device-tree build outputs and intermediates (the *.tmp pattern
        # also catches the leading-dot dtc temp files such as
        # ".virt64.dtb.dts.tmp")
        '*.dtb', '*.dtbo', '*.tmp',
        # vmlinux + kallsyms-pass artefacts (`.tmp_vmlinux*.{syms,kallsyms.S}`,
        # `.vmlinux.export.c`, `.version` — the build counter file at root).
        'vmlinux*', '.vmlinux.*', '.version', '.tmp_*', '.missing-*',
        'System.map', 'Module.symvers',
        'modules.builtin', 'modules.builtin.modinfo', 'modules.order',
        # u-boot build outputs (final binaries, maps, configs)
        'u-boot', 'u-boot.cfg*', 'u-boot.sym', 'u-boot.srec',
        'u-boot.map', 'u-boot.lds', 'u-boot.bin', 'u-boot.bin.*',
        'u-boot-dtb*', 'u-boot-nodtb*',
        # u-boot kbuild-generated config + example/efi build outputs that
        # leak into IB_TARGET after a `make` (include/generated and
        # include/config are dir-excluded below; these live elsewhere).
        'autoconf.mk', 'autoconf.mk.dep', '*.srec',
        'efi_miniapp_*.h',
        # initramfs build deps
        '.initramfs_*',
        # what do_attach_infrabase leaves next to a tree nested in this one
        # (so3/usr in so3)
        '*.attach.sha256', '*.attach.pf', '*.attach.series', '*.back',
    ]
    # Exclude the quilt working directory + kbuild-generated trees:
    #   patches/      — quilt staging from do_attach_infrabase
    #   include/generated/ — kbuild's auto-generated arch headers (cfi.h,
    #                       delay.h, etc. from arch/*/include/asm/Kbuild)
    #   include/config/    — kbuild's symbol-tracking files
    # patches/ is the quilt staging of do_attach_infrabase, generated/ and
    # config/ the kbuild-generated trees. A recipe adds its own through
    # IB_UPDIFF_EXCLUDE: an entry with a '/' is a path from the tree root (a
    # leading '/' names a file at the root), any other a name at any depth.

    names = exclude_patterns + ['.git', '.pc', 'patches', 'generated', 'config']
    path_excludes = []
    for x in (d.getVar('IB_UPDIFF_EXCLUDE') or '').split():
        if '/' in x:
            path_excludes.append(x.lstrip('/'))
        else:
            names.append(x)
    hidden = re.compile('|'.join(fnmatch.translate(p) for p in names))

    hashes = ib_attach_hashes(target_dir)
    if hashes is None:
        bb.warn(f"do_diffcompose: no attach manifest for {target_dir}, "
                "comparing the whole tree to pristine")

    edited = []
    for rel in ib_edited_files(target_dir, source_dir, hashes, hidden):
        if any(fnmatch.fnmatch(rel, p) for p in path_excludes):
            continue

        output_patch = os.path.join(output_dir, f"{len(edited) + 1:04d}-"
                                                f"{os.path.basename(rel)}.patch")
        cmd = ['diff', '--no-dereference', '-uN',
               '--label', f'a/{rel}', '--label', f'b/{rel}',
               os.path.join(source_dir, rel), os.path.join('.', rel)]
        with open(output_patch, 'wb') as out:
            subprocess.run(cmd, stdout=out, cwd=target_dir, check=False)

        # A binary file would make do_patch fail with "Only garbage was found
        # in the patch input".

        with open(output_patch, 'rb') as f:
            binary = f.readline().startswith(b'Binary files ')
        if binary:
            bb.note(f"do_diffcompose: skipping binary file: {rel}")
            os.remove(output_patch)
            continue

        edited.append(rel)

    with open(os.path.join(output_dir, '.edited'), 'w',
              errors='surrogateescape') as f:
        f.writelines(rel + '\n' for rel in edited)
}

def generate_src_uri(patch_directory, header=None):
    patches = sorted(p for p in os.listdir(patch_directory)
                     if p.endswith(".patch"))
    # Keep the existing .inc's leading comment block (its copyright and any
    # notes), so repeated updiff runs don't churn it; standard one otherwise.
    src_uri = header or "# Copyright (c) 2025-2026 EDGEMTech SA\n\n"
    src_uri += "SRC_URI += \"\\ \n"
    for patch in patches:
        src_uri += "    file://" + patch + " \\ \n"
    src_uri += "\"\n"
    return src_uri


def patch_identifier(path):
    """Return the relative path encoded in a patch's "--- a/<path>" or
    "+++ b/<path>" header line. Patches produced by do_diffcompose always
    carry git-style a/<rel> b/<rel> labels, so this identifier uniquely
    designates the source file the patch targets, independent of the
    patch's own filename or sequence number."""
    src = None
    dst = None
    try:
        with open(path, 'r', errors='replace') as f:
            for i, line in enumerate(f):
                if i > 8:
                    break
                if line.startswith('--- '):
                    src = line[4:].rstrip('\r\n').split('\t', 1)[0]
                elif line.startswith('+++ '):
                    dst = line[4:].rstrip('\r\n').split('\t', 1)[0]
                    break
    except OSError:
        return None
    for s in (dst, src):
        if not s or s == '/dev/null':
            continue
        if s.startswith('a/') or s.startswith('b/'):
            return s[2:]
        return s
    return None


def index_patchset(directory):
    """Build {relative_path: [patch_filename, ...]} for a patchset
    directory. The list lets callers detect when several patches in the
    series modify the same source file (a chain) and treat that case
    specially — consolidation only works when exactly one patch targets a
    given file."""
    idx = {}
    if not os.path.isdir(directory):
        return idx
    for fname in sorted(os.listdir(directory)):
        if not fname.endswith('.patch'):
            continue
        pid = patch_identifier(os.path.join(directory, fname))
        if pid is not None:
            idx.setdefault(pid, []).append(fname)
    return idx

def ib_sha256(path):
    """Hex SHA-256 of the file at `path`."""
    import hashlib

    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)

    return h.hexdigest()

def ib_attach_hashes(target):
    """{rel: sha256} recorded by the last attach of `target`, None without a
    manifest. Lines escaped by sha256sum (a name with a newline or a backslash)
    are left out."""
    manifest = target + '.attach.sha256'
    if not os.path.isfile(manifest):
        return None

    hashes = {}
    with open(manifest, 'r', errors='surrogateescape') as f:
        for line in f:
            if line[:1] != '\\' and line[64:68] == '  ./':
                hashes[line[68:].rstrip('\n')] = line[:64]

    return hashes

def ib_neighbour_hashes(target):
    """{rel: sha256}, relative to `target`, of what the recipes sharing it
    attached: so3 and usr-so3 both write so3/usr, from either side."""
    import glob

    suffix = '.attach.sha256'
    hashes = {}
    for manifest in glob.glob(glob.escape(target) + '/*' + suffix):
        name = os.path.basename(manifest)[:-len(suffix)]
        nested = ib_attach_hashes(os.path.join(target, name)) or {}
        hashes.update({os.path.join(name, rel): h for rel, h in nested.items()})

    parent, name = os.path.split(os.path.normpath(target))
    prefix = name + '/'
    for rel, h in (ib_attach_hashes(parent) or {}).items():
        if rel.startswith(prefix):
            hashes[rel[len(prefix):]] = h

    return hashes

def ib_edited_files(target, pristine, hashes, hidden):
    """Paths edited in `target`, created and deleted ones included, compared
    to the `hashes` of the attach, or to `pristine` when `hashes` is None. A
    file or directory whose name matches the regex `hidden` is skipped, and so
    are symlinks, which a patch cannot carry. A file as a neighbouring recipe
    attached it is that recipe's, not an edit."""
    import filecmp

    neighbours = ib_neighbour_hashes(target)
    edited = []
    for root, dirs, files in os.walk(target):
        dirs[:] = [x for x in dirs if not hidden.match(x)]

        for name in files:
            path = os.path.join(root, name)
            if hidden.match(name) or os.path.islink(path):
                continue

            rel = os.path.relpath(path, target)
            if hashes is not None:
                changed = rel not in hashes or ib_sha256(path) != hashes[rel]
            else:
                src = os.path.join(pristine, rel)
                changed = not os.path.isfile(src) \
                    or not filecmp.cmp(src, path, shallow=False)
            if changed and neighbours.get(rel) != ib_sha256(path):
                edited.append(rel)

    known = hashes
    if known is None:
        known = [os.path.relpath(os.path.join(root, name), pristine)
                 for root, _, files in os.walk(pristine) for name in files]
    edited += [rel for rel in known
               if not os.path.lexists(os.path.join(target, rel))
               and not any(hidden.match(p) for p in rel.split('/'))]

    return edited

def ib_refresh_attach_manifest(d, rels):
    """Rewrite the ${IB_TARGET}.attach.sha256 entries for the given
    relative paths with the files' current hashes: added when the file is
    new, dropped when it is gone. Other entries are kept as they are."""
    target = d.getVar('IB_TARGET')
    manifest = target + '.attach.sha256'
    if not os.path.isfile(manifest):
        return

    # sha256sum lines are "<hash>  ./<rel>". A line it escaped (leading
    # backslash, for a name with a newline or backslash) is kept verbatim.
    entries = {}
    with open(manifest, 'r', errors='surrogateescape') as f:
        for line in f:
            line = line.rstrip('\n')
            if not line.startswith('\\') and len(line) > 66 and line[64:66] == '  ':
                entries[line[66:]] = line[:64]
            else:
                entries[line] = None

    for rel in rels:
        key = './' + rel
        path = os.path.join(target, rel)
        if os.path.isfile(path) and not os.path.islink(path):
            entries[key] = ib_sha256(path)
        else:
            entries.pop(key, None)

    with open(manifest, 'w', errors='surrogateescape') as f:
        for key in sorted(entries):
            f.write(key + '\n' if entries[key] is None
                    else f"{entries[key]}  {key}\n")

addtask do_updiff
do_updiff[nostamp] = "1"

# Fold the files staged by do_diffcompose into the patch series do_patch applies
# (src_patches(), so the bbappend patchsets are seen too). Given the patches of
# the series touching an edited file, its chain:
#
#   * KEEP: the chain already yields the file, nothing to write.
#   * REFUSE: the chain no longer yields what was attached, so the series moved
#     behind the tree and rewriting it would revert that change.
#   * FOLD: the tail of the chain lying in one patchset (see ib_fold_run) becomes
#     one fresh section on top of what the earlier patches leave. A patch losing
#     its last section is deleted, and the file reverting to that state only
#     removes the tail.
#   * APPEND: no patch touches the file: a new patch goes to the last patchset of
#     the series with a plain .inc, or to ${IB_PATCHSET_DIR}/0001-${PF}/.
#
# Nothing is written unless every file was resolved.

def ib_patch_split(path):
    """Split a patch file into (preamble, [(src, dst, text), ...]), one entry
    per file section ("--- " line directly followed by "+++ ")."""
    with open(path, 'r', errors='surrogateescape') as f:
        lines = f.readlines()
    starts = [i for i in range(len(lines) - 1)
              if lines[i].startswith('--- ') and lines[i + 1].startswith('+++ ')]
    if not starts:
        return ''.join(lines), []
    sections = []
    for n, i in enumerate(starts):
        end = starts[n + 1] if n + 1 < len(starts) else len(lines)
        src = lines[i][4:].rstrip('\r\n').split('\t', 1)[0]
        dst = lines[i + 1][4:].rstrip('\r\n').split('\t', 1)[0]
        sections.append((src, dst, ''.join(lines[i:end])))
    return ''.join(lines[:starts[0]]), sections

def ib_patch_target(src, dst, strip):
    """Path a section targets once `patch -p<strip>` has stripped it."""
    for p in (dst, src):
        if not p or p == '/dev/null':
            continue
        parts = p.split('/')
        if len(parts) <= strip:
            continue
        return os.path.normpath('/'.join(parts[strip:]))
    return None

def ib_patch_series(d):
    """The patches do_patch applies, in order, split and indexed:
    (series, index) with series = [{path, strip, preamble, sections}] and
    index = {rel: [(entry, section_number), ...]} in application order."""
    series = []
    index = {}
    for url in src_patches(d):
        _, _, local, _, _, parm = bb.fetch.decodeurl(url)
        if 'patchdir' in parm:
            bb.warn(f"do_updiff: {os.path.basename(local)} uses patchdir=, "
                    "not handled; its files are ignored")
            continue
        strip = int(parm.get('striplevel', '1'))
        preamble, sections = ib_patch_split(local)
        entry = {'path': local, 'strip': strip,
                 'preamble': preamble, 'sections': sections}
        series.append(entry)
        for n, (src, dst, _) in enumerate(sections):
            rel = ib_patch_target(src, dst, strip)
            if rel:
                index.setdefault(rel, []).append((entry, n))
    return series, index

def ib_apply_chain(rel, chain, pristine, tmpdir):
    """Content of `rel` after applying `chain` (its sections for rel only)
    on top of the pristine file; None if the file does not exist then.
    Raises RuntimeError with patch's output when a section does not apply."""
    import shutil
    import subprocess
    import tempfile

    work = tempfile.mkdtemp(dir=tmpdir)
    try:
        dest = os.path.join(work, rel)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        src = os.path.join(pristine, rel)
        if os.path.isfile(src):
            shutil.copyfile(src, dest)
        for entry, n in chain:
            sec = os.path.join(work, '.section.patch')
            with open(sec, 'w', errors='surrogateescape') as f:
                f.write(entry['sections'][n][2])
            r = subprocess.run(['patch', f"-p{entry['strip']}", '-d', work,
                                '-s', '-f', '--no-backup-if-mismatch',
                                '-r', '-', '-i', sec],
                               capture_output=True, text=True)
            os.remove(sec)
            if r.returncode != 0:
                raise RuntimeError(f"{os.path.basename(entry['path'])}: "
                                   f"{(r.stdout + r.stderr).strip()}")
        if not os.path.isfile(dest):
            return None
        with open(dest, 'rb') as f:
            return f.read()
    finally:
        shutil.rmtree(work, ignore_errors=True)

def ib_inc_is_plain(inc_path):
    """True when an .inc only holds what generate_src_uri() writes (comments,
    SRC_URI += "\\, file:// lines, closing quote), so it can be regenerated
    without losing anything."""
    import re

    if not os.path.isfile(inc_path):
        return True
    with open(inc_path) as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith('#'):
                continue
            if re.match(r'^SRC_URI \+= "\\$', s) or s == '"' \
                    or re.match(r'^file://\S+( \\)?$', s):
                continue
            return False
    return True

def ib_inc_for(setdir):
    return setdir.rstrip('/') + '-patches.inc'

def ib_set_is_plain(setdir, pf):
    """True for a numbered patchset of `pf` whose .inc is a generated one."""
    import re

    return bool(re.match(r'^\d{4}-' + re.escape(pf) + r'$',
                         os.path.basename(setdir))) \
        and ib_inc_is_plain(ib_inc_for(setdir))

def ib_patch_has_text(preamble):
    """True when a patch's preamble holds a description, not just the headers
    `diff` or git print."""
    import re

    return any(not re.match(r'^(diff |index |new file|deleted file|Index: |=+$|$)',
                            line)
               for line in preamble.splitlines())

def ib_foldable(entry, pf):
    """True when updiff may rewrite or delete this patch. Patchsets stay apart
    through their .inc, and a description is someone's work to keep."""
    return entry['strip'] == 1 and not ib_patch_has_text(entry['preamble']) \
        and ib_set_is_plain(os.path.dirname(entry['path']), pf)

def ib_fold_run(chain, pf):
    """The tail of `chain` to merge: its last link, plus the links before it
    in the same patchset. A base recipe and a bbappend are never merged."""
    last = chain[-1]
    if not ib_foldable(last[0], pf):
        return [last]

    setdir = os.path.dirname(last[0]['path'])
    run = [last]
    for link in reversed(chain[:-1]):
        if os.path.dirname(link[0]['path']) != setdir \
                or not ib_foldable(link[0], pf):
            break
        run.insert(0, link)

    return run

def ib_inc_drop(path):
    """Remove the deleted patch `path` from its patchset's .inc."""
    inc_path = ib_inc_for(os.path.dirname(path))
    uri = 'file://' + os.path.basename(path)

    with open(inc_path) as f:
        lines = [l for l in f if l.split(';')[0].split()[:1] != [uri]]
    with open(inc_path, 'w') as f:
        f.writelines(lines)

python do_updiff() {
    import hashlib
    import os
    import shutil
    import subprocess
    import tempfile

    pf = d.getVar('PF')
    target = d.getVar('IB_TARGET')
    pristine = d.getVar('S') + '.pristine'
    staging = os.path.join(d.getVar('IB_PATCHSET_DIR'), pf)

    if not os.path.isdir(staging):
        bb.fatal(f"do_updiff: staging directory {staging} not found "
                 "(run do_diffcompose first?)")

    def fail(msg):
        shutil.rmtree(staging, ignore_errors=True)
        bb.fatal(f"do_updiff: {msg} Nothing was written.")

    series, index = ib_patch_series(d)
    staged = index_patchset(staging)
    hashes = ib_attach_hashes(target)

    with open(os.path.join(staging, '.edited'), errors='surrogateescape') as f:
        edited = sorted(line.rstrip('\n') for line in f)

    # A patch that built the tree but left the series makes an edited file look
    # unpatched: all it brought would be appended with the edit.

    attached = ib_attached_series(d)
    gone = (attached or set()) - ib_series_names(d)
    if gone and edited:
        fail(f"{target} was built with patches no longer in the series "
             f"({', '.join(sorted(gone)[:5])}): an override probably changed "
             "since the build. Restore it to fold your edits, or run "
             f"'./scripts/build.sh -c {d.getVar('PN')}' to rebuild the tree "
             "from the current series.")

    # A real edit adds a handful of files. Dozens of new ones is a built tree
    # seen through an incomplete exclude list: refuse before writing anything.

    limit = int(d.getVar('IB_UPDIFF_MAX_NEW') or 20)
    new = sorted(rel for rel in staged if rel not in index)
    if len(new) > limit:
        fail(f"{len(new)} files in {target} have no patch yet (limit {limit}), "
             f"e.g. {', '.join(new[:5])}. That is build output taken for "
             "source: name it in IB_UPDIFF_EXCLUDE, or raise IB_UPDIFF_MAX_NEW "
             "if the count is right.")

    # Where new patches go: the last patchset of the series with a plain
    # .inc, else the recipe's own 0001-${PF}/ (created on first use).
    append_dir = None
    for entry in reversed(series):
        setdir = os.path.dirname(entry['path'])
        if ib_set_is_plain(setdir, pf):
            append_dir = setdir
            break
    if append_dir is None:
        append_dir = os.path.join(d.getVar('IB_PATCHSET_DIR'), f"0001-{pf}")

    def final_of(rel):
        path = os.path.join(target, rel)
        if os.path.isfile(path) and not os.path.islink(path):
            with open(path, 'rb') as f:
                return f.read()
        return None

    # First pass: decide everything, write nothing.
    kept, rewritten, rewrites, appends = [], [], {}, []
    tmpdir = tempfile.mkdtemp(prefix='updiff-', dir=d.getVar('TMPDIR'))
    try:
        for rel in edited:
            chain = index.get(rel, [])
            if not chain:
                if rel in staged:
                    appends.append((rel, os.path.join(staging, staged[rel][0])))
                continue

            final = final_of(rel)
            run = ib_fold_run(chain, pf)
            try:
                replay = ib_apply_chain(rel, chain, pristine, tmpdir)
                if replay == final:
                    kept.append(rel)
                    continue

                digest = None if replay is None \
                    else hashlib.sha256(replay).hexdigest()
                if hashes is not None and digest != hashes.get(rel):
                    fail(f"{rel} was edited and its patches changed since the "
                         "attach. Save the edit, re-attach with "
                         "IB_FORCE_ATTACH=1, then redo it.")

                base = ib_apply_chain(rel, chain[:-len(run)], pristine, tmpdir)
            except RuntimeError as e:
                fail(f"{rel}: the series no longer applies ({e}).")

            head, section = run[0]
            if base == final:
                if not ib_foldable(chain[-1][0], pf):
                    fail(f"{rel} is back to its state before "
                         f"{os.path.basename(chain[-1][0]['path'])}: that patch "
                         "would become empty. Remove its section by hand (and "
                         "the patch from its .inc if it was the only one).")
                text = ''
            else:
                if head['strip'] != 1:
                    bb.warn(f"do_updiff: {rel}: {os.path.basename(head['path'])} "
                            f"uses striplevel={head['strip']}, skipped")
                    continue

                a = os.path.join(tmpdir, 'a')
                b = os.path.join(tmpdir, 'b')
                for path, content in ((a, base), (b, final)):
                    if content is None:
                        if os.path.exists(path):
                            os.remove(path)
                    else:
                        with open(path, 'wb') as f:
                            f.write(content)
                r = subprocess.run(['diff', '-uN', '--label', f'a/{rel}',
                                    '--label', f'b/{rel}', a, b],
                                   capture_output=True, text=True,
                                   errors='surrogateescape')
                if r.stdout.startswith('Binary files '):
                    bb.warn(f"do_updiff: {rel} is binary, skipped")
                    continue
                text = r.stdout

            rewrites.setdefault(head['path'], (head, {}))[1][section] = text
            for entry, n in run[1:]:
                rewrites.setdefault(entry['path'], (entry, {}))[1][n] = ''
            rewritten.append(rel)
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)

    # Second pass: write.

    removed, added = [], set()
    for path, (entry, new) in rewrites.items():
        sections = [new.get(n, s[2]) for n, s in enumerate(entry['sections'])]
        if not any(sections) and not ib_patch_has_text(entry['preamble']):
            os.remove(path)
            ib_inc_drop(path)
            removed.append(path)
            continue

        with open(path, 'w', errors='surrogateescape') as f:
            f.write(entry['preamble'] + ''.join(sections))

    if appends:
        os.makedirs(append_dir, exist_ok=True)
        used = [int(p[:4]) for p in os.listdir(append_dir)
                if p.endswith('.patch') and p[:4].isdigit()]
        next_number = max(used, default=0) + 1
        for rel, spath in appends:
            name = f"{next_number:04d}-{os.path.basename(rel)}.patch"
            shutil.copyfile(spath, os.path.join(append_dir, name))
            added.add(name)
            next_number += 1

        # A set the recipe does not require yet applies nothing, so what it
        # holds did not join the series.

        if append_dir not in {os.path.dirname(e['path']) for e in series}:
            added.clear()
            bb.warn(f"do_updiff: {ib_inc_for(append_dir)} is not used yet: "
                    "require it from the recipe")

        inc_path = ib_inc_for(append_dir)
        header = None
        if os.path.isfile(inc_path):
            lines = []
            with open(inc_path) as f:
                for line in f:
                    if line.startswith('#') or not line.strip():
                        lines.append(line)
                    else:
                        break
            header = ''.join(lines) or None
        with open(inc_path, 'w') as f:
            f.write(generate_src_uri(append_dir, header))

    if attached is not None:
        ib_record_attached_series(d, (attached | added)
                                  - {os.path.basename(p) for p in removed})

    # The series now yields these files, so the dirty-tree guards of
    # base.bbclass must stop flagging them.

    ib_refresh_attach_manifest(d, kept + rewritten + [rel for rel, _ in appends])

    shutil.rmtree(staging)

    if not edited:
        bb.plain("do_updiff: nothing edited since the attach")
        return

    sets = sorted({os.path.relpath(os.path.dirname(path), d.getVar('IB_DIR'))
                   for path in rewrites})
    bb.plain(f"do_updiff: {len(kept)} unchanged, {len(rewritten)} rewritten, "
             f"{len(removed)} merged away, {len(appends)} appended"
             + (f" (into {os.path.relpath(append_dir, d.getVar('IB_DIR'))})"
                if appends else "")
             + (f"; patchsets touched: {', '.join(sets)}" if sets else ""))
}

addtask do_diffcompose before do_updiff


EXPORT_FUNCTIONS do_patch do_diffcompose do_updiff
