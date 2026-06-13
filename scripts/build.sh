#!/bin/sh

# General build script for the infrabase infrastructure.

# Copyright (c) 2014-2023 REDS Institute, HEIG-VD
# Copyright (c) 2023-2025 EDGEMTech

progname=$(basename $0)

pr_usage()
{
	printf "Infrabase build script\n\n"
	printf "Usage: $progname [-h] [-l] [-a|-b|-x|-k|-f|-r] <recipe_name> [-c][-v]\n"
}

pr_help()
{

	printf "\nAvailable options:\n"
	printf "    -h                           Print this help\n"
	printf "    -l                           List available BSPs, kernels, components\n"
	printf "    -a <bsp_recipe_name>         Build all, the name of BSP is required\n"
	printf "    -k <kernel_recipe_name>      Build kernel only\n"
	printf "    -x <component_recipe_name>   Build component or tool\n"
	printf "    -r <rootfs_recipe_name>      Build rootfs\n"
	printf "    -f                           Create and format filesystem image\n"
	printf "    -b                           Build uboot only\n"
	printf "    -v                           Emit verbose build logs\n"
	printf "    -c                           Clean before rebuilding\n\n"
	printf "Examples: \n\n"
	printf "$progname -l                     Print all recipes\n"
	printf "$progname -l -a                  Print all BSP recipes\n"
	printf "$progname -l -k                  Print all kernel recipes\n"
	printf "$progname -v -a bsp-linux -c     Clean and rebuild all emitting verbose logs\n"
}

if test $# -eq 0
then
	pr_usage
	printf "\nUse $progname -h to show help\n"
	exit 1
fi

. ./env.sh
. ./scripts/common/bblayers.sh

layernames=''
recipename=''
dolist=0
dobuild=0
doclean=0
optverbose=0
rootprivs=0

while test $# -gt 0
do
	case "$1" in
		-l)
			dolist=1
			;;
		-h)
			# Help summary
			pr_usage
			pr_help
			exit
			;;
		-a)
			if ! test -n "$2"
			then
				# List all recipes in 'meta-bsp'
				layernames="meta-bsp"
			else
				recipename="$2"
				dobuild=1
			fi
			;;
		-r)
			if ! test -n "$2"
			then
				layernames="meta-rootfs"
			else
				recipename="$2"
				dobuild=1
			fi
			;;
		-b)
			layernames="meta-uboot"
			recipename="uboot"
			dobuild=1
			;;
		-x)
			if test -n "$2"
			then
				recipename="$2"
				dobuild=1
			else
				layernames="$IB_AUX_LAYERS"
			fi
			;;
		-c)
			doclean=1
			;;
		-v)
			optverbose=1
			;;
		-k)
			if ! test -n "$2"
			then
				layernames="meta-linux meta-so3"
			else
				recipename="$2"
				dobuild=1
			fi
			;;
		-f)
			recipename="filesystem"
			rootprivs=1
			;;
		-*)
			printf "Error: unknown option: $1\n"
			pr_usage
			exit 1
			;;
	esac
	shift
done

show_platform

if test -z $recipename && test $dolist -eq 0
then
	printf "Error: Please specify recipe name\n\n"
	pr_usage
	exit 1
fi

# The user is willing to list available recipes
# dolist action is available for component options
if test $dolist -eq 1
then
	if test -z "$layernames"
	then
		printf "Listing ALL available recipes:\n"
	else
		printf "Listing recipes in layer(s): $layernames\n"
	fi

	available_recipes "$layernames"
	exit
fi

IB_BB_OPTS=''

if test $optverbose -eq 1
then
	IB_BB_OPTS='-vDDD'
fi

if test $doclean -eq 1
then
	bitbake $recipename -c clean $IB_BB_OPTS
fi

if test $dobuild -eq 1
then
	if test $rootprivs -eq 1
	then
		printf "\n *** NOTE: *** '$recipename' requires root access\n"
		printf "you may be prompted for the password\n\n"

		preservedvars='IB_TOOLCHAIN_PATH,IB_UNPRIVILEDGED_USER_ID,IB_UNPRIVILEDGED_GROUP_ID'
		sudo --preserve-env=$preservedvars sh -c ". $(pwd)/env.sh; bitbake $recipename ${IB_BB_OPTS}"
	else
		bitbake $recipename $IB_BB_OPTS
	fi
fi
