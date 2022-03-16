# How to build
gcc app.c -fPIC -shared -o app.so  
gcc mini_loader.c dlsym.c -dl -o mini_loader

# How to run
./mini_loader