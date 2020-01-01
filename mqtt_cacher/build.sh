VERSION=1.0.2
echo "Building container for $VERSION"
docker run --rm --mount type=bind,src=$(pwd),dst=/wd -w /wd -e CGO_ENABLED=1 -it aroth-gobuild:1.0.0 go build

docker build . -t aroth-mqttheartbeat:$VERSION
