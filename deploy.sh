#! /usr/bin/env sh

if [ "$#" -ne 2 ];
then
  printf "Usage: ./deploy.sh exe directory\n"
fi

list=$(ntldd -R $1 | grep \mingw64 | sed 's/.dll.*/.dll/')
for dll in $list;
do
  pkg=`pacman -Qo $dll | sed 's/.* is owned by //' | tr ' ' '-'`
  pkglist="$pkglist $pkg"
done
printf "$pkglist out"
# remove duplicates
pkglist=`echo $pkglist | tr ' ' '\n' | sort | uniq`
printf "$pkglist\n"

mkdir -p "$2"

for pkg in $pkglist
do
  tmp=`mktemp -d`
  cd $tmp
  printf "ooo $pkg\n"
  tar -xf /c/msys64/var/cache/pacman/pkg/$pkg-any.pkg.tar.xz
  # more fine-grained control is possible here
  cp -r $PWD/mingw64/bin/*.dll $2
  cp -r $PWD/mingw64/share/*.dll $2
  cd ..
  rm -r $tmp
done