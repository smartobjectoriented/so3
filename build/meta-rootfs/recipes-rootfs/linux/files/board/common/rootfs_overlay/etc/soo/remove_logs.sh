#!/bin/sh

# Remove the README in Capsules image folder
rm -f /mnt/capsules/image/README

# Remove the snapshots
rm -f /mnt/capsules/snapshot/*

# Remove SOO log files
rm -f /var/log/soo/*
