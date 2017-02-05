PORT=5001

# CLOSE TESTING PROGRAMS
pkill server

# TESTING PART

# Starts the server
echo "Starting Server"
build/Server/server $PORT &
sleep .5

# Starts the client
echo "Starting Client"
build/Client/deliver ug56@eecg.utoronto.ca $PORT << 'EOF'
ftp Twice.jpg
EOF