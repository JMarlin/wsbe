#!/bin/sh

emcc -c -o listnode.bc listnode.c 
emcc -c -o list.bc list.c
emcc -c -o context.bc context.c 
emcc -c -o window.bc window.c 
emcc -c -o desktop.bc desktop.c 
emcc -c -o entry.bc entry.c 
emcc -c -o rect.bc rect.c
emcc -c -o button.bc button.c
emcc -c -o textbox.bc textbox.c
emcc -c -o calculator.bc calculator.c
emcc -c -o ../fake_lib/fake_os.bc ../fake_lib/fake_os.c
emcc -o ../current_build.js ../fake_lib/fake_os.bc listnode.bc textbox.bc calculator.bc button.bc list.bc context.bc window.bc desktop.bc entry.bc rect.bc -s NO_EXIT_RUNTIME=1
