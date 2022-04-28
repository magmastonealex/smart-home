VERSION=1.0.5
echo "Building container for $VERSION"

docker build . -t docker.svcs.alexroth.me/borg-backup:$VERSION
docker push docker.svcs.alexroth.me/borg-backup:$VERSION
