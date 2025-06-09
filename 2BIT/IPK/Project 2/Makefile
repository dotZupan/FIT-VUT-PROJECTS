OUT_DIR=./bin/Release/net9.0
EXECUTABLE=ipk25chat-client
PROJECT_FILE=./ipk25chat-client.csproj

all:
	dotnet publish $(PROJECT_FILE) \
		-c Release \
		-r linux-x64 \
		--self-contained true \
		/p:PublishSingleFile=true \
		-o $(OUT_DIR)

	cp $(OUT_DIR)/$(EXECUTABLE) .

clean:
	rm -rf $(OUT_DIR) ./$(EXECUTABLE) ./obj

run:
	./$(EXECUTABLE)
