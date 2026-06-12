# Copyright (C) 2006  OpenedHand LTD
# Copyright (C) 2023-2025 EDGEMTech Ltd
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

# Infrabase patch generation algorithm
python patch_do_diffcompose() {
    import subprocess
  
    source_dir = d.getVar('S')
    target_dir = d.getVar('IB_TARGET')
    output_dir = d.getVar('FILE_DIRNAME') + '/files/' + d.getVar('PF')
    
    # Create the output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Diff files
    
    # To avoid having the temporary file in the directory to patch,
    # we create it in the parent directory.
    diff_files_tmp = os.path.join(d.getVar('TMPDIR'), "diff_files.tmp")
    
    statement = 'diff -Nrq -x "*.su" -x "*.o.d" -x "*.ko" -x "*.mod.*" -x "*.o" -x "*.cmd" -x "*.a" --no-dereference --exclude=.git ' + source_dir + ' . > ' + diff_files_tmp
    subprocess.call(statement, shell=True, cwd=target_dir)
 
    # Prefix before patch (ex. : 0001-mypatch.patch)
    new_number = 1
    
    with open(diff_files_tmp, "r") as diff_file:
        for line in diff_file:
            line = line.strip()
            line_parts = line.split()
            
            type = line_parts[0]
            src = line_parts[1]
            target = line_parts[3]
                 
            filename = os.path.basename(target)
            statement = 'diff --no-dereference -Naur ' + src + ' ' + target + ' --exclude=.git > ' + output_dir + '/' + f"{new_number:04d}-{filename}" + '.patch'                
            subprocess.call(statement, shell=True, cwd=target_dir)
            
            new_number += 1
            
    os.remove(diff_files_tmp)
}

def generate_src_uri(patch_directory):
    patches = [patch for patch in os.listdir(patch_directory) if patch.endswith(".patch")]
    src_uri = "SRC_URI += \"\\ \n"
    for patch in patches:
        src_uri += "    file://" + patch + " \\ \n"
    src_uri += "\"\n"
    return src_uri.strip()

addtask do_updiff
do_updiff[nostamp] = "1"

python do_updiff() {
    
    # Get the highest four-digit number among existing files
    highest_number = 0
    
    patchdir = d.getVar('FILE_DIRNAME') + '/files/'
    
    new_number = get_next_number(patchdir)
    	
    patchfile = d.getVar('PF') + ".patch"
    
    if not os.path.exists(patchfile):
        patchfile = d.getVar('PF')
        
    # Rename the file with the new prefix
    
    new_filename = f"{new_number:04d}-{patchfile}"
    
    os.rename(patchdir + patchfile, patchdir + new_filename)
    print(f"Renamed {patchfile} to {new_filename}")
    
    src_uri = generate_src_uri(patchdir + new_filename)
    with open(patchdir + new_filename + "-patches.inc", 'w') as file:
        file.write(src_uri)
}

addtask do_diffcompose before do_updiff


EXPORT_FUNCTIONS do_patch do_diffcompose do_updiff
