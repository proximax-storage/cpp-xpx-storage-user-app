# Running Storage User App in Docker

## Prerequisite
- Docker

## Build Docker Image

`docker build -t cpp-xpx-so-user-app:1-bullseye -f Dockerfile . `

## Run Docker Image

### Start interactive shell in Docker Image
```
docker run -it --rm -v /tmp/.X11-unix/:/tmp/.X11-unix/  --env DISPLAY=unix$DISPLAY --net="host" -v ~/.Xauthority:/dotXauthority  sirius-so-app:1-jammy
```

### Run Storage Client App
`./StorageClientApp`

If you encounter *Authorization* problem, run the following to merge host authorization to the one in docker:
`xauth merge /dotXauthority`