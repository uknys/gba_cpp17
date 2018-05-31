Vagrant.configure("2") do |config|
  config.vm.box = "archlinux/archlinux"
 
  config.vm.provision "shell", inline: <<-SHELL
    sudo pacman-key --recv F7FD5492264BB9D0
    sudo pacman-key --lsign F7FD5492264BB9D0

    sudo cat <<EOT >> /etc/pacman.conf
[dkp-libs]
Server = http://downloads.devkitpro.org/packages

[dkp-linux]
Server = http://downloads.devkitpro.org/packages/linux

EOT

    sudo pacman -U https://downloads.devkitpro.org/devkitpro-keyring-r1.787e015-2-any.pkg.tar.xz --noconfirm
    sudo pacman -Syu --noconfirm

    sudo pacman -S ninja devkitARM --noconfirm

    echo 'export DEVKITPRO=/opt/devkitpro' >> /home/vagrant/.bash_profile
    echo 'export DEVKITARM=$DEVKITPRO/devkitARM' >> /home/vagrant/.bash_profile
    echo 'export PATH=$DEVKITARM/bin:$PATH' >> /home/vagrant/.bash_profile
  SHELL
end
