# SPDX-License-Identifier: GPL-2.0
#
# (c) 2010-2018 Joe Perches <joe@perches.com>
use Encode qw(decode encode);
my $max_line_length = 100;
my $u_boot = 0;
my $allow_c99_comments = 1; # Can be overridden by --ignore C99_COMMENT_TOLERANCE
# git output parsing needs US English output, so first set backtick child process LANGUAGE
my $git_command ='export LANGUAGE=en_US.UTF-8; git';
my $tabsize = 8;
  --max-line-length=n        set the maximum line length, (default $max_line_length)
                             if exceeded, warn on patches
                             requires --strict for use with --file
  --tab-size=n               set the number of spaces for tab (default $tabsize)
  --u-boot                   Run additional checks for U-Boot
	'tab-size=i'	=> \$tabsize,
	'u-boot'	=> \$u_boot,
my $perl_version_ok = 1;
	$perl_version_ok = 0;
	exit(1) if (!$ignore_perl_version);
# skip TAB size 1 to avoid additional checks on $tabsize - 1
die "Invalid TAB size: $tabsize\n" if ($tabsize < 2);

			__refconst|
			__refdata|
			__ro_after_init|
	debug|
	printf|
our $allocFunctions = qr{(?x:
	(?:(?:devm_)?
		(?:kv|k|v)[czm]alloc(?:_node|_array)? |
		kstrdup(?:_const)? |
		kmemdup(?:_nul)?) |
	(?:\w+)?alloc_skb(?:_ip_align)? |
				# dev_alloc_skb/netdev_alloc_skb, et al
	dma_alloc_coherent
)};

	Co-developed-by:|
our %deprecated_apis = (
	"synchronize_rcu_bh"			=> "synchronize_rcu",
	"synchronize_rcu_bh_expedited"		=> "synchronize_rcu_expedited",
	"call_rcu_bh"				=> "call_rcu",
	"rcu_barrier_bh"			=> "rcu_barrier",
	"synchronize_sched"			=> "synchronize_rcu",
	"synchronize_sched_expedited"		=> "synchronize_rcu_expedited",
	"call_rcu_sched"			=> "call_rcu",
	"rcu_barrier_sched"			=> "rcu_barrier",
	"get_state_synchronize_sched"		=> "get_state_synchronize_rcu",
	"cond_synchronize_sched"		=> "cond_synchronize_rcu",
);

#Create a search pattern for all these strings to speed up a loop below
our $deprecated_apis_search = "";
foreach my $entry (keys %deprecated_apis) {
	$deprecated_apis_search .= '|' if ($deprecated_apis_search ne "");
	$deprecated_apis_search .= $entry;
}
$deprecated_apis_search = "(?:${deprecated_apis_search})";

			(?:(?:\s|\*|\[\])+\s*const|(?:\s|\*\s*(?:const\s*)?|\[\])+|(?:\s*\[\s*\])+){0,4}
			(?:(?:\s|\*|\[\])+\s*const|(?:\s|\*\s*(?:const\s*)?|\[\])+|(?:\s*\[\s*\])+){0,4}
