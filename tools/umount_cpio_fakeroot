#!/bin/bash

truncate -s 0 $1
find . | cpio -o --format='newc' >> $1
