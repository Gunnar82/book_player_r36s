SHELL := /bin/bash

.PHONY: r36s gpm2804 clean-r36s clean-gpm2804

R36S_IMAGE := hoerspiel-player-r36s
R36S_CONTAINER := hoerspiel-r36s

BATOCERA_DOCKERFILE := Dockerfile.gpm2804-batocera
BATOCERA_DIST := dist-batocera

r36s:
	docker build --platform linux/arm64 -t $(R36S_IMAGE) .
	-docker rm -f $(R36S_CONTAINER) >/dev/null 2>&1
	docker create --platform linux/arm64 --name $(R36S_CONTAINER) $(R36S_IMAGE) /bin/true
	docker cp $(R36S_CONTAINER):/build/hoerspiel_player ./hoerspiel_player
	mkdir -p ./lib
	docker cp $(R36S_CONTAINER):/usr/lib/aarch64-linux-gnu/libqrencode.so.4 ./lib/libqrencode.so.4
	docker rm $(R36S_CONTAINER)
	@echo
	@echo "R36S fertig: ./hoerspiel_player"

gpm2804:
	rm -rf ./$(BATOCERA_DIST)
	docker build --platform linux/arm64 \
		-f $(BATOCERA_DOCKERFILE) \
		--target export \
		--output type=local,dest=./$(BATOCERA_DIST) \
		.
	@echo
	@echo "GPM2804/Batocera fertig: ./$(BATOCERA_DIST)"

clean-r36s:
	-docker rm -f $(R36S_CONTAINER)
	-docker image rm $(R36S_IMAGE)
	rm -f ./hoerspiel_player

clean-gpm2804:
	rm -rf ./$(BATOCERA_DIST)
