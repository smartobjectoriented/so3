###################################################################
#
#   Various utility functions
#
#   Copyright (c) 2025-2026 EDGEMTech Ltd
#
#   Authors:
#       EDGEMTech Ltd, Daniel Rossier (daniel.rossier@edgemtech.ch)
#       EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
#
###################################################################

def base_set_filespath(path, d):
    filespath = []
    extrapaths = (d.getVar("FILESEXTRAPATHS") or "")

    # Remove default flag which was used for checking
    extrapaths = extrapaths.replace("__default:", "")

    # Don't prepend empty strings to the path list
    if extrapaths != "":
        path = extrapaths.split(":") + path

    # The ":" ensures we have an 'empty' override
    overrides = (":" + (d.getVar("FILESOVERRIDES") or "")).split(":")
    overrides.reverse()
    for o in overrides:
        for p in path:
            if p != "":
                filespath.append(os.path.join(p, o))
    return ":".join(filespath)


# Run a single command under `sudo -n` (non-interactive). bitbake itself
# runs unprivileged: only the specific operations that genuinely need
# root (mount/umount/losetup/mkfs/parted/...) are escalated here.
#
# The caller chain (deploy.sh / build.sh / mount.sh / umount.sh) MUST
# have validated the sudo timestamp via `sudo_session_start` before
# bitbake is launched — otherwise the very first `utils_sudo` call
# fails fast with sudo's standard "a password is required" message.
#
# Args:
#   cmd:   list of argv items (preferred) — escapes nothing for you
#          but is shell-injection-proof
#   shell: when True, `cmd` is a shell command string and the call
#          becomes `sudo -n sh -c <cmd>`. Use this for pipelines.
#          Stdout redirection inside `cmd` runs as root: pass
#          `stdout=open(path, 'wb')` instead to keep the output file
#          user-owned.
#   **kwargs: forwarded to subprocess.run (check, capture_output,
#             input, stdin, stdout, stderr, text, cwd, env, ...).

def utils_sudo(cmd, shell=False, **kwargs):
    import subprocess
    if shell:
        return subprocess.run(["sudo", "-n", "sh", "-c", cmd], **kwargs)
    return subprocess.run(["sudo", "-n"] + list(cmd), **kwargs)
