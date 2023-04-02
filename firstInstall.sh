# Store current directory in variable
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

echo ">> Installing dependencies for program..."

# Update and upgrade
apt update && apt upgrade -y

# Get coral set up - see https://coral.ai/docs/accelerator/get-started/
echo ">> Installing pycoral..."
echo "deb https://packages.cloud.google.com/apt coral-edgetpu-stable main" | sudo tee /etc/apt/sources.list.d/coral-edgetpu.list
curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | sudo apt-key add -
apt-get install -y libedgetpu1-std
apt-get install -y python3-pycoral

# Install dependencies for program
echo ">> Installing python dependencies..."
apt install -y python3-dev python3-pip 
python3 -m pip install opencv-python==4.5.5.64
python3 -m pip install opencv-contrib-python==4.5.5.64 

# More dependencies
apt-get -y install libgl1-mesa-glx

# Make build folder and build program
echo ">> Building program..."
cd $DIR
mkdir build
cp -R $DIR/py/* $DIR/build
cd build
cmake ..
make

chmod +x $DIR/*.sh