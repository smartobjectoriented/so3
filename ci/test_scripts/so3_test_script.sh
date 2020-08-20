#!/bin/bash
script=${BASH_SOURCE[0]}
# Get the path of this script
SCRIPTPATH=$(realpath $(dirname $script))

read -r -d '' expect_script <<EOF
    set timeout 10
    proc close_qemu {} {
         send "\x01"
         send "x"
    }
    
    spawn ./st2
    sleep 1
    expect {
         -re ".*so3%.*" { puts "prompt found !\n"; close_qemu; exit 0 }
         timeout { puts "timeout !\n"; exit 1 }
    }
EOF

read -r -d '' expect_script_test <<EOS
set timeout 10
proc close_qemu {} {
     send "\x01"
     send "x"
}

spawn ./st2
sleep 1
expect {
     -re ".*so3%.*" { puts "\nprompt found !\n"; send $1; send "\n" }
     timeout { puts "\ntimeout !\n"; exit 1 }
}
expect {
     -re ".*SO3.*PASS.*\n" { }
     -re ".*SO3.*FAIL.*\n" { close_qemu; exit 2 }
     -re ".*exec failed.*" { puts "\nExec of $1 failed !\n"; close_qemu; exit 3}
     timeout { puts "\ntimeout !\n"; exit 1 }
}
expect {
     -re ".*so3%.*" { close_qemu; exit 0 }
     timeout { puts "\ntimeout !\n"; exit 1 }
}
EOS

cd ${SCRIPTPATH}/../..
if [ "$#" -eq 0 ]; then
    echo "Testing SO3 launch"
    expect -c "$expect_script"
    exit $?
else
    if [ "$#" -gt 1 ]; then
        echo "Illegal number of parameters"
        exit 1
    fi
    
    expect -c "$expect_script_test"
fi
