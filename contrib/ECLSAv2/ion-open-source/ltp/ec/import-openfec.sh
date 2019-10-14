#!/bin/bash
#Author: Nicola Alessi  (nicola.alessi@studio.unibo.it)
#Project supervisor: Carlo Caini (carlo.caini@unibo.it)

downloadURL="http://openfec.org/files/openfec_v1_4_2.tgz"
openfecFolder="openfec"

if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi


filename=$(ls -1 | egrep "^openfec.*(tgz|zip)$")


if [ "$filename" == "" ]
then
	echo "Openfec not found."
	echo "Do you want to download from internet? [Y/N]"
	read answer
	if [ "${answer,,}" == "y" ] || [ "${answare,,}" == "yes" ] #case insensitive check
	then 
		wget "$downloadURL"

		filename=$(ls -1 | egrep "^openfec.*(tgz|zip)$")
		if [ "$filename" == "" ]
			then
			echo "Something went wrong with file downloading. Aborting..."
			exit
		fi	
		echo "Download completed!"
		chmod 0777 "$filename"
	
	else
		echo "aborting..."
		exit
	fi
fi
echo "Extracting $filename ..."
rm -fr "$openfecFolder"
foldername=$(tar -xvf "$filename" | sed -n 1p)
mv "$foldername" "$openfecFolder"
echo "Extraction completed"
echo "Generating makefile for openfec"

	echo -e "all:" > "$openfecFolder"/Makefile
	echo -e "\t(cd build_debug/; cmake .. -DDEBUG:STRING=ON ; make)" >> "$openfecFolder"/Makefile
	echo -e "install:" >> "$openfecFolder"/Makefile
	echo -e "\t(cd build_debug/; cmake .. -DDEBUG:STRING=ON ; make)" >> "$openfecFolder"/Makefile
	echo -e "\tcp -d bin/Debug/libopenfec.so* /usr/lib/" >> "$openfecFolder"/Makefile
	echo -e "clean:" >> "$openfecFolder"/Makefile
	echo -e "\t(cd build_debug/; make clean)" >> "$openfecFolder"/Makefile
	echo -e "\trm -f /usr/lib/libopenfec.so*" >> "$openfecFolder"/Makefile
	echo -e "\trm -rf build_debug/*" >> "$openfecFolder"/Makefile

#this bug causes the ML failures using a 32 bit architectures
patch -p0 < 32bit_bugfix.patch 
chmod 0777 -R "$openfecFolder"

echo "Done! You should run 'make install' command within openfec folder"

echo "Do you want to run 'make install' now? [Y/N]"
read answer
	if [ "${answer,,}" == "y" ] || [ "${answare,,}" == "yes" ] #case insensitive check
	then 
	cd "$openfecFolder"
	make install
	fi

cd ..
chmod 0777 -R "$openfecFolder"
