#
# Copyright OpenEmbedded Contributors
#
# SPDX-License-Identifier: MIT
#

inherit logging
inherit patch
inherit utils

# Specific to IB: we always consider the ${S} directory as our source patched directory.
# Hence, we have to move all specific subdirs like git to this subdir. 
# At the moment, only git is supported.

FILESPATH = "${@base_set_filespath(["${FILE_DIRNAME}/${P}", "${FILE_DIRNAME}/${PN}", "${FILE_DIRNAME}/files"], d)}"

# THISDIR only works properly with imediate expansion as it has to run
# in the context of the location its used (:=)

THISDIR = "${@os.path.dirname(d.getVar('FILE'))}"

# ---------------------------------------------------------------------------
# The boot chain
# ---------------------------------------------------------------------------
# IB_BOOT_CHAIN names every stage that runs before the payload, in the order
# it runs, joined by "+":
#
#     atf + optee + uboot + avz + mcuboot
#
# So "uboot" is U-Boot alone, "atf+optee+uboot+avz" is the edge-m1 capsule
# chain, "uboot+mcuboot" is U-Boot handing over to MCUboot, and "mcuboot" is
# MCUboot as the first stage with no U-Boot at all.
#
# Ordered, and validated as such: "atf+avz+uboot" is refused, because AVZ is
# entered by U-Boot's guest-boot and cannot precede it. A stage may appear at
# most once, "optee" requires "atf", "avz" requires "uboot", and a chain must
# contain something that can enter a payload.
#
# Everything else about the boot shape is DERIVED from it:
#
#   IB_HYPERVISOR        "avz" when the chain has it, else "none"
#   IB_CHAIN_HAS_<STAGE> "1" per stage present, for the recipes that ask
#   IB_CHAIN_STAGES      the stages, space-separated
#   IB_TARGET_ITS        gets the platform's _avz bundle when AVZ is in
#   IB_ZEPHYR_BOOT_APP   "mcuboot" when the chain names it
#
# That derivation is the point. The shape used to live in four variables
# that had to agree by hand, and three separate boot failures came from them
# disagreeing — a CI cell whose ITS name said one thing and whose deploy said
# another, and twice a guest ITB carrying a bootloader's payload as though it
# were bootable on its own. One ordered string cannot disagree with itself.
#
# What a platform supports is still declared per platform in
# build/conf/local.conf, as a set of stages rather than a list of whole
# chains (IB_BOOT_STAGES_SUPPORTED / IB_BOOT_STAGES_REQUIRED).
#
# "full" is accepted as a LEGACY alias for "atf+optee+uboot+avz". It is
# expanded here, once, so that no recipe, script or .inc has to know about it.
# Kept so an edge-m1 / so3 tree aligning onto this one does not have to change
# its local.conf in the same step.
#
# Why base.bbclass: IB_BOOT_CHAIN is read from recipes whose OVERRIDES carry
# no :linux (uboot, atf, optee), so a scoped assignment in local.conf is
# invisible to them. Normalising per-recipe, in the class every recipe
# inherits, is the only place that reaches all of them.

