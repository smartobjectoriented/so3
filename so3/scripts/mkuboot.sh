#!/bin/sh
#


BOARD=$2
SRC_DIR=$1
DEST_DIR=$PWD

mkimage -f $SRC_DIR/$BOARD.its $DEST_DIR/$BOARD.itb
