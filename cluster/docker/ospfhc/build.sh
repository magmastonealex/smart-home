VERSION=1.0.1
echo "Building container for $VERSION"

docker build . -t docker.svcs.alexroth.me/anycaster:$VERSION