def ib_normalize_boot_axes(d):
    """Parse the chain, derive what follows from it, refuse what cannot work."""

    # The chain is an ORDERED list of the stages that run before the payload,
    # written in the order they run and joined by "+". This is the whole boot
    # shape in one value: "what runs, in what order".
    #
    # Everything else is derived from it. Before, the shape was spread over
    # four variables that had to agree — the chain, IB_HYPERVISOR, the "_avz"
    # suffix on IB_TARGET_ITS and IB_ZEPHYR_BOOT_APP — and three separate
    # boot failures came from them disagreeing: a CI cell whose ITS name said
    # one thing and whose deploy said another, and twice a guest ITB carrying
    # a bootloader's payload as though it were bootable on its own.
    STAGES = ("atf", "optee", "uboot", "avz", "mcuboot")

    # What each stage requires of the ones before it. A chain is valid when
    # its stages are a subsequence of STAGES and every requirement is met.
    REQUIRES = {
        "optee":   ("atf",),    # OP-TEE is BL32, loaded by ATF's BL2
        "avz":     ("uboot",),  # AVZ is entered by U-Boot's guest-boot
    }

    ALIASES = {
        # The edge-m1 capsule chain, from before the stages were spelled out.
        # Kept so a tree aligning onto this one need not change local.conf in
        # the same step.
        "full": "atf+optee+uboot+avz",
    }

    chain = (d.getVar('IB_BOOT_CHAIN') or "").strip()
    plat = d.getVar('IB_PLATFORM') or "<unset>"

    # An empty chain has always meant "bare U-Boot" in this build system.
    if chain == "":
        chain = "uboot"
    chain = ALIASES.get(chain, chain)

    stages = [t for t in chain.split("+") if t]

    unknown = [t for t in stages if t not in STAGES]
    if unknown:
        bb.fatal("IB_BOOT_CHAIN=\"%s\": unknown stage%s %s.\n"
                 "Known stages, in the order they may appear: %s."
                 % (chain, "" if len(unknown) == 1 else "s",
                    ", ".join('"%s"' % u for u in unknown), " ".join(STAGES)))

    order = [STAGES.index(t) for t in stages]
    if order != sorted(order) or len(set(order)) != len(order):
        bb.fatal("IB_BOOT_CHAIN=\"%s\" is not in boot order.\n"
                 "Stages run in this order and each appears at most once: %s.\n"
                 "So \"atf+uboot+avz\", not \"atf+avz+uboot\" — AVZ is started "
                 "by U-Boot's guest-boot, it does not run before it."
                 % (chain, " ".join(STAGES)))

    for stage, needs in REQUIRES.items():
        if stage in stages:
            missing = [n for n in needs if n not in stages]
            if missing:
                bb.fatal("IB_BOOT_CHAIN=\"%s\": \"%s\" needs %s in the chain."
                         % (chain, stage, " and ".join('"%s"' % m for m in missing)))

    if "uboot" not in stages and "mcuboot" not in stages:
        bb.fatal("IB_BOOT_CHAIN=\"%s\" has nothing that can enter a payload.\n"
                 "A chain needs \"uboot\" or \"mcuboot\"." % chain)

    chain = "+".join(stages)

    # Derived, so that nothing has to be kept in step by hand.
    #
    # IB_HYPERVISOR stays a variable of its own because that is the question
    # most readers actually ask ("is there a hypervisor"), and asking it of a
    # string is worse than asking it of a name.
    hyp = "avz" if "avz" in stages else "none"

    d.setVar('IB_CHAIN_STAGES', " ".join(stages))
    for stage in STAGES:
        d.setVar('IB_CHAIN_HAS_%s' % stage.upper(),
                 "1" if stage in stages else "")

    # The bootloader that sits between the chain and the payload, when the
    # chain names one. Overridable: this says WHETHER, a tree may still say
    # WHICH by setting IB_ZEPHYR_BOOT_APP to something else.
    if "mcuboot" in stages and not d.getVar('IB_ZEPHYR_BOOT_APP'):
        d.setVar('IB_ZEPHYR_BOOT_APP', "mcuboot")

    # The AVZ bundle is the same ITB for every OS on a platform — it carries
    # AVZ and its device tree, and the OS lives in the guest ITB beside it.
    # So the target is ${IB_PLATFORM}_avz whenever AVZ is in the chain, and
    # no layer has to declare it. An explicit _avz name still wins, for a
    # platform whose file is spelled differently.
    its = d.getVar('IB_TARGET_ITS') or ""
    if hyp == "avz" and not its.endswith("_avz"):
        d.setVar('IB_TARGET_ITS', "%s_avz" % plat)

    # Platform capability check. These are facts about the SoC and about
    # what is available upstream, declared per platform in conf/local.conf;
    # failing here, at parse time, beats failing in the middle of a firmware
    # link or — worse — booting a board that then stays silent.
    #
    # Declared as a SET OF STAGES, not as a list of whole chains. The chain
    # is an ordered list, so enumerating the acceptable chains would mean
    # enumerating every combination of the stages a platform allows — six
    # for a platform that runs ATF, OP-TEE and AVZ — and forgetting one
    # refuses a build that works. A stage set says the same thing once, and
    # says it the way the reason is actually phrased: "rpi4_64 has no OP-TEE
    # upstream", "virt32 has no EL2".
    #
    # IB_BOOT_STAGES_REQUIRED is the other half: a stage the platform cannot
    # boot WITHOUT. The i.MX8MP boot ROM always installs BL31 before U-Boot,
    # so "atf" is required there and a bare "uboot" chain does not exist on
    # that SoC.

    allowed = (d.getVar('IB_BOOT_STAGES_SUPPORTED') or "").split()
    if allowed:
        refused = [s for s in stages if s not in allowed]
        if refused:
            bb.fatal("Platform \"%s\" cannot run boot stage%s %s "
                     "(IB_BOOT_CHAIN=\"%s\").\n"
                     "Supported on this platform: %s.\n"
                     "See IB_BOOT_STAGES_SUPPORTED in build/conf/local.conf "
                     "for why."
                     % (plat, "" if len(refused) == 1 else "s",
                        ", ".join('"%s"' % r for r in refused), chain,
                        " ".join(allowed)))

    required = (d.getVar('IB_BOOT_STAGES_REQUIRED') or "").split()
    missing = [s for s in required if s not in stages]
    if missing:
        bb.fatal("Platform \"%s\" cannot boot without %s "
                 "(IB_BOOT_CHAIN=\"%s\").\n"
                 "Required on this platform: %s.\n"
                 "See IB_BOOT_STAGES_REQUIRED in build/conf/local.conf for why."
                 % (plat, " and ".join('"%s"' % m for m in missing), chain,
                    " ".join(required)))

    # Write the normalised values back. Any scoped variant still in effect
    # has to go with them: getVar() resolves overrides, but setVar() writes
    # the BASE name only — so an `IB_BOOT_CHAIN:linux = "full"` (the shape an
    # edge-m1 tree carries) would keep winning on the next read and the
    # expansion above would silently not stick. Dropping the in-effect
    # variants makes the normalised value the single answer for every reader.

    for var, value in (('IB_BOOT_CHAIN', chain), ('IB_HYPERVISOR', hyp)):
        for override in (d.getVar('OVERRIDES') or "").split(':'):
            if override:
                d.delVar('%s:%s' % (var, override))
        d.setVar(var, value)


