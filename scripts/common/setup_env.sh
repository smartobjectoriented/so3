# Source me from any user-facing script under scripts/ to keep the
# build environment aligned with the user's intent. Three trees may
# disagree at invocation time:
#
#   A) script tree — derived from `command -v "$0"` (where the binary
#                    came from: explicit path or PATH lookup).
#   B) cwd tree    — found by walking up $PWD looking for env.sh.
#   C) env tree    — value of IB_ROOT_DIR in the parent shell.
#
# Target = B if cwd lives inside an infrabase tree, else A. This honors
# the "I cd'd here, build here" intuition — without it, a PATH-resolved
# binary from a sibling worktree (because env.sh was sourced there
# earlier) silently wins and the user's `cd` is ignored.
#
# When C disagrees with the target, env.sh is re-sourced from the
# target (interactive prompt) or the script aborts (non-interactive,
# e.g. CI). The switch is local to THIS invocation — the parent shell
# stays as it was; the user is told to re-source env.sh manually if
# they want the change to persist.
#
# Once accepted, the choice is remembered per parent shell session via
# a marker file in /tmp tagged with $PPID and the target tree. As long
# as the user's interactive shell stays alive, subsequent invocations
# from the same shell skip the prompt and silently re-source. New
# shells (different PPID) re-ask. The chpwd hook installed by env.sh
# is still the better path — it switches the parent shell's env for
# real — but the marker keeps the UX clean when a user hasn't sourced
# env.sh in their interactive shell yet.

_ib_script_path="$(command -v -- "$0")"
_ib_script_dir="$(cd "$(dirname "$_ib_script_path")" && pwd)"
_ib_script_tree="$(cd "$_ib_script_dir/.." && pwd)"

_ib_cwd_tree="$(readlink -f "$PWD" 2>/dev/null || pwd)"
while [ "$_ib_cwd_tree" != "/" ] && [ ! -f "$_ib_cwd_tree/env.sh" ]; do
	_ib_cwd_tree="$(dirname "$_ib_cwd_tree")"
done
[ -f "$_ib_cwd_tree/env.sh" ] || _ib_cwd_tree=""

if [ -n "$_ib_cwd_tree" ]; then
	_ib_target="$_ib_cwd_tree"
else
	_ib_target="$_ib_script_tree"
fi

_ib_env_tree=""
if [ -n "$IB_ROOT_DIR" ]; then
	_ib_env_tree="$(realpath "$IB_ROOT_DIR" 2>/dev/null || echo "$IB_ROOT_DIR")"
fi

_ib_marker="/tmp/.infrabase-accept.$PPID.$(printf '%s' "$_ib_target" | sed 's|/|_|g' | sed 's|_|__SLASH__|g')"

if [ -n "$_ib_env_tree" ] && [ "$_ib_env_tree" = "$_ib_target" ]; then
	# Env already wired to the target tree; normalize cwd silently.
	cd "$_ib_target"
elif [ -f "$_ib_marker" ]; then
	# This shell session already accepted the switch — silent re-source.
	cd "$_ib_target"
	. ./env.sh
else
	echo "" >&2
	echo "[infrabase] env switch required for this invocation:" >&2
	echo "    target tree : $_ib_target" >&2
	if [ -n "$IB_ROOT_DIR" ]; then
		echo "    current env : $IB_ROOT_DIR" >&2
	else
		echo "    current env : <unset — env.sh not yet sourced>" >&2
	fi
	if [ "$_ib_script_tree" != "$_ib_target" ]; then
		echo "    script path : $_ib_script_path" >&2
		echo "                  (resolved via PATH from a sibling tree)" >&2
	fi
	echo "" >&2
	echo "    env.sh will be re-sourced from the target tree for this" >&2
	echo "    invocation only. To make the switch permanent in your" >&2
	echo "    interactive shell, run:" >&2
	echo "" >&2
	echo "        cd $_ib_target && . ./env.sh" >&2
	echo "" >&2
	if [ -t 0 ]; then
		printf "Continue with env.sh from %s? [y/N] " "$_ib_target" >&2
		read -r _ib_answer
		case "$_ib_answer" in
			y|Y|yes|YES) ;;
			*)
				echo "Aborted by user." >&2
				exit 1
				;;
		esac
	else
		echo "ERROR: refusing to reconfigure env in non-interactive mode." >&2
		echo "       Source env.sh from $_ib_target first," >&2
		echo "       or run the script interactively to confirm." >&2
		exit 1
	fi
	# Remember the accept for this shell session so the next invocation
	# from the same parent shell is silent.
	: > "$_ib_marker" 2>/dev/null || true
	cd "$_ib_target"
	. ./env.sh
	echo "[infrabase] env switched to $_ib_target for this run" >&2
fi

unset _ib_script_path _ib_script_dir _ib_script_tree \
      _ib_cwd_tree _ib_env_tree _ib_target _ib_marker _ib_answer
