VERSION=1.0.6
echo "Building container for $VERSION"
CGO_ENABLED=0 go build -tags netgo

docker build . -t aroth-einkserver:$VERSION
