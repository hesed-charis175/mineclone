# Mineclone

A voxel-based game core


---

## Setup

```bash
# 1. Clone the repository
git clone https://github.com
cd mineclone

# 2. Generate the texture atlas
pip install --break-system-packages pillow
mkdir assets
python3 tools/generate_atlas.py

# 3. Build the project
mkdir build
cd build
cmake ..
make -j
cd ..

# 4. Run the game
./build/minecraft_clone

```
