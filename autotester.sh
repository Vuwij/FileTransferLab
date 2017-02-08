PORT=5007
FILENAME=Twice.jpg

# BUILD FILES
make

# CLOSE TESTING PROGRAMS
pkill server

# Starts the server
echo "Starting Server"
cd build/Server
rm -f Twice.jpg
./server $PORT &
cd ../..
sleep .5

# Starts the client
echo "Starting Client"
echo ""
echo "Client             Server"
build/Client/deliver 127.0.0.1 $PORT << 'EOF'
ftp Twice.jpg
EOF

# Verify that it works
diff $FILENAME build/Server/$FILENAME