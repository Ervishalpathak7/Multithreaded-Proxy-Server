all:
    g++ -std=c++17 Server.cpp ThreadPool.cpp -o proxy_server

run:
    ./proxy_server
