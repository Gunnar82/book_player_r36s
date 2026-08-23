SHELL := /bin/bash

R36S_IMAGE := hoerspiel-player-r36s
R36S_CONTAINER := hoerspiel-r36s

GPM2804_DOCKERFILE := Dockerfile.gpm2804-batocera
GPM2804_DIST := dist-batocera

.PHONY: r36s gpm2804 clean-r36s clean-gpm2804 clean

r36s:
	docker build --progress=plain --platform linux/arm64 -t $(R36S_IMAGE) .
	-docker rm -f $(R36S_CONTAINER) >/dev/null 2>&1
	docker create --platform linux/arm64 --name $(R36S_CONTAINER) $(R36S_IMAGE) /bin/true
	docker cp $(R36S_CONTAINER):/build/hoerspiel_player ./hoerspiel_player
	docker rm $(R36S_CONTAINER)
	@echo
	@echo "R36S fertig: ./hoerspiel_player"

gpm2804:
	rm -rf ./$(GPM2804_DIST)
	docker build --progress=plain --platform linux/arm64 \
		-f $(GPM2804_DOCKERFILE) \
		--target export \
		--output type=local,dest=./$(GPM2804_DIST) \
		.
	@echo
	@echo "GPM2804/Batocera fertig: ./$(GPM2804_DIST)"

clean-r36s:
	-docker rm -f $(R36S_CONTAINER)
	-docker image rm $(R36S_IMAGE)
	rm -f ./hoerspiel_player

clean-gpm2804:
	rm -rf ./$(GPM2804_DIST)

clean: clean-r36s clean-gpm2804