# Runs before the anonymous python below (and before any class that
# inherits base), so every later reader sees the normalised values.
python () {
    ib_normalize_boot_axes(d)
}


python () {
    import sys
    import os

    # Fetch the BBLAYERS variable from the BitBake datastore
    bblayers = d.getVar('BBLAYERS', True)
    if bblayers:
        for layer_path in bblayers.split():
            lib_path = os.path.join(layer_path, 'lib')
            if os.path.isdir(lib_path) and lib_path not in sys.path:
                sys.path.insert(0, lib_path)

    need_machine = d.getVar('COMPATIBLE_PLATFORM')
    if need_machine and not bb.utils.to_boolean(d.getVar('PARSE_ALL_RECIPES', False)):
        import re
        compat_machines = (d.getVar('PLATFORMOVERRIDES') or "").split(":")
        for m in compat_machines:
            if re.match(need_machine, m):
                break
        else:
            raise bb.parse.SkipRecipe("incompatible with machine %s (not in COMPATIBLE_PLATFORM)" % d.getVar('IB_PLATFORM'))

    # Boot STAGES this recipe cannot be built behind. Stated as what is
    # excluded rather than what is allowed, unlike COMPATIBLE_PLATFORM: a
    # recipe that lists what it supports has to be revisited every time a
    # stage is added.
    #
    # A stage and not a whole chain, because the chain is now an ordered list
    # — "mcuboot" and "uboot+mcuboot" both put MCUboot in front of the
    # payload, and a recipe that cannot be MCUboot's payload cannot be it in
    # either.
    #
    # SkipRecipe and not bb.fatal: bitbake parses every recipe in BBFILES
    # whatever is being built, so a fatal here would stop a build of some
    # other recipe that is perfectly compatible. Skipped, the recipe is
    # simply not available, and asking for it by name says so.
    bad = (d.getVar('INCOMPATIBLE_BOOT_STAGE') or "").split()
    stages = (d.getVar('IB_CHAIN_STAGES') or "").split()
    clash = [b for b in bad if b in stages]
    if clash and not bb.utils.to_boolean(d.getVar('PARSE_ALL_RECIPES', False)):
        raise bb.parse.SkipRecipe(
            'cannot be built into a chain containing "%s" '
            '(INCOMPATIBLE_BOOT_STAGE)' % clash[0])

}


