PORT=5005

# BUILD FILES
make

# CLOSE TESTING PROGRAMS
pkill server

# TESTING PART

# Starts the server
echo "Starting Server"
cd build/Server
rm -f Twice.jpg
./server $PORT &
cd ../..
sleep .5

# Starts the client
echo "Starting Client"
build/Client/deliver 127.0.0.1 $PORT << 'EOF'
ftp Twice.jpg
EOF