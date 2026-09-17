APP := $(notdir $(CURDIR))

all r ram flash jflash:
	$(MAKE) -C ../../ APP=$(APP) $@

clean:
	$(MAKE) -C ../../ APP=$(APP) $@

.PHONY: all r ram flash jflash clean
