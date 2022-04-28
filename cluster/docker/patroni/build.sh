VERSION=1.0.4
echo "Building container for $VERSION"

docker build . -t docker.svcs.alexroth.me/postgres-patroni:$VERSION
docker push docker.svcs.alexroth.me/postgres-patroni:$VERSION
