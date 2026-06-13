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

# Infrabase patch generation algorithm
#
# Produces per-file patches in ${FILE_DIRNAME}/files/${PF}/ comparing the
# pristine upstream sources (${S}.pristine, saved by patch_save_pristine
# right after do_unpack) against the local working tree (IB_TARGET). Each
# staged patch is therefore a self-contained "upstream → final" diff for
# one source file; do_updiff can drop it in place at the slot of the
# existing patch for that file, fully replacing the previous content.
#
# Patch headers use git-style "a/<rel>" / "b/<rel>" labels so the resulting
# patches apply cleanly with `patch -p1` from the source tree root,
# independent of where the project was checked out.  Earlier versions of
# this task wrote absolute paths on both sides (e.g. /home/.../<user>/.../),
# which silently broke as soon as the repository was cloned anywhere else.
#
# The staging directory ${FILE_DIRNAME}/files/${PF}/ is wiped and recreated
# on every run so that stale patches from previous invocations do not leak
# into the new patchset (do_updiff promotes this directory in place into
# the existing numbered patchset).

python patch_do_diffcompose() {
    import os
    import re
    import shlex
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
    output_dir = os.path.join(d.getVar('FILE_DIRNAME'), 'files', d.getVar('PF'))

    if os.path.isdir(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)

    diff_files_tmp = os.path.join(d.getVar('TMPDIR'), "diff_files.tmp")

    # Enumerate differing/added/removed files.  cwd=target_dir so the target
    # side of each line is reported as "./<rel>".
    #
    # Excludes filter out build artefacts that leak into IB_TARGET when the
    # user runs `make` before `updiff`. Without this filter the patchset
    # gets polluted with hundreds of generated files (kconfig output, dtc
    # intermediates, kbuild auto-generated headers, etc.). Patterns are
    # matched against basenames by `diff -x`.
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
        # initramfs build deps
        '.initramfs_*',
    ]
    # Exclude the quilt working directory + kbuild-generated trees:
    #   patches/      — quilt staging from do_attach_infrabase
    #   include/generated/ — kbuild's auto-generated arch headers (cfi.h,
    #                       delay.h, etc. from arch/*/include/asm/Kbuild)
    #   include/config/    — kbuild's symbol-tracking files
    excludes = ' '.join(f'-x "{p}"' for p in exclude_patterns)
    excludes += ' --exclude=.git --exclude=patches'
    excludes += ' --exclude=generated --exclude=config'
    statement = (
        f'diff -Nrq {excludes} --no-dereference '
        f'{shlex.quote(source_dir)} . > {shlex.quote(diff_files_tmp)}'
    )
    subprocess.call(statement, shell=True, cwd=target_dir)

    new_number = 1
    src_norm = source_dir.rstrip('/') + '/'

    with open(diff_files_tmp, "r") as diff_file:
        for raw_line in diff_file:
            line = raw_line.rstrip('\n')
            if not line:
                continue

            src_path = None
            tgt_path = None
            rel = None

            # Form 1: "Files <src> and <tgt> differ"
            m = re.match(r'^Files (.+) and (.+) differ$', line)
            if m:
                src_path = m.group(1)
                tgt_path = m.group(2)
                if tgt_path.startswith('./'):
                    rel = tgt_path[2:]
                elif src_path.startswith(src_norm):
                    rel = src_path[len(src_norm):]
                else:
                    bb.warn(f"do_diffcompose: cannot derive relative path for: {line}")
                    continue
            else:
                # Form 2: "Only in <dir>: <name>"
                m = re.match(r'^Only in (.+): (.+)$', line)
                if not m:
                    bb.warn(f"do_diffcompose: unrecognised diff -Nrq line: {line}")
                    continue
                only_dir = m.group(1)
                only_name = m.group(2)
                full = os.path.join(only_dir, only_name)
                if only_dir == source_dir or only_dir.startswith(src_norm):
                    # Present only in source -> file deleted in target.
                    src_path = full
                    rel = os.path.relpath(full, source_dir)
                    tgt_path = os.path.join('.', rel)
                else:
                    # only_dir is relative to cwd=target_dir; file added locally.
                    tgt_path = full
                    rel = os.path.relpath(full, '.')
                    src_path = os.path.join(source_dir, rel)

            filename = os.path.basename(rel)
            output_patch = os.path.join(output_dir, f"{new_number:04d}-{filename}.patch")

            # Per-file unified diff with git-style a/<rel> b/<rel> labels so
            # the patch applies cleanly with `patch -p1` regardless of the
            # checkout location.
            cmd = [
                'diff', '--no-dereference', '-uN',
                '--label', f'a/{rel}',
                '--label', f'b/{rel}',
                src_path, tgt_path,
            ]
            with open(output_patch, 'wb') as out:
                subprocess.run(cmd, stdout=out, cwd=target_dir, check=False)

            # `diff -uN` emits "Binary files <a> and <b> differ" instead of
            # a usable hunk when either side is binary. Writing that as a
            # .patch file makes do_patch fail later with "Only garbage was
            # found in the patch input". These are typically build
            # artefacts (e.g. compiled scripts/kconfig/conf) that leaked
            # into IB_TARGET; just drop the patch and move on.
            with open(output_patch, 'rb') as f:
                first = f.readline()
            if first.startswith(b'Binary files '):
                bb.note(f"do_diffcompose: skipping binary file: {rel}")
                os.remove(output_patch)
                continue

            new_number += 1

    os.remove(diff_files_tmp)
}

def generate_src_uri(patch_directory):
    patches = sorted(p for p in os.listdir(patch_directory)
                     if p.endswith(".patch"))
    src_uri = "SRC_URI += \"\\ \n"
    for patch in patches:
        src_uri += "    file://" + patch + " \\ \n"
    src_uri += "\"\n"
    return src_uri.strip()


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

