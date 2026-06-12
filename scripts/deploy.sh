#!/bin/sh

# General deployment script for the infrabase infrastructure.

# Copyright (c) 2014-2023 REDS Institute, HEIG-VD
# Copyright (c) 2023-2025 EDGEMTech

progname=$(basename $0)

pr_usage()
{
	printf "Infrabase deployment script\n\n"
	printf "Usage: $progname [-h] [-l] [-a|-b|-r|-x] [-v] ... \n"
}

pr_help()
{
	printf "\nAvailable options:\n"
	printf "    -h                        Print this help\n"
	printf "    -l                        List available recipes per layer or globally\n"
	printf "    -a <bsp_recipe_name>      Deploy all, kernel, uboot, rootfs, usr\n"
	printf "    -b                        Deploy uboot\n"
	printf "    -r <rootfs_recipe_name>   Deploy specified rootfs\n"
	printf "    -x <aux_recipe_name>      Deploy auxiliary component\n"
	printf "    -v                        Emit logs during deployment\n"
	printf "Examples: \n\n"
	printf "$progname -l -a               List all deployable BSP recipes\n"
	printf "$progname -l -x               List all deployable auxiliary components\n"
	printf "$progname -b -v               Deploy uboot with verbose logs\n"
	printf "$progname -a bsp-linux -v     Deploy ALL rootfs, kernel, uboot with verbose logs\n"
}

if test $# -eq 0
then
	pr_usage
	printf "\nUse $progname -h to show help\n"
	exit 1
fi

. ./env.sh
. ./scripts/common/bblayers.sh

recipename=''
layernames=''
optverbose=0
dolist=0
dodeploy=0

while test $# -gt 0
do
	case "$1" in
		-h)
			# Help summary
			pr_usage
			pr_help
			exit
			;;
		-a)
			if ! test -n "$2"
			then
				layernames="meta-bsp"
			else
				recipename="$2"
				dodeploy=1
			fi
			;;
		-x)
			if ! test -n "$2"
			then
				layernames="$IB_AUX_LAYERS"
			else
				recipename="$2"
				dodeploy=1
			fi
			;;
		-r)
			if ! test -n "$2"
			then
				layernames="meta-rootfs"
			else
				recipename="$2"
				dodeploy=1
			fi
			;;
		-b)
			# We only currently have uboot for everything
			# So no optarg for -b
			layernames="meta-uboot"
			recipename="uboot"
			dodeploy=1
			;;
		-l)
			dolist=1
			;;
		-v)
			optverbose=1
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
	printf "Error: please specify a recipe name\n\n"
	pr_usage
	exit 1
fi

if test $dolist -eq 1
then
	if test -z "$layernames"
	then
		echo "Listing ALL available deployable recipes:"
	else
		echo "Listing available deployable recipes in layer(s): $layernames"
	fi

	recipes=$(available_recipes "$layernames")

	for r in $recipes
	do
		bitbake -c listtasks "$r" | grep "do_deploy" >/dev/null 2>&1
		if test $? -eq 0
		then
			# Recipe has a deployment task - list it
			echo $r
		fi
	done
	exit
fi

IB_BB_OPTS=''

if test $optverbose -eq 1
then
	IB_BB_OPTS='-vDDD'
fi

if test $dodeploy -eq 1
then
	printf "\n*** NOTE: *** Deployment requires root access, to be able to mount/umount\n"
	printf "and access loop devices, you may be prompted for the password\n\n"

	# NOTE: Currently these variables need to be specified in /etc/sudoers
	# and it is not clear why at this stage - this is not the case with
	# the user created during installation of the system - deeper investigations needed..
	preservedvars='IB_TOOLCHAIN_PATH,IB_UNPRIVILEDGED_USER_ID,IB_UNPRIVILEDGED_GROUP_ID'

	deploycmd=". $(pwd)/env.sh; bitbake $recipename -c do_deploy ${IB_BB_OPTS}"
	sudo --preserve-env=$preservedvars sh -c "$deploycmd; echo \$? > /tmp/ib-deploy-res;"
	res=$(cat /tmp/ib-deploy-res)

	if test $res -ne 0
	then
		# This is required to properly fail the CI
		# otherwise 0 is returned and job continues
		exit 1
	fi
fi
