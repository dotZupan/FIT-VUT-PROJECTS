APP = ipk-l4-scan
DOTNET = dotnet
RUNTIME ?= linux-x64

.PHONY: build publish clean

all: publish

build:
    $(DOTNET) build

publish: build
    $(DOTNET) publish -c Release \
        -r $(RUNTIME) \
        --self-contained true \
        -p:PublishSingleFile=true \
        -o out
    cp out/$(APP)* .

clean:
    rm -rf ./out
    rm -f ./$(APP)
    rm -f ./*.pdb
    $(DOTNET) clean
