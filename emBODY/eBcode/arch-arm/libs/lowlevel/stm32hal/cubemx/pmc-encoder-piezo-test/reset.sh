#!/usr/bin/expect
set timeout 30
spawn telnet
expect "telnet>"
send "open localhost 4444\n"
expect ">"
send "reset \n"
