SHELL := /bin/bash

R36S_IMAGE := hoerspiel-player-r36s
R36S_CONTAINER := hoerspiel-r36s
R36S_DIST := dist-r36s

GPM2804_DOCKERFILE := Dockerfile.gpm2804-batocera
GPM2804_DIST := dist-batocera

UPDATE_BASE_URL ?=

.PHONY: all r36s gpm2804 updatepackage check-update-base-url clean-r36s clean-gpm2804 clean

all: check-update-base-url r36s gpm2804
	@UPDATE_BASE_URL="$(UPDATE_BASE_URL)" bash ./scripts/create_update_package.sh

r36s:
	rm -rf ./$(R36S_DIST)
	docker build --progress=plain --platform linux/arm64 \
		--target export \
		--output type=local,dest=./$(R36S_DIST) \
		.
	@echo
	@echo "R36S fertig: ./$(R36S_DIST)"

gpm2804:
	rm -rf ./$(GPM2804_DIST)
	docker build --progress=plain --platform linux/arm64 \
		-f $(GPM2804_DOCKERFILE) \
		--target export \
		--output type=local,dest=./$(GPM2804_DIST) \
		.
	@echo
	@echo "GPM2804/Batocera fertig: ./$(GPM2804_DIST)"

check-update-base-url:
	@if [[ -z "$(strip $(UPDATE_BASE_URL))" ]]; then \
		echo "Fehler: UPDATE_BASE_URL ist verpflichtend."; \
		echo "Aufruf: make all UPDATE_BASE_URL=https://DEIN-SERVER"; \
		exit 1; \
	fi

updatepackage: check-update-base-url r36s gpm2804
	@UPDATE_BASE_URL="$(UPDATE_BASE_URL)" bash ./scripts/create_update_package.sh

clean-r36s:
	-docker rm -f $(R36S_CONTAINER)
	-docker image rm $(R36S_IMAGE)
	rm -rf ./$(R36S_DIST)

clean-gpm2804:
	rm -rf ./$(GPM2804_DIST)

clean: clean-r36s clean-gpm2804