our %maintained_status = ();

	if (!exists($maintained_status{$filename})) {
		$maintained_status{$filename} = `perl $root/scripts/get_maintainer.pl --status --nom --nol --nogit --nogit-fallback -f $filename 2>&1`;
	}

	return $maintained_status{$filename} =~ /obsolete/i;
}
sub is_SPDX_License_valid {
	my ($license) = @_;

	return 1 if (!$tree || which("python") eq "" || !(-e "$root/scripts/spdxcheck.py") || !(-e "$root/.git"));

	my $root_path = abs_path($root);
	my $status = `cd "$root_path"; echo "$license" | python scripts/spdxcheck.py -`;
	return 0 if ($status ne "");
	return 1;
		my $git_last_include_commit = `${git_command} log --no-merges --pretty=format:"%h%n" -1 -- include`;
		$files = `${git_command} ls-files "include/*.h"`;
	my $output = `${git_command} log --no-color --format='%H %s' -1 $commit 2>&1`;
	if ($lines[0] =~ /^error: short SHA1 $commit is ambiguous/) {
		my $lines = `${git_command} log --no-color --no-merges --pretty=format:'%H %s' $git_range`;
$allow_c99_comments = !defined $ignore_type{"C99_COMMENT_TOLERANCE"};
	if (!$perl_version_ok) {
      An upgrade to at least perl $minimum_perl_version is suggested.
	my $name_comment = "";
	$name =~ s/(\s*\([^\)]+\))\s*//;
	if (defined($1)) {
		$name_comment = trim($1);
	}
	return ($name, $name_comment, $address, $comment);
sub reformat_email {
	my ($email) = @_;

	my ($email_name, $name_comment, $email_address, $comment) = parse_email($email);
	return format_email($email_name, $email_address);
}

sub same_email_addresses {
	my ($email1, $email2) = @_;

	my ($email1_name, $name1_comment, $email1_address, $comment1) = parse_email($email1);
	my ($email2_name, $name2_comment, $email2_address, $comment2) = parse_email($email2);

	return $email1_name eq $email2_name &&
	       $email1_address eq $email2_address;
}

			for (; ($n % $tabsize) != 0; $n++) {
	my $source_indent = $tabsize;
sub get_raw_comment {
	my ($line, $rawline) = @_;
	my $comment = '';

	for my $i (0 .. (length($line) - 1)) {
		if (substr($line, $i, 1) eq "$;") {
			$comment .= substr($rawline, $i, 1);
		}
	}

	return $comment;
}

# Args:
#   line: Patch line to check
#   auto: Auto variable name, e.g. "per_child_auto"
#   suffix: Suffix to expect on member, e.g. "_priv"
#   warning: Warning name, e.g. "PRIV_AUTO"
sub u_boot_struct_name {
	my ($line, $auto, $suffix, $warning, $herecurr) = @_;

	# Use _priv as a suffix for the device-private data struct
	if ($line =~ /^\+\s*\.${auto}\s*=\s*sizeof\(struct\((\w+)\).*/) {
		my $struct_name = $1;
		if ($struct_name !~ /^\w+${suffix}/) {
			WARN($warning,
				 "struct \'$struct_name\' should have a ${suffix} suffix\n"
				 . $herecurr);
		}
	}
}

# Checks specific to U-Boot
sub u_boot_line {
	my ($realfile, $line, $rawline, $herecurr) = @_;

	# ask for a test if a new uclass ID is added
	if ($realfile =~ /uclass-id.h/ && $line =~ /^\+/) {
		WARN("NEW_UCLASS",
		     "Possible new uclass - make sure to add a sandbox driver, plus a test in test/dm/<name>.c\n" . $herecurr);
	}

	# try to get people to use the livetree API
	if ($line =~ /^\+.*fdtdec_/) {
		WARN("LIVETREE",
		     "Use the livetree API (dev_read_...)\n" . $herecurr);
	}

	# add tests for new commands
	if ($line =~ /^\+.*do_($Ident)\(struct cmd_tbl.*/) {
		WARN("CMD_TEST",
		     "Possible new command - make sure you add a test\n" . $herecurr);
	}

	# use if instead of #if
	if ($realfile =~ /\.c$/ && $line =~ /^\+#if.*CONFIG.*/) {
		WARN("PREFER_IF",
		     "Use 'if (IS_ENABLED(CONFIG...))' instead of '#if or #ifdef' where possible\n" . $herecurr);
	}

	# prefer strl(cpy|cat) over strn(cpy|cat)
	if ($line =~ /\bstrn(cpy|cat)\s*\(/) {
		WARN("STRL",
		     "strl$1 is preferred over strn$1 because it always produces a nul-terminated string\n" . $herecurr);
	}

	# use defconfig to manage CONFIG_CMD options
	if ($line =~ /\+\s*#\s*(define|undef)\s+(CONFIG_CMD\w*)\b/) {
		ERROR("DEFINE_CONFIG_CMD",
		      "All commands are managed by Kconfig\n" . $herecurr);
	}

	# Don't put common.h and dm.h in header files
	if ($realfile =~ /\.h$/ && $rawline =~ /^\+#include\s*<(common|dm)\.h>*/) {
		ERROR("BARRED_INCLUDE_IN_HDR",
		      "Avoid including common.h and dm.h in header files\n" . $herecurr);
	}

	# Do not disable fdt / initrd relocation
	if ($rawline =~ /.*(fdt|initrd)_high=0xffffffff/) {
		ERROR("DISABLE_FDT_OR_INITRD_RELOC",
		     "fdt or initrd relocation disabled at boot time\n" . $herecurr);
	}

	# make sure 'skip_board_fixup' is not
	if ($rawline =~ /.*skip_board_fixup.*/) {
		ERROR("SKIP_BOARD_FIXUP",
		     "Avoid setting skip_board_fixup env variable\n" . $herecurr);
	}

	# Do not use CONFIG_ prefix in CONFIG_IS_ENABLED() calls
	if ($line =~ /^\+.*CONFIG_IS_ENABLED\(CONFIG_\w*\).*/) {
		ERROR("CONFIG_IS_ENABLED_CONFIG",
		      "CONFIG_IS_ENABLED() takes values without the CONFIG_ prefix\n" . $herecurr);
	}

	# Use _priv as a suffix for the device-private data struct
	if ($line =~ /^\+\s*\.priv_auto\s*=\s*sizeof\(struct\((\w+)\).*/) {
		my $struct_name = $1;
		if ($struct_name !~ /^\w+_priv/) {
			WARN("PRIV_AUTO", "struct \'$struct_name\' should have a _priv suffix");
		}
	}

	# Check struct names for the 'auto' members of struct driver
	u_boot_struct_name($line, "priv_auto", "_priv", "PRIV_AUTO", $herecurr);
	u_boot_struct_name($line, "plat_auto", "_plat", "PLAT_AUTO", $herecurr);
	u_boot_struct_name($line, "per_child_auto", "_priv", "CHILD_PRIV_AUTO", $herecurr);
	u_boot_struct_name($line, "per_child_plat_auto", "_plat",
		"CHILD_PLAT_AUTO", $herecurr);

	# Now the ones for struct uclass, skipping those in common with above
	u_boot_struct_name($line, "per_device_auto", "_priv",
		"DEVICE_PRIV_AUTO", $herecurr);
	u_boot_struct_name($line, "per_device_plat_auto", "_plat",
		"DEVICE_PLAT_AUTO", $herecurr);
}

	my $author = '';
	my $authorsignoff = 0;
	my $is_binding_patch = -1;
	my $has_patch_separator = 0;	#Found a --- line
	my $commit_log_lines = 0;	#Number of commit log lines
		my $raw_comment = get_raw_comment($line, $rawline);

# check if it's a mode change, rename or start of a patch
		if (!$in_commit_log &&
		    ($line =~ /^ mode change [0-7]+ => [0-7]+ \S+\s*$/ ||
		    ($line =~ /^rename (?:from|to) \S+\s*$/ ||
		     $line =~ /^diff --git a\/[\w\/\.\_\-]+ b\/\S+\s*$/))) {
			$is_patch = 1;
		}

			if ($realfile !~ /^MAINTAINERS/) {
				my $last_binding_patch = $is_binding_patch;

				$is_binding_patch = () = $realfile =~ m@^(?:Documentation/devicetree/|include/dt-bindings/)@;

				if (($last_binding_patch != -1) &&
				    ($last_binding_patch ^ $is_binding_patch)) {
					WARN("DT_SPLIT_BINDING_PATCH",
					     "DT binding docs and includes should be a separate patch. See: Documentation/devicetree/bindings/submitting-patches.txt\n");
				}
			}

# Verify the existence of a commit log if appropriate
# 2 is used because a $signature is counted in $commit_log_lines
		if ($in_commit_log) {
			if ($line !~ /^\s*$/) {
				$commit_log_lines++;	#could be a $signature
			}
		} elsif ($has_commit_log && $commit_log_lines < 2) {
			WARN("COMMIT_MESSAGE",
			     "Missing commit description - Add an appropriate one\n");
			$commit_log_lines = 2;	#warn only once
		}

# Check the patch for a From:
		if (decode("MIME-Header", $line) =~ /^From:\s*(.*)/) {
			$author = $1;
			$author = encode("utf8", $author) if ($line =~ /=\?utf-8\?/i);
			$author =~ s/"//g;
			$author = reformat_email($author);
		}

		if ($line =~ /^\s*signed-off-by:\s*(.*)/i) {
			if ($author ne '') {
				if (same_email_addresses($1, $author)) {
					$authorsignoff = 1;
				}
			}
		}

# Check for patch separator
		if ($line =~ /^---$/) {
			$has_patch_separator = 1;
			$in_commit_log = 0;
			my ($email_name, $name_comment, $email_address, $comment) = parse_email($email);
				if (!same_email_addresses($email, $suggested_email)) {

# Check Co-developed-by: immediately followed by Signed-off-by: with same name and email
			if ($sign_off =~ /^co-developed-by:$/i) {
				if ($email eq $author) {
					WARN("BAD_SIGN_OFF",
					      "Co-developed-by: should not be used to attribute nominal patch author '$author'\n" . "$here\n" . $rawline);
				}
				if (!defined $lines[$linenr]) {
					WARN("BAD_SIGN_OFF",
                                             "Co-developed-by: must be immediately followed by Signed-off-by:\n" . "$here\n" . $rawline);
				} elsif ($rawlines[$linenr] !~ /^\s*signed-off-by:\s*(.*)/i) {
					WARN("BAD_SIGN_OFF",
					     "Co-developed-by: must be immediately followed by Signed-off-by:\n" . "$here\n" . $rawline . "\n" .$rawlines[$linenr]);
				} elsif ($1 ne $email) {
					WARN("BAD_SIGN_OFF",
					     "Co-developed-by and Signed-off-by: name/email do not match \n" . "$here\n" . $rawline . "\n" .$rawlines[$linenr]);
				}
			}
# Check for Gerrit Change-Ids not in any patch context
		if ($realfile eq '' && !$has_patch_separator && $line =~ /^\s*change-id:/i) {
			      "Remove Gerrit Change-Id's before submitting upstream\n" . $herecurr);
		     $line =~ /^\s*\[\<[0-9a-fA-F]{8,}\>\]/) ||
		     $line =~ /^(?:\s+\w+:\s+[0-9a-fA-F]+){3,3}/ ||
		     $line =~ /^\s*\#\d+\s*\[[0-9a-fA-F]+\]\s*\w+ at [0-9a-fA-F]+/) {
					# stack dump address styles
		    $line !~ /^\s*(?:Link|Patchwork|http|https|BugLink|base-commit):/i &&
# Check for adding new DT bindings not in schema format
		if (!$in_commit_log &&
		    ($line =~ /^new file mode\s*\d+\s*$/) &&
		    ($realfile =~ m@^Documentation/devicetree/bindings/.*\.txt$@)) {
			WARN("DT_SCHEMA_BINDING_PATCH",
			     "DT bindings should be in DT schema format. See: Documentation/devicetree/writing-schema.rst\n");
		}

# check for invalid commit id
		if ($in_commit_log && $line =~ /(^fixes:|\bcommit)\s+([0-9a-f]{6,40})\b/i) {
			my $id;
			my $description;
			($id, $description) = git_commit_info($2, undef, undef);
			if (!defined($id)) {
				WARN("UNKNOWN_COMMIT_ID",
				     "Unknown commit id '$2', maybe rebased or not pulled?\n" . $herecurr);
			}
		}

			my $vp_file = $dt_path . "vendor-prefixes.yaml";
				`grep -Eq "\\"\\^\Q$vendor\E,\\.\\*\\":" $vp_file`;
				} elsif (($checklicenseline == 2) || $realfile =~ /\.(sh|pl|py|awk|tc|yaml)$/) {
# check SPDX comment style for .[chsS] files
				if ($realfile =~ /\.[chsS]$/ &&
				    $rawline =~ /SPDX-License-Identifier:/ &&
				    $rawline !~ m@^\+\s*\Q$comment\E\s*@) {
					WARN("SPDX_LICENSE_TAG",
					     "Improper SPDX comment style for '$realfile', please use '$comment' instead\n" . $herecurr);
				}

				    $rawline !~ m@^\+\Q$comment\E SPDX-License-Identifier: @) {
				} elsif ($rawline =~ /(SPDX-License-Identifier: .*)/) {
					my $spdx_license = $1;
					if (!is_SPDX_License_valid($spdx_license)) {
						WARN("SPDX_LICENSE_TAG",
						     "'$spdx_license' is not supported in LICENSES/...\n" . $herecurr);
					}
					if ($realfile =~ m@^Documentation/devicetree/bindings/@ &&
					    not $spdx_license =~ /GPL-2\.0.*BSD-2-Clause/) {
						my $msg_level = \&WARN;
						$msg_level = \&CHK if ($file);
						if (&{$msg_level}("SPDX_LICENSE_TAG",

								  "DT binding documents should be licensed (GPL-2.0-only OR BSD-2-Clause)\n" . $herecurr) &&
						    $fix) {
							$fixed[$fixlinenr] =~ s/SPDX-License-Identifier: .*/SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)/;
						}
					}
# check for using SPDX-License-Identifier on the wrong line number
		if ($realline != $checklicenseline &&
		    $rawline =~ /\bSPDX-License-Identifier:/ &&
		    substr($line, @-, @+ - @-) eq "$;" x (@+ - @-)) {
			WARN("SPDX_LICENSE_TAG",
			     "Misplaced SPDX-License-Identifier tag - use line $checklicenseline instead\n" . $herecurr);
		}

				my $msg_level = \&WARN;
				$msg_level = \&CHK if ($file);
				&{$msg_level}($msg_type,
					      "line length of $length exceeds $max_line_length columns\n" . $herecurr);
		if ($u_boot) {
			u_boot_line($realfile, $line, $rawline, $herecurr);
		}

# more than $tabsize must use tabs.
					   s/(^\+.*) {$tabsize,$tabsize}\t/$1\t\t/) {}
# check for assignments on the start of a line
		if ($sline =~ /^\+\s+($Assignment)[^=]/) {
			CHK("ASSIGNMENT_CONTINUATIONS",
			    "Assignment operator '$1' should be on the previous line\n" . $hereprev);
		}

		if ($perl_version_ok &&
			if ($indent % $tabsize) {
					$fixed[$fixlinenr] =~ s@(^\+\t+) +@$1 . "\t" x ($indent/$tabsize)@e;
		if ($perl_version_ok &&
					"\t" x ($pos / $tabsize) .
					" "  x ($pos % $tabsize);
		      $sline =~ /^\+\s+(?:static\s+)?(?:const\s+)?(?:union|struct|enum|typedef)\b/ ||
			    (($sindent % $tabsize) != 0 ||
			     ($sindent > $indent + $tabsize))) {
# check for unnecessary <signed> int declarations of short/long/long long
		while ($sline =~ m{\b($TypeMisordered(\s*\*)*|$C90_int_types)\b}g) {
			my $type = trim($1);
			next if ($type !~ /\bint\b/);
			next if ($type !~ /\b(?:short|long\s+long|long)\b/);
			my $new_type = $type;
			$new_type =~ s/\b\s*int\s*\b/ /;
			$new_type =~ s/\b\s*(?:un)?signed\b\s*/ /;
			$new_type =~ s/^const\s+//;
			$new_type = "unsigned $new_type" if ($type =~ /\bunsigned\b/);
			$new_type = "const $new_type" if ($type =~ /^const\b/);
			$new_type =~ s/\s+/ /g;
			$new_type = trim($new_type);
			if (WARN("UNNECESSARY_INT",
				 "Prefer '$new_type' over '$type' as the int is unnecessary\n" . $herecurr) &&
			    $fix) {
				$fixed[$fixlinenr] =~ s/\b\Q$type\E\b/$new_type/;
			}
		}

		}

# check for initialized const char arrays that should be static const
		if ($line =~ /^\+\s*const\s+(char|unsigned\s+char|_*u8|(?:[us]_)?int8_t)\s+\w+\s*\[\s*(?:\w+\s*)?\]\s*=\s*"/) {
			if (WARN("STATIC_CONST_CHAR_ARRAY",
				 "const array should probably be static const\n" . $herecurr) &&
			    $fix) {
				$fixed[$fixlinenr] =~ s/(^.\s*)const\b/${1}static const/;
			}
		}
		}
		if ($line =~ /(\b$Type\s*$Ident)\s*\(\s*\)/) {
		if ($perl_version_ok &&
			    $prefix !~ /[{,:]\s+$/) {
						$ok = 1;
		    $line =~ /\b(?:else|do)\{/) {
				$fixed[$fixlinenr] =~ s/^(\+.*(?:do|else|\)))\{/$1 {/;
		if ($line =~ /}(?!(?:,|;|\)|\}))\S/) {
		    $perl_version_ok && defined($stat) &&
			if ($perl_version_ok &&
		if ($perl_version_ok &&
		if ($perl_version_ok &&
			$s =~ s/$;//g;	# Remove any comments
			$s =~ s/$;//g;	# Remove any comments
#Ignore SI style variants like nS, mV and dB
#(ie: max_uV, regulator_min_uA_show, RANGE_mA_VALUE)
			    $var !~ /^(?:[a-z0-9_]*|[A-Z0-9_]*)?_?[a-z][A-Z](?:_[a-z0-9_]+|_[A-Z0-9_]+)?$/ &&
				$define_args =~ s/\\\+?//g;
			# Flatten any obvious string concatenation.
				$tmp_stmt =~ s/\b(sizeof|typeof|__typeof__|__builtin\w+|typecheck\s*\(\s*$Type\s*,|\#+)\s*\(*\s*$arg\s*\)*\b//g;
				my $use_cnt = () = $tmp_stmt =~ /\b$arg\b/g;
		if ($perl_version_ok &&
		if ($line =~ /$String[A-Za-z0-9_]/ || $line =~ /[A-Za-z0-9_]$String/) {
			if (CHK("CONCATENATED_STRING",
				"Concatenated strings should use spaces between elements\n" . $herecurr) &&
			    $fix) {
				while ($line =~ /($String)/g) {
					my $extracted_string = substr($rawline, $-[0], $+[0] - $-[0]);
					$fixed[$fixlinenr] =~ s/\Q$extracted_string\E([A-Za-z0-9_])/$extracted_string $1/;
					$fixed[$fixlinenr] =~ s/([A-Za-z0-9_])\Q$extracted_string\E/$1 $extracted_string/;
				}
			}
			if (WARN("STRING_FRAGMENTS",
				 "Consecutive strings are generally better as a single string\n" . $herecurr) &&
			    $fix) {
				while ($line =~ /($String)(?=\s*")/g) {
					my $extracted_string = substr($rawline, $-[0], $+[0] - $-[0]);
					$fixed[$fixlinenr] =~ s/\Q$extracted_string\E\s*"/substr($extracted_string, 0, -1)/e;
				}
			}
			WARN("IF_0",
			     "Consider removing the code enclosed by this #if 0 and its #endif\n" . $herecurr);
		}

# warn about #if 1
		if ($line =~ /^.\s*\#\s*if\s+1\b/) {
			WARN("IF_1",
			     "Consider removing the #if 1 and its #endif\n" . $herecurr);
			if ($s =~ /(?:^|\n)[ \+]\s*(?:$Type\s*)?\Q$testval\E\s*=\s*(?:\([^\)]*\)\s*)?\s*$allocFunctions\s*\(/ &&
			    $s !~ /\b__GFP_NOWARN\b/ ) {
		if ($perl_version_ok &&
		if ($perl_version_ok) {
				    "usleep_range is preferred over udelay; see Documentation/timers/timers-howto.rst\n" . $herecurr);
				     "msleep < 20ms can sleep for up to 20ms; see Documentation/timers/timers-howto.rst\n" . $herecurr);
# Check for __attribute__ section, prefer __section
		if ($realfile !~ m@\binclude/uapi/@ &&
		    $line =~ /\b__attribute__\s*\(\s*\(.*_*section_*\s*\(\s*("[^"]*")/) {
			my $old = substr($rawline, $-[1], $+[1] - $-[1]);
			my $new = substr($old, 1, -1);
			if (WARN("PREFER_SECTION",
				 "__section(\"$new\") is preferred over __attribute__((section($old)))\n" . $herecurr) &&
			    $fix) {
				$fixed[$fixlinenr] =~ s/\b__attribute__\s*\(\s*\(\s*_*section_*\s*\(\s*\Q$old\E\s*\)\s*\)\s*\)/__section($new)/;
			}
		}

		if ($perl_version_ok &&
		if ($perl_version_ok &&
				my $specifier;
				my $extension;
				my $qualifier;
				my $bad_specifier = "";
				while ($fmt =~ /(\%[\*\d\.]*p(\w)(\w*))/g) {
					$qualifier = $3;
					if ($extension !~ /[SsBKRraEehMmIiUDdgVCbGNOxtf]/ ||
					    ($extension eq "f" &&
					     defined $qualifier && $qualifier !~ /^w/)) {
		if ($perl_version_ok &&
#		if ($perl_version_ok &&
#		if ($perl_version_ok &&
#		if ($perl_version_ok &&
		if ($perl_version_ok &&
		if ($perl_version_ok &&
				     "usleep_range should not use min == max args; see Documentation/timers/timers-howto.rst\n" . "$here\n$stat\n");
				     "usleep_range args reversed, use min then max; see Documentation/timers/timers-howto.rst\n" . "$here\n$stat\n");
		if ($perl_version_ok &&
		if ($perl_version_ok &&
# while avoiding uninitialized_var(x)
		    $stat =~ /^.\s*(?:extern\s+)?$Type\s*(?:($Ident)|\(\s*\*\s*$Ident\s*\))\s*\(\s*([^{]+)\s*\)\s*;/s &&
		    (!defined($1) ||
		     (defined($1) && $1 ne "uninitialized_var")) &&
		     $2 ne "void") {
			my $args = trim($2);
				if ($arg =~ /^$Type$/ &&
					$arg !~ /enum\s+$Ident$/) {
		if ($perl_version_ok &&
# check for pointless casting of alloc functions
		if ($line =~ /\*\s*\)\s*$allocFunctions\b/) {
		if ($perl_version_ok &&
		    $line =~ /\b($Lval)\s*\=\s*(?:$balanced_parens)?\s*((?:kv|k|v)[mz]alloc(?:_node)?)\s*\(\s*(sizeof\s*\(\s*struct\s+$Lval\s*\))/) {
		if ($perl_version_ok &&
		if ($perl_version_ok &&
		    $line =~ /\b($Lval)\s*\=\s*(?:$balanced_parens)?\s*krealloc\s*\(\s*($Lval)\s*,/ &&
		    $1 eq $3) {
# check for /* fallthrough */ like comment, prefer fallthrough;
		my @fallthroughs = (
			'fallthrough',
			'@fallthrough@',
			'lint -fallthrough[ \t]*',
			'intentional(?:ly)?[ \t]*fall(?:(?:s | |-)[Tt]|t)hr(?:ough|u|ew)',
			'(?:else,?\s*)?FALL(?:S | |-)?THR(?:OUGH|U|EW)[ \t.!]*(?:-[^\n\r]*)?',
			'Fall(?:(?:s | |-)[Tt]|t)hr(?:ough|u|ew)[ \t.!]*(?:-[^\n\r]*)?',
			'fall(?:s | |-)?thr(?:ough|u|ew)[ \t.!]*(?:-[^\n\r]*)?',
		    );
		if ($raw_comment ne '') {
			foreach my $ft (@fallthroughs) {
				if ($raw_comment =~ /$ft/) {
					my $msg_level = \&WARN;
					$msg_level = \&CHK if ($file);
					&{$msg_level}("PREFER_FALLTHROUGH",
						      "Prefer 'fallthrough;' over fallthrough comment\n" . $herecurr);
					last;
				}
			}
		}

		if ($perl_version_ok &&
# check for spin_is_locked(), suggest lockdep instead
		if ($line =~ /\bspin_is_locked\(/) {
			WARN("USE_LOCKDEP",
			     "Where possible, use lockdep_assert_held instead of assertions based on spin_is_locked\n" . $herecurr);
		}

# check for deprecated apis
		if ($line =~ /\b($deprecated_apis_search)\b\s*\(/) {
			my $deprecated_api = $1;
			my $new_api = $deprecated_apis{$deprecated_api};
			WARN("DEPRECATED_API",
			     "Deprecated use of '$deprecated_api', prefer '$new_api' instead\n" . $herecurr);
		}

		if ($perl_version_ok &&
# nested likely/unlikely calls
		if ($line =~ /\b(?:(?:un)?likely)\s*\(\s*!?\s*(IS_ERR(?:_OR_NULL|_VALUE)?|WARN)/) {
			WARN("LIKELY_MISUSE",
			     "nested (un)?likely() calls, $1 already uses unlikely() internally\n" . $herecurr);
		}

		if ($perl_version_ok &&
		if ($perl_version_ok &&

# check for sysctl duplicate constants
		if ($line =~ /\.extra[12]\s*=\s*&(zero|one|int_max)\b/) {
			WARN("DUPLICATED_SYSCTL_CONST",
				"duplicated sysctl range checking value '$1', consider using the shared one in include/linux/sysctl.h\n" . $herecurr);
		}
	if ($is_patch && $has_commit_log && $chk_signoff) {
		if ($signoff == 0) {
			ERROR("MISSING_SIGN_OFF",
			      "Missing Signed-off-by: line(s)\n");
		} elsif (!$authorsignoff) {
			WARN("NO_AUTHOR_SIGN_OFF",
			     "Missing Signed-off-by: line by nominal patch author '$author'\n");
		}