VERSION=1.0.6
echo "Building container for $VERSION"

docker build . -t docker.svcs.alexroth.me/mysql-backup:$VERSION
docker push docker.svcs.alexroth.me/mysql-backup:$VERSION
