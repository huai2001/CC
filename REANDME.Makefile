$ gcc --version

make path
#Debug and Release
#debug=1,0 

#Debug
make .a platform=linux debug=1
#Release
make .a platform=linux

make .dylib platfrom=linux target=widgets all=1 debug=1

#dynamiclib
make .dylib platform=osx
make .so platform=linux

#$ Windows
proj.Win/libcc.vcxproj

#$ Android
cd /CC/proj.Android/JNI
./build_NDK9.sh
./build_NDK11.sh

#Debug
$NDK/ndk-build NDK_DEBUG=1
#Release
$NDK/ndk-build

#Unicode_Debug
$NDK/ndk-build NDK_DEBUG=1 NDK_UNICODE=1

#Unicode_Release
$NDK/ndk-build NDK_UNICODE=1


#Install MySQL8 devel
#Centos
wget https://repo.mysql.com//mysql80-community-release-el7-7.noarch.rpm
rpm -ivh mysql80-community-release-el7-7.noarch.rpm

yum -y install mysql-devel