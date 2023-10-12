cd ..
diff -x "config" -x "scripts" -x "*.o" -x ".*" \
    -x "so3_ci.patch" -x "generated" -x "*.s" -x "sdcard*" -Naru ../base/so3 . > so3_ci.patch

