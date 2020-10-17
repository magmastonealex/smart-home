VERSION=1.0.0
echo "Building container for $VERSION"

docker build . -t aroth-doorlocked:$VERSION