BB_DEFAULT_TASK ?= "build"
CLASSOVERRIDE ?= "class-target"

die() {
	bbfatal_log "$*"
}

BASEDEPENDS = ""

DEPENDS:prepend = "${BASEDEPENDS} "
 
# THISDIR only works properly with imediate expansion as it has to run
# in the context of the location its used (:=)

THISDIR = "${@os.path.dirname(d.getVar('FILE'))}"

addtask fetch
do_fetch[dirs] = "${DL_DIR}"
do_fetch[file-checksums] = "${@bb.fetch.get_checksum_file_list(d)}"

do_fetch[vardeps] += "SRCREV"
python do_fetch() {

    src_uri = (d.getVar('SRC_URI') or "").split()
    if not src_uri:
        return

    try:
        fetcher = bb.fetch2.Fetch(src_uri, d)
        fetcher.download()
    except bb.fetch2.BBFetchException as e:
        bb.fatal("Bitbake Fetcher Error: " + repr(e))
}

addtask listtasks
do_listtasks[nostamp] = "1"
python do_listtasks() {
    taskdescs = {}
    maxlen = 0
    for e in d.keys():
        if d.getVarFlag(e, 'task'):
            maxlen = max(maxlen, len(e))
            if e.endswith('_setscene'):
                desc = "%s (setscene version)" % (d.getVarFlag(e[:-9], 'doc') or '')
            else:
                desc = d.getVarFlag(e, 'doc') or ''
            taskdescs[e] = desc

    tasks = sorted(taskdescs.keys())
    for taskname in tasks:
        bb.plain("%s  %s" % (taskname.ljust(maxlen), taskdescs[taskname]))
}

do_unpack[dirs] = "${WORKDIR}"
do_unpack[cleandirs] = "${@d.getVar('S') if os.path.normpath(d.getVar('S')) != os.path.normpath(d.getVar('WORKDIR')) else os.path.join('${S}', 'patches')}"

python do_unpack() {
    src_uri = (d.getVar('SRC_URI') or "").split()
    if not src_uri:
        return

    try:
        fetcher = bb.fetch2.Fetch(src_uri, d)
        fetcher.unpack(d.getVar('WORKDIR'))
    except bb.fetch2.BBFetchException as e:
        bb.fatal("Bitbake Fetcher Error: " + repr(e))
}

addtask do_handle_symlinks
python do_handle_symlinks() {
    import subprocess
    
    s = d.getVar('IB_SYMLINK:%s' % d.getVar('PN'))
    
    if s:
        l = d.getVar('IB_SYMLINK:%s' % d.getVar('PN')).split()
    
        for i in range(0, len(l), 3):
            where = l[i]
            src = l[i+1] if i + 1 < len(l) else None
            link = l[i+2] if i + 2 < len(l) else None
 
            where = d.getVar('S') + '/' + where 
 
            statement = 'ln -fs ' + src + ' ' + link
            subprocess.call(statement, shell=True, cwd=where)
}