addtask do_updiff
do_updiff[nostamp] = "1"

# Merge the staged patchset produced by do_diffcompose
# (${FILE_DIRNAME}/files/${PF}/) into the canonical numbered patchset.
#
# Staged patches are self-contained "pristine → IB_TARGET" diffs (one per
# source file). do_updiff decides, per file, between three outcomes:
#
#   * CONSOLIDATE — the patchset already has exactly one patch targeting
#     this file. We overwrite that patch in place with the new content
#     (keeping its NNNN prefix and series position). This works because
#     no earlier patch in the series modifies the file (otherwise the
#     existing patch would coexist with another), so the slot's ${S}
#     state at do_patch time equals the pristine state our diff is based
#     on — the regenerated patch applies cleanly.
#   * APPEND — no patch exists yet for this file. The next-available
#     NNNN gets the new patch at the end of the series, where ${S} is
#     already at its post-all-previous-patches state and the diff (which
#     was generated against pristine) is the complete divergence from
#     upstream → applies cleanly.
#   * SKIP (chain warning) — the patchset already has TWO OR MORE patches
#     for this file (a chain of incremental edits accumulated over past
#     append-only updiff runs). Consolidation would have to pick one
#     entry of the chain to overwrite, and the others would either be
#     redundant or actively conflicting. Append would re-apply the chain's
#     cumulative effect a second time at the end, which do_patch rejects.
#     We log a warning and leave the chain alone; resolution is manual:
#     either delete every patch in the chain except the first one (then
#     re-run updiff to consolidate into that first slot), or hand-merge
#     them into one patch.
#
# Patches present in the target but not in staging are ALWAYS kept
# untouched (e.g. when an existing patch creates a file that's been
# excluded from the diff, or when staging is empty because the user
# hasn't edited anything yet).
#
# If no numbered patchset exists yet, a fresh 0001-${PF}/ is populated.
# The corresponding NNNN-${PF}-patches.inc is regenerated (sorted) from
# the actual contents of the patchset directory.

python do_updiff() {
    import os
    import re
    import shutil
    import filecmp

    patchdir = d.getVar('FILE_DIRNAME') + '/files/'
    pf = d.getVar('PF')
    staging = os.path.join(patchdir, pf)

    if not os.path.isdir(staging):
        bb.fatal(f"do_updiff: staging directory {staging} not found "
                 "(run do_diffcompose first?)")

    # Find existing numbered patchsets matching this PF.
    pattern = re.compile(r'^(\d{4})-' + re.escape(pf) + r'$')
    existing = []
    if os.path.isdir(patchdir):
        for entry in os.listdir(patchdir):
            full = os.path.join(patchdir, entry)
            m = pattern.match(entry)
            if m and os.path.isdir(full):
                existing.append((int(m.group(1)), entry))

    if existing:
        existing.sort()
        target_dirname = existing[0][1]
        target_dir = os.path.join(patchdir, target_dirname)
        bb.note(f"do_updiff: merging into existing patchset {target_dirname}")
    else:
        target_dirname = f"0001-{pf}"
        target_dir = os.path.join(patchdir, target_dirname)
        os.makedirs(target_dir, exist_ok=True)
        bb.note(f"do_updiff: creating initial patchset {target_dirname}")

    target_idx = index_patchset(target_dir)
    staging_idx = index_patchset(staging)

    # Highest existing NNNN prefix in the target — used for new patches.
    used_numbers = set()
    for fnames in target_idx.values():
        for fname in fnames:
            try:
                used_numbers.add(int(fname[:4]))
            except ValueError:
                pass
    next_number = max(used_numbers, default=0) + 1

    kept = consolidated = appended = skipped_chain = 0

    # Per-file: CONSOLIDATE (overwrite the single existing patch),
    # APPEND (no existing patch yet), or SKIP (existing chain — manual
    # consolidation required). See the docstring above.
    for pid, sname_list in staging_idx.items():
        # do_diffcompose only ever emits one patch per pid.
        sname = sname_list[0]
        spath = os.path.join(staging, sname)
        existing = target_idx.get(pid, [])
        if len(existing) == 1:
            tname = existing[0]
            tpath = os.path.join(target_dir, tname)
            if filecmp.cmp(spath, tpath, shallow=False):
                kept += 1
            else:
                shutil.copyfile(spath, tpath)
                consolidated += 1
        elif len(existing) == 0:
            new_name = f"{next_number:04d}-{os.path.basename(pid)}.patch"
            shutil.copyfile(spath, os.path.join(target_dir, new_name))
            next_number += 1
            appended += 1
        else:
            bb.warn(
                f"do_updiff: {pid} already has {len(existing)} patches "
                f"in the series ({', '.join(existing)}). Refusing to "
                f"append a cumulative diff on top: it would re-apply the "
                f"chain's content and break do_patch. Manually merge the "
                f"chain into a single patch, then re-run updiff."
            )
            skipped_chain += 1

    # Drop the staging directory.
    shutil.rmtree(staging)

    # Regenerate the .inc manifest (sorted, deterministic).
    inc_path = os.path.join(patchdir, target_dirname + "-patches.inc")
    with open(inc_path, 'w') as f:
        f.write(generate_src_uri(target_dir))

    bb.plain(f"do_updiff: patchset {target_dirname}: "
             f"{kept} unchanged, {consolidated} consolidated, "
             f"{appended} appended, {skipped_chain} skipped (existing chain) "
             f"({sum(len(v) for v in target_idx.values())} existing patches kept intact)")
}

addtask do_diffcompose before do_updiff


EXPORT_FUNCTIONS do_patch do_diffcompose do_updiff
