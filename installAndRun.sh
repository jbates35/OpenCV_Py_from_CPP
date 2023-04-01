# Store current directory in variable
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Build and run
echo ">> Building program..."
cd build
cmake ..
make
echo ">> Running program..."
./ocvPyCpp

# Get back to original directory
cd $DIR