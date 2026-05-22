rm -f ./bin/* &&
echo -e "\n> BUILDING FOR DESKTOP:\n" &&
./scripts/build-desktop.sh &&
echo -e "\n> BUILDING FOR WEB:\n"
./scripts/build-web.sh
