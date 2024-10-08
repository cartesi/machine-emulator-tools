#!/bin/bash
# Modified from the original
set -e

trap 'echo >&2 Ctrl+C captured, exiting; exit 1' SIGINT

image="$1"; shift
platform="$1"; shift

docker buildx build --platform $platform --load --pull -t repo-info:local-dpkg -f Dockerfile.local-dpkg . 1>&2

name="repo-info-local-$$-$RANDOM"
trap "docker rm -vf '$name-data' > /dev/null || :" EXIT

docker create \
    --platform $platform \
	--name "$name-data" \
	-v /etc \
	-v /lib/apk \
	-v /usr/lib/rpm \
	-v /usr/share/apk \
	-v /usr/share/doc \
	-v /var/lib \
	"$image" \
	bogus > /dev/null

echo '# `'"$image"'`'

size="$(
	docker inspect -f '{{ .VirtualSize }}' "$image" | awk '{
		oneKb = 1000;
		oneMb = 1000 * oneKb;
		oneGb = 1000 * oneMb;
		if ($1 >= oneGb) {
			printf "~ %.2f Gb", $1 / oneGb
		} else if ($1 >= oneMb) {
			printf "~ %.2f Mb", $1 / oneMb
		} else if ($1 >= oneKb) {
			printf "~ %.2f Kb", $1 / oneKb
		} else {
			printf "%d bytes", $1
		}
	}'
)"

docker inspect -f '
## Docker Metadata

- Image ID: `{{ .Id }}`
- Created: `{{ .Created }}`
- Virtual Size: '"$size"'
  (total size of all layers on-disk)
- Arch: `{{ .Os }}`/`{{ .Architecture }}`
{{ if .Config.Entrypoint }}- Entrypoint: `{{ json .Config.Entrypoint }}`
{{ end }}{{ if .Config.Cmd }}- Command: `{{ json .Config.Cmd }}`
{{ end }}- Environment:{{ range .Config.Env }}{{ "\n" }}  - `{{ . }}`{{ end }}{{ if .Config.Labels }}
- Labels:{{ range $k, $v := .Config.Labels }}{{ "\n" }}  - `{{ $k }}={{ $v }}`{{ end }}{{ end }}' "$image"

docker run --platform $platform --rm --volumes-from "$name-data" -v /etc/ssl repo-info:local-dpkg || :
