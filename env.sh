#!/bin/sh

# Strip the previous tree's contributions from PATH before adding new
# ones. Without this, `cd ../sibling && . ./env.sh` would leave both
# trees' scripts/ entries on PATH, and the older entry could win for
# `which build.sh`.
if [ -n "$IB_ROOT_DIR" ] && [ "$IB_ROOT_DIR" != "$PWD" ]; then
	_escaped_root=$(printf '%s' "$IB_ROOT_DIR" | sed 's|[][\.*^$()+?{}\|]|\\&|g')
	PATH=$(printf '%s:' "$PATH" | \
		sed -e "s|$_escaped_root/scripts:||g" \
		    -e "s|$_escaped_root/build/bitbake/bin:||g" \
		    -e 's/:$//')
	unset _escaped_root
fi

export IB_ROOT_DIR=$PWD
export BUILDDIR="$PWD/build"
export BBPATH=$BUILDDIR
# Prepend so the active tree's scripts always win, even if leftover
# entries from another tree are still on PATH.
PATH="$PWD/scripts:$BUILDDIR/bitbake/bin:$PATH"

# Specify auxiliary layers for the -x component opt
export IB_AUX_LAYERS="meta-usr meta-torizon meta-so3 meta-qemu meta-atf"

# TODO: It would be nice to test for the presence of such and
# such compiler in meta-bsp
if ! test -z $IB_TOOLCHAIN_PATH
then
	PATH="$PATH:$IB_TOOLCHAIN_PATH"
fi

# Variables passed through to bitbake task scripts. bitbake now runs
# unprivileged so the IB_UNPRIVILEDGED_USER_ID/GROUP_ID variables that
# used to be needed to chown root-written files back are gone; only
# build inputs remain.
if test -z "$BB_ENV_PASSTHROUGH_ADDITIONS"
then
	BB_ENV_PASSTHROUGH_ADDITIONS='IB_TOOLCHAIN_PATH IB_ROOT_DIR'
	export BB_ENV_PASSTHROUGH_ADDITIONS
fi

export PATH

# Auto-switch the build env when cwd moves into a sibling infrabase
# tree. The hook runs in the user's interactive shell (bash via
# PROMPT_COMMAND, zsh via chpwd_functions) so the switch is a real
# parent-shell change, not a subshell-local one. The function walks
# up from $PWD looking for env.sh; when it finds one belonging to a
# different tree than IB_ROOT_DIR, it prompts the user and (on y)
# re-sources that env.sh in-place. Cwd is preserved across the switch.
#
# A declined target is remembered in _ib_declined_tree so the user
# isn't re-prompted for the same tree on every subsequent prompt
# cycle (bash's PROMPT_COMMAND fires per-prompt, not per-cd). Cd'ing
# into a different tree resets the prompt.
_ib_chpwd_hook() {
	# Per-session opt-out — `export IB_NO_AUTO_SWITCH=1` to silence the
	# prompt without unwrapping cd/pushd/popd; `unset IB_NO_AUTO_SWITCH`
	# to re-enable.
	[ "${IB_NO_AUTO_SWITCH:-}" = "1" ] && return 0
	_t="$PWD"
	while [ "$_t" != "/" ] && [ ! -f "$_t/env.sh" ]; do
		_t=$(dirname "$_t")
	done
	if [ ! -f "$_t/env.sh" ]; then
		unset _t
		return 0
	fi
	_t=$(builtin cd "$_t" && pwd)
	if [ "$_t" = "$IB_ROOT_DIR" ] || [ "$_t" = "${_ib_declined_tree:-}" ]; then
		unset _t
		return 0
	fi

	printf '\n[infrabase] env switch requested:\n' >&2
	printf '    current : %s\n' "${IB_ROOT_DIR:-<unset>}" >&2
	printf '    target  : %s\n\n' "$_t" >&2
	printf 'Switch env to %s? [y/N] ' "$_t" >&2
	# The hook fires from the interactive shell prompt, so plain
	# `read` reads from the user's terminal (stdin == tty).
	read -r _ib_answer
	case "$_ib_answer" in
		y|Y|yes|YES)
			_ib_orig_pwd="$PWD"
			builtin cd "$_t" && . ./env.sh
			builtin cd "$_ib_orig_pwd" 2>/dev/null
			unset _ib_orig_pwd _ib_declined_tree
			;;
		*)
			printf '[infrabase] declined — staying on %s\n' \
				"${IB_ROOT_DIR:-<unset>}" >&2
			_ib_declined_tree="$_t"
			;;
	esac
	unset _t _ib_answer
}

# Wrap cd / pushd / popd so the env-switch check fires after every
# directory change. This is more robust than installing the hook via
# PROMPT_COMMAND (bash) or chpwd_functions (zsh): some shell configs
# rewrite PROMPT_COMMAND aggressively (e.g. edkrepo's
# `edkrepo_combo_chpwd` strips entries it didn't add), and a function
# override is immune. `builtin cd` inside the hook avoids recursion
# when the hook itself cds during the re-source.
cd() {
	builtin cd "$@" && _ib_chpwd_hook
}

pushd() {
	builtin pushd "$@" && _ib_chpwd_hook
}

popd() {
	builtin popd "$@" && _ib_chpwd_hook
}

# Quick toggles for the auto-switch prompt. These are shell functions
# (not scripts) so they run in the current shell and can flip the env
# var that the hook reads. A standalone executable would only flip it
# inside its own subshell.
ib_autoswitch_off() {
	export IB_NO_AUTO_SWITCH=1
	echo "[infrabase] auto-switch OFF (cd between trees stays silent)" >&2
}

ib_autoswitch_on() {
	unset IB_NO_AUTO_SWITCH
	echo "[infrabase] auto-switch ON" >&2
}

ib_autoswitch_status() {
	if [ "${IB_NO_AUTO_SWITCH:-}" = "1" ]; then
		echo "auto-switch: OFF"
	else
		echo "auto-switch: ON"
	fi
}
