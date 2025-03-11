wget https://apt.devkitpro.org/install-devkitpro-pacman
chmod +x install-devkitpro-pacman 
sudo ./install-devkitpro-pacman 
rm ./install-devkitpro-pacman
sudo dkp-pacman -S $(cat requirements.txt)
sudo apt install -y doxygen

