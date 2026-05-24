
#change current directory to this file
SCRIPT_PATH="$(dirname "$0")"
cd "$SCRIPT_PATH" || exit 1

#dl link: https://github.com/libsdl-org/SDL_mixer/archive/refs/tags/release-2.8.0.zip
#unzip to this folder, then run this .sh
       # cuz SDL_mixer need download deps manually
       # you could Run the "download.sh" script in the "SDL_mixer-release-2.8.0/external" folder
               # or just run this .sh is same effect of the "download.sh"

# sh SDL_mixer-release-2.8.0/external/download.sh

#---------------------------------
# "SDL_mixer-release-2.8.0/external/download.sh" may only run once, this script allow run multiple times.
MY_SDL2_MIXER_ROOT="SDL_mixer-release-2.8.0"

if [ ! -d "$MY_SDL2_MIXER_ROOT" ]; then
    echo "Directory $MY_SDL2_MIXER_ROOT not found!"
    exit 1
fi
cd "$MY_SDL2_MIXER_ROOT" || exit 1

step=0

# Read .gitmodules line by line, preserving whitespace initially
while IFS= read -r line || [ -n "$line" ]; do
    # Skip section headers like [submodule "external/FLAC"]
    if [[ "$line" == *\[* ]]; then
        continue
    fi

    # Check if the line contains an equals sign
    if [[ "$line" == *=* ]]; then
        # Extract the value after '=' and trim whitespace using xargs
        value=$(echo "$line" | cut -d'=' -f2- | xargs)
    else
        continue
    fi

    ((step++))

    if [ "$step" -eq 1 ]; then
        repo_path="$value"
    elif [ "$step" -eq 2 ]; then
        repo_url="$value"
    elif [ "$step" -eq 3 ]; then
        repo_branch="$value"
        
        if [ ! -d "$repo_path/.git" ]; then
            if [ -d "$repo_path" ]; then
                rm -rf "$repo_path"
            fi
            echo "Cloning $repo_path ($repo_branch)..."
            git clone --recursive "$repo_url" "$repo_path" -b "$repo_branch"
        else
            echo "Skipping $repo_path - already exists"
        fi
        step=0
    fi
done < .gitmodules

echo "All $MY_SDL2_MIXER_ROOT external modules cloned successfully!!"