def move_gitdir(d, dst_dir):
    import os
    import subprocess
    import shlex

    target_dir = d.getVar('S')
    gitdir = os.path.join(d.getVar('WORKDIR'), 'git')
    
    # Make sure the target directory exists
    cmd = f"mkdir -p {target_dir}/{dst_dir}"
    result = subprocess.run(cmd, shell=True, check=True)

    # Copy everything except .git, preserving symlinks/metadata
    cmd = (
        "find . -mindepth 1 -path './.git' -prune -o "
        "-exec cp -a --parents -t {} {{}} +"
    ).format(shlex.quote(os.path.join(target_dir, dst_dir)))
    
    result = subprocess.run(cmd, shell=True, check=True, cwd=gitdir)

    # Now erase the git directory
    cmd = f"rm -rf {gitdir}"
    result = subprocess.run(cmd, shell=True, check=True)


# If some files are fetched from a git directory, bitbake
# unpacked them to ${WORKDIR}/git directory. So, we want
# to move the contents to the target ${S} directory so that
# doing a updiff task will use the same approach for all recipes

# Additional note: this function can be overriden in recipes
# (.bbappend or other) using prepend (if many). So, the function
# may return.

python do_handle_fetch_git() {
    import os
    import subprocess
    import shlex
 
    workdir = d.getVar('WORKDIR')
    src_dir = os.path.join(workdir, 'git')

    if not os.path.isdir(src_dir):
        bb.note(f"Source directory {src_dir} does not exist — skipping copy.")
        return

    move_gitdir(d, '.')
 
}
do_unpack[postfuncs] = "do_handle_fetch_git"
 
# Default to the safe behaviour; set IB_FORCE_ATTACH=1 (env or local.conf)
# to override the dirty-tree guard below.
IB_FORCE_ATTACH ??= "0"

do_attach_infrabase () {
	ib_manifest="${IB_TARGET}.attach.sha256"

	# Guard against silently clobbering local edits.
	#
	# do_attach_infrabase regenerates ${IB_TARGET} from the freshly
	# fetched+patched ${S}. The fetched component trees (avz/, u-boot/,
	# qemu/, ...) are gitignored, so an edit made directly in ${IB_TARGET}
	# that hasn't been folded back into the patch set (via do_updiff) has no
	# version-control safety net — re-attaching would destroy it. After each
	# attach we record a sha256 manifest of the files we wrote; on the next
	# attach we verify those files are still untouched and abort if any were
	# modified or removed. Build artefacts produced by a later `make` are not
	# in the manifest, so a freshly-built tree still verifies clean. Newly
	# *added* source files are not tracked by the manifest — the ${IB_TARGET}.back
	# copy remains the last-resort net for that case.
	if [ -d "${IB_TARGET}" ] && [ -f "$ib_manifest" ] && [ "${IB_FORCE_ATTACH}" != "1" ]; then
		# The quilt staging left by do_patch (patches/, series, .pc) is a
		# build product, not source -- it is regenerated on every patch run
		# and is gitignored. Recorded in the manifest it made the guard fire
		# on its own artefacts and refuse a perfectly clean tree. Filtered
		# here as well as pruned below, so manifests recorded before this
		# fix stop blocking too. `|| true`: grep exits 1 when it filters
		# everything away, and bitbake runs shell tasks under `set -e`.
		ib_dirty=$(cd "${IB_TARGET}" && LC_ALL=C sha256sum -c --quiet "$ib_manifest" 2>/dev/null \
			| sed -n 's/: FAILED.*$//p' \
			| grep -vE '(^|/)(\.pc|patches)/' || true)
		if [ -n "$ib_dirty" ]; then
			bbwarn "Local modifications detected in ${IB_TARGET}, not captured in the ${PN} patch set:"
			echo "$ib_dirty" | sed 's|^\./|    |' >&2
			bbfatal "Refusing to re-attach ${PN} (would overwrite the files above). Run 'bitbake ${PN} -c updiff' to fold them into the patch set, or set IB_FORCE_ATTACH=1 to discard them and re-attach (prior tree is kept in ${IB_TARGET}.back)."
		fi
	fi

	echo "Attaching ${PN} to ${IB_TARGET}"

	if [ -d "${IB_TARGET}" ]; then
		rm -rf ${IB_TARGET}.back
		mv ${IB_TARGET} ${IB_TARGET}.back
	fi

	mkdir -p ${IB_TARGET}
	cp -r ${S}/. ${IB_TARGET}

	# Record what we just wrote so the next attach can detect local edits.
	( cd "${IB_TARGET}" && find . \( -name .pc -o -name patches \) -prune -o -type f -print0 \
		| LC_ALL=C sort -z | xargs -0 sha256sum ) > "$ib_manifest" 2>/dev/null || rm -f "$ib_manifest"

	# Record WHICH recipe the tree now belongs to. Several recipes can share
	# one IB_TARGET -- uboot_2022.04 and uboot_2024.07 both attach to
	# u-boot/ -- and each has its own do_attach_infrabase stamp. Without
	# this marker, attaching one and then building the other finds a valid
	# stamp, skips the attach, and compiles the wrong source tree. See
	# do_check_attach() in this class.
	echo "${PF}" > "${IB_TARGET}.attach.pf"
}

