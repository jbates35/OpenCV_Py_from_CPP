#Get current directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Run python
echo ">> Running python test script..."
cd ./py
python3 ./fishML.py

# Get back to original directory
cd $DIR