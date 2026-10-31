SHELL := cmd.exe
.SHELLFLAGS := /c

CC      := clang
WINDRES := windres
CFLAGS  := -std=c11 -O2 -Wall -Wextra
LDFLAGS := -mwindows -s

SRC := src/main.c
RC  := src/resource.rc
RES := bin/resource.o
OUT := bin/keyboard.exe

.PHONY: target all run kill clean
.DEFAULT_GOAL := target

all: $(OUT) ## Build bin/keyboard.exe

target: SHELL := sh
target: .SHELLFLAGS := -c
target:
	@printf "Use 'make \033[0;36mtarget\033[0m' where \033[0;36mtarget\033[0m is one of the following:\n\n"
	@awk -F ':|##' '/^[^\t].+?:.*?##/ {t[++c]=$$1; d[c]=$$NF; type[c]=1; if(length($$1)>m) m=length($$1)} /^##@/ {type[++c]=0; text[c]=substr($$0, 5)} END {for(i=1;i<=c;i++) if(type[i]==1) printf "  \033[0;36m%-*s\033[0m %s\n", m, t[i], d[i]; else {if(h++) printf "\n"; printf "\033[1m%s\033[0m\n", text[i]}}' $(MAKEFILE_LIST)

$(OUT): $(SRC) $(RES) src/resource.h
	$(CC) $(CFLAGS) $(SRC) $(RES) -o $@ $(LDFLAGS)

$(RES): $(RC) src/resource.h res/icon.ico
	@if not exist bin mkdir bin
	$(WINDRES) $(RC) -o $@

run: kill $(OUT) ## Stop, rebuild and start it
	@start "" "$(subst /,\,$(OUT))"

kill: ## Stop the running keyboard.exe
	@taskkill /IM keyboard.exe >nul 2>&1 || exit 0
	@powershell -NoProfile -Command "Wait-Process -Name keyboard -Timeout 5 -ErrorAction SilentlyContinue; exit 0"
	@taskkill /F /IM keyboard.exe >nul 2>&1 || exit 0

clean: ## Remove build output
	@if exist bin rmdir /s /q bin