# Refuse to build against a tree that belongs to another recipe.
#
# IB_TARGET is a path in the source tree, not under tmp/, and more than one
# recipe can claim it: uboot_2022.04 and uboot_2024.07 both attach to
# u-boot/, selected by PREFERRED_VERSION per platform. do_attach_infrabase
# is stamped per recipe, so after building platform A and switching to
# platform B, B's recipe finds its own stamp from an earlier session, skips
# the attach, and runs do_configure against A's sources.
#
# That fails loudly when the defconfig is missing -- rpi4_64_defconfig in a
# 2024.07 tree -- and SILENTLY when it is not. A virt64 image was built with
# U-Boot 2022.04 instead of 2024.07; the two differ in
# CONFIG_POSITION_INDEPENDENT, so ATF loaded BL33 at 0x60000000 and jumped
# into a binary linked for 0x0. No output, no error, and a whole secure boot
# chain to bisect before the cause turned out to be the build system.
#
# The marker written by do_attach_infrabase names the recipe that owns the
# tree. This task is nostamp, so it is the one thing that always runs, and it
# refuses rather than repairing: re-attaching means deleting a tree the user
# may have edited, and that decision is theirs. A tree with no marker was
# attached before markers existed and is adopted silently.

do_check_attach[nostamp] = "1"
python do_check_attach() {
    import os

    target = d.getVar('IB_TARGET')
    pf = d.getVar('PF')
    if not target or not pf or not os.path.isdir(target):
        return

    try:
        with open(target + '.attach.pf') as f:
            owner = f.read().strip()
    except OSError:
        return

    if owner == pf:
        return

    bb.fatal(
        "%s is attached to %s, not to %s.\n"
        "Both recipes share this directory, and each has its own attach "
        "stamp, so building now would compile the other recipe's sources.\n"
        "Re-attach before continuing:\n"
        "    rm -f %s.do_attach_infrabase\n"
        "    IB_FORCE_ATTACH=1 bitbake %s -c attach_infrabase\n"
        "Anything you edited in %s that is not in the patch series will be "
        "lost; the previous tree is kept as %s.back."
        % (target, owner, pf, d.getVar('STAMP'), d.getVar('PN'), target, target)
    )
}

addtask do_check_attach after do_attach_infrabase before do_configure

addtask cleansstate after do_clean
python do_cleansstate() {
        sstate_clean_cachefiles(d)
}
addtask cleanall after do_cleansstate
do_cleansstate[nostamp] = "1"

python do_cleanall() {
    src_uri = (d.getVar('SRC_URI') or "").split()
    if not src_uri:
        return

    try:
        fetcher = bb.fetch2.Fetch(src_uri, d)
        fetcher.clean()
    except bb.fetch2.BBFetchException as e:
        bb.fatal(str(e))
}
do_cleanall[nostamp] = "1"

addtask do_fetch before do_unpack
addtask do_unpack before do_patch
addtask do_patch before do_handle_symlinks
addtask do_handle_symlinks before do_attach_infrabase
addtask do_attach_infrabase before do_configure
addtask do_configure before do_build
addtask do_build 

EXPORT_FUNCTIONS do_fetch listtasks do_unpack do_attach_infrabase do_handle_symlinks


