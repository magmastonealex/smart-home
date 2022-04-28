VERSION=1.0.8
echo "Building container for $VERSION"

docker build . -t docker.svcs.alexroth.me/pgback:$VERSION
docker push docker.svcs.alexroth.me/pgback:$VERSION
