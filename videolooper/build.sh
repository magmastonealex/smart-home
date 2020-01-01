VERSION=1.0.2
echo "Building container for $VERSION"
CGO_ENABLED=0 go build -tags netgo

docker build . -t aroth-videolooper:$VERSION
