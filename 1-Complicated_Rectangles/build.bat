emcc -c -o blog_code.bc blog_code.c &
emcc -c -o ..\fake_lib\fake_os.bc ..\fake_lib\fake_os.c &
emcc -o ..\current_build.js ..\fake_lib\fake_os.bc blog_code.bc &
