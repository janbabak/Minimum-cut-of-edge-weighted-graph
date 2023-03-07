# Open MP sandbox
Sandbox for running C, C++ project with openMP dependency in docker.

## Build container
```bash
docker build -t openmpsandbox .
```

## Run container
```bash
docker run -it -v `pwd`:/work openmpsandbox 
```