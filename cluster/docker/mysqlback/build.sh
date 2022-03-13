VERSION=1.0.4
echo "Building container for $VERSION"

docker build . -t docker.svcs.alexroth.me/mysql-backup:$VERSION
