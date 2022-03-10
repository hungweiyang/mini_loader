gcc app.c -fPIC -shared -o app.so
gcc mini_loader.c -o mini_loader
./mini_loader